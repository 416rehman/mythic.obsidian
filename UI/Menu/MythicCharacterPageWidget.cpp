// Copyright Stellar Games. All Rights Reserved.

#include "MythicCharacterPageWidget.h"


#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/HorizontalBox.h"
#include "UI/MythicUIKit.h"
#include "Progression/Runes/MythicRuneComponent.h"
#include "Progression/Runes/MythicRuneDefinition.h"
#include "UI/Menu/MythicRunePickerWidget.h"
#include "UI/Inventory/MythicSocketRowWidget.h"
#include "UI/Widgets/MythicSectionHeader.h"
#include "UI/MythicUIStyle.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "CommonButtonBase.h"
#include "Components/VerticalBox.h"
#include "Blueprint/WidgetTree.h"
#include "TimerManager.h"
#include "CommonActivatableWidget.h"
#include "CommonTextBlock.h"
#include "INotifyFieldValueChanged.h"
#include "Components/Image.h"
#include "Components/ListView.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TileView.h"
#include "Components/TreeView.h"
#include "Components/PanelWidget.h"
#include "Components/VerticalBoxSlot.h"
#include "Itemization/Inventory/ViewModels/InventoryVM.h"
#include "Itemization/Inventory/ViewModels/ItemSlotVM.h"
#include "Itemization/Inventory/ViewModels/ItemComparisonVM.h"
#include "Itemization/Inventory/MythicInventoryActionTypes.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/InventorySlotDefinition.h"
#include "Itemization/MythicTags_Inventory.h"
#include "InputAction.h"
#include "MVVMSubsystem.h"
#include "View/MVVMView.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Proficiencies.h"
#include "GAS/MythicTags_GAS.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerState.h"
#include "UObject/UObjectIterator.h"
#include "UI/MythicHUDLayout.h"
#include "UI/Widgets/MythicBoundActionButton.h"

namespace {
const FName Char_Percent(TEXT("Percent"));
const FName Char_ChipPercent(TEXT("ChipPercent"));
const FName Char_FillStart(TEXT("FillColorStart"));
const FName Char_FillEnd(TEXT("FillColorEnd"));
}

void UMythicInventoryActionClickProxy::HandleClicked() {
    if (UMythicCharacterPageWidget *CharacterPage = Page.Get()) {
        CharacterPage->ExecuteInventoryUICommand(Command, Payload);
    }
}

void UMythicCharacterPageWidget::NativeConstruct() {
    // Pool before Super: activation can fire inside it, and a refresh must find these already built.
    BuildBagChrome();

    Super::NativeConstruct();
    BuildInventoryInteractionChrome();

    if (BagCard) {
        FSlateBrush NoDraw;
        NoDraw.DrawAs = ESlateBrushDrawType::NoDrawType;
        BagCard->SetBrush(NoDraw);
        BagCard->SetPadding(FMargin(0.0f));
    }

    // The character's name heads this column, so the shared panel's own "STATS" eyebrow is redundant here.
    if (WidgetTree) {
        if (UUserWidget *Panel = Cast<UUserWidget>(WidgetTree->FindWidget(TEXT("StatPanel")))) {
            if (Panel->WidgetTree) {
                if (UWidget *PanelTitle = Panel->WidgetTree->FindWidget(TEXT("TitleText"))) {
                    PanelTitle->SetVisibility(ESlateVisibility::Collapsed);
                }
            }
        }
    }

    if (Img_XpBar && XpBarMaterial) {
        FSlateBrush Brush;
        Brush.SetResourceObject(XpBarMaterial);
        Brush.ImageSize = FVector2D(320.0f, 3.0f);
        Img_XpBar->SetBrush(Brush);
        XpBarMID = Img_XpBar->GetDynamicMaterial();
        if (XpBarMID) {
            XpBarMID->SetVectorParameterValue(Char_FillStart, XpFillStart);
            XpBarMID->SetVectorParameterValue(Char_FillEnd, XpFillEnd);
        }
    }
}

void UMythicCharacterPageWidget::BindProgression() {
    if (bProgressionBound) {
        return;
    }
    const APlayerController *PC = GetOwningPlayer();
    if (!PC) {
        return;
    }
    UAbilitySystemComponent *ASC = nullptr;
    if (APlayerState *PS = PC->PlayerState) {
        ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PS);
    }
    if (!ASC) {
        return;
    }

    ProgressionEventHandle = ASC->AddGameplayEventTagContainerDelegate(
        FGameplayTagContainer(GAS_EVENT_PROFICIENCY_GAINED),
        FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &UMythicCharacterPageWidget::HandleProficiencyEvent));
    bProgressionBound = true;
}

void UMythicCharacterPageWidget::UnbindProgression() {
    if (!bProgressionBound) {
        return;
    }
    if (const APlayerController *PC = GetOwningPlayer()) {
        if (APlayerState *PS = PC->PlayerState) {
            if (UAbilitySystemComponent *ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PS)) {
                ASC->RemoveGameplayEventTagContainerDelegate(FGameplayTagContainer(GAS_EVENT_PROFICIENCY_GAINED),
                                                             ProgressionEventHandle);
            }
        }
    }
    ProgressionEventHandle.Reset();
    bProgressionBound = false;
}

void UMythicCharacterPageWidget::HandleProficiencyEvent(FGameplayTag Tag, const FGameplayEventData *Payload) {
    RefreshHeader();
}

void UMythicCharacterPageWidget::RefreshHeader() {
    const AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer());
    if (!PC) {
        return;
    }

    if (Txt_CharacterName) {
        const APlayerState *PS = PC->PlayerState;
        FString PlayerName = PS ? PS->GetPlayerName() : FString();

        // A default PlayerState name leaks the OS identity - either the machine name or the login user.
        const FString MachineName = FPlatformProcess::ComputerName();
        const FString OsUserName = FPlatformProcess::UserName();
        const bool bLeakedOsName =
            (!MachineName.IsEmpty() && PlayerName.StartsWith(MachineName)) ||
            (!OsUserName.IsEmpty() && PlayerName.StartsWith(OsUserName));
        if (PlayerName.IsEmpty() || bLeakedOsName) {
            Txt_CharacterName->SetText(NSLOCTEXT("Mythic", "UnnamedCharacter", "Unnamed"));
        }
        else {
            Txt_CharacterName->SetText(FText::FromString(PlayerName));
        }
    }

    const int32 Level = PC->GetPlayerLevel();
    if (Txt_Level && Level != LastShownLevel) {
        LastShownLevel = Level;
        Txt_Level->SetText(FText::Format(NSLOCTEXT("Mythic", "CharLevel", "Level {0}"), FText::AsNumber(Level)));
    }

    float Current = 0.0f;
    float Max = 0.0f;
    if (const APlayerState *PS = PC->PlayerState) {
        if (const UAbilitySystemComponent *ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(
                const_cast<APlayerState *>(PS))) {
            if (const UMythicAttributeSet_Proficiencies *ProfSet = ASC->GetSet<UMythicAttributeSet_Proficiencies>()) {
                Current = ProfSet->GetOverallXp();
                Max = ProfSet->GetOverallXpMax();
            }
        }
    }

    const bool bProgressionLive = Max > 0.0f;
    const float Fraction = bProgressionLive ? FMath::Clamp(PC->GetPlayerLevelProgress(), 0.0f, 1.0f) : 0.0f;

    if (Img_XpBar) {
        Img_XpBar->SetVisibility(bProgressionLive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
    if (XpBarMID && bProgressionLive) {
        XpBarMID->SetScalarParameterValue(Char_Percent, Fraction);
        XpBarMID->SetScalarParameterValue(Char_ChipPercent, Fraction);
    }

    if (Txt_XpValue) {
        if (bProgressionLive) {
            // Progress through THIS level, not lifetime totals - "45 / 127929" tells a player nothing.
            int32 WindowLevel = 1;
            float IntoLevel = 0.0f;
            float LevelSpan = 0.0f;
            UMythicAttributeSet_Proficiencies::GetLevelXpWindow(Current, Max, WindowLevel, IntoLevel, LevelSpan);
            Txt_XpValue->SetVisibility(ESlateVisibility::HitTestInvisible);
            Txt_XpValue->SetText(FText::Format(NSLOCTEXT("Mythic", "CharXp", "{0} / {1}"),
                                               FText::AsNumber(FMath::FloorToInt(LevelSpan > 0.0f ? IntoLevel : Current)),
                                               FText::AsNumber(FMath::CeilToInt(LevelSpan > 0.0f ? LevelSpan : Max))));
        }
        else {
            Txt_XpValue->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void UMythicCharacterPageWidget::NativeOnActivated() {
    Super::NativeOnActivated();
    BuildDetailsCard();
    BorrowInventory();
    BindSlotSelection();
    BindInventoryEvents();
    BindInventoryInputs();
    BindProgression();
    RefreshHeader();
    BuildSockets();
    RefreshSockets();
    SelectFirstOccupiedSlot();
    RefreshInventoryActionBar();

    // Focus one tick late: at activation the borrowed strips are not yet in a visible Slate path, and
    // SetFocus on a widget without one fails silently.
    if (const UWorld *World = GetWorld()) {
        World->GetTimerManager().SetTimerForNextTick(
            FTimerDelegate::CreateUObject(this, &UMythicCharacterPageWidget::FocusInitialSlot));
    }
}

UWidget *UMythicCharacterPageWidget::NativeGetDesiredFocusTarget() const {
    if (UListView *Strip = WeaponStrip.Get()) {
        return Strip;
    }
    return Super::NativeGetDesiredFocusTarget();
}

void UMythicCharacterPageWidget::FocusInitialSlot() {
    if (!IsActivated() || !GetOwningLocalPlayer()) {
        return;
    }
    UListView *Strip = WeaponStrip.Get();
    if (Strip && Strip->GetVisibility() != ESlateVisibility::Collapsed) {
        Strip->SetFocus();
    }
}

bool UMythicCharacterPageWidget::TryHandleNestedBackAction() {
    if (InventoryPageState == EInventoryPageState::Pending) {
        return true;
    }
    if (InventoryPageState != EInventoryPageState::Browsing) {
        CloseInventoryModal(true);
        return true;
    }
    return false;
}

void UMythicCharacterPageWidget::BuildInventoryInteractionChrome() {
    if (!WidgetTree) {
        return;
    }

    const UMythicUIStyleSettings &Style = FMythicUIStyle::Get();
    UPanelWidget *PageStack = Cast<UPanelWidget>(WidgetTree->FindWidget(TEXT("PageStack")));
    if (!InventoryFeedback && PageStack) {
        InventoryFeedback = FMythicUIStyle::MakeText(this, EMythicTextRole::Subtle);
        InventoryFeedback->SetVisibility(ESlateVisibility::Collapsed);
        PageStack->AddChild(InventoryFeedback);
    }
    if (!InventoryActionBar && PageStack) {
        InventoryActionBar = WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(), TEXT("InventoryActionBar_Runtime"));
        PageStack->AddChild(InventoryActionBar);
    }
    BuildInventoryActionBar();

    if (InventoryModalLayer) {
        return;
    }
    UOverlay *PageOverlay = Cast<UOverlay>(WidgetTree->FindWidget(TEXT("PageOverlay")));
    if (!PageOverlay) {
        return;
    }

    InventoryModalLayer = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(), TEXT("InventoryModalLayer_Runtime"));
    InventoryModalLayer->SetBrushColor(
        FLinearColor(Style.Surface.R, Style.Surface.G, Style.Surface.B, 0.97f));
    InventoryModalLayer->SetPadding(FMargin(Style.SpaceL));
    InventoryModalLayer->SetVisibility(ESlateVisibility::Collapsed);

    USizeBox *ModalWidth = WidgetTree->ConstructWidget<USizeBox>();
    ModalWidth->SetWidthOverride(460.0f);
    InventoryModalLayer->AddChild(ModalWidth);

    UVerticalBox *Column = WidgetTree->ConstructWidget<UVerticalBox>();
    ModalWidth->AddChild(Column);

    InventoryModalTitle = FMythicUIStyle::MakeText(this, EMythicTextRole::Heading);
    Column->AddChild(InventoryModalTitle);
    InventoryModalBody = FMythicUIStyle::MakeText(this, EMythicTextRole::Subtle);
    if (UVerticalBoxSlot *BodySlot = Cast<UVerticalBoxSlot>(Column->AddChild(InventoryModalBody))) {
        BodySlot->SetPadding(FMargin(0.0f, Style.SpaceS, 0.0f, Style.SpaceM));
    }
    InventoryModalOptions = WidgetTree->ConstructWidget<UVerticalBox>();
    Column->AddChild(InventoryModalOptions);

    if (UOverlaySlot *ModalSlot = Cast<UOverlaySlot>(PageOverlay->AddChildToOverlay(InventoryModalLayer))) {
        ModalSlot->SetHorizontalAlignment(HAlign_Right);
        ModalSlot->SetVerticalAlignment(VAlign_Center);
        ModalSlot->SetPadding(FMargin(Style.SpaceL));
    }
}

UMythicBoundActionButton *UMythicCharacterPageWidget::CreateInventoryActionButton(
    TSubclassOf<UCommonButtonStyle> StyleClass) {
    if (!InventoryActionBar || !WidgetTree) {
        return nullptr;
    }
    const UMythicUIStyleSettings &Style = FMythicUIStyle::Get();
    UClass *ButtonClass = Style.ActionButtonClass.IsNull()
        ? nullptr
        : Style.ActionButtonClass.LoadSynchronous();
    if (!ButtonClass || !ButtonClass->IsChildOf(UMythicBoundActionButton::StaticClass())) {
        UE_LOG(Myth, Error, TEXT("Character inventory requires an interactive Mythic action-button class."));
        return nullptr;
    }

    UMythicBoundActionButton *Button = WidgetTree->ConstructWidget<UMythicBoundActionButton>(ButtonClass);
    if (StyleClass) {
        Button->SetStyle(StyleClass);
    }
    Button->SetMinDimensions(FMath::RoundToInt(Style.ActionButtonMinWidth),
                             FMath::RoundToInt(Style.ActionButtonMinHeight));
    Button->SetIsSelectable(true);
    Button->SetIsInteractableWhenSelected(true);
    Button->SetShouldSelectUponReceivingFocus(true);
    Button->bNavigateToNextWidgetOnDisable = true;
    if (UHorizontalBoxSlot *ButtonSlot = Cast<UHorizontalBoxSlot>(InventoryActionBar->AddChild(Button))) {
        ButtonSlot->SetVerticalAlignment(VAlign_Center);
        ButtonSlot->SetPadding(FMargin(InventoryActionBar->GetChildrenCount() > 1 ? Style.ActionButtonGap : 0.0f,
                                       0.0f, 0.0f, 0.0f));
    }
    Button->SetActionBarPromptOnly(false);
    return Button;
}

void UMythicCharacterPageWidget::BuildInventoryActionBar() {
    if (!InventoryActionBar || PrimaryActionButton) {
        return;
    }
    const UMythicUIStyleSettings &Style = FMythicUIStyle::Get();
    PrimaryActionButton = CreateInventoryActionButton(Style.PrimaryActionButtonStyle.LoadSynchronous());
    ActionsActionButton = CreateInventoryActionButton(Style.SecondaryActionButtonStyle.LoadSynchronous());
    CompareActionButton = CreateInventoryActionButton(Style.SecondaryActionButtonStyle.LoadSynchronous());
    SortActionButton = CreateInventoryActionButton(Style.QuietActionButtonStyle.LoadSynchronous());

    if (PrimaryActionButton) {
        PrimaryActionButton->OnClicked().AddUObject(this, &UMythicCharacterPageWidget::HandlePrimaryInventoryAction);
    }
    if (ActionsActionButton) {
        ActionsActionButton->OnClicked().AddUObject(this, &UMythicCharacterPageWidget::HandleInventoryActionsAction);
    }
    if (CompareActionButton) {
        CompareActionButton->OnClicked().AddUObject(this, &UMythicCharacterPageWidget::HandleCompareInventoryAction);
    }
    if (SortActionButton) {
        SortActionButton->OnClicked().AddUObject(this, &UMythicCharacterPageWidget::HandleSortInventoryAction);
    }
}

void UMythicCharacterPageWidget::BindInventoryInputs() {
    ReleaseInventoryInputs();
    AddUIInputContext();

    auto Bind = [this](TSoftObjectPtr<UInputAction> &ActionPtr, FName FunctionName,
                       FInputActionBindingHandle &OutHandle) {
        UInputAction *Action = ActionPtr.LoadSynchronous();
        if (!Action) {
            return;
        }
        FInputActionExecutedDelegate Callback;
        Callback.BindUFunction(this, FunctionName);
        RegisterInputActionBinding(Action, IE_Pressed, Callback, false, OutHandle);
    };
    Bind(InventoryPrimaryInputAction,
         GET_FUNCTION_NAME_CHECKED(UMythicCharacterPageWidget, HandlePrimaryInventoryAction), PrimaryBinding);
    Bind(InventoryActionsInputAction,
         GET_FUNCTION_NAME_CHECKED(UMythicCharacterPageWidget, HandleInventoryActionsAction), ActionsBinding);
    Bind(InventoryCompareInputAction,
         GET_FUNCTION_NAME_CHECKED(UMythicCharacterPageWidget, HandleCompareInventoryAction), CompareBinding);
    Bind(InventoryPreviousCategoryInputAction,
         GET_FUNCTION_NAME_CHECKED(UMythicCharacterPageWidget, HandlePreviousInventoryCategory), PreviousCategoryBinding);
    Bind(InventoryNextCategoryInputAction,
         GET_FUNCTION_NAME_CHECKED(UMythicCharacterPageWidget, HandleNextInventoryCategory), NextCategoryBinding);
    Bind(InventorySortInputAction,
         GET_FUNCTION_NAME_CHECKED(UMythicCharacterPageWidget, HandleSortInventoryAction), SortBinding);

    if (PrimaryActionButton && PrimaryBinding.Handle.IsValid()) {
        PrimaryActionButton->SetRepresentedAction(PrimaryBinding.Handle);
    }
    if (ActionsActionButton && ActionsBinding.Handle.IsValid()) {
        ActionsActionButton->SetRepresentedAction(ActionsBinding.Handle);
    }
    if (CompareActionButton && CompareBinding.Handle.IsValid()) {
        CompareActionButton->SetRepresentedAction(CompareBinding.Handle);
    }
    if (SortActionButton && SortBinding.Handle.IsValid()) {
        SortActionButton->SetRepresentedAction(SortBinding.Handle);
    }
}

void UMythicCharacterPageWidget::ReleaseInventoryInputs() {
    const FInputActionBindingHandle Handles[] = {
        PrimaryBinding, ActionsBinding, CompareBinding, PreviousCategoryBinding, NextCategoryBinding, SortBinding
    };
    for (const FInputActionBindingHandle &Handle : Handles) {
        if (Handle.Handle.IsValid()) {
            UnregisterInputBinding(Handle);
        }
    }
    PrimaryBinding = FInputActionBindingHandle();
    ActionsBinding = FInputActionBindingHandle();
    CompareBinding = FInputActionBindingHandle();
    PreviousCategoryBinding = FInputActionBindingHandle();
    NextCategoryBinding = FInputActionBindingHandle();
    SortBinding = FInputActionBindingHandle();
    RemoveUIInputContext();
}

void UMythicCharacterPageWidget::BindInventoryEvents() {
    if (UMythicInventoryComponent *Inventory = GetInventoryComponent()) {
        Inventory->OnSlotUpdated.AddUniqueDynamic(this, &UMythicCharacterPageWidget::HandleInventorySlotUpdated);
    }
    if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer())) {
        PC->OnInventoryActionReceiptReceived.AddUniqueDynamic(
            this, &UMythicCharacterPageWidget::HandleInventoryActionReceipt);
    }
}

