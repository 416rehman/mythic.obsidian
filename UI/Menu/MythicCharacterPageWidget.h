// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ListViewBase.h"
#include "UI/MythicActivatableWidget.h"
#include "MythicCharacterPageWidget.generated.h"

class UCommonTextBlock;
class UImage;
class UListView;
class UListViewBase;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMythicHUDLayout;
class UPanelWidget;
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

protected:
    virtual void NativeConstruct() override;
    virtual void NativeOnActivated() override;
    virtual void NativeOnDeactivated() override;
    virtual void NativeDestruct() override;

    /** Where the borrowed inventory sits while this tab is up. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> InventoryHost;


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
     * The sockets only display and route: every write still goes through UMythicPerkComponent's server RPCs, which
     * re-run the slotting rules. Clicking one opens the picker with that slot already chosen.
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> SocketStrip;

    /** How many sockets to build. Matches the perk component's MaxPerkSlots. */
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
    void ReturnInventory();

    void BindSlotSelection();
    void UnbindSlotSelection();
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
