// Copyright Stellar Games. All Rights Reserved.

#include "UI/Menu/MythicRuneSocketWidget.h"

#include "Animation/WidgetAnimation.h"
#include "CommonButtonBase.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Input/Events.h"
#include "UI/MythicUIKit.h"
#include "UI/MythicUIStyle.h"

void UMythicRuneSocketWidget::NativeOnInitialized() {
    Super::NativeOnInitialized();

    FMythicUIStyle::WireFocusRings(this);

    UTexture2D *WellArt = WellTexture.IsNull() ? nullptr : WellTexture.LoadSynchronous();
    if (Well && WellArt) {
        Well->SetBrushFromTexture(WellArt, false);
    }
    else if (const UMythicUIKit *Kit = UMythicUIKit::Get()) {
        if (Well) {
            Well->SetBrush(Kit->MakeBrush(WellKitId, EMythicUIState::Normal, WellSize));
        }
    }
    LoadedBezelIron = BezelIron.IsNull() ? nullptr : BezelIron.LoadSynchronous();
    LoadedBezelGold = BezelGold.IsNull() ? nullptr : BezelGold.LoadSynchronous();
    if (Seal && !SealTexture.IsNull()) {
        if (UTexture2D *Chains = SealTexture.LoadSynchronous()) {
            Seal->SetBrushFromTexture(Chains, false);
        }
    }
    if (Glow && !GlowTexture.IsNull()) {
        if (UTexture2D *Ring = GlowTexture.LoadSynchronous()) {
            Glow->SetBrushFromTexture(Ring, false);
        }
    }
    ApplyBezel();

    if (Hit) {
        Hit->OnClicked().AddWeakLambda(this, [this]() { OnPressed.Broadcast(SlotIndex); });
        Hit->OnHovered().AddWeakLambda(this, [this]() {
            SetHoverVisual(true);
            OnHoverChanged.Broadcast(SlotIndex, true);
        });
        Hit->OnUnhovered().AddWeakLambda(this, [this]() {
            SetHoverVisual(false);
            OnHoverChanged.Broadcast(SlotIndex, false);
        });
        // The ring is this widget's child, not the button's, so WireFocusRings cannot find it; the hit lights it here.
        Hit->OnFocusReceived().AddWeakLambda(this, [this]() {
            bFocused = true;
            ApplyBezel();
            ApplyFocusRing();
            OnFocusChanged.Broadcast(SlotIndex, true);
        });
        Hit->OnFocusLost().AddWeakLambda(this, [this]() {
            bFocused = false;
            ApplyBezel();
            ApplyFocusRing();
            OnFocusChanged.Broadcast(SlotIndex, false);
        });
    }

    for (UWidgetAnimation *Anim : {Land.Get(), Unland.Get(), Refuse.Get(), Unseal.Get()}) {
        if (Anim) {
            FWidgetAnimationDynamicEvent Finished;
            Finished.BindDynamic(this, &UMythicRuneSocketWidget::HandleAnimationFinished);
            BindToAnimationFinished(Anim, Finished);
        }
    }

    SetState(CurrentState, nullptr, FLinearColor::White);
}