void UMythicCharacterPageWidget::ReleaseInventoryEvents() {
    if (UMythicInventoryComponent *Inventory = GetInventoryComponent()) {
        Inventory->OnSlotUpdated.RemoveDynamic(this, &UMythicCharacterPageWidget::HandleInventorySlotUpdated);
    }
    if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer())) {
        PC->OnInventoryActionReceiptReceived.RemoveDynamic(
            this, &UMythicCharacterPageWidget::HandleInventoryActionReceipt);
    }
}

void UMythicCharacterPageWidget::HandlePreviousInventoryCategory() {
    if (InventoryPageState == EInventoryPageState::Browsing) {
        CycleBagCategoryBack();
    }
}

void UMythicCharacterPageWidget::HandleNextInventoryCategory() {
    if (InventoryPageState == EInventoryPageState::Browsing) {
        CycleBagCategoryForward();
    }
}

void UMythicCharacterPageWidget::NativeOnDeactivated() {
    CloseInventoryModal(false);
    ResetInventoryInteractionState();
    ReleaseInventoryInputs();
    ReleaseInventoryEvents();
    UnbindSlotSelection();
    ReturnInventory();
    UnbindProgression();
    Super::NativeOnDeactivated();
}

void UMythicCharacterPageWidget::NativeDestruct() {
    CloseInventoryModal(false);
    ResetInventoryInteractionState();
    ReleaseInventoryInputs();
    ReleaseInventoryEvents();
    UnbindSlotSelection();
    ReturnInventory();
    UnbindProgression();
    Super::NativeDestruct();
}

void UMythicCharacterPageWidget::ResetInventoryInteractionState() {
    PendingRequestId = 0;
    PendingActionValue = INDEX_NONE;
    InventoryPageState = EInventoryPageState::Browsing;
    QuantityPurpose = EQuantityPurpose::None;
    QuantityValue = 1;
    QuantityMaximum = 1;
    ActiveTargetSlotIndex = INDEX_NONE;
    StickyEquipmentTargetSlotIndex = INDEX_NONE;
    bTargetPickerForComparison = false;
    bSelectionRestoreScheduled = false;
    ActionSourceGuid.Invalidate();
    ActionSourceSlotIndex = INDEX_NONE;
    ComparisonVM = nullptr;
}

void UMythicCharacterPageWidget::CollectSlotLists(UUserWidget *Root, TArray<UListViewBase *> &Out) {
    if (!Root || !Root->WidgetTree) {
        return;
    }

    Root->WidgetTree->ForEachWidget([&Out](UWidget *Widget) {
        if (UListViewBase *List = Cast<UListViewBase>(Widget)) {
            Out.AddUnique(List);
        }
        else if (UUserWidget *Nested = Cast<UUserWidget>(Widget)) {
            CollectSlotLists(Nested, Out);
        }
    });
}

void UMythicCharacterPageWidget::BindSlotSelection() {
    if (!DetailsCard || BoundSlotLists.Num() > 0) {
        return;
    }

    UUserWidget *Inventory = Cast<UUserWidget>(BorrowedInventory.Get());
    if (!Inventory) {
        return;
    }

    TArray<UListViewBase *> Lists;
    CollectSlotLists(Inventory, Lists);

    for (UListViewBase *List : Lists) {
        // The category rail is also a UListView. Its entries are UInventoryTabVMs and must never participate
        // in item selection or clear the detail card when a category changes.
        bool bContainsItemSlots = false;
        if (UListView *ConcreteList = Cast<UListView>(List)) {
            for (UObject *Entry : ConcreteList->GetListItems()) {
                if (Cast<UItemSlotVM>(Entry)) {
                    bContainsItemSlots = true;
                    break;
                }
            }
        }
        if (!bContainsItemSlots) {
            continue;
        }
        if (ITypedUMGListView<UObject *> *Typed = AsTypedList(List)) {
            Typed->OnItemSelectionChanged().AddUObject(this, &UMythicCharacterPageWidget::HandleSlotSelectionChanged);
            BoundSlotLists.Add(List);
        }
    }
}

void UMythicCharacterPageWidget::SelectFirstOccupiedSlot() {
    /**
     * Open on something rather than on a hole.
     *
     * The page used to activate with no selection, so the detail column - the widest thing on screen - was
     * an empty rectangle around one line of italic hint text every single time the page was opened. The
     * placeholder is for a genuinely empty bag, not for the normal case.
     */
    for (const TWeakObjectPtr<UListViewBase> &Weak : BoundSlotLists) {
        // UTileView derives from UListView, so one cast covers both kinds of grid on this page.
        UListView *List = Cast<UListView>(Weak.Get());
        if (!List) {
            continue;
        }
        for (UObject *Item : List->GetListItems()) {
            const UItemSlotVM *SlotVM = Cast<UItemSlotVM>(Item);
            if (SlotVM && SlotVM->TryGetItemInstance()) {
                List->SetSelectedItem(Item);
                return;
            }
        }
    }

    ShowDetailsFor(nullptr);
}

ITypedUMGListView<UObject *> *UMythicCharacterPageWidget::AsTypedList(UListViewBase *List) {
    if (UListView *LV = Cast<UListView>(List)) {
        return LV;
    }
    if (UTileView *TV = Cast<UTileView>(List)) {
        return TV;
    }
    if (UTreeView *TrV = Cast<UTreeView>(List)) {
        return TrV;
    }
    return nullptr;
}

void UMythicCharacterPageWidget::UnbindSlotSelection() {
    for (const TWeakObjectPtr<UListViewBase> &Weak : BoundSlotLists) {
        if (ITypedUMGListView<UObject *> *Typed = AsTypedList(Weak.Get())) {
            Typed->OnItemSelectionChanged().RemoveAll(this);
        }
    }
    BoundSlotLists.Reset();
    SelectedList.Reset();
    SelectedItemGuid.Invalidate();
    LastSelectedSlotIndex = INDEX_NONE;
    ShowDetailsFor(nullptr);
}

