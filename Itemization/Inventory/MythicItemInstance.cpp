#include "MythicItemInstance.h"

#include "MythicInventoryComponent.h"
#include "Itemization/Affixes/MythicAffixCompiler.h"
#include "Itemization/Affixes/MythicAffixGeneration.h"
#include "Itemization/Affixes/MythicAffixRng.h"
#include "Itemization/Affixes/MythicItemizationDataRegistrySubsystem.h"
#include "Itemization/Affixes/MythicTags_Affixes.h"
#include "Fragments/ItemFragment.h"
#include "Fragments/FragmentTypes.h"
#include "Fragments/Passive/AffixesFragment.h"
#include "Fragments/Passive/MythicGemFragment.h"
#include "Fragments/Passive/SocketsFragment.h"
#include "Fragments/Passive/YieldQualityFragment.h"
#include "Itemization/Loot/MythicWorldItem.h"
#include "Mythic/Mythic.h"
#include "Engine/GameInstance.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

namespace {
const FGuid ItemFragmentFrameMagic(0x4D594954, 0x454D4652, 0x41474D31, 0xB729D06C);
constexpr int32 ItemFragmentFrameVersion = 1;
constexpr int32 MaxFragmentsPerItem = 64;
constexpr int32 MaxFragmentClassPathBytes = 4096;
constexpr int32 MaxFragmentPayloadBytes = 8 * 1024 * 1024;
constexpr int64 MaxItemFragmentPayloadBytes = 16ll * 1024ll * 1024ll;

bool IsValidYieldQuality(const EMythicYieldQuality Quality) {
    return Quality == EMythicYieldQuality::Ragged
        || Quality == EMythicYieldQuality::Common
        || Quality == EMythicYieldQuality::Fine
        || Quality == EMythicYieldQuality::Pristine;
}

bool SerializeBoundedAscii(FArchive &Ar, FString &Value, const int32 MaxBytes) {
    int32 ByteCount = 0;
    TArray<uint8> Bytes;
    if (Ar.IsSaving()) {
        if (Value.IsEmpty() || !FCString::IsPureAnsi(*Value)) {
            Ar.SetError();
            return false;
        }
        const FTCHARToUTF8 Utf8(*Value);
        ByteCount = Utf8.Length();
        if (ByteCount <= 0 || ByteCount > MaxBytes) {
            Ar.SetError();
            return false;
        }
        Ar << ByteCount;
        if (Ar.IsError()) return false;
        Ar.Serialize(const_cast<ANSICHAR *>(Utf8.Get()), ByteCount);
        return !Ar.IsError();
    }

    Ar << ByteCount;
    if (Ar.IsError() || ByteCount <= 0 || ByteCount > MaxBytes) {
        Ar.SetError();
        return false;
    }
    const int64 Remaining = Ar.TotalSize() >= 0 ? Ar.TotalSize() - Ar.Tell() : ByteCount;
    if (Remaining < ByteCount) {
        Ar.SetError();
        return false;
    }
    Bytes.SetNumUninitialized(ByteCount + 1);
    Ar.Serialize(Bytes.GetData(), ByteCount);
    if (Ar.IsError()) return false;
    for (int32 Index = 0; Index < ByteCount; ++Index) {
        if (Bytes[Index] == 0 || Bytes[Index] > 0x7f) {
            Ar.SetError();
            return false;
        }
    }
    Bytes[ByteCount] = 0;
    Value = UTF8_TO_TCHAR(reinterpret_cast<const ANSICHAR *>(Bytes.GetData()));
    return !Value.IsEmpty();
}

UItemFragment *FindUnusedFragmentTemplate(UItemDefinition *Definition, UClass *FragmentClass,
                                          TArray<UItemFragment *> &UsedTemplates) {
    if (!Definition || !FragmentClass) return nullptr;
    for (UItemFragment *DefinitionFragment : Definition->Fragments) {
        if (DefinitionFragment && DefinitionFragment->GetClass() == FragmentClass
            && !UsedTemplates.Contains(DefinitionFragment)) {
            UsedTemplates.Add(DefinitionFragment);
            return DefinitionFragment;
        }
    }
    return nullptr;
}

UClass *ResolveFramedFragmentClass(UItemDefinition *Definition, const FString &ClassPathString) {
    if (ClassPathString.IsEmpty()) return nullptr;

    // The loaded item definition is the cook-safe source of its fragment classes. No frame reader sync-loads.
    if (Definition) {
        for (const UItemFragment *DefinitionFragment : Definition->Fragments) {
            if (DefinitionFragment
                && FSoftClassPath(DefinitionFragment->GetClass()).ToString() == ClassPathString) {
                UClass *FragmentClass = DefinitionFragment->GetClass();
                return FragmentClass->IsChildOf(UItemFragment::StaticClass())
                    && !FragmentClass->HasAnyClassFlags(
                        CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists)
                    ? FragmentClass : nullptr;
            }
        }
    }
    const FSoftClassPath ClassPath(ClassPathString);
    UClass *FragmentClass = ClassPath.IsValid() ? ClassPath.ResolveClass() : nullptr;
    return FragmentClass
        && FSoftClassPath(FragmentClass).ToString() == ClassPathString
        && FragmentClass->IsChildOf(UItemFragment::StaticClass())
        && !FragmentClass->HasAnyClassFlags(
            CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists)
        ? FragmentClass : nullptr;
}

uint64 DeriveFactoryAffixSeed(const FGuid &ItemGuid, const FPrimaryAssetId &ProfileId) {
    uint64 Seed = 0;
    FMythicAffixRngFactory::DeriveItemSeed(ItemGuid, ProfileId, Seed);
    return Seed;
}
}

