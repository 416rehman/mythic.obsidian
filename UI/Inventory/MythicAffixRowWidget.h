// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Itemization/Inventory/ViewModels/MythicItemComparisonTypes.h"
#include "MythicAffixRowWidget.generated.h"

class UCommonTextBlock;

/**
 * Canonical C++ presentation base for an affix row.
 *
 * The affix view-data builder remains the sole authority for localized labels,
 * modifier-aware values, and ranges. This widget only lays those display-ready
 * values out for the existing WBP_Affix contract.
 */
UCLASS(Abstract, Blueprintable)
class MYTHIC_API UMythicAffixRowWidget : public UUserWidget {
    GENERATED_BODY()

public:
    /** Updates this row from the tooltip VM's compatibility wrapper. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Affixes")
    void SetFromAffixDisplayData(const FAffixDisplayData &InDisplayData);

    /**
     * Atomically presents one ordinary or baseline-only affix row and its zero-or-more inline stat-channel deltas.
     * Multi-channel rows retain each typed diff for Blueprint layouts that can render independent chips.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Affixes|Comparison")
    void SetPresentation(const FMythicAffixRowPresentation &InPresentation);

    /** Clears all transient comparison text, color, glyph, and accessibility state while retaining the affix row. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Affixes|Comparison")
    void ClearDeltaPresentation();

    /** Canonical, identity-safe data available to Blueprint presentation logic. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Mythic|Affixes")
    FMythicAffixViewData ViewData;

    /** Last complete ordinary/baseline-only row projection consumed by this pooled widget. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Mythic|Affixes|Comparison")
    FMythicAffixRowPresentation Presentation;

protected:
    /** Player-facing affix name/template, with the legacy attribute label as a one-release fallback. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Attribute;

    /** Rolled value, or labelled values for a multi-channel affix. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Roll;

    /** Authored roll range, or labelled ranges for a multi-channel affix. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> RollRange;

    /** Optional compact signed delta text; collapsed when the canonical diff renders equal. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes|Comparison", meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> DeltaText;

    /** Optional up/down movement glyph; color communicates benefit independently from direction. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes|Comparison", meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> MovementIcon;

    /** Optional equipped value shown beside the ordinary candidate value or a baseline-only loss row. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes|Comparison", meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> EquippedBaselineText;

    /** Optional non-color comparison summary available to accessible/native fallback layouts. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes|Comparison", meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> ComparisonAccessibleText;

    /** Called after the canonical data and optional CommonTextBlock bindings have been updated. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Affixes",
              meta = (DisplayName = "On Affix Presentation Updated"))
    void OnAffixPresentationUpdated(const FMythicAffixViewData &InViewData);

    /** Called after all typed comparison data and optional native fallback bindings are updated. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Affixes|Comparison",
              meta = (DisplayName = "On Affix Comparison Updated"))
    void OnAffixComparisonUpdated(const FMythicAffixRowPresentation &InPresentation);

private:
    uint32 PresentationMutationSerial = 0;
};
