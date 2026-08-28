#include "Itemization/Inventory/Fragments/Passive/AffixesFragment.h"

#include "Itemization/Affixes/MythicAffixRng.h"
#include "Itemization/Affixes/MythicAffixDefinition.h"
#include "Itemization/Affixes/MythicItemizationDataRegistrySubsystem.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Settings/MythicDeveloperSettings.h"
#include "System/MythicAssetManager.h"
#include "Mythic/Mythic.h"
#include "Net/UnrealNetwork.h"

namespace {
bool SnapshotsEqual(const FRolledAffix &A, const FRolledAffix &B) {
    return A.AffixDefinition == B.AffixDefinition
        && A.TierRank == B.TierRank
        && A.Magnitude == B.Magnitude
        && A.bIsLocked == B.bIsLocked;
}
}

void UAffixesFragment::Serialize(FArchive &Ar) {
    Super::Serialize(Ar);
    // SavePackage serializes objects in multiple passes. Mutating a reflected field while saving can change which
    // property names are required after the name-harvest pass and corrupt the package. SaveGame restores a runtime
    // fragment without PostLoad, so restore the transient callback only on that exact load path.
    if (Ar.IsLoading() && Ar.IsSaveGame()) {
        AffixSnapshots.SetOwner(this);
    }
}

void UAffixesFragment::PostLoad() {
    Super::PostLoad();
    AffixSnapshots.SetOwner(this);
}

void UAffixesFragment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, AffixesConfig, COND_InitialOrOwner, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION(ThisClass, AffixSnapshots, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(ThisClass, AffixesRuntimeReplicatedData, COND_InitialOrOwner);
}

bool UAffixesFragment::HasServerAuthority() const {
    const AActor *Owner = GetOwningActor();
    return Owner && Owner->HasAuthority();
}

UMythicItemizationDataRegistrySubsystem *UAffixesFragment::ResolveRegistry() const {
    const UWorld *World = GetWorld();
    const UGameInstance *GameInstance = World ? World->GetGameInstance() : nullptr;
    return GameInstance ? GameInstance->GetSubsystem<UMythicItemizationDataRegistrySubsystem>() : nullptr;
}

void UAffixesFragment::OnInstanced(UMythicItemInstance *Instance) {
    Super::OnInstanced(Instance);
    AffixSnapshots.SetOwner(this);
    if (!Instance || !HasServerAuthority()) return;
    // Fresh generation is exclusively owned by InitializeTransactional and its pinned compiled profile. A fragment
    // reaching this generic callback empty is an invalid construction path, never a cue to consult mutable data.
    if (AffixSnapshots.Items.IsEmpty()) {
        bGenerationFailed = true;
        UE_LOG(Myth, Error,
               TEXT("AffixesFragment on %s bypassed the transactional item factory; generation refused."),
               *GetNameSafe(Instance));
    }
}

void UAffixesFragment::OnItemActivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemActivated(ItemInstance);
    if (!HasServerAuthority()) return;
    // The inventory mutation emits one authoritative whole-equipment reconciliation after every fragment has
    // activated. Per-snapshot application here would create transient stat churn and a second control path.
}

void UAffixesFragment::OnItemDeactivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemDeactivated(ItemInstance);
    // Removal is intentionally deferred until the inventory slot mutation is committed, then reconciled once.
}

