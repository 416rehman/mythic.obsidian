// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Itemization/Inventory/ViewModels/MythicItemComparisonTypes.h"
#include "MythicDPSWidget.generated.h"

class UCommonTextBlock;

/**
 * Typed native presentation base for the dedicated weapon attack block in item details.
 *
 * This widget owns no combat math and holds no item or fragment reference. It renders one atomic, display-ready
 * projection so Blueprint layout can evolve without becoming a second gameplay authority.
 */
UCLASS(Abstract, Blueprintable)
class MYTHIC_API UMythicDPSWidget : public UUserWidget {
    GENERATED_BODY()

public:
    /** Replaces every displayed attack metric from one canonical item-local projection. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Item Details|Attack")
    void SetAttackDisplayData(const FMythicWeaponAttackViewData &InAttackDisplayData);

    /** Clears all displayed attack metrics when the details card no longer represents an eligible weapon. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Item Details|Attack")
    void ClearAttackDisplayData();

    /**
     * Adds inline DPS, effective-APS, and average-hit deltas to the already-presented candidate attack block.
     * The candidate projection must match AttackDisplayData or comparison is rejected as stale.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Item Details|Attack|Comparison")
    void SetAttackComparisonData(
        const FMythicWeaponAttackComparisonViewData &InComparisonData);

    /** Clears transient equipped values and delta glyphs without clearing the ordinary candidate attack block. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Item Details|Attack|Comparison")
    void ClearAttackComparisonData();

    /** Last canonical attack projection rendered by this pooled widget; invalid after it is cleared. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Mythic|Item Details|Attack")
    FMythicWeaponAttackViewData AttackDisplayData;

    /** Last validated inline attack comparison; invalid whenever the candidate or target changes. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient,
              Category = "Mythic|Item Details|Attack|Comparison")
    FMythicWeaponAttackComparisonViewData AttackComparisonData;

protected:
    /** Required text block that presents item-local sustained damage per second. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UCommonTextBlock> DamagePerSecondText;

    /** Required text block that presents composed item-local damage dealt by one hit. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UCommonTextBlock> DamagePerHitText;

    /** Required text block that presents effective item-local attacks per second. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UCommonTextBlock> AttacksPerSecondText;

    /** Optional signed sustained-DPS delta shown beside the existing candidate DPS value. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Item Details|Attack|Comparison",
              meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> DamagePerSecondDeltaText;

    /** Optional equipped sustained-DPS baseline. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Item Details|Attack|Comparison",
              meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> DamagePerSecondBaselineText;

    /** Optional numeric movement glyph for sustained DPS. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Item Details|Attack|Comparison",
              meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> DamagePerSecondMovementIcon;

    /** Optional signed effective-attacks-per-second delta. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Item Details|Attack|Comparison",
              meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> AttacksPerSecondDeltaText;

    /** Optional equipped effective-attacks-per-second baseline. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Item Details|Attack|Comparison",
              meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> AttacksPerSecondBaselineText;

    /** Optional numeric movement glyph for effective attacks per second. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Item Details|Attack|Comparison",
              meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> AttacksPerSecondMovementIcon;

    /** Optional signed expected average-hit delta attached to the ordinary min/max damage range. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Item Details|Attack|Comparison",
              meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> AverageDamagePerHitDeltaText;

    /** Optional equipped expected average-hit baseline. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Item Details|Attack|Comparison",
              meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> AverageDamagePerHitBaselineText;

    /** Optional numeric movement glyph for expected average damage per hit. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Item Details|Attack|Comparison",
              meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> AverageDamagePerHitMovementIcon;

    /** Optional combined non-color description of all visible attack deltas. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Item Details|Attack|Comparison",
              meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> AttackComparisonAccessibleText;

    /** Called after native text bindings and AttackDisplayData have been updated atomically. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Item Details|Attack",
              meta = (DisplayName = "On Attack Presentation Updated"))
    void OnAttackPresentationUpdated(const FMythicWeaponAttackViewData &InAttackDisplayData);

    /** Called after the typed attack comparison and every optional native fallback binding are updated. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Item Details|Attack|Comparison",
              meta = (DisplayName = "On Attack Comparison Updated"))
    void OnAttackComparisonUpdated(
        const FMythicWeaponAttackComparisonViewData &InComparisonData);

private:
    void ClearAttackComparisonDataInternal();
    uint32 PresentationMutationSerial = 0;
};