void UMythicItemInstance::Serialize(FArchive &Ar) {
    Super::Serialize(Ar);

    if (!Ar.IsSaveGame() || Ar.IsError()) return;

    if (Ar.IsSaving()) {
        if (ItemFragments.Num() > MaxFragmentsPerItem
            || ItemFragments.ContainsByPredicate([](const UItemFragment *Fragment) { return Fragment == nullptr; })) {
            Ar.SetError();
            return;
        }

        FGuid Marker = ItemFragmentFrameMagic;
        int32 Version = ItemFragmentFrameVersion;
        int32 FragmentCount = ItemFragments.Num();
        Ar << Marker;
        Ar << Version;
        Ar << FragmentCount;
        if (Ar.IsError()) return;

        int64 TotalPayloadBytes = 0;
        for (UItemFragment *Fragment : ItemFragments) {
            FString ClassPathString = FSoftClassPath(Fragment->GetClass()).ToString();
            if (!SerializeBoundedAscii(Ar, ClassPathString, MaxFragmentClassPathBytes)) return;

            TArray<uint8> Payload;
            FMemoryWriter PayloadWriter(Payload, true);
            FObjectAndNameAsStringProxyArchive PayloadArchive(PayloadWriter, false);
            PayloadArchive.ArIsSaveGame = true;
            Fragment->Serialize(PayloadArchive);
            if (PayloadArchive.IsError() || Payload.Num() > MaxFragmentPayloadBytes) {
                Ar.SetError();
                return;
            }
            TotalPayloadBytes += Payload.Num();
            if (TotalPayloadBytes > MaxItemFragmentPayloadBytes) {
                Ar.SetError();
                return;
            }

            int32 PayloadSize = Payload.Num();
            Ar << PayloadSize;
            if (PayloadSize > 0) Ar.Serialize(Payload.GetData(), PayloadSize);
            if (Ar.IsError()) return;
        }
        return;
    }

    FGuid Marker;
    Ar << Marker;
    if (Ar.IsError()) return;
    if (Marker != ItemFragmentFrameMagic) {
        UE_LOG(MythSaveLoad, Error, TEXT("Invalid item fragment frame marker"));
        Ar.SetError();
        return;
    }

    TArray<TObjectPtr<UItemFragment>> StagedFragments;
    TArray<UItemFragment *> UsedTemplates;
    int32 Version = 0;
    int32 FragmentCount = 0;
    Ar << Version;
    if (Ar.IsError() || Version != ItemFragmentFrameVersion) {
        UE_LOG(MythSaveLoad, Error, TEXT("Unknown item fragment frame version %d"), Version);
        Ar.SetError();
        return;
    }
    Ar << FragmentCount;
    if (Ar.IsError() || FragmentCount < 0 || FragmentCount > MaxFragmentsPerItem) {
        Ar.SetError();
        return;
    }

    StagedFragments.Reserve(FragmentCount);
    int64 TotalPayloadBytes = 0;
    for (int32 Index = 0; Index < FragmentCount; ++Index) {
        FString ClassPathString;
        if (!SerializeBoundedAscii(Ar, ClassPathString, MaxFragmentClassPathBytes)) return;

        int32 PayloadSize = 0;
        Ar << PayloadSize;
        const int64 Remaining = Ar.TotalSize() >= 0 ? Ar.TotalSize() - Ar.Tell() : PayloadSize;
        TotalPayloadBytes += FMath::Max(PayloadSize, 0);
        if (Ar.IsError() || PayloadSize < 0 || PayloadSize > MaxFragmentPayloadBytes
            || TotalPayloadBytes > MaxItemFragmentPayloadBytes || Remaining < PayloadSize) {
            Ar.SetError();
            return;
        }

        TArray<uint8> Payload;
        Payload.SetNumUninitialized(PayloadSize);
        if (PayloadSize > 0) Ar.Serialize(Payload.GetData(), PayloadSize);
        if (Ar.IsError()) return;

        UClass *FragmentClass = ResolveFramedFragmentClass(ItemDefinition, ClassPathString);
        if (!FragmentClass || !FragmentClass->IsChildOf(UItemFragment::StaticClass())) {
            UE_LOG(MythSaveLoad, Error, TEXT("Missing framed item fragment class %s"), *ClassPathString);
            Ar.SetError();
            return;
        }
        UItemFragment *Template = FindUnusedFragmentTemplate(ItemDefinition, FragmentClass, UsedTemplates);
        UItemFragment *NewFragment = NewObject<UItemFragment>(
            this, FragmentClass, NAME_None, RF_NoFlags, Template);
        if (!NewFragment) {
            Ar.SetError();
            return;
        }

        FMemoryReader PayloadReader(Payload, true);
        PayloadReader.ArMaxSerializeSize = MaxFragmentPayloadBytes;
        PayloadReader.ArIsNetArchive = true;
        FObjectAndNameAsStringProxyArchive PayloadArchive(PayloadReader, true);
        PayloadArchive.ArIsSaveGame = true;
        PayloadArchive.ArMaxSerializeSize = MaxFragmentPayloadBytes;
        PayloadArchive.ArIsNetArchive = true;
        NewFragment->Serialize(PayloadArchive);
        if (PayloadArchive.IsError() || !PayloadReader.AtEnd()) {
            UE_LOG(MythSaveLoad, Error, TEXT("Corrupt payload for item fragment class %s"), *ClassPathString);
            Ar.SetError();
            return;
        }
        StagedFragments.Add(NewFragment);
    }

    // Publish only after every nested fragment archive has consumed a valid complete payload.
    for (UItemFragment *Fragment : StagedFragments) Fragment->SetOwnerItemInstance(this);
    ItemFragments = MoveTemp(StagedFragments);
}

