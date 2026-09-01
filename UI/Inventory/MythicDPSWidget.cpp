// Copyright Stellar Games. All Rights Reserved.

#include "UI/Inventory/MythicDPSWidget.h"

#include "CommonTextBlock.h"
#include "UI/MythicUIStyle.h"

namespace {

void SetDPSOptionalText(UCommonTextBlock *TextBlock, const FText &Text) {
    if (!TextBlock) {
        return;
    }
    TextBlock->SetText(Text);
    TextBlock->SetVisibility(
        Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

FLinearColor ResolveComparisonColor(const FAttributeDiff &Diff) {
    const UMythicUIStyleSettings &Style = FMythicUIStyle::Get();
    if (Diff.Verdict == EMythicComparisonVerdict::Better) {
        return Style.Positive;
    }
    if (Diff.Verdict == EMythicComparisonVerdict::Worse) {
        return Style.Negative;
    }
    return Style.InkSubtle;
}

FText GetMovementGlyph(const FAttributeDiff &Diff) {
    if (Diff.Movement == EMythicStatValueMovement::Increase) {
        return NSLOCTEXT("MythicDPS", "MovementIncrease", "\u2191");
    }
    if (Diff.Movement == EMythicStatValueMovement::Decrease) {
        return NSLOCTEXT("MythicDPS", "MovementDecrease", "\u2193");
    }
    return FText::GetEmpty();
}

bool ApplyMetricComparison(
    const FAttributeDiff &Diff,
    UCommonTextBlock *DeltaText,
    UCommonTextBlock *BaselineText,
    UCommonTextBlock *MovementText) {
    if (!Diff.ComparisonTag.IsValid()
        || Diff.Movement == EMythicStatValueMovement::Equal
        || Diff.FormattedDelta.IsEmpty()) {
        SetDPSOptionalText(DeltaText, FText::GetEmpty());
        SetDPSOptionalText(BaselineText, FText::GetEmpty());
        SetDPSOptionalText(MovementText, FText::GetEmpty());
        return false;
    }

    SetDPSOptionalText(DeltaText, Diff.FormattedDelta);
    SetDPSOptionalText(
        BaselineText,
        FText::Format(
            NSLOCTEXT("MythicDPS", "EquippedMetricBaseline", "Equipped {0}"),
            Diff.FormattedCurrentValue));
    SetDPSOptionalText(MovementText, GetMovementGlyph(Diff));

    const FLinearColor ComparisonColor = ResolveComparisonColor(Diff);
    if (DeltaText) {
        DeltaText->SetColorAndOpacity(FSlateColor(ComparisonColor));
    }
    if (MovementText) {
        MovementText->SetColorAndOpacity(FSlateColor(ComparisonColor));
    }
    if (BaselineText) {
        BaselineText->SetColorAndOpacity(FSlateColor(FMythicUIStyle::Get().InkSubtle));
    }
    return true;
}

void ClearMetricComparison(
    UCommonTextBlock *DeltaText,
    UCommonTextBlock *BaselineText,
    UCommonTextBlock *MovementText) {
    SetDPSOptionalText(DeltaText, FText::GetEmpty());
    SetDPSOptionalText(BaselineText, FText::GetEmpty());
    SetDPSOptionalText(MovementText, FText::GetEmpty());
}

} // namespace

void UMythicDPSWidget::SetAttackDisplayData(
    const FMythicWeaponAttackViewData &InAttackDisplayData) {
    if (!InAttackDisplayData.bIsValid) {
        ClearAttackDisplayData();
        return;
    }

    const uint32 MutationSerial = ++PresentationMutationSerial;
    ClearAttackComparisonDataInternal();
    if (MutationSerial != PresentationMutationSerial) {
        return;
    }
    AttackDisplayData = InAttackDisplayData;
    if (DamagePerSecondText) {
        DamagePerSecondText->SetText(AttackDisplayData.DamagePerSecondText);
    }
    if (DamagePerHitText) {
        DamagePerHitText->SetText(AttackDisplayData.DamagePerHitText);
    }
    if (AttacksPerSecondText) {
        AttacksPerSecondText->SetText(AttackDisplayData.AttacksPerSecondText);
    }
    OnAttackPresentationUpdated(AttackDisplayData);
}

void UMythicDPSWidget::ClearAttackDisplayData() {
    const uint32 MutationSerial = ++PresentationMutationSerial;
    ClearAttackComparisonDataInternal();
    if (MutationSerial != PresentationMutationSerial) {
        return;
    }
    AttackDisplayData = FMythicWeaponAttackViewData();
    if (DamagePerSecondText) {
        DamagePerSecondText->SetText(FText::GetEmpty());
    }
    if (DamagePerHitText) {
        DamagePerHitText->SetText(FText::GetEmpty());
    }
    if (AttacksPerSecondText) {
        AttacksPerSecondText->SetText(FText::GetEmpty());
    }
    OnAttackPresentationUpdated(AttackDisplayData);
}

void UMythicDPSWidget::SetAttackComparisonData(
    const FMythicWeaponAttackComparisonViewData &InComparisonData) {
    ++PresentationMutationSerial;
    if (!AttackDisplayData.bIsValid
        || !InComparisonData.bIsValid
        || !InComparisonData.bHasEquippedWeaponAttack
        || !InComparisonData.bHasComparisonDeltas
        || !(InComparisonData.InspectedAttack == AttackDisplayData)) {
        ClearAttackComparisonDataInternal();
        return;
    }

    AttackComparisonData = InComparisonData;
    TArray<FText> AccessibleParts;
    AccessibleParts.Reserve(3);
    const auto Apply = [&AccessibleParts](
        const FAttributeDiff &Diff,
        UCommonTextBlock *Delta,
        UCommonTextBlock *Baseline,
        UCommonTextBlock *Movement) {
        if (ApplyMetricComparison(Diff, Delta, Baseline, Movement)
            && !Diff.AccessibleSummary.IsEmpty()) {
            AccessibleParts.Add(Diff.AccessibleSummary);
        }
    };

    Apply(
        AttackComparisonData.DamagePerSecondComparison,
        DamagePerSecondDeltaText,
        DamagePerSecondBaselineText,
        DamagePerSecondMovementIcon);
    Apply(
        AttackComparisonData.EffectiveAttacksPerSecondComparison,
        AttacksPerSecondDeltaText,
        AttacksPerSecondBaselineText,
        AttacksPerSecondMovementIcon);
    Apply(
        AttackComparisonData.AverageDamagePerHitComparison,
        AverageDamagePerHitDeltaText,
        AverageDamagePerHitBaselineText,
        AverageDamagePerHitMovementIcon);

    SetDPSOptionalText(
        AttackComparisonAccessibleText,
        AccessibleParts.IsEmpty()
            ? FText::GetEmpty()
            : FText::Join(
                NSLOCTEXT("MythicDPS", "AccessibleMetricSeparator", " "),
                AccessibleParts));
    OnAttackComparisonUpdated(AttackComparisonData);
}

void UMythicDPSWidget::ClearAttackComparisonData() {
    ++PresentationMutationSerial;
    ClearAttackComparisonDataInternal();
}

void UMythicDPSWidget::ClearAttackComparisonDataInternal() {
    AttackComparisonData = FMythicWeaponAttackComparisonViewData();
    ClearMetricComparison(
        DamagePerSecondDeltaText,
        DamagePerSecondBaselineText,
        DamagePerSecondMovementIcon);
    ClearMetricComparison(
        AttacksPerSecondDeltaText,
        AttacksPerSecondBaselineText,
        AttacksPerSecondMovementIcon);
    ClearMetricComparison(
        AverageDamagePerHitDeltaText,
        AverageDamagePerHitBaselineText,
        AverageDamagePerHitMovementIcon);
    SetDPSOptionalText(AttackComparisonAccessibleText, FText::GetEmpty());
    OnAttackComparisonUpdated(AttackComparisonData);
}
