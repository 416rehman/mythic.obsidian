
#include "ItemComparisonVM.h"

#include "ItemTooltipVM.h"
#include "MythicStatDelta.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/InventorySlotDefinition.h"
#include "Itemization/Inventory/ViewModels/MythicTags_ItemMetrics.h"

namespace {

bool IsComparableWeaponAttack(const FMythicWeaponAttackViewData &Attack) {
    return Attack.bIsValid
        && FMath::IsFinite(Attack.MinimumDamagePerHit)
        && Attack.MinimumDamagePerHit >= 0.0f
        && FMath::IsFinite(Attack.MaximumDamagePerHit)
        && Attack.MaximumDamagePerHit >= Attack.MinimumDamagePerHit
        && FMath::IsFinite(Attack.AverageDamagePerHit)
        && Attack.AverageDamagePerHit >= Attack.MinimumDamagePerHit
        && Attack.AverageDamagePerHit <= Attack.MaximumDamagePerHit
        && FMath::IsFinite(Attack.AttackSpeedBonus)
        && FMath::IsFinite(Attack.BaseAttacksPerSecond)
        && Attack.BaseAttacksPerSecond > 0.0f
        && FMath::IsFinite(Attack.DamagePerSecond)
        && Attack.DamagePerSecond >= 0.0f
        && FMath::IsFinite(Attack.AttacksPerSecond)
        && Attack.AttacksPerSecond > 0.0f
        && FMath::IsFinite(Attack.AttackTimeSeconds)
        && Attack.AttackTimeSeconds > 0.0f;
}

void AppendWeaponAttackMetrics(const FMythicWeaponAttackViewData &Attack,
                               TArray<FMythicComparableStat> &OutStats) {
    if (!IsComparableWeaponAttack(Attack)) {
        return;
    }

    OutStats.Emplace(
        ITEM_METRIC_WEAPON_DAMAGE_PER_SECOND,
        NSLOCTEXT("MythicComparison", "WeaponDamagePerSecond", "Damage per Second"),
        Attack.DamagePerSecond);
    OutStats.Emplace(
        ITEM_METRIC_WEAPON_ATTACKS_PER_SECOND,
        NSLOCTEXT("MythicComparison", "WeaponAttacksPerSecond", "Attacks per Second"),
        Attack.AttacksPerSecond);
}

} // namespace

void UItemComparisonVM::SetInspectedItem(UItemTooltipVM *InInspectedItem) {
    if (UE_MVVM_SET_PROPERTY_VALUE(InspectedItem, InInspectedItem)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(InspectedItem);
    }
}

UItemTooltipVM *UItemComparisonVM::GetInspectedItem() const { return InspectedItem; }

void UItemComparisonVM::SetEquippedItem(UItemTooltipVM *InEquippedItem) {
    if (UE_MVVM_SET_PROPERTY_VALUE(EquippedItem, InEquippedItem)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(EquippedItem);
    }
}

UItemTooltipVM *UItemComparisonVM::GetEquippedItem() const { return EquippedItem; }

void UItemComparisonVM::SetAttributeDiffs(TArray<FAttributeDiff> InAttributeDiffs) {
    if (UE_MVVM_SET_PROPERTY_VALUE(AttributeDiffs, InAttributeDiffs)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(AttributeDiffs);
    }
}

TArray<FAttributeDiff> UItemComparisonVM::GetAttributeDiffs() const { return AttributeDiffs; }

void UItemComparisonVM::SetWeaponAttackComparison(
    FMythicWeaponAttackComparisonViewData InWeaponAttackComparison) {
    if (UE_MVVM_SET_PROPERTY_VALUE(WeaponAttackComparison, InWeaponAttackComparison)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(WeaponAttackComparison);
    }
}

FMythicWeaponAttackComparisonViewData UItemComparisonVM::GetWeaponAttackComparison() const {
    return WeaponAttackComparison;
}

FMythicWeaponAttackComparisonViewData UItemComparisonVM::BuildWeaponAttackComparison(
    const FMythicWeaponAttackViewData &InspectedAttack,
    const FMythicWeaponAttackViewData &EquippedAttack) {
    if (!IsComparableWeaponAttack(InspectedAttack)) {
        return FMythicWeaponAttackComparisonViewData();
    }

    FMythicWeaponAttackComparisonViewData Candidate;
    Candidate.InspectedAttack = InspectedAttack;
    Candidate.bHasEquippedWeaponAttack = IsComparableWeaponAttack(EquippedAttack);
    if (Candidate.bHasEquippedWeaponAttack) {
        Candidate.EquippedAttack = EquippedAttack;
    }

    TArray<FMythicComparableStat> InspectedStats;
    TArray<FMythicComparableStat> EquippedStats;
    AppendWeaponAttackMetrics(Candidate.InspectedAttack, InspectedStats);
    AppendWeaponAttackMetrics(Candidate.EquippedAttack, EquippedStats);

    bool bFoundDamagePerSecond = false;
    bool bFoundAttacksPerSecond = false;
    for (const FAttributeDiff &Diff : FMythicStatDeltaCore::ComputeDiffs(
             InspectedStats, EquippedStats)) {
        if (Diff.ComparisonTag == ITEM_METRIC_WEAPON_DAMAGE_PER_SECOND.GetTag()) {
            Candidate.DamagePerSecondComparison = Diff;
            bFoundDamagePerSecond = true;
        }
        else if (Diff.ComparisonTag == ITEM_METRIC_WEAPON_ATTACKS_PER_SECOND.GetTag()) {
            Candidate.EffectiveAttacksPerSecondComparison = Diff;
            bFoundAttacksPerSecond = true;
        }
    }
    if (!bFoundDamagePerSecond || !bFoundAttacksPerSecond) {
        return FMythicWeaponAttackComparisonViewData();
    }

    Candidate.bIsValid = true;
    return Candidate;
}

