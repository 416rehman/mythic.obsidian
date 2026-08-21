// Copyright Stellar Games. All Rights Reserved.

#include "MythicSocketPanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "Itemization/Inventory/Fragments/Passive/MythicGemFragment.h"
#include "Itemization/Inventory/Fragments/Passive/SocketsFragment.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Sockets/MythicSocketComponent.h"
#include "Player/MythicPlayerController.h"
#include "TimerManager.h"
#include "UI/MythicUIStyle.h"

void UMythicSocketClickProxy::HandleClicked() {
    if (UMythicSocketPanelWidget *Owner = Panel.Get()) {
        Owner->ActivateSocket(HostItem.Get(), SocketIndex);
    }
}

void UMythicGemClickProxy::HandleClicked() {
    if (UMythicSocketPanelWidget *Owner = Panel.Get()) {
        Owner->SelectGem(Gem.Get());
    }
}


void UMythicSocketPanelWidget::NativeConstruct() {
    if (!bPoolsBuilt) {
        bPoolsBuilt = true;

        if (ItemList) {
            for (int32 i = 0; i < PrewarmItemRows; ++i) {
                GetOrCreateItemRow(i);
            }
        }
        if (GemList) {
            for (int32 i = 0; i < PrewarmGemRows; ++i) {
                GetOrCreateGemRow(i);
            }
        }
    }

    Super::NativeConstruct();
}

void UMythicSocketPanelWidget::NativeOnActivated() {
    Super::NativeOnActivated();
    Bind();
    Refresh();
}

void UMythicSocketPanelWidget::NativeOnDeactivated() {
    Unbind();
    Super::NativeOnDeactivated();
}

void UMythicSocketPanelWidget::NativeDestruct() {
    Unbind();
    Super::NativeDestruct();
}

UMythicSocketComponent *UMythicSocketPanelWidget::GetSocketComponent() const {
    if (const APlayerController *PC = GetOwningPlayer()) {
        if (APawn *Pawn = PC->GetPawn()) {
            return Pawn->FindComponentByClass<UMythicSocketComponent>();
        }
    }
    return nullptr;
}

void UMythicSocketPanelWidget::Bind() {
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer());
    if (!PC) {
        return;
    }
    for (UMythicInventoryComponent *Inv : PC->GetAllInventoryComponents()) {
        if (!Inv || WatchedInventories.Contains(Inv)) {
            continue;
        }
        Inv->OnSlotUpdated.AddDynamic(this, &UMythicSocketPanelWidget::HandleSlotUpdated);
        WatchedInventories.Add(Inv);
    }
}

void UMythicSocketPanelWidget::Unbind() {
    for (const TWeakObjectPtr<UMythicInventoryComponent> &Weak : WatchedInventories) {
        if (UMythicInventoryComponent *Inv = Weak.Get()) {
            Inv->OnSlotUpdated.RemoveDynamic(this, &UMythicSocketPanelWidget::HandleSlotUpdated);
        }
    }
    WatchedInventories.Reset();
    bRefreshArmed = false;
}

void UMythicSocketPanelWidget::HandleSlotUpdated(int32 UpdatedSlotIndex) {
    RequestRefresh();
}

void UMythicSocketPanelWidget::RequestRefresh() {
    UWorld *World = GetWorld();
    if (!World || bRefreshArmed) {
        return;
    }
    bRefreshArmed = true;
    World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]() {
        bRefreshArmed = false;
        Refresh();
    }));
}


FMythicSocketItemRow &UMythicSocketPanelWidget::GetOrCreateItemRow(int32 Index) {
    if (ItemPool.IsValidIndex(Index)) {
        return ItemPool[Index];
    }

    FMythicSocketItemRow Row;
    UVerticalBox *Box = WidgetTree->ConstructWidget<UVerticalBox>();
    Row.Box = Box;

    Row.ItemName = FMythicUIStyle::MakeText(this, EMythicTextRole::Heading);
    Box->AddChild(Row.ItemName);

    Row.SocketSummary = FMythicUIStyle::MakeText(this, EMythicTextRole::Subtle);
    Box->AddChild(Row.SocketSummary);

    UHorizontalBox *Strip = WidgetTree->ConstructWidget<UHorizontalBox>();
    Row.ChipStrip = Strip;
    if (UVerticalBoxSlot *S = Cast<UVerticalBoxSlot>(Box->AddChild(Strip))) {
        S->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
    }

    for (int32 i = 0; i < ChipsPerRow; ++i) {
        FMythicSocketChip Chip;
        UCommonTextBlock *ChipLabel = nullptr;
        Chip.Button = FMythicUIStyle::MakeButton(this, EMythicTextRole::Body, ChipLabel);
        Chip.Label = ChipLabel;
        Chip.Label->SetJustification(ETextJustify::Center);
        Chip.Label->SetMinDesiredWidth(96.0f);

        Chip.Proxy = NewObject<UMythicSocketClickProxy>(this);
        Chip.Proxy->Panel = this;
        Chip.Proxy->SocketIndex = i;
        FMythicUIStyle::BindButtonClicked(Chip.Button, Chip.Proxy,
                                          GET_FUNCTION_NAME_CHECKED(UMythicSocketClickProxy, HandleClicked));

        if (UHorizontalBoxSlot *S = Cast<UHorizontalBoxSlot>(Strip->AddChild(Chip.Button))) {
            S->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));
        }
        Chip.Button->SetVisibility(ESlateVisibility::Collapsed);
        Row.Chips.Add(Chip);
    }

    Box->SetVisibility(ESlateVisibility::Collapsed);
    if (UVerticalBoxSlot *S = Cast<UVerticalBoxSlot>(ItemList->AddChild(Box))) {
        S->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
    }

    ItemPool.Add(Row);
    return ItemPool.Last();
}