void UMythicItemInstance::SetStackSize(const int32 newQuantity) {
    auto owner = this->GetOwningActor();
    checkf(owner->HasAuthority(), TEXT("Only the server can set the stack size of an item instance"));

    if (!ItemDefinition) {
        UE_LOG(Myth, Error, TEXT("SetStackSize: ItemDefinition is null on %s; cannot clamp stack"), *GetName());
        return;
    }

    const auto newQty = FMath::Min(newQuantity, ItemDefinition->StackSizeMax);
    if (newQty != Quantity) {
        Quantity = newQty;

        auto inventory = this->GetInventoryComponent();
        if (!inventory) {
            UE_LOG(Myth, Verbose, TEXT("SetStackSize: ItemInstance %s is not in an inventory"), *GetName());
            return;
        }
        auto slot = inventory->GetItem(this->SlotIndex);
        if (!slot) {
            UE_LOG(Myth, Verbose, TEXT("SetStackSize: ItemInstance %s is not in a valid slot"), *GetName());
            return;
        }

        if (Quantity <= 0) {
            inventory->SetItemInSlot(this->SlotIndex, nullptr);
        }

        inventory->NotifyItemInstanceUpdated(this->SlotIndex);
    }
}

int32 UMythicItemInstance::ClampInitialStackQuantity(int32 Requested, int32 StackSizeMax) {
    if (StackSizeMax <= 1) {
        return 1;
    }
    return FMath::Clamp(Requested, 1, StackSizeMax);
}

