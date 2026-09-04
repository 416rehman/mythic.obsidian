// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "FieldNotificationId.h"
#include "UI/ViewModels/MythicStatDisplay.h"
#include "MythicStatSheetWidget.generated.h"

class UMythicStatSheetViewModel;
class UBorder;
class UHorizontalBox;
class UImage;
class UVerticalBox;
class UPanelWidget;
class UTextBlock;

USTRUCT()
struct FMythicStatRowWidgets {
    GENERATED_BODY()

    /** The row's plate and its one hit-taking node. Draws nothing for derived rows, the kit plate for primaries. */
    UPROPERTY()
    TObjectPtr<UBorder> Backing;

    UPROPERTY()
    TObjectPtr<UHorizontalBox> Box;

    UPROPERTY()
    TObjectPtr<UTextBlock> Label;

    UPROPERTY()
    TObjectPtr<UVerticalBox> ValueBox;

    UPROPERTY()
    TObjectPtr<UImage> Bar;

    UPROPERTY()
    TObjectPtr<UTextBlock> Value;

    UPROPERTY()
    TObjectPtr<UTextBlock> Bonus;
};

USTRUCT()
struct FMythicSummaryCardWidgets {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UBorder> Plate;

    UPROPERTY()
    TObjectPtr<UImage> Icon;

    UPROPERTY()
    TObjectPtr<UTextBlock> Value;

    UPROPERTY()
    TObjectPtr<UTextBlock> Label;
};

USTRUCT()
struct FMythicStatTooltipLineWidgets {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UHorizontalBox> Box;

    UPROPERTY()
    TObjectPtr<UTextBlock> Label;

    UPROPERTY()
    TObjectPtr<UTextBlock> Value;

    UPROPERTY()
    TObjectPtr<UTextBlock> Diminished;
};

USTRUCT()
struct FMythicStatTooltipWidgets {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UBorder> Plate;

    UPROPERTY()
    TObjectPtr<UVerticalBox> Lines;

    UPROPERTY()
    TObjectPtr<UTextBlock> Title;

    UPROPERTY()
    TArray<FMythicStatTooltipLineWidgets> LinePool;
};

UCLASS()
class MYTHIC_API UMythicStatSheetWidget : public UCommonActivatableWidget {
    GENERATED_BODY()

public:
    /** Returns the live view model that owns the stat sheet's data and Ability System bindings. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Stats")
    UMythicStatSheetViewModel *GetStatSheetViewModel() const { return ViewModel; }

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeOnActivated() override;
    virtual void NativeOnDeactivated() override;
    virtual void SetVisibility(ESlateVisibility InVisibility) override;

    /** Rows are added here, top to bottom. The shipped Blueprint uses the VerticalBox in its ScrollBox. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> StatList;

    /** "12 stats modified" — one number that says the build is doing something before any of it is read. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> SummaryText;

    /** The headline card rail, above the list. Absent from the Blueprint means no cards, never an error. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> SummaryCards;

    /** One font for every row. A single size keeps the whole sheet inside one font-atlas entry. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Stats")
    FSlateFontInfo RowFont;

    /** Primaries read one step larger. The second — and last — atlas entry the sheet pays for. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Stats")
    FSlateFontInfo PrimaryRowFont;

    /** Sections that start closed. Later drawers a player opens on demand, not walls they scroll past. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Stats")
    TArray<FGameplayTag> DefaultCollapsedCategoryTags;

    /**
     * The house section header, shared with every other screen.
     *
     * The sheet used to draw its own: a stat row re-texted in its own font and colour. That made a third
     * heading style beside the settings screen's and the header component's, so three screens announcing a
     * group looked like three products.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Stats")
    TSubclassOf<class UMythicSectionHeader> SectionHeaderClass;

    /** Default text colour for stat labels. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Stats")
    FLinearColor LabelColor = FLinearColor(0.910f, 0.886f, 0.839f, 0.80f);

    /** Default text colour for final stat values. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Stats")
    FLinearColor ValueColor = FLinearColor(0.910f, 0.886f, 0.839f, 1.0f);

    /** Text colour used for beneficial current-minus-base deltas. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Stats")
    FLinearColor BonusColor = FLinearColor(0.788f, 0.663f, 0.416f, 1.0f);

    /** Text colour used for detrimental current-minus-base deltas. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Stats")
    FLinearColor PenaltyColor = FLinearColor(0.651f, 0.357f, 0.294f, 1.0f);

    /**
     * Rows built up front at construct. Widget creation is a frame spike, so the pool is filled once and then only
     * ever re-texted. Sized to cover the full sheet with every category expanded (~90 attributes plus headings), so
     * in practice the runtime path never has to grow it.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Stats", meta = (ClampMin = "0"))
    int32 PrewarmRowCount = 110;

private:
    void BindIfVisible();
    void Unbind();
    void HandleFieldChanged(UObject *Object, UE::FieldNotification::FFieldId FieldId);
    void ScheduleRebuild();
    void Rebuild();

    FMythicStatRowWidgets &GetOrCreateRow(int32 Index);

    void SetRowGap(FMythicStatRowWidgets &Row, float TopGap);

    class UMythicSectionHeader *GetOrCreateHeader(int32 Index);

    /** Re-parents pooled widgets only when the section shape changed - child order is the costliest invalidation. */
    void ReorderIfShapeChanged(const TArray<int32> &NewShape);
    void ApplyLine(FMythicStatRowWidgets &Row, const struct FMythicStatLine &Line);

    void BuildSummaryCardPool();
    void BuildTooltipPool();
    void ApplySummaries();
    void ApplyContributionTooltip(FMythicStatRowWidgets &Row, const struct FMythicStatLine &Line);

    UFUNCTION()
    void HandleSectionToggled(class UMythicSectionHeader *Header);

    UPROPERTY()
    TObjectPtr<UMythicStatSheetViewModel> ViewModel;

    UPROPERTY()
    TArray<FMythicStatRowWidgets> RowPool;

    UPROPERTY()
    TArray<TObjectPtr<class UMythicSectionHeader>> HeaderPool;

    UPROPERTY()
    TArray<FMythicSummaryCardWidgets> CardPool;

    UPROPERTY()
    TArray<FMythicStatTooltipWidgets> TooltipPool;

    /** Line count per section as last built. Same shape means re-text only. */
    TArray<int32> Shape;

    /** Category behind each pooled header, rebuilt every Rebuild, so a header click knows its drawer. */
    TArray<FGameplayTag> HeaderCategories;

    TSet<FGameplayTag> CollapsedSections;

    int32 UsedContributionTooltips = 0;

    bool bCollapseInitialized = false;

    bool bBound = false;

    // The view model publishes Sections, Summaries and ModifiedStatCount back to back, so an
    // uncoalesced handler rebuilds the whole sheet two or three times for one attribute change.
    bool bRebuildScheduled = false;
};
