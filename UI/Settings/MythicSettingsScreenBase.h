#pragma once

#include "CoreMinimal.h"
#include "UI/MythicActivatableWidget.h"
#include "UI/Settings/MythicSettingDefinition.h"
#include "MythicSettingsScreenBase.generated.h"

class UCommonButtonBase;
class UCommonButtonGroupBase;
class UCommonTextBlock;
class UMythicSettingRowBase;
class UPanelWidget;
class UVerticalBox;
class UHorizontalBox;

/**
 * Behaviour for the settings screen. Constructs no widget trees: the Widget Blueprint owns the rail, the
 * list, the detail panel and every row. This class answers what to show and applies what changed.
 */
UCLASS(Abstract, Blueprintable)
class MYTHIC_API UMythicSettingsScreenBase : public UMythicActivatableWidget {
    GENERATED_BODY()

public:
    /** The catalog drives everything on screen. Without one the screen is empty by design, not by accident. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Settings")
    TSoftObjectPtr<UMythicSettingsCatalog> Catalog;

    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    UMythicSettingsCatalog *GetCatalog() const;

protected:
    /**
     * Apply and Restore Defaults are real CommonUI actions, not buttons with words next to them.
     *
     * Bound with ShowInActionBar, so the shell's CommonBoundActionBar draws them with the correct glyph for
     * whatever the player is holding, and the same press works on a pad without a cursor ever existing. A
     * hand-typed legend cannot do either, and goes stale the moment a key is rebound.
     */
    UFUNCTION()
    void HandleApplyAction();

    UFUNCTION()
    void HandleRestoreDefaultsAction();

    /**
     * Exclusive selection for the rail.
     *
     * SetIsSelected(false) is ignored by a CommonButtonBase that is neither toggleable nor in a group, so
     * every category the player visited stayed underlined and the rail showed several active tabs at once.
     * A button group owns that invariant instead of each button being asked to give up its own state.
     */
    UPROPERTY()
    TObjectPtr<UCommonButtonGroupBase> RailGroup;

    /** Enhanced Input actions for the two things this screen commits. Designer-assignable. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Settings|Input")
    TSoftObjectPtr<class UInputAction> ApplyInputAction;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Settings|Input")
    TSoftObjectPtr<class UInputAction> RestoreDefaultsInputAction;

    FInputActionBindingHandle ApplyBinding;
    FInputActionBindingHandle RestoreBinding;

public:

    /** Tabs, in authored order. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    TArray<FMythicSettingCategory> GetCategories() const;

    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    int32 GetActiveCategoryIndex() const { return ActiveCategory; }

    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetActiveCategoryIndex(int32 Index);

    /** Step to the next or previous tab, wrapping. The shoulder buttons land here. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void CycleCategory(int32 Delta);

    /**
     * The rows for a tab, already ordered by that category's authored group order, with a blank-SourceName
     * entry inserted before each group as its heading. One list, so a caller just walks it.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    TArray<FMythicSettingDefinition> GetRowsForCategory(int32 CategoryIndex) const;

    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    TArray<FMythicSettingDefinition> GetRowsForActiveCategory() const;

    /**
     * The row Blueprint that draws a control kind, or null when a setting of that kind would draw
     * nothing at all - the failure mode where a setting exists in data and never appears on screen.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    TSubclassOf<UMythicSettingRowBase> GetRowClassFor(EMythicSettingControl Control) const;

    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    TSubclassOf<UMythicSettingRowBase> GetGroupHeadingClass() const { return GroupHeadingClass; }

    /** True when a group heading should be drawn before this row. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    static bool IsGroupHeading(const FMythicSettingDefinition &Row);

    /** The description shown in the detail panel, for whichever row has focus. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetFocusedRow(const FMythicSettingDefinition &Row);

    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    FMythicSettingDefinition GetFocusedRow() const { return FocusedRow; }

    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    bool HasPendingApply() const { return bPendingApply; }

    /** A row staged a change that waits on Apply. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void MarkPendingApply();

    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void ApplyAndSave();

    /** Every setting back to its authored default. One loop over the catalog, so none can be missed. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void RestoreDefaults();

    /** Redraw hooks for the Blueprint. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Settings")
    void OnCategoryChanged();

    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Settings")
    void OnFocusedRowChanged();

    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Settings")
    void OnPendingApplyChanged();

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeOnActivated() override;
    virtual void NativeOnDeactivated() override;
    virtual UWidget *NativeGetDesiredFocusTarget() const override;

    /**
     * Row Blueprint per control kind. A setting with no entry here draws nothing, which is why
     * IsDataValid refuses a catalog that names a control the screen cannot draw.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Settings")
    TMap<EMythicSettingControl, TSubclassOf<UMythicSettingRowBase>> RowClasses;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Settings")
    TSubclassOf<UMythicSettingRowBase> GroupHeadingClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Settings")
    TSubclassOf<UCommonButtonBase> TabButtonClass;

    /** Name of the text widget inside the tab button that carries its label, as the menu shell does it. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Settings")
    FName TabLabelWidgetName = TEXT("Text_ActionName");

    /** Names the tab you are on. The shell header names the screen; this names the section. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Text_Title;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Text_Breadcrumb;

    /** The detail panel: what the focused row is, what it does, and what it is set to. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Text_DetailTitle;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Text_DetailBody;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Text_DetailValue;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Text_Status;

    /** The "CURRENTLY" label. Bound so it can be hidden with the value it introduces. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Text_DetailNow;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Settings")
    FText ApplyLabel = NSLOCTEXT("Mythic", "SettingsApply", "Apply");

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Settings")
    FText DefaultsLabel = NSLOCTEXT("Mythic", "SettingsDefaults", "Restore Defaults");

    /** Shown when nothing has focus yet, so the detail panel is never three empty lines. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Settings")
    FText EmptyDetailHint = NSLOCTEXT("Mythic", "SettingsHint", "Choose a setting to read what it does.");

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Settings")
    FText PendingApplyHint = NSLOCTEXT("Mythic", "SettingsPending", "Some changes need Apply.");

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> Rail;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> RowList;

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Settings")
    int32 ActiveCategory = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Settings")
    FMythicSettingDefinition FocusedRow;

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Settings")
    bool bPendingApply = false;

private:
    /**
     * Every row of every tab is built once, here, and then only ever shown or hidden. Switching tabs is a
     * visibility flip rather than an add/remove, which keeps tab changes out of the child-order
     * invalidation band entirely — the most expensive thing a Slate tree can do.
     */
    void BuildScreen();

    void ApplyCategoryVisibility();

    /** Pushes title, detail panel and footer text into whichever bound widgets exist. */
    void PushChrome();

    void LabelButton(UCommonButtonBase *Button, const FText &Label) const;

    void HandleTabClicked(int32 CategoryIndex);

    /** One container per tab, parented to RowList in authored order. */
    UPROPERTY()
    TArray<TObjectPtr<UPanelWidget>> CategoryContainers;

    UPROPERTY()
    TArray<TObjectPtr<UCommonButtonBase>> TabButtons;

    /** Rows of the active tab, so focus can start on the first real setting rather than a heading. */
    UPROPERTY()
    TArray<TObjectPtr<UMythicSettingRowBase>> ActiveRows;

    bool bScreenBuilt = false;
};
