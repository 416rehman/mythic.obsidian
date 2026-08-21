// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UI/MythicActivatableWidget.h"
#include "MythicMenuShell.generated.h"

class UCommonActivatableWidgetSwitcher;
class UCommonButtonBase;
class UMythicInputGlyph;
class UCommonTabListWidgetBase;
class UCommonTextBlock;
class UTextBlock;

USTRUCT(BlueprintType)
struct FMythicMenuPage {
    GENERATED_BODY()

    /** Stable id. Used to open the shell straight onto a page ("open on Map") and to keep tab order independent of index. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu")
    FName PageId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu")
    FText TabLabel;

    /** The screen itself. Every page is a CommonActivatableWidget so the switcher can deactivate the ones off screen. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu")
    TSubclassOf<UCommonActivatableWidget> PageClass;

    /**
     * The tab button for this page. Leave unset to fall back to the shell's DefaultTabButtonClass — one button class
     * for every tab is what keeps the strip visually consistent, so per-page overrides should be rare.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu")
    TSubclassOf<UCommonButtonBase> TabButtonClass;

    /** Hides a page until its system is unlocked (a second rune slot, the first incantation learned, and so on). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu")
    FGameplayTag RequiredUnlockTag;
};

UCLASS()
class MYTHIC_API UMythicMenuShell : public UMythicActivatableWidget {
    GENERATED_BODY()

public:
    /** Open on a specific page. An unknown or locked id falls back to the first available page. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Menu")
    void OpenPage(FName PageId);

    UFUNCTION(BlueprintPure, Category = "Mythic|Menu")
    FName GetActivePageId() const { return ActivePageId; }

    /** The page widget for an id, or null if that page is not registered. Lets a caller push context into a page. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Menu")
    UCommonActivatableWidget *GetPageWidget(FName PageId) const;

    /**
     * Step to the next (+1) or previous (-1) tab, wrapping at both ends. Wrapping matters on a controller: the strip
     * is a ring you flick through, not a list you run out of.
     * Only pages that actually built a tab take part, so a locked system is skipped rather than landing on nothing.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Menu")
    void CyclePage(int32 Delta);

    /**
     * The page class registered under an id, or null when nothing is. Public because "is this screen
     * actually reachable?" is a question worth asserting: a screen can compile, save and read back
     * perfectly while no entry point points at it.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Menu")
    TSubclassOf<UCommonActivatableWidget> GetRegisteredPageClass(FName PageId) const;

    /** The leftmost available tab — what a bare Tab press opens. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Menu")
    FName GetFirstPageId() const;

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeOnActivated() override;
    virtual void NativeOnDeactivated() override;

    /** Pages, in tab order. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Menu")
    TArray<FMythicMenuPage> Pages;

    /** Page shown when the shell is opened without a specific target. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Menu")
    FName DefaultPageId;

    /** Tab button used for every page that does not override it. One class = one consistent tab strip. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Menu")
    TSubclassOf<UCommonButtonBase> DefaultTabButtonClass;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTabListWidgetBase> TabList;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonActivatableWidgetSwitcher> ContentSwitcher;

    /** Big header naming the screen you are on. Reference menus all state the current page loudly; a tab strip alone
     *  makes you hunt for which one is lit. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Txt_ScreenTitle;

    /**
     * Input glyphs bracketing the tab strip — previous on the far left, next on the far right. They are fed the real
     * binding handles, so they show Q/E on a keyboard and LB/RB on a pad without anything hardcoding a key.
     *
     * UMythicInputGlyph rather than UCommonActionWidget: the CommonUI widget only reads icons out of DataTable rows,
     * and this project's UI actions live in the ini as tags, so it drew nothing.
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UMythicInputGlyph> Glyph_PrevTab;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UMythicInputGlyph> Glyph_NextTab;

    /** Fired after the active page changes, so a Blueprint can retitle a header or play a transition. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Menu")
    void OnPageChanged(FName PageId);

    /**
     * Tab-cycling actions. Bound here rather than on the HUD layout because they only mean anything while the menu is
     * up — CommonUI activates a widget's bindings with the widget, so the same keys stay free during play.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Menu", meta = (Categories = "UI.Action"))
    FGameplayTag PreviousTabAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Menu", meta = (Categories = "UI.Action"))
    FGameplayTag NextTabAction;

    /** Colour of the tab you are on. Without a distinct selected state a tab strip is just a row of words. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Menu")
    FLinearColor SelectedTabColor = FLinearColor(0.96f, 0.90f, 0.72f, 1.0f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Menu")
    FLinearColor UnselectedTabColor = FLinearColor(0.55f, 0.50f, 0.42f, 1.0f);

    /**
     * Name of the text widget inside a tab button that carries its label. The project's button already ships one
     * (Text_ActionName); naming it here rather than hardcoding it means a different button class only needs a config
     * change. Without this the whole strip reads whatever placeholder the button was authored with.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Menu")
    FName TabLabelWidgetName = TEXT("Text_ActionName");

private:
    void BuildPages();

    void RegisterTabs();

    bool IsPageUnlocked(const FMythicMenuPage &Page) const;

    UFUNCTION()
    void HandleTabSelected(FName TabId);

    void ApplyTabSelectionVisuals();

    UPROPERTY()
    TMap<FName, TObjectPtr<UTextBlock>> TabLabels;

    UPROPERTY()
    TArray<FName> OrderedPageIds;

    UPROPERTY()
    TMap<FName, TObjectPtr<UCommonActivatableWidget>> PageWidgets;

    FUIActionBindingHandle PrevTabBinding;
    FUIActionBindingHandle NextTabBinding;

    FName ActivePageId;
    bool bPagesBuilt = false;
};