bool UAffixesFragment::RerollUnlockedAffixes(int32 ItemLevel) {
    if (!HasServerAuthority()) return false;
    FText Reason;
    if (!CanApplyCraftOp(Reason)) return false;
    struct FReplacement { int32 Index; FRolledAffix New; };
    TArray<FReplacement> Replacements;
    UMythicItemInstance *OwningItem = GetOwningItemInstance();
    UMythicItemizationDataRegistrySubsystem *Registry = ResolveRegistry();
    if (!OwningItem || !Registry || !Registry->IsCoreSemanticReady()) return false;
    FGameplayTagContainer ContextTags;
    OwningItem->GetTypeProbe(ContextTags);
    ItemLevel = FMath::Max(1, ItemLevel);
    for (int32 Index = 0; Index < AffixSnapshots.Items.Num(); ++Index) {
        const FRolledAffix &Old = AffixSnapshots.Items[Index].Affix;
        if (Old.bIsLocked) continue;
        const UMythicAffixDefinition *Definition = Registry->FindAffix(
            Old.AffixDefinition.GetPrimaryAssetId());
        const FMythicAffixTierProgressionDefinition *Progression = Definition
            ? Definition->ResolveTierProgression(ContextTags) : nullptr;
        const FMythicAffixTierDefinition *Tier = Progression
            && Progression->Tiers.IsValidIndex(Old.TierRank - 1)
                ? &Progression->Tiers[Old.TierRank - 1] : nullptr;
        float MinMagnitude = 0.0f;
        float MaxMagnitude = 0.0f;
        if (!Definition || !Tier
            || !UMythicAffixDefinition::ResolveMagnitudeBand(
                Tier->Magnitude, ItemLevel, MinMagnitude, MaxMagnitude)) {
            return false;
        }
        FReplacement &Replacement = Replacements.AddDefaulted_GetRef();
        Replacement.Index = Index; Replacement.New = Old;
        Replacement.New.Provenance.MutationRevision++;
        const uint64 Seed = (static_cast<uint64>(Old.RollGuid.A) << 32) | Old.RollGuid.B;
        FMythicAffixRngV1 Rng(Seed ^ static_cast<uint64>(Replacement.New.Provenance.MutationRevision),
                              (static_cast<uint64>(Old.RollGuid.C) << 32) | Old.RollGuid.D);
        const double Unit = (static_cast<double>(Rng.NextUInt32()) + 0.5) / 4294967296.0;
        const float Raw = MinMagnitude == MaxMagnitude ? MinMagnitude
            : static_cast<float>(MinMagnitude + (MaxMagnitude - MinMagnitude) * Unit);
        Replacement.New.Magnitude = Definition->Quantization.Apply(Raw);
        Replacement.New.Provenance.GeneratedItemLevel = ItemLevel;
        Replacement.New.Provenance.DefinitionRevision = Definition->Revision;
        if (!FMath::IsFinite(Replacement.New.Magnitude)
            || (MythicAffix::ModifierRequiresNonZeroMagnitude(Definition->ModifierOp)
                && FMath::IsNearlyZero(Replacement.New.Magnitude))
            || !Replacement.New.IsGameplayValid()) return false;
    }
    if (Replacements.IsEmpty()) {
        // A fully locked item has no legal reroll mutation. Report failure so the forge control layer cannot charge
        // currency for a successful-looking no-op.
        return false;
    }

    TArray<FRolledAffix> ProposedSnapshots;
    ProposedSnapshots.Reserve(AffixSnapshots.Items.Num());
    for (const FMythicReplicatedAffixItem &Item : AffixSnapshots.Items) {
        ProposedSnapshots.Add(Item.Affix);
    }
    for (const FReplacement &Replacement : Replacements) {
        ProposedSnapshots[Replacement.Index] = Replacement.New;
    }

    UMythicInventoryComponent *Inventory = OwningItem ? OwningItem->GetInventoryComponent() : nullptr;
    FMythicInventorySlotEntry Slot;
    const bool bEquipped = Inventory
        && Inventory->GetSlotEntry(OwningItem->GetSlot(), Slot)
        && Slot.SlottedItemInstance == OwningItem
        && Slot.IsGearSlot();
    if (bEquipped
        && !Inventory->ReconcileEquippedAffixSnapshotMutationTransactional(OwningItem, ProposedSnapshots)) {
        return false;
    }

    // The only fallible operation was the complete GAS transition above. Publish replicated/save snapshots last.
    for (const FReplacement &Replacement : Replacements) {
        FMythicReplicatedAffixItem &Item = AffixSnapshots.Items[Replacement.Index];
        Item.Affix = Replacement.New;
        AffixSnapshots.MarkItemDirty(Item);
    }
    RefreshOwningInventoryPresentation(false);
    return true;
}

