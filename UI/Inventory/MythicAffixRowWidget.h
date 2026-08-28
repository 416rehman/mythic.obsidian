// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Itemization/Inventory/ViewModels/ItemTooltipVM.h"
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

    /** Canonical, identity-safe data available to Blueprint presentation logic. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Mythic|Affixes")
    FMythicAffixViewData ViewData;

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

    /** Called after the canonical data and optional CommonTextBlock bindings have been updated. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Affixes",
              meta = (DisplayName = "On Affix Presentation Updated"))
    void OnAffixPresentationUpdated(const FMythicAffixViewData &InViewData);
};
