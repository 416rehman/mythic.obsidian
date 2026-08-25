// Copyright Stellar Games. All Rights Reserved.

#include "UI/Inventory/MythicGemPickerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/RichTextBlock.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Itemization/Inventory/Fragments/Passive/MythicGemFragment.h"
#include "Itemization/Inventory/Fragments/Passive/SocketsFragment.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Sockets/MythicSocketComponent.h"
#include "Itemization/Sockets/MythicSocketTypes.h"
#include "Mythic/Mythic.h"
#include "Player/MythicPlayerController.h"
#include "UI/Inventory/MythicSocketRowWidget.h"
#include "UI/MythicUIManagerSubsystem.h"
#include "UI/MythicUIStyle.h"
#include "UI/ViewModels/MythicEffectDescriber.h"
#include "UI/ViewModels/MythicStatTextLibrary.h"

namespace {
FText GemLeafName(const FGameplayTag &GemType) {
    if (!GemType.IsValid()) {
        return FText::GetEmpty();
    }
    const FString Full = GemType.ToString();
    int32 Dot = INDEX_NONE;
    return FText::FromString(Full.FindLastChar(TEXT('.'), Dot) ? Full.RightChop(Dot + 1) : Full);
}

FText DescribeGem(const UMythicGemFragment *Gem, int32 ItemLevel, bool bRich) {
    TArray<FText> Lines;
    Lines.Reserve(Gem->GrantedAffixes.Num());
    for (const FRolledAffix &Affix : Gem->GrantedAffixes) {
        const FMythicEffectLine Line =
            MythicEffectDescriber::DescribeRolledModifier(Affix.Attribute, Affix.Value, Affix.Definition, ItemLevel);
        Lines.Add(bRich ? Line.RichText
                        : FText::Format(NSLOCTEXT("Mythic", "GemPickerAffix", "{0} {1}"), Line.Value, Line.Label));
    }
    return FText::Join(FText::FromString(TEXT("   ")), Lines);
}
}

void UMythicGemPickerRowProxy::HandleClicked() {
    if (UMythicGemPickerWidget *P = Picker.Get()) {
        P->SocketRow(RowIndex);
    }
}

void UMythicGemPickerWidget::NativeOnInitialized() {
    Super::NativeOnInitialized();
    // At Initialize, not Construct: the picker is not in a tree until it opens, so anything left to
    // NativeConstruct would build its pool on the first click.
    BuildRows(PrewarmRows);
}

void UMythicGemPickerWidget::NativeOnDeactivated() {
    bOnLayer = false;
    Super::NativeOnDeactivated();
}

UWidget *UMythicGemPickerWidget::NativeGetDesiredFocusTarget() const {
    if (UWidget *First = FMythicUIStyle::FindFirstFocusable(const_cast<UMythicGemPickerWidget *>(this))) {
        return First;
    }
    return Super::NativeGetDesiredFocusTarget();
}

FGameplayTag UMythicGemPickerWidget::GetGemType(UMythicItemInstance *Item) {
    if (!Item) {
        return FGameplayTag();
    }
    const UMythicGemFragment *Gem = Item->GetFragment<UMythicGemFragment>();
    // The server refuses a gem with no granted affixes, so the list must not offer one.
    return (Gem && Gem->IsGem()) ? Gem->GetGemType() : FGameplayTag();
}

void UMythicGemPickerWidget::CollectGems(const APlayerController *PC, TArray<UMythicItemInstance *> &OutGems) {
    OutGems.Reset();

    const AMythicPlayerController *Owner = Cast<AMythicPlayerController>(PC);
    if (!Owner) {
        return;
    }
    for (UMythicInventoryComponent *Inv : Owner->GetAllInventoryComponents()) {
        if (!Inv) {
            continue;
        }
        for (const FMythicInventorySlotEntry &Entry : Inv->GetAllSlots()) {
            UMythicItemInstance *Item = Entry.SlottedItemInstance;
            if (Item && GetGemType(Item).IsValid()) {
                OutGems.Add(Item);
            }
        }
    }
}

