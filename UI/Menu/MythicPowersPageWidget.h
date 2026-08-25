// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UI/MythicActivatableWidget.h"
#include "MythicPowersPageWidget.generated.h"

class UCommonTextBlock;
class UImage;
class UMythicPowersPageWidget;
class UMythicSectionHeader;
class UMythicSkillComponent;
class UMythicSkillDefinition;
class UPanelWidget;
class UWidget;

UCLASS()
class MYTHIC_API UMythicPowerRowProxy : public UObject {
    GENERATED_BODY()

public:
    UPROPERTY()
    TWeakObjectPtr<UMythicPowersPageWidget> Page;

    UPROPERTY()
    int32 RowIndex = INDEX_NONE;

    UFUNCTION()
    void HandleClicked();
};

UCLASS()
class MYTHIC_API UMythicModifierRowProxy : public UObject {
    GENERATED_BODY()

public:
    UPROPERTY()
    TWeakObjectPtr<UMythicPowersPageWidget> Page;

    UPROPERTY()
    int32 ModifierIndex = INDEX_NONE;

    UFUNCTION()
    void HandleClicked();
};

/**
 * The Powers page: what you cast, as opposed to what you wear.
 *
 * Abilities are the one lane today. Incantations are the page's other half and are unbuilt (#150), so the lane
 * switcher arrives with them rather than sitting on screen with nothing behind it.
 *
 * The page only displays and routes. Equipping, levelling and switching a modifier all go through
 * UMythicSkillComponent's server functions, which re-run every rule, so nothing here decides anything.
 */
UCLASS()
class MYTHIC_API UMythicPowersPageWidget : public UMythicActivatableWidget {
    GENERATED_BODY()

public:
    /** Selects a row and redraws the detail column. Called by the row proxies. */
    void SelectRow(int32 RowIndex);

    /** Asks the server to turn one modifier on the selected ability on or off. */
    void ToggleModifier(int32 ModifierIndex);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeOnActivated() override;
    virtual void NativeOnDeactivated() override;
    virtual UWidget *NativeGetDesiredFocusTarget() const override;

    /** Where the ability rows go. One row per definition in the library, built once and re-texted after. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> AbilityHost;

    /** Where the selected ability's modifier rows go. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> ModifierHost;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Txt_SelectedName;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Txt_SelectedDescription;

    /** "Level 3 - 1 point unspent". Empty when nothing is selected. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Txt_Progress;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Img_SelectedIcon;

    /** Shown while nothing is selected, so the detail column is never just a hole. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UWidget> DetailsPlaceholder;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Powers")
    TSubclassOf<UMythicSectionHeader> SectionHeaderClass;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Powers")
    TSubclassOf<UUserWidget> AbilityRowClass;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Powers")
    TSubclassOf<UUserWidget> ModifierRowClass;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Powers")
    FName RowNameText = TEXT("NameText");

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Powers")
    FName RowDetailText = TEXT("DetailText");

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Powers")
    FName RowIconImage = TEXT("IconImage");

    /** How far a row the player cannot use yet is faded, so locked reads without colour alone carrying it. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Powers", meta = (ClampMin = "0.1", ClampMax = "1.0"))
    float LockedRowOpacity = 0.45f;

private:
    struct FPowerRow {
        TObjectPtr<UUserWidget> Widget;
        TObjectPtr<UMythicPowerRowProxy> Proxy;
        TObjectPtr<UMythicSkillDefinition> Skill;
        bool bUnlocked = false;
    };

    struct FModifierRow {
        TObjectPtr<UUserWidget> Widget;
        TObjectPtr<UMythicModifierRowProxy> Proxy;
    };

    void BuildRows();

    /** Bound to the component's dynamic change delegate, so it must be a UFUNCTION. */
    UFUNCTION()
    void RefreshRows();

    void RefreshDetails();

    /** The library, sorted by name so the list does not reorder itself between sessions. */
    void LoadLibrary();

    UMythicSkillComponent *GetSkills() const;

    static void BindFirstButton(UUserWidget *RowWidget, UObject *Proxy, FName FunctionName);

    void SetRowText(UUserWidget *RowWidget, FName SlotName, const FText &Text) const;

    TArray<FPowerRow> Rows;
    TArray<FModifierRow> ModifierRows;

    UPROPERTY()
    TArray<TObjectPtr<UMythicSkillDefinition>> Library;

    UPROPERTY(Transient)
    TObjectPtr<UMythicSectionHeader> AbilityHeader;

    UPROPERTY(Transient)
    TObjectPtr<UMythicSectionHeader> ModifierHeader;

    int32 SelectedRow = INDEX_NONE;

};
