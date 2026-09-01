// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManagerTypes.h"
#include "GameplayTagContainer.h"
#include "Components/ListViewBase.h"
#include "FieldNotificationId.h"
#include "Itemization/Inventory/MythicInventoryActionTypes.h"
#include "UI/MythicActivatableWidget.h"
#include "MythicCharacterPageWidget.generated.h"

class UBorder;
class UCommonButtonBase;
class UCommonTextBlock;
class UHorizontalBox;
class UImage;
class UInputAction;
class UInventorySelectionVM;
class UInventoryTabVM;
class UInventoryVM;
class UItemSlotVM;
class UListView;
class UListViewBase;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMythicHUDLayout;
class UMythicBoundActionButton;
class UMythicInventoryComponent;
class UMythicInventoryInteractionCoordinator;
class UMythicItemDetailsWidget;
class UMythicSectionHeader;
class UOverlay;
class UPanelWidget;
class USizeBox;
class UVerticalBox;
class UWidget;
struct FGameplayEventData;
struct FAnalogInputEvent;
struct FGeometry;

class UMythicCharacterPageWidget;

/** Local commands shared by clickable inventory controls and controller input. */
UENUM()
enum class EMythicInventoryUICommand : uint8 {
    EquipOrUnequip,
    Use,
    BeginMove,
    Split,
    Drop,
    ToggleJunk,
    SortByRarity,
    SortByType,
    SortByName,
    SortByValue,
    SortByWeight,
    QuantityDecrease,
    QuantityIncrease,
    QuantityDecreaseLarge,
    QuantityIncreaseLarge,
    ConfirmQuantity,
    Cancel,
};

/** Payload-bearing click bridge for menu rows built from the project UI kit. */
UCLASS()
class MYTHIC_API UMythicInventoryActionClickProxy : public UObject {
    GENERATED_BODY()

public:
    UPROPERTY()
    TWeakObjectPtr<UMythicCharacterPageWidget> Page;

    UPROPERTY()
    EMythicInventoryUICommand Command = EMythicInventoryUICommand::Cancel;

    UPROPERTY()
    int32 Payload = INDEX_NONE;

    UFUNCTION()
    void HandleClicked();
};

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

/** What colour a rune's category draws in. Authored so a new category is a row, not a code change. */
USTRUCT(BlueprintType)
struct FMythicRuneCategoryColour {
    GENERATED_BODY()

    /** Rune category whose socket mark receives this colour. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Runes", meta = (Categories = "Rune.Category"))
    FGameplayTag Category;

    /** Player-facing tint used for runes in Category. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Runes")
    FLinearColor Colour = FLinearColor::White;
};

UCLASS()
class MYTHIC_API UMythicCharacterPageWidget : public UMythicActivatableWidget {
    GENERATED_BODY()

public:
    void OpenSocketPicker(int32 SlotIndex);

    /** Closes the character page's active inventory modal; browsing declines so the menu shell handles Back. */
    virtual bool TryHandleNestedBackAction() override;

    /** Redraws the socket strip after the picker commits a change. */
    void NotifyRunesChanged();

    /** Advances the bag's active category. The page-local inventory context maps this to Right Trigger. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Inventory")
    void CycleBagCategoryForward();

    /** Moves to the previous bag category. The page-local inventory context maps this to Left Trigger. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Inventory")
    void CycleBagCategoryBack();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeOnActivated() override;
    virtual void NativeOnDeactivated() override;
    virtual void NativeDestruct() override;
    virtual UWidget *NativeGetDesiredFocusTarget() const override;
    virtual FReply NativeOnAnalogValueChanged(
        const FGeometry &InGeometry,
        const FAnalogInputEvent &InAnalogEvent) override;

    /** Performs the contextual primary verb for the selected slot (equip, unequip, use, or inspect). */
    UFUNCTION()
    void HandlePrimaryInventoryAction();