UMythicItemInstance *UMythicItemInstance::CloneForStackSplit(
    UObject *NewOuter, const int32 NewQuantity) const {
    constexpr int32 MaxSemanticCloneBytes = 32 * 1024 * 1024;
    const AActor *AuthorityOwner = GetOwningActor();
    if (!NewOuter || !AuthorityOwner || !AuthorityOwner->HasAuthority()
        || !ItemDefinition || !ItemInstanceGuid.IsValid()
        || NewQuantity <= 0 || NewQuantity > Quantity) {
        return nullptr;
    }

    // The current save frame is the canonical deep-copy contract: it copies only persistent gameplay state and
    // reconstructs fragment instances from the already-loaded Item Definition without inheriting replication owners.
    TArray<uint8> Payload;
    FMemoryWriter Writer(Payload, true);
    FObjectAndNameAsStringProxyArchive SaveArchive(Writer, false);
    SaveArchive.ArIsSaveGame = true;
    const_cast<UMythicItemInstance *>(this)->Serialize(SaveArchive);
    if (SaveArchive.IsError() || Payload.IsEmpty() || Payload.Num() > MaxSemanticCloneBytes) {
        return nullptr;
    }

    UMythicItemInstance *Clone = NewObject<UMythicItemInstance>(NewOuter, GetClass());
    if (!Clone) return nullptr;
    auto RejectClone = [Clone]() -> UMythicItemInstance * {
        Clone->MarkAsGarbage();
        return nullptr;
    };

    FMemoryReader Reader(Payload, true);
    Reader.ArMaxSerializeSize = MaxSemanticCloneBytes;
    Reader.ArIsNetArchive = true;
    FObjectAndNameAsStringProxyArchive LoadArchive(Reader, false);
    LoadArchive.ArIsSaveGame = true;
    LoadArchive.ArMaxSerializeSize = MaxSemanticCloneBytes;
    LoadArchive.ArIsNetArchive = true;
    Clone->Serialize(LoadArchive);
    if (LoadArchive.IsError() || !Reader.AtEnd()
        || Clone->ItemDefinition != ItemDefinition
        || Clone->ItemInstanceGuid != ItemInstanceGuid) {
        return RejectClone();
    }

    const FGuid OldItemGuid = ItemInstanceGuid;
    const FGuid NewItemGuid = FGuid::NewGuid();
    if (!NewItemGuid.IsValid() || NewItemGuid == OldItemGuid) return RejectClone();

    auto DeriveRollGuid = [&NewItemGuid](const FGuid &OldRollGuid) {
        FMythicAffixCanonicalWriter Fields("MYTHIC_ITEM_STACK_CLONE_ROLL_FIELDS_V1");
        Fields.AddGuid(NewItemGuid);
        Fields.AddGuid(OldRollGuid);
        return Fields.IsValid()
            ? FMythicAffixRngFactory::GuidFromCanonicalBytes(
                "Mythic.Item.StackClone.Roll.V1", Fields.GetBytes())
            : FGuid();
    };
    auto DeriveSocketGuid = [&NewItemGuid](const FGuid &OldSocketGuid) {
        FMythicAffixCanonicalWriter Fields("MYTHIC_ITEM_STACK_CLONE_SOCKET_FIELDS_V1");
        Fields.AddGuid(NewItemGuid);
        Fields.AddGuid(OldSocketGuid);
        return Fields.IsValid()
            ? FMythicAffixRngFactory::GuidFromCanonicalBytes(
                "Mythic.Item.StackClone.Socket.V1", Fields.GetBytes())
            : FGuid();
    };

    TSet<FGuid> OldRollGuids;
    TSet<FGuid> NewRollGuids;
    auto RekeyItemOwnedSnapshot = [&](FRolledAffix &Snapshot,
                                      const FGameplayTag RequiredSourceKind) {
        if (!Snapshot.IsGameplayValid() || OldRollGuids.Contains(Snapshot.RollGuid)
            || Snapshot.Provenance.SourceItemGuid != OldItemGuid
            || Snapshot.Provenance.OriginSocketGuid.IsValid()
            || Snapshot.Provenance.SourceKind == AFFIX_SOURCE_SOCKET
            || (RequiredSourceKind.IsValid()
                && Snapshot.Provenance.SourceKind != RequiredSourceKind)) {
            return false;
        }
        OldRollGuids.Add(Snapshot.RollGuid);
        const FGuid NewRollGuid = DeriveRollGuid(Snapshot.RollGuid);
        if (!NewRollGuid.IsValid() || NewRollGuids.Contains(NewRollGuid)) return false;
        NewRollGuids.Add(NewRollGuid);
        Snapshot.RollGuid = NewRollGuid;
        Snapshot.Provenance.SourceItemGuid = NewItemGuid;
        return true;
    };

    for (UItemFragment *Fragment : Clone->ItemFragments) {
        if (!Fragment) return RejectClone();
        if (UAffixesFragment *Affixes = Cast<UAffixesFragment>(Fragment)) {
            for (FMythicReplicatedAffixItem &Row : Affixes->AffixSnapshots.Items) {
                if (!RekeyItemOwnedSnapshot(Row.Affix, FGameplayTag())) return RejectClone();
            }
            Affixes->AffixSnapshots.MarkArrayDirty();
        }
        else if (UMythicGemFragment *Gem = Cast<UMythicGemFragment>(Fragment)) {
            for (FMythicReplicatedAffixItem &Row : Gem->GrantedAffixSnapshots.Items) {
                if (!Row.Affix.bIsLocked
                    || !RekeyItemOwnedSnapshot(Row.Affix, AFFIX_SOURCE_GEM.GetTag())) {
                    return RejectClone();
                }
            }
            Gem->GrantedAffixSnapshots.MarkArrayDirty();
        }
        else if (USocketsFragment *Sockets = Cast<USocketsFragment>(Fragment)) {
            TSet<FGuid> OldSocketGuids;
            TSet<FGuid> NewSocketGuids;
            for (FMythicReplicatedSocketItem &Socket : Sockets->SocketStates.Items) {
                const FGuid OldSocketGuid = Socket.SocketGuid;
                const bool bStructurallyFilled = Socket.bFilled
                    && Socket.SocketedGemType.IsValid() && Socket.SourceGemItemGuid.IsValid()
                    && !Socket.SocketedAffixSnapshots.IsEmpty()
                    && FMythicSocketMath::IsGemCompatible(
                        Socket.SocketedGemType, Socket.SocketColor);
                const bool bStructurallyEmpty = !Socket.bFilled
                    && !Socket.SocketedGemType.IsValid() && !Socket.SourceGemItemGuid.IsValid()
                    && Socket.SocketedAffixSnapshots.IsEmpty();
                if (!OldSocketGuid.IsValid() || OldSocketGuids.Contains(OldSocketGuid)
                    || (!bStructurallyFilled && !bStructurallyEmpty)) {
                    return RejectClone();
                }
                OldSocketGuids.Add(OldSocketGuid);
                const FGuid NewSocketGuid = DeriveSocketGuid(OldSocketGuid);
                if (!NewSocketGuid.IsValid() || NewSocketGuids.Contains(NewSocketGuid)) {
                    return RejectClone();
                }
                NewSocketGuids.Add(NewSocketGuid);

                for (FRolledAffix &Snapshot : Socket.SocketedAffixSnapshots) {
                    if (!Snapshot.IsGameplayValid() || !Snapshot.bIsLocked
                        || OldRollGuids.Contains(Snapshot.RollGuid)
                        || Snapshot.Provenance.SourceKind != AFFIX_SOURCE_SOCKET
                        || Snapshot.Provenance.SourceItemGuid != Socket.SourceGemItemGuid
                        || Snapshot.Provenance.OriginSocketGuid != OldSocketGuid) {
                        return RejectClone();
                    }
                    OldRollGuids.Add(Snapshot.RollGuid);
                    const FGuid NewRollGuid = DeriveRollGuid(Snapshot.RollGuid);
                    if (!NewRollGuid.IsValid() || NewRollGuids.Contains(NewRollGuid)) {
                        return RejectClone();
                    }
                    NewRollGuids.Add(NewRollGuid);
                    Snapshot.RollGuid = NewRollGuid;
                    Snapshot.Provenance.OriginSocketGuid = NewSocketGuid;
                }
                Socket.SocketGuid = NewSocketGuid;
            }
            Sockets->SocketStates.MarkArrayDirty();
        }
    }

    Clone->ItemInstanceGuid = NewItemGuid;
    Clone->Quantity = NewQuantity;
    Clone->OwningInventory = nullptr;
    Clone->SlotIndex = INDEX_NONE;
    return Clone;
}

