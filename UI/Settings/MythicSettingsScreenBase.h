
#pragma once

#include "CoreMinimal.h"
#include "UI/MythicActivatableWidget.h"
#include "UI/Settings/MythicSettingDefinition.h"
#include "MythicSettingsScreenBase.generated.h"

/**
 * Behaviour for the settings screen. Constructs no widgets: the Widget Blueprint owns the rail, the list,
 * the detail panel and every row. This class answers what to show and applies what changed.
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

    /** Tabs, in authored order. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    TArray<FMythicSettingCategory> GetCategories() const;

    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    int32 GetActiveCategoryIndex() const { return ActiveCategory; }

    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetActiveCategoryIndex(int32 Index);

    /**
     * The rows for the active tab, already ordered by the category's authored group order, with a blank-Id
     * entry inserted before each group as its heading. One list, so the Blueprint just walks it.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    TArray<FMythicSettingDefinition> GetRowsForActiveCategory() const;

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
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Settings")
    int32 ActiveCategory = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Settings")
    FMythicSettingDefinition FocusedRow;

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Settings")
    bool bPendingApply = false;
};
