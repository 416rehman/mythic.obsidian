#include "Itemization/Inventory/Fragments/Passive/MythicGemFragment.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Itemization/Affixes/MythicAffixDefinition.h"
#include "Itemization/Affixes/MythicAffixGeneration.h"
#include "Itemization/Affixes/MythicAffixRng.h"
#include "Itemization/Affixes/MythicItemizationDataRegistrySubsystem.h"
#include "Itemization/Affixes/MythicTags_Affixes.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Mythic/Mythic.h"
#include "Net/UnrealNetwork.h"

namespace {
bool SnapshotsEquivalentForStacking(const FRolledAffix &A, const FRolledAffix &B) {
    return A.AffixDefinition == B.AffixDefinition
        && A.TierRank == B.TierRank
        && A.Magnitude == B.Magnitude
        && A.bIsLocked == B.bIsLocked;
}
}

void UMythicGemFragment::Serialize(FArchive &Ar) {
    Super::Serialize(Ar);
    // Package saves must be observational; SaveGame loads do not receive PostLoad and restore the callback here.
    if (Ar.IsLoading() && Ar.IsSaveGame()) {
        GrantedAffixSnapshots.SetOwner(this);
    }
}

void UMythicGemFragment::PostLoad() {
    Super::PostLoad();
    GrantedAffixSnapshots.SetOwner(this);
    if (!HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)) {
        RequestRuntimeData();
    }
}

void UMythicGemFragment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(ThisClass, GemType, COND_InitialOrOwner);
    DOREPLIFETIME_CONDITION(ThisClass, GrantedAffixSnapshots, COND_OwnerOnly);
}

bool UMythicGemFragment::HasServerAuthority() const {
    const AActor *Owner = GetOwningActor();
    return Owner && Owner->HasAuthority();
}

UMythicItemizationDataRegistrySubsystem *UMythicGemFragment::ResolveRegistry() const {
    const UWorld *World = GetWorld();
    const UGameInstance *GameInstance = World ? World->GetGameInstance() : nullptr;
    return GameInstance ? GameInstance->GetSubsystem<UMythicItemizationDataRegistrySubsystem>() : nullptr;
}

uint64 UMythicGemFragment::DeriveGrantSeed(const FGuid &ItemInstanceGuid) {
    FMythicAffixCanonicalWriter Writer("MYTHIC_GEM_GRANT_SEED_V1");
    Writer.AddGuid(ItemInstanceGuid);
    FMythicAffixRngV1 Rng(0, 0);
    return Writer.IsValid() && FMythicAffixRngFactory::FromCanonicalBytes(Writer.GetBytes(), Rng)
               ? (static_cast<uint64>(Rng.NextUInt32()) << 32) | Rng.NextUInt32()
               : 0;
}

void UMythicGemFragment::OnInstanced(UMythicItemInstance *Instance) {
    Super::OnInstanced(Instance);
    GrantedAffixSnapshots.SetOwner(this);
    if (!Instance || !HasServerAuthority()) {
        return;
    }
    RequestRuntimeData();
}

void UMythicGemFragment::OnInventorySlotChanged(UMythicInventoryComponent *NewInventory, int32 NewSlot) {
    Super::OnInventorySlotChanged(NewInventory, NewSlot);
    GrantedAffixSnapshots.SetOwner(this);
    RequestRuntimeData();
}

void UMythicGemFragment::RequestRuntimeData() {
    GrantedAffixSnapshots.SetOwner(this);
    if (bRuntimeRequestInFlight || bRuntimeMaterializationFailed) {
        return;
    }

    UMythicItemizationDataRegistrySubsystem *Registry = ResolveRegistry();
    if (!Registry) {
        // The fragment can be deserialized before it is attached to a world/inventory. Slot attachment retries.
        return;
    }

    bRuntimeRequestInFlight = true;
    const uint32 RequestRevision = ++RuntimeDataRevision;
    TWeakObjectPtr<UMythicGemFragment> WeakThis(this);
    FOnMythicItemizationDataReady Completion = FOnMythicItemizationDataReady::CreateLambda(
        [WeakThis, RequestRevision](const bool bReady) {
            if (WeakThis.IsValid()) {
                WeakThis->HandleRuntimeDataReady(bReady, RequestRevision);
            }
        });

    if (!GrantedAffixSnapshots.Items.IsEmpty()) {
        Registry->RequestCoreSemanticDataAsync(MoveTemp(Completion));
    }
    else if (!GrantSpecs.IsEmpty()) {
        Registry->RequestGrantClosureAsync(GrantSpecs, MoveTemp(Completion));
    }
    else {
        // A barren gem has no gameplay closure. It remains unusable by design.
        bRuntimeRequestInFlight = false;
    }
}

