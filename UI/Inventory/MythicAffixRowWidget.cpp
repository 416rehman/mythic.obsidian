// Copyright Stellar Games. All Rights Reserved.

#include "UI/Inventory/MythicAffixRowWidget.h"

#include "CommonTextBlock.h"
#include "Components/Widget.h"
#include "UI/Inventory/MythicItemComparisonPresentation.h"
#include "UI/Settings/MythicUserSettings.h"
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
        if (FMythicItemComparisonPresentation::HasVisibleDelta(Diff)) {
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

        const FText Part = FMythicItemComparisonPresentation::BuildDeltaToken(*Diff);
        if (Part.IsEmpty()) {
            continue;
        }
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
        const FText Glyph = FMythicItemComparisonPresentation::BuildMovementGlyph(*Diff);
        if (!Glyph.IsEmpty()) {
            Parts.Add(Glyph);
        }
    }
    return Parts.IsEmpty()
        ? FText::GetEmpty()
        : FText::Join(NSLOCTEXT("MythicAffixRow", "MovementSeparator", " / "), Parts);
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
    return FMythicItemComparisonPresentation::ResolveCombinedOutcomeColor(Diffs);
}

bool AreAllDiffsCandidateOnly(const TArray<const FAttributeDiff *> &Diffs) {
    if (Diffs.IsEmpty()) {
        return false;
    }
    for (const FAttributeDiff *Diff : Diffs) {
        if (!Diff || !Diff->bCandidateOnly) {
            return false;
        }
    }
    return true;
}

bool AreAllDiffsBaselineOnly(const TArray<const FAttributeDiff *> &Diffs) {
    if (Diffs.IsEmpty()) {
        return false;
    }
    for (const FAttributeDiff *Diff : Diffs) {
        if (!Diff || !Diff->bBaselineOnly) {
            return false;
        }
    }
    return true;
}

void ClearComparisonBindings(
    UCommonTextBlock *Delta,
    UCommonTextBlock *Movement,
    UCommonTextBlock *Accessible) {
    SetAffixOptionalText(Delta, FText::GetEmpty());
    SetAffixOptionalText(Movement, FText::GetEmpty());
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

    const TArray<const FAttributeDiff *> VisibleDiffs = GetVisibleDiffs(Presentation);
    const bool bCandidateOnlyRow = AreAllDiffsCandidateOnly(VisibleDiffs);
    const bool bBaselineOnlyRow = Presentation.bBaselineOnly
        || AreAllDiffsBaselineOnly(VisibleDiffs);
    const bool bOneSidedRow = bCandidateOnlyRow || bBaselineOnlyRow;

    const TArray<const FMythicAffixValueViewData *> Channels = GetDisplayOrderedValues(ViewData);
    const FText RollText = bOneSidedRow
        ? BuildDeltaText(VisibleDiffs)
        : Channels.Num() > 0
            ? JoinChannelText(Channels, false)
            : FormatLegacyValue(Presentation.DisplayData);
    const UMythicUserSettings *UserSettings = UMythicUserSettings::Get();
    const bool bShowRange = !Presentation.bComparisonActive
        || !UserSettings
        || UserSettings->GetShowItemComparisonRollRanges();
    const FText RangeText = !bShowRange
        ? FText::GetEmpty()
        : Channels.Num() > 0
            ? JoinChannelText(Channels, true)
            : FText::GetEmpty();

    SetAffixOptionalText(Attribute, AttributeText);
    SetAffixOptionalText(Roll, RollText);
    SetAffixOptionalText(RollRange, RangeText);
    if (Roll) {
        if (!bHasCapturedDefaultRollColor) {
            DefaultRollColor = Roll->GetColorAndOpacity();
            bHasCapturedDefaultRollColor = true;
        }
        Roll->SetColorAndOpacity(DefaultRollColor);
    }

    ClearComparisonBindings(DeltaText, MovementIcon, ComparisonAccessibleText);
    if (!VisibleDiffs.IsEmpty()) {
        const FLinearColor ComparisonColor = ResolveComparisonColor(VisibleDiffs);
        if (bOneSidedRow) {
            if (Roll) {
                Roll->SetColorAndOpacity(FSlateColor(ComparisonColor));
            }
        }
        else {
            SetAffixOptionalText(DeltaText, BuildDeltaText(VisibleDiffs));
            SetAffixOptionalText(MovementIcon, BuildMovementText(VisibleDiffs));
        }
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
    Presentation.bComparisonActive = false;
    Presentation.bBaselineOnly = false;
    Presentation.AccessibleSummary = FText::GetEmpty();
    ClearComparisonBindings(DeltaText, MovementIcon, ComparisonAccessibleText);
    if (Roll && bHasCapturedDefaultRollColor) {
        Roll->SetColorAndOpacity(DefaultRollColor);
    }
    OnAffixComparisonUpdated(Presentation);
}
