#include "UI/Nameplate/MythicNameplateActionRailWidget.h"

#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "UI/Nameplate/MythicNameplateVisualStyle.h"
#include "UI/Widgets/MythicInputGlyph.h"

void UMythicNameplateActionRailWidget::NativeOnInitialized() {
    Super::NativeOnInitialized();
    Projection.Actions.Reserve(2);
    SetVisibility(ESlateVisibility::Collapsed);
}

void UMythicNameplateActionRailWidget::ApplyProjection(
    const FMythicNameplateActionRailProjection &InProjection) {
    if (!InProjection.IsPresentable()) {
        ResetForPool();
        return;
    }

    Projection.Instance = InProjection.Instance;
    Projection.Actions.Reset();
    for (const FMythicNameplateActionProjection &Action :
         InProjection.Actions) {
        if (Projection.Actions.Num() >= 2) {
            break;
        }
        if (Action.InputActionTag.IsValid()
            && !Action.ResolvedLabel.IsEmpty()) {
            Projection.Actions.Add(Action);
        }
    }
    Projection.bInspectAvailable = InProjection.bInspectAvailable
        && InProjection.InspectInputActionTag.IsValid()
        && !InProjection.ResolvedInspectLabel.IsEmpty();
    Projection.InspectInputActionTag = Projection.bInspectAvailable
        ? InProjection.InspectInputActionTag : FGameplayTag();
    Projection.ResolvedInspectLabel = Projection.bInspectAvailable
        ? InProjection.ResolvedInspectLabel : FText::GetEmpty();
    if (Projection.Actions.IsEmpty()
        && !Projection.bInspectAvailable) {
        ResetForPool();
        return;
    }
    const FMythicNameplateActionProjection *Primary =
        Projection.Actions.IsEmpty() ? nullptr : &Projection.Actions[0];
    ApplyEntry(PrimaryGlyph, PrimaryActionText,
               Primary ? Primary->InputActionTag : FGameplayTag(),
               Primary ? Primary->ResolvedLabel : FText::GetEmpty());

    const FMythicNameplateActionProjection *Secondary = nullptr;
    if (!Projection.bInspectAvailable && Projection.Actions.Num() > 1) {
        Secondary = &Projection.Actions[1];
    }
    if (Projection.bInspectAvailable) {
        ApplyEntry(SecondaryGlyph, SecondaryActionText,
                   Projection.InspectInputActionTag,
                   Projection.ResolvedInspectLabel);
    } else {
        ApplyEntry(SecondaryGlyph, SecondaryActionText,
                   Secondary ? Secondary->InputActionTag : FGameplayTag(),
                   Secondary ? Secondary->ResolvedLabel : FText::GetEmpty());
    }

    SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    OnActionRailProjectionChanged();
}

void UMythicNameplateActionRailWidget::SetPresentationStyle(
    UMythicNameplateVisualStyle *InStyle,
    const FMythicNameplateRenderPreferences &InPreferences) {
    const bool bReducedMotionEnabled =
        !RenderPreferences.bReducedMotion
        && InPreferences.bReducedMotion;
    VisualStyle = InStyle;
    RenderPreferences = InPreferences;
    RenderPreferences.Scale = FMath::Clamp(RenderPreferences.Scale,
                                            0.75f, 1.5f);
    if (bReducedMotionEnabled) {
        StopAllAnimations();
    }
    SetRenderScale(FVector2D(RenderPreferences.Scale));

    for (UTextBlock *Text : {PrimaryActionText.Get(),
                             SecondaryActionText.Get()}) {
        if (Text) {
            Text->SetAutoWrapText(false);
            Text->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
        }
    }
    if (!VisualStyle) {
        return;
    }

    if (RailSizeBox) {
        RailSizeBox->SetMaxDesiredWidth(
            VisualStyle->ActionRailMaximumSize.X);
        RailSizeBox->SetMaxDesiredHeight(
            VisualStyle->ActionRailMaximumSize.Y);
    }
    if (RailSurface) {
        // Keep the focused action prompt visually attached to the world, not
        // rendered as a second panel beneath the nameplate.
        RailSurface->SetBrushColor(FLinearColor::Transparent);
    }
    FSlateFontInfo ActionFont = VisualStyle->SecondaryFont;
    ActionFont.Size = 13;
    for (UTextBlock *Text : {PrimaryActionText.Get(),
                             SecondaryActionText.Get()}) {
        if (Text) {
            Text->SetFont(ActionFont);
            Text->SetColorAndOpacity(
                FSlateColor(VisualStyle->PrimaryTextColor));
        }
    }
    for (UMythicInputGlyph *Glyph : {PrimaryGlyph.Get(),
                                     SecondaryGlyph.Get()}) {
        if (Glyph) {
            Glyph->GlyphHeight = 18.0f;
        }
    }
}

void UMythicNameplateActionRailWidget::ResetForPool() {
    Projection.Instance.Reset();
    Projection.Actions.Reset();
    Projection.bInspectAvailable = false;
    Projection.InspectInputActionTag = FGameplayTag();
    Projection.ResolvedInspectLabel = FText::GetEmpty();
    ResetEntry(PrimaryGlyph, PrimaryActionText);
    ResetEntry(SecondaryGlyph, SecondaryActionText);
    StopAllAnimations();
    SetRenderTranslation(FVector2D::ZeroVector);
    SetRenderOpacity(0.0f);
    OnActionRailReleased();
    SetVisibility(ESlateVisibility::Collapsed);
}

void UMythicNameplateActionRailWidget::ApplyEntry(
    UMythicInputGlyph *Glyph, UTextBlock *Text,
    const FGameplayTag &InputTag, const FText &Label) {
    const bool bVisible = InputTag.IsValid() && !Label.IsEmpty();
    if (Glyph) {
        Glyph->SetActionTag(bVisible ? InputTag : FGameplayTag());
        Glyph->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible
                                      : ESlateVisibility::Collapsed);
    }
    if (Text) {
        Text->SetText(bVisible ? Label : FText::GetEmpty());
        Text->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible
                                     : ESlateVisibility::Collapsed);
    }
}

void UMythicNameplateActionRailWidget::ResetEntry(
    UMythicInputGlyph *Glyph, UTextBlock *Text) {
    ApplyEntry(Glyph, Text, FGameplayTag(), FText::GetEmpty());
}