void UMythicCharacterPageWidget::HandleSlotSelectionChanged(UObject *Item) {
    UItemSlotVM *SlotVM = Cast<UItemSlotVM>(Item);
    if (!SlotVM || bSynchronizingSelection) {
        return;
    }

    UListViewBase *SourceList = FindListSelecting(Item);
    if (InventoryPageState == EInventoryPageState::MoveTarget) {
        ActiveTargetSlotIndex = SlotVM->GetAbsoluteIndex();
        SetFeedback(FText::Format(NSLOCTEXT("MythicInventory", "MoveTargetSelected", "Move to {0} — press Move Here to confirm"),
                                  GetSlotDisplayName(ActiveTargetSlotIndex)));
        RefreshInventoryActionBar();
        return;
    }

    if (UMythicItemInstance *ItemInstance = SlotVM->TryGetItemInstance()) {
        SetSelectedSlot(SlotVM, SourceList);
    }
}

UListViewBase *UMythicCharacterPageWidget::FindListSelecting(UObject *Item) const {
    for (const TWeakObjectPtr<UListViewBase> &WeakList : BoundSlotLists) {
        if (UListView *List = Cast<UListView>(WeakList.Get())) {
            if (List->GetSelectedItem() == Item) {
                return List;
            }
        }
    }
    return nullptr;
}

void UMythicCharacterPageWidget::ClearOtherListSelections(UListViewBase *Except) {
    TGuardValue<bool> SelectionGuard(bSynchronizingSelection, true);
    for (const TWeakObjectPtr<UListViewBase> &WeakList : BoundSlotLists) {
        UListView *List = Cast<UListView>(WeakList.Get());
        if (List && List != Except && List->GetSelectedItem()) {
            List->ClearSelection();
        }
    }
}

void UMythicCharacterPageWidget::SetSelectedSlot(UItemSlotVM *SlotVM, UListViewBase *SourceList) {
    UMythicItemInstance *Item = SlotVM ? SlotVM->TryGetItemInstance() : nullptr;
    if (!Item || !Item->GetItemInstanceGuid().IsValid()) {
        return;
    }
    ClearOtherListSelections(SourceList);
    SelectedList = SourceList;
    SelectedItemGuid = Item->GetItemInstanceGuid();
    LastSelectedSlotIndex = SlotVM->GetAbsoluteIndex();
    ActionSourceGuid.Invalidate();
    ActionSourceSlotIndex = INDEX_NONE;
    ActiveTargetSlotIndex = INDEX_NONE;
    ShowDetailsFor(SlotVM);
    if (UInventoryVM *VM = BoundInventoryVM.Get()) {
        if (VM->SelectionVM) {
            VM->SelectionVM->SetSelectedSlotVM(SlotVM);
        }
    }
    SetFeedback(FText::GetEmpty());
    RefreshInventoryActionBar();
}

UItemSlotVM *UMythicCharacterPageWidget::GetSelectedSlot() const {
    UInventoryVM *VM = BoundInventoryVM.Get();
    if (!VM) {
        return nullptr;
    }
    const FGuid Guid = ActionSourceGuid.IsValid() ? ActionSourceGuid : SelectedItemGuid;
    return VM->FindSlotByItemGuid(Guid);
}

UMythicInventoryComponent *UMythicCharacterPageWidget::GetInventoryComponent() const {
    return BoundInventoryVM.IsValid() ? BoundInventoryVM->GetOwningInventoryComponent() : nullptr;
}

void UMythicCharacterPageWidget::ScheduleRestoreSelection() {
    if (bSelectionRestoreScheduled || !GetWorld()) {
        return;
    }
    bSelectionRestoreScheduled = true;
    GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]() {
        bSelectionRestoreScheduled = false;
        RestoreSelectionByGuid();
    }));
}

void UMythicCharacterPageWidget::RestoreSelectionByGuid() {
    if (!IsActivated() || InventoryPageState == EInventoryPageState::MoveTarget) {
        return;
    }
    UInventoryVM *VM = BoundInventoryVM.Get();
    if (!VM) {
        return;
    }

    UItemSlotVM *SlotToSelect = VM->FindSlotByItemGuid(SelectedItemGuid);
    if (!SlotToSelect) {
        int32 BestDistance = MAX_int32;
        for (UItemSlotVM *Candidate : VM->AbsoluteIndexToSlotVM) {
            if (!Candidate || !Candidate->TryGetItemInstance()) {
                continue;
            }
            const int32 Distance = LastSelectedSlotIndex == INDEX_NONE
                ? Candidate->GetAbsoluteIndex()
                : FMath::Abs(Candidate->GetAbsoluteIndex() - LastSelectedSlotIndex);
            if (Distance < BestDistance) {
                BestDistance = Distance;
                SlotToSelect = Candidate;
            }
        }
    }
    if (!SlotToSelect) {
        SelectedItemGuid.Invalidate();
        LastSelectedSlotIndex = INDEX_NONE;
        ShowDetailsFor(nullptr);
        RefreshInventoryActionBar();
        return;
    }

    for (const TWeakObjectPtr<UListViewBase> &WeakList : BoundSlotLists) {
        UListView *List = Cast<UListView>(WeakList.Get());
        if (List && List->GetListItems().Contains(SlotToSelect)) {
            TGuardValue<bool> SelectionGuard(bSynchronizingSelection, true);
            ClearOtherListSelections(List);
            List->SetSelectedItem(SlotToSelect);
            SelectedList = List;
            SelectedItemGuid = SlotToSelect->TryGetItemInstance()->GetItemInstanceGuid();
            LastSelectedSlotIndex = SlotToSelect->GetAbsoluteIndex();
            ShowDetailsFor(SlotToSelect);
            RefreshInventoryActionBar();
            return;
        }
    }
}

bool UMythicCharacterPageWidget::BuildSourceLocator(FMythicInventorySourceLocator &OutSource) const {
    UItemSlotVM *SlotVM = GetSelectedSlot();
    UMythicInventoryComponent *Inventory = GetInventoryComponent();
    UMythicItemInstance *Item = SlotVM ? SlotVM->TryGetItemInstance() : nullptr;
    if (!Inventory || !Item || !Item->GetItemInstanceGuid().IsValid() || Item->GetStacks() < 1) {
        return false;
    }
    OutSource.Inventory = Inventory;
    OutSource.SlotIndex = SlotVM->GetAbsoluteIndex();
    OutSource.ExpectedItemGuid = Item->GetItemInstanceGuid();
    OutSource.ExpectedQuantity = Item->GetStacks();
    return OutSource.IsStructurallyValid();
}

bool UMythicCharacterPageWidget::BuildTargetLocator(
    int32 SlotIndex, FMythicInventoryTargetLocator &OutTarget) const {
    UMythicInventoryComponent *Inventory = GetInventoryComponent();
    if (!Inventory) {
        return false;
    }
    FMythicInventorySlotEntry Entry;
    if (!Inventory->GetSlotEntry(SlotIndex, Entry)) {
        return false;
    }
    OutTarget.Inventory = Inventory;
    OutTarget.SlotIndex = SlotIndex;
    OutTarget.bExpectEmpty = Entry.SlottedItemInstance == nullptr;
    if (Entry.SlottedItemInstance) {
        OutTarget.ExpectedOccupantGuid = Entry.SlottedItemInstance->GetItemInstanceGuid();
        OutTarget.ExpectedOccupantQuantity = Entry.SlottedItemInstance->GetStacks();
    }
    else {
        OutTarget.ExpectedOccupantGuid.Invalidate();
        OutTarget.ExpectedOccupantQuantity = 0;
    }
    return OutTarget.IsStructurallyValid();
}

FText UMythicCharacterPageWidget::GetSlotDisplayName(int32 SlotIndex) const {
    UMythicInventoryComponent *Inventory = GetInventoryComponent();
    if (!Inventory) {
        return NSLOCTEXT("MythicInventory", "UnknownSlot", "Unknown Slot");
    }
    FMythicInventorySlotEntry Entry;
    if (!Inventory->GetSlotEntry(SlotIndex, Entry) || !Entry.SlotDefinition) {
        return FText::Format(NSLOCTEXT("MythicInventory", "SlotNumber", "Slot {0}"),
                             FText::AsNumber(SlotIndex + 1));
    }

    int32 TotalMatching = 0;
    int32 Ordinal = 0;
    for (int32 Index = 0; Index < Inventory->GetAllSlots().Num(); ++Index) {
        const FMythicInventorySlotEntry &Candidate = Inventory->GetAllSlots()[Index];
        if (Candidate.SlotDefinition == Entry.SlotDefinition) {
            ++TotalMatching;
            if (Index <= SlotIndex) {
                ++Ordinal;
            }
        }
    }
    const FText BaseName = Entry.SlotDefinition->DisplayName.IsEmpty()
        ? FText::FromString(Entry.GroupTag.ToString())
        : Entry.SlotDefinition->DisplayName;
    return TotalMatching > 1
        ? FText::Format(NSLOCTEXT("MythicInventory", "RepeatedSlotName", "{0} {1}"),
                        BaseName, FText::AsNumber(Ordinal))
        : BaseName;
}

TArray<UMythicCharacterPageWidget::FEquipmentTarget>
UMythicCharacterPageWidget::BuildEquipmentTargets() const {
    TArray<FEquipmentTarget> Targets;
    UMythicInventoryComponent *Inventory = GetInventoryComponent();
    UItemSlotVM *SourceSlot = GetSelectedSlot();
    UMythicItemInstance *Item = SourceSlot ? SourceSlot->TryGetItemInstance() : nullptr;
    if (!Inventory || !Item || !Item->GetItemDefinition()
        || !Item->GetItemDefinition()->ItemType.MatchesTag(ITEMIZATION_TYPE_EQUIPMENT)) {
        return Targets;
    }
    for (int32 Index = 0; Index < Inventory->GetAllSlots().Num(); ++Index) {
        const FMythicInventorySlotEntry &Entry = Inventory->GetAllSlots()[Index];
        if (!Entry.IsGearSlot() || Index == SourceSlot->GetAbsoluteIndex()
            || !Inventory->CanSlotAcceptItem(Index, Item, true, Item)) {
            continue;
        }
        FEquipmentTarget Target;
        Target.SlotIndex = Index;
        Target.DisplayName = GetSlotDisplayName(Index);
        Target.bExpectEmpty = Entry.SlottedItemInstance == nullptr;
        if (Entry.SlottedItemInstance) {
            Target.OccupantGuid = Entry.SlottedItemInstance->GetItemInstanceGuid();
            Target.OccupantQuantity = Entry.SlottedItemInstance->GetStacks();
        }
        Targets.Add(MoveTemp(Target));
    }
    Targets.Sort([](const FEquipmentTarget &A, const FEquipmentTarget &B) {
        return A.SlotIndex < B.SlotIndex;
    });
    return Targets;
}

void UMythicCharacterPageWidget::RefreshInventoryActionBar() {
    UItemSlotVM *SlotVM = GetSelectedSlot();
    UMythicInventoryComponent *Inventory = GetInventoryComponent();
    UMythicItemInstance *Item = SlotVM ? SlotVM->TryGetItemInstance() : nullptr;
    const bool bHasItem = Item && Item->GetItemInstanceGuid().IsValid();
    const bool bBrowsing = InventoryPageState == EInventoryPageState::Browsing;
    const bool bMoveTarget = InventoryPageState == EInventoryPageState::MoveTarget;

    if (PrimaryActionButton) {
        FText Label = NSLOCTEXT("MythicInventory", "InspectAction", "Inspect");
        bool bEnabled = bBrowsing && bHasItem;
        if (bMoveTarget) {
            Label = NSLOCTEXT("MythicInventory", "MoveHereAction", "Move Here");
            bEnabled = ActiveTargetSlotIndex != INDEX_NONE
                && ActiveTargetSlotIndex != ActionSourceSlotIndex;
        }
        else if (bBrowsing && bHasItem) {
            if (SlotVM->GetIsEquipped()) {
                Label = NSLOCTEXT("MythicInventory", "UnequipAction", "Unequip");
            }
            else if (BuildEquipmentTargets().Num() > 0) {
                Label = NSLOCTEXT("MythicInventory", "EquipAction", "Equip");
            }
            else if (Inventory && Inventory->CanUseItemInSlot(SlotVM->GetAbsoluteIndex())) {
                Label = NSLOCTEXT("MythicInventory", "UseAction", "Use");
            }
        }
        PrimaryActionButton->SetLabelOverride(Label);
        PrimaryActionButton->SetIsEnabled(bEnabled);
    }
    if (ActionsActionButton) {
        ActionsActionButton->SetLabelOverride(
            bMoveTarget ? NSLOCTEXT("MythicInventory", "CancelMoveAction", "Cancel Move")
                        : NSLOCTEXT("MythicInventory", "ActionsAction", "Actions"));
        ActionsActionButton->SetIsEnabled((bBrowsing && bHasItem) || bMoveTarget);
    }
    if (CompareActionButton) {
        CompareActionButton->SetLabelOverride(NSLOCTEXT("MythicInventory", "CompareAction", "Compare"));
        CompareActionButton->SetIsEnabled(
            bBrowsing && bHasItem && !SlotVM->GetIsEquipped() && BuildEquipmentTargets().Num() > 0);
    }
    if (SortActionButton) {
        SortActionButton->SetLabelOverride(NSLOCTEXT("MythicInventory", "SortAction", "Sort"));
        bool bCanSort = false;
        if (bBrowsing && Inventory && SlotVM) {
            FMythicInventorySlotEntry Entry;
            bCanSort = Inventory->GetSlotEntry(SlotVM->GetAbsoluteIndex(), Entry) && !Entry.IsGearSlot();
        }
        SortActionButton->SetIsEnabled(bCanSort);
    }
}

