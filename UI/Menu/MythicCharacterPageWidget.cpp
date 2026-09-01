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
#include "Itemization/Inventory/MythicInventoryActionTypes.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/InventoryProfile.h"
#include "Itemization/Inventory/InventorySlotDefinition.h"
#include "Itemization/MythicTags_Inventory.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "Input/Events.h"
#include "Engine/LocalPlayer.h"
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
#include "UI/Inventory/MythicInventoryInteractionCoordinator.h"
#include "UI/Inventory/MythicInventoryInteractionPolicy.h"
#include "UI/Inventory/MythicItemDetailsWidget.h"
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
    if (UMythicInventoryInteractionCoordinator *Coordinator = GetInventoryInteractionCoordinator()) {
        Coordinator->SetCharacterInventoryPageActive(true);
    }
    BindInventoryInputs();
    BindProgression();
    RefreshHeader();
    BuildSockets();
    RefreshSockets();
    RestoreSelectionByGuid();
    RefreshInventoryActionBar();

    // Focus one tick late: at activation the borrowed strips are not yet in a visible Slate path, and
    // SetFocus on a widget without one fails silently.
    if (const UWorld *World = GetWorld()) {
        World->GetTimerManager().SetTimerForNextTick(
            FTimerDelegate::CreateUObject(this, &UMythicCharacterPageWidget::FocusInitialSlot));
    }
}

UWidget *UMythicCharacterPageWidget::NativeGetDesiredFocusTarget() const {
    if (const UItemSlotVM *Selected = GetSelectedSlot()) {
        for (const TWeakObjectPtr<UListViewBase> &WeakList : BoundSlotLists) {
            if (UListView *List = Cast<UListView>(WeakList.Get())) {
                if (UUserWidget *Entry = List->GetEntryWidgetFromItem(const_cast<UItemSlotVM *>(Selected))) {
                    return Entry;
                }
            }
        }
    }
    if (UListView *Strip = WeaponStrip.Get()) {
        return Strip;
    }
    return Super::NativeGetDesiredFocusTarget();
}

void UMythicCharacterPageWidget::FocusInitialSlot() {
    if (!IsActivated() || !GetOwningLocalPlayer()) {
        return;
    }
    // The borrowed inventory's nested MVVM SetListItems bindings may publish one frame after activation.
    // Rebind against the now-populated lists and derive the authoritative VM from the first physical slot.
    BindSlotSelection();
    BindBagViewModel();
    if (BoundInventoryVM.IsValid()) {
        RestoreSelectionByGuid();
    }
    else {
        SelectFirstOccupiedSlot();
    }
    BindInventoryEvents();
    if (UItemSlotVM *Selected = GetSelectedSlot()) {
        for (const TWeakObjectPtr<UListViewBase> &WeakList : BoundSlotLists) {
            if (UListView *List = Cast<UListView>(WeakList.Get())) {
                if (UUserWidget *Entry = List->GetEntryWidgetFromItem(Selected)) {
                    Entry->SetFocus();
                    return;
                }
            }
        }
    }
    UListView *Strip = WeaponStrip.Get();
    if (Strip && Strip->GetVisibility() != ESlateVisibility::Collapsed) {
        Strip->SetFocus();
    }
}

bool UMythicCharacterPageWidget::TryHandleNestedBackAction() {
    if (InventoryPageState != EInventoryPageState::Browsing) {
        CloseInventoryModal(true);
        return true;
    }
    return false;
}

FReply UMythicCharacterPageWidget::NativeOnAnalogValueChanged(
    const FGeometry &InGeometry,
    const FAnalogInputEvent &InAnalogEvent) {
    if (InAnalogEvent.GetKey() == EKeys::Gamepad_RightY
        && FMath::Abs(InAnalogEvent.GetAnalogValue()) >= 0.18f
        && DetailsCard
        && DetailsCard->ScrollDetailsBy(-InAnalogEvent.GetAnalogValue() * 56.0f)) {
        return FReply::Handled();
    }
    return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
}

FReply UMythicCharacterPageWidget::NativeOnPreviewMouseButtonDown(
    const FGeometry &InGeometry,
    const FPointerEvent &InMouseEvent) {
    // WBP_InventoryItemSlot legitimately captures the left press to begin drag detection. Pin the hovered item
    // during the tunnel phase so that drag handling cannot prevent an ordinary click from opening its details.
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
        && InventoryPageState == EInventoryPageState::Browsing
        && HoveredItemGuid.IsValid()) {
        UItemSlotVM *HoveredSlot = DisplayedDetailsSlot.Get();
        UMythicItemInstance *HoveredItem = HoveredSlot ? HoveredSlot->TryGetItemInstance() : nullptr;
        if (HoveredItem && HoveredItem->GetItemInstanceGuid() == HoveredItemGuid) {
            SetSelectedSlot(HoveredSlot, FindListContaining(HoveredSlot));
        }
    }
    return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
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
    TargetActionButton = CreateInventoryActionButton(Style.SecondaryActionButtonStyle.LoadSynchronous());
    SortActionButton = CreateInventoryActionButton(Style.QuietActionButtonStyle.LoadSynchronous());

    if (PrimaryActionButton) {
        PrimaryActionButton->OnClicked().AddUObject(this, &UMythicCharacterPageWidget::HandlePrimaryInventoryAction);
    }
    if (ActionsActionButton) {
        ActionsActionButton->OnClicked().AddUObject(this, &UMythicCharacterPageWidget::HandleInventoryActionsAction);
    }
    if (TargetActionButton) {
        TargetActionButton->OnClicked().AddUObject(this, &UMythicCharacterPageWidget::HandleCycleInventoryTarget);
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
         GET_FUNCTION_NAME_CHECKED(UMythicCharacterPageWidget, HandleCycleInventoryTarget), CompareBinding);
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
    if (TargetActionButton && CompareBinding.Handle.IsValid()) {
        TargetActionButton->SetRepresentedAction(CompareBinding.Handle);
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
    if (UMythicInventoryInteractionCoordinator *Coordinator = GetInventoryInteractionCoordinator()) {
        Coordinator->OnFeedback.AddUniqueDynamic(
            this, &ThisClass::HandleInventoryInteractionFeedback);
        Coordinator->OnCompleted.AddUniqueDynamic(
            this, &ThisClass::HandleInventoryActionReceipt);
        Coordinator->OnPendingChanged.AddUniqueDynamic(
            this, &ThisClass::HandleInventoryPendingChanged);
    }
}

void UMythicCharacterPageWidget::ReleaseInventoryEvents() {
    if (UMythicInventoryComponent *Inventory = GetInventoryComponent()) {
        Inventory->OnSlotUpdated.RemoveDynamic(this, &UMythicCharacterPageWidget::HandleInventorySlotUpdated);
    }
    if (UMythicInventoryInteractionCoordinator *Coordinator = GetInventoryInteractionCoordinator()) {
        Coordinator->OnFeedback.RemoveDynamic(
            this, &ThisClass::HandleInventoryInteractionFeedback);
        Coordinator->OnCompleted.RemoveDynamic(
            this, &ThisClass::HandleInventoryActionReceipt);
        Coordinator->OnPendingChanged.RemoveDynamic(
            this, &ThisClass::HandleInventoryPendingChanged);
    }
}

