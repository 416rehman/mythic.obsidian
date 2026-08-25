// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ListViewBase.h"
#include "FieldNotificationId.h"
#include "UI/MythicActivatableWidget.h"
#include "MythicCharacterPageWidget.generated.h"

class UBorder;
class UCommonTextBlock;
class UImage;
class UInventorySelectionVM;
class UInventoryTabVM;
class UInventoryVM;
class UListView;
class UListViewBase;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMythicHUDLayout;
class UMythicSectionHeader;
class UPanelWidget;
class UVerticalBox;
class UWidget;
struct FGameplayEventData;

class UMythicCharacterPageWidget;

UCLASS()
class MYTHIC_API UMythicRuneSocketClickProxy : public UObject {
    GENERATED_BODY()

public:
    UPROPERTY()
    TWeakObjectPtr<UMythicCharacterPageWidget> Page;

    UPROPERTY()
    int32 SlotIndex = INDEX_NONE;

    UFUNCTION()
    void HandleClicked();
};

UCLASS()
class MYTHIC_API UMythicCharacterPageWidget : public UMythicActivatableWidget {
    GENERATED_BODY()

public:
    void OpenSocketPicker(int32 SlotIndex);

    /** Advances the bag's active category. Bound to LB/RB CommonUI actions by the page Blueprint. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Inventory")
    void CycleBagCategoryForward();

    UFUNCTION(BlueprintCallable, Category = "Mythic|Inventory")
    void CycleBagCategoryBack();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeOnActivated() override;
    virtual void NativeOnDeactivated() override;
    virtual void NativeDestruct() override;
    virtual UWidget *NativeGetDesiredFocusTarget() const override;

    /** Where the borrowed inventory sits while this tab is up. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> InventoryHost;

    /** Grouped-card ground behind the bag column; brushed from the kit when the page authors one. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UBorder> BagCard;

    /** Where Right from the bag lands, so pad focus can cross into the stat sheet. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UWidget> StatSheetHost;

    /** The house header announcing the bag: active category, used / capacity, category icon. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Inventory")
    TSubclassOf<UMythicSectionHeader> BagHeaderClass;

    /** Catalogue id for the empty-state glyph. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Inventory")
    FName BagEmptyGlyphId = TEXT("SlotTex.Round");

    /**
     * Borrowed equipment strips whose ancestor chain carries one of these names are stood up as vertical
     * columns while on this page, and laid back down when the HUD takes the widget back.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Inventory")
    TArray<FName> VerticalStripNames = { TEXT("Armor"), TEXT("Accessories") };

    /** Names the strip that takes first focus when the page opens. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Inventory")
    FName WeaponStripName = TEXT("Weapon");


    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Txt_CharacterName;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Txt_Level;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Img_XpBar;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Txt_XpValue;


    /**
     * The four rune sockets, on the character screen where a build is actually read.
     *
     * Runes used to live behind their own tab, which meant the one screen that shows what your character IS did not
     * show the four choices that shape it most. Empty sockets are drawn as sockets rather than written as the words
     * "not yet earned", so an unfilled one reads as something to fill.
     *
     * The sockets only display and route: every write still goes through UMythicRuneComponent's server RPCs, which
     * re-run the slotting rules. Clicking one opens the picker with that slot already chosen.
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> SocketStrip;

    /**
     * The house section header, so the socket row is announced like every other group on the page.
     *
     * Four unexplained rings in the corner of a screen tell a player nothing about what they are or what
     * putting something in one would do.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes")
    TSubclassOf<class UMythicSectionHeader> SocketHeaderClass;

    /** The widget inside the borrowed inventory that holds the bag, as opposed to an equipment strip. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Inventory")
    FName BagWidgetName = TEXT("WBP_InventorySlots");

    /** How many sockets to build. Matches the rune component's MaxSlots. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes", meta = (ClampMin = "1", ClampMax = "8"))
    int32 SocketCount = 4;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> DetailsHost;

    /** Shown while nothing is selected, so the right-hand column is never just a hole. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UWidget> DetailsPlaceholder;

    /** WBP_ItemDetails. One instance for the page's lifetime — never created per selection. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Character")
    TSubclassOf<UUserWidget> ItemDetailsClass;

    /** Shared with the proficiency tracks and the vital orbs, so every bar in the game is the same bar. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Character")
    TObjectPtr<UMaterialInterface> XpBarMaterial;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Character")
    FLinearColor XpFillStart = FLinearColor(0.85f, 0.70f, 0.30f, 1.0f);

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Character")
    FLinearColor XpFillEnd = FLinearColor(0.55f, 0.42f, 0.16f, 1.0f);

private:
    struct FMythicRuneSocket {
        TObjectPtr<UWidget> Button;
        TObjectPtr<UImage> Well;
        TObjectPtr<UImage> Mark;
        int32 SlotIndex = INDEX_NONE;

        UPROPERTY()
        TObjectPtr<UMythicRuneSocketClickProxy> Proxy;
    };

    void BuildSockets();

    void RefreshSockets();


    TArray<FMythicRuneSocket> Sockets;


    UMythicHUDLayout *FindHUDLayout() const;

    void RefreshHeader();

    void BindProgression();
    void UnbindProgression();
    void HandleProficiencyEvent(FGameplayTag Tag, const FGameplayEventData *Payload);

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> XpBarMID;

    FDelegateHandle ProgressionEventHandle;
    bool bProgressionBound = false;

    int32 LastShownLevel = MIN_int32;

    void BorrowInventory();

    /** Sets the borrowed bag to wrap onto new rows; the shared asset stays horizontal for equipment strips. */
    void WrapBorrowedBag(class UWidget *Inventory);