void UMythicCharacterPageWidget::HandlePrimaryInventoryAction() {
    if (InventoryPageState == EInventoryPageState::MoveTarget) {
        ConfirmMoveTarget();
        return;
    }
    if (InventoryPageState != EInventoryPageState::Browsing) {
        return;
    }

    UItemSlotVM *SlotVM = GetSelectedSlot();
    UMythicInventoryComponent *Inventory = GetInventoryComponent();
    if (!SlotVM || !SlotVM->TryGetItemInstance() || !Inventory) {
        return;
    }
    if (SlotVM->GetIsEquipped()) {
        BeginMoveTargetSelection();
        return;
    }

    const TArray<FEquipmentTarget> Targets = BuildEquipmentTargets();
    if (Targets.Num() > 0) {
        const FEquipmentTarget *Sticky = Targets.FindByPredicate([this](const FEquipmentTarget &Target) {
            return Target.SlotIndex == StickyEquipmentTargetSlotIndex;
        });
        if (Sticky) {
            SubmitMoveToSlot(Sticky->SlotIndex);
        }
        else if (Targets.Num() == 1) {
            StickyEquipmentTargetSlotIndex = Targets[0].SlotIndex;
            SubmitMoveToSlot(Targets[0].SlotIndex);
        }
        else {
            OpenEquipmentTargetPicker(false);
        }
        return;
    }
    if (Inventory->CanUseItemInSlot(SlotVM->GetAbsoluteIndex())) {
        SubmitUse();
        return;
    }
    ShowDetailsFor(SlotVM);
    SetFeedback(NSLOCTEXT("MythicInventory", "InspectFeedback", "Item details are open on the right."));
}

void UMythicCharacterPageWidget::HandleInventoryActionsAction() {
    if (InventoryPageState == EInventoryPageState::MoveTarget) {
        CloseInventoryModal(true);
        return;
    }
    if (InventoryPageState == EInventoryPageState::Browsing && GetSelectedSlot()) {
        OpenActionMenu();
    }
}

void UMythicCharacterPageWidget::HandleCompareInventoryAction() {
    if (InventoryPageState != EInventoryPageState::Browsing || !GetSelectedSlot()
        || GetSelectedSlot()->GetIsEquipped()) {
        return;
    }
    const TArray<FEquipmentTarget> Targets = BuildEquipmentTargets();
    if (Targets.Num() == 0) {
        SetFeedback(NSLOCTEXT("MythicInventory", "NoCompareTarget", "No compatible equipment slot is available."), true);
        return;
    }
    const FEquipmentTarget *Sticky = Targets.FindByPredicate([this](const FEquipmentTarget &Target) {
        return Target.SlotIndex == StickyEquipmentTargetSlotIndex;
    });
    if (Sticky) {
        OpenComparison(Sticky->SlotIndex);
    }
    else if (Targets.Num() == 1) {
        StickyEquipmentTargetSlotIndex = Targets[0].SlotIndex;
        OpenComparison(Targets[0].SlotIndex);
    }
    else {
        OpenEquipmentTargetPicker(true);
    }
}

void UMythicCharacterPageWidget::HandleSortInventoryAction() {
    if (InventoryPageState == EInventoryPageState::Browsing && GetSelectedSlot()) {
        OpenSortMenu();
    }
}

void UMythicCharacterPageWidget::ClearModalOptions() {
    InventoryClickProxies.Reset();
    if (InventoryModalOptions) {
        InventoryModalOptions->ClearChildren();
    }
}

UWidget *UMythicCharacterPageWidget::AddModalCommand(
    const FText &Label, EMythicInventoryUICommand Command, int32 Payload,
    bool bEnabled, const FText &Tooltip) {
    if (!InventoryModalOptions) {
        return nullptr;
    }
    UCommonTextBlock *LabelWidget = nullptr;
    UWidget *Button = FMythicUIStyle::MakeButton(this, EMythicTextRole::Body, LabelWidget);
    if (!Button) {
        return nullptr;
    }
    if (LabelWidget) {
        LabelWidget->SetText(Label);
    }
    Button->SetIsEnabled(bEnabled);
    if (!Tooltip.IsEmpty()) {
        Button->SetToolTipText(Tooltip);
    }

    UMythicInventoryActionClickProxy *Proxy = NewObject<UMythicInventoryActionClickProxy>(this);
    Proxy->Page = this;
    Proxy->Command = Command;
    Proxy->Payload = Payload;
    InventoryClickProxies.Add(Proxy);
    FMythicUIStyle::BindButtonClicked(
        Button, Proxy, GET_FUNCTION_NAME_CHECKED(UMythicInventoryActionClickProxy, HandleClicked));

    if (UVerticalBoxSlot *OptionSlot = Cast<UVerticalBoxSlot>(InventoryModalOptions->AddChild(Button))) {
        OptionSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, FMythicUIStyle::Get().SpaceS));
    }
    return Button;
}

void UMythicCharacterPageWidget::OpenActionMenu() {
    UItemSlotVM *SlotVM = GetSelectedSlot();
    UMythicInventoryComponent *Inventory = GetInventoryComponent();
    UMythicItemInstance *Item = SlotVM ? SlotVM->TryGetItemInstance() : nullptr;
    if (!SlotVM || !Inventory || !Item || !InventoryModalLayer) {
        return;
    }
    ActionSourceGuid = Item->GetItemInstanceGuid();
    ActionSourceSlotIndex = SlotVM->GetAbsoluteIndex();
    InventoryPageState = EInventoryPageState::ActionMenu;
    ClearModalOptions();
    InventoryModalTitle->SetText(SlotVM->GetItemName());
    InventoryModalBody->SetText(NSLOCTEXT("MythicInventory", "ActionMenuHint", "Choose an action. Every option uses the same authoritative item identity."));

    const bool bCanTake = Inventory->CanPlayerTakeFromSlot(SlotVM->GetAbsoluteIndex());
    const bool bEquipped = SlotVM->GetIsEquipped();
    const bool bCanEquip = !bEquipped && BuildEquipmentTargets().Num() > 0;
    AddModalCommand(bEquipped
                        ? NSLOCTEXT("MythicInventory", "UnequipMenu", "Unequip")
                        : NSLOCTEXT("MythicInventory", "EquipMenu", "Equip"),
                    EMythicInventoryUICommand::EquipOrUnequip,
                    INDEX_NONE, bEquipped ? bCanTake : bCanEquip);
    AddModalCommand(NSLOCTEXT("MythicInventory", "UseMenu", "Use"),
                    EMythicInventoryUICommand::Use, INDEX_NONE,
                    Inventory->CanUseItemInSlot(SlotVM->GetAbsoluteIndex()));
    AddModalCommand(NSLOCTEXT("MythicInventory", "MoveMenu", "Move to Slot"),
                    EMythicInventoryUICommand::BeginMove, INDEX_NONE, bCanTake);
    AddModalCommand(NSLOCTEXT("MythicInventory", "CompareMenu", "Compare"),
                    EMythicInventoryUICommand::Compare, INDEX_NONE,
                    !bEquipped && BuildEquipmentTargets().Num() > 0);
    AddModalCommand(NSLOCTEXT("MythicInventory", "SplitMenu", "Split Stack"),
                    EMythicInventoryUICommand::Split, INDEX_NONE,
                    bCanTake && Item->GetStacks() > 1);
    AddModalCommand(NSLOCTEXT("MythicInventory", "DropMenu", "Drop"),
                    EMythicInventoryUICommand::Drop, INDEX_NONE, bCanTake);
    AddModalCommand(SlotVM->GetIsJunk()
                        ? NSLOCTEXT("MythicInventory", "UnmarkJunkMenu", "Remove Junk Mark")
                        : NSLOCTEXT("MythicInventory", "MarkJunkMenu", "Mark as Junk"),
                    EMythicInventoryUICommand::ToggleJunk, INDEX_NONE, bCanTake);
    AddModalCommand(NSLOCTEXT("MythicInventory", "SortMenu", "Sort Category"),
                    EMythicInventoryUICommand::OpenSortMenu, INDEX_NONE, !bEquipped,
                    NSLOCTEXT("MythicInventory", "SortMenuTooltip", "Opens all sort choices."));
    AddModalCommand(NSLOCTEXT("MythicInventory", "InspectMenu", "Inspect"),
                    EMythicInventoryUICommand::Inspect);
    AddModalCommand(NSLOCTEXT("MythicInventory", "CancelMenu", "Back"),
                    EMythicInventoryUICommand::Cancel);
    InventoryModalLayer->SetVisibility(ESlateVisibility::Visible);
    if (UWidget *First = InventoryModalOptions->GetChildAt(0)) {
        First->SetFocus();
    }
    RefreshInventoryActionBar();
}

void UMythicCharacterPageWidget::OpenSortMenu() {
    UItemSlotVM *SlotVM = GetSelectedSlot();
    UMythicInventoryComponent *Inventory = GetInventoryComponent();
    if (!SlotVM || !Inventory || !InventoryModalLayer) {
        return;
    }
    FMythicInventorySlotEntry Entry;
    if (!Inventory->GetSlotEntry(SlotVM->GetAbsoluteIndex(), Entry) || Entry.IsGearSlot()) {
        SetFeedback(NSLOCTEXT("MythicInventory", "SortEquipmentDenied", "Equipment slots keep their authored order."), true);
        return;
    }
    if (UMythicItemInstance *Item = SlotVM->TryGetItemInstance()) {
        ActionSourceGuid = Item->GetItemInstanceGuid();
        ActionSourceSlotIndex = SlotVM->GetAbsoluteIndex();
    }
    InventoryPageState = EInventoryPageState::SortMenu;
    ClearModalOptions();
    InventoryModalTitle->SetText(NSLOCTEXT("MythicInventory", "SortTitle", "Sort Category"));
    InventoryModalBody->SetText(NSLOCTEXT("MythicInventory", "SortHint", "Choose how this carried-item group is ordered."));
    AddModalCommand(NSLOCTEXT("MythicInventory", "SortRarity", "Rarity"), EMythicInventoryUICommand::SortByRarity);
    AddModalCommand(NSLOCTEXT("MythicInventory", "SortType", "Type"), EMythicInventoryUICommand::SortByType);
    AddModalCommand(NSLOCTEXT("MythicInventory", "SortName", "Name"), EMythicInventoryUICommand::SortByName);
    AddModalCommand(NSLOCTEXT("MythicInventory", "SortValue", "Value"), EMythicInventoryUICommand::SortByValue);
    AddModalCommand(NSLOCTEXT("MythicInventory", "SortWeight", "Weight"), EMythicInventoryUICommand::SortByWeight);
    AddModalCommand(NSLOCTEXT("MythicInventory", "CancelSort", "Back"), EMythicInventoryUICommand::Cancel);
    InventoryModalLayer->SetVisibility(ESlateVisibility::Visible);
    InventoryModalOptions->GetChildAt(0)->SetFocus();
    RefreshInventoryActionBar();
}

