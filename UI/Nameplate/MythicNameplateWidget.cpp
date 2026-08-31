#include "UI/Nameplate/MythicNameplateWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "UI/Nameplate/MythicNameplateViewModel.h"
#include "UI/Nameplate/MythicNameplateVisualStyle.h"
#include "UI/Widgets/MythicBarWidget.h"

#define LOCTEXT_NAMESPACE "MythicNameplateWidget"

namespace {
void ApplyOptionalText(UTextBlock *Widget, const FText &Text) {
    if (!Widget) {
        return;
    }
    Widget->SetText(Text);
    Widget->SetVisibility(Text.IsEmpty()
        ? ESlateVisibility::Collapsed
        : ESlateVisibility::SelfHitTestInvisible);
}

void ConfigureSingleLineText(UTextBlock *Widget,
                             const FSlateFontInfo &Font,
                             const FLinearColor &Color) {
    if (!Widget) {
        return;
    }
    Widget->SetFont(Font);
    Widget->SetColorAndOpacity(FSlateColor(Color));
    Widget->SetAutoWrapText(false);
    Widget->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
}

void ApplyOptionalIcon(UImage *Image, USizeBox *Bounds,
                       const FMythicNameplateIconToken *Token,
                       const float LogicalScale = 1.0f) {
    const bool bRenderable = Token && Token->IsRenderable();
    const ESlateVisibility Visibility = bRenderable
        ? ESlateVisibility::SelfHitTestInvisible
        : ESlateVisibility::Collapsed;

    if (Image) {
        Image->SetBrushFromTexture(
            bRenderable ? Token->Texture.Get() : nullptr, false);
        if (bRenderable) {
            Image->SetColorAndOpacity(Token->Tint);
        }
        Image->SetVisibility(Visibility);
    }
    if (Bounds) {
        if (bRenderable) {
            const float SafeScale = FMath::Clamp(LogicalScale,
                                                 0.5f, 2.0f);
            Bounds->SetWidthOverride(Token->LogicalSize.X * SafeScale);
            Bounds->SetHeightOverride(Token->LogicalSize.Y * SafeScale);
        }
        Bounds->SetVisibility(Visibility);
    }
}

bool IsTerminalOrRescueCue(const EMythicNameplatePrimaryCue Cue) {
    return Cue == EMythicNameplatePrimaryCue::Downed
        || Cue == EMythicNameplatePrimaryCue::Dying;
}

bool IsActionableCue(const EMythicNameplatePrimaryCue Cue) {
    return Cue == EMythicNameplatePrimaryCue::DirectedTalk
        || Cue == EMythicNameplatePrimaryCue::QuestTurnIn
        || Cue == EMythicNameplatePrimaryCue::QuestOffer
        || Cue == EMythicNameplatePrimaryCue::Service;
}

const FMythicNameplateIconToken *ResolvePrimarySemanticIcon(
    const UMythicNameplateVisualStyle *Style,
    const FMythicNameplateProjection &Projection) {
    if (!Style) {
        return nullptr;
    }

    // A plate has one emblem slot. Life-state safety, authored rank, danger,
    // and contextual meaning must not accumulate into competing symbols.
    // Viewer attention is deliberately excluded: the reticle and disclosure
    // treatment already communicate focus/target selection.
    if (IsTerminalOrRescueCue(Projection.PrimaryCue)) {
        return Style->ResolveCueIcon(Projection.PrimaryCue);
    }
    if (IsActionableCue(Projection.PrimaryCue)) {
        return Style->ResolveCueIcon(Projection.PrimaryCue);
    }
    if (const FMythicNameplateIconToken *Target =
            Style->ResolveTargetIcon(Projection.AttentionState)) {
        return Target;
    }
    if (const FMythicNameplateIconToken *Rank =
            Style->ResolveRankIcon(Projection.PresentedCombatRank)) {
        return Rank;
    }
    if (const FMythicNameplateIconToken *Threat =
            Style->ResolveThreatIcon(Projection.ThreatBand)) {
        return Threat;
    }

    // A generic action is already represented by the input glyph plus verb
    // in the action rail. Repeating an abstract interaction mark here adds no
    // player-facing meaning.
    if (Projection.PrimaryCue == EMythicNameplatePrimaryCue::OtherAction
        || Projection.PrimaryCue
            == EMythicNameplatePrimaryCue::ObservableActivity) {
        return nullptr;
    }
    return Style->ResolveCueIcon(Projection.PrimaryCue);
}
} // namespace

