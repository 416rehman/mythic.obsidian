

#include "MythicWorldItem.h"

#include "Mythic.h"
#include "NavigationSystem.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Affixes/MythicItemizationHash.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/InventoryProviderInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"


class UNavigationSystemV1;

namespace {
const FGuid WorldItemFrameMagic(0x4D59574C, 0x44495445, 0x4D465231, 0x8A931C57);
constexpr int32 WorldItemFrameVersion = 1;
constexpr int32 MaxWorldItemClassPathBytes = 4096;
constexpr int32 MaxWorldItemStableIdBytes = 4096;
constexpr int32 MaxWorldItemPayloadBytes = 16 * 1024 * 1024;
constexpr int32 MaxWorldItemCustomDataBytes = MaxWorldItemPayloadBytes
    + MaxWorldItemClassPathBytes + MaxWorldItemStableIdBytes + 256;

bool SerializeWorldItemBoundedUtf8(FArchive &Ar, FString &Value, const int32 MaxBytes) {
    int32 ByteCount = 0;
    TArray<uint8> Bytes;
    if (Ar.IsSaving()) {
        const FTCHARToUTF8 Utf8(*Value);
        ByteCount = Utf8.Length();
        if (ByteCount <= 0 || ByteCount > MaxBytes) {
            Ar.SetError();
            return false;
        }
        Ar << ByteCount;
        if (!Ar.IsError()) Ar.Serialize(const_cast<ANSICHAR *>(Utf8.Get()), ByteCount);
        return !Ar.IsError();
    }

    Ar << ByteCount;
    const int64 Remaining = Ar.TotalSize() >= 0 ? Ar.TotalSize() - Ar.Tell() : ByteCount;
    if (Ar.IsError() || ByteCount <= 0 || ByteCount > MaxBytes || Remaining < ByteCount) {
        Ar.SetError();
        return false;
    }
    Bytes.SetNumUninitialized(ByteCount + 1);
    Ar.Serialize(Bytes.GetData(), ByteCount);
    if (Ar.IsError()) return false;
    for (int32 Index = 0; Index < ByteCount; ++Index) {
        if (Bytes[Index] == 0) {
            Ar.SetError();
            return false;
        }
    }
    Bytes[ByteCount] = 0;
    const FUTF8ToTCHAR Converted(
        reinterpret_cast<const ANSICHAR *>(Bytes.GetData()), ByteCount);
    Value = FString(Converted.Length(), Converted.Get());
    const FTCHARToUTF8 RoundTrip(*Value);
    if (Value.IsEmpty() || RoundTrip.Length() != ByteCount
        || FMemory::Memcmp(RoundTrip.Get(), Bytes.GetData(), ByteCount) != 0) {
        Ar.SetError();
        return false;
    }
    return true;
}

bool ComputePayloadHash(const TArray<uint8> &Payload, FSHA256Signature &OutHash) {
    return !Payload.IsEmpty() && Payload.Num() <= MaxWorldItemPayloadBytes
        && MythicItemizationHash::Sha256(MakeArrayView(Payload), OutHash);
}

UClass *ResolveLoadedItemClass(const FString &ClassPathString) {
    const FSoftClassPath ClassPath(ClassPathString);
    UClass *ItemClass = ClassPath.IsValid() ? ClassPath.ResolveClass() : nullptr;
    if (!ItemClass || !ItemClass->IsChildOf(UMythicItemInstance::StaticClass())
        || ItemClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists)
        || FSoftClassPath(ItemClass).ToString() != ClassPathString) {
        return nullptr;
    }
    return ItemClass;
}

