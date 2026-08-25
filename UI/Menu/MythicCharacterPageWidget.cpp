// Copyright Stellar Games. All Rights Reserved.

#include "MythicCharacterPageWidget.h"


#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "UI/MythicUIKit.h"
#include "Progression/Runes/MythicRuneComponent.h"
#include "Progression/Runes/MythicRuneDefinition.h"
#include "UI/Menu/MythicRunePickerWidget.h"
#include "UI/Inventory/MythicSocketRowWidget.h"
#include "UI/Widgets/MythicSectionHeader.h"
#include "UI/MythicUIStyle.h"
#include "Components/Border.h"
#include "Components/Button.h"
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

namespace {
const FName Char_Percent(TEXT("Percent"));
const FName Char_ChipPercent(TEXT("ChipPercent"));
const FName Char_FillStart(TEXT("FillColorStart"));
const FName Char_FillEnd(TEXT("FillColorEnd"));
}

void UMythicCharacterPageWidget::NativeConstruct() {
    // Pool before Super: activation can fire inside it, and a refresh must find these already built.
    BuildBagChrome();

    Super::NativeConstruct();

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
    BindProgression();
    RefreshHeader();
    BuildSockets();
    RefreshSockets();
    SelectFirstOccupiedSlot();

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
        // Focus alone: the page opens showing the figure, and the first deliberate move opens the details.
        Strip->SetSelectedItem(nullptr);
    }
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