void UMythicNameplateWidget::NativeOnInitialized() {
    Super::NativeOnInitialized();
    PrewarmStatusBadges();
}

void UMythicNameplateWidget::NativeConstruct() {
    Super::NativeConstruct();
    // NativeOnInitialized is one-shot, but pooled widgets can reconstruct
    // after their HUD layer is removed and re-added.
    PrewarmStatusBadges();
    RefreshNativeBindings();
}

void UMythicNameplateWidget::NativeDestruct() {
    ResetStatusBadges();
    // Dynamic badge children remain owned by WidgetTree and are retained for
    // the next reconstruction of this prewarmed pool slot.
    Super::NativeDestruct();
}

void UMythicNameplateWidget::SetNameplateViewModel(
    UMythicNameplateViewModel *InViewModel) {
    ViewModel = InViewModel;
}

void UMythicNameplateWidget::SetPresentationStyle(
    UMythicNameplateVisualStyle *InVisualStyle,
    const FMythicNameplateRenderPreferences &InPreferences) {
    FMythicNameplateRenderPreferences Sanitized = InPreferences;
    Sanitized.Scale = FMath::Clamp(Sanitized.Scale, 0.75f, 1.5f);
    const bool bReducedMotionEnabled =
        !RenderPreferences.bReducedMotion && Sanitized.bReducedMotion;
    const bool bPreferencesChanged = RenderPreferences != Sanitized;
    const bool bStyleChanged = VisualStyle != InVisualStyle;
    if (!bPreferencesChanged && !bStyleChanged) {
        return;
    }

    VisualStyle = InVisualStyle;
    RenderPreferences = Sanitized;
    if (bReducedMotionEnabled) {
        StopAllAnimations();
    }
    SetRenderScale(FVector2D(RenderPreferences.Scale));
    RefreshNativeBindings();
    OnNameplateRenderPreferencesChanged();
}

void UMythicNameplateWidget::SetRenderPreferences(
    const FMythicNameplateRenderPreferences &InPreferences) {
    SetPresentationStyle(VisualStyle, InPreferences);
}

void UMythicNameplateWidget::NotifyProjectionChanged() {
    RefreshNativeBindings();
    OnNameplateProjectionChanged();
}

void UMythicNameplateWidget::ResetForPool() {
    if (ViewModel) {
        ViewModel->Reset();
    }
    ResetNativeBindings();
    SetRenderTranslation(FVector2D::ZeroVector);
    SetRenderOpacity(0.0f);
    StopAllAnimations();
    OnNameplateReleased();
    SetVisibility(ESlateVisibility::Collapsed);
}

void UMythicNameplateWidget::RefreshNativeBindings() {
    if (!ViewModel || !ViewModel->GetProjection().IsPresentable()) {
        ResetNativeBindings();
        return;
    }

    const FMythicNameplateProjection &Projection =
        ViewModel->GetProjection();
    ApplyVisualStyle(Projection);
    ApplyOptionalText(NameText, Projection.ResolvedName);
    ApplyOptionalText(SubtitleText, Projection.ResolvedSubtitle);
    ApplyOptionalIcon(SemanticIcon, SemanticIconBox,
                      ResolvePrimarySemanticIcon(VisualStyle, Projection));
    ApplyOptionalText(LevelText, Projection.ResolvedLevelText);

    if (HealthBar) {
        HealthBar->SetPercent(FMath::Clamp(Projection.HealthFraction,
                                          0.0f, 1.0f));
        HealthBar->SetVisibility(Projection.bShowHealth
            ? ESlateVisibility::SelfHitTestInvisible
            : ESlateVisibility::Collapsed);
    }
    if (HealthSizeBox) {
        HealthSizeBox->SetVisibility(Projection.bShowHealth
            ? ESlateVisibility::SelfHitTestInvisible
            : ESlateVisibility::Collapsed);
    }

    FNumberFormattingOptions PercentFormat;
    PercentFormat.SetMaximumFractionalDigits(0);
    ApplyOptionalText(
        HealthPercentText,
        Projection.bShowHealth && Projection.bHealthPercentEligible
                && RenderPreferences.bShowHealthPercent
            ? FText::AsPercent(
                  FMath::Clamp(Projection.HealthFraction, 0.0f, 1.0f),
                  &PercentFormat)
            : FText::GetEmpty());

    RefreshStatusBadges(Projection);
}