void UMythicGemFragment::HandleRuntimeDataReady(const bool bSuccess,
                                                const uint32 RequestRevision) {
    if (RequestRevision != RuntimeDataRevision) return;
    bRuntimeRequestInFlight = false;
    if (!bSuccess) {
        bRuntimeMaterializationFailed = true;
        UE_LOG(Myth, Error, TEXT("Gem fragment %s failed to load its required typed affix data."), *GetName());
        return;
    }

    if (HasServerAuthority()) {
        bool bCommitted = true;
        if (GrantedAffixSnapshots.Items.IsEmpty() && !GrantSpecs.IsEmpty()) {
            bCommitted = MaterializeFreshGrants();
        }
        else if (!GrantedAffixSnapshots.Items.IsEmpty()) {
            const UMythicItemInstance *Item = GetOwningItemInstance();
            const UMythicItemizationDataRegistrySubsystem *Registry = ResolveRegistry();
            TSet<FGuid> RollGuids;
            bCommitted = Item && Registry && Item->GetItemInstanceGuid().IsValid()
                && GrantedAffixSnapshots.Items.Num() <= MythicAffixSerialization::MaxAffixesPerContainer;
            for (const FMythicReplicatedAffixItem &Row : GrantedAffixSnapshots.Items) {
                const FRolledAffix &Snapshot = Row.Affix;
                const UMythicAffixDefinition *Definition = bCommitted
                    ? Registry->FindAffix(Snapshot.AffixDefinition.GetPrimaryAssetId()) : nullptr;
                bCommitted = bCommitted && Snapshot.IsGameplayValid() && Snapshot.bIsLocked
                    && Definition && Definition->TargetStat.IsValid()
                    && Snapshot.Provenance.SourceKind == AFFIX_SOURCE_GEM
                    && Snapshot.Provenance.SourceItemGuid == Item->GetItemInstanceGuid()
                    && !Snapshot.Provenance.OriginSocketGuid.IsValid()
                    && !RollGuids.Contains(Snapshot.RollGuid)
                    && (!MythicAffix::ModifierRequiresNonZeroMagnitude(Definition->ModifierOp)
                        || !FMath::IsNearlyZero(Snapshot.Magnitude));
                if (!bCommitted) break;
                RollGuids.Add(Snapshot.RollGuid);
            }
        }
        if (!bCommitted) {
            bRuntimeMaterializationFailed = true;
            UE_LOG(Myth, Error,
                   TEXT("Gem fragment %s could not validate or materialize its typed affix snapshots."),
                   *GetName());
            return;
        }
    }
    RefreshOwningInventoryPresentation();
}

bool UMythicGemFragment::MaterializeFreshGrants() {
    if (!HasServerAuthority() || GrantSpecs.IsEmpty() || !GrantedAffixSnapshots.Items.IsEmpty()
        || GrantSpecs.Num() > MythicAffixSerialization::MaxAffixesPerContainer) {
        return false;
    }

    UMythicItemInstance *Item = GetOwningItemInstance();
    UMythicItemizationDataRegistrySubsystem *Registry = ResolveRegistry();
    if (!Item || !Registry || !Item->GetItemInstanceGuid().IsValid()) {
        return false;
    }

    const bool bCommitted = MaterializeFreshGrantsForItem(*Item, *Registry);
    if (bCommitted) RefreshOwningInventoryPresentation();
    return bCommitted;
}

