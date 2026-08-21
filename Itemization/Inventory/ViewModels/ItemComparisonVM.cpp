
#include "ItemComparisonVM.h"

#include "ItemTooltipVM.h"
#include "MythicStatDelta.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/InventorySlotDefinition.h"

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

void UItemComparisonVM::SetUpgradeScore(int32 InUpgradeScore) {
    if (UE_MVVM_SET_PROPERTY_VALUE(UpgradeScore, InUpgradeScore)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(UpgradeScore);
    }
}

int32 UItemComparisonVM::GetUpgradeScore() const { return UpgradeScore; }

void UItemComparisonVM::SetIsUpgradeOverall(bool bInIsUpgradeOverall) {
    if (UE_MVVM_SET_PROPERTY_VALUE(bIsUpgradeOverall, bInIsUpgradeOverall)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bIsUpgradeOverall);
    }
}

bool UItemComparisonVM::GetIsUpgradeOverall() const { return bIsUpgradeOverall; }

static void BuildComparableStats(const UItemTooltipVM *Tooltip, TArray<FMythicComparableStat> &OutStats) {
    if (!Tooltip) {
        return;
    }
    if (Tooltip->GetDamageMax() > 0.0f) {
        OutStats.Emplace(FName(TEXT("Base.DamageMin")), NSLOCTEXT("MythicComparison", "DamageMin", "Minimum Damage"), Tooltip->GetDamageMin());
        OutStats.Emplace(FName(TEXT("Base.DamageMax")), NSLOCTEXT("MythicComparison", "DamageMax", "Maximum Damage"), Tooltip->GetDamageMax());
    }
    if (Tooltip->GetAttackSpeed() > 0.0f) {
        OutStats.Emplace(FName(TEXT("Base.AttackSpeed")), NSLOCTEXT("MythicComparison", "AttackSpeed", "Attack Speed"), Tooltip->GetAttackSpeed());
    }
    if (Tooltip->GetMaxDurability() > 0.0f) {
        OutStats.Emplace(FName(TEXT("Base.MaxDurability")), NSLOCTEXT("MythicComparison", "MaxDurability", "Durability"), Tooltip->GetMaxDurability());
    }
    for (const FAffixDisplayData &Affix : Tooltip->GetAffixes()) {
        OutStats.Emplace(FName(*Affix.AttributeName.ToString()), Affix.AttributeName, Affix.Value, Affix.bLowerIsBetter, Affix.bIsPercentage);
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
            if (!Entry.bEquipmentSlot || !Entry.SlotDefinition) {
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
            int32 BestScore = TNumericLimits<int32>::Min();
            for (const FMythicInventorySlotEntry &Entry : AllSlots) {
                if (!SlotAcceptsItem(Entry)) {
                    continue;
                }
                bFoundCandidateSlot = true;
                if (!Entry.SlottedItemInstance) {
                    EquippedInstance = nullptr;
                    break;
                }
                UItemTooltipVM *CandidateTooltip = UItemTooltipVM::CreateFromItemInstance(VM, Entry.SlottedItemInstance);
                TArray<FMythicComparableStat> CandidateStats;
                BuildComparableStats(CandidateTooltip, CandidateStats);
                const int32 Score = FMythicStatDeltaCore::ComputeUpgradeScore(FMythicStatDeltaCore::ComputeDiffs(InspectedStats, CandidateStats));
                if (Score > BestScore) {
                    BestScore = Score;
                    EquippedInstance = Entry.SlottedItemInstance;
                }
            }
        }
    }

    if (bFoundCandidateSlot) {
        TArray<FMythicComparableStat> EquippedStats;
        if (EquippedInstance) {
            UItemTooltipVM *EquippedTooltip = UItemTooltipVM::CreateFromItemInstance(VM, EquippedInstance);
            VM->SetEquippedItem(EquippedTooltip);
            BuildComparableStats(EquippedTooltip, EquippedStats);
        }
        const TArray<FAttributeDiff> Diffs = FMythicStatDeltaCore::ComputeDiffs(InspectedStats, EquippedStats);
        const int32 Score = FMythicStatDeltaCore::ComputeUpgradeScore(Diffs);
        VM->SetAttributeDiffs(Diffs);
        VM->SetUpgradeScore(Score);
        VM->SetIsUpgradeOverall(Score > 0);
    }

    return VM;
}
