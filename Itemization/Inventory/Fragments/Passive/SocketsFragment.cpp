#include "Itemization/Inventory/Fragments/Passive/SocketsFragment.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Itemization/Affixes/MythicAffixApplicationComponent.h"
#include "Itemization/Affixes/MythicAffixDefinition.h"
#include "Itemization/Affixes/MythicAffixRng.h"
#include "Itemization/Affixes/MythicItemizationDataRegistrySubsystem.h"
#include "Itemization/Affixes/MythicTags_Affixes.h"
#include "Itemization/Inventory/Fragments/Passive/AffixesFragment.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Mythic/Mythic.h"
#include "Net/UnrealNetwork.h"

namespace MythicSocketSerialization {
const FGuid ReplicatedSocketArrayMagic(0x4D59534F, 0x434B4554, 0x41525231, 0xB41F027D);
}

namespace {
bool SerializeSocketTag(FArchive &Ar, FGameplayTag &Tag, const bool bRequired) {
    FString TagText = Ar.IsSaving() && Tag.IsValid() ? Tag.ToString() : FString();
    const bool bSerialized = MythicFragmentSerialization::SerializeBoundedUtf8(
        Ar, TagText, MythicFragmentSerialization::MaxIdentityStringBytes, bRequired);
    if (!bSerialized) return false;

    if (Ar.IsLoading()) {
        const FName TagName = TagText.IsEmpty() ? NAME_None : FName(*TagText);
        Tag = TagName.IsNone() ? FGameplayTag() : FGameplayTag::RequestGameplayTag(TagName, false);
        if ((!TagText.IsEmpty() && (!Tag.IsValid() || Tag.ToString() != TagText))
            || (bRequired && !Tag.IsValid())) {
            Ar.SetError();
        }
    }
    else if (bRequired && !Tag.IsValid()) {
        Ar.SetError();
    }
    return !Ar.IsError();
}

void NotifySocketOwner(UObject *Owner) {
    if (USocketsFragment *Fragment = Cast<USocketsFragment>(Owner)) {
        Fragment->OnSocketStatesReplicated();
    }
}

}

bool FMythicReplicatedSocketItem::NetSerialize(FArchive &Ar, UPackageMap *Map, bool &bOutSuccess) {
    bOutSuccess = true;
    Ar << SocketGuid;

    auto NetTag = [&Ar, Map, &bOutSuccess](FGameplayTag &Tag) {
        bool bLocalSuccess = true;
        Tag.NetSerialize(Ar, Map, bLocalSuccess);
        bOutSuccess &= bLocalSuccess;
    };
    NetTag(SocketColor);

    Ar.SerializeBits(&bFilled, 1);
    if (bFilled) {
        NetTag(SocketedGemType);
        Ar << SourceGemItemGuid;
    }
    else if (Ar.IsLoading()) {
        SocketedGemType = FGameplayTag();
        SourceGemItemGuid.Invalidate();
    }

    if (Ar.IsSaving() && SocketedAffixSnapshots.Num() > MythicSocketSerialization::MaxAffixesPerSocket) {
        bOutSuccess = false;
        return true;
    }
    uint8 Count = Ar.IsSaving() ? static_cast<uint8>(SocketedAffixSnapshots.Num()) : 0;
    Ar.SerializeBits(&Count, 7);
    if (Count > MythicSocketSerialization::MaxAffixesPerSocket) {
        bOutSuccess = false;
        return true;
    }
    if (Ar.IsLoading()) {
        SocketedAffixSnapshots.SetNum(Count);
    }
    for (FRolledAffix &Snapshot : SocketedAffixSnapshots) {
        bool bSnapshotSuccess = true;
        Snapshot.NetSerialize(Ar, Map, bSnapshotSuccess);
        bOutSuccess &= bSnapshotSuccess;
    }

    bOutSuccess &= SocketGuid.IsValid();
    bOutSuccess &= !bFilled || (SocketedGemType.IsValid() && SourceGemItemGuid.IsValid() && Count > 0
        && FMythicSocketMath::IsGemCompatible(SocketedGemType, SocketColor));
    bOutSuccess &= bFilled || (!SocketedGemType.IsValid() && !SourceGemItemGuid.IsValid() && Count == 0);
    return true;
}