void UMythicCharacterPageWidget::HandlePreviousInventoryCategory() {
    if (InventoryPageState == EInventoryPageState::Quantity) {
        ExecuteInventoryUICommand(EMythicInventoryUICommand::QuantityDecreaseLarge, INDEX_NONE);
    }
    else if (InventoryPageState == EInventoryPageState::Browsing) {
        CycleBagCategoryBack();
    }
}

void UMythicCharacterPageWidget::HandleNextInventoryCategory() {
    if (InventoryPageState == EInventoryPageState::Quantity) {
        ExecuteInventoryUICommand(EMythicInventoryUICommand::QuantityIncreaseLarge, INDEX_NONE);
    }
    else if (InventoryPageState == EInventoryPageState::Browsing) {
        CycleBagCategoryForward();
    }
}

void UMythicCharacterPageWidget::NativeOnDeactivated() {
    if (UMythicInventoryInteractionCoordinator *Coordinator = GetInventoryInteractionCoordinator()) {
        Coordinator->SetCharacterInventoryPageActive(false);
    }
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
    if (UMythicInventoryInteractionCoordinator *Coordinator = GetInventoryInteractionCoordinator()) {
        Coordinator->SetCharacterInventoryPageActive(false);
    }
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
    InventoryPageState = EInventoryPageState::Browsing;
    QuantityPurpose = EQuantityPurpose::None;
    QuantityValue = 1;
    QuantityMaximum = 1;
    ActiveTargetSlotIndex = INDEX_NONE;
    bSelectionRestoreScheduled = false;
    HoveredItemGuid.Invalidate();
    DisplayedDetailsSlot.Reset();
    ++DetailsPositionSerial;
    ActionSourceGuid.Invalidate();
    ActionSourceSlotIndex = INDEX_NONE;
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
    if (!DetailsCard) {
        return;
    }

    UUserWidget *Inventory = Cast<UUserWidget>(BorrowedInventory.Get());
    if (!Inventory) {
        return;
    }

    TArray<UListViewBase *> Lists;
    CollectSlotLists(Inventory, Lists);

    for (UListViewBase *List : Lists) {
        if (!List || BoundSlotLists.Contains(List)) {
            continue;
        }
        // Bind before MVVM's delayed SetListItems runs. Category-list events are harmless because every handler
        // is type-gated to UItemSlotVM, and ClearOtherListSelections preserves non-item selections.
        if (ITypedUMGListView<UObject *> *Typed = AsTypedList(List)) {
            Typed->OnItemSelectionChanged().AddUObject(this, &UMythicCharacterPageWidget::HandleSlotSelectionChanged);
            Typed->OnItemClicked().AddUObject(this, &UMythicCharacterPageWidget::HandleSlotClicked);
            Typed->OnItemIsHoveredChanged().AddUObject(this, &UMythicCharacterPageWidget::HandleSlotHoverChanged);
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
            Typed->OnItemClicked().RemoveAll(this);
            Typed->OnItemIsHoveredChanged().RemoveAll(this);
        }
    }
    BoundSlotLists.Reset();
    SelectedList.Reset();
    HoveredItemGuid.Invalidate();
    DisplayedDetailsSlot.Reset();
    ++DetailsPositionSerial;
    ShowDetailsFor(nullptr);
}

void UMythicCharacterPageWidget::HandleSlotSelectionChanged(UObject *Item) {
    UItemSlotVM *SlotVM = Cast<UItemSlotVM>(Item);
    if (!SlotVM || bSynchronizingSelection) {
        return;
    }

    UListViewBase *SourceList = FindListSelecting(Item);
    if (!SourceList) {
        SourceList = FindListContaining(Item);
    }
    // Context, quantity, and sort are modal transactions over the item/group captured when they opened.
    // Ignore focus leakage into the borrowed inventory so mouse and controller navigation cannot silently
    // retarget a destructive command while its original labels and confirmation affordance remain visible.
    if (InventoryPageState == EInventoryPageState::Context
        || InventoryPageState == EInventoryPageState::Quantity
        || InventoryPageState == EInventoryPageState::Sort) {
        return;
    }
    if (InventoryPageState == EInventoryPageState::MoveTarget) {
        FText RejectionReason;
        if (!CanMoveSelectionToSlot(SlotVM->GetAbsoluteIndex(), &RejectionReason)) {
            ActiveTargetSlotIndex = INDEX_NONE;
            SetFeedback(RejectionReason, true);
        }
        else {
            ActiveTargetSlotIndex = SlotVM->GetAbsoluteIndex();
            SetFeedback(FText::Format(
                NSLOCTEXT("MythicInventory", "MoveTargetSelected", "Move to {0} — press Move Here to confirm"),
                GetSlotDisplayName(ActiveTargetSlotIndex)));
        }
        RefreshInventoryActionBar();
        return;
    }

    if (UMythicItemInstance *ItemInstance = SlotVM->TryGetItemInstance()) {
        SetSelectedSlot(SlotVM, SourceList);
    }
}

void UMythicCharacterPageWidget::HandleSlotClicked(UObject *Item) {
    UItemSlotVM *SlotVM = Cast<UItemSlotVM>(Item);
    if (!SlotVM || !SlotVM->TryGetItemInstance() || bSynchronizingSelection) {
        return;
    }

    if (InventoryPageState == EInventoryPageState::MoveTarget) {
        HandleSlotSelectionChanged(Item);
        return;
    }
    if (InventoryPageState != EInventoryPageState::Browsing) {
        return;
    }

    // Clicking pins the exact physical item even when a particular list's selection mode is disabled or the
    // item was already selected. Hover remains a preview only; all inventory actions continue to use this pin.
    HoveredItemGuid.Invalidate();
    SetSelectedSlot(SlotVM, FindListContaining(Item));
}