static void BuildComparableStats(const UItemTooltipVM *Tooltip, TArray<FMythicComparableStat> &OutStats) {
    if (!Tooltip) {
        return;
    }
    AppendWeaponAttackMetrics(Tooltip->GetWeaponAttack(), OutStats);
    if (Tooltip->GetMaxDurability() > 0.0f) {
        OutStats.Emplace(ITEM_METRIC_DURABILITY, NSLOCTEXT("MythicComparison", "MaxDurability", "Durability"), Tooltip->GetMaxDurability());
    }
    for (const FAffixDisplayData &Affix : Tooltip->GetAffixes()) {
        if (Affix.bOwnedByWeaponAttackPresentation) {
            continue;
        }
        for (const FMythicAffixValueViewData &Value : Affix.ViewData.Values) {
            OutStats.Emplace(Value.StatTag, Value.StatLabel, Value.ComparisonValue, Value.ContributionIdentity,
                             Value.ComparisonDirection, Affix.bIsPercentage);
        }
    }
}

UItemComparisonVM *UItemComparisonVM::CreateComparison(UObject *Outer, UMythicItemInstance *Inspected, UMythicInventoryComponent *Inventory,
                                                       int32 TargetSlotIndex) {
    if (!Outer || !Inspected) {
        return nullptr;
    }

    UItemDefinition *InspectedDef = Inspected->GetItemDefinition();
    if (!InspectedDef) {
        return nullptr;
    }

    UItemComparisonVM *VM = NewObject<UItemComparisonVM>(Outer);
    UItemTooltipVM *InspectedTooltip = UItemTooltipVM::CreateFromItemInstance(VM, Inspected);
    VM->SetInspectedItem(InspectedTooltip);

    TArray<FMythicComparableStat> InspectedStats;
    BuildComparableStats(InspectedTooltip, InspectedStats);

    UMythicItemInstance *EquippedInstance = nullptr;
    bool bFoundCandidateSlot = false;
    if (Inventory) {
        FGameplayTagContainer InspectedProbe;
        Inspected->GetTypeProbe(InspectedProbe);

        const TArray<FMythicInventorySlotEntry> &AllSlots = Inventory->GetAllSlots();
        auto SlotAcceptsItem = [&InspectedProbe](const FMythicInventorySlotEntry &Entry) {
            if (!Entry.IsGearSlot() || !Entry.SlotDefinition) {
                return false;
            }
            const FGameplayTagContainer &Whitelist = Entry.SlotDefinition->WhitelistedItemTypes;
            return Whitelist.IsEmpty() || InspectedProbe.HasAny(Whitelist);
        };

        if (AllSlots.IsValidIndex(TargetSlotIndex) && SlotAcceptsItem(AllSlots[TargetSlotIndex])) {
            EquippedInstance = AllSlots[TargetSlotIndex].SlottedItemInstance;
            bFoundCandidateSlot = true;
        }
        else {
            for (const FMythicInventorySlotEntry &Entry : AllSlots) {
                if (!SlotAcceptsItem(Entry)) {
                    continue;
                }
                bFoundCandidateSlot = true;
                if (!Entry.SlottedItemInstance) {
                    EquippedInstance = nullptr;
                    break;
                }
                if (!EquippedInstance) {
                    EquippedInstance = Entry.SlottedItemInstance;
                }
            }
        }
    }

    if (bFoundCandidateSlot) {
        TArray<FMythicComparableStat> EquippedStats;
        UItemTooltipVM *EquippedTooltip = nullptr;
        if (EquippedInstance) {
            EquippedTooltip = UItemTooltipVM::CreateFromItemInstance(VM, EquippedInstance);
            VM->SetEquippedItem(EquippedTooltip);
            BuildComparableStats(EquippedTooltip, EquippedStats);
        }
        const TArray<FAttributeDiff> Diffs = FMythicStatDeltaCore::ComputeDiffs(InspectedStats, EquippedStats);
        VM->SetAttributeDiffs(Diffs);

        const FMythicWeaponAttackViewData InspectedAttack = InspectedTooltip
            ? InspectedTooltip->GetWeaponAttack()
            : FMythicWeaponAttackViewData();
        const FMythicWeaponAttackViewData EquippedAttack = EquippedTooltip
            ? EquippedTooltip->GetWeaponAttack()
            : FMythicWeaponAttackViewData();
        VM->SetWeaponAttackComparison(
            BuildWeaponAttackComparison(InspectedAttack, EquippedAttack));
    }

    return VM;
}