bool FMythicReplicatedSocketArray::Serialize(FArchive &Ar) {
    if (!Ar.IsSaveGame()) {
        return false;
    }

    if (Ar.IsSaving()) {
        FGuid Magic = MythicSocketSerialization::ReplicatedSocketArrayMagic;
        int32 Version = MythicSocketSerialization::ReplicatedSocketArrayVersion;
        Ar << Magic;
        Ar << Version;
    }
    else {
        FGuid Magic;
        int32 Version = 0;
        Ar << Magic;
        Ar << Version;
        if (Ar.IsError() || Magic != MythicSocketSerialization::ReplicatedSocketArrayMagic
            || Version != MythicSocketSerialization::ReplicatedSocketArrayVersion) {
            Ar.SetError();
            return true;
        }
    }

    int32 Count = Ar.IsSaving() ? Items.Num() : 0;
    Ar << Count;
    if (Count < 0 || Count > MythicSocketSerialization::MaxSocketsPerItem) {
        Ar.SetError();
        return true;
    }
    TArray<FMythicReplicatedSocketItem> StagedItems;
    TArray<FMythicReplicatedSocketItem> &Rows = Ar.IsLoading() ? StagedItems : Items;
    if (Ar.IsLoading()) StagedItems.SetNum(Count);

    TSet<FGuid> SocketGuids;
    for (FMythicReplicatedSocketItem &Item : Rows) {
        Ar << Item.SocketGuid;
        if (!SerializeSocketTag(Ar, Item.SocketColor, false)
            || !SerializeSocketTag(Ar, Item.SocketedGemType, false)) {
            return true;
        }
        Ar << Item.SourceGemItemGuid;
        Ar << Item.bFilled;

        int32 SnapshotCount = Ar.IsSaving() ? Item.SocketedAffixSnapshots.Num() : 0;
        Ar << SnapshotCount;
        if (SnapshotCount < 0 || SnapshotCount > MythicSocketSerialization::MaxAffixesPerSocket) {
            Ar.SetError();
            return true;
        }
        if (Ar.IsLoading()) {
            Item.SocketedAffixSnapshots.SetNum(SnapshotCount);
        }
        for (FRolledAffix &Snapshot : Item.SocketedAffixSnapshots) {
            Snapshot.Serialize(Ar);
            if (Ar.IsError()) {
                return true;
            }
        }

        if (!Item.SocketGuid.IsValid() || SocketGuids.Contains(Item.SocketGuid)
            || (Item.bFilled && (!Item.SocketedGemType.IsValid() || !Item.SourceGemItemGuid.IsValid()
                                 || Item.SocketedAffixSnapshots.IsEmpty()
                                 || !FMythicSocketMath::IsGemCompatible(
                                     Item.SocketedGemType, Item.SocketColor)))
            || (!Item.bFilled && (Item.SocketedGemType.IsValid() || Item.SourceGemItemGuid.IsValid()
                                  || !Item.SocketedAffixSnapshots.IsEmpty()))) {
            Ar.SetError();
            return true;
        }
        SocketGuids.Add(Item.SocketGuid);
        if (Ar.IsLoading()) {
            Item.ReplicationID = INDEX_NONE;
            Item.ReplicationKey = INDEX_NONE;
            Item.MostRecentArrayReplicationKey = INDEX_NONE;
        }
    }
    if (Ar.IsLoading()) {
        Items = MoveTemp(StagedItems);
        ArrayReplicationKey = 0;
        IDCounter = 0;
    }
    return true;
}

void FMythicReplicatedSocketArray::PostReplicatedAdd(const TArrayView<int32> &Added, int32 FinalSize) {
    NotifySocketOwner(Owner.Get());
}

void FMythicReplicatedSocketArray::PostReplicatedChange(const TArrayView<int32> &Changed, int32 FinalSize) {
    NotifySocketOwner(Owner.Get());
}

