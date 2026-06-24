// 

#include "ItemComparisonVM.h"

#include "ItemTooltipVM.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/InventorySlotDefinition.h"
#include "Itemization/Inventory/Fragments/Passive/AffixesFragment.h"

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

// build a name-keyed map of affix values from a tooltip's affix display data
static TMap<FString, const FAffixDisplayData *> BuildAffixMap(const TArray<FAffixDisplayData> &Affixes) {
    TMap<FString, const FAffixDisplayData *> Map;
    for (const FAffixDisplayData &Affix : Affixes) {
        Map.Add(Affix.AttributeName.ToString(), &Affix);
    }
    return Map;
}

UItemComparisonVM *UItemComparisonVM::CreateComparison(UObject *Outer, UMythicItemInstance *Inspected, UMythicInventoryComponent *Inventory) {
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

    // find the matching equipment slot for this item's type in the inventory
    UMythicItemInstance *EquippedInstance = nullptr;
    if (Inventory) {
        FGameplayTagContainer InspectedProbe;
        Inspected->GetTypeProbe(InspectedProbe);

        const auto &AllSlots = Inventory->GetAllSlots();
        for (const FMythicInventorySlotEntry &Entry : AllSlots) {
            if (!Entry.bEquipmentSlot) {
                continue;
            }
            if (!Entry.SlotDefinition) {
                continue;
            }

            // an empty whitelist accepts all types; otherwise match
            const FGameplayTagContainer &Whitelist = Entry.SlotDefinition->WhitelistedItemTypes;
            if (Whitelist.IsEmpty() || InspectedProbe.HasAny(Whitelist)) {
                EquippedInstance = Entry.SlottedItemInstance;
                break;
            }
        }
    }

    // build equipped tooltip if there is a matching equipped item
    if (EquippedInstance) {
        UItemTooltipVM *EquippedTooltip = UItemTooltipVM::CreateFromItemInstance(VM, EquippedInstance);
        VM->SetEquippedItem(EquippedTooltip);

        // diff the affix attributes
        TArray<FAttributeDiff> Diffs;

        auto InspectedMap = BuildAffixMap(InspectedTooltip->GetAffixes());
        auto EquippedMap = BuildAffixMap(EquippedTooltip->GetAffixes());

        // collect all unique attribute names from both sides
        TSet<FString> AllNames;
        for (auto &Pair : InspectedMap) {
            AllNames.Add(Pair.Key);
        }
        for (auto &Pair : EquippedMap) {
            AllNames.Add(Pair.Key);
        }

        for (const FString &AttrName : AllNames) {
            FAttributeDiff Diff;
            Diff.AttributeName = FText::FromString(AttrName);

            const FAffixDisplayData *const *EquippedAffix = EquippedMap.Find(AttrName);
            const FAffixDisplayData *const *InspectedAffix = InspectedMap.Find(AttrName);

            Diff.CurrentValue = EquippedAffix ? (*EquippedAffix)->Value : 0.0f;
            Diff.NewValue = InspectedAffix ? (*InspectedAffix)->Value : 0.0f;
            Diff.Delta = Diff.NewValue - Diff.CurrentValue;

            // for "lower is better" attributes, a negative delta is an upgrade
            bool bLowerIsBetter = false;
            if (InspectedAffix) {
                bLowerIsBetter = (*InspectedAffix)->bLowerIsBetter;
            }
            else if (EquippedAffix) {
                bLowerIsBetter = (*EquippedAffix)->bLowerIsBetter;
            }

            Diff.bIsUpgrade = bLowerIsBetter
                ? (Diff.Delta < 0.0f)
                : (Diff.Delta > 0.0f);

            Diffs.Add(Diff);
        }

        VM->SetAttributeDiffs(Diffs);
    }

    return VM;
}