FMythicItemInitializeResult UMythicItemInstance::InitializeTransactional(
    const FMythicCreateItemRequest &Request,
    const FCompiledAffixProfile *OptionalCompiledProfile) {
    FMythicItemInitializeResult Result;
    if (ItemDefinition || !ItemFragments.IsEmpty() || ItemInstanceGuid.IsValid()
        || !Request.ItemDefinition || !Request.OwningActor || !Request.OwningActor->HasAuthority()
        || !Request.OwningActor->IsUsingRegisteredSubObjectList()
        || Request.ItemLevel < 1 || Request.Quantity < 1) {
        Result.DiagnosticCode = TEXT("InvalidTransactionalInitializeRequest");
        return Result;
    }

    const UAffixesFragment *DefinitionAffixes = nullptr;
    const UMythicGemFragment *DefinitionGem = nullptr;
    const UYieldQualityFragment *DefinitionYieldQuality = nullptr;
    for (const UItemFragment *Source : Request.ItemDefinition->Fragments) {
        if (!Source) {
            Result.DiagnosticCode = TEXT("NullDefinitionFragment");
            return Result;
        }
        if (const UAffixesFragment *Affixes = Cast<UAffixesFragment>(Source)) {
            if (DefinitionAffixes) {
                Result.DiagnosticCode = TEXT("MultipleAffixesFragments");
                return Result;
            }
            DefinitionAffixes = Affixes;
        }
        if (const UMythicGemFragment *Gem = Cast<UMythicGemFragment>(Source)) {
            if (DefinitionGem || Gem->GrantSpecs.IsEmpty()
                || Gem->GrantSpecs.Num() > MythicAffixSerialization::MaxAffixesPerContainer) {
                Result.DiagnosticCode = DefinitionGem
                    ? FName(TEXT("MultipleGemFragments")) : FName(TEXT("InvalidGemGrantCount"));
                return Result;
            }
            DefinitionGem = Gem;
        }
        if (const UYieldQualityFragment *YieldQuality =
                Cast<UYieldQualityFragment>(Source)) {
            if (DefinitionYieldQuality) {
                Result.DiagnosticCode = TEXT("MultipleYieldQualityFragments");
                return Result;
            }
            DefinitionYieldQuality = YieldQuality;
        }
    }

    if (Request.YieldQualityOverride.IsSet()
        && (!DefinitionYieldQuality
            || !IsValidYieldQuality(Request.YieldQualityOverride.GetValue()))) {
        Result.DiagnosticCode = DefinitionYieldQuality
            ? FName(TEXT("InvalidYieldQualityOverride"))
            : FName(TEXT("MissingYieldQualityFragmentForOverride"));
        return Result;
    }

    if (DefinitionAffixes) {
        if (!DefinitionAffixes->AffixesConfig.AffixProfile.IsValid()
            || !OptionalCompiledProfile
            || OptionalCompiledProfile->ProfileId
                != DefinitionAffixes->AffixesConfig.AffixProfile.GetPrimaryAssetId()) {
            Result.DiagnosticCode = TEXT("CompiledProfileMismatch");
            return Result;
        }
    }
    else if (OptionalCompiledProfile) {
        Result.DiagnosticCode = TEXT("UnexpectedCompiledProfile");
        return Result;
    }

    TArray<TObjectPtr<UItemFragment>> StagedFragments;
    StagedFragments.Reserve(Request.ItemDefinition->Fragments.Num());
    UAffixesFragment *StagedAffixes = nullptr;
    UMythicGemFragment *StagedGem = nullptr;
    UYieldQualityFragment *StagedYieldQuality = nullptr;
    for (UItemFragment *Source : Request.ItemDefinition->Fragments) {
        UItemFragment *Fragment = NewObject<UItemFragment>(
            this, Source->GetClass(), NAME_None, RF_NoFlags, Source);
        if (!Fragment) {
            Result.Status = EMythicItemInitializeStatus::GenerationFailed;
            Result.DiagnosticCode = TEXT("FragmentAllocationFailed");
            return Result;
        }
        Fragment->SetOwnerItemInstance(this);
        StagedFragments.Add(Fragment);
        if (UAffixesFragment *Affixes = Cast<UAffixesFragment>(Fragment)) StagedAffixes = Affixes;
        if (UMythicGemFragment *Gem = Cast<UMythicGemFragment>(Fragment)) StagedGem = Gem;
        if (UYieldQualityFragment *YieldQuality =
                Cast<UYieldQualityFragment>(Fragment)) {
            StagedYieldQuality = YieldQuality;
        }
    }

    if (Request.YieldQualityOverride.IsSet()) {
        check(StagedYieldQuality);
        // The resolved outcome is written into the unreachable staged graph before OnInstanced publishes its tag.
        // No delivery retry or later mutable balance lookup can change the item that commits below.
        StagedYieldQuality->QualityTier =
            Request.YieldQualityOverride.GetValue();
    }

    const FGuid StagedItemInstanceGuid = FGuid::NewGuid();
    const int32 StagedQuantity = ClampInitialStackQuantity(
        Request.Quantity, Request.ItemDefinition->StackSizeMax);
    if (!StagedItemInstanceGuid.IsValid()) {
        Result.Status = EMythicItemInitializeStatus::GenerationFailed;
        Result.DiagnosticCode = TEXT("ItemIdentityAllocationFailed");
        return Result;
    }

    if (StagedAffixes) {
        FMythicAffixRollRequest RollRequest;
        RollRequest.ItemInstanceGuid = StagedItemInstanceGuid;
        RollRequest.ItemLevel = Request.ItemLevel;
        RollRequest.Rarity = Request.ItemDefinition->Rarity.GetValue();
        if (Request.ItemDefinition->ItemType.IsValid()) {
            RollRequest.ContextTags.AddTag(Request.ItemDefinition->ItemType);
        }
        RollRequest.ContextTags.AppendTags(ItemTags);
        RollRequest.ProfileId = OptionalCompiledProfile->ProfileId;
        RollRequest.Seed = Request.ServerSeed != 0
            ? Request.ServerSeed : DeriveFactoryAffixSeed(StagedItemInstanceGuid, RollRequest.ProfileId);
        RollRequest.AlgorithmVersion = OptionalCompiledProfile->Policy.AlgorithmVersion;

        TArray<FRolledAffix> GeneratedSnapshots;
        if (!FMythicAffixGenerator::Generate(
                RollRequest, *OptionalCompiledProfile, GeneratedSnapshots, nullptr)) {
            Result.Status = EMythicItemInitializeStatus::GenerationFailed;
            Result.DiagnosticCode = TEXT("AffixGenerationFailed");
            return Result;
        }
        StagedAffixes->AffixSnapshots.SetOwner(StagedAffixes);
        StagedAffixes->AffixSnapshots.ReplaceAll(MoveTemp(GeneratedSnapshots));
        StagedAffixes->AffixesRuntimeReplicatedData.bCorrupted = false;
    }

    if (StagedGem) {
        UGameInstance *GameInstance = Request.OwningActor->GetGameInstance();
        UMythicItemizationDataRegistrySubsystem *Registry = GameInstance
            ? GameInstance->GetSubsystem<UMythicItemizationDataRegistrySubsystem>() : nullptr;
        if (!Registry) {
            Result.Status = EMythicItemInitializeStatus::GenerationFailed;
            Result.DiagnosticCode = TEXT("GemGrantRegistryUnavailable");
            return Result;
        }

        // The staging-safe gem seam reads the owning item context. Publish these values only inside this unreachable
        // candidate, and roll them back if any grant fails before replication/inventory ownership is registered.
        ItemDefinition = Request.ItemDefinition;
        ItemLevel = Request.ItemLevel;
        Quantity = StagedQuantity;
        ItemInstanceGuid = StagedItemInstanceGuid;
        if (!StagedGem->MaterializeFreshGrantsForItem(*this, *Registry)) {
            ItemDefinition = nullptr;
            ItemLevel = 1;
            Quantity = 1;
            ItemInstanceGuid.Invalidate();
            Result.Status = EMythicItemInitializeStatus::GenerationFailed;
            Result.DiagnosticCode = TEXT("GemGrantMaterializationFailed");
            return Result;
        }
    }

    // Publish the completed value graph to the still-unregistered item, then register it exactly once.
    ItemDefinition = Request.ItemDefinition;
    ItemLevel = Request.ItemLevel;
    Quantity = StagedQuantity;
    ItemInstanceGuid = StagedItemInstanceGuid;
    ItemFragments = MoveTemp(StagedFragments);
    SetOwner(Request.OwningActor);
    for (UItemFragment *Fragment : ItemFragments) {
        Fragment->SetOwner(Request.OwningActor);
        if (Fragment == StagedAffixes || Fragment == StagedGem) {
            // Generation already used the pinned closure above. Calling UAffixesFragment::OnInstanced would look up
            // mutable registry state a second time; GemFragment would also start an async callback after commit.
            Fragment->SetOwnerItemInstance(this);
        }
        else {
            Fragment->OnInstanced(this);
        }
    }

    UE_LOG(Myth, Verbose, TEXT("Transactionally initialized level %d item %s"), ItemLevel, *GetName());
    Result.Status = EMythicItemInitializeStatus::Success;
    Result.DiagnosticCode = NAME_None;
    return Result;
}