void FMythicReplicatedSocketArray::PreReplicatedRemove(const TArrayView<int32> &Removed, int32 FinalSize) {
    NotifySocketOwner(Owner.Get());
}

void USocketsFragment::Serialize(FArchive &Ar) {
    Super::Serialize(Ar);
    // Package saves must be observational; SaveGame loads do not receive PostLoad and restore the callback here.
    if (Ar.IsLoading() && Ar.IsSaveGame()) {
        SocketStates.SetOwner(this);
    }
}

void USocketsFragment::PostLoad() {
    Super::PostLoad();
    SocketStates.SetOwner(this);
    if (!HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)) {
        RequestRuntimeData();
    }
}

void USocketsFragment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(ThisClass, SocketStates, COND_OwnerOnly);
}

bool USocketsFragment::HasServerAuthority() const {
    const AActor *Owner = GetOwningActor();
    return Owner && Owner->HasAuthority();
}

UMythicItemizationDataRegistrySubsystem *USocketsFragment::ResolveRegistry() const {
    const UWorld *World = GetWorld();
    const UGameInstance *GameInstance = World ? World->GetGameInstance() : nullptr;
    return GameInstance ? GameInstance->GetSubsystem<UMythicItemizationDataRegistrySubsystem>() : nullptr;
}

UMythicAffixApplicationComponent *USocketsFragment::ResolveApplicationComponent(
    UMythicItemInstance *ItemInstance) const {
    AActor *InventoryOwner = ItemInstance ? ItemInstance->GetInventoryOwner() : nullptr;
    auto FindOnActor = [](AActor *Actor) -> UMythicAffixApplicationComponent * {
        return Actor ? Actor->FindComponentByClass<UMythicAffixApplicationComponent>() : nullptr;
    };
    if (UMythicAffixApplicationComponent *Direct = FindOnActor(InventoryOwner)) {
        return Direct;
    }
    if (APawn *Pawn = Cast<APawn>(InventoryOwner)) {
        if (UMythicAffixApplicationComponent *OnState = FindOnActor(Pawn->GetPlayerState())) {
            return OnState;
        }
        if (AController *Controller = Pawn->GetController()) {
            if (UMythicAffixApplicationComponent *OnController = FindOnActor(Controller)) {
                return OnController;
            }
            if (UMythicAffixApplicationComponent *OnState =
                    FindOnActor(Controller->GetPlayerState<APlayerState>())) {
                return OnState;
            }
        }
    }
    if (AController *Controller = Cast<AController>(InventoryOwner)) {
        if (UMythicAffixApplicationComponent *OnState =
                FindOnActor(Controller->GetPlayerState<APlayerState>())) {
            return OnState;
        }
    }
    return FindOnActor(InventoryOwner ? InventoryOwner->GetOwner() : nullptr);
}

FGuid USocketsFragment::DeriveSocketAffixRollGuid(const FGuid &HostItemGuid, const FGuid &SocketGuid,
                                                   const FGuid &SourceGemItemGuid,
                                                   const FGuid &SourceRollGuid) {
    if (!HostItemGuid.IsValid() || !SocketGuid.IsValid() || !SourceGemItemGuid.IsValid()
        || !SourceRollGuid.IsValid()) {
        return FGuid();
    }
    FMythicAffixCanonicalWriter Fields("MYTHIC_AFFIX_SOCKET_ROLL_FIELDS_V2");
    Fields.AddGuid(HostItemGuid);
    Fields.AddGuid(SocketGuid);
    Fields.AddGuid(SourceGemItemGuid);
    Fields.AddGuid(SourceRollGuid);
    return Fields.IsValid()
               ? FMythicAffixRngFactory::GuidFromCanonicalBytes("Mythic.Affix.Socket.Roll.V2", Fields.GetBytes())
               : FGuid();
}