void UMythicNameplateWidget::ResetNativeBindings() {
    ApplyOptionalText(NameText, FText::GetEmpty());
    ApplyOptionalText(SubtitleText, FText::GetEmpty());
    ApplyOptionalIcon(SemanticIcon, SemanticIconBox, nullptr);
    ApplyOptionalText(LevelText, FText::GetEmpty());
    ApplyOptionalText(HealthPercentText, FText::GetEmpty());
    ApplyOptionalText(StatusText, FText::GetEmpty());
    ResetStatusBadges();
    if (HealthBar) {
        HealthBar->SetPercent(0.0f);
        HealthBar->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (HealthSizeBox) {
        HealthSizeBox->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UMythicNameplateWidget::ApplyVisualStyle(
    const FMythicNameplateProjection &Projection) {
    if (!VisualStyle) {
        return;
    }

    const FMythicNameplateProfileGeometry &Geometry =
        VisualStyle->ResolveGeometry(Projection.DisclosureTier,
                                     Projection.VisualFamily,
                                     Projection.PresentedCombatRank);
    if (PlateSizeBox) {
        PlateSizeBox->SetMaxDesiredWidth(Geometry.MaximumSize.X);
        PlateSizeBox->SetMaxDesiredHeight(Geometry.MaximumSize.Y);
    }
    if (HealthSizeBox) {
        HealthSizeBox->SetWidthOverride(Geometry.HealthBandWidth);
        HealthSizeBox->SetHeightOverride(Geometry.HealthBandHeight);
    }
    if (PlateSurface) {
        // World-space identity is never a card. Readability comes from the
        // outlined type, emblem silhouette, and health-band backplate.
        PlateSurface->SetBrushColor(FLinearColor::Transparent);
    }

    FLinearColor PrimaryTextColor = VisualStyle->PrimaryTextColor;
    FLinearColor SecondaryTextColor = VisualStyle->SecondaryTextColor;

    FSlateFontInfo IdentityFont = VisualStyle->IdentityFont;
    IdentityFont.Size = Geometry.IdentityFontSize;
    ConfigureSingleLineText(NameText, IdentityFont, PrimaryTextColor);

    FSlateFontInfo SecondaryFont = VisualStyle->SecondaryFont;
    ConfigureSingleLineText(SubtitleText, SecondaryFont,
                            SecondaryTextColor);
    ConfigureSingleLineText(LevelText, SecondaryFont,
                            SecondaryTextColor);
    ConfigureSingleLineText(HealthPercentText, SecondaryFont,
                            SecondaryTextColor);
    ConfigureSingleLineText(StatusText, SecondaryFont,
                            PrimaryTextColor);
    ConfigureSingleLineText(StatusOverflowText, SecondaryFont,
                            SecondaryTextColor);

    for (UTextBlock *Label : StatusBadgeLabels) {
        ConfigureSingleLineText(Label, SecondaryFont, PrimaryTextColor);
    }
    for (UTextBlock *Stack : StatusBadgeStacks) {
        ConfigureSingleLineText(Stack, SecondaryFont, PrimaryTextColor);
    }

    if (HealthBar) {
        const FLinearColor HealthColor =
            Projection.VisualFamily
                    == EMythicNameplateVisualFamily::AllySafety
                ? VisualStyle->AllyHealthFillColor
                : VisualStyle->HealthFillColor;
        HealthBar->SetColorAndOpacity(HealthColor);
    }
}

void UMythicNameplateWidget::PrewarmStatusBadges() {
    if (!StatusBadgeHost || !WidgetTree || !StatusBadgeRoots.IsEmpty()) {
        return;
    }

    StatusBadgeRoots.Reserve(StatusBadgeCapacity);
    StatusBadgeIconBoxes.Reserve(StatusBadgeCapacity);
    StatusBadgeIcons.Reserve(StatusBadgeCapacity);
    StatusBadgeLabels.Reserve(StatusBadgeCapacity);
    StatusBadgeStacks.Reserve(StatusBadgeCapacity);

    for (int32 Index = 0; Index < StatusBadgeCapacity; ++Index) {
        UHorizontalBox *BadgeRoot =
            WidgetTree->ConstructWidget<UHorizontalBox>();
        USizeBox *IconBox = WidgetTree->ConstructWidget<USizeBox>();
        UImage *Icon = WidgetTree->ConstructWidget<UImage>();
        UTextBlock *Label = WidgetTree->ConstructWidget<UTextBlock>();
        UTextBlock *Stack = WidgetTree->ConstructWidget<UTextBlock>();
        if (!BadgeRoot || !IconBox || !Icon || !Label || !Stack) {
            break;
        }

        IconBox->SetWidthOverride(18.0f);
        IconBox->SetHeightOverride(18.0f);
        IconBox->AddChild(Icon);
        BadgeRoot->AddChildToHorizontalBox(IconBox);
        if (UHorizontalBoxSlot *LabelSlot =
                BadgeRoot->AddChildToHorizontalBox(Label)) {
            LabelSlot->SetPadding(FMargin(3.0f, 0.0f, 0.0f, 0.0f));
            LabelSlot->SetVerticalAlignment(VAlign_Center);
        }
        if (UHorizontalBoxSlot *StackSlot =
                BadgeRoot->AddChildToHorizontalBox(Stack)) {
            StackSlot->SetPadding(FMargin(2.0f, 0.0f, 0.0f, 0.0f));
            StackSlot->SetVerticalAlignment(VAlign_Center);
        }
        if (UHorizontalBoxSlot *RootSlot =
                StatusBadgeHost->AddChildToHorizontalBox(BadgeRoot)) {
            RootSlot->SetPadding(FMargin(0.0f, 0.0f, 5.0f, 0.0f));
            RootSlot->SetVerticalAlignment(VAlign_Center);
        }

        Label->SetAutoWrapText(false);
        Label->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
        Stack->SetAutoWrapText(false);
        BadgeRoot->SetVisibility(ESlateVisibility::Collapsed);

        StatusBadgeRoots.Add(BadgeRoot);
        StatusBadgeIconBoxes.Add(IconBox);
        StatusBadgeIcons.Add(Icon);
        StatusBadgeLabels.Add(Label);
        StatusBadgeStacks.Add(Stack);
    }

    StatusOverflowText = WidgetTree->ConstructWidget<UTextBlock>();
    if (StatusOverflowText) {
        StatusOverflowText->SetAutoWrapText(false);
        StatusOverflowText->SetTextOverflowPolicy(
            ETextOverflowPolicy::Ellipsis);
        StatusOverflowText->SetVisibility(ESlateVisibility::Collapsed);
        if (UHorizontalBoxSlot *OverflowSlot =
                StatusBadgeHost->AddChildToHorizontalBox(
                    StatusOverflowText)) {
            OverflowSlot->SetVerticalAlignment(VAlign_Center);
        }
    }
    StatusBadgeHost->SetVisibility(ESlateVisibility::Collapsed);
}

void UMythicNameplateWidget::RefreshStatusBadges(
    const FMythicNameplateProjection &Projection) {
    if (!StatusBadgeHost || StatusBadgeRoots.IsEmpty()) {
        ApplyOptionalText(
            StatusText,
            Projection.Statuses.IsEmpty()
                ? FText::GetEmpty()
                : Projection.Statuses[0].ResolvedLabel);
        return;
    }

    ApplyOptionalText(StatusText, FText::GetEmpty());
    const int32 VisibleCount = FMath::Min3(
        Projection.Statuses.Num(), StatusBadgeRoots.Num(),
        StatusBadgeCapacity);
    for (int32 Index = 0; Index < StatusBadgeRoots.Num(); ++Index) {
        UHorizontalBox *Root = StatusBadgeRoots[Index];
        USizeBox *IconBox = StatusBadgeIconBoxes[Index];
        UImage *Icon = StatusBadgeIcons[Index];
        UTextBlock *Label = StatusBadgeLabels[Index];
        UTextBlock *Stack = StatusBadgeStacks[Index];
        if (Index >= VisibleCount) {
            if (Icon) {
                Icon->SetBrushFromTexture(nullptr);
            }
            if (Root) {
                Root->SetVisibility(ESlateVisibility::Collapsed);
            }
            continue;
        }

        const FMythicNameplateStatusCandidate &Status =
            Projection.Statuses[Index];
        UTexture2D *ResidentIcon = Status.Icon.Get();
        const bool bHasResidentIcon = ResidentIcon != nullptr;
        if (Icon) {
            Icon->SetBrushFromTexture(ResidentIcon, false);
            FLinearColor IconTint = Status.DisplayColor.GetClamped();
            // Status identity belongs to the glyph, not a colored rail or
            // backplate. Keep the glyph opaque and let the owning nameplate's
            // distance/transition opacity fade the complete badge uniformly.
            IconTint.A = 1.0f;
            Icon->SetColorAndOpacity(IconTint);
        }
        if (IconBox) {
            IconBox->SetVisibility(bHasResidentIcon
                ? ESlateVisibility::SelfHitTestInvisible
                : ESlateVisibility::Collapsed);
        }

        const bool bShowResolvedLabel =
            !bHasResidentIcon
            || (RenderPreferences.bShowStatusText && Index == 0);
        ApplyOptionalText(Label, bShowResolvedLabel
            ? Status.ResolvedLabel : FText::GetEmpty());
        ApplyOptionalText(
            Stack,
            Status.StackCount > 1
                ? FText::Format(LOCTEXT("StatusStack", "x{0}"),
                                FText::AsNumber(Status.StackCount))
                : FText::GetEmpty());
        if (Root) {
            Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
    }

    ApplyOptionalText(
        StatusOverflowText,
        Projection.StatusOverflowCount > 0
            ? FText::Format(LOCTEXT("StatusOverflow", "+{0}"),
                            FText::AsNumber(
                                Projection.StatusOverflowCount))
            : FText::GetEmpty());
    StatusBadgeHost->SetVisibility(
        VisibleCount > 0 || Projection.StatusOverflowCount > 0
            ? ESlateVisibility::SelfHitTestInvisible
            : ESlateVisibility::Collapsed);
}

void UMythicNameplateWidget::ResetStatusBadges() {
    for (int32 Index = 0; Index < StatusBadgeRoots.Num(); ++Index) {
        if (StatusBadgeIcons.IsValidIndex(Index)
            && StatusBadgeIcons[Index]) {
            StatusBadgeIcons[Index]->SetBrushFromTexture(nullptr);
            StatusBadgeIcons[Index]->SetColorAndOpacity(
                FLinearColor::White);
        }
        if (StatusBadgeIconBoxes.IsValidIndex(Index)
            && StatusBadgeIconBoxes[Index]) {
            StatusBadgeIconBoxes[Index]->SetVisibility(
                ESlateVisibility::Collapsed);
        }
        if (StatusBadgeLabels.IsValidIndex(Index)) {
            ApplyOptionalText(StatusBadgeLabels[Index], FText::GetEmpty());
        }
        if (StatusBadgeStacks.IsValidIndex(Index)) {
            ApplyOptionalText(StatusBadgeStacks[Index], FText::GetEmpty());
        }
        if (StatusBadgeRoots[Index]) {
            StatusBadgeRoots[Index]->SetVisibility(
                ESlateVisibility::Collapsed);
        }
    }
    ApplyOptionalText(StatusOverflowText, FText::GetEmpty());
    if (StatusBadgeHost) {
        StatusBadgeHost->SetVisibility(ESlateVisibility::Collapsed);
    }
}

#undef LOCTEXT_NAMESPACE
