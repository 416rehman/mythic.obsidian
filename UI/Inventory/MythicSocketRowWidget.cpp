// Copyright Stellar Games. All Rights Reserved.

#include "UI/Inventory/MythicSocketRowWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Itemization/Inventory/Fragments/Passive/SocketsFragment.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Sockets/MythicSocketTypes.h"
#include "Mythic/Mythic.h"
#include "Player/MythicPlayerController.h"
#include "TimerManager.h"
#include "UI/Inventory/MythicGemPickerWidget.h"
#include "UI/MythicUIKit.h"
#include "UI/Widgets/MythicSectionHeader.h"

void UMythicSocketWellClickProxy::HandleClicked() {
    if (UMythicSocketRowWidget *R = Row.Get()) {
        R->OpenPickerFor(SocketIndex);
    }
}

void UMythicSocketRowWidget::NativeConstruct() {
    BuildWells();

    if (!GemPicker && GemPickerClass && GetOwningLocalPlayer()) {
        GemPicker = CreateWidget<UMythicGemPickerWidget>(GetOwningPlayer(), GemPickerClass);
    }

    Super::NativeConstruct();

    BindInventories();
    RefreshWells();
}

void UMythicSocketRowWidget::NativeDestruct() {
    UnbindInventories();
    Super::NativeDestruct();
}

void UMythicSocketRowWidget::SetItem(UMythicItemInstance *Item) {
    HostItem = Item;
    // Bags a client had not replicated at construct time still get watched, and re-binding is a no-op.
    BindInventories();
    RefreshWells();
}

void UMythicSocketRowWidget::NotifySocketsChanged() {
    RequestRefresh();
}