void USocketsFragment::OnInstanced(UMythicItemInstance *Instance) {
    Super::OnInstanced(Instance);
    SocketStates.SetOwner(this);
    if (!Instance || !HasServerAuthority()) {
        return;
    }
    if (!SocketStates.Items.IsEmpty()) {
        RequestRuntimeData();
        return;
    }

    const UItemDefinition *Definition = Instance->GetItemDefinition();
    if (!Definition) {
        return;
    }
    const FMythicSocketCountTable &Table = CountTableOverride.Rules.IsEmpty()
                                                ? FMythicSocketMath::DefaultSocketCountTable()
                                                : CountTableOverride;
    const int32 Count = FMythicSocketMath::RollSocketCount(
        Definition->ItemType, Instance->GetItemLevel(), static_cast<int32>(Definition->Rarity.GetValue()),
        Table, FMath::FRand());
    if (Count < 0 || Count > MythicSocketSerialization::MaxSocketsPerItem) {
        bRuntimeDataFailed = true;
        UE_LOG(Myth, Error, TEXT("Socket count %d exceeds the bounded runtime schema on %s."), Count,
               *GetNameSafe(Instance));
        return;
    }

    SocketStates.Items.Reset(Count);
    for (int32 Index = 0; Index < Count; ++Index) {
        FMythicReplicatedSocketItem &Socket = SocketStates.Items.AddDefaulted_GetRef();
        Socket.SocketGuid = FGuid::NewGuid();
        Socket.SocketColor = RolledSocketColor;
        if (!Socket.SocketGuid.IsValid()) {
            SocketStates.Items.Reset();
            bRuntimeDataFailed = true;
            return;
        }
    }
    SocketStates.MarkArrayDirty();
    bRuntimeDataReady = true;
    RefreshOwningInventoryPresentation();
}

void USocketsFragment::OnInventorySlotChanged(UMythicInventoryComponent *NewInventory, int32 NewSlot) {
    Super::OnInventorySlotChanged(NewInventory, NewSlot);
    SocketStates.SetOwner(this);
    RequestRuntimeData();
}

void USocketsFragment::RequestRuntimeData() {
    SocketStates.SetOwner(this);
    if (bRuntimeDataFailed || bRuntimeRequestInFlight) {
        return;
    }
    if (bRuntimeDataReady) {
        return;
    }

    UMythicItemizationDataRegistrySubsystem *Registry = ResolveRegistry();
    if (!Registry) {
        return;
    }
    bRuntimeRequestInFlight = true;
    RequestCanonicalSnapshotClosure(++RuntimeDataRevision);
}

