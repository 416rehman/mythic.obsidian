// Copyright Stellar Games. All Rights Reserved.

#include "MythicRuneHudCellWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace {
const FName RuneCell_Progress(TEXT("Progress"));
}

void UMythicRuneHudCellWidget::NativeOnInitialized() {
    Super::NativeOnInitialized();
    for (UWidget *Child : {static_cast<UWidget *>(Icon.Get()), static_cast<UWidget *>(Radial.Get()), static_cast<UWidget *>(Stacks.Get())}) {
        if (Child) {
            Child->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
    if (Radial) {
        RadialMaterial = Radial->GetDynamicMaterial();
    }
}

const FLinearColor &UMythicRuneHudCellWidget::TintFor(EMythicRuneHudState State) const {
    switch (State) {
    case EMythicRuneHudState::Cooldown:
        return CooldownTint;
    case EMythicRuneHudState::Active:
        return ActiveTint;
    default:
        return ReadyTint;
    }
}

void UMythicRuneHudCellWidget::SetEntry(const FMythicRuneBadgeEntry &Entry) {
    if (Icon) {
        const FSoftObjectPath IconPath = Entry.Icon.ToSoftObjectPath();
        if (IconPath != ShownIcon) {
            ShownIcon = IconPath;
            // An empty brush paints a white quad, so a rune with no art shows nothing rather than a blank plate.
            if (IconPath.IsNull()) {
                Icon->SetVisibility(ESlateVisibility::Collapsed);
            }
            else {
                Icon->SetBrushFromSoftTexture(Entry.Icon, false);
                Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
            }
        }
        if (ShownState != Entry.State) {
            Icon->SetColorAndOpacity(TintFor(Entry.State));
        }
    }
    ShownState = Entry.State;

    bTimed = Entry.EndTimeSeconds > 0.0;
    if (Radial) {
        Radial->SetVisibility(bTimed ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }

    if (Stacks && ShownStacks != Entry.Stacks) {
        ShownStacks = Entry.Stacks;
        if (Entry.Stacks > 0) {
            Stacks->SetText(FText::AsNumber(Entry.Stacks));
            Stacks->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else {
            Stacks->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void UMythicRuneHudCellWidget::SetProgress(float Progress) {
    if (!bTimed) {
        return;
    }
    if (!RadialMaterial && Radial) {
        RadialMaterial = Radial->GetDynamicMaterial();
    }
    if (RadialMaterial) {
        RadialMaterial->SetScalarParameterValue(RuneCell_Progress, FMath::Clamp(Progress, 0.0f, 1.0f));
    }
}

void UMythicRuneHudCellWidget::Clear() {
    ShownIcon.Reset();
    ShownState = EMythicRuneHudState::Hidden;
    ShownStacks = INDEX_NONE;
    bTimed = false;
    SetTicking(false);
    for (UWidget *Child : {static_cast<UWidget *>(Icon.Get()), static_cast<UWidget *>(Radial.Get()), static_cast<UWidget *>(Stacks.Get())}) {
        if (Child) {
            Child->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void UMythicRuneHudCellWidget::SetTicking(bool bTicking) {
    if (Radial) {
        Radial->ForceVolatile(bTicking && bTimed);
    }
}