#if WITH_DEV_AUTOMATION_TESTS
void UMythicItemInstance::InitializeFixtureForTests(
    UItemDefinition *ItemDef, const int32 QuantityIfStackable, const int32 Level) {
    checkf(this->GetOwningActor()->HasAuthority(), TEXT("Only the server can initialize an item instance"));

    checkf(EnsureNewItemInstanceGuid(), TEXT("A new item instance requires a stable identity before initialization"));

    this->ItemDefinition = ItemDef;
    this->ItemLevel = Level;
    this->Quantity = ClampInitialStackQuantity(QuantityIfStackable, ItemDef->StackSizeMax);
    UE_LOG(Myth, Verbose, TEXT("Initialized level %d automation fixture item %s"), Level, *GetName());

    for (int i = 0; i < ItemDef->Fragments.Num(); i++) {
        auto FragmentSource = ItemDef->Fragments[i];
        if (!FragmentSource) {
            UE_LOG(Myth, Error, TEXT("ItemInstance %s has a null fragment at index %d"), *GetName(), i);
            continue;
        }

        AddFragmentFixtureForTests(FragmentSource);
    }
}

bool UMythicItemInstance::AssignNewItemInstanceGuid() {
    const AActor *Owner = GetOwningActor();
    if (!ensureMsgf(Owner && Owner->HasAuthority(), TEXT("AssignNewItemInstanceGuid is authority-only"))) {
        return false;
    }

    ItemInstanceGuid = FGuid::NewGuid();
    return ItemInstanceGuid.IsValid();
}