void USocketsFragment::RequestCanonicalSnapshotClosure(const uint32 RequestRevision) {
    if (RequestRevision != RuntimeDataRevision) return;
    UMythicItemizationDataRegistrySubsystem *Registry = ResolveRegistry();
    if (!Registry) {
        bRuntimeRequestInFlight = false;
        return;
    }

    if (!Registry->IsCoreSemanticReady()) {
        TWeakObjectPtr<USocketsFragment> WeakThis(this);
        Registry->RequestCoreSemanticDataAsync(
            FOnMythicItemizationDataReady::CreateLambda([WeakThis, RequestRevision](const bool bReady) {
                if (!WeakThis.IsValid()) return;
                if (!bReady) {
                    WeakThis->HandleCanonicalClosureReady(false, RequestRevision);
                    return;
                }
                WeakThis->RequestCanonicalSnapshotClosure(RequestRevision);
            }));
        return;
    }

    TSet<FGuid> SocketGuids;
    TSet<FGuid> RollGuids;
    const UMythicItemInstance *Host = GetOwningItemInstance();
    for (const FMythicReplicatedSocketItem &Socket : SocketStates.Items) {
        if (!Socket.SocketGuid.IsValid() || SocketGuids.Contains(Socket.SocketGuid)
            || (Socket.bFilled && (!Socket.SocketedGemType.IsValid()
                                   || !Socket.SourceGemItemGuid.IsValid()
                                   || Socket.SocketedAffixSnapshots.IsEmpty()
                                   || !FMythicSocketMath::IsGemCompatible(
                                       Socket.SocketedGemType, Socket.SocketColor)))
            || (!Socket.bFilled && (Socket.SocketedGemType.IsValid()
                                    || Socket.SourceGemItemGuid.IsValid()
                                    || !Socket.SocketedAffixSnapshots.IsEmpty()))
            || Socket.SocketedAffixSnapshots.Num() > MythicSocketSerialization::MaxAffixesPerSocket) {
            HandleCanonicalClosureReady(false, RequestRevision);
            return;
        }
        SocketGuids.Add(Socket.SocketGuid);
        for (const FRolledAffix &Snapshot : Socket.SocketedAffixSnapshots) {
            const UMythicAffixDefinition *Definition = Registry->FindAffix(
                Snapshot.AffixDefinition.GetPrimaryAssetId());
            if (!Snapshot.IsGameplayValid() || !Snapshot.AffixDefinition.IsValid()
                || Snapshot.TierRank <= 0 || !Snapshot.bIsLocked
                || !Host || !Definition || !Definition->TargetStat.IsValid()
                || (MythicAffix::ModifierRequiresNonZeroMagnitude(Definition->ModifierOp)
                    && FMath::IsNearlyZero(Snapshot.Magnitude))
                || Snapshot.Provenance.SourceKind != AFFIX_SOURCE_SOCKET
                || Snapshot.Provenance.OriginSocketGuid != Socket.SocketGuid
                || Snapshot.Provenance.SourceItemGuid != Socket.SourceGemItemGuid
                || RollGuids.Contains(Snapshot.RollGuid)) {
                HandleCanonicalClosureReady(false, RequestRevision);
                return;
            }
            RollGuids.Add(Snapshot.RollGuid);
        }
    }
    HandleCanonicalClosureReady(true, RequestRevision);
}

void USocketsFragment::HandleCanonicalClosureReady(const bool bSuccess,
                                                    const uint32 RequestRevision) {
    if (RequestRevision != RuntimeDataRevision) return;
    bRuntimeRequestInFlight = false;
    bRuntimeDataReady = bSuccess;
    if (!bSuccess) {
        bRuntimeDataFailed = true;
        UE_LOG(Myth, Error, TEXT("Socket fragment %s failed its typed affix-definition closure."), *GetName());
        return;
    }
    // The core Affix/Stat Definition closure is now resident. Inventory notification drives one authoritative whole-equipment
    // transaction; this fragment never performs activation-time per-snapshot application.
    RefreshOwningInventoryPresentation();
}

bool USocketsFragment::BuildSocketCandidates(const FGuid &SocketGuid, const FGuid &SourceGemItemGuid,
                                              TConstArrayView<FRolledAffix> GemSnapshots,
                                              TArray<FRolledAffix> &OutCandidates) const {
    OutCandidates.Reset();
    const UMythicItemInstance *Host = GetOwningItemInstance();
    if (!Host || !Host->GetItemInstanceGuid().IsValid() || !SocketGuid.IsValid()
        || !SourceGemItemGuid.IsValid() || GemSnapshots.IsEmpty()
        || GemSnapshots.Num() > MythicSocketSerialization::MaxAffixesPerSocket) {
        return false;
    }
    const UMythicItemizationDataRegistrySubsystem *Registry = ResolveRegistry();
    if (!Registry || !Registry->IsCoreSemanticReady()) {
        return false;
    }

    TSet<FGuid> RollGuids;
    OutCandidates.Reserve(GemSnapshots.Num());
    for (const FRolledAffix &Source : GemSnapshots) {
        const UMythicAffixDefinition *Definition = Registry->FindAffix(
            Source.AffixDefinition.GetPrimaryAssetId());
        if (!Source.IsGameplayValid() || Source.Provenance.SourceKind != AFFIX_SOURCE_GEM
            || Source.Provenance.SourceItemGuid != SourceGemItemGuid
            || Source.Provenance.OriginSocketGuid.IsValid()
            || !Source.AffixDefinition.IsValid() || Source.TierRank <= 0 || !Source.bIsLocked
            || !Definition || !Definition->TargetStat.IsValid()
            || (MythicAffix::ModifierRequiresNonZeroMagnitude(Definition->ModifierOp)
                && FMath::IsNearlyZero(Source.Magnitude))) {
            OutCandidates.Reset();
            return false;
        }

        FRolledAffix Candidate = Source;
        Candidate.RollGuid = DeriveSocketAffixRollGuid(
            Host->GetItemInstanceGuid(), SocketGuid, SourceGemItemGuid, Source.RollGuid);
        Candidate.Provenance.SourceItemGuid = SourceGemItemGuid;
        Candidate.Provenance.OriginSocketGuid = SocketGuid;
        Candidate.Provenance.SourceKind = AFFIX_SOURCE_SOCKET;
        Candidate.bIsLocked = true;
        if (!Candidate.IsGameplayValid() || RollGuids.Contains(Candidate.RollGuid)) {
            OutCandidates.Reset();
            return false;
        }
        RollGuids.Add(Candidate.RollGuid);
        OutCandidates.Add(MoveTemp(Candidate));
    }
    return true;
}

