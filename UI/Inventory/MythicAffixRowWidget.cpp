// Copyright Stellar Games. All Rights Reserved.

#include "UI/Inventory/MythicAffixRowWidget.h"

#include "CommonTextBlock.h"
#include "Components/Widget.h"

namespace {

void SetOptionalText(UCommonTextBlock *TextBlock, const FText &Text) {
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
    FNumberFormattingOptions Options;
    Options.SetMinimumFractionalDigits(0);
    Options.SetMaximumFractionalDigits(2);

    return DisplayData.bIsPercentage
               ? FText::AsPercent(DisplayData.Value, &Options)
               : FText::AsNumber(DisplayData.Value, &Options);
}

} // namespace

void UMythicAffixRowWidget::SetFromAffixDisplayData(const FAffixDisplayData &InDisplayData) {
    ViewData = InDisplayData.ViewData;

    FText AttributeText = ViewData.DisplayName;
    if (AttributeText.IsEmpty()) {
        AttributeText = InDisplayData.AttributeName;
    }

    const TArray<const FMythicAffixValueViewData *> Channels = GetDisplayOrderedValues(ViewData);
    const FText RollText = Channels.Num() > 0 ? JoinChannelText(Channels, false) : FormatLegacyValue(InDisplayData);
    const FText RangeText = Channels.Num() > 0 ? JoinChannelText(Channels, true) : FText::GetEmpty();

    SetOptionalText(Attribute, AttributeText);
    SetOptionalText(Roll, RollText);
    SetOptionalText(RollRange, RangeText);

    OnAffixPresentationUpdated(ViewData);
}