void UMythicCharacterPageWidget::OpenQuantityPanel(EQuantityPurpose Purpose) {
    UItemSlotVM *SlotVM = GetSelectedSlot();
    UMythicItemInstance *Item = SlotVM ? SlotVM->TryGetItemInstance() : nullptr;
    if (!Item || !InventoryModalLayer) {
        return;
    }
    QuantityPurpose = Purpose;
    QuantityMaximum = Purpose == EQuantityPurpose::Split
        ? FMath::Max(1, Item->GetStacks() - 1)
        : Item->GetStacks();
    QuantityValue = Purpose == EQuantityPurpose::Split
        ? FMath::Clamp(Item->GetStacks() / 2, 1, QuantityMaximum)
        : QuantityMaximum;
    InventoryPageState = EInventoryPageState::Quantity;
    ClearModalOptions();
    InventoryModalTitle->SetText(
        Purpose == EQuantityPurpose::Split
            ? NSLOCTEXT("MythicInventory", "SplitQuantityTitle", "Split Stack")
            : NSLOCTEXT("MythicInventory", "DropQuantityTitle", "Drop Quantity"));
    InventoryModalBody->SetText(FText::Format(
        NSLOCTEXT("MythicInventory", "QuantityReadout", "Quantity: {0} / {1}"),
        FText::AsNumber(QuantityValue), FText::AsNumber(QuantityMaximum)));
    AddModalCommand(NSLOCTEXT("MythicInventory", "QuantityDecrease", "−  Decrease"),
                    EMythicInventoryUICommand::QuantityDecrease);
    AddModalCommand(NSLOCTEXT("MythicInventory", "QuantityIncrease", "+  Increase"),
                    EMythicInventoryUICommand::QuantityIncrease);
    AddModalCommand(Purpose == EQuantityPurpose::Split
                        ? NSLOCTEXT("MythicInventory", "ConfirmSplit", "Confirm Split")
                        : NSLOCTEXT("MythicInventory", "ConfirmDrop", "Drop"),
                    EMythicInventoryUICommand::ConfirmQuantity);
    AddModalCommand(NSLOCTEXT("MythicInventory", "CancelQuantity", "Cancel"),
                    EMythicInventoryUICommand::Cancel);
    InventoryModalLayer->SetVisibility(ESlateVisibility::Visible);
    InventoryModalOptions->GetChildAt(0)->SetFocus();
    RefreshInventoryActionBar();
}

void UMythicCharacterPageWidget::OpenEquipmentTargetPicker(bool bForComparison) {
    const TArray<FEquipmentTarget> Targets = BuildEquipmentTargets();
    if (Targets.Num() == 0 || !InventoryModalLayer) {
        SetFeedback(NSLOCTEXT("MythicInventory", "NoEquipmentTarget", "No compatible equipment slot is available."), true);
        return;
    }
    bTargetPickerForComparison = bForComparison;
    InventoryPageState = EInventoryPageState::EquipmentTarget;
    ClearModalOptions();
    InventoryModalTitle->SetText(
        bForComparison
            ? NSLOCTEXT("MythicInventory", "CompareTargetTitle", "Compare With")
            : NSLOCTEXT("MythicInventory", "EquipTargetTitle", "Equip To"));
    InventoryModalBody->SetText(NSLOCTEXT(
        "MythicInventory", "EquipmentTargetHint",
        "Choose the exact slot. This same target is used for comparison and equip."));
    for (const FEquipmentTarget &Target : Targets) {
        FText Occupant = NSLOCTEXT("MythicInventory", "EmptyTarget", "Empty");
        if (!Target.bExpectEmpty) {
            FMythicInventorySlotEntry Entry;
            if (GetInventoryComponent()->GetSlotEntry(Target.SlotIndex, Entry)
                && Entry.SlottedItemInstance && Entry.SlottedItemInstance->GetItemDefinition()) {
                Occupant = Entry.SlottedItemInstance->GetItemDefinition()->Name;
            }
        }
        AddModalCommand(
            FText::Format(NSLOCTEXT("MythicInventory", "EquipmentTargetRow", "{0}  —  {1}"),
                          Target.DisplayName, Occupant),
            EMythicInventoryUICommand::ChooseEquipmentTarget, Target.SlotIndex);
    }
    AddModalCommand(NSLOCTEXT("MythicInventory", "CancelTarget", "Cancel"),
                    EMythicInventoryUICommand::Cancel);
    InventoryModalLayer->SetVisibility(ESlateVisibility::Visible);
    InventoryModalOptions->GetChildAt(0)->SetFocus();
    RefreshInventoryActionBar();
}

void UMythicCharacterPageWidget::OpenComparison(int32 TargetSlotIndex) {
    UItemSlotVM *SlotVM = GetSelectedSlot();
    UMythicItemInstance *Item = SlotVM ? SlotVM->TryGetItemInstance() : nullptr;
    FMythicInventoryTargetLocator Target;
    if (!Item || !BuildTargetLocator(TargetSlotIndex, Target) || !InventoryModalLayer) {
        SetFeedback(NSLOCTEXT("MythicInventory", "StaleCompareTarget", "That equipment slot changed. Choose it again."), true);
        return;
    }
    StickyEquipmentTargetSlotIndex = TargetSlotIndex;
    ActiveTargetSlotIndex = TargetSlotIndex;
    ComparisonVM = UItemComparisonVM::CreateComparison(
        this, Item, GetInventoryComponent(), TargetSlotIndex,
        Target.bExpectEmpty, Target.ExpectedOccupantGuid);
    if (!ComparisonVM) {
        SetFeedback(NSLOCTEXT("MythicInventory", "CompareUnavailable", "Comparison is unavailable because the target changed."), true);
        return;
    }

    InventoryPageState = EInventoryPageState::Comparison;
    ClearModalOptions();
    InventoryModalTitle->SetText(FText::Format(
        NSLOCTEXT("MythicInventory", "CompareTitle", "Compare — {0}"), GetSlotDisplayName(TargetSlotIndex)));
    InventoryModalBody->SetText(FText::Format(
        NSLOCTEXT("MythicInventory", "CompareBody", "{0} versus the exact item currently in {1}"),
        SlotVM->GetItemName(), GetSlotDisplayName(TargetSlotIndex)));

    const UMythicUIStyleSettings &Style = FMythicUIStyle::Get();
    int32 VisibleRows = 0;
    for (const FAttributeDiff &Diff : ComparisonVM->GetAttributeDiffs()) {
        if (VisibleRows++ >= 12) {
            break;
        }
        UCommonTextBlock *Row = FMythicUIStyle::MakeText(this, EMythicTextRole::Body);
        const bool bNeutral = FMath::IsNearlyZero(Diff.Delta);
        const TCHAR *Arrow = bNeutral ? TEXT("•") : (Diff.bIsUpgrade ? TEXT("▲") : TEXT("▼"));
        Row->SetText(FText::Format(
            NSLOCTEXT("MythicInventory", "ComparisonDeltaRow", "{0}  {1}: {2}"),
            FText::FromString(Arrow), Diff.AttributeName, FText::AsNumber(Diff.Delta)));
        Row->SetColorAndOpacity(FSlateColor(
            bNeutral ? Style.InkSubtle : (Diff.bIsUpgrade ? Style.Positive : Style.Negative)));
        if (UVerticalBoxSlot *RowSlot = Cast<UVerticalBoxSlot>(InventoryModalOptions->AddChild(Row))) {
            RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, Style.SpaceXS));
        }
    }
    if (VisibleRows == 0) {
        UCommonTextBlock *NoDelta = FMythicUIStyle::MakeText(this, EMythicTextRole::Subtle);
        NoDelta->SetText(NSLOCTEXT("MythicInventory", "NoComparisonDeltas", "No comparable stat changes."));
        InventoryModalOptions->AddChild(NoDelta);
    }
    AddModalCommand(NSLOCTEXT("MythicInventory", "EquipComparedItem", "Equip Here"),
                    EMythicInventoryUICommand::ConfirmComparisonEquip, TargetSlotIndex);
    AddModalCommand(NSLOCTEXT("MythicInventory", "CloseComparison", "Back"),
                    EMythicInventoryUICommand::Cancel);
    InventoryModalLayer->SetVisibility(ESlateVisibility::Visible);
    InventoryModalOptions->GetChildAt(FMath::Max(0, InventoryModalOptions->GetChildrenCount() - 2))->SetFocus();
    RefreshInventoryActionBar();
}

void UMythicCharacterPageWidget::CloseInventoryModal(bool bRestoreSourceSelection) {
    if (InventoryModalLayer) {
        InventoryModalLayer->SetVisibility(ESlateVisibility::Collapsed);
    }
    ClearModalOptions();
    ComparisonVM = nullptr;
    QuantityPurpose = EQuantityPurpose::None;
    ActiveTargetSlotIndex = INDEX_NONE;
    bTargetPickerForComparison = false;
    if (InventoryPageState != EInventoryPageState::Pending) {
        InventoryPageState = EInventoryPageState::Browsing;
    }
    if (bRestoreSourceSelection && ActionSourceGuid.IsValid()) {
        SelectedItemGuid = ActionSourceGuid;
        ScheduleRestoreSelection();
    }
    ActionSourceGuid.Invalidate();
    ActionSourceSlotIndex = INDEX_NONE;
    SetFeedback(FText::GetEmpty());
    RefreshInventoryActionBar();
}

void UMythicCharacterPageWidget::SetFeedback(const FText &Message, bool bIsError) {
    if (!InventoryFeedback) {
        return;
    }
    InventoryFeedback->SetText(Message);
    InventoryFeedback->SetColorAndOpacity(FSlateColor(
        bIsError ? FMythicUIStyle::Get().Negative : FMythicUIStyle::Get().InkSubtle));
    InventoryFeedback->SetVisibility(Message.IsEmpty()
        ? ESlateVisibility::Collapsed
        : ESlateVisibility::HitTestInvisible);
}

void UMythicCharacterPageWidget::BeginMoveTargetSelection() {
    FMythicInventorySourceLocator Source;
    if (!BuildSourceLocator(Source)) {
        SetFeedback(NSLOCTEXT("MythicInventory", "InvalidMoveSource", "That item is no longer available."), true);
        return;
    }
    ActionSourceGuid = Source.ExpectedItemGuid;
    ActionSourceSlotIndex = Source.SlotIndex;
    ActiveTargetSlotIndex = INDEX_NONE;
    InventoryPageState = EInventoryPageState::MoveTarget;
    if (InventoryModalLayer) {
        InventoryModalLayer->SetVisibility(ESlateVisibility::Collapsed);
    }
    SetFeedback(NSLOCTEXT(
        "MythicInventory", "ChooseMoveTarget",
        "Choose a destination slot, then press Move Here. Back cancels without dropping the item."));
    RefreshInventoryActionBar();
}

void UMythicCharacterPageWidget::ConfirmMoveTarget() {
    if (InventoryPageState == EInventoryPageState::MoveTarget && ActiveTargetSlotIndex != INDEX_NONE) {
        SubmitMoveToSlot(ActiveTargetSlotIndex);
    }
}

void UMythicCharacterPageWidget::ExecuteInventoryUICommand(
    EMythicInventoryUICommand Command, int32 Payload) {
    switch (Command) {
    case EMythicInventoryUICommand::EquipOrUnequip:
        if (UItemSlotVM *SlotVM = GetSelectedSlot()) {
            if (SlotVM->GetIsEquipped()) {
                BeginMoveTargetSelection();
            }
            else {
                const TArray<FEquipmentTarget> Targets = BuildEquipmentTargets();
                if (Targets.Num() == 1) {
                    StickyEquipmentTargetSlotIndex = Targets[0].SlotIndex;
                    SubmitMoveToSlot(Targets[0].SlotIndex);
                }
                else {
                    OpenEquipmentTargetPicker(false);
                }
            }
        }
        break;
    case EMythicInventoryUICommand::Use:
        SubmitUse();
        break;
    case EMythicInventoryUICommand::BeginMove:
        BeginMoveTargetSelection();
        break;
    case EMythicInventoryUICommand::Compare:
        InventoryPageState = EInventoryPageState::Browsing;
        HandleCompareInventoryAction();
        break;
    case EMythicInventoryUICommand::Split:
        OpenQuantityPanel(EQuantityPurpose::Split);
        break;
    case EMythicInventoryUICommand::Drop:
        OpenQuantityPanel(EQuantityPurpose::Drop);
        break;
    case EMythicInventoryUICommand::ToggleJunk:
        SubmitSetJunk();
        break;
    case EMythicInventoryUICommand::Inspect:
        CloseInventoryModal(true);
        if (UItemSlotVM *SlotVM = GetSelectedSlot()) {
            ShowDetailsFor(SlotVM);
        }
        break;
    case EMythicInventoryUICommand::OpenSortMenu:
        OpenSortMenu();
        break;
    case EMythicInventoryUICommand::SortByRarity:
        SubmitSort(static_cast<int32>(ESortMode::ByRarity));
        break;
    case EMythicInventoryUICommand::SortByType:
        SubmitSort(static_cast<int32>(ESortMode::ByType));
        break;
    case EMythicInventoryUICommand::SortByName:
        SubmitSort(static_cast<int32>(ESortMode::ByName));
        break;
    case EMythicInventoryUICommand::SortByValue:
        SubmitSort(static_cast<int32>(ESortMode::ByValue));
        break;
    case EMythicInventoryUICommand::SortByWeight:
        SubmitSort(static_cast<int32>(ESortMode::ByWeight));
        break;
    case EMythicInventoryUICommand::QuantityDecrease:
        QuantityValue = FMath::Clamp(QuantityValue - 1, 1, QuantityMaximum);
        if (InventoryModalBody) {
            InventoryModalBody->SetText(FText::Format(
                NSLOCTEXT("MythicInventory", "QuantityReadout", "Quantity: {0} / {1}"),
                FText::AsNumber(QuantityValue), FText::AsNumber(QuantityMaximum)));
        }
        break;
    case EMythicInventoryUICommand::QuantityIncrease:
        QuantityValue = FMath::Clamp(QuantityValue + 1, 1, QuantityMaximum);
        if (InventoryModalBody) {
            InventoryModalBody->SetText(FText::Format(
                NSLOCTEXT("MythicInventory", "QuantityReadout", "Quantity: {0} / {1}"),
                FText::AsNumber(QuantityValue), FText::AsNumber(QuantityMaximum)));
        }
        break;
    case EMythicInventoryUICommand::ConfirmQuantity:
        if (QuantityPurpose == EQuantityPurpose::Split) {
            SubmitSplit(QuantityValue);
        }
        else if (QuantityPurpose == EQuantityPurpose::Drop) {
            SubmitDrop(QuantityValue);
        }
        break;
    case EMythicInventoryUICommand::ChooseEquipmentTarget:
        StickyEquipmentTargetSlotIndex = Payload;
        if (bTargetPickerForComparison) {
            OpenComparison(Payload);
        }
        else {
            SubmitMoveToSlot(Payload);
        }
        break;
    case EMythicInventoryUICommand::ConfirmComparisonEquip:
        SubmitMoveToSlot(Payload);
        break;
    case EMythicInventoryUICommand::Cancel:
        CloseInventoryModal(true);
        break;
    }
}