void UMythicGemPickerWidget::BuildRows(int32 Count) {
    for (int32 i = 0; i < Count; ++i) {
        GetOrCreateRow(i);
    }
}

FMythicGemPickerRow &UMythicGemPickerWidget::GetOrCreateRow(int32 Index) {
    if (Rows.IsValidIndex(Index)) {
        return Rows[Index];
    }

    FMythicGemPickerRow Row;
    if (RowHost && RowClass && GetOwningPlayer()) {
        Row.Widget = CreateWidget<UUserWidget>(GetOwningPlayer(), RowClass);
    }
    if (Row.Widget) {
        Row.Proxy = NewObject<UMythicGemPickerRowProxy>(this);
        Row.Proxy->Picker = this;
        Row.Proxy->RowIndex = Rows.Num();

        // Any button in the row drives the row, so a designer can lay the row out however it reads best.
        TArray<UWidget *> Children;
        Row.Widget->WidgetTree->GetAllWidgets(Children);
        for (UWidget *Child : Children) {
            if (UButton *Button = Cast<UButton>(Child)) {
                Button->OnClicked.AddDynamic(Row.Proxy, &UMythicGemPickerRowProxy::HandleClicked);
                break;
            }
        }

        Row.Widget->SetVisibility(ESlateVisibility::Collapsed);
        RowHost->AddChild(Row.Widget);
    }

    Rows.Add(Row);
    return Rows.Last();
}