bool DeserializeItemPayload(AMythicWorldItem &Owner,
                            const FString &ClassPathString,
                            const TArray<uint8> &Payload,
                            UMythicItemInstance *&OutItem,
                            FName &OutDiagnostic) {
    OutItem = nullptr;
    if (Payload.IsEmpty() || Payload.Num() > MaxWorldItemPayloadBytes) {
        OutDiagnostic = TEXT("InvalidItemPayloadSize");
        return false;
    }

    UClass *ItemClass = ResolveLoadedItemClass(ClassPathString);
    if (!ItemClass) {
        // Save restore is not an asset-loading hot path. The world/itemization closure must be prewarmed first.
        OutDiagnostic = TEXT("ItemClassNotLoadedOrInvalid");
        return false;
    }

    UMythicItemInstance *StagedItem = NewObject<UMythicItemInstance>(&Owner, ItemClass);
    if (!StagedItem) {
        OutDiagnostic = TEXT("ItemAllocationFailed");
        return false;
    }

    FMemoryReader PayloadReader(Payload, true);
    PayloadReader.ArMaxSerializeSize = MaxWorldItemPayloadBytes;
    // Save games are not network archives, but this flag engages UE's generic TArray allocation ceiling while
    // retaining the same binary layout. Item fragments apply their own tighter, semantic count/byte limits.
    PayloadReader.ArIsNetArchive = true;
    FObjectAndNameAsStringProxyArchive ItemArchive(PayloadReader, false);
    ItemArchive.ArIsSaveGame = true;
    ItemArchive.ArMaxSerializeSize = MaxWorldItemPayloadBytes;
    ItemArchive.ArIsNetArchive = true;
    StagedItem->Serialize(ItemArchive);
    if (ItemArchive.IsError() || !PayloadReader.AtEnd() || !StagedItem->GetItemDefinition()) {
        OutDiagnostic = TEXT("CorruptTruncatedOrUnloadedItemPayload");
        StagedItem->MarkAsGarbage();
        return false;
    }

    if (!StagedItem->GetItemInstanceGuid().IsValid()) {
        OutDiagnostic = TEXT("MissingCurrentItemInstanceGuid");
        StagedItem->MarkAsGarbage();
        return false;
    }

    OutItem = StagedItem;
    return true;
}
}

void AMythicWorldItem::PostInitializeComponents() {
    Super::PostInitializeComponents();

    if (HasAuthority() && !HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)
        && !WorldItemSaveGuid.IsValid()) {
        WorldItemSaveGuid = FGuid::NewGuid();
    }
}

void AMythicWorldItem::BeginPlay() {
    Super::BeginPlay();

    if (HasAuthority() && !WorldItemSaveGuid.IsValid()) {
        WorldItemSaveGuid = FGuid::NewGuid();
    }

    if (HasAuthority() && IsValid(StaticMesh)) {
        StaticMesh->SetGenerateOverlapEvents(true);
        StaticMesh->OnComponentBeginOverlap.AddUniqueDynamic(this, &AMythicWorldItem::OnPickupOverlap);
    }
}

void AMythicWorldItem::OnPickupOverlap(UPrimitiveComponent *OverlappedComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp,
                                       int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult) {
    if (!HasAuthority() || !bAutoPickup || !ItemInstance || IsActorBeingDestroyed()) {
        return;
    }
    if (StaticMesh && StaticMesh->IsSimulatingPhysics()) {
        return;
    }

    const UItemDefinition *Def = ItemInstance->GetItemDefinition();
    if (!Def || !ShouldAutoPickup(Def->StackSizeMax)) {
        return;
    }

    const APawn *Pawn = Cast<APawn>(OtherActor);
    if (!Pawn) {
        return;
    }
    AController *PawnController = Pawn->GetController();
    if (!PawnController || (TargetRecipient && TargetRecipient != PawnController)) {
        return;
    }

    IInventoryProviderInterface *Provider = Cast<IInventoryProviderInterface>(PawnController);
    if (!Provider) {
        return;
    }
    if (UMythicInventoryComponent *Inventory = Provider->GetInventoryForWorldItem(this)) {
        Inventory->PickupItem(this);
    }
}

AMythicWorldItem::AMythicWorldItem() {
    this->bReplicates = true;
    this->bReplicateUsingRegisteredSubObjectList = true;

    StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));

    RootComponent = StaticMesh;

    this->SetActorEnableCollision(true);
    StaticMesh->SetSimulatePhysics(false);
    StaticMesh->SetCollisionProfileName(TEXT("OverlapAll"));

    SetNetCullDistanceSquared(FMath::Square(8000.f));
}

void AMythicWorldItem::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    Super::EndPlay(EndPlayReason);

    UE_LOG(Myth, Verbose, TEXT("AMythicWorldItem::EndPlay: %s"), *GetName());
}