void UMythicCharacterPageWidget::SubmitMoveToSlot(int32 TargetSlotIndex) {
    FMythicInventorySourceLocator Source;
    FMythicInventoryTargetLocator Target;
    if (!BuildSourceLocator(Source) || !BuildTargetLocator(TargetSlotIndex, Target)
        || Source.SlotIndex == Target.SlotIndex) {
        SetFeedback(NSLOCTEXT("MythicInventory", "MoveTargetChanged", "The source or destination changed. Choose again."), true);
        return;
    }
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer());
    BeginPendingRequest(PC ? PC->SubmitInventoryMove(Source, Target) : 0,
                        static_cast<int32>(EMythicInventoryAction::Move));
}

void UMythicCharacterPageWidget::SubmitUse() {
    FMythicInventorySourceLocator Source;
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer());
    if (!BuildSourceLocator(Source) || !PC) {
        SetFeedback(NSLOCTEXT("MythicInventory", "UseSourceChanged", "That item is no longer available."), true);
        return;
    }
    BeginPendingRequest(PC->SubmitInventoryUse(Source), static_cast<int32>(EMythicInventoryAction::Use));
}

void UMythicCharacterPageWidget::SubmitSplit(int32 Quantity) {
    FMythicInventorySourceLocator Source;
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer());
    if (!BuildSourceLocator(Source) || !PC || Quantity < 1 || Quantity >= Source.ExpectedQuantity) {
        SetFeedback(NSLOCTEXT("MythicInventory", "SplitSourceChanged", "The stack changed. Reopen Split."), true);
        return;
    }
    BeginPendingRequest(PC->SubmitInventorySplit(Source, Quantity),
                        static_cast<int32>(EMythicInventoryAction::Split));
}

void UMythicCharacterPageWidget::SubmitDrop(int32 Quantity) {
    FMythicInventorySourceLocator Source;
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer());
    if (!BuildSourceLocator(Source) || !PC || Quantity < 1 || Quantity > Source.ExpectedQuantity) {
        SetFeedback(NSLOCTEXT("MythicInventory", "DropSourceChanged", "The stack changed. Reopen Drop."), true);
        return;
    }
    BeginPendingRequest(PC->SubmitInventoryDropQuantity(Source, Quantity),
                        static_cast<int32>(EMythicInventoryAction::DropQuantity));
}

void UMythicCharacterPageWidget::SubmitSetJunk() {
    FMythicInventorySourceLocator Source;
    UItemSlotVM *SlotVM = GetSelectedSlot();
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer());
    if (!BuildSourceLocator(Source) || !SlotVM || !PC) {
        SetFeedback(NSLOCTEXT("MythicInventory", "JunkSourceChanged", "That item is no longer available."), true);
        return;
    }
    BeginPendingRequest(PC->SubmitInventorySetJunk(Source, !SlotVM->GetIsJunk()),
                        static_cast<int32>(EMythicInventoryAction::SetJunk));
}

void UMythicCharacterPageWidget::SubmitSort(int32 SortModeValue) {
    UItemSlotVM *SlotVM = GetSelectedSlot();
    UMythicInventoryComponent *Inventory = GetInventoryComponent();
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer());
    FMythicInventorySlotEntry Entry;
    if (!SlotVM || !Inventory || !PC
        || !Inventory->GetSlotEntry(SlotVM->GetAbsoluteIndex(), Entry) || Entry.IsGearSlot()) {
        SetFeedback(NSLOCTEXT("MythicInventory", "SortSourceChanged", "That category cannot be sorted."), true);
        return;
    }
    const ESortMode Mode = static_cast<ESortMode>(SortModeValue);
    BeginPendingRequest(PC->SubmitInventorySort(Inventory, Entry.GroupTag, Mode),
                        static_cast<int32>(EMythicInventoryAction::Sort));
}

void UMythicCharacterPageWidget::BeginPendingRequest(int64 RequestId, int32 ActionValue) {
    if (RequestId <= 0) {
        SetFeedback(NSLOCTEXT("MythicInventory", "RequestNotSent", "The inventory request could not be sent."), true);
        return;
    }
    PendingRequestId = RequestId;
    PendingActionValue = ActionValue;
    InventoryPageState = EInventoryPageState::Pending;
    if (InventoryModalLayer) {
        InventoryModalLayer->SetVisibility(ESlateVisibility::Collapsed);
    }
    ClearModalOptions();
    SetFeedback(NSLOCTEXT("MythicInventory", "RequestPending", "Updating inventory…"));
    RefreshInventoryActionBar();

    // Standalone and listen-server RPCs may complete synchronously inside SubmitInventory*. The controller buffers
    // owning-client receipts before broadcasting so the page can reconcile a completion that arrived before the
    // Submit call returned its correlation ID.
    if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer())) {
        FMythicInventoryActionReceipt BufferedReceipt;
        if (PC->ConsumeReceivedInventoryActionReceipt(RequestId, BufferedReceipt)) {
            HandleInventoryActionReceipt(BufferedReceipt);
        }
    }
}

void UMythicCharacterPageWidget::HandleInventorySlotUpdated(int32 SlotIndex) {
    ScheduleRestoreSelection();
}

void UMythicCharacterPageWidget::HandleInventoryActionReceipt(
    const FMythicInventoryActionReceipt &Receipt) {
    if (Receipt.RequestId != PendingRequestId) {
        return;
    }
    if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer())) {
        FMythicInventoryActionReceipt ConsumedReceipt;
        PC->ConsumeReceivedInventoryActionReceipt(Receipt.RequestId, ConsumedReceipt);
    }
    PendingRequestId = 0;
    PendingActionValue = INDEX_NONE;
    InventoryPageState = EInventoryPageState::Browsing;
    ActionSourceGuid.Invalidate();
    ActionSourceSlotIndex = INDEX_NONE;

    if (Receipt.WasSuccessful()) {
        SetFeedback(NSLOCTEXT("MythicInventory", "RequestSucceeded", "Inventory updated."));
        ScheduleRestoreSelection();
    }
    else {
        FText Failure = NSLOCTEXT("MythicInventory", "RequestRejected", "That action could not be completed.");
        switch (Receipt.Result) {
        case EMythicInventoryActionResult::StaleSource:
        case EMythicInventoryActionResult::StaleTarget:
            Failure = NSLOCTEXT("MythicInventory", "RequestStale", "The inventory changed before confirmation. Try again.");
            break;
        case EMythicInventoryActionResult::SourceProtected:
        case EMythicInventoryActionResult::TargetProtected:
        case EMythicInventoryActionResult::UnauthorizedInventory:
            Failure = NSLOCTEXT("MythicInventory", "RequestProtected", "That item or slot cannot be changed.");
            break;
        case EMythicInventoryActionResult::InventoryFull:
            Failure = NSLOCTEXT("MythicInventory", "RequestFull", "There is no compatible free slot.");
            break;
        case EMythicInventoryActionResult::IncompatibleTarget:
            Failure = NSLOCTEXT("MythicInventory", "RequestIncompatible", "That item cannot go in the selected slot.");
            break;
        case EMythicInventoryActionResult::InvalidQuantity:
            Failure = NSLOCTEXT("MythicInventory", "RequestQuantityChanged", "The stack quantity changed. Choose a new amount.");
            break;
        case EMythicInventoryActionResult::NotUsable:
            Failure = NSLOCTEXT("MythicInventory", "RequestNotUsable", "That item cannot be used right now.");
            break;
        default:
            break;
        }
        SetFeedback(Failure, true);
        ScheduleRestoreSelection();
    }
    RefreshInventoryActionBar();
}

void UMythicCharacterPageWidget::BuildDetailsCard() {
    if (DetailsCard || !DetailsHost || !ItemDetailsClass) {
        return;
    }

    if (!GetOwningLocalPlayer()) {
        return;
    }

    DetailsCard = CreateWidget<UUserWidget>(GetOwningPlayer(), ItemDetailsClass);
    if (!DetailsCard) {
        return;
    }

    DetailsHost->AddChild(DetailsCard);
    DetailsCard->SetVisibility(ESlateVisibility::Collapsed);

    // The socket row takes an item instance, which no view model hands it. Found by class rather than by
    // name so renaming the widget on the card cannot quietly stop the row updating.
    if (DetailsCard->WidgetTree) {
        TArray<UWidget *> Children;
        DetailsCard->WidgetTree->GetAllWidgets(Children);
        for (UWidget *Child : Children) {
            if (UMythicSocketRowWidget *Row = Cast<UMythicSocketRowWidget>(Child)) {
                SocketRow = Row;
                break;
            }
        }
    }
}