FMythicGemRow &UMythicSocketPanelWidget::GetOrCreateGemRow(int32 Index) {
    if (GemPool.IsValidIndex(Index)) {
        return GemPool[Index];
    }

    FMythicGemRow Row;
    UCommonTextBlock *Label = nullptr;
    Row.Button = FMythicUIStyle::MakeButton(this, EMythicTextRole::Body, Label);
    Row.Label = Label;

    Row.Proxy = NewObject<UMythicGemClickProxy>(this);
    Row.Proxy->Panel = this;
    FMythicUIStyle::BindButtonClicked(Row.Button, Row.Proxy,
                                      GET_FUNCTION_NAME_CHECKED(UMythicGemClickProxy, HandleClicked));

    Row.Button->SetVisibility(ESlateVisibility::Collapsed);
    if (UVerticalBoxSlot *S = Cast<UVerticalBoxSlot>(GemList->AddChild(Row.Button))) {
        S->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
    }

    GemPool.Add(Row);
    return GemPool.Last();
}


FText UMythicSocketPanelWidget::ShortGemName(const FGameplayTag &GemType) {
    if (!GemType.IsValid()) {
        return NSLOCTEXT("Mythic", "SocketEmpty", "Empty");
    }
    FString Full = GemType.ToString();
    FString Leaf = Full;
    int32 Dot = INDEX_NONE;
    if (Full.FindLastChar(TEXT('.'), Dot)) {
        Leaf = Full.RightChop(Dot + 1);
    }
    return FText::FromString(Leaf);
}


