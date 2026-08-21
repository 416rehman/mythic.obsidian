// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "FieldNotification/FieldId.h"
#include "MythicStatSheetWidget.generated.h"

class UMythicStatSheetViewModel;
class UHorizontalBox;
class UImage;
class UOverlay;
class UPanelWidget;
class UTextBlock;

USTRUCT()
struct FMythicStatRowWidgets {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UHorizontalBox> Box;

    UPROPERTY()
    TObjectPtr<UTextBlock> Label;

    UPROPERTY()
    TObjectPtr<UOverlay> ValueBox;

    UPROPERTY()
    TObjectPtr<UImage> Bar;

    UPROPERTY()
    TObjectPtr<UTextBlock> Value;

    UPROPERTY()
    TObjectPtr<UTextBlock> Bonus;
};

UCLASS()
class MYTHIC_API UMythicStatSheetWidget : public UCommonActivatableWidget {
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Mythic|Stats")
    UMythicStatSheetViewModel *GetStatSheetViewModel() const { return ViewModel; }

    /** Progressive disclosure toggle — wire to a checkbox or a controller face button. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Stats")
    void ToggleShowUnmodified();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeOnActivated() override;
    virtual void NativeOnDeactivated() override;
    virtual void SetVisibility(ESlateVisibility InVisibility) override;

    /** Rows are added here. The shipped Blueprint uses the VerticalBox inside its ScrollBox. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> StatList;

    /** "12 stats modified" — one number that says the build is doing something before any of it is read. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> SummaryText;

    /** One font for every row. A single size keeps the whole sheet inside one font-atlas entry. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Stats")
    FSlateFontInfo RowFont;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Stats")
    FSlateFontInfo HeadingFont;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Stats")
    FLinearColor HeadingColor = FLinearColor(0.381326f, 0.234551f, 0.088656f, 1.0f);

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Stats")
    FLinearColor LabelColor = FLinearColor(0.72f, 0.66f, 0.55f, 1.0f);

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Stats")
    FLinearColor ValueColor = FLinearColor(0.94f, 0.90f, 0.82f, 1.0f);

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Stats")
    FLinearColor BonusColor = FLinearColor(0.45f, 0.72f, 0.42f, 1.0f);

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Stats")
    FLinearColor PenaltyColor = FLinearColor(0.78f, 0.35f, 0.30f, 1.0f);

    /**
     * Space above each category heading. Without it the sheet is one unbroken column of ~30 lines and a player
     * cannot see where Vitality ends and Offense begins.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Stats", meta = (ClampMin = "0"))
    float HeadingTopPadding = 10.0f;

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
    void Rebuild();

    FMythicStatRowWidgets &GetOrCreateRow(int32 Index);

    void SetRowGap(FMythicStatRowWidgets &Row, float TopGap);

    void ApplyHeading(FMythicStatRowWidgets &Row, const FText &Heading);
    void ApplyLine(FMythicStatRowWidgets &Row, const struct FMythicStatLine &Line);

    UPROPERTY()
    TObjectPtr<UMythicStatSheetViewModel> ViewModel;

    UPROPERTY()
    TArray<FMythicStatRowWidgets> RowPool;

    bool bBound = false;
};