bool UMythicGemFragment::MaterializeFreshGrantsForItem(
    UMythicItemInstance &OwningItem,
    const UMythicItemizationDataRegistrySubsystem &ReadyRegistry) {
    GrantedAffixSnapshots.SetOwner(this);
    if (GrantSpecs.IsEmpty() || !GrantedAffixSnapshots.Items.IsEmpty()
        || GrantSpecs.Num() > MythicAffixSerialization::MaxAffixesPerContainer
        || GetOwningItemInstance() != &OwningItem
        || !OwningItem.GetItemInstanceGuid().IsValid() || !ReadyRegistry.IsCoreSemanticReady()) {
        return false;
    }

    FMythicAffixGrantContext Context;
    Context.ItemInstanceGuid = OwningItem.GetItemInstanceGuid();
    Context.ItemLevel = FMath::Max(1, OwningItem.GetItemLevel());
    Context.Rarity = OwningItem.GetItemDefinition()
                         ? OwningItem.GetItemDefinition()->Rarity.GetValue()
                         : EItemRarity::Common;
    OwningItem.GetTypeProbe(Context.ContextTags);
    Context.Seed = DeriveGrantSeed(Context.ItemInstanceGuid);

    TArray<FRolledAffix> Candidates;
    Candidates.Reserve(GrantSpecs.Num());
    TSet<FGuid> RollGuids;
    for (int32 Index = 0; Index < GrantSpecs.Num(); ++Index) {
        const FMythicAffixGrantSpec &Authored = GrantSpecs[Index];
        if (!Authored.GrantGuid.IsValid() || !Authored.AffixDefinition.IsValid()
            || Authored.TierMode != EMythicAffixGrantTierMode::ExactTier
            || Authored.ExactTierRank <= 0 || !Authored.RollGroup.IsValid()
            || Authored.SourceKind != AFFIX_SOURCE_GEM || !Authored.bLocked) {
            return false;
        }

        // Container semantics are authoritative: gem grants are always locked and tagged as gem source.
        FMythicAffixGrantSpec RuntimeSpec = Authored;
        RuntimeSpec.SourceKind = AFFIX_SOURCE_GEM;
        RuntimeSpec.bLocked = true;
        Context.RollOrdinal = Index;

        FRolledAffix Candidate;
        if (!FMythicAffixGrantService::Materialize(RuntimeSpec, Context, ReadyRegistry, Candidate, nullptr)
            || !Candidate.IsGameplayValid() || !Candidate.bIsLocked
            || Candidate.Provenance.SourceKind != AFFIX_SOURCE_GEM
            || Candidate.Provenance.SourceItemGuid != OwningItem.GetItemInstanceGuid()
            || Candidate.Provenance.OriginSocketGuid.IsValid()
            || RollGuids.Contains(Candidate.RollGuid)) {
            return false;
        }
        RollGuids.Add(Candidate.RollGuid);
        Candidates.Add(MoveTemp(Candidate));
    }

    GrantedAffixSnapshots.ReplaceAll(MoveTemp(Candidates));
    return true;
}

bool UMythicGemFragment::IsGem() const {
    return GemType.IsValid()
        && (!GrantedAffixSnapshots.Items.IsEmpty() || !GrantSpecs.IsEmpty());
}

int32 UMythicGemFragment::GetGrantedAffixCount() const {
    return !GrantedAffixSnapshots.Items.IsEmpty() ? GrantedAffixSnapshots.Items.Num() : GrantSpecs.Num();
}

void UMythicGemFragment::GetGrantedAffixSnapshots(TArray<FRolledAffix> &OutSnapshots) const {
    GrantedAffixSnapshots.GetSnapshots(OutSnapshots);
}