void UMythicCharacterPageWidget::HandleSlotHoverChanged(UObject *Item, const bool bIsHovered) {
    UItemSlotVM *SlotVM = Cast<UItemSlotVM>(Item);
    UMythicItemInstance *ItemInstance = SlotVM ? SlotVM->TryGetItemInstance() : nullptr;
    if (!ItemInstance || InventoryPageState != EInventoryPageState::Browsing) {
        return;
    }

    const FGuid ItemGuid = ItemInstance->GetItemInstanceGuid();
    if (!ItemGuid.IsValid()) {
        return;
    }
    if (bIsHovered) {
        // Hover is ordinary selection: the same physical GUID drives details, actions, comparison, and focus.
        HoveredItemGuid = ItemGuid;
        SetSelectedSlot(SlotVM, FindListContaining(Item));
        return;
    }

    if (HoveredItemGuid == ItemGuid) {
        HoveredItemGuid.Invalidate();
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

UListViewBase *UMythicCharacterPageWidget::FindListContaining(UObject *Item) const {
    if (!Item) {
        return nullptr;
    }
    for (const TWeakObjectPtr<UListViewBase> &WeakList : BoundSlotLists) {
        if (UListView *List = Cast<UListView>(WeakList.Get());
            List && List->GetListItems().Contains(Item)) {
            return List;
        }
    }
    return nullptr;
}

UUserWidget *UMythicCharacterPageWidget::FindEntryWidgetForItem(UObject *Item) const {
    if (!Item) {
        return nullptr;
    }
    for (const TWeakObjectPtr<UListViewBase> &WeakList : BoundSlotLists) {
        if (UListView *List = Cast<UListView>(WeakList.Get()); List) {
            if (UUserWidget *Entry = List->GetEntryWidgetFromItem(Item)) {
                return Entry;
            }
        }
    }
    return nullptr;
}

void UMythicCharacterPageWidget::ClearOtherListSelections(UListViewBase *Except) {
    TGuardValue<bool> SelectionGuard(bSynchronizingSelection, true);
    for (const TWeakObjectPtr<UListViewBase> &WeakList : BoundSlotLists) {
        UListView *List = Cast<UListView>(WeakList.Get());
        if (List && List != Except && Cast<UItemSlotVM>(List->GetSelectedItem())) {
            List->ClearSelection();
        }
    }
}

void UMythicCharacterPageWidget::SetSelectedSlot(UItemSlotVM *SlotVM, UListViewBase *SourceList) {
    UMythicItemInstance *Item = SlotVM ? SlotVM->TryGetItemInstance() : nullptr;
    if (!Item || !Item->GetItemInstanceGuid().IsValid()) {
        return;
    }
    BindBagViewModel(SlotVM->GetParentInventoryVM());
    BindInventoryEvents();
    ClearOtherListSelections(SourceList);
    if (UListView *List = Cast<UListView>(SourceList);
        List && List->GetSelectedItem() != SlotVM) {
        TGuardValue<bool> SelectionGuard(bSynchronizingSelection, true);
        List->SetSelectedItem(SlotVM);
    }
    SelectedList = SourceList;
    SelectedItemGuid = Item->GetItemInstanceGuid();
    LastSelectedSlotIndex = SlotVM->GetAbsoluteIndex();
    ActionSourceGuid.Invalidate();
    ActionSourceSlotIndex = INDEX_NONE;
    ActiveTargetSlotIndex = INDEX_NONE;
    ResolveActiveEquipmentTarget();
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
    // A modal owns focus until it closes. Its close path schedules the canonical source restoration.
    if (!IsActivated() || InventoryPageState != EInventoryPageState::Browsing) {
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
            if (ITypedUMGListView<UObject *> *Typed = AsTypedList(List)) {
                Typed->RequestNavigateToItem(SlotToSelect);
            }
            SelectedList = List;
            SelectedItemGuid = SlotToSelect->TryGetItemInstance()->GetItemInstanceGuid();
            LastSelectedSlotIndex = SlotToSelect->GetAbsoluteIndex();
            ResolveActiveEquipmentTarget();
            ShowDetailsFor(SlotToSelect);
            RefreshInventoryActionBar();
            if (UWorld *World = GetWorld()) {
                TWeakObjectPtr<UListView> WeakResolvedList = List;
                TWeakObjectPtr<UItemSlotVM> WeakSlot = SlotToSelect;
                World->GetTimerManager().SetTimerForNextTick(
                    FTimerDelegate::CreateWeakLambda(this, [WeakResolvedList, WeakSlot]() {
                        if (UListView *ResolvedList = WeakResolvedList.Get()) {
                            if (UUserWidget *Entry = ResolvedList->GetEntryWidgetFromItem(WeakSlot.Get())) {
                                Entry->SetFocus();
                            }
                        }
                    }));
            }
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

bool UMythicCharacterPageWidget::CanMoveSelectionToSlot(
    const int32 TargetSlotIndex,
    FText *OutReason) const {
    UMythicInventoryComponent *Inventory = GetInventoryComponent();
    FMythicInventorySourceLocator Source;
    FMythicInventoryTargetLocator Target;
    EMythicInventoryActionResult Result = EMythicInventoryActionResult::InvalidRequest;
    if (Inventory && BuildSourceLocator(Source) && BuildTargetLocator(TargetSlotIndex, Target)) {
        Result = Inventory->ValidatePlayerMoveItem(Source, Target);
    }
    if (Result == EMythicInventoryActionResult::Succeeded) {
        return true;
    }
    if (OutReason) {
        *OutReason = UMythicInventoryInteractionCoordinator::DescribeResult(Result);
    }
    return false;
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
UMythicCharacterPageWidget::BuildEquipmentTargets(UItemSlotVM *SourceSlotOverride) const {
    TArray<FEquipmentTarget> Targets;
    UMythicInventoryComponent *Inventory = GetInventoryComponent();
    UItemSlotVM *SourceSlot = SourceSlotOverride ? SourceSlotOverride : GetSelectedSlot();
    UMythicItemInstance *Item = SourceSlot ? SourceSlot->TryGetItemInstance() : nullptr;
    if (!Inventory || !Item || !Item->GetItemDefinition()
        || !Inventory->GetAllSlots().IsValidIndex(SourceSlot->GetAbsoluteIndex())
        || !Item->GetItemDefinition()->ItemType.MatchesTag(ITEMIZATION_TYPE_EQUIPMENT)) {
        return Targets;
    }
    const FGameplayTag ItemFamily = Item->GetItemDefinition()->ItemType;
    const FEquipmentTargetKey *StickyKey = StickyEquipmentTargetKeys.Find(ItemFamily);
    const FMythicInventorySlotEntry &SourceEntry =
        Inventory->GetAllSlots()[SourceSlot->GetAbsoluteIndex()];
    if (!SourceEntry.bCanPlayerTake) {
        return Targets;
    }

    for (int32 Index = 0; Index < Inventory->GetAllSlots().Num(); ++Index) {
        const FMythicInventorySlotEntry &Entry = Inventory->GetAllSlots()[Index];
        if (!Entry.IsGearSlot() || Index == SourceSlot->GetAbsoluteIndex()
            || !Entry.bCanPlayerPut
            || !Inventory->CanSlotAcceptItem(Index, Item, true, Item)) {
            continue;
        }

        // Occupied targets are valid only when the exact incumbent can be returned to the source slot.
        // This mirrors the transactional server swap check, preventing a comparison/equip promise that authority
        // must later reject because a helmet cannot be swapped into a weapon slot (or into protected storage).
        if (Entry.SlottedItemInstance
            && (!Entry.bCanPlayerTake
                || !SourceEntry.bCanPlayerPut
                || !Inventory->CanSlotAcceptItem(
                    SourceSlot->GetAbsoluteIndex(), Entry.SlottedItemInstance, true,
                    Entry.SlottedItemInstance))) {
            continue;
        }
        FEquipmentTarget Target;
        Target.Key = BuildEquipmentTargetKey(Index);
        Target.SlotIndex = Index;
        if (Inventory->InventoryProfile) {
            if (const FInventorySlotGroup *Group =
                    Inventory->InventoryProfile->SlotGroups.Find(Entry.GroupTag)) {
                Target.GroupDisplayOrder = Group->DisplayOrder;
            }
        }
        Target.DisplayName = GetSlotDisplayName(Index);
        Target.bExpectEmpty = Entry.SlottedItemInstance == nullptr;
        if (Entry.SlottedItemInstance) {
            Target.OccupantGuid = Entry.SlottedItemInstance->GetItemInstanceGuid();
            Target.OccupantQuantity = Entry.SlottedItemInstance->GetStacks();
        }
        Targets.Add(MoveTemp(Target));
    }
    Targets.Sort([StickyKey](const FEquipmentTarget &A, const FEquipmentTarget &B) {
        const bool bASticky = StickyKey && A.Key == *StickyKey;
        const bool bBSticky = StickyKey && B.Key == *StickyKey;
        if (bASticky != bBSticky) {
            return bASticky;
        }
        if (A.bExpectEmpty != B.bExpectEmpty) {
            return A.bExpectEmpty;
        }
        if (A.GroupDisplayOrder != B.GroupDisplayOrder) {
            return A.GroupDisplayOrder < B.GroupDisplayOrder;
        }
        const FString AGroup = A.Key.GroupTag.ToString();
        const FString BGroup = B.Key.GroupTag.ToString();
        if (AGroup != BGroup) {
            return AGroup < BGroup;
        }
        if (A.Key.EntryIndex != B.Key.EntryIndex) {
            return A.Key.EntryIndex < B.Key.EntryIndex;
        }
        const FString ADefinition = A.Key.SlotDefinitionId.ToString();
        const FString BDefinition = B.Key.SlotDefinitionId.ToString();
        if (ADefinition != BDefinition) {
            return ADefinition < BDefinition;
        }
        if (A.Key.RepetitionOrdinal != B.Key.RepetitionOrdinal) {
            return A.Key.RepetitionOrdinal < B.Key.RepetitionOrdinal;
        }
        return A.SlotIndex < B.SlotIndex;
    });
    return Targets;
}

UMythicCharacterPageWidget::FEquipmentTargetKey
UMythicCharacterPageWidget::BuildEquipmentTargetKey(const int32 SlotIndex) const {
    FEquipmentTargetKey Key;
    UMythicInventoryComponent *Inventory = GetInventoryComponent();
    if (!Inventory || !Inventory->GetAllSlots().IsValidIndex(SlotIndex)) {
        return Key;
    }

    const FMythicInventorySlotEntry &Entry = Inventory->GetAllSlots()[SlotIndex];
    Key.GroupTag = Entry.GroupTag;
    Key.EntryIndex = Entry.EntryIndex;
    if (Entry.SlotDefinition) {
        Key.SlotDefinitionId = Entry.SlotDefinition->GetPrimaryAssetId();
    }
    for (int32 Index = 0; Index < SlotIndex; ++Index) {
        const FMythicInventorySlotEntry &Candidate = Inventory->GetAllSlots()[Index];
        if (Candidate.GroupTag == Entry.GroupTag
            && Candidate.EntryIndex == Entry.EntryIndex
            && Candidate.SlotDefinition == Entry.SlotDefinition) {
            ++Key.RepetitionOrdinal;
        }
    }
    return Key;
}

void UMythicCharacterPageWidget::ResolveActiveEquipmentTarget() {
    ActiveTargetSlotIndex = INDEX_NONE;
    UItemSlotVM *SlotVM = GetSelectedSlot();
    if (!SlotVM || SlotVM->GetIsEquipped()) {
        return;
    }
    const TArray<FEquipmentTarget> Targets = BuildEquipmentTargets();
    if (Targets.Num() > 0) {
        ActiveTargetSlotIndex = Targets[0].SlotIndex;
    }
}

void UMythicCharacterPageWidget::CycleActiveEquipmentTarget() {
    UItemSlotVM *SlotVM = GetSelectedSlot();
    UMythicItemInstance *Item = SlotVM ? SlotVM->TryGetItemInstance() : nullptr;
    if (!Item || !Item->GetItemDefinition()) {
        return;
    }

    TArray<FEquipmentTarget> Targets = BuildEquipmentTargets();
    if (Targets.Num() < 2) {
        return;
    }
    // Cycle in canonical authored order, independent of the sticky-first presentation sort.
    Targets.Sort([](const FEquipmentTarget &A, const FEquipmentTarget &B) {
        if (A.bExpectEmpty != B.bExpectEmpty) {
            return A.bExpectEmpty;
        }
        if (A.GroupDisplayOrder != B.GroupDisplayOrder) {
            return A.GroupDisplayOrder < B.GroupDisplayOrder;
        }
        if (A.Key.GroupTag != B.Key.GroupTag) {
            return A.Key.GroupTag.ToString() < B.Key.GroupTag.ToString();
        }
        if (A.Key.EntryIndex != B.Key.EntryIndex) {
            return A.Key.EntryIndex < B.Key.EntryIndex;
        }
        const FString ADefinition = A.Key.SlotDefinitionId.ToString();
        const FString BDefinition = B.Key.SlotDefinitionId.ToString();
        if (ADefinition != BDefinition) {
            return ADefinition < BDefinition;
        }
        if (A.Key.RepetitionOrdinal != B.Key.RepetitionOrdinal) {
            return A.Key.RepetitionOrdinal < B.Key.RepetitionOrdinal;
        }
        return A.SlotIndex < B.SlotIndex;
    });
    int32 CurrentIndex = Targets.IndexOfByPredicate([this](const FEquipmentTarget &Target) {
        return Target.SlotIndex == ActiveTargetSlotIndex;
    });
    CurrentIndex = CurrentIndex == INDEX_NONE ? 0 : (CurrentIndex + 1) % Targets.Num();
    const FEquipmentTarget &Next = Targets[CurrentIndex];
    ActiveTargetSlotIndex = Next.SlotIndex;
    StickyEquipmentTargetKeys.Add(Item->GetItemDefinition()->ItemType, Next.Key);
    RefreshDetailsForSelection();
    RefreshInventoryActionBar();
}

int32 UMythicCharacterPageWidget::FindUnequipDestination() const {
    UMythicInventoryComponent *Inventory = GetInventoryComponent();
    UItemSlotVM *SourceSlot = GetSelectedSlot();
    UMythicItemInstance *Item = SourceSlot ? SourceSlot->TryGetItemInstance() : nullptr;
    if (!Inventory || !SourceSlot || !Item
        || !Inventory->GetAllSlots().IsValidIndex(SourceSlot->GetAbsoluteIndex())) {
        return INDEX_NONE;
    }
    const FMythicInventorySlotEntry &SourceEntry =
        Inventory->GetAllSlots()[SourceSlot->GetAbsoluteIndex()];
    if (!SourceEntry.IsGearSlot() || !SourceEntry.bCanPlayerTake) {
        return INDEX_NONE;
    }

    TArray<int32> Candidates;
    for (int32 Index = 0; Index < Inventory->GetAllSlots().Num(); ++Index) {
        const FMythicInventorySlotEntry &Entry = Inventory->GetAllSlots()[Index];
        if (!Entry.IsGearSlot() && !Entry.SlottedItemInstance && Entry.bCanPlayerPut
            && Inventory->CanSlotAcceptItem(Index, Item, true, Item)) {
            Candidates.Add(Index);
        }
    }
    Candidates.Sort([Inventory](const int32 A, const int32 B) {
        const FMythicInventorySlotEntry &Left = Inventory->GetAllSlots()[A];
        const FMythicInventorySlotEntry &Right = Inventory->GetAllSlots()[B];
        const FInventorySlotGroup *LeftGroup = Inventory->InventoryProfile
            ? Inventory->InventoryProfile->SlotGroups.Find(Left.GroupTag) : nullptr;
        const FInventorySlotGroup *RightGroup = Inventory->InventoryProfile
            ? Inventory->InventoryProfile->SlotGroups.Find(Right.GroupTag) : nullptr;
        const int32 LeftOrder = LeftGroup ? LeftGroup->DisplayOrder : 0;
        const int32 RightOrder = RightGroup ? RightGroup->DisplayOrder : 0;
        if (LeftOrder != RightOrder) {
            return LeftOrder < RightOrder;
        }
        if (Left.GroupTag != Right.GroupTag) {
            return Left.GroupTag.ToString() < Right.GroupTag.ToString();
        }
        if (Left.EntryIndex != Right.EntryIndex) {
            return Left.EntryIndex < Right.EntryIndex;
        }
        return A < B;
    });
    return Candidates.IsEmpty() ? INDEX_NONE : Candidates[0];
}

UMythicInventoryInteractionCoordinator *
UMythicCharacterPageWidget::GetInventoryInteractionCoordinator() const {
    return GetOwningLocalPlayer()
        ? GetOwningLocalPlayer()->GetSubsystem<UMythicInventoryInteractionCoordinator>()
        : nullptr;
}

bool UMythicCharacterPageWidget::IsMutationPending() const {
    const UMythicInventoryInteractionCoordinator *Coordinator =
        GetInventoryInteractionCoordinator();
    return Coordinator && Coordinator->IsMutationPending();
}

void UMythicCharacterPageWidget::RefreshInventoryActionBar() {
    UItemSlotVM *SlotVM = GetSelectedSlot();
    UMythicInventoryComponent *Inventory = GetInventoryComponent();
    UMythicItemInstance *Item = SlotVM ? SlotVM->TryGetItemInstance() : nullptr;
    const bool bHasItem = Item && Item->GetItemInstanceGuid().IsValid();
    const bool bBrowsing = InventoryPageState == EInventoryPageState::Browsing;
    const bool bMoveTarget = InventoryPageState == EInventoryPageState::MoveTarget;
    const bool bPending = IsMutationPending();
    const TArray<FEquipmentTarget> EquipmentTargets =
        bHasItem && !SlotVM->GetIsEquipped() ? BuildEquipmentTargets() : TArray<FEquipmentTarget>();

    if (PrimaryActionButton) {
        FText Label;
        bool bRelevant = false;
        bool bEnabled = false;
        if (bMoveTarget) {
            Label = NSLOCTEXT("MythicInventory", "MoveHereAction", "Move Here");
            bRelevant = true;
            bEnabled = ActiveTargetSlotIndex != INDEX_NONE
                && CanMoveSelectionToSlot(ActiveTargetSlotIndex) && !bPending;
        }
        else if (bBrowsing && bHasItem) {
            if (SlotVM->GetIsEquipped()) {
                Label = NSLOCTEXT("MythicInventory", "UnequipAction", "Unequip");
                bRelevant = true;
                bEnabled = FindUnequipDestination() != INDEX_NONE && !bPending;
            }
            else if (EquipmentTargets.Num() > 0) {
                Label = NSLOCTEXT("MythicInventory", "EquipAction", "Equip");
                bRelevant = true;
                bEnabled = ActiveTargetSlotIndex != INDEX_NONE && !bPending;
            }
            else if (Inventory && Inventory->CanUseItemInSlot(SlotVM->GetAbsoluteIndex())) {
                Label = NSLOCTEXT("MythicInventory", "UseAction", "Use");
                bRelevant = true;
                bEnabled = !bPending;
            }
        }
        PrimaryActionButton->SetVisibility(
            bRelevant ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        PrimaryActionButton->SetLabelOverride(Label);
        PrimaryActionButton->SetIsEnabled(bEnabled);
    }
    if (ActionsActionButton) {
        ActionsActionButton->SetLabelOverride(
            bMoveTarget ? NSLOCTEXT("MythicInventory", "CancelMoveAction", "Cancel Move")
                        : NSLOCTEXT("MythicInventory", "ActionsAction", "Actions"));
        ActionsActionButton->SetIsEnabled((bBrowsing && bHasItem) || bMoveTarget);
    }
    if (TargetActionButton) {
        const bool bCanCycle = bBrowsing && bHasItem && !SlotVM->GetIsEquipped()
            && EquipmentTargets.Num() > 1;
        TargetActionButton->SetLabelOverride(NSLOCTEXT("MythicInventory", "TargetAction", "Target"));
        TargetActionButton->SetVisibility(
            bCanCycle ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        TargetActionButton->SetIsEnabled(bCanCycle);
    }
    if (SortActionButton) {
        SortActionButton->SetLabelOverride(NSLOCTEXT("MythicInventory", "SortAction", "Sort"));
        bool bCanSort = false;
        if (bBrowsing && Inventory && SlotVM) {
            FMythicInventorySlotEntry Entry;
            bCanSort = Inventory->GetSlotEntry(SlotVM->GetAbsoluteIndex(), Entry) && !Entry.IsGearSlot();
        }
        SortActionButton->SetIsEnabled(bCanSort && !bPending);
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
    if (IsMutationPending()) {
        SetFeedback(NSLOCTEXT("MythicInventory", "RequestAlreadyPending", "Finish syncing the current inventory change first."));
        return;
    }

    UItemSlotVM *SlotVM = GetSelectedSlot();
    UMythicInventoryComponent *Inventory = GetInventoryComponent();
    UMythicItemInstance *Item = SlotVM ? SlotVM->TryGetItemInstance() : nullptr;
    if (!Item || !Item->GetItemDefinition() || !Inventory) {
        return;
    }
    if (SlotVM->GetIsEquipped()) {
        const int32 Destination = FindUnequipDestination();
        if (Destination == INDEX_NONE) {
            SetFeedback(NSLOCTEXT("MythicInventory", "NoUnequipSpace", "No compatible carried slot is available."), true);
            return;
        }
        SubmitMoveToSlot(Destination);
        return;
    }

    const TArray<FEquipmentTarget> Targets = BuildEquipmentTargets();
    if (Targets.Num() > 0) {
        ResolveActiveEquipmentTarget();
        const FEquipmentTarget *Target = Targets.FindByPredicate([this](const FEquipmentTarget &Candidate) {
            return Candidate.SlotIndex == ActiveTargetSlotIndex;
        });
        Target = Target ? Target : &Targets[0];
        StickyEquipmentTargetKeys.Add(Item->GetItemDefinition()->ItemType, Target->Key);
        SubmitMoveToSlot(Target->SlotIndex);
        return;
    }
    if (Inventory->CanUseItemInSlot(SlotVM->GetAbsoluteIndex())) {
        SubmitUse();
        return;
    }
}

void UMythicCharacterPageWidget::HandleInventoryActionsAction() {
    if (InventoryPageState == EInventoryPageState::MoveTarget) {
        CloseInventoryModal(true);
        return;
    }
    if (InventoryPageState == EInventoryPageState::Browsing && GetSelectedSlot()) {
        OpenContextMenu();
    }
}

void UMythicCharacterPageWidget::HandleCycleInventoryTarget() {
    if (InventoryPageState != EInventoryPageState::Browsing || !GetSelectedSlot()
        || GetSelectedSlot()->GetIsEquipped()) {
        return;
    }
    CycleActiveEquipmentTarget();
}

void UMythicCharacterPageWidget::HandleSortInventoryAction() {
    if (InventoryPageState == EInventoryPageState::Browsing
        && GetSelectedSlot() && !IsMutationPending()) {
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
    bool bEnabled, const FText &Tooltip, const bool bRequiresHold) {
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
    if (UCommonButtonBase *CommonButton = Cast<UCommonButtonBase>(Button)) {
        CommonButton->SetRequiresHold(bRequiresHold);
    }
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

void UMythicCharacterPageWidget::OpenContextMenu() {
    UItemSlotVM *SlotVM = GetSelectedSlot();
    UMythicInventoryComponent *Inventory = GetInventoryComponent();
    UMythicItemInstance *Item = SlotVM ? SlotVM->TryGetItemInstance() : nullptr;
    if (!SlotVM || !Inventory || !Item || !InventoryModalLayer) {
        return;
    }
    ActionSourceGuid = Item->GetItemInstanceGuid();
    ActionSourceSlotIndex = SlotVM->GetAbsoluteIndex();
    InventoryPageState = EInventoryPageState::Context;
    ClearModalOptions();
    InventoryModalTitle->SetText(SlotVM->GetItemName());
    InventoryModalBody->SetText(NSLOCTEXT(
        "MythicInventory", "ActionMenuHint",
        "Available actions for this exact item."));

    const bool bCanTake = Inventory->CanPlayerTakeFromSlot(SlotVM->GetAbsoluteIndex());
    const bool bEquipped = SlotVM->GetIsEquipped();
    const bool bCurrency = Item->GetItemDefinition()
        && Item->GetItemDefinition()->ItemType.MatchesTag(ITEMIZATION_TYPE_CURRENCY);
    const TArray<FEquipmentTarget> EquipmentTargets =
        bEquipped ? TArray<FEquipmentTarget>() : BuildEquipmentTargets();
    const bool bUsable = Inventory->CanUseItemInSlot(SlotVM->GetAbsoluteIndex());

    FMythicInventoryInteractionPolicyInput PolicyInput;
    PolicyInput.bIsEquipped = bEquipped;
    PolicyInput.bEquippable = !bEquipped && EquipmentTargets.Num() > 0;
    PolicyInput.bUsable = !bEquipped && bUsable;
    PolicyInput.bPrimaryEnabled = bEquipped
        ? FindUnequipDestination() != INDEX_NONE
        : (PolicyInput.bEquippable || bUsable);
    PolicyInput.bMoveRelevant = true;
    PolicyInput.bMoveEnabled = bCanTake;
    PolicyInput.bManualJunk = SlotVM->GetIsManuallyMarkedJunk();
    PolicyInput.bCanToggleManualJunk = bCanTake && !bEquipped && !bCurrency;
    PolicyInput.bCanDrop = bCanTake && !bEquipped && !bCurrency;
    PolicyInput.bMutationPending = IsMutationPending();
    PolicyInput.Quantity = Item->GetStacks();
    PolicyInput.Rarity = SlotVM->GetRarity();
    PolicyInput.PrimaryDisabledReason = bEquipped
        ? EMythicInventoryActionResult::InventoryFull
        : EMythicInventoryActionResult::IncompatibleTarget;
    PolicyInput.MoveDisabledReason = EMythicInventoryActionResult::SourceProtected;

    const TArray<FMythicInventoryContextAction> Actions =
        FMythicInventoryInteractionPolicy::BuildContextActions(PolicyInput);
    for (const FMythicInventoryContextAction &Action : Actions) {
        EMythicInventoryUICommand Command = EMythicInventoryUICommand::BeginMove;
        switch (Action.Verb) {
        case EMythicInventoryContextVerb::Equip:
        case EMythicInventoryContextVerb::Unequip:
            Command = EMythicInventoryUICommand::EquipOrUnequip;
            break;
        case EMythicInventoryContextVerb::Use:
            Command = EMythicInventoryUICommand::Use;
            break;
        case EMythicInventoryContextVerb::Split:
            Command = EMythicInventoryUICommand::Split;
            break;
        case EMythicInventoryContextVerb::Move:
            Command = EMythicInventoryUICommand::BeginMove;
            break;
        case EMythicInventoryContextVerb::ToggleManualJunk:
            Command = EMythicInventoryUICommand::ToggleJunk;
            break;
        case EMythicInventoryContextVerb::Drop:
            Command = EMythicInventoryUICommand::Drop;
            break;
        }
        const FText DisabledTooltip = Action.bEnabled
            ? FText::GetEmpty()
            : UMythicInventoryInteractionCoordinator::DescribeResult(Action.DisabledReason);
        AddModalCommand(
            Action.Label, Command, INDEX_NONE, Action.bEnabled, DisabledTooltip,
            Action.bRequiresHold);
    }
    InventoryModalLayer->SetVisibility(ESlateVisibility::Visible);
    if (InventoryModalOptions->GetChildrenCount() > 0) {
        UWidget *First = InventoryModalOptions->GetChildAt(0);
        First->SetFocus();
    }
    RefreshInventoryActionBar();
}

void UMythicCharacterPageWidget::OpenSortMenu() {
    UItemSlotVM *SlotVM = GetSelectedSlot();
    UMythicInventoryComponent *Inventory = GetInventoryComponent();
    if (!SlotVM || !Inventory || !InventoryModalLayer || IsMutationPending()) {
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
    InventoryPageState = EInventoryPageState::Sort;
    ClearModalOptions();
    InventoryModalTitle->SetText(NSLOCTEXT("MythicInventory", "SortTitle", "Sort Category"));
    InventoryModalBody->SetText(NSLOCTEXT("MythicInventory", "SortHint", "Choose how this carried-item group is ordered."));
    AddModalCommand(NSLOCTEXT("MythicInventory", "SortRarity", "Rarity"), EMythicInventoryUICommand::SortByRarity);
    AddModalCommand(NSLOCTEXT("MythicInventory", "SortType", "Type"), EMythicInventoryUICommand::SortByType);
    AddModalCommand(NSLOCTEXT("MythicInventory", "SortName", "Name"), EMythicInventoryUICommand::SortByName);
    AddModalCommand(NSLOCTEXT("MythicInventory", "SortValue", "Value"), EMythicInventoryUICommand::SortByValue);
    AddModalCommand(NSLOCTEXT("MythicInventory", "SortWeight", "Weight"), EMythicInventoryUICommand::SortByWeight);
    InventoryModalLayer->SetVisibility(ESlateVisibility::Visible);
    InventoryModalOptions->GetChildAt(0)->SetFocus();
    RefreshInventoryActionBar();
}

void UMythicCharacterPageWidget::OpenQuantityPanel(EQuantityPurpose Purpose) {
    UItemSlotVM *SlotVM = GetSelectedSlot();
    UMythicItemInstance *Item = SlotVM ? SlotVM->TryGetItemInstance() : nullptr;
    if (!Item || !InventoryModalLayer || IsMutationPending()) {
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
    InventoryModalLayer->SetVisibility(ESlateVisibility::Visible);
    InventoryModalOptions->GetChildAt(0)->SetFocus();
    RefreshInventoryActionBar();
}

void UMythicCharacterPageWidget::CloseInventoryModal(bool bRestoreSourceSelection) {
    if (InventoryModalLayer) {
        InventoryModalLayer->SetVisibility(ESlateVisibility::Collapsed);
    }
    ClearModalOptions();
    QuantityPurpose = EQuantityPurpose::None;
    ActiveTargetSlotIndex = INDEX_NONE;
    InventoryPageState = EInventoryPageState::Browsing;
    if (bRestoreSourceSelection && ActionSourceGuid.IsValid()) {
        SelectedItemGuid = ActionSourceGuid;
        ScheduleRestoreSelection();
    }
    ActionSourceGuid.Invalidate();
    ActionSourceSlotIndex = INDEX_NONE;
    ResolveActiveEquipmentTarget();
    RefreshDetailsForSelection();
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
    if (IsMutationPending()) {
        SetFeedback(NSLOCTEXT("MythicInventory", "RequestAlreadyPending", "Finish syncing the current inventory change first."));
        return;
    }
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
    (void)Payload;
    switch (Command) {
    case EMythicInventoryUICommand::EquipOrUnequip:
        CloseInventoryModal(true);
        HandlePrimaryInventoryAction();
        break;
    case EMythicInventoryUICommand::Use:
        CloseInventoryModal(true);
        SubmitUse();
        break;
    case EMythicInventoryUICommand::BeginMove:
        BeginMoveTargetSelection();
        break;
    case EMythicInventoryUICommand::Split:
        OpenQuantityPanel(EQuantityPurpose::Split);
        break;
    case EMythicInventoryUICommand::Drop:
        if (UItemSlotVM *SlotVM = GetSelectedSlot(); SlotVM && SlotVM->GetQuantity() <= 1) {
            CloseInventoryModal(true);
            SubmitDrop(1);
        }
        else {
            OpenQuantityPanel(EQuantityPurpose::Drop);
        }
        break;
    case EMythicInventoryUICommand::ToggleJunk:
        CloseInventoryModal(true);
        SubmitSetJunk();
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
    case EMythicInventoryUICommand::QuantityDecreaseLarge:
        QuantityValue = FMath::Clamp(QuantityValue - 10, 1, QuantityMaximum);
        if (InventoryModalBody) {
            InventoryModalBody->SetText(FText::Format(
                NSLOCTEXT("MythicInventory", "QuantityReadout", "Quantity: {0} / {1}"),
                FText::AsNumber(QuantityValue), FText::AsNumber(QuantityMaximum)));
        }
        break;
    case EMythicInventoryUICommand::QuantityIncreaseLarge:
        QuantityValue = FMath::Clamp(QuantityValue + 10, 1, QuantityMaximum);
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
    case EMythicInventoryUICommand::Cancel:
        CloseInventoryModal(true);
        break;
    }
}

void UMythicCharacterPageWidget::SubmitMoveToSlot(int32 TargetSlotIndex) {
    FMythicInventorySourceLocator Source;
    FMythicInventoryTargetLocator Target;
    if (!CanMoveSelectionToSlot(TargetSlotIndex)
        || !BuildSourceLocator(Source) || !BuildTargetLocator(TargetSlotIndex, Target)
        || Source.SlotIndex == Target.SlotIndex) {
        SetFeedback(NSLOCTEXT("MythicInventory", "MoveTargetChanged", "The source or destination changed. Choose again."), true);
        return;
    }
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer());
    HandleSubmittedRequest(PC ? PC->SubmitInventoryMove(Source, Target) : 0);
}

void UMythicCharacterPageWidget::SubmitUse() {
    FMythicInventorySourceLocator Source;
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer());
    if (!BuildSourceLocator(Source) || !PC) {
        SetFeedback(NSLOCTEXT("MythicInventory", "UseSourceChanged", "That item is no longer available."), true);
        return;
    }
    HandleSubmittedRequest(PC->SubmitInventoryUse(Source));
}

void UMythicCharacterPageWidget::SubmitSplit(int32 Quantity) {
    FMythicInventorySourceLocator Source;
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer());
    if (!BuildSourceLocator(Source) || !PC || Quantity < 1 || Quantity >= Source.ExpectedQuantity) {
        SetFeedback(NSLOCTEXT("MythicInventory", "SplitSourceChanged", "The stack changed. Reopen Split."), true);
        return;
    }
    HandleSubmittedRequest(PC->SubmitInventorySplit(Source, Quantity));
}

void UMythicCharacterPageWidget::SubmitDrop(int32 Quantity) {
    FMythicInventorySourceLocator Source;
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer());
    if (!BuildSourceLocator(Source) || !PC || Quantity < 1 || Quantity > Source.ExpectedQuantity) {
        SetFeedback(NSLOCTEXT("MythicInventory", "DropSourceChanged", "The stack changed. Reopen Drop."), true);
        return;
    }
    HandleSubmittedRequest(PC->SubmitInventoryDropQuantity(Source, Quantity));
}

void UMythicCharacterPageWidget::SubmitSetJunk() {
    FMythicInventorySourceLocator Source;
    UItemSlotVM *SlotVM = GetSelectedSlot();
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer());
    if (!BuildSourceLocator(Source) || !SlotVM || !PC) {
        SetFeedback(NSLOCTEXT("MythicInventory", "JunkSourceChanged", "That item is no longer available."), true);
        return;
    }
    HandleSubmittedRequest(PC->SubmitInventorySetJunk(
        Source, !SlotVM->GetIsManuallyMarkedJunk()));
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
    HandleSubmittedRequest(PC->SubmitInventorySort(Inventory, Entry.GroupTag, Mode));
}

void UMythicCharacterPageWidget::HandleSubmittedRequest(const int64 RequestId) {
    if (RequestId <= 0) {
        SetFeedback(
            IsMutationPending()
                ? NSLOCTEXT("MythicInventory", "RequestAlreadyPending", "Finish syncing the current inventory change first.")
                : NSLOCTEXT("MythicInventory", "RequestNotSent", "The inventory request could not be sent."),
            true);
        RefreshInventoryActionBar();
        return;
    }

    InventoryPageState = EInventoryPageState::Browsing;
    if (InventoryModalLayer) {
        InventoryModalLayer->SetVisibility(ESlateVisibility::Collapsed);
    }
    ClearModalOptions();
    ActionSourceGuid.Invalidate();
    ActionSourceSlotIndex = INDEX_NONE;
    if (IsMutationPending()) {
        SetFeedback(NSLOCTEXT("MythicInventory", "RequestPending", "Updating inventory..."));
    }
    RefreshInventoryActionBar();
}

void UMythicCharacterPageWidget::HandleInventorySlotUpdated(const int32 SlotIndex) {
    (void)SlotIndex;
    ScheduleRestoreSelection();
}

void UMythicCharacterPageWidget::HandleInventoryInteractionFeedback(
    const FText &Message,
    const bool bIsError) {
    SetFeedback(Message, bIsError);
}

void UMythicCharacterPageWidget::HandleInventoryPendingChanged(
    const bool bPending,
    const FMythicInventoryActionSubmission &Submission) {
    (void)bPending;
    (void)Submission;
    RefreshInventoryActionBar();
}

void UMythicCharacterPageWidget::HandleInventoryActionReceipt(
    const FMythicInventoryActionReceipt &Receipt) {
    (void)Receipt;
    InventoryPageState = EInventoryPageState::Browsing;
    ActionSourceGuid.Invalidate();
    ActionSourceSlotIndex = INDEX_NONE;
    ScheduleRestoreSelection();
    RefreshInventoryActionBar();
}

void UMythicCharacterPageWidget::BuildDetailsCard() {
    if (DetailsCard || !DetailsHost || !ItemDetailsClass) {
        return;
    }

    if (!GetOwningLocalPlayer()) {
        return;
    }

    DetailsCard = CreateWidget<UMythicItemDetailsWidget>(GetOwningPlayer(), ItemDetailsClass);
    if (!DetailsCard) {
        return;
    }

    DetailsHost->AddChild(DetailsCard);
    DetailsCard->SetVisibility(ESlateVisibility::Collapsed);
    DetailsSurface = DetailsHost->GetParent();
    if (!DetailsSurface) {
        DetailsSurface = DetailsHost.Get();
    }
    DetailsSurface->SetVisibility(ESlateVisibility::Collapsed);
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
    UItemSlotVM *SelectedSlot = Cast<UItemSlotVM>(SlotVM);
    UMythicItemInstance *SelectedItem = SelectedSlot
        ? SelectedSlot->TryGetItemInstance() : nullptr;
    const bool bHasItem = SelectedItem != nullptr && DetailsCard != nullptr;

    if (DetailsCard) {
        if (bHasItem) {
            if (UMVVMView *View = UMVVMSubsystem::GetViewFromUserWidget(DetailsCard)) {
                View->SetViewModelByClass(TScriptInterface<INotifyFieldValueChanged>(SlotVM));
            }

            FMythicItemDetailsComparisonContext Context;
            if (!SelectedSlot->GetIsEquipped()) {
                const TArray<FEquipmentTarget> Targets = BuildEquipmentTargets(SelectedSlot);
                const bool bPinnedSelection = SelectedItemGuid == SelectedItem->GetItemInstanceGuid();
                const int32 PreferredTarget = bPinnedSelection ? ActiveTargetSlotIndex : INDEX_NONE;
                const FEquipmentTarget *Target = Targets.FindByPredicate([PreferredTarget](const FEquipmentTarget &Candidate) {
                    return Candidate.SlotIndex == PreferredTarget;
                });
                if (!Target && Targets.Num() > 0) {
                    Target = &Targets[0];
                    if (bPinnedSelection) {
                        ActiveTargetSlotIndex = Target->SlotIndex;
                    }
                }
                if (Target) {
                    Context.bComparisonActive = true;
                    Context.bTargetEmpty = Target->bExpectEmpty;
                    Context.TargetSlotIndex = Target->SlotIndex;
                    Context.bCanCycleTarget = Targets.Num() > 1;
                    Context.TargetLabel = Target->DisplayName;
                    if (Target->bExpectEmpty) {
                        Context.BaselineItem = nullptr;
                    }
                    else {
                        FMythicInventorySlotEntry Entry;
                        if (UMythicInventoryComponent *Inventory = GetInventoryComponent();
                            Inventory && Inventory->GetSlotEntry(Target->SlotIndex, Entry)
                            && Entry.SlottedItemInstance) {
                            Context.BaselineItem = Entry.SlottedItemInstance;
                            Context.ExpectedBaselineGuid =
                                Entry.SlottedItemInstance->GetItemInstanceGuid();
                        }
                    }
                }
            }
            DetailsCard->PresentItemStatSections(SelectedItem, Context);
            DisplayedDetailsSlot = SelectedSlot;
            ScheduleDetailsPosition(SelectedSlot);
        }
        else {
            DetailsCard->ClearPresentedItem();
            DisplayedDetailsSlot.Reset();
            ++DetailsPositionSerial;
        }
        DetailsCard->SetVisibility(bHasItem ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
    }

    if (DetailsSurface) {
        DetailsSurface->SetVisibility(
            bHasItem ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
    }

    if (SocketRow) {
        SocketRow->SetItem(bHasItem ? SelectedItem : nullptr);
    }

    if (DetailsPlaceholder) {
        DetailsPlaceholder->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UMythicCharacterPageWidget::RefreshDetailsForSelection() {
    ShowDetailsFor(GetSelectedSlot());
}

void UMythicCharacterPageWidget::ScheduleDetailsPosition(UItemSlotVM *SlotVM) {
    if (!SlotVM || !DetailsSurface) {
        return;
    }

    const uint32 PositionSerial = ++DetailsPositionSerial;
    PositionDetailsBeside(SlotVM);
    if (UWorld *World = GetWorld()) {
        const TWeakObjectPtr<UMythicCharacterPageWidget> WeakThis(this);
        const TWeakObjectPtr<UItemSlotVM> WeakSlot(SlotVM);
        World->GetTimerManager().SetTimerForNextTick(
            FTimerDelegate::CreateLambda([WeakThis, WeakSlot, PositionSerial]() {
                UMythicCharacterPageWidget *Page = WeakThis.Get();
                UItemSlotVM *ResolvedSlot = WeakSlot.Get();
                if (Page && ResolvedSlot && Page->DetailsPositionSerial == PositionSerial
                    && Page->DisplayedDetailsSlot.Get() == ResolvedSlot) {
                    Page->PositionDetailsBeside(ResolvedSlot);
                }
            }));
    }
}

void UMythicCharacterPageWidget::PositionDetailsBeside(UItemSlotVM *SlotVM) {
    UUserWidget *EntryWidget = FindEntryWidgetForItem(SlotVM);
    UOverlaySlot *SurfaceSlot = DetailsSurface
        ? Cast<UOverlaySlot>(DetailsSurface->Slot) : nullptr;
    if (!EntryWidget || !SurfaceSlot) {
        return;
    }

    const FGeometry PageGeometry = GetCachedGeometry();
    const FGeometry EntryGeometry = EntryWidget->GetCachedGeometry();
    const FVector2D PageSize = PageGeometry.GetLocalSize();
    if (PageSize.X <= 1.0f || PageSize.Y <= 1.0f) {
        return;
    }

    const FVector2D EntryTopLeft = PageGeometry.AbsoluteToLocal(EntryGeometry.GetAbsolutePosition());
    const FVector2D EntryBottomRight = PageGeometry.AbsoluteToLocal(
        EntryGeometry.LocalToAbsolute(EntryGeometry.GetLocalSize()));
    FVector2D CardSize = DetailsSurface->GetDesiredSize();
    if (CardSize.X <= 1.0f) {
        CardSize.X = 380.0f;
    }
    if (CardSize.Y <= 1.0f) {
        CardSize.Y = 520.0f;
    }

    constexpr float Gap = 12.0f;
    constexpr float EdgePadding = 16.0f;
    const float RightX = EntryBottomRight.X + Gap;
    const float LeftX = EntryTopLeft.X - CardSize.X - Gap;
    const bool bFitsRight = RightX + CardSize.X <= PageSize.X - EdgePadding;
    const bool bFitsLeft = LeftX >= EdgePadding;
    float X = RightX;
    if (!bFitsRight && bFitsLeft) {
        X = LeftX;
    }
    else if (!bFitsRight && !bFitsLeft) {
        const float SpaceRight = PageSize.X - EntryBottomRight.X;
        const float SpaceLeft = EntryTopLeft.X;
        X = SpaceRight >= SpaceLeft ? RightX : LeftX;
    }
    X = FMath::Clamp(X, EdgePadding, FMath::Max(EdgePadding, PageSize.X - CardSize.X - EdgePadding));
    const float Y = FMath::Clamp(
        EntryTopLeft.Y,
        EdgePadding,
        FMath::Max(EdgePadding, PageSize.Y - CardSize.Y - EdgePadding));

    SurfaceSlot->SetHorizontalAlignment(HAlign_Left);
    SurfaceSlot->SetVerticalAlignment(VAlign_Top);
    SurfaceSlot->SetPadding(FMargin(FMath::RoundToFloat(X), FMath::RoundToFloat(Y), 0.0f, 0.0f));
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

void UMythicCharacterPageWidget::BindBagViewModel(UInventoryVM *PreferredVM) {
    if (BoundSelectionVM.IsValid()) {
        return;
    }
    UInventoryVM *VM = PreferredVM ? PreferredVM : ResolveInventoryVM();
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
