// Copyright Stellar Games. All Rights Reserved.

#include "MythicCharacterPageWidget.h"


#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "UI/MythicUIStyle.h"
#include "Components/Button.h"
#include "Blueprint/WidgetTree.h"
#include "CommonActivatableWidget.h"
#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "Components/ListView.h"
#include "Components/TileView.h"
#include "Components/TreeView.h"
#include "Components/PanelWidget.h"
#include "Components/VerticalBoxSlot.h"
#include "Itemization/Inventory/ViewModels/ItemSlotVM.h"
#include "MVVMSubsystem.h"
#include "View/MVVMView.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Proficiencies.h"
#include "GAS/MythicTags_GAS.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Player/MythicPlayerController.h"
#include "UObject/UObjectIterator.h"
#include "UI/MythicHUDLayout.h"

namespace {
const FName Char_Percent(TEXT("Percent"));
const FName Char_ChipPercent(TEXT("ChipPercent"));
const FName Char_FillStart(TEXT("FillColorStart"));
const FName Char_FillEnd(TEXT("FillColorEnd"));
}

void UMythicCharacterPageWidget::NativeConstruct() {
    Super::NativeConstruct();

    if (Img_XpBar && XpBarMaterial) {
        FSlateBrush Brush;
        Brush.SetResourceObject(XpBarMaterial);
        Brush.ImageSize = FVector2D(320.0f, 12.0f);
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

        const FString MachineName = FPlatformProcess::ComputerName();
        if (PlayerName.IsEmpty() || (!MachineName.IsEmpty() && PlayerName.StartsWith(MachineName))) {
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
            Txt_XpValue->SetVisibility(ESlateVisibility::HitTestInvisible);
            Txt_XpValue->SetText(FText::Format(NSLOCTEXT("Mythic", "CharXp", "{0} / {1}"),
                                               FText::AsNumber(FMath::FloorToInt(Current)),
                                               FText::AsNumber(FMath::FloorToInt(Max))));
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
    BindProgression();
    RefreshHeader();
    BuildSockets();
    RefreshSockets();
    ShowDetailsFor(nullptr);
}

void UMythicCharacterPageWidget::NativeOnDeactivated() {
    UnbindSlotSelection();
    ReturnInventory();
    UnbindProgression();
    Super::NativeOnDeactivated();
}

void UMythicCharacterPageWidget::NativeDestruct() {
    UnbindSlotSelection();
    ReturnInventory();
    UnbindProgression();
    Super::NativeDestruct();
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
        if (ITypedUMGListView<UObject *> *Typed = AsTypedList(List)) {
            Typed->OnItemSelectionChanged().AddUObject(this, &UMythicCharacterPageWidget::HandleSlotSelectionChanged);
            BoundSlotLists.Add(List);
        }
    }
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
    ShowDetailsFor(nullptr);
}

void UMythicCharacterPageWidget::HandleSlotSelectionChanged(UObject *Item) {
    UItemSlotVM *SlotVM = Cast<UItemSlotVM>(Item);
    ShowDetailsFor(SlotVM && SlotVM->TryGetItemInstance() ? SlotVM : nullptr);
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

    if (UPanelSlot *Added = InventoryHost->AddChild(Inventory)) {
        if (UVerticalBoxSlot *V = Cast<UVerticalBoxSlot>(Added)) {
            V->SetHorizontalAlignment(HAlign_Fill);
            V->SetVerticalAlignment(VAlign_Fill);
            V->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }
    }
    Inventory->SetVisibility(ESlateVisibility::Visible);

    if (Activatable && GetOwningLocalPlayer()) {
        Activatable->ActivateWidget();
    }
}

void UMythicCharacterPageWidget::ReturnInventory() {
    UWidget *Inventory = BorrowedInventory.Get();
    BorrowedInventory.Reset();
    if (!Inventory) {
        return;
    }

    if (UCommonActivatableWidget *Activatable = Cast<UCommonActivatableWidget>(Inventory)) {
        Activatable->DeactivateWidget();
    }
    if (InventoryHost) {
        InventoryHost->RemoveChild(Inventory);
    }
    if (UMythicHUDLayout *Layout = Lender.Get()) {
        Layout->ReturnInventoryWidget(Inventory);
    }
}


void UMythicCharacterPageWidget::BuildSockets() {
    if (!SocketStrip || Sockets.Num() > 0) {
        return;
    }
    static const FSoftObjectPath WellPath(
        TEXT("/Game/Mythic/UI/Globals/materials/kit/MI_UI_SlotTex_Round.MI_UI_SlotTex_Round"));
    UMaterialInterface *WellMat = Cast<UMaterialInterface>(WellPath.TryLoad());

    for (int32 i = 0; i < FMath::Clamp(SocketCount, 1, 8); ++i) {
        FMythicRuneSocket Socket;
        Socket.SlotIndex = i;

        UOverlay *Stack = WidgetTree->ConstructWidget<UOverlay>();

        Socket.Well = WidgetTree->ConstructWidget<UImage>();
        if (WellMat) {
            FSlateBrush Brush;
            Brush.SetResourceObject(WellMat);
            Brush.DrawAs = ESlateBrushDrawType::Image;
            Brush.ImageSize = FVector2D(64.0f, 64.0f);
            Socket.Well->SetBrush(Brush);
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
        Hit->OnClicked.AddDynamic(Socket.Proxy, &UMythicRuneSocketClickProxy::HandleClicked);

        Socket.Button = Stack;
        SocketStrip->AddChild(Stack);
        Sockets.Add(Socket);
    }
    RefreshSockets();
}

void UMythicCharacterPageWidget::RefreshSockets() {
    for (FMythicRuneSocket &Socket : Sockets) {
        if (Socket.Mark) {
            Socket.Mark->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void UMythicRuneSocketClickProxy::HandleClicked() {
    if (UMythicCharacterPageWidget *P = Page.Get()) {
        P->OpenSocketPicker(SlotIndex);
    }
}

void UMythicCharacterPageWidget::OpenSocketPicker(int32 SlotIndex) {
    UMythicHUDLayout *Layout = GetTypedOuter<UMythicHUDLayout>();
    if (!Layout) {
        const APlayerController *PC = GetOwningPlayer();
        for (TObjectIterator<UMythicHUDLayout> It; It; ++It) {
            if (IsValid(*It) && !It->HasAnyFlags(RF_ClassDefaultObject) && It->GetOwningPlayer() == PC) {
                Layout = *It;
                break;
            }
        }
    }
    if (!Layout) {
        return;
    }
    Layout->PendingRuneSlot = SlotIndex;
    Layout->OpenMenuOnPage(TEXT("Powers"));
}