bool USocketsFragment::HasHostRollGuidCollision(const TSet<FGuid> &CandidateRollGuids) const {
    for (const FMythicReplicatedSocketItem &Socket : SocketStates.Items) {
        for (const FRolledAffix &Snapshot : Socket.SocketedAffixSnapshots) {
            if (CandidateRollGuids.Contains(Snapshot.RollGuid)) {
                return true;
            }
        }
    }
    if (UMythicItemInstance *Host = GetOwningItemInstance()) {
        if (const UAffixesFragment *Affixes = Host->GetFragment<UAffixesFragment>()) {
            for (const FMythicReplicatedAffixItem &Item : Affixes->GetAffixSnapshots().Items) {
                if (CandidateRollGuids.Contains(Item.Affix.RollGuid)) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool USocketsFragment::ApplySnapshotsCommitLast(TConstArrayView<FRolledAffix> Snapshots,
                                                 UMythicAffixApplicationComponent &Application) {
    UMythicItemInstance *Host = GetOwningItemInstance();
    if (!Host || !HasServerAuthority()) {
        return false;
    }

    for (const FRolledAffix &Snapshot : Snapshots) {
        if (!Snapshot.IsGameplayValid() || Snapshot.Provenance.SourceKind != AFFIX_SOURCE_SOCKET
            || !Snapshot.Provenance.SourceItemGuid.IsValid() || !Snapshot.Provenance.OriginSocketGuid.IsValid()
            || Application.IsRegistered(Snapshot.RollGuid)) {
            return false;
        }
    }
    return Application.ApplySnapshotsTransactional(Host, Snapshots);
}

bool USocketsFragment::RemoveSnapshotsCommitLast(TConstArrayView<FRolledAffix> Snapshots,
                                                  UMythicAffixApplicationComponent &Application) {
    UMythicItemInstance *Host = GetOwningItemInstance();
    if (!Host || !HasServerAuthority()) {
        return false;
    }

    for (const FRolledAffix &Snapshot : Snapshots) {
        if (!Application.IsRegistered(Snapshot.RollGuid)) {
            return false;
        }
    }
    return Application.RemoveSnapshotsTransactional(Host, Snapshots);
}

bool USocketsFragment::ServerSocketGem(const int32 SocketIndex, const FGameplayTag &GemType,
                                        const FGuid &SourceGemItemGuid,
                                        TConstArrayView<FRolledAffix> GemAffixSnapshots) {
    if (!HasServerAuthority() || !SocketStates.Items.IsValidIndex(SocketIndex) || !GemType.IsValid()
        || !SourceGemItemGuid.IsValid() || GemAffixSnapshots.IsEmpty()
        || bRuntimeDataFailed || !bRuntimeDataReady) {
        return false;
    }
    FMythicReplicatedSocketItem &Socket = SocketStates.Items[SocketIndex];
    if (Socket.bFilled || !FMythicSocketMath::IsGemCompatible(GemType, Socket.SocketColor)) {
        return false;
    }

    TArray<FRolledAffix> Candidates;
    if (!BuildSocketCandidates(Socket.SocketGuid, SourceGemItemGuid, GemAffixSnapshots, Candidates)) {
        return false;
    }
    TSet<FGuid> CandidateRollGuids;
    for (const FRolledAffix &Candidate : Candidates) {
        CandidateRollGuids.Add(Candidate.RollGuid);
    }
    if (CandidateRollGuids.Num() != Candidates.Num() || HasHostRollGuidCollision(CandidateRollGuids)) {
        return false;
    }

    if (bItemActive) {
        UMythicAffixApplicationComponent *Application = ActiveApplicationComponent;
        if (!Application) {
            Application = ResolveApplicationComponent(GetOwningItemInstance());
        }
        if (!Application || !ApplySnapshotsCommitLast(Candidates, *Application)) {
            return false;
        }
        ActiveApplicationComponent = Application;
    }

    // Commit replicated/save state only after the active permanent-stat transaction succeeds.
    Socket.SocketedGemType = GemType;
    Socket.SourceGemItemGuid = SourceGemItemGuid;
    Socket.SocketedAffixSnapshots = MoveTemp(Candidates);
    Socket.bFilled = true;
    SocketStates.MarkItemDirty(Socket);
    RefreshOwningInventoryPresentation();
    return true;
}

FGameplayTag USocketsFragment::ServerUnsocketGem(const int32 SocketIndex) {
    if (!HasServerAuthority() || !SocketStates.Items.IsValidIndex(SocketIndex)
        || bRuntimeDataFailed || !bRuntimeDataReady) {
        return FGameplayTag();
    }
    FMythicReplicatedSocketItem &Socket = SocketStates.Items[SocketIndex];
    if (!Socket.bFilled) {
        return FGameplayTag();
    }
    if (bItemActive) {
        UMythicAffixApplicationComponent *Application = ActiveApplicationComponent;
        if (!Application) {
            Application = ResolveApplicationComponent(GetOwningItemInstance());
        }
        if (!Application || !RemoveSnapshotsCommitLast(Socket.SocketedAffixSnapshots, *Application)) {
            return FGameplayTag();
        }
        ActiveApplicationComponent = Application;
    }

    const FGameplayTag RemovedGemType = Socket.SocketedGemType;
    Socket.SocketedGemType = FGameplayTag();
    Socket.SourceGemItemGuid.Invalidate();
    Socket.SocketedAffixSnapshots.Reset();
    Socket.bFilled = false;
    SocketStates.MarkItemDirty(Socket);
    RefreshOwningInventoryPresentation();
    return RemovedGemType;
}

bool USocketsFragment::ServerAddSocket() {
    if (!HasServerAuthority() || bRuntimeDataFailed || !bRuntimeDataReady) {
        return false;
    }
    const FMythicSocketCountTable &Table = CountTableOverride.Rules.IsEmpty()
                                                ? FMythicSocketMath::DefaultSocketCountTable()
                                                : CountTableOverride;
    const int32 HardCap = FMath::Clamp(Table.HardCap, 0, MythicSocketSerialization::MaxSocketsPerItem);
    if (SocketStates.Items.Num() >= HardCap) {
        return false;
    }

    FMythicReplicatedSocketItem &Socket = SocketStates.Items.AddDefaulted_GetRef();
    Socket.SocketGuid = FGuid::NewGuid();
    Socket.SocketColor = RolledSocketColor;
    if (!Socket.SocketGuid.IsValid()) {
        SocketStates.Items.Pop(EAllowShrinking::No);
        return false;
    }
    SocketStates.MarkItemDirty(Socket);
    RefreshOwningInventoryPresentation();
    return true;
}

int32 USocketsFragment::GetSocketCount() const {
    return SocketStates.Items.Num();
}

int32 USocketsFragment::GetFilledSocketCount() const {
    int32 Count = 0;
    for (const FMythicReplicatedSocketItem &Socket : SocketStates.Items) {
        Count += Socket.bFilled ? 1 : 0;
    }
    return Count;
}

const FMythicReplicatedSocketItem *USocketsFragment::GetSocketState(const int32 SocketIndex) const {
    return SocketStates.Items.IsValidIndex(SocketIndex) ? &SocketStates.Items[SocketIndex] : nullptr;
}

FGameplayTag USocketsFragment::GetSocketColor(const int32 SocketIndex) const {
    if (const FMythicReplicatedSocketItem *Socket = GetSocketState(SocketIndex)) {
        return Socket->SocketColor;
    }
    return FGameplayTag();
}

FGameplayTag USocketsFragment::GetSocketedGemType(const int32 SocketIndex) const {
    if (const FMythicReplicatedSocketItem *Socket = GetSocketState(SocketIndex)) {
        return Socket->SocketedGemType;
    }
    return FGameplayTag();
}

bool USocketsFragment::IsSocketFilled(const int32 SocketIndex) const {
    if (const FMythicReplicatedSocketItem *Socket = GetSocketState(SocketIndex)) {
        return Socket->bFilled;
    }
    return false;
}

void USocketsFragment::OnItemActivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemActivated(ItemInstance);
    if (!HasServerAuthority()) {
        return;
    }
    bItemActive = true;
    RequestRuntimeData();
}

void USocketsFragment::OnItemDeactivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemDeactivated(ItemInstance);
    bItemActive = false;
    ActiveApplicationComponent = nullptr;
}

bool USocketsFragment::CanBeStackedWith(const UItemFragment *Other) const {
    if (!Super::CanBeStackedWith(Other)) {
        return false;
    }
    const USocketsFragment *OtherSockets = Cast<USocketsFragment>(Other);
    if (!OtherSockets || GetSocketCount() != OtherSockets->GetSocketCount()
        || GetFilledSocketCount() > 0 || OtherSockets->GetFilledSocketCount() > 0) {
        return false;
    }
    for (int32 Index = 0; Index < GetSocketCount(); ++Index) {
        if (GetSocketColor(Index) != OtherSockets->GetSocketColor(Index)) {
            return false;
        }
    }
    return true;
}

void USocketsFragment::OnSocketStatesReplicated() {
    SocketStates.SetOwner(this);
    ++RuntimeDataRevision;
    bRuntimeRequestInFlight = false;
    bRuntimeDataReady = false;
    bRuntimeDataFailed = false;
    RequestRuntimeData();
    RefreshOwningInventoryPresentation();
}

void USocketsFragment::RefreshOwningInventoryPresentation() const {
    if (UMythicItemInstance *Item = GetOwningItemInstance()) {
        if (UMythicInventoryComponent *Inventory = Item->GetInventoryComponent()) {
            Inventory->NotifyItemInstanceUpdated(Item->GetSlot());
        }
    }
}

#if WITH_EDITOR
bool USocketsFragment::IsValidFragment(FText &OutErrorMessage) const {
    const FMythicSocketCountTable &Table = CountTableOverride.Rules.IsEmpty()
                                                ? FMythicSocketMath::DefaultSocketCountTable()
                                                : CountTableOverride;
    if (Table.HardCap < 0 || Table.HardCap > MythicSocketSerialization::MaxSocketsPerItem
        || Table.ItemLevelsPerSocket < 1) {
        OutErrorMessage = FText::FromString(
            TEXT("Socket count table exceeds the bounded schema or has an invalid level cadence."));
        return false;
    }
    for (const FMythicSocketCountRule &Rule : Table.Rules) {
        int32 Previous = 0;
        if (!Rule.ItemTypeParent.IsValid()) {
            OutErrorMessage = FText::FromString(TEXT("Every socket-count rule requires an ItemType parent."));
            return false;
        }
        for (const int32 Value : Rule.MaxByRarity) {
            if (Value < Previous || Value < 0 || Value > Table.HardCap) {
                OutErrorMessage = FText::FromString(
                    TEXT("Socket caps must be bounded and non-decreasing by rarity."));
                return false;
            }
            Previous = Value;
        }
    }
    return true;
}
#endif