bool UMythicItemInstance::EnsureNewItemInstanceGuid() {
    return ItemInstanceGuid.IsValid() || AssignNewItemInstanceGuid();
}

void UMythicItemInstance::AddFragmentFixtureForTests(TObjectPtr<UItemFragment> FragmentSource) {
    auto owner = this->GetOwningActor();
    checkf(owner->HasAuthority(), TEXT("Only the server can add fragments to an item instance"));

    UItemFragment *Fragment = NewObject<UItemFragment>(this, FragmentSource->GetClass(), NAME_None, RF_NoFlags, FragmentSource);
    Fragment->SetOwner(owner);

    ItemFragments.Add(Fragment);

    Fragment->OnInstanced(this);
}
#endif

void UMythicItemInstance::OnActiveItem() {
    for (int i = 0; i < ItemFragments.Num(); i++) {
        if (ItemFragments[i] == nullptr) { continue; }
        ItemFragments[i]->OnItemActivated(this);
    }
}

void UMythicItemInstance::OnInactiveItem() {
    for (int i = ItemFragments.Num() - 1; i >= 0; i--) {
        if (ItemFragments[i] == nullptr) { continue; }
        ItemFragments[i]->OnItemDeactivated(this);
    }
}

void UMythicItemInstance::OnClientActiveItem() {
    for (int i = 0; i < ItemFragments.Num(); i++) {
        if (ItemFragments[i] == nullptr) { continue; }
        ItemFragments[i]->OnClientItemActivated(this);
    }
}

void UMythicItemInstance::OnClientInactiveItem() {
    for (int i = 0; i < ItemFragments.Num(); i++) {
        if (ItemFragments[i] == nullptr) { continue; }
        ItemFragments[i]->OnClientItemDeactivated(this);
    }
}

int32 UMythicItemInstance::CountFragmentsOfClass(
    const TSubclassOf<UItemFragment> FragmentClass) const {
    if (!FragmentClass) {
        return 0;
    }
    int32 Count = 0;
    for (const UItemFragment *Fragment : ItemFragments) {
        Count += Fragment && Fragment->IsA(FragmentClass) ? 1 : 0;
    }
    return Count;
}

void UMythicItemInstance::SetInventory(UMythicInventoryComponent *NewInventory, int32 NewSlotIndex) {
    this->OwningInventory = NewInventory;
    this->SlotIndex = NewSlotIndex;
    for (TObjectPtr ItemFragment : this->ItemFragments) {
        ItemFragment->OnInventorySlotChanged(NewInventory, NewSlotIndex);
    }
}

int32 UMythicItemInstance::GetSlot() const {
    return this->SlotIndex;
}

UMythicInventoryComponent *UMythicItemInstance::GetInventoryComponent() const {
    return this->OwningInventory;
}

AActor *UMythicItemInstance::GetInventoryOwner() const {
    if (auto InventoryComponent = GetInventoryComponent()) {
        return InventoryComponent->GetOwner();
    }
    return nullptr;
}

