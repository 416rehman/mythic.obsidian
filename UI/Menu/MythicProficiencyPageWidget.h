// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "GameplayTagContainer.h"
#include "MythicProficiencyPageWidget.generated.h"

class UImage;
class UMaterialInstanceDynamic;
class UMythicSectionHeader;
class UPanelWidget;
class UTextBlock;
class UUniformGridPanel;
class UVerticalBox;
struct FGameplayEventData;
struct FProficiencySummary;

/** One authored family of tracks: the label the section announces, and the tracks that belong under it. */
USTRUCT()
struct FMythicProficiencyFamily {
    GENERATED_BODY()

    /** Localized section heading shown above this family of proficiency tracks. */
    UPROPERTY(EditDefaultsOnly, Category = "Family")
    FText Label;

    /** Track tags assigned to this family in player-facing display order. */
    UPROPERTY(EditDefaultsOnly, Category = "Family", meta = (Categories = "Proficiency"))
    TArray<FGameplayTag> Tracks;
};

USTRUCT()
struct FMythicProficiencyRow {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UPanelWidget> Box;

    UPROPERTY()
    TObjectPtr<UTextBlock> Name;

    UPROPERTY()
    TObjectPtr<UImage> Icon;

    UPROPERTY()
    TObjectPtr<UTextBlock> Level;

    UPROPERTY()
    TObjectPtr<UTextBlock> Progress;

    UPROPERTY()
    TObjectPtr<UTextBlock> Milestone;

    UPROPERTY()
    TObjectPtr<UImage> Bar;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> BarMaterial;

    /** Which family grid and cell this card currently sits in, so a refresh re-parents only on change. */
    int32 FamilyIndex = INDEX_NONE;

    int32 CellIndex = INDEX_NONE;
};

UCLASS()
class MYTHIC_API UMythicProficiencyPageWidget : public UCommonActivatableWidget {
    GENERATED_BODY()

public:
    UMythicProficiencyPageWidget();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeOnActivated() override;
    virtual void NativeOnDeactivated() override;

    /** Optional Blueprint panel that receives the generated proficiency family sections and track rows. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> TrackList;

    /** Optional empty-state label shown when the player has no valid proficiency summaries. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Txt_Empty;

    /** Bar material. Needs a scalar "Percent"; the shared UI progress material already provides one. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Proficiency")
    TObjectPtr<UMaterialInterface> BarMaterialAsset;

    /** Progress-bar fill color at zero progress. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Proficiency")
    FLinearColor BarFillStart = FLinearColor(0.62f, 0.74f, 0.40f, 1.0f);

    /** Progress-bar fill color at full progress. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Proficiency")
    FLinearColor BarFillEnd = FLinearColor(0.32f, 0.44f, 0.20f, 1.0f);

    /** Text color used for proficiency track names. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Proficiency")
    FLinearColor NameColor = FLinearColor(0.93f, 0.88f, 0.76f, 1.0f);

    /** Low-emphasis text color used for levels and progress details. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Proficiency")
    FLinearColor SubtleColor = FLinearColor(0.66f, 0.60f, 0.50f, 1.0f);

    /** Tracks are a fixed, small set; this covers them without ever growing at runtime. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Proficiency", meta = (ClampMin = "0"))
    int32 PrewarmRowCount = 16;

    /** Columns the track grid uses. The grid sizes its own cells, so this is a shape, not a width. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Proficiency", meta = (ClampMin = "1", ClampMax = "6"))
    int32 TrackColumns = 4;

    /** The bar's slot fills the row, so this is only its minimum — the track grows with the page. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Proficiency")
    float BarWidth = 220.0f;

    /** Fixed row height of each progress bar in slate units. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Proficiency")
    float BarHeight = 14.0f;

    /** Fixed column widths keep every track's bar starting and ending on the same two pixels down the whole list. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Proficiency")
    float NameColumnWidth = 170.0f;

    /** Fixed width reserved for the numeric level column. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Proficiency")
    float LevelColumnWidth = 52.0f;

    /** Fixed width reserved for current-versus-required XP text. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Proficiency")
    float ProgressColumnWidth = 76.0f;

    /** Milestone column. Wide enough for a name plus its level without wrapping the common cases. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Proficiency")
    float MilestoneColumnWidth = 210.0f;

    /** Milestone text. Warmer than the XP figure so the reward reads as the reward. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Proficiency")
    FLinearColor MilestoneColor = FLinearColor(0.86f, 0.70f, 0.40f, 1.0f);

    /** Air between tracks. Set once when a row is built; a refresh never touches layout. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Proficiency")
    float RowGap = 7.0f;

    /**
     * The families the page groups tracks under, in display order. A track no family claims lands in a
     * trailing "More" section rather than vanishing, so new content is visible before it is curated.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Proficiency")
    TArray<FMythicProficiencyFamily> Families;

    /** The house section header, shared with the stat sheet and the character page. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Proficiency")
    TSubclassOf<UMythicSectionHeader> SectionHeaderClass;

private:
    void Refresh();
    void Bind();
    void Unbind();
    void HandleProficiencyEvent(FGameplayTag Tag, const FGameplayEventData *Payload);

    FMythicProficiencyRow &GetOrCreateRow(int32 Index);
    void ApplyRow(FMythicProficiencyRow &Row, const FProficiencySummary &Summary);

    UPROPERTY()
    TArray<FMythicProficiencyRow> RowPool;

    void EnsureSections();

    UPROPERTY()
    TObjectPtr<UVerticalBox> SectionStack;

    UPROPERTY()
    TArray<TObjectPtr<UMythicSectionHeader>> SectionHeaders;

    UPROPERTY()
    TArray<TObjectPtr<UUniformGridPanel>> FamilyGrids;

    FDelegateHandle EventHandle;
    bool bBound = false;
};
