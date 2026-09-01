
#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "MythicItemComparisonTypes.h"
#include "ItemComparisonVM.generated.h"

class UMythicItemInstance;
class UMythicInventoryComponent;

UCLASS()
class MYTHIC_API UItemComparisonVM : public UMVVMViewModelBase {
    GENERATED_BODY()

public:
    /** Display projection for the candidate item currently being inspected. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Comparison")
    UItemTooltipVM *InspectedItem = nullptr;

    /** Display projection for the item in the replacement slot, or null when that slot is empty. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Comparison")
    UItemTooltipVM *EquippedItem = nullptr;

    /**
     * Tag-union rows for canonical affix contributions plus weapon average hit, DPS, effective APS, and durability.
     * These are display projections; immutable affix snapshots and live semantic definitions remain authority.
     */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Comparison")
    TArray<FAttributeDiff> AttributeDiffs;

    /**
     * High-signal weapon summary built from the same canonical projection as the DPS block. This remains invalid
     * for non-weapons and does not replace the detailed affix contribution rows in AttributeDiffs.
     */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Comparison")
    FMythicWeaponAttackComparisonViewData WeaponAttackComparison;

    /**
     * Builds a display-only comparison against one explicitly chosen equipment slot. The expected-empty flag and
     * occupant GUID make stale presentation fail closed instead of silently comparing against a different item.
     */
    UFUNCTION(BlueprintCallable, Category="Mythic|Comparison")
    static UItemComparisonVM *CreateComparison(UObject *Outer, UMythicItemInstance *Inspected, UMythicInventoryComponent *Inventory,
                                               int32 TargetSlotIndex, bool bExpectEmpty,
                                               FGuid ExpectedTargetOccupantGuid);

    /**
     * Builds the atomic typed weapon comparison from two canonical item-local attack projections.
     * When bSuppressEmptyBaseline is true, a missing equipped projection retains candidate attack data but emits no
     * zero-baseline metric deltas.
     */
    static FMythicWeaponAttackComparisonViewData BuildWeaponAttackComparison(
        const FMythicWeaponAttackViewData &InspectedAttack,
        const FMythicWeaponAttackViewData &EquippedAttack,
        bool bSuppressEmptyBaseline = true);

    void SetInspectedItem(UItemTooltipVM *InInspectedItem);
    UItemTooltipVM *GetInspectedItem() const;
    void SetEquippedItem(UItemTooltipVM *InEquippedItem);
    UItemTooltipVM *GetEquippedItem() const;
    void SetAttributeDiffs(TArray<FAttributeDiff> InAttributeDiffs);
    TArray<FAttributeDiff> GetAttributeDiffs() const;
    void SetWeaponAttackComparison(FMythicWeaponAttackComparisonViewData InWeaponAttackComparison);
    FMythicWeaponAttackComparisonViewData GetWeaponAttackComparison() const;
};
