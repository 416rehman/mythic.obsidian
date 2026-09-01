// Copyright Stellar Games. All Rights Reserved.

#include "UI/Inventory/MythicDPSWidget.h"

#include "CommonTextBlock.h"
#include "UI/Inventory/MythicItemComparisonPresentation.h"

namespace {

void SetDPSOptionalText(UCommonTextBlock *TextBlock, const FText &Text) {
    if (!TextBlock) {
        return;
    }
    TextBlock->SetText(Text);
    TextBlock->SetVisibility(
        Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

bool ApplyMetricComparison(
    const FAttributeDiff &Diff,
    UCommonTextBlock *DeltaText,
    UCommonTextBlock *MovementText,
    const bool bLabelAsAverage = false) {
    if (!FMythicItemComparisonPresentation::HasVisibleDelta(Diff)) {
        SetDPSOptionalText(DeltaText, FText::GetEmpty());
        SetDPSOptionalText(MovementText, FText::GetEmpty());
        return false;
    }

    const FText DeltaToken = FMythicItemComparisonPresentation::BuildDeltaToken(Diff);
    SetDPSOptionalText(
        DeltaText,
        bLabelAsAverage
            ? FText::Format(NSLOCTEXT("MythicDPS", "AverageDelta", "AVG {0}"), DeltaToken)
            : DeltaToken);
    SetDPSOptionalText(
        MovementText,
        FMythicItemComparisonPresentation::BuildMovementGlyph(Diff));

    const FLinearColor ComparisonColor =
        FMythicItemComparisonPresentation::ResolveOutcomeColor(Diff.Verdict);
    if (DeltaText) {
        DeltaText->SetColorAndOpacity(FSlateColor(ComparisonColor));
    }
    if (MovementText) {
        MovementText->SetColorAndOpacity(FSlateColor(ComparisonColor));
    }
    return true;
}

void ClearMetricComparison(
    UCommonTextBlock *DeltaText,
    UCommonTextBlock *MovementText) {
    SetDPSOptionalText(DeltaText, FText::GetEmpty());
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
        UCommonTextBlock *Movement,
        const bool bLabelAsAverage = false) {
        if (ApplyMetricComparison(Diff, Delta, Movement, bLabelAsAverage)
            && !Diff.AccessibleSummary.IsEmpty()) {
            AccessibleParts.Add(Diff.AccessibleSummary);
        }
    };

    Apply(
        AttackComparisonData.DamagePerSecondComparison,
        DamagePerSecondDeltaText,
        DamagePerSecondMovementIcon);
    Apply(
        AttackComparisonData.EffectiveAttacksPerSecondComparison,
        AttacksPerSecondDeltaText,
        AttacksPerSecondMovementIcon);
    Apply(
        AttackComparisonData.AverageDamagePerHitComparison,
        AverageDamagePerHitDeltaText,
        AverageDamagePerHitMovementIcon,
        true);

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
        DamagePerSecondMovementIcon);
    ClearMetricComparison(
        AttacksPerSecondDeltaText,
        AttacksPerSecondMovementIcon);
    ClearMetricComparison(
        AverageDamagePerHitDeltaText,
        AverageDamagePerHitMovementIcon);
    SetDPSOptionalText(AttackComparisonAccessibleText, FText::GetEmpty());
    OnAttackComparisonUpdated(AttackComparisonData);
}
