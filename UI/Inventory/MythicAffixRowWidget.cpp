// Copyright Stellar Games. All Rights Reserved.

#include "UI/Inventory/MythicAffixRowWidget.h"

#include "CommonTextBlock.h"
#include "Components/Widget.h"
#include "UI/MythicUIStyle.h"
#include "UI/ViewModels/MythicStatDisplay.h"

namespace {

void SetAffixOptionalText(UCommonTextBlock *TextBlock, const FText &Text) {
    if (!TextBlock) {
        return;
    }

    TextBlock->SetText(Text);
    TextBlock->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

TArray<const FMythicAffixValueViewData *> GetDisplayOrderedValues(const FMythicAffixViewData &ViewData) {
    TArray<const FMythicAffixValueViewData *> Result;
    Result.Reserve(ViewData.Values.Num());

    const FMythicAffixValueViewData *PrimaryValue = nullptr;
    if (ViewData.PrimaryStatTag.IsValid()) {
        PrimaryValue = ViewData.Values.FindByPredicate([&ViewData](const FMythicAffixValueViewData &Value) {
            return Value.StatTag == ViewData.PrimaryStatTag;
        });
    }

    if (PrimaryValue) {
        Result.Add(PrimaryValue);
    }

    for (const FMythicAffixValueViewData &Value : ViewData.Values) {
        if (&Value != PrimaryValue) {
            Result.Add(&Value);
        }
    }

    return Result;
}

FText LabelChannelValue(const FMythicAffixValueViewData &Channel, const FText &Value) {
    if (Channel.StatLabel.IsEmpty()) {
        return Value;
    }

    return FText::Format(NSLOCTEXT("MythicAffixRow", "LabelledChannelValue", "{0}: {1}"),
                         Channel.StatLabel, Value);
}

FText JoinChannelText(const TArray<const FMythicAffixValueViewData *> &Channels, const bool bUseRange) {
    TArray<FText> Parts;
    Parts.Reserve(Channels.Num());

    const bool bNeedsLabels = Channels.Num() > 1;
    for (const FMythicAffixValueViewData *Channel : Channels) {
        if (!Channel) {
            continue;
        }

        const FText &Part = bUseRange ? Channel->FormattedRange : Channel->FormattedValue;
        if (Part.IsEmpty()) {
            continue;
        }

        Parts.Add(bNeedsLabels ? LabelChannelValue(*Channel, Part) : Part);
    }

    return Parts.Num() > 0
               ? FText::Join(NSLOCTEXT("MythicAffixRow", "ChannelSeparator", " / "), Parts)
               : FText::GetEmpty();
}

FText FormatLegacyValue(const FAffixDisplayData &DisplayData) {
    FMythicStatNumberPresentation Presentation;
    Presentation.Format = DisplayData.bIsPercentage
        ? EMythicStatFormat::Percent
        : EMythicStatFormat::Flat;
    Presentation.DecimalPlaces = 2;
    return MythicStatDisplay::FormatValue(DisplayData.Value, Presentation);
}

TArray<const FAttributeDiff *> GetVisibleDiffs(
    const FMythicAffixRowPresentation &Presentation) {
    TArray<const FAttributeDiff *> Result;
    Result.Reserve(Presentation.ValueDiffs.Num());
    for (const FAttributeDiff &Diff : Presentation.ValueDiffs) {
        if (Diff.Movement != EMythicStatValueMovement::Equal
            && !Diff.FormattedDelta.IsEmpty()) {
            Result.Add(&Diff);
        }
    }
    return Result;
}

FText LabelComparisonChannel(const FAttributeDiff &Diff, const FText &Value) {
    return Diff.AttributeName.IsEmpty()
        ? Value
        : FText::Format(
            NSLOCTEXT("MythicAffixRow", "LabelledComparisonChannel", "{0}: {1}"),
            Diff.AttributeName,
            Value);
}

FText BuildDeltaText(const TArray<const FAttributeDiff *> &Diffs) {
    TArray<FText> Parts;
    Parts.Reserve(Diffs.Num());
    for (const FAttributeDiff *Diff : Diffs) {
        if (!Diff) {
            continue;
        }

        FText Part = Diff->bAggregatedFromMultipleContributions
            ? FText::Format(
                NSLOCTEXT("MythicAffixRow", "NetComparisonDelta", "Net {0}"),
                Diff->FormattedDelta)
            : Diff->FormattedDelta;
        Parts.Add(Diffs.Num() > 1 ? LabelComparisonChannel(*Diff, Part) : Part);
    }
    return Parts.IsEmpty()
        ? FText::GetEmpty()
        : FText::Join(NSLOCTEXT("MythicAffixRow", "ComparisonChannelSeparator", " / "), Parts);
}

FText BuildMovementText(const TArray<const FAttributeDiff *> &Diffs) {
    TArray<FText> Parts;
    Parts.Reserve(Diffs.Num());
    for (const FAttributeDiff *Diff : Diffs) {
        if (!Diff) {
            continue;
        }
        Parts.Add(Diff->Movement == EMythicStatValueMovement::Increase
            ? NSLOCTEXT("MythicAffixRow", "MovementIncrease", "\u2191")
            : NSLOCTEXT("MythicAffixRow", "MovementDecrease", "\u2193"));
    }
    return Parts.IsEmpty()
        ? FText::GetEmpty()
        : FText::Join(NSLOCTEXT("MythicAffixRow", "MovementSeparator", " / "), Parts);
}

FText BuildBaselineText(const TArray<const FAttributeDiff *> &Diffs) {
    TArray<FText> Parts;
    Parts.Reserve(Diffs.Num());
    for (const FAttributeDiff *Diff : Diffs) {
        if (!Diff) {
            continue;
        }
        const FText Value = Diffs.Num() > 1
            ? LabelComparisonChannel(*Diff, Diff->FormattedCurrentValue)
            : Diff->FormattedCurrentValue;
        Parts.Add(FText::Format(
            NSLOCTEXT("MythicAffixRow", "EquippedBaselineValue", "Equipped {0}"),
            Value));
    }
    return Parts.IsEmpty()
        ? FText::GetEmpty()
        : FText::Join(NSLOCTEXT("MythicAffixRow", "BaselineChannelSeparator", " / "), Parts);
}

FText BuildAccessibleText(const TArray<const FAttributeDiff *> &Diffs) {
    TArray<FText> Parts;
    Parts.Reserve(Diffs.Num());
    for (const FAttributeDiff *Diff : Diffs) {
        if (Diff && !Diff->AccessibleSummary.IsEmpty()) {
            Parts.Add(Diff->AccessibleSummary);
        }
    }
    return Parts.IsEmpty()
        ? FText::GetEmpty()
        : FText::Join(NSLOCTEXT("MythicAffixRow", "AccessibleChannelSeparator", " "), Parts);
}

FLinearColor ResolveComparisonColor(const TArray<const FAttributeDiff *> &Diffs) {
    bool bHasBetter = false;
    bool bHasWorse = false;
    for (const FAttributeDiff *Diff : Diffs) {
        bHasBetter |= Diff && Diff->Verdict == EMythicComparisonVerdict::Better;
        bHasWorse |= Diff && Diff->Verdict == EMythicComparisonVerdict::Worse;
    }

    const UMythicUIStyleSettings &Style = FMythicUIStyle::Get();
    if (bHasBetter && !bHasWorse) {
        return Style.Positive;
    }
    if (bHasWorse && !bHasBetter) {
        return Style.Negative;
    }
    return Style.InkSubtle;
}

void ClearComparisonBindings(
    UCommonTextBlock *Delta,
    UCommonTextBlock *Movement,
    UCommonTextBlock *Baseline,
    UCommonTextBlock *Accessible) {
    SetAffixOptionalText(Delta, FText::GetEmpty());
    SetAffixOptionalText(Movement, FText::GetEmpty());
    SetAffixOptionalText(Baseline, FText::GetEmpty());
    SetAffixOptionalText(Accessible, FText::GetEmpty());
}

} // namespace

void UMythicAffixRowWidget::SetFromAffixDisplayData(const FAffixDisplayData &InDisplayData) {
    FMythicAffixRowPresentation OrdinaryPresentation;
    OrdinaryPresentation.DisplayData = InDisplayData;
    SetPresentation(OrdinaryPresentation);
}

void UMythicAffixRowWidget::SetPresentation(
    const FMythicAffixRowPresentation &InPresentation) {
    const uint32 MutationSerial = ++PresentationMutationSerial;
    Presentation = InPresentation;
    ViewData = Presentation.DisplayData.ViewData;

    FText AttributeText = ViewData.DisplayName;
    if (AttributeText.IsEmpty()) {
        AttributeText = Presentation.DisplayData.AttributeName;
    }

    const TArray<const FMythicAffixValueViewData *> Channels = GetDisplayOrderedValues(ViewData);
    const FText RollText = Presentation.bBaselineOnly
        ? NSLOCTEXT("MythicAffixRow", "MissingCandidateValue", "\u2014")
        : Channels.Num() > 0
            ? JoinChannelText(Channels, false)
            : FormatLegacyValue(Presentation.DisplayData);
    const FText RangeText = Presentation.bBaselineOnly
        ? FText::GetEmpty()
        : Channels.Num() > 0
            ? JoinChannelText(Channels, true)
            : FText::GetEmpty();

    SetAffixOptionalText(Attribute, AttributeText);
    SetAffixOptionalText(Roll, RollText);
    SetAffixOptionalText(RollRange, RangeText);

    ClearComparisonBindings(
        DeltaText, MovementIcon, EquippedBaselineText, ComparisonAccessibleText);
    const TArray<const FAttributeDiff *> VisibleDiffs = GetVisibleDiffs(Presentation);
    if (!VisibleDiffs.IsEmpty()) {
        const FLinearColor ComparisonColor = ResolveComparisonColor(VisibleDiffs);
        SetAffixOptionalText(DeltaText, BuildDeltaText(VisibleDiffs));
        SetAffixOptionalText(MovementIcon, BuildMovementText(VisibleDiffs));
        SetAffixOptionalText(EquippedBaselineText, BuildBaselineText(VisibleDiffs));
        SetAffixOptionalText(
            ComparisonAccessibleText,
            Presentation.AccessibleSummary.IsEmpty()
                ? BuildAccessibleText(VisibleDiffs)
                : Presentation.AccessibleSummary);
        if (DeltaText) {
            DeltaText->SetColorAndOpacity(FSlateColor(ComparisonColor));
        }
        if (MovementIcon) {
            MovementIcon->SetColorAndOpacity(FSlateColor(ComparisonColor));
        }
        if (EquippedBaselineText) {
            EquippedBaselineText->SetColorAndOpacity(
                FSlateColor(FMythicUIStyle::Get().InkSubtle));
        }
    }

    OnAffixPresentationUpdated(ViewData);
    if (MutationSerial != PresentationMutationSerial) {
        return;
    }
    OnAffixComparisonUpdated(Presentation);
}

void UMythicAffixRowWidget::ClearDeltaPresentation() {
    ++PresentationMutationSerial;
    Presentation.ValueDiffs.Reset();
    Presentation.bBaselineOnly = false;
    Presentation.AccessibleSummary = FText::GetEmpty();
    ClearComparisonBindings(
        DeltaText, MovementIcon, EquippedBaselineText, ComparisonAccessibleText);
    OnAffixComparisonUpdated(Presentation);
}