    /** Opens the complete controller-navigable action menu for the selected item. */
    UFUNCTION()
    void HandleInventoryActionsAction();

    /** Cycles the exact equipment target used by both inline comparison and Equip. */
    UFUNCTION()
    void HandleCycleInventoryTarget();

    /** Opens the data-safe sort choices for the selected carried group. */
    UFUNCTION()
    void HandleSortInventoryAction();

    UFUNCTION()
    void HandlePreviousInventoryCategory();

    UFUNCTION()
    void HandleNextInventoryCategory();

    /** Receives the authoritative result for a correlated player-inventory request. */
    UFUNCTION()
    void HandleInventoryActionReceipt(const FMythicInventoryActionReceipt &Receipt);

    /** Presents already-localized coordinator feedback without creating page-local transport state. */
    UFUNCTION()
    void HandleInventoryInteractionFeedback(const FText &Message, bool bIsError);

    /** Refreshes mutation affordances while navigation and item details remain interactive. */
    UFUNCTION()
    void HandleInventoryPendingChanged(
        bool bPending,
        const FMythicInventoryActionSubmission &Submission);

    /** Coalesces replicated slot changes so selection is restored after the view model refreshes. */
    UFUNCTION()
    void HandleInventorySlotUpdated(int32 SlotIndex);

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

    /** Contextual confirm action: Enter/A. Mouse activates the same rail button directly. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Inventory|Input")
    TSoftObjectPtr<UInputAction> InventoryPrimaryInputAction;

    /** Opens the selected item's complete action list: F/X. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Inventory|Input")
    TSoftObjectPtr<UInputAction> InventoryActionsInputAction;

    /** Cycles the exact compatible equipment target: Left Shift/Y. Comparison updates inline in Item Details. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Inventory|Input")
    TSoftObjectPtr<UInputAction> InventoryCompareInputAction;

    /** Moves to the previous carried category: Left Bracket/Left Trigger. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Inventory|Input")
    TSoftObjectPtr<UInputAction> InventoryPreviousCategoryInputAction;

    /** Moves to the next carried category: Right Bracket/Right Trigger. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Inventory|Input")
    TSoftObjectPtr<UInputAction> InventoryNextCategoryInputAction;

    /** Opens the selected category's sort choices: R/Left Stick Click. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Inventory|Input")
    TSoftObjectPtr<UInputAction> InventorySortInputAction;

    /** Optional authored host for interactive inventory controls; C++ adds one to PageStack when absent. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UHorizontalBox> InventoryActionBar;


    /** Character name displayed in the page header. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Txt_CharacterName;

    /** Current data-driven character level displayed in the page header. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Txt_Level;

    /** Material-backed progress bar for the current character-level XP interval. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Img_XpBar;

    /** Numeric current/required XP readout paired with Img_XpBar. */
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

    /** Tints a worn rune's mark by what it is for, so a build reads as a shape before it is read as words. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes")
    TArray<FMythicRuneCategoryColour> RuneCategoryColours;

    /** The rune library, opened against whichever socket was clicked. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes")
    TSubclassOf<class UMythicRunePickerWidget> RunePickerClass;

    /** Host that receives the persistent item-details widget for the selected physical item. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> DetailsHost;

    /** Shown while nothing is selected, so the right-hand column is never just a hole. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UWidget> DetailsPlaceholder;

    /** WBP_ItemDetails. One typed instance for the page's lifetime; never created per selection or comparison. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Character")
    TSubclassOf<UMythicItemDetailsWidget> ItemDetailsClass;

    /** Shared with the proficiency tracks and the vital orbs, so every bar in the game is the same bar. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Character")
    TObjectPtr<UMaterialInterface> XpBarMaterial;

    /** Leading colour of the character-level XP fill gradient. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Character")
    FLinearColor XpFillStart = FLinearColor(0.85f, 0.70f, 0.30f, 1.0f);

    /** Trailing colour of the character-level XP fill gradient. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Character")
    FLinearColor XpFillEnd = FLinearColor(0.55f, 0.42f, 0.16f, 1.0f);

private:
    friend class UMythicInventoryActionClickProxy;

    enum class EInventoryPageState : uint8 {
        Browsing,
        Context,
        MoveTarget,
        Quantity,
        Sort,
    };

    enum class EQuantityPurpose : uint8 {
        None,
        Split,
        Drop,
    };

    /** Stable authored identity for a repeated equipment slot, independent of its current absolute array index. */
    struct FEquipmentTargetKey {
        FGameplayTag GroupTag;
        FPrimaryAssetId SlotDefinitionId;
        int32 EntryIndex = INDEX_NONE;
        int32 RepetitionOrdinal = 0;