void AMythicWorldItem::SetItemInstance(UMythicItemInstance *ItemInst) {
    checkf(HasAuthority(), TEXT("AMythicWorldItem::SetItemInstance: Only call this on the server"));
    if (!ItemInst || !ItemInst->GetItemDefinition() || !ItemInst->GetItemInstanceGuid().IsValid()) {
        UE_LOG(Myth, Error,
               TEXT("AMythicWorldItem::SetItemInstance: current item, definition, and stable GUID are required"));
        return;
    }

    ItemInst->SetOwner(this);
    this->ItemInstance = ItemInst;

    OnRep_ItemInstance();
}

void AMythicWorldItem::OnRep_ItemInstance() {
    UE_LOG(Myth, Verbose, TEXT("AMythicWorldItem::OnRep_ItemInstance: %s"), *GetName());
    OnItemInstanceUpdated();
}

void AMythicWorldItem::OnHit(UPrimitiveComponent *HitComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp, FVector NormalImpulse,
                             const FHitResult &Hit) {
    if (Hit.ImpactPoint.Z < GetActorLocation().Z || Hit.Normal.Z > 0.5f) {
        UE_LOG(Myth, Verbose, TEXT("AMythicWorldItem::OnHit: %s"), *GetName());
        StaticMesh->SetSimulatePhysics(false);
        StaticMesh->SetEnableGravity(false);
        StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        StaticMesh->SetCollisionResponseToAllChannels(ECR_Overlap);

        SetNetDormancy(DORM_DormantAll);
    }
}

void AMythicWorldItem::EmulateDropPhysics(const FVector &location, float radius) {
    FVector TargetLoc = location;
    FNavLocation RandomizedLocation;
    UNavigationSystemV1 *NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (NavSys && NavSys->GetRandomPointInNavigableRadius(location, radius, RandomizedLocation, nullptr)) {
        TargetLoc = RandomizedLocation.Location;
        UE_LOG(Myth, Warning, TEXT("Randomized location in %f radius from %s to %s"), radius, *location.ToString(), *RandomizedLocation.Location.ToString());
    }

    FVector SuggestedVelocity;
    UGameplayStatics::SuggestProjectileVelocity_CustomArc(
        GetWorld(),
        SuggestedVelocity,
        location,
        TargetLoc);

    this->StaticMesh->SetCollisionResponseToAllChannels(ECR_Block);
    this->StaticMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    this->StaticMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    this->StaticMesh->SetSimulatePhysics(true);
    this->StaticMesh->SetEnableGravity(true);
    this->StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    this->StaticMesh->SetPhysicsLinearVelocity(SuggestedVelocity);

    this->StaticMesh->SetNotifyRigidBodyCollision(true);
    this->StaticMesh->OnComponentHit.AddDynamic(this, &AMythicWorldItem::OnHit);
}

void AMythicWorldItem::OnRep_TargetRecipient() {
    if (TargetRecipient) {
        APlayerController *OwnerPlayerController = Cast<APlayerController>(GetOwner());
        if (!OwnerPlayerController) {
            UE_LOG(Myth, Warning, TEXT("AMythicWorldItem::OnRep_ForAll: Owner is not a player controller"));
            return;
        }


        auto isLocalPlayer = OwnerPlayerController->IsLocalController();
        SetActorHiddenInGame(!isLocalPlayer);
    }
    else {
        SetActorHiddenInGame(false);
    }
}

void AMythicWorldItem::SetTargetRecipient(AController *NewTargetRecipient) {
    checkf(HasAuthority(), TEXT("AMythicWorldItem::SetIsPrivate: Only call this on the server"));

    this->TargetRecipient = NewTargetRecipient;
    OnRep_TargetRecipient();
}

FString AMythicWorldItem::GetSaveableActorId() const {
    // Package-loaded/placed actors already have a stable path. Runtime actor names are allocation-order dependent,
    // so their persisted GUID is the only cross-session identity suitable for world-save reconciliation.
    if (!HasAnyFlags(RF_WasLoaded) && WorldItemSaveGuid.IsValid()) {
        return FString::Printf(TEXT("world-item/%s"),
                               *WorldItemSaveGuid.ToString(EGuidFormats::DigitsWithHyphensLower));
    }
    return GetPathName();
}