FReply UMythicRuneSocketWidget::NativeOnMouseButtonDown(const FGeometry &InGeometry, const FPointerEvent &InMouseEvent) {
    if (bInteractive && InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton) {
        OnAltPressed.Broadcast(SlotIndex);
        return FReply::Handled();
    }
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UMythicRuneSocketWidget::SetState(EMythicRuneSocketState State, UTexture2D *Icon, FLinearColor Tint) {
    CurrentState = State;
    CategoryTint = Tint;
    const bool bSealed = State == EMythicRuneSocketState::Sealed;
    const FLinearColor Dim(SealedMarkDim, SealedMarkDim, SealedMarkDim, 1.0f);

    if (Mark) {
        if (Icon) {
            Mark->SetBrushFromTexture(Icon, false);
            Mark->SetColorAndOpacity(bSealed ? Dim : FLinearColor::White);
            Mark->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else {
            Mark->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
    // Refreshes the glow and the ring together, so a socket that gains or loses a rune while chosen re-reads
    // its colour from the new category instead of keeping the last one.
    ApplyFocusRing();
    if (Well) {
        Well->SetColorAndOpacity(bSealed ? Dim : FLinearColor::White);
    }
    if (Seal) {
        Seal->SetVisibility(bSealed ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
    // A socket that changed rune while chosen re-reads its ring colour from the new category.
    ApplyBezel();
}

void UMythicRuneSocketWidget::SetSelected(bool bInSelected) {
    bSelected = bInSelected;
    ApplyBezel();
}

void UMythicRuneSocketWidget::ApplyBezel() {
    if (!Bezel) {
        return;
    }
    // Iron in every state. A chosen socket says so by wearing the rune's category colour rather than turning
    // brass, so the ring answers "which socket" and "which kind of rune" at once.
    UTexture2D *Wanted = LoadedBezelIron ? LoadedBezelIron.Get() : LoadedBezelGold.Get();
    if (Wanted) {
        Bezel->SetBrushFromTexture(Wanted, false);
    }

    const bool bChosen = bSelected || bFocused;
    const bool bEmpty = CurrentState != EMythicRuneSocketState::Filled;
    FLinearColor Ring = FLinearColor::White;
    if (bChosen && !bEmpty) {
        Ring = FMath::Lerp(FLinearColor::White, CategoryTint, FMath::Clamp(SelectedRingCategoryLean, 0.0f, 1.0f));
        Ring.A = 1.0f;
    }
    Bezel->SetColorAndOpacity(Ring);
}

void UMythicRuneSocketWidget::SetInteractive(bool bInInteractive) {
    bInteractive = bInInteractive;
    if (Hit) {
        Hit->SetVisibility(bInteractive ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}

void UMythicRuneSocketWidget::SetHoverVisual(bool bInHovered) {
    bHovered = bInHovered;
    SetRenderScale(FVector2D(bHovered ? 1.06f : 1.0f));
    ApplyFocusRing();
}

void UMythicRuneSocketWidget::ApplyGlow() {
    if (!Glow) {
        return;
    }
    // Nothing glows at rest. A filled socket blooms in its rune's category colour only under cursor or focus.
    const bool bLit = (bFocused || bHovered) && CurrentState == EMythicRuneSocketState::Filled;
    FLinearColor Lit = CategoryTint;
    Lit.A = bLit ? HoverGlowAlpha : 0.0f;
    Glow->SetColorAndOpacity(Lit);
    Glow->SetVisibility(bLit ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UMythicRuneSocketWidget::ApplyFocusRing() {
    ApplyGlow();
    if (!FocusRing) {
        return;
    }
    if (UImage *Ring = Cast<UImage>(FocusRing)) {
        // The ring wears the rune's category colour; an empty socket keeps it neutral.
        FLinearColor Colour = CurrentState == EMythicRuneSocketState::Filled ? CategoryTint : FLinearColor::White;
        Colour.A = HoverGlowAlpha;
        Ring->SetColorAndOpacity(Colour);
    }
    if (bFocused || bHovered) {
        // Unseal keys the ring's opacity down to zero and leaves it there.
        FocusRing->SetRenderOpacity(1.0f);
        FocusRing->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
    else if (!(Unseal && IsAnimationPlaying(Unseal))) {
        FocusRing->SetVisibility(ESlateVisibility::Collapsed);
    }
}

UWidget *UMythicRuneSocketWidget::GetFocusWidget() const {
    return Hit;
}

void UMythicRuneSocketWidget::Play(UWidgetAnimation *Anim) {
    if (!Anim) {
        return;
    }
    // Volatile only while something moves; a still socket is cached like any other.
    ForceVolatile(true);
    PlayAnimation(Anim);
}

void UMythicRuneSocketWidget::HandleAnimationFinished() {
    for (const UWidgetAnimation *Anim : {Land.Get(), Unland.Get(), Refuse.Get(), Unseal.Get()}) {
        if (Anim && IsAnimationPlaying(Anim)) {
            return;
        }
    }
    if (FocusRing && !bFocused && !bHovered) {
        FocusRing->SetVisibility(ESlateVisibility::Collapsed);
    }
    ForceVolatile(false);
}

void UMythicRuneSocketWidget::PlayLand() {
    Play(Land);
}

void UMythicRuneSocketWidget::PlayUnland() {
    Play(Unland);
}

void UMythicRuneSocketWidget::PlayRefuse() {
    Play(Refuse);
}

void UMythicRuneSocketWidget::PlayUnseal() {
    // Unseal flashes the ring by opacity, and a Collapsed widget animates to nothing.
    if (FocusRing && Unseal) {
        FocusRing->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
    Play(Unseal);
}
