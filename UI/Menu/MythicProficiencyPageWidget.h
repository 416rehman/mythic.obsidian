// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "GameplayTagContainer.h"
#include "MythicProficiencyPageWidget.generated.h"

class UImage;
class UMaterialInstanceDynamic;
class UPanelWidget;
class UTextBlock;
struct FGameplayEventData;
struct FProficiencySummary;

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
};

UCLASS()
class MYTHIC_API UMythicProficiencyPageWidget : public UCommonActivatableWidget {
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    virtual void NativeOnActivated() override;
    virtual void NativeOnDeactivated() override;

    /** Rows are added here. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> TrackList;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Txt_Empty;

    /** Bar material. Needs a scalar "Percent"; the shared UI progress material already provides one. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Proficiency")
    TObjectPtr<UMaterialInterface> BarMaterialAsset;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Proficiency")
    FLinearColor BarFillStart = FLinearColor(0.62f, 0.74f, 0.40f, 1.0f);

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Proficiency")
    FLinearColor BarFillEnd = FLinearColor(0.32f, 0.44f, 0.20f, 1.0f);

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Proficiency")
    FLinearColor NameColor = FLinearColor(0.93f, 0.88f, 0.76f, 1.0f);

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Proficiency")
    FLinearColor SubtleColor = FLinearColor(0.66f, 0.60f, 0.50f, 1.0f);

    /** Tracks are a fixed, small set; this covers them without ever growing at runtime. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Proficiency", meta = (ClampMin = "0"))
    int32 PrewarmRowCount = 16;

    /** The bar's slot fills the row, so this is only its minimum — the track grows with the page. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Proficiency")
    float BarWidth = 220.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Proficiency")
    float BarHeight = 14.0f;

    /** Fixed column widths keep every track's bar starting and ending on the same two pixels down the whole list. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Proficiency")
    float NameColumnWidth = 170.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Proficiency")
    float LevelColumnWidth = 52.0f;

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

private:
    void Refresh();
    void Bind();
    void Unbind();
    void HandleProficiencyEvent(FGameplayTag Tag, const FGameplayEventData *Payload);

    FMythicProficiencyRow &GetOrCreateRow(int32 Index);
    void ApplyRow(FMythicProficiencyRow &Row, const FProficiencySummary &Summary);

    UPROPERTY()
    TArray<FMythicProficiencyRow> RowPool;

    FDelegateHandle EventHandle;
    bool bBound = false;
};