bool UMythicGemFragment::CanBeStackedWith(const UItemFragment *Other) const {
    if (!Super::CanBeStackedWith(Other)) {
        return false;
    }
    const UMythicGemFragment *OtherGem = Cast<UMythicGemFragment>(Other);
    const UMythicItemInstance *OurItem = GetOwningItemInstance();
    const UMythicItemInstance *OtherItem = OtherGem ? OtherGem->GetOwningItemInstance() : nullptr;
    if (!OtherGem || GemType != OtherGem->GemType || GrantedAffixSnapshots.Items.IsEmpty()
        || GrantedAffixSnapshots.Items.Num() != OtherGem->GrantedAffixSnapshots.Items.Num()
        || !OurItem || !OtherItem || !OurItem->GetItemInstanceGuid().IsValid()
        || !OtherItem->GetItemInstanceGuid().IsValid()) {
        return false;
    }
    auto HasCanonicalGemProvenance = [](const UMythicGemFragment &Gem,
                                        const FGuid ItemGuid) {
        for (const FMythicReplicatedAffixItem &Row : Gem.GrantedAffixSnapshots.Items) {
            const FRolledAffix &Snapshot = Row.Affix;
            if (!Snapshot.IsGameplayValid() || !Snapshot.bIsLocked
                || Snapshot.Provenance.SourceKind != AFFIX_SOURCE_GEM
                || Snapshot.Provenance.SourceItemGuid != ItemGuid
                || Snapshot.Provenance.OriginSocketGuid.IsValid()) {
                return false;
            }
        }
        return true;
    };
    if (!HasCanonicalGemProvenance(*this, OurItem->GetItemInstanceGuid())
        || !HasCanonicalGemProvenance(*OtherGem, OtherItem->GetItemInstanceGuid())) {
        return false;
    }
    TArray<bool> Used;
    Used.Init(false, OtherGem->GrantedAffixSnapshots.Items.Num());
    for (const FMythicReplicatedAffixItem &Ours : GrantedAffixSnapshots.Items) {
        bool bMatched = false;
        for (int32 Index = 0; Index < OtherGem->GrantedAffixSnapshots.Items.Num(); ++Index) {
            if (!Used[Index]
                && SnapshotsEquivalentForStacking(
                    Ours.Affix, OtherGem->GrantedAffixSnapshots.Items[Index].Affix)) {
                Used[Index] = true;
                bMatched = true;
                break;
            }
        }
        if (!bMatched) return false;
    }
    return true;
}

void UMythicGemFragment::OnAffixSnapshotsReplicated() {
    GrantedAffixSnapshots.SetOwner(this);
    ++RuntimeDataRevision;
    bRuntimeRequestInFlight = false;
    bRuntimeMaterializationFailed = false;
    RequestRuntimeData();
    RefreshOwningInventoryPresentation();
}

void UMythicGemFragment::RefreshOwningInventoryPresentation() const {
    if (UMythicItemInstance *Item = GetOwningItemInstance()) {
        if (UMythicInventoryComponent *Inventory = Item->GetInventoryComponent()) {
            Inventory->NotifyItemInstanceUpdated(Item->GetSlot());
        }
    }
}

#if WITH_EDITOR
bool UMythicGemFragment::IsValidFragment(FText &OutErrorMessage) const {
    if (!GemType.IsValid()) {
        OutErrorMessage = FText::FromString(TEXT("GemType must be a valid Itemization.Gem tag."));
        return false;
    }
    if (GrantSpecs.IsEmpty()) {
        OutErrorMessage = FText::FromString(TEXT("A gem must author at least one exact affix GrantSpec."));
        return false;
    }
    TSet<FGuid> GrantGuids;
    for (const FMythicAffixGrantSpec &Grant : GrantSpecs) {
        if (!Grant.GrantGuid.IsValid() || GrantGuids.Contains(Grant.GrantGuid)
            || !Grant.AffixDefinition.IsValid()
            || Grant.TierMode != EMythicAffixGrantTierMode::ExactTier || Grant.ExactTierRank <= 0
            || !Grant.RollGroup.IsValid() || Grant.SourceKind != AFFIX_SOURCE_GEM
            || !Grant.bLocked) {
            OutErrorMessage = FText::FromString(
                TEXT("Gem GrantSpecs require unique identities, exact typed Affix Definition/tier ranks, a roll group, Gem source, and locked state."));
            return false;
        }
        GrantGuids.Add(Grant.GrantGuid);
    }
    return true;
}
#endif