void UMythicSocketRowWidget::BuildWells() {
    if (!WellHost || Wells.Num() > 0 || !WidgetTree) {
        return;
    }

    const UMythicUIKit *Kit = UMythicUIKit::Get();
    const int32 Count = FMath::Clamp(MaxSockets, 1, 8);

    for (int32 i = 0; i < Count; ++i) {
        FMythicSocketWell Well;

        UOverlay *Stack = WidgetTree->ConstructWidget<UOverlay>();

        // Plate first: an Overlay paints children in the order they were added, so a plate added last
        // would paint over its own mark.
        Well.Plate = WidgetTree->ConstructWidget<UImage>();
        if (Kit) {
            Well.Plate->SetBrush(Kit->MakeBrush(WellComponentId, EMythicUIState::Normal, WellSize));
        }
        Well.Plate->SetVisibility(ESlateVisibility::HitTestInvisible);
        Stack->AddChildToOverlay(Well.Plate);

        Well.Mark = WidgetTree->ConstructWidget<UImage>();
        Well.Mark->SetVisibility(ESlateVisibility::Collapsed);
        if (UOverlaySlot *MarkSlot = Cast<UOverlaySlot>(Stack->AddChildToOverlay(Well.Mark))) {
            MarkSlot->SetHorizontalAlignment(HAlign_Center);
            MarkSlot->SetVerticalAlignment(VAlign_Center);
            MarkSlot->SetPadding(FMargin(MarkInset));
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

        Well.Proxy = NewObject<UMythicSocketWellClickProxy>(this);
        Well.Proxy->Row = this;
        Well.Proxy->SocketIndex = i;
        Hit->OnClicked.AddDynamic(Well.Proxy, &UMythicSocketWellClickProxy::HandleClicked);

        Well.Root = Stack;
        Stack->SetVisibility(ESlateVisibility::Collapsed);
        WellHost->AddChild(Stack);
        Wells.Add(Well);
    }
}

const FMythicGemMark *UMythicSocketRowWidget::FindMark(const FGameplayTag &GemType) const {
    for (const FMythicGemMark &Entry : GemMarks) {
        if (Entry.GemType.IsValid() && GemType.MatchesTag(Entry.GemType)) {
            return &Entry;
        }
    }
    return nullptr;
}

void UMythicSocketRowWidget::RefreshWells() {
    UMythicItemInstance *Item = HostItem.Get();
    const USocketsFragment *Sockets = Item ? Item->GetFragment<USocketsFragment>() : nullptr;
    const int32 Count = Sockets ? Sockets->GetSocketCount() : 0;

    // An item without sockets shows no socket heading and no empty wells.
    SetVisibility(Count > 0 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
    if (Count <= 0) {
        return;
    }

    if (Count > Wells.Num()) {
        UE_LOG(Myth, Warning, TEXT("SocketRow: item has %d sockets but the row was built for %d; raise MaxSockets."),
               Count, Wells.Num());
    }

    TArray<UMythicItemInstance *> Gems;
    UMythicGemPickerWidget::CollectGems(GetOwningPlayer(), Gems);
    const UMythicUIKit *Kit = UMythicUIKit::Get();

    for (int32 i = 0; i < Wells.Num(); ++i) {
        FMythicSocketWell &Well = Wells[i];
        if (!Well.Root) {
            continue;
        }
        if (!Sockets->Sockets.IsValidIndex(i)) {
            Well.Root->SetVisibility(ESlateVisibility::Collapsed);
            Well.Proxy->SocketIndex = INDEX_NONE;
            continue;
        }

        const FMythicSocketSlot &SocketSlot = Sockets->Sockets[i];
        const bool bFits = Gems.ContainsByPredicate([&SocketSlot](UMythicItemInstance *Gem) {
            return FMythicSocketMath::IsGemCompatible(UMythicGemPickerWidget::GetGemType(Gem), SocketSlot.SocketColor);
        });
        const bool bCanFill = !SocketSlot.bFilled && bFits && GemPicker != nullptr;

        // Three states a player can tell apart at a glance: a filled well, an empty one they can fill, and
        // one nothing they carry can fill.
        if (Well.Plate && Kit) {
            const EMythicUIState PlateState = SocketSlot.bFilled ? EMythicUIState::Selected
                                              : bCanFill         ? EMythicUIState::Normal
                                                                 : EMythicUIState::Disabled;
            Well.Plate->SetBrush(Kit->MakeBrush(WellComponentId, PlateState, WellSize));
        }

        const FGameplayTag MarkTag = SocketSlot.bFilled ? SocketSlot.SocketedGemType : SocketSlot.SocketColor;
        const FMythicGemMark *Entry = MarkTag.IsValid() ? FindMark(MarkTag) : nullptr;
        UTexture2D *Icon = Entry ? Entry->Mark.LoadSynchronous() : nullptr;
        if (Well.Mark) {
            if (Icon) {
                Well.Mark->SetBrushFromTexture(Icon, true);
                FLinearColor Tint = Entry->Colour;
                Tint.A *= SocketSlot.bFilled ? 1.0f : RestrictedMarkOpacity;
                Well.Mark->SetColorAndOpacity(Tint);
                Well.Mark->SetVisibility(ESlateVisibility::HitTestInvisible);
            }
            else {
                Well.Mark->SetVisibility(ESlateVisibility::Collapsed);
            }
        }

        Well.Proxy->SocketIndex = i;
        // A well that cannot take a gem must not answer a click, or the player learns the control lies.
        // HitTestInvisible drops the hit area with it; the fillable case leaves the hit to that one child.
        Well.Root->SetVisibility(bCanFill ? ESlateVisibility::SelfHitTestInvisible
                                          : ESlateVisibility::HitTestInvisible);
        Well.Root->SetRenderOpacity(bCanFill || SocketSlot.bFilled ? 1.0f : InertWellOpacity);
    }

    const FText Trailing = FText::Format(NSLOCTEXT("Mythic", "SocketRowCount", "{0} of {1} set"),
                                         FText::AsNumber(Sockets->GetFilledSocketCount()),
                                         FText::AsNumber(Count));
    if (Header) {
        Header->SetHeader(NSLOCTEXT("Mythic", "SocketRowHeading", "Sockets"), Trailing, nullptr);
    }
    if (Txt_Label) {
        Txt_Label->SetText(FText::Format(NSLOCTEXT("Mythic", "SocketRowLabel", "Sockets - {0}"), Trailing));
    }
}

void UMythicSocketRowWidget::OpenPickerFor(int32 SocketIndex) {
    UMythicItemInstance *Item = HostItem.Get();
    if (!Item || SocketIndex == INDEX_NONE || !GemPicker) {
        return;
    }

    const USocketsFragment *Sockets = Item->GetFragment<USocketsFragment>();
    if (!Sockets || !Sockets->Sockets.IsValidIndex(SocketIndex) || Sockets->Sockets[SocketIndex].bFilled) {
        return;
    }

    GemPicker->OpenForSocket(Item, SocketIndex, this);
}

void UMythicSocketRowWidget::RequestRefresh() {
    UWorld *World = GetWorld();
    if (!World || bRefreshArmed) {
        return;
    }
    bRefreshArmed = true;
    World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]() {
        bRefreshArmed = false;
        RefreshWells();
    }));
}

void UMythicSocketRowWidget::BindInventories() {
    const AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer());
    if (!PC) {
        return;
    }
    for (UMythicInventoryComponent *Inv : PC->GetAllInventoryComponents()) {
        if (!Inv || WatchedInventories.Contains(Inv)) {
            continue;
        }
        Inv->OnSlotUpdated.AddDynamic(this, &UMythicSocketRowWidget::HandleSlotUpdated);
        WatchedInventories.Add(Inv);
    }
}

void UMythicSocketRowWidget::UnbindInventories() {
    for (const TWeakObjectPtr<UMythicInventoryComponent> &Weak : WatchedInventories) {
        if (UMythicInventoryComponent *Inv = Weak.Get()) {
            Inv->OnSlotUpdated.RemoveDynamic(this, &UMythicSocketRowWidget::HandleSlotUpdated);
        }
    }
    WatchedInventories.Reset();
    bRefreshArmed = false;
}

void UMythicSocketRowWidget::HandleSlotUpdated(int32 UpdatedSlotIndex) {
    RequestRefresh();
}