    /** Makes the borrowed bag wrap to its column; the shared asset stays neutral for the equipment rows. */
    void ReturnInventory();

    void BindSlotSelection();
    void UnbindSlotSelection();

    void BuildBagChrome();
    void BindBagViewModel();
    void UnbindBagViewModel();
    void HandleBagFieldChanged(UObject *Object, UE::FieldNotification::FFieldId FieldId);
    void RefreshBagHeader();
    void CycleBagCategory(int32 Direction);
    void FocusInitialSlot();

    UInventoryVM *ResolveInventoryVM() const;

    static bool ChainHasName(UWidget *Leaf, const TArray<FName> &Names);

    UPROPERTY(Transient)
    TObjectPtr<UMythicSectionHeader> BagHeader;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> BagEmptyState;

    UPROPERTY(Transient)
    TObjectPtr<UImage> BagEmptyGlyph;

    UPROPERTY(Transient)
    TObjectPtr<UCommonTextBlock> BagEmptyLine;

    TWeakObjectPtr<UInventoryVM> BoundInventoryVM;
    TWeakObjectPtr<UInventorySelectionVM> BoundSelectionVM;
    FDelegateHandle BagTabHandle;

    TWeakObjectPtr<UListView> WeaponStrip;
    TArray<TWeakObjectPtr<UListView>> ReorientedStrips;

    /** Selects the first slot that actually holds something, so the detail column opens with content. */
    void SelectFirstOccupiedSlot();
    static void CollectSlotLists(UUserWidget *Root, TArray<class UListViewBase *> &Out);

    static ITypedUMGListView<UObject *> *AsTypedList(class UListViewBase *List);

    void HandleSlotSelectionChanged(UObject *Item);

    void BuildDetailsCard();

    void ShowDetailsFor(UObject *SlotVM);

    TArray<TWeakObjectPtr<class UListViewBase>> BoundSlotLists;

    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> DetailsCard;

    UPROPERTY(Transient)
    TWeakObjectPtr<UWidget> BorrowedInventory;

    UPROPERTY(Transient)
    TWeakObjectPtr<UMythicHUDLayout> Lender;
};
