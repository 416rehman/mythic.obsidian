// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/MythicActivatableWidget.h"
#include "MythicSettingsPageWidget.generated.h"

class UButton;
class UCommonTextBlock;
class UMythicSettingsPageWidget;
class UPanelWidget;

UCLASS()
class MYTHIC_API UMythicSettingStepProxy : public UObject {
    GENERATED_BODY()

public:
    UPROPERTY()
    TWeakObjectPtr<UMythicSettingsPageWidget> Page;

    UPROPERTY()
    int32 RowIndex = INDEX_NONE;

    UPROPERTY()
    int32 Delta = 0;

    UFUNCTION()
    void HandleClicked();
};

USTRUCT()
struct FMythicSettingsRow {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UPanelWidget> Box;

    UPROPERTY()
    TObjectPtr<UCommonTextBlock> Label;

    UPROPERTY()
    TObjectPtr<UWidget> Left;

    UPROPERTY()
    TObjectPtr<UCommonTextBlock> Value;

    UPROPERTY()
    TObjectPtr<UWidget> Right;

    UPROPERTY()
    TObjectPtr<UMythicSettingStepProxy> LeftProxy;

    UPROPERTY()
    TObjectPtr<UMythicSettingStepProxy> RightProxy;

    UPROPERTY()
    TObjectPtr<UCommonTextBlock> Description;
};

UCLASS()
class MYTHIC_API UMythicSettingsPageWidget : public UMythicActivatableWidget {
    GENERATED_BODY()

public:
    void StepSetting(int32 RowIndex, int32 Delta);

    /** Commit staged video/scalability changes and write the ini. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void ApplyAndSave();

    /** Back to engine defaults, applied immediately so the player can see what they got. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void RestoreDefaults();

    /**
     * Category names in authored order, for the page's tab strip. Seven of them, which is past the three-or-four
     * a reader holds at once - which is exactly why they are tabs rather than one scrolling list.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    TArray<FText> GetCategoryNames() const;

    /** Shows one category. An index outside the list shows everything, which is the pre-tab behaviour. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetActiveCategory(int32 CategoryIndex);

    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    int32 GetActiveCategory() const { return ActiveCategory; }

protected:
    virtual void NativeConstruct() override;
    virtual void NativeOnActivated() override;

    /** Rows go here. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> SettingsList;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Apply;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Defaults;

    /** Says whether anything is waiting on Apply. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Txt_Status;

private:
    enum class ESettingControl : uint8 {
        Stepper,
        Slider,
        Toggle,
        Dropdown,
        Rebind,
        Action,
    };

    struct FSettingDef {
        FText Label;
        bool bHeading = false;

        // Which tab this belongs to. Inherited from the most recent Heading() when the definition is built.
        FText Category;
        TFunction<FText()> Read;
        TFunction<void(int32)> Step;
        bool bNeedsApply = false;

        ESettingControl Control = ESettingControl::Stepper;

        FText Description;

        TFunction<float()> ReadNormalised;
        TFunction<void(float)> SetNormalised;
        TFunction<FText()> ReadDisplay;
        float StepFraction = 0.05f;

        TFunction<int32()> OptionCount;
        TFunction<int32()> ReadIndex;
        TFunction<void(int32)> SetIndex;
        TFunction<FText(int32)> OptionLabel;

        TFunction<bool()> ReadBool;
        TFunction<void(bool)> SetBool;

        TFunction<void()> RestoreDefault;
        TFunction<bool()> IsAtDefault;

        TFunction<bool()> IsEnabled;
    };

    void BuildDefinitions();
    void Refresh();

    FMythicSettingsRow &GetOrCreateRow(int32 Index);

    UFUNCTION()
    void HandleApplyClicked();

    UFUNCTION()
    void HandleDefaultsClicked();

    TArray<FSettingDef> Definitions;

    UPROPERTY()
    TArray<FMythicSettingsRow> RowPool;

    bool bPendingApply = false;
    bool bBuilt = false;

    // Index into GetCategoryNames(). INDEX_NONE shows every category, the behaviour before tabs existed.
    int32 ActiveCategory = 0;
};
