// Copyright Stellar Games. All Rights Reserved.

#include "UI/Menu/MythicRunePickerCellWidget.h"

#include "Animation/WidgetAnimation.h"
#include "CommonButtonBase.h"
#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "UI/MythicUIKit.h"
#include "UI/MythicUIStyle.h"

void UMythicRunePickerCellWidget::NativeOnInitialized() {
    Super::NativeOnInitialized();

    FMythicUIStyle::WireFocusRings(this);
    FMythicUIStyle::ApplyTextStyle(NameText, EMythicTextRole::Heading);

    if (Socket) {
        Socket->SetInteractive(false);
    }

    if (Hit) {
        Hit->OnClicked().AddWeakLambda(this, [this]() { OnPressed.Broadcast(CellIndex); });
        Hit->OnHovered().AddWeakLambda(this, [this]() {
            bHovered = true;
            ApplyBacking();
            if (Socket) {
                Socket->SetHoverVisual(true);
            }
            Play(HoverAnim);
            OnHoverChanged.Broadcast(CellIndex, true);
        });
        Hit->OnUnhovered().AddWeakLambda(this, [this]() {
            bHovered = false;
            bPressed = false;
            ApplyBacking();
            if (Socket) {
                Socket->SetHoverVisual(false);
            }
            OnHoverChanged.Broadcast(CellIndex, false);
        });
        Hit->OnPressed().AddWeakLambda(this, [this]() {
            bPressed = true;
            ApplyBacking();
        });
        Hit->OnReleased().AddWeakLambda(this, [this]() {
            bPressed = false;
            ApplyBacking();
        });
        // The ring is this widget's child, not the button's, so WireFocusRings cannot find it; the hit lights it here.
        Hit->OnFocusReceived().AddWeakLambda(this, [this]() {
            bFocused = true;
            ApplySocketSelection();
            if (FocusRing) {
                FocusRing->SetVisibility(ESlateVisibility::HitTestInvisible);
            }
            Play(HoverAnim);
            OnFocusChanged.Broadcast(CellIndex, true);
        });
        Hit->OnFocusLost().AddWeakLambda(this, [this]() {
            bFocused = false;
            ApplySocketSelection();
            if (FocusRing) {
                FocusRing->SetVisibility(ESlateVisibility::Collapsed);
            }
            OnFocusChanged.Broadcast(CellIndex, false);
        });
    }

    for (UWidgetAnimation *Anim : {HoverAnim.Get(), Refuse.Get()}) {
        if (Anim) {
            FWidgetAnimationDynamicEvent Finished;
            Finished.BindDynamic(this, &UMythicRunePickerCellWidget::HandleAnimationFinished);
            BindToAnimationFinished(Anim, Finished);
        }
    }

    ApplyBacking();
}

void UMythicRunePickerCellWidget::SetCellState(const FMythicRuneCellState &State) {
    Worn = State.Worn;
    const UMythicUIStyleSettings &S = FMythicUIStyle::Get();

    if (NameText) {
        NameText->SetText(State.Name);
    }

    if (NameText) {
        NameText->SetColorAndOpacity(!State.bUnlocked ? S.InkLabel
                                     : State.Worn == EMythicRuneWorn::Here ? S.AccentBright
                                                                            : S.Ink);
    }
    if (WornMark) {
        WornMark->SetVisibility(State.Worn == EMythicRuneWorn::Elsewhere ? ESlateVisibility::HitTestInvisible
                                                                          : ESlateVisibility::Collapsed);
    }

    if (Socket) {
        const EMythicRuneSocketState SocketState = State.bClear ? EMythicRuneSocketState::Empty
                                                   : State.bUnlocked ? EMythicRuneSocketState::Filled
                                                                     : EMythicRuneSocketState::Sealed;
        Socket->SetState(SocketState, State.Icon, State.Tint);
    }
    ApplySocketSelection();
}

void UMythicRunePickerCellWidget::ApplySocketSelection() {
    if (Socket) {
        Socket->SetSelected(bFocused || Worn == EMythicRuneWorn::Here);
    }
}

void UMythicRunePickerCellWidget::ApplyBacking() {
    if (!CellBacking) {
        return;
    }
    const UMythicUIKit *Kit = UMythicUIKit::Get();
    if (!Kit) {
        return;
    }
    const FName Id = bPressed ? BackingPressId : (bHovered ? BackingHoverId : BackingIdleId);
    // Fill/Fill under the overlay, so the brush's own size is only a hint and the kit default is enough.
    CellBacking->SetBrush(Kit->MakeBrush(Id));
}

void UMythicRunePickerCellWidget::Play(UWidgetAnimation *Anim) {
    if (!Anim) {
        return;
    }
    ForceVolatile(true);
    PlayAnimation(Anim);
}

void UMythicRunePickerCellWidget::HandleAnimationFinished() {
    for (const UWidgetAnimation *Anim : {HoverAnim.Get(), Refuse.Get()}) {
        if (Anim && IsAnimationPlaying(Anim)) {
            return;
        }
    }
    ForceVolatile(false);
}

void UMythicRunePickerCellWidget::PlayRefuse() {
    Play(Refuse);
}

UWidget *UMythicRunePickerCellWidget::GetFocusWidget() const {
    return Hit;
}