        bool operator==(const FEquipmentTargetKey &Other) const {
            return GroupTag == Other.GroupTag
                && SlotDefinitionId == Other.SlotDefinitionId
                && EntryIndex == Other.EntryIndex
                && RepetitionOrdinal == Other.RepetitionOrdinal;
        }
    };

    struct FEquipmentTarget {
        FEquipmentTargetKey Key;
        int32 SlotIndex = INDEX_NONE;
        int32 GroupDisplayOrder = 0;
        FText DisplayName;
        bool bExpectEmpty = true;
        FGuid OccupantGuid;
        int32 OccupantQuantity = 0;
    };

    struct FMythicRuneSocket {
        TObjectPtr<UWidget> Button;
        TObjectPtr<UImage> Well;
        TObjectPtr<UImage> Mark;
        int32 SlotIndex = INDEX_NONE;
        TObjectPtr<UMythicRuneSocketClickProxy> Proxy;
    };

    void BuildSockets();

    void RefreshSockets();

    FLinearColor RuneCategoryColour(const class UMythicRuneDefinition *Rune) const;


    TArray<FMythicRuneSocket> Sockets;

    /** One picker for the page's lifetime. Creating a widget per click is a frame spike. */
    UPROPERTY()
    TObjectPtr<class UMythicRunePickerWidget> RunePicker;

    /** The socket row inside the details card, cached when the card is built. */
    UPROPERTY()
    TObjectPtr<class UMythicSocketRowWidget> SocketRow;


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

    void BuildInventoryInteractionChrome();
    void BuildInventoryActionBar();
    UMythicBoundActionButton *CreateInventoryActionButton(TSubclassOf<class UCommonButtonStyle> StyleClass);
    void BindInventoryInputs();
    void ReleaseInventoryInputs();
    void BindInventoryEvents();
    void ReleaseInventoryEvents();
    void ResetInventoryInteractionState();
    void RefreshInventoryActionBar();
    void RefreshDetailsForSelection();
    void ExecuteInventoryUICommand(EMythicInventoryUICommand Command, int32 Payload);

    void OpenContextMenu();
    void OpenSortMenu();
    void OpenQuantityPanel(EQuantityPurpose Purpose);
    void CloseInventoryModal(bool bRestoreSourceSelection = true);
    void ClearModalOptions();
    UWidget *AddModalCommand(const FText &Label, EMythicInventoryUICommand Command,
                             int32 Payload = INDEX_NONE, bool bEnabled = true,
                             const FText &Tooltip = FText::GetEmpty(),
                             bool bRequiresHold = false);
    void SetFeedback(const FText &Message, bool bIsError = false);