void UMythicGemPickerWidget::OpenForSocket(UMythicItemInstance *HostItem, int32 InSocketIndex,
                                           UMythicSocketRowWidget *InOpener) {
    Host = HostItem;
    SocketIndex = InSocketIndex;
    Opener = InOpener;
    SocketColor = FGameplayTag();

    const USocketsFragment *Sockets = HostItem ? HostItem->GetFragment<USocketsFragment>() : nullptr;
    if (!Sockets || !Sockets->Sockets.IsValidIndex(SocketIndex)) {
        UE_LOG(Myth, Warning, TEXT("GemPicker: socket %d does not exist on the item."), SocketIndex);
        return;
    }
    SocketColor = Sockets->Sockets[SocketIndex].SocketColor;

    if (SlotLabel) {
        SlotLabel->SetText(SocketColor.IsValid()
                               ? FText::Format(NSLOCTEXT("Mythic", "GemPickerSlotColour", "Socket {0} - {1} only"),
                                               FText::AsNumber(SocketIndex + 1), GemLeafName(SocketColor))
                               : FText::Format(NSLOCTEXT("Mythic", "GemPickerSlot", "Socket {0}"),
                                               FText::AsNumber(SocketIndex + 1)));
    }

    TArray<UMythicItemInstance *> Carried;
    CollectGems(GetOwningPlayer(), Carried);

    TArray<UMythicItemInstance *> Fitting;
    for (UMythicItemInstance *Gem : Carried) {
        if (FMythicSocketMath::IsGemCompatible(GetGemType(Gem), SocketColor)) {
            Fitting.Add(Gem);
        }
    }

    BuildRows(Fitting.Num());

    for (int32 i = 0; i < Rows.Num(); ++i) {
        FMythicGemPickerRow &Row = Rows[i];
        if (!Row.Widget) {
            continue;
        }
        if (!Fitting.IsValidIndex(i)) {
            Row.Widget->SetVisibility(ESlateVisibility::Collapsed);
            Row.Gem = nullptr;
            continue;
        }

        UMythicItemInstance *Gem = Fitting[i];
        Row.Gem = Gem;
        // The row's button takes the hit, not the row.
        Row.Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

        const UItemDefinition *Def = Gem->GetItemDefinition();
        if (UTextBlock *NameText = Cast<UTextBlock>(Row.Widget->GetWidgetFromName(RowNameText))) {
            NameText->SetText(Def ? Def->Name : GemLeafName(GetGemType(Gem)));
        }

        if (const UMythicGemFragment *Frag = Gem->GetFragment<UMythicGemFragment>()) {
            UWidget *Desc = Row.Widget->GetWidgetFromName(RowDescriptionText);
            if (URichTextBlock *Rich = Cast<URichTextBlock>(Desc)) {
                Rich->SetText(DescribeGem(Frag, Gem->GetItemLevel(), true));
            }
            else if (UTextBlock *Plain = Cast<UTextBlock>(Desc)) {
                Plain->SetText(DescribeGem(Frag, Gem->GetItemLevel(), false));
            }
        }

        if (UTextBlock *CountText = Cast<UTextBlock>(Row.Widget->GetWidgetFromName(RowCountText))) {
            const FText Count = UMythicStatTextLibrary::FormatStackCount(Gem->GetStacks());
            CountText->SetText(Count);
            CountText->SetVisibility(Count.IsEmpty() ? ESlateVisibility::Collapsed
                                                     : ESlateVisibility::HitTestInvisible);
        }

        if (UImage *Icon = Cast<UImage>(Row.Widget->GetWidgetFromName(RowIconImage))) {
            if (UTexture2D *Texture = (Def && !Def->Icon2d.IsNull()) ? Def->Icon2d.LoadSynchronous() : nullptr) {
                Icon->SetBrushFromTexture(Texture, true);
                Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
            }
            else {
                Icon->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
    }

    // A list with no rows and no reason is a dead end: say which gem the socket wants.
    const bool bEmpty = Fitting.Num() == 0;
    if (Txt_Empty) {
        Txt_Empty->SetText(
            Carried.Num() == 0
                ? NSLOCTEXT("Mythic", "GemPickerNoGems",
                            "You are carrying no gems. Gems drop from ore veins and chests.")
                : FText::Format(NSLOCTEXT("Mythic", "GemPickerNoneFit",
                                          "This socket takes {0} gems. None of the gems you carry fit it."),
                                GemLeafName(SocketColor)));
        Txt_Empty->SetVisibility(bEmpty ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
    if (EmptyState) {
        EmptyState->SetVisibility(bEmpty ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }

    ShowOnLayer();
    if (!IsActivated()) {
        ActivateWidget();
    }
}

void UMythicGemPickerWidget::SocketRow(int32 RowIndex) {
    if (!Rows.IsValidIndex(RowIndex)) {
        return;
    }
    UMythicItemInstance *Gem = Rows[RowIndex].Gem.Get();
    UMythicItemInstance *HostItem = Host.Get();
    if (!Gem || !HostItem) {
        return;
    }

    const APawn *Pawn = GetOwningPlayer() ? GetOwningPlayer()->GetPawn() : nullptr;
    UMythicSocketComponent *Socketer = Pawn ? Pawn->FindComponentByClass<UMythicSocketComponent>() : nullptr;
    if (!Socketer) {
        UE_LOG(Myth, Warning, TEXT("GemPicker: the pawn carries no socket component to commit through."));
        return;
    }

    // The server re-runs every socketing rule; this call is a request, not the decision.
    Socketer->ServerSocketGem(HostItem, SocketIndex, Gem);

    if (Opener) {
        Opener->NotifySocketsChanged();
    }
    Close();
}

void UMythicGemPickerWidget::Close() {
    if (bOnLayer) {
        if (UGameInstance *GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr) {
            if (UMythicUIManagerSubsystem *UIManager = GI->GetSubsystem<UMythicUIManagerSubsystem>()) {
                UIManager->RemoveWidgetInstanceFromLayer(PickerLayerTag, GetOwningPlayer(), this);
            }
        }
        bOnLayer = false;
        return;
    }
    DeactivateWidget();
}

void UMythicGemPickerWidget::ShowOnLayer() {
    if (bOnLayer || !PickerLayerTag.IsValid()) {
        return;
    }
    UGameInstance *GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    UMythicUIManagerSubsystem *UIManager = GI ? GI->GetSubsystem<UMythicUIManagerSubsystem>() : nullptr;
    if (!UIManager) {
        return;
    }
    UIManager->AddWidgetInstanceToLayer(PickerLayerTag, GetOwningPlayer(), this);
    bOnLayer = true;
}