void UMythicCharacterPageWidget::ShowDetailsFor(UObject *SlotVM) {
    const bool bHasItem = SlotVM != nullptr && DetailsCard != nullptr;

    if (DetailsCard) {
        if (bHasItem) {
            if (UMVVMView *View = UMVVMSubsystem::GetViewFromUserWidget(DetailsCard)) {
                View->SetViewModelByClass(TScriptInterface<INotifyFieldValueChanged>(SlotVM));
            }
        }
        DetailsCard->SetVisibility(bHasItem ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
    }

    if (SocketRow) {
        const UItemSlotVM *SelectedSlot = bHasItem ? Cast<UItemSlotVM>(SlotVM) : nullptr;
        SocketRow->SetItem(SelectedSlot ? SelectedSlot->TryGetItemInstance() : nullptr);
    }

    if (DetailsPlaceholder) {
        DetailsPlaceholder->SetVisibility(bHasItem ? ESlateVisibility::Collapsed
                                                   : ESlateVisibility::SelfHitTestInvisible);
    }
}

UMythicHUDLayout *UMythicCharacterPageWidget::FindHUDLayout() const {
    if (UMythicHUDLayout *Cached = Lender.Get()) {
        return Cached;
    }

    const APlayerController *PC = GetOwningPlayer();
    if (!PC) {
        return nullptr;
    }

    for (TObjectIterator<UMythicHUDLayout> It; It; ++It) {
        UMythicHUDLayout *Layout = *It;
        if (!IsValid(Layout) || Layout->HasAnyFlags(RF_ClassDefaultObject)) {
            continue;
        }
        if (Layout->GetOwningPlayer() == PC) {
            return Layout;
        }
    }
    return nullptr;
}

void UMythicCharacterPageWidget::WrapBorrowedBag(UWidget *Inventory) {
    /**
     * Make the bag wrap onto new rows; leave the equipment strips alone.
     *
     * One asset serves both jobs. The five equipment strips are single rows and want the shipped horizontal
     * orientation; the bag has to reflow as the window changes, which is the vertical one - UE names these by
     * the SCROLL axis, so "vertical" is the mode where items flow left to right and wrap onto a new row.
     *
     * UListView exposes Orientation as a property with a getter but no setter, so it is set through
     * reflection, on the borrowed instance, before the widget is rebuilt in its new parent. The asset keeps
     * shipping horizontal; the named flank strips are stood up below and restored on return.
     */
    UUserWidget *Root = Cast<UUserWidget>(Inventory);
    if (!Root) {
        return;
    }

    UUserWidget *BagHost = Cast<UUserWidget>(Root->GetWidgetFromName(BagWidgetName));
    if (!BagHost || !BagHost->WidgetTree) {
        UE_LOG(Myth, Warning, TEXT("CharacterPage: no bag widget named '%s'; the grid keeps the HUD's layout."),
               *BagWidgetName.ToString());
        return;
    }

    UTileView *Bag = nullptr;
    BagHost->WidgetTree->ForEachWidget([&Bag](UWidget *Widget) {
        if (UTileView *Tile = Cast<UTileView>(Widget)) {
            Bag = Tile;
        }
    });
    if (!Bag) {
        return;
    }

    FByteProperty *Prop = FindFProperty<FByteProperty>(UListView::StaticClass(), TEXT("Orientation"));
    if (!Prop) {
        UE_LOG(Myth, Warning, TEXT("CharacterPage: UListView has no Orientation property; the bag will not wrap."));
        return;
    }
    Prop->SetPropertyValue_InContainer(Bag, Orient_Vertical);

    /**
     * A wrapping tile view asks for the width of ONE cell, because it can always wrap. So every Automatic
     * ancestor between it and the page squeezes it back to a single column. It needs exactly one ancestor
     * that fills, and that ancestor is whichever one sits in a horizontal box.
     */
    for (UWidget *Node = Bag; Node; Node = Node->GetParent()) {
        if (UHorizontalBoxSlot *RowSlot = Cast<UHorizontalBoxSlot>(Node->Slot)) {
            RowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            break;
        }
    }

    /**
     * The crossing edges of the focus graph.
     *
     * Analog navigation picks by geometry, and across the spine's gutter it picks wrong - focus falls off
     * the bag's left edge instead of landing on the equipment lists. One explicit rule each way makes the
     * crossing deterministic: left from the bag reaches the gear, right from any equipment strip reaches
     * the bag. Everything inside a list still navigates itself.
     */
    TArray<UListViewBase *> Lists;
    CollectSlotLists(Root, Lists);
    TArray<UListView *> Strips;
    for (UListViewBase *ListBase : Lists) {
        UListView *List = Cast<UListView>(ListBase);
        if (!List || List == Bag) {
            continue;
        }
        Strips.Add(List);

        /**
         * The Armor and Accessories strips stand up as columns on this page only - the paper doll hangs
         * them down the figure's flanks. Same per-instance reflection write as the bag above; the shared
         * asset still ships horizontal, and ReturnInventory lays these back down for the HUD.
         */
        if (ChainHasName(List, VerticalStripNames)) {
            Prop->SetPropertyValue_InContainer(List, Orient_Vertical);
            ReorientedStrips.AddUnique(List);
            // The slots widget's SetInventoryHeight drives its own SizeBox from SlotsPerRow on every
            // entry generation, so the box cannot be pinned from outside - the column count is the one
            // input it respects.
            if (UUserWidget *SlotsWidget = List->GetTypedOuter<UUserWidget>()) {
                if (FIntProperty *PerRow = FindFProperty<FIntProperty>(SlotsWidget->GetClass(), TEXT("SlotsPerRow"))) {
                    PerRow->SetPropertyValue_InContainer(SlotsWidget, 1);
                }
                if (UFunction *Recompute = SlotsWidget->FindFunction(FName(TEXT("SetInventoryHeight")))) {
                    TArray<uint8> Parms;
                    Parms.AddZeroed(Recompute->ParmsSize);
                    SlotsWidget->ProcessEvent(Recompute, Parms.GetData());
                }
            }
            // The rail SizeBoxes author the single-column width (96-110); the tile only columns to it if
            // every horizontal-box ancestor on the way up fills.
            for (UWidget *Node = List; Node; Node = Node->GetParent() ? Node->GetParent()
                                                                      : Cast<UWidget>(Node->GetTypedOuter<UUserWidget>())) {
                if (Node->GetFName().ToString().EndsWith(TEXT("Rail"))) {
                    break;
                }
                if (UHorizontalBoxSlot *RowSlot = Cast<UHorizontalBoxSlot>(Node->Slot)) {
                    RowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
                }
            }
        }
        if (!WeaponStrip.IsValid() && !WeaponStripName.IsNone() &&
            ChainHasName(List, TArray<FName>{WeaponStripName})) {
            WeaponStrip = List;
        }
        List->SetNavigationRuleExplicit(EUINavigation::Right, Bag);
    }
    if (!WeaponStrip.IsValid() && Strips.Num() > 0) {
        WeaponStrip = Strips[0];
    }
    for (int32 i = 0; i + 1 < Strips.Num(); ++i) {
        Strips[i]->SetNavigationRuleExplicit(EUINavigation::Down, Strips[i + 1]);
        Strips[i + 1]->SetNavigationRuleExplicit(EUINavigation::Up, Strips[i]);
    }
    if (Strips.Num() > 0) {
        Bag->SetNavigationRuleExplicit(EUINavigation::Left, Strips[0]);
    }
    if (StatSheetHost) {
        Bag->SetNavigationRuleExplicit(EUINavigation::Right, StatSheetHost);
    }
}

bool UMythicCharacterPageWidget::ChainHasName(UWidget *Leaf, const TArray<FName> &Names) {
    if (Names.Num() == 0) {
        return false;
    }
    int32 Guard = 64;
    for (UWidget *Node = Leaf; Node && Guard-- > 0;) {
        if (Names.Contains(Node->GetFName())) {
            return true;
        }
        UWidget *Next = Node->GetParent();
        if (!Next) {
            // Crossing a widget-tree boundary: the owning user widget is itself a widget in an outer tree.
            UUserWidget *Owner = Node->GetTypedOuter<UUserWidget>();
            Next = (Owner && Owner != Node) ? static_cast<UWidget *>(Owner) : nullptr;
        }
        Node = Next;
    }
    return false;
}

void UMythicCharacterPageWidget::BorrowInventory() {
    if (!InventoryHost || BorrowedInventory.IsValid()) {
        return;
    }

    UMythicHUDLayout *Layout = FindHUDLayout();
    if (!Layout) {
        return;
    }

    UWidget *Inventory = Layout->BorrowInventoryWidget();
    if (!Inventory) {
        return;
    }

    Lender = Layout;
    BorrowedInventory = Inventory;

    UCommonActivatableWidget *Activatable = Cast<UCommonActivatableWidget>(Inventory);

    if (Activatable) {
        Activatable->DeactivateWidget();
    }

    // The title belongs to the satchel column, directly over its tabs and grid; falling back to the card
    // host only when the borrowed asset has no header seat.
    if (BagHeader) {
        UPanelWidget *HeaderSeat = nullptr;
        if (UUserWidget *InventoryWidget = Cast<UUserWidget>(Inventory)) {
            HeaderSeat = Cast<UPanelWidget>(InventoryWidget->GetWidgetFromName(TEXT("BagHeaderHost")));
        }
        if (!HeaderSeat) {
            HeaderSeat = InventoryHost;
        }
        if (BagHeader->GetParent() != HeaderSeat) {
            BagHeader->RemoveFromParent();
            HeaderSeat->AddChild(BagHeader);
        }
    }

    // Content-hugging: the card ends where the grid ends instead of stretching a void to the page floor.
    if (UPanelSlot *Added = InventoryHost->AddChild(Inventory)) {
        if (UVerticalBoxSlot *V = Cast<UVerticalBoxSlot>(Added)) {
            V->SetHorizontalAlignment(HAlign_Fill);
            V->SetVerticalAlignment(VAlign_Top);
            V->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        }
    }

    Inventory->SetVisibility(ESlateVisibility::Visible);
    WrapBorrowedBag(Inventory);
    BindBagViewModel();

    // Opening onto an empty category reads as a broken bag; land on the first tab that holds anything.
    if (UInventoryVM *VM = BoundInventoryVM.Get()) {
        if (UInventorySelectionVM *Selection = BoundSelectionVM.Get()) {
            auto TabOccupied = [](const UInventoryTabVM *Tab) {
                if (!Tab) {
                    return false;
                }
                for (const UItemSlotVM *SlotVM : Tab->GetSlots()) {
                    if (SlotVM && (SlotVM->GetQuantity() > 0 || SlotVM->GetIcon() != nullptr)) {
                        return true;
                    }
                }
                return false;
            };
            if (!TabOccupied(Selection->GetSelectedTabVM())) {
                for (UInventoryTabVM *Tab : VM->InventoryTabs) {
                    if (TabOccupied(Tab)) {
                        Selection->SetSelectedTabVM(Tab);
                        break;
                    }
                }
            }
            // The tab rail is a list with its own selection; left unsynced it highlights a different
            // category than the grid shows.
            if (UUserWidget *RootWidget = Cast<UUserWidget>(Inventory)) {
                if (UListView *TabsList = Cast<UListView>(RootWidget->GetWidgetFromName(TEXT("Tabs")))) {
                    if (UInventoryTabVM *ActiveTab = Selection->GetSelectedTabVM()) {
                        TabsList->SetSelectedItem(ActiveTab);
                    }
                }
            }
        }
    }
    RefreshBagHeader();

    if (Activatable && GetOwningLocalPlayer()) {
        Activatable->ActivateWidget();
    }
}

void UMythicCharacterPageWidget::ReturnInventory() {
    UWidget *Inventory = BorrowedInventory.Get();
    BorrowedInventory.Reset();
    UnbindBagViewModel();
    WeaponStrip.Reset();
    if (!Inventory) {
        ReorientedStrips.Reset();
        return;
    }

    // Lay the columned strips back down before the HUD rebuilds them.
    if (FByteProperty *Prop = FindFProperty<FByteProperty>(UListView::StaticClass(), TEXT("Orientation"))) {
        for (const TWeakObjectPtr<UListView> &Weak : ReorientedStrips) {
            if (UListView *List = Weak.Get()) {
                Prop->SetPropertyValue_InContainer(List, Orient_Horizontal);
                // Hand SlotsPerRow back to the template's value and let the widget recompute its own box.
                if (UUserWidget *SlotsWidget = List->GetTypedOuter<UUserWidget>()) {
                    if (FIntProperty *PerRow = FindFProperty<FIntProperty>(SlotsWidget->GetClass(), TEXT("SlotsPerRow"))) {
                        const UObject *Template = SlotsWidget->GetArchetype();
                        const int32 Shipped = Template ? PerRow->GetPropertyValue_InContainer(Template)
                                                       : PerRow->GetPropertyValue_InContainer(SlotsWidget);
                        PerRow->SetPropertyValue_InContainer(SlotsWidget, FMath::Max(1, Shipped));
                    }
                    if (UFunction *Recompute = SlotsWidget->FindFunction(FName(TEXT("SetInventoryHeight")))) {
                        TArray<uint8> Parms;
                        Parms.AddZeroed(Recompute->ParmsSize);
                        SlotsWidget->ProcessEvent(Recompute, Parms.GetData());
                    }
                }
                for (UWidget *Node = List; Node; Node = Node->GetParent() ? Node->GetParent()
                                                                          : Cast<UWidget>(Node->GetTypedOuter<UUserWidget>())) {
                    if (Node->GetFName().ToString().EndsWith(TEXT("Rail"))) {
                        break;
                    }
                    if (UHorizontalBoxSlot *RowSlot = Cast<UHorizontalBoxSlot>(Node->Slot)) {
                        RowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
                    }
                }
            }
        }
    }
    ReorientedStrips.Reset();

    if (UCommonActivatableWidget *Activatable = Cast<UCommonActivatableWidget>(Inventory)) {
        Activatable->DeactivateWidget();
    }
    // The header rides in the borrowed tree while the page is open; it must not ship back to the HUD.
    if (BagHeader && BagHeader->GetParent() && BagHeader->GetParent() != InventoryHost) {
        BagHeader->RemoveFromParent();
    }
    if (InventoryHost) {
        InventoryHost->RemoveChild(Inventory);
        if (BagEmptyState && BagEmptyState->GetParent() == InventoryHost) {
            InventoryHost->RemoveChild(BagEmptyState);
        }
    }
    if (UMythicHUDLayout *Layout = Lender.Get()) {
        Layout->ReturnInventoryWidget(Inventory);
    }
}


void UMythicCharacterPageWidget::BuildBagChrome() {
    if (!InventoryHost || !WidgetTree) {
        return;
    }

    if (!BagHeader && BagHeaderClass && GetOwningPlayer()) {
        BagHeader = CreateWidget<UMythicSectionHeader>(GetOwningPlayer(), BagHeaderClass);
        if (BagHeader) {
            BagHeader->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
    }

    if (!BagEmptyState) {
        const UMythicUIStyleSettings &Style = FMythicUIStyle::Get();

        BagEmptyState = WidgetTree->ConstructWidget<UVerticalBox>();

        BagEmptyGlyph = WidgetTree->ConstructWidget<UImage>();
        if (const UMythicUIKit *Kit = UMythicUIKit::Get()) {
            BagEmptyGlyph->SetBrush(Kit->MakeBrush(BagEmptyGlyphId, EMythicUIState::Normal, FVector2D(48.0, 48.0)));
        }
        BagEmptyGlyph->SetColorAndOpacity(
            FLinearColor(Style.InkSubtle.R, Style.InkSubtle.G, Style.InkSubtle.B, 0.45f));
        if (UVerticalBoxSlot *GlyphSlot = Cast<UVerticalBoxSlot>(BagEmptyState->AddChild(BagEmptyGlyph))) {
            GlyphSlot->SetHorizontalAlignment(HAlign_Center);
            GlyphSlot->SetPadding(FMargin(0.0f, Style.SpaceL, 0.0f, Style.SpaceS));
        }

        BagEmptyLine = FMythicUIStyle::MakeText(this, EMythicTextRole::Body);
        if (BagEmptyLine) {
            BagEmptyLine->SetText(NSLOCTEXT("Mythic", "BagEmptyLine",
                                            "Nothing here yet - the world is generous to the thorough"));
            if (UVerticalBoxSlot *LineSlot = Cast<UVerticalBoxSlot>(BagEmptyState->AddChild(BagEmptyLine))) {
                LineSlot->SetHorizontalAlignment(HAlign_Center);
                LineSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, Style.SpaceL));
            }
        }

        BagEmptyState->SetVisibility(ESlateVisibility::Collapsed);
    }
}