void AMythicWorldItem::SerializeCustomData(TArray<uint8> &OutCustomData) {
    OutCustomData.Reset();
    if (!HasAuthority() || !ItemInstance || !ItemInstance->GetItemDefinition()
        || !ItemInstance->GetItemInstanceGuid().IsValid()) {
        UE_LOG(MythSaveLoad, Error,
               TEXT("World item save rejected [%s]: authority, definition and stable item identity are required"),
               *GetNameSafe(this));
        return;
    }

    TArray<uint8> ItemPayload;
    FMemoryWriter ItemWriter(ItemPayload, true);
    FObjectAndNameAsStringProxyArchive ItemArchive(ItemWriter, false);
    ItemArchive.ArIsSaveGame = true;
    ItemInstance->Serialize(ItemArchive);
    if (ItemArchive.IsError() || ItemPayload.IsEmpty()
        || ItemPayload.Num() > MaxWorldItemPayloadBytes) {
        UE_LOG(MythSaveLoad, Error,
               TEXT("World item save rejected [%s]: nested item payload failed or exceeded %d bytes"),
               *GetNameSafe(this), MaxWorldItemPayloadBytes);
        return;
    }

    if (!WorldItemSaveGuid.IsValid()) WorldItemSaveGuid = FGuid::NewGuid();
    FString StableContainerId = GetSaveableActorId();
    FString ItemClassPath = FSoftClassPath(ItemInstance->GetClass()).ToString();
    FSHA256Signature PayloadHash{};
    if (!WorldItemSaveGuid.IsValid() || !ComputePayloadHash(ItemPayload, PayloadHash)) {
        UE_LOG(MythSaveLoad, Error, TEXT("World item save rejected [%s]: identity/hash creation failed"),
               *GetNameSafe(this));
        return;
    }

    TArray<uint8> StagedCustomData;
    FMemoryWriter FrameWriter(StagedCustomData, true);
    FGuid Magic = WorldItemFrameMagic;
    int32 Version = WorldItemFrameVersion;
    int32 PayloadSize = ItemPayload.Num();
    FrameWriter << Magic;
    FrameWriter << Version;
    FrameWriter << WorldItemSaveGuid;
    if (!SerializeWorldItemBoundedUtf8(FrameWriter, StableContainerId, MaxWorldItemStableIdBytes)
        || !SerializeWorldItemBoundedUtf8(FrameWriter, ItemClassPath, MaxWorldItemClassPathBytes)) {
        UE_LOG(MythSaveLoad, Error,
               TEXT("World item save rejected [%s]: invalid or unbounded save identity/item class path"),
               *GetNameSafe(this));
        return;
    }
    FrameWriter << PayloadSize;
    FrameWriter.Serialize(PayloadHash.Signature, UE_ARRAY_COUNT(PayloadHash.Signature));
    FrameWriter.Serialize(ItemPayload.GetData(), ItemPayload.Num());
    if (FrameWriter.IsError() || StagedCustomData.Num() > MaxWorldItemCustomDataBytes) {
        UE_LOG(MythSaveLoad, Error, TEXT("World item save rejected [%s]: outer frame write failed"),
               *GetNameSafe(this));
        return;
    }

    OutCustomData = MoveTemp(StagedCustomData);
}