void UMythicItemInstance::AddTag(const FGameplayTag &Tag) {
    checkf(this->GetOwningActor()->HasAuthority(), TEXT("Only the server can add tags to an item instance"));

    if (HasTag(Tag)) {
        return;
    }

    ItemTags.AddTag(Tag);
}

void UMythicItemInstance::RemoveTag(const FGameplayTag &Tag) {
    checkf(this->GetOwningActor()->HasAuthority(), TEXT("Only the server can remove tags from an item instance"));

    ItemTags.RemoveTag(Tag);
}

bool UMythicItemInstance::HasTag(const FGameplayTag &Tag) const {
    return ItemTags.HasTag(Tag);
}

void UMythicItemInstance::GetTypeProbe(FGameplayTagContainer &Out) const {
    Out.Reset();
    if (ItemDefinition) {
        Out.AddTag(ItemDefinition->ItemType);
    }
    Out.AppendTags(ItemTags);
}

void UMythicItemInstance::ServerApplyTransform(const FGameplayTag &NewItemType,
                                               const FGameplayTagContainer &TagsToAdd,
                                               const FGameplayTagContainer &TagsToRemove,
                                               UItemDefinition *OptionalNewDef) {
    checkf(GetOwningActor() && GetOwningActor()->HasAuthority(), TEXT("ServerApplyTransform: authority only"));

    for (const FGameplayTag &T : TagsToRemove) {
        ItemTags.RemoveTag(T);
    }
    for (const FGameplayTag &T : TagsToAdd) {
        if (!ItemTags.HasTag(T)) {
            ItemTags.AddTag(T);
        }
    }
    if (NewItemType.IsValid() && !ItemTags.HasTag(NewItemType)) {
        ItemTags.AddTag(NewItemType);
    }
    if (OptionalNewDef) {
        ItemDefinition = OptionalNewDef;
    }

    if (OwningInventory) {
        OwningInventory->NotifyItemInstanceUpdated(SlotIndex);
    }
}

bool UMythicItemInstance::isStackableWith(const UMythicItemInstance *Other) const {
    if (ItemDefinition && ItemDefinition->StackSizeMax <= 0) {
        return false;
    }

    if (!Other) {
        return false;
    }
    if (ItemFragments.Num() != Other->ItemFragments.Num()) {
        return false;
    }

    for (int i = 0; i < ItemFragments.Num(); i++) {
        if (ItemFragments[i] == nullptr) { continue; }
        if (!ItemFragments[i]->CanBeStackedWith(Other->ItemFragments[i])) {
            return false;
        }
    }

    return true;
}

void UMythicItemInstance::ConsumeItem(int32 StackQty) {
    if (auto Inventory = this->GetInventoryComponent()) {
        Inventory->ServerRemoveItem(this, StackQty);
        return;
    }

    this->SetStackSize(this->GetStacks() - StackQty);
    if (this->GetStacks() <= 0) {
        auto WorldItem = Cast<AMythicWorldItem>(this->GetOwningActor());
        this->Destroy();
        if (WorldItem) {
            WorldItem->Destroy();
        }
    }
}

void UMythicItemInstance::OnDestroyed() {
    if (IsValid(this)) {
        auto inventory = this->GetInventoryComponent();
        if (IsValid(inventory)) {
            inventory->SetItemInSlot(this->GetSlot(), nullptr);
        }

        for (auto Fragment : ItemFragments) {
            if (IsValid(Fragment)) {
                Fragment->MarkAsGarbage();
            }
        }

        ItemFragments.Empty();
    }
}

void UMythicItemInstance::OnRep_Quantity() {
    if (OwningInventory) {
        OwningInventory->NotifyItemInstanceUpdated(SlotIndex);
    }
}

void UMythicItemInstance::OnRep_ItemDefinition() {
    if (OwningInventory) {
        OwningInventory->NotifyItemInstanceUpdated(SlotIndex);
    }
}

void UMythicItemInstance::OnRep_OwningInventory() {
    if (OwningInventory && SlotIndex != INDEX_NONE) {
        OwningInventory->NotifyItemInstanceUpdated(SlotIndex);
    }
}

void UMythicItemInstance::OnRep_SlotIndex() {
    if (OwningInventory && SlotIndex != INDEX_NONE) {
        OwningInventory->NotifyItemInstanceUpdated(SlotIndex);
    }
}

void UMythicItemInstance::OnRep_MarkedJunk() {
    if (OwningInventory && SlotIndex != INDEX_NONE) {
        OwningInventory->NotifyItemInstanceUpdated(SlotIndex);
    }
}

void UMythicItemInstance::ServerSetMarkedJunk(bool bJunk) {
    checkf(GetOwningActor() && GetOwningActor()->HasAuthority(), TEXT("Only the server can set the junk flag on an item instance"));

    if (bMarkedJunk == bJunk) {
        return;
    }
    bMarkedJunk = bJunk;
    if (OwningInventory && SlotIndex != INDEX_NONE) {
        OwningInventory->NotifyItemInstanceUpdated(SlotIndex);
    }
}
