
#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "ItemTooltipVM.h"
#include "ItemComparisonVM.generated.h"

class UMythicItemInstance;
class UMythicInventoryComponent;

/** One typed item-comparison row with canonical units and data-driven benefit direction. */
USTRUCT(BlueprintType)
struct FAttributeDiff {
    GENERATED_BODY()

    /** Stat.Attribute.* or ItemMetric.* identity. Localized labels are presentation only. */
    UPROPERTY(BlueprintReadOnly, Category="Mythic|Comparison")
    FGameplayTag ComparisonTag;

    /** Localized player-facing row label resolved from canonical stat or item-metric presentation data. */
    UPROPERTY(BlueprintReadOnly, Category="Mythic|Comparison")
    FText AttributeName;

    /** Comparable contribution supplied by the currently equipped item, expressed in the row's canonical units. */
    UPROPERTY(BlueprintReadOnly, Category="Mythic|Comparison")
    float CurrentValue = 0.0f;

    /** Comparable contribution supplied by the inspected item, expressed in the row's canonical units. */
    UPROPERTY(BlueprintReadOnly, Category="Mythic|Comparison")
    float NewValue = 0.0f;

    /** Signed NewValue minus CurrentValue; use bIsUpgrade for benefit because some stats prefer lower values. */
    UPROPERTY(BlueprintReadOnly, Category="Mythic|Comparison")
    float Delta = 0.0f;

    /** Data-driven benefit verdict using ComparisonDirection, not a hard-coded positive-delta assumption. */
    UPROPERTY(BlueprintReadOnly, Category="Mythic|Comparison")
    bool bIsUpgrade = false;

    /** Canonical direction from StatDefinition that determines whether higher or lower contributions are beneficial. */
    UPROPERTY(BlueprintReadOnly, Category="Mythic|Comparison")
    EMythicStatComparisonDirection ComparisonDirection = EMythicStatComparisonDirection::Neutral;

    /** Canonical no-contribution identity used when one compared item does not provide this stat or metric. */
    UPROPERTY(BlueprintReadOnly, Category="Mythic|Comparison")
    float NeutralValue = 0.0f;

    bool operator==(const FAttributeDiff& Other) const {
        return ComparisonTag == Other.ComparisonTag &&
               AttributeName.EqualTo(Other.AttributeName) &&
               FMath::IsNearlyEqual(CurrentValue, Other.CurrentValue) &&
               FMath::IsNearlyEqual(NewValue, Other.NewValue) &&
               FMath::IsNearlyEqual(Delta, Other.Delta) &&
               bIsUpgrade == Other.bIsUpgrade &&
               ComparisonDirection == Other.ComparisonDirection &&
               FMath::IsNearlyEqual(NeutralValue, Other.NeutralValue);
    }
};

/** Atomic weapon-specific comparison projection for high-signal AAA ARPG item cards. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicWeaponAttackComparisonViewData {
    GENERATED_BODY()

    /** True when the inspected item supplied a complete canonical weapon attack projection. */
    UPROPERTY(BlueprintReadOnly, Category="Mythic|Comparison|Weapon Attack")
    bool bIsValid = false;

    /** True when the replacement slot contains a weapon with a complete canonical attack projection. */
    UPROPERTY(BlueprintReadOnly, Category="Mythic|Comparison|Weapon Attack")
    bool bHasEquippedWeaponAttack = false;

    /** Canonical attack projection for the inspected candidate, identical in meaning to the item-details DPS block. */
    UPROPERTY(BlueprintReadOnly, Category="Mythic|Comparison|Weapon Attack")
    FMythicWeaponAttackViewData InspectedAttack;

    /** Canonical attack projection for the equipped item, or an invalid zero projection when the slot is empty. */
    UPROPERTY(BlueprintReadOnly, Category="Mythic|Comparison|Weapon Attack")
    FMythicWeaponAttackViewData EquippedAttack;

    /** Typed sustained-DPS comparison, with NewValue representing the inspected item and CurrentValue the equipped item. */
    UPROPERTY(BlueprintReadOnly, Category="Mythic|Comparison|Weapon Attack")
    FAttributeDiff DamagePerSecondComparison;

    /** Typed effective-APS comparison after the exact combat AttackSpeed clamp, never raw authored montage cadence. */
    UPROPERTY(BlueprintReadOnly, Category="Mythic|Comparison|Weapon Attack")
    FAttributeDiff EffectiveAttacksPerSecondComparison;

    bool operator==(const FMythicWeaponAttackComparisonViewData& Other) const {
        return bIsValid == Other.bIsValid &&
               bHasEquippedWeaponAttack == Other.bHasEquippedWeaponAttack &&
               InspectedAttack == Other.InspectedAttack &&
               EquippedAttack == Other.EquippedAttack &&
               DamagePerSecondComparison == Other.DamagePerSecondComparison &&
               EffectiveAttacksPerSecondComparison == Other.EffectiveAttacksPerSecondComparison;
    }
};

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
     * Tag-union rows for canonical affix contributions plus weapon DPS, effective APS, and durability metrics.
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
     * Builds a display-only comparison against the item the candidate would replace. TargetSlotIndex addresses
     * Inventory->GetAllSlots(); -1 prefers an empty compatible slot, then the first stable compatible slot.
     */
    UFUNCTION(BlueprintCallable, Category="Mythic|Comparison")
    static UItemComparisonVM *CreateComparison(UObject *Outer, UMythicItemInstance *Inspected, UMythicInventoryComponent *Inventory,
                                               int32 TargetSlotIndex = -1);

    /** Builds the atomic typed weapon comparison from two canonical item-local attack projections. */
    static FMythicWeaponAttackComparisonViewData BuildWeaponAttackComparison(
        const FMythicWeaponAttackViewData &InspectedAttack,
        const FMythicWeaponAttackViewData &EquippedAttack);

    void SetInspectedItem(UItemTooltipVM *InInspectedItem);
    UItemTooltipVM *GetInspectedItem() const;
    void SetEquippedItem(UItemTooltipVM *InEquippedItem);
    UItemTooltipVM *GetEquippedItem() const;
    void SetAttributeDiffs(TArray<FAttributeDiff> InAttributeDiffs);
    TArray<FAttributeDiff> GetAttributeDiffs() const;
    void SetWeaponAttackComparison(FMythicWeaponAttackComparisonViewData InWeaponAttackComparison);
    FMythicWeaponAttackComparisonViewData GetWeaponAttackComparison() const;
};