bool AMythicWorldItem::TryDeserializeCustomData(const TArray<uint8> &InCustomData) {
    if (!HasAuthority() || InCustomData.IsEmpty()
        || InCustomData.Num() > MaxWorldItemCustomDataBytes) {
        UE_LOG(MythSaveLoad, Error,
               TEXT("World item restore rejected [%s]: unauthorized, empty or oversized outer payload"),
               *GetNameSafe(this));
        return false;
    }

    FString ItemClassPath;
    TArray<uint8> ItemPayload;
    FGuid RestoredSaveGuid;
    FString StableContainerId;

    FMemoryReader FrameReader(InCustomData, true);
    FGuid Magic;
    FrameReader << Magic;
    if (FrameReader.IsError()) {
        UE_LOG(MythSaveLoad, Error, TEXT("World item restore rejected [%s]: truncated outer marker"),
               *GetNameSafe(this));
        return false;
    }

    if (Magic != WorldItemFrameMagic) {
        UE_LOG(MythSaveLoad, Error,
               TEXT("World item restore rejected [%s]: unsupported save frame"),
               *GetNameSafe(this));
        return false;
    }
    {
        int32 Version = 0;
        int32 PayloadSize = 0;
        FSHA256Signature SavedHash{};
        FrameReader << Version;
        FrameReader << RestoredSaveGuid;
        if (FrameReader.IsError() || Version != WorldItemFrameVersion
            || !RestoredSaveGuid.IsValid()
            || !SerializeWorldItemBoundedUtf8(FrameReader, StableContainerId,
                                               MaxWorldItemStableIdBytes)
            || !SerializeWorldItemBoundedUtf8(FrameReader, ItemClassPath, MaxWorldItemClassPathBytes)) {
            UE_LOG(MythSaveLoad, Error,
                   TEXT("World item restore rejected [%s]: invalid/unsupported framed header version %d"),
                   *GetNameSafe(this), Version);
            return false;
        }
        const FString ExpectedStableContainerId = HasAnyFlags(RF_WasLoaded)
            ? GetPathName()
            : FString::Printf(TEXT("world-item/%s"),
                              *RestoredSaveGuid.ToString(EGuidFormats::DigitsWithHyphensLower));
        if (StableContainerId != ExpectedStableContainerId) {
            UE_LOG(MythSaveLoad, Error,
                   TEXT("World item restore rejected [%s]: save identity does not match this container"),
                   *GetNameSafe(this));
            return false;
        }

        FrameReader << PayloadSize;
        FrameReader.Serialize(SavedHash.Signature, UE_ARRAY_COUNT(SavedHash.Signature));
        const int64 Remaining = FrameReader.TotalSize() - FrameReader.Tell();
        if (FrameReader.IsError() || PayloadSize <= 0 || PayloadSize > MaxWorldItemPayloadBytes
            || Remaining != PayloadSize) {
            UE_LOG(MythSaveLoad, Error,
                   TEXT("World item restore rejected [%s]: invalid/truncated framed payload size %d"),
                   *GetNameSafe(this), PayloadSize);
            return false;
        }

        ItemPayload.SetNumUninitialized(PayloadSize);
        FrameReader.Serialize(ItemPayload.GetData(), PayloadSize);
        FSHA256Signature ActualHash{};
        if (FrameReader.IsError() || !FrameReader.AtEnd()
            || !ComputePayloadHash(ItemPayload, ActualHash)
            || FMemory::Memcmp(SavedHash.Signature, ActualHash.Signature,
                               UE_ARRAY_COUNT(SavedHash.Signature)) != 0) {
            UE_LOG(MythSaveLoad, Error,
                   TEXT("World item restore rejected [%s]: nested item checksum mismatch"),
                   *GetNameSafe(this));
            return false;
        }
    }

    UMythicItemInstance *StagedItem = nullptr;
    FName Diagnostic = NAME_None;
    if (!DeserializeItemPayload(*this, ItemClassPath, ItemPayload, StagedItem, Diagnostic)) {
        UE_LOG(MythSaveLoad, Error, TEXT("World item restore rejected [%s]: %s"),
               *GetNameSafe(this), *Diagnostic.ToString());
        return false;
    }

    return CommitStagedRestore(StagedItem, RestoredSaveGuid);
}

bool AMythicWorldItem::CommitStagedRestore(
    UMythicItemInstance *StagedItem, const FGuid &RestoredSaveGuid) {
    if (!StagedItem || !RestoredSaveGuid.IsValid()
        || !StagedItem->GetItemDefinition() || !StagedItem->GetItemInstanceGuid().IsValid()) {
        if (StagedItem) StagedItem->MarkAsGarbage();
        return false;
    }

    // Nothing touches replication or presentation until the complete current-format structural graph validates.
    // Fragment/application reconciliation owns the nonblocking live Affix/Stat Definition closure afterward.
    StagedItem->SetOwner(this);
    if (StagedItem->GetOwningActor() != this) {
        StagedItem->MarkAsGarbage();
        return false;
    }

    UMythicItemInstance *PreviousItem = ItemInstance;
    ItemInstance = StagedItem;
    WorldItemSaveGuid = RestoredSaveGuid;
    if (PreviousItem && PreviousItem != StagedItem) {
        if (PreviousItem->GetOwningActor() == this) PreviousItem->Destroy();
        else PreviousItem->MarkAsGarbage();
    }
    OnRep_ItemInstance();
    return true;
}

void AMythicWorldItem::DeserializeCustomData(const TArray<uint8> &InCustomData) {
    TryDeserializeCustomData(InCustomData);
}