    UItemSlotVM *GetSelectedSlot() const;
    UMythicInventoryComponent *GetInventoryComponent() const;
    bool BuildSourceLocator(struct FMythicInventorySourceLocator &OutSource) const;
    bool BuildTargetLocator(int32 SlotIndex, struct FMythicInventoryTargetLocator &OutTarget) const;
    bool CanMoveSelectionToSlot(int32 TargetSlotIndex, FText *OutReason = nullptr) const;
    TArray<FEquipmentTarget> BuildEquipmentTargets() const;
    int32 FindUnequipDestination() const;
    void ResolveActiveEquipmentTarget();
    void CycleActiveEquipmentTarget();
    FEquipmentTargetKey BuildEquipmentTargetKey(int32 SlotIndex) const;
    bool IsMutationPending() const;
    UMythicInventoryInteractionCoordinator *GetInventoryInteractionCoordinator() const;
    FText GetSlotDisplayName(int32 SlotIndex) const;
    void BeginMoveTargetSelection();
    void ConfirmMoveTarget();
    void SubmitMoveToSlot(int32 TargetSlotIndex);
    void SubmitUse();
    void SubmitSplit(int32 Quantity);
    void SubmitDrop(int32 Quantity);
    void SubmitSetJunk();
    void SubmitSort(int32 SortModeValue);
    void HandleSubmittedRequest(int64 RequestId);

    void SetSelectedSlot(UItemSlotVM *SlotVM, UListViewBase *SourceList);
    void ClearOtherListSelections(UListViewBase *Except);
    void ScheduleRestoreSelection();
    void RestoreSelectionByGuid();
    UListViewBase *FindListSelecting(UObject *Item) const;

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

    FInputActionBindingHandle PrimaryBinding;
    FInputActionBindingHandle ActionsBinding;
    FInputActionBindingHandle CompareBinding;
    FInputActionBindingHandle PreviousCategoryBinding;
    FInputActionBindingHandle NextCategoryBinding;
    FInputActionBindingHandle SortBinding;

    EInventoryPageState InventoryPageState = EInventoryPageState::Browsing;
    EQuantityPurpose QuantityPurpose = EQuantityPurpose::None;
    int32 QuantityValue = 1;
    int32 QuantityMaximum = 1;
    int32 ActiveTargetSlotIndex = INDEX_NONE;
    bool bSynchronizingSelection = false;
    bool bSelectionRestoreScheduled = false;
    FGuid SelectedItemGuid;
    FGuid ActionSourceGuid;
    int32 LastSelectedSlotIndex = INDEX_NONE;
    int32 ActionSourceSlotIndex = INDEX_NONE;
    TWeakObjectPtr<UListViewBase> SelectedList;

    /** Last target choice per item family; stable authored keys survive slot-array reorder and replication refresh. */
    TMap<FGameplayTag, FEquipmentTargetKey> StickyEquipmentTargetKeys;

    UPROPERTY(Transient)
    TObjectPtr<UCommonTextBlock> InventoryFeedback;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> InventoryModalLayer;

    UPROPERTY(Transient)
    TObjectPtr<UCommonTextBlock> InventoryModalTitle;

    UPROPERTY(Transient)
    TObjectPtr<UCommonTextBlock> InventoryModalBody;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> InventoryModalOptions;

    UPROPERTY(Transient)
    TObjectPtr<UMythicBoundActionButton> PrimaryActionButton;

    UPROPERTY(Transient)
    TObjectPtr<UMythicBoundActionButton> ActionsActionButton;

    UPROPERTY(Transient)
    TObjectPtr<UMythicBoundActionButton> TargetActionButton;

    UPROPERTY(Transient)
    TObjectPtr<UMythicBoundActionButton> SortActionButton;

    UPROPERTY()
    TArray<TObjectPtr<UMythicInventoryActionClickProxy>> InventoryClickProxies;

    /** Keeps the native rune click bridges reachable by GC for as long as their socket buttons exist. */
    UPROPERTY()
    TArray<TObjectPtr<UMythicRuneSocketClickProxy>> RuneSocketClickProxies;

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
    TObjectPtr<UMythicItemDetailsWidget> DetailsCard;

    UPROPERTY(Transient)
    TWeakObjectPtr<UWidget> BorrowedInventory;

    UPROPERTY(Transient)
    TWeakObjectPtr<UMythicHUDLayout> Lender;
};