UInventoryVM *UMythicCharacterPageWidget::ResolveInventoryVM() const {
    UUserWidget *Inventory = Cast<UUserWidget>(BorrowedInventory.Get());
    if (!Inventory) {
        return nullptr;
    }
    TArray<UListViewBase *> Lists;
    CollectSlotLists(Inventory, Lists);
    for (UListViewBase *ListBase : Lists) {
        UListView *List = Cast<UListView>(ListBase);
        if (!List) {
            continue;
        }
        for (UObject *Item : List->GetListItems()) {
            if (const UItemSlotVM *SlotVM = Cast<UItemSlotVM>(Item)) {
                if (UInventoryVM *VM = SlotVM->GetParentInventoryVM()) {
                    return VM;
                }
            }
        }
    }
    return nullptr;
}

void UMythicCharacterPageWidget::BindBagViewModel() {
    if (BoundSelectionVM.IsValid()) {
        return;
    }
    UInventoryVM *VM = ResolveInventoryVM();
    if (!VM || !VM->SelectionVM) {
        return;
    }
    BoundInventoryVM = VM;
    BoundSelectionVM = VM->SelectionVM;

    const INotifyFieldValueChanged::FFieldValueChangedDelegate Delegate =
        INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(
            this, &UMythicCharacterPageWidget::HandleBagFieldChanged);
    using FDesc = UInventorySelectionVM::FFieldNotificationClassDescriptor;
    BagTabHandle = VM->SelectionVM->AddFieldValueChangedDelegate(FDesc::SelectedTabVM, Delegate);
}

void UMythicCharacterPageWidget::UnbindBagViewModel() {
    if (UInventorySelectionVM *Selection = BoundSelectionVM.Get()) {
        if (BagTabHandle.IsValid()) {
            using FDesc = UInventorySelectionVM::FFieldNotificationClassDescriptor;
            Selection->RemoveFieldValueChangedDelegate(FDesc::SelectedTabVM, BagTabHandle);
        }
    }
    BagTabHandle.Reset();
    BoundSelectionVM = nullptr;
    BoundInventoryVM = nullptr;
}

void UMythicCharacterPageWidget::HandleBagFieldChanged(UObject *Object, UE::FieldNotification::FFieldId FieldId) {
    RefreshBagHeader();
}

void UMythicCharacterPageWidget::RefreshBagHeader() {
    UInventoryVM *VM = BoundInventoryVM.Get();
    UInventoryTabVM *Tab = BoundSelectionVM.IsValid() ? BoundSelectionVM->GetSelectedTabVM() : nullptr;
    if (!Tab && VM) {
        for (UInventoryTabVM *Candidate : VM->InventoryTabs) {
            if (Candidate) {
                Tab = Candidate;
                break;
            }
        }
    }
    if (!Tab) {
        if (BagEmptyState) {
            BagEmptyState->SetVisibility(ESlateVisibility::Collapsed);
        }
        return;
    }

    int32 Occupied = 0;
    const TArray<TObjectPtr<UItemSlotVM>> Slots = Tab->GetSlots();
    for (const UItemSlotVM *SlotVM : Slots) {
        if (SlotVM && (SlotVM->GetQuantity() > 0 || SlotVM->GetIcon() != nullptr)) {
            ++Occupied;
        }
    }

    if (BagHeader) {
        BagHeader->SetHeader(Tab->GetTabName(),
                             FText::Format(NSLOCTEXT("Mythic", "BagCapacity", "{0} / {1}"),
                                           FText::AsNumber(Occupied), FText::AsNumber(Slots.Num())),
                             Tab->GetTabIcon());
    }
    // The all-capacity grid is its own empty state; the prose line never earns space on this page.
    if (BagEmptyState) {
        BagEmptyState->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UMythicCharacterPageWidget::CycleBagCategoryForward() {
    CycleBagCategory(1);
}

void UMythicCharacterPageWidget::CycleBagCategoryBack() {
    CycleBagCategory(-1);
}

void UMythicCharacterPageWidget::CycleBagCategory(int32 Direction) {
    UInventoryVM *VM = BoundInventoryVM.Get();
    UInventorySelectionVM *Selection = BoundSelectionVM.Get();
    if (!VM || !Selection) {
        return;
    }
    TArray<UInventoryTabVM *> Tabs;
    for (UInventoryTabVM *Tab : VM->InventoryTabs) {
        if (Tab) {
            Tabs.Add(Tab);
        }
    }
    if (Tabs.Num() == 0) {
        return;
    }
    const int32 Current = Tabs.IndexOfByKey(Selection->GetSelectedTabVM());
    const int32 Next = Current == INDEX_NONE
                           ? (Direction >= 0 ? 0 : Tabs.Num() - 1)
                           : (Current + Direction + Tabs.Num()) % Tabs.Num();
    // Through the setter so FieldNotify fires and every bound surface re-filters.
    Selection->SetSelectedTabVM(Tabs[Next]);
}

void UMythicCharacterPageWidget::BuildSockets() {
    if (!SocketStrip || Sockets.Num() > 0) {
        return;
    }

    if (SocketHeaderClass && GetOwningPlayer()) {
        if (UMythicSectionHeader *Header =
                CreateWidget<UMythicSectionHeader>(GetOwningPlayer(), SocketHeaderClass)) {
            Header->SetHeader(NSLOCTEXT("Mythic", "SocketStripHeading", "Runes"), FText::GetEmpty(), nullptr);
            Header->SetVisibility(ESlateVisibility::HitTestInvisible);
            SocketStrip->AddChild(Header);
        }
    }
    // Through the catalogue, not a hardcoded path: a moved or renamed material becomes a missing element
    // plus a log line here, instead of silently loading nothing.
    const UMythicUIKit *Kit = UMythicUIKit::Get();

    for (int32 i = 0; i < FMath::Clamp(SocketCount, 1, 8); ++i) {
        FMythicRuneSocket Socket;
        Socket.SlotIndex = i;

        UOverlay *Stack = WidgetTree->ConstructWidget<UOverlay>();

        Socket.Well = WidgetTree->ConstructWidget<UImage>();
        if (Kit) {
            Socket.Well->SetBrush(
                Kit->MakeBrush(TEXT("SlotTex.Round"), EMythicUIState::Normal, FVector2D(64.0, 64.0)));
        }
        Socket.Well->SetVisibility(ESlateVisibility::HitTestInvisible);
        Stack->AddChildToOverlay(Socket.Well);

        Socket.Mark = WidgetTree->ConstructWidget<UImage>();
        Socket.Mark->SetVisibility(ESlateVisibility::Collapsed);
        if (UOverlaySlot *MarkSlot = Cast<UOverlaySlot>(Stack->AddChildToOverlay(Socket.Mark))) {
            MarkSlot->SetHorizontalAlignment(HAlign_Center);
            MarkSlot->SetVerticalAlignment(VAlign_Center);
            MarkSlot->SetPadding(FMargin(14.0f));
        }

        UButton *Hit = WidgetTree->ConstructWidget<UButton>();
        FButtonStyle Clear;
        Clear.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
        Clear.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
        Clear.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
        Clear.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
        Hit->SetStyle(Clear);
        if (UOverlaySlot *HitSlot = Cast<UOverlaySlot>(Stack->AddChildToOverlay(Hit))) {
            HitSlot->SetHorizontalAlignment(HAlign_Fill);
            HitSlot->SetVerticalAlignment(VAlign_Fill);
        }

        Socket.Proxy = NewObject<UMythicRuneSocketClickProxy>(this);
        Socket.Proxy->Page = this;
        Socket.Proxy->SlotIndex = i;
        RuneSocketClickProxies.Add(Socket.Proxy);
        Hit->OnClicked.AddDynamic(Socket.Proxy, &UMythicRuneSocketClickProxy::HandleClicked);

        Socket.Button = Stack;
        SocketStrip->AddChild(Stack);
        Sockets.Add(Socket);
    }
    RefreshSockets();
}

void UMythicCharacterPageWidget::RefreshSockets() {
    const UMythicRuneComponent *Runes = nullptr;
    if (const APlayerController *PC = GetOwningPlayer()) {
        if (const AMythicPlayerState *PS = PC->GetPlayerState<AMythicPlayerState>()) {
            Runes = PS->GetRuneComponent();
        }
    }
    const UMythicUIKit *Kit = UMythicUIKit::Get();

    for (FMythicRuneSocket &Socket : Sockets) {
        const bool bUnlocked = Runes && Runes->IsSlotUnlocked(Socket.SlotIndex);
        const UMythicRuneDefinition *Worn = Runes ? Runes->GetRuneInSlot(Socket.SlotIndex) : nullptr;

        // Three states a player can tell apart at a glance: a socket they have not earned, an earned one
        // standing empty, and one that is filled. Four identical rings say none of that.
        if (Socket.Well && Kit) {
            const EMythicUIState WellState = !bUnlocked ? EMythicUIState::Disabled
                                             : Worn     ? EMythicUIState::Selected
                                                        : EMythicUIState::Normal;
            Socket.Well->SetBrush(Kit->MakeBrush(TEXT("SlotTex.Round"), WellState, FVector2D(64.0, 64.0)));
        }

        if (Socket.Mark) {
            UTexture2D *Icon = Worn ? Worn->Icon.LoadSynchronous() : nullptr;
            if (Icon) {
                Socket.Mark->SetBrushFromTexture(Icon, true);
                Socket.Mark->SetColorAndOpacity(RuneCategoryColour(Worn));
                Socket.Mark->SetVisibility(ESlateVisibility::HitTestInvisible);
            }
            else {
                Socket.Mark->SetVisibility(ESlateVisibility::Collapsed);
            }
        }

        // A socket that cannot be filled yet must not answer a click, or the player learns the control lies.
        if (Socket.Button) {
            Socket.Button->SetVisibility(bUnlocked ? ESlateVisibility::Visible
                                                   : ESlateVisibility::HitTestInvisible);
            Socket.Button->SetRenderOpacity(bUnlocked ? 1.0f : 0.4f);
        }
    }
}

FLinearColor UMythicCharacterPageWidget::RuneCategoryColour(const UMythicRuneDefinition *Rune) const {
    if (Rune) {
        for (const FMythicRuneCategoryColour &Entry : RuneCategoryColours) {
            if (Entry.Category.IsValid() && Rune->CategoryTags.HasTag(Entry.Category)) {
                return Entry.Colour;
            }
        }
    }
    return FLinearColor::White;
}

void UMythicRuneSocketClickProxy::HandleClicked() {
    if (UMythicCharacterPageWidget *P = Page.Get()) {
        P->OpenSocketPicker(SlotIndex);
    }
}

void UMythicCharacterPageWidget::OpenSocketPicker(int32 SlotIndex) {
    if (!RunePickerClass) {
        UE_LOG(Myth, Warning, TEXT("CharacterPage: socket %d has no rune picker class assigned."), SlotIndex);
        return;
    }
    const UMythicRuneComponent *Runes = nullptr;
    if (const APlayerController *PC = GetOwningPlayer()) {
        if (const AMythicPlayerState *PS = PC->GetPlayerState<AMythicPlayerState>()) {
            Runes = PS->GetRuneComponent();
        }
    }
    if (!Runes || !Runes->IsSlotUnlocked(SlotIndex)) {
        return;
    }

    // One picker for the page's lifetime, re-pointed at the chosen socket. Creating a widget per click is a
    // frame spike, and the pool of one is all this needs.
    if (!RunePicker) {
        RunePicker = CreateWidget<UMythicRunePickerWidget>(GetOwningPlayer(), RunePickerClass);
        if (!RunePicker) {
            return;
        }
    }
    RunePicker->OpenForSlot(SlotIndex, this);
}

void UMythicCharacterPageWidget::NotifyRunesChanged() {
    RefreshSockets();
}