void UAffixesFragment::SetAffixLocked(int32 AffixIndex, bool bLocked) {
    if (!HasServerAuthority()) return;
    FText Reason; if (!CanApplyCraftOp(Reason)) return;
    if (AffixSnapshots.Items.IsValidIndex(AffixIndex)) {
        FMythicReplicatedAffixItem &Item = AffixSnapshots.Items[AffixIndex];
        if (Item.Affix.bIsLocked != bLocked) {
            FRolledAffix Replacement = Item.Affix;
            Replacement.bIsLocked = bLocked;
            Replacement.Provenance.MutationRevision++;
            Item.Affix = MoveTemp(Replacement);
            AffixSnapshots.MarkItemDirty(Item);
            RefreshOwningInventoryPresentation();
        }
    }
}

bool UAffixesFragment::CanApplyCraftOp(FText &OutReason) const {
    if (AffixesRuntimeReplicatedData.bCorrupted) {
        OutReason = NSLOCTEXT("Mythic", "CraftRefusedCorrupted", "This item is corrupted and can no longer be crafted.");
        return false;
    }
    OutReason = FText::GetEmpty(); return true;
}

void UAffixesFragment::ServerCorruptItem() {
    if (!HasServerAuthority()) return;
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    if (Settings && !Settings->bItemCorruptionEnabled) return;
    AffixesRuntimeReplicatedData.bCorrupted = true;
    RefreshOwningInventoryPresentation();
}

void UAffixesFragment::OnRep_AffixesConfig() { AffixSnapshots.SetOwner(this); RefreshOwningInventoryPresentation(); }
void UAffixesFragment::OnAffixSnapshotsReplicated() { AffixSnapshots.SetOwner(this); RefreshOwningInventoryPresentation(); }

void UAffixesFragment::RefreshOwningInventoryPresentation(const bool bReconcileAffixes) const {
    UMythicItemInstance *Item = GetOwningItemInstance();
    UMythicInventoryComponent *Inventory = Item ? Item->GetInventoryComponent() : nullptr;
    if (Inventory && Item->GetSlot() != INDEX_NONE) {
        Inventory->NotifyItemInstanceUpdated(Item->GetSlot(), bReconcileAffixes);
    }
}

bool UAffixesFragment::CanBeStackedWith(const UItemFragment *Other) const {
    if (!Super::CanBeStackedWith(Other)) return false;
    const UAffixesFragment *OtherAffixes = Cast<UAffixesFragment>(Other);
    if (!OtherAffixes || AffixesConfig.AffixProfile != OtherAffixes->AffixesConfig.AffixProfile
        || AffixSnapshots.Items.Num() != OtherAffixes->AffixSnapshots.Items.Num()) return false;
    TArray<bool> Used; Used.Init(false, OtherAffixes->AffixSnapshots.Items.Num());
    for (const FMythicReplicatedAffixItem &Ours : AffixSnapshots.Items) {
        bool bMatched = false;
        for (int32 Index = 0; Index < OtherAffixes->AffixSnapshots.Items.Num(); ++Index) {
            if (!Used[Index] && SnapshotsEqual(Ours.Affix, OtherAffixes->AffixSnapshots.Items[Index].Affix)) {
                Used[Index] = true; bMatched = true; break;
            }
        }
        if (!bMatched) return false;
    }
    return AffixesRuntimeReplicatedData.bCorrupted == OtherAffixes->AffixesRuntimeReplicatedData.bCorrupted;
}

#if WITH_EDITOR
bool UAffixesFragment::IsValidFragment(FText &OutErrorMessage) const {
    if (!AffixesConfig.AffixProfile.IsValid()
        || AffixesConfig.AffixProfile.GetPrimaryAssetId().PrimaryAssetType != UMythicAssetManager::AffixProfileType) {
        OutErrorMessage = FText::FromString(TEXT("AffixesFragment requires exactly one typed Affix Profile asset."));
        return false;
    }
    return Super::IsValidFragment(OutErrorMessage);
}
#endif