void UMythicSocketPanelWidget::Refresh() {
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer());
    if (!PC) {
        return;
    }

    TArray<UMythicItemInstance *> Hosts;
    TArray<UMythicItemInstance *> Gems;
    for (UMythicInventoryComponent *Inv : PC->GetAllInventoryComponents()) {
        if (!Inv) {
            continue;
        }
        for (const FMythicInventorySlotEntry &Entry : Inv->GetAllSlots()) {
            UMythicItemInstance *Item = Entry.SlottedItemInstance;
            if (!Item) {
                continue;
            }
            if (Entry.bEquipmentSlot) {
                if (const USocketsFragment *Sockets = Item->GetFragment<USocketsFragment>()) {
                    if (Sockets->GetSocketCount() > 0) {
                        Hosts.Add(Item);
                    }
                }
            }
            else if (Item->GetFragment<UMythicGemFragment>()) {
                Gems.Add(Item);
            }
        }
    }

    int32 UsedRows = 0;
    for (UMythicItemInstance *Host : Hosts) {
        if (!ItemList) {
            break;
        }
        const USocketsFragment *Sockets = Host->GetFragment<USocketsFragment>();
        if (!Sockets) {
            continue;
        }
        FMythicSocketItemRow &Row = GetOrCreateItemRow(UsedRows++);
        Row.Box->SetVisibility(ESlateVisibility::Visible);

        const UItemDefinition *Def = Host->GetItemDefinition();
        Row.ItemName->SetText(Def ? Def->Name : FText::GetEmpty());

        Row.SocketSummary->SetText(FText::Format(NSLOCTEXT("Mythic", "SocketsFilled", "{0} of {1} sockets set"),
                                                 FText::AsNumber(Sockets->GetFilledSocketCount()),
                                                 FText::AsNumber(Sockets->GetSocketCount())));
        Row.SocketSummary->SetColorAndOpacity(FSlateColor(FMythicUIStyle::Get().InkSubtle));

        for (int32 i = 0; i < Row.Chips.Num(); ++i) {
            FMythicSocketChip &Chip = Row.Chips[i];
            if (!Sockets->Sockets.IsValidIndex(i)) {
                Chip.Button->SetVisibility(ESlateVisibility::Collapsed);
                continue;
            }
            const FMythicSocketSlot &SocketSlot = Sockets->Sockets[i];
            Chip.Proxy->HostItem = Host;
            Chip.Proxy->SocketIndex = i;
            Chip.Button->SetVisibility(ESlateVisibility::Visible);

            if (SocketSlot.bFilled) {
                Chip.Label->SetText(ShortGemName(SocketSlot.SocketedGemType));
                Chip.Label->SetColorAndOpacity(FSlateColor(FMythicUIStyle::Get().Ink));
            }
            else {
                Chip.Label->SetText(SocketSlot.SocketColor.IsValid()
                                        ? FText::Format(NSLOCTEXT("Mythic", "SocketOnly", "{0} only"),
                                                        ShortGemName(SocketSlot.SocketColor))
                                        : NSLOCTEXT("Mythic", "SocketEmptySlot", "Empty"));
                Chip.Label->SetColorAndOpacity(FSlateColor(FMythicUIStyle::Get().InkSubtle));
            }
        }
    }
    for (int32 i = UsedRows; i < ItemPool.Num(); ++i) {
        ItemPool[i].Box->SetVisibility(ESlateVisibility::Collapsed);
        for (FMythicSocketChip &Chip : ItemPool[i].Chips) {
            Chip.Proxy->HostItem = nullptr;
        }
    }

    int32 UsedGems = 0;
    for (UMythicItemInstance *Gem : Gems) {
        if (!GemList) {
            break;
        }
        FMythicGemRow &Row = GetOrCreateGemRow(UsedGems++);
        Row.Proxy->Gem = Gem;
        Row.Button->SetVisibility(ESlateVisibility::Visible);

        const UItemDefinition *Def = Gem->GetItemDefinition();
        Row.Label->SetText(Def ? Def->Name : FText::GetEmpty());
        const bool bSelected = SelectedGem.Get() == Gem;
        Row.Label->SetColorAndOpacity(FSlateColor(bSelected ? FMythicUIStyle::Get().Caution
                                                            : FMythicUIStyle::Get().Ink));
    }
    for (int32 i = UsedGems; i < GemPool.Num(); ++i) {
        GemPool[i].Button->SetVisibility(ESlateVisibility::Collapsed);
        GemPool[i].Proxy->Gem = nullptr;
    }

    if (Txt_NoItems) {
        Txt_NoItems->SetText(NSLOCTEXT("Mythic", "NoSocketedGear",
                                       "Nothing you are wearing has sockets. Socketed gear comes from chests and the stronger foes."));
        Txt_NoItems->SetVisibility(UsedRows == 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
    FMythicUIStyle::ShowEmptyState(this, TEXT("EmptyState_Items"), UsedRows == 0);
    if (Txt_NoGems) {
        Txt_NoGems->SetText(NSLOCTEXT("Mythic", "NoGems", "You are carrying no gems. Gems drop from ore veins and chests."));
        Txt_NoGems->SetVisibility(UsedGems == 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
    FMythicUIStyle::ShowEmptyState(this, TEXT("EmptyState_Gems"), UsedGems == 0);
    if (Txt_Hint) {
        Txt_Hint->SetText(SelectedGem.IsValid()
                              ? NSLOCTEXT("Mythic", "SocketHintPlace", "Now choose an empty socket.")
                              : NSLOCTEXT("Mythic", "SocketHintPick",
                                          "Choose a gem, then an empty socket. Click a set socket to take the gem out."));
        Txt_Hint->SetVisibility((UsedRows == 0 && UsedGems == 0) ? ESlateVisibility::Collapsed
                                                                  : ESlateVisibility::HitTestInvisible);
    }
}


void UMythicSocketPanelWidget::SelectGem(UMythicItemInstance *Gem) {
    SelectedGem = (SelectedGem.Get() == Gem) ? nullptr : Gem;
    Refresh();
}

void UMythicSocketPanelWidget::ActivateSocket(UMythicItemInstance *HostItem, int32 SocketIndex) {
    if (!HostItem || SocketIndex == INDEX_NONE) {
        return;
    }
    UMythicSocketComponent *Socketer = GetSocketComponent();
    if (!Socketer) {
        return;
    }

    const USocketsFragment *Sockets = HostItem->GetFragment<USocketsFragment>();
    if (!Sockets || !Sockets->Sockets.IsValidIndex(SocketIndex)) {
        return;
    }

    if (Sockets->Sockets[SocketIndex].bFilled) {
        Socketer->ServerUnsocketGem(HostItem, SocketIndex);
    }
    else if (UMythicItemInstance *Gem = SelectedGem.Get()) {
        Socketer->ServerSocketGem(HostItem, SocketIndex, Gem);
        SelectedGem = nullptr;
    }
    else {
        if (Txt_Hint) {
            Txt_Hint->SetText(NSLOCTEXT("Mythic", "SocketNeedGem", "Choose a gem first."));
        }
        return;
    }

    RequestRefresh();
}
