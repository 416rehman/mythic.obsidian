// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Itemization/Inventory/ViewModels/ItemTooltipVM.h"
#include "Stats/MythicStatTypes.h"
#include "MythicItemComparisonTypes.generated.h"

class UMythicItemInstance;

/** Describes how the candidate's numeric value moved relative to the equipped baseline. */
UENUM(BlueprintType)
enum class EMythicStatValueMovement : uint8 {
    /** Candidate and baseline render as equal at the stat's authored precision. */
    Equal,

    /** The candidate's numeric value is greater than the baseline value. */
    Increase,

    /** The candidate's numeric value is less than the baseline value. */
    Decrease
};

/** Player-facing benefit verdict derived independently from numeric movement. */
UENUM(BlueprintType)
enum class EMythicComparisonVerdict : uint8 {
    /** The change is equal, authored as neutral, or has conflicting comparison semantics. */
    Neutral,

    /** The candidate change is beneficial under the stat's authored comparison direction. */
    Better,

    /** The candidate change is detrimental under the stat's authored comparison direction. */
    Worse
};

namespace MythicItemComparison {
inline bool AreNumberPresentationsEquivalent(
    const FMythicStatNumberPresentation &Left,
    const FMythicStatNumberPresentation &Right) {
    return Left.Format == Right.Format
        && Left.DecimalPlaces == Right.DecimalPlaces
        && Left.UnitSuffix.EqualTo(Right.UnitSuffix);
}
}

/** One typed item-comparison row with canonical units, formatting, movement, and benefit semantics. */
USTRUCT(BlueprintType)
struct MYTHIC_API FAttributeDiff {
    GENERATED_BODY()

    /** Stat.Attribute.* or ItemMetric.* identity; localized labels are presentation only. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison")
    FGameplayTag ComparisonTag;

    /** Localized player-facing row label resolved from canonical stat or item-metric presentation data. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison")
    FText AttributeName;

    /** Comparable contribution supplied by the equipped baseline, expressed in canonical source units. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison")
    float CurrentValue = 0.0f;

    /** Comparable contribution supplied by the inspected candidate, expressed in canonical source units. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison")
    float NewValue = 0.0f;

    /** Signed candidate-minus-equipped change after canonical display quantization, expressed in source units. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison")
    float Delta = 0.0f;

    /** Canonical no-contribution identity used when either compared item does not provide this contribution. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison")
    float ContributionIdentity = 0.0f;

    /** Compatibility alias for ContributionIdentity retained while existing Blueprint/native bindings migrate. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison",
              meta = (DeprecatedProperty, DeprecationMessage = "Use ContributionIdentity."))
    float NeutralValue = 0.0f;

    /** Canonical direction that determines whether a larger or smaller contribution is beneficial. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison")
    EMythicStatComparisonDirection ComparisonDirection = EMythicStatComparisonDirection::Neutral;

    /** Canonical format, precision, and suffix used for values and signed deltas. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison")
    FMythicStatNumberPresentation NumberPresentation;

    /** Numeric movement determined only by candidate-minus-equipped sign at canonical display precision. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison")
    EMythicStatValueMovement Movement = EMythicStatValueMovement::Equal;

    /** Benefit verdict determined from Movement and the independently authored ComparisonDirection. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison")
    EMythicComparisonVerdict Verdict = EMythicComparisonVerdict::Neutral;

    /** Display-ready equipped value formatted by the canonical stat presentation helper. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison")
    FText FormattedCurrentValue;

    /** Display-ready candidate value formatted by the canonical stat presentation helper. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison")
    FText FormattedNewValue;

    /** Display-ready signed candidate-minus-equipped delta; empty when values render as equal. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison")
    FText FormattedDelta;

    /** Localized non-color description of values, movement, and benefit for assistive presentation. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison")
    FText AccessibleSummary;

    /** True when only the candidate supplies this canonical contribution. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison")
    bool bCandidateOnly = false;

    /** True when only the equipped baseline supplies this canonical contribution. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison")
    bool bBaselineOnly = false;

    /** True when at least one side folded multiple contributions into this one net comparison row. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison")
    bool bAggregatedFromMultipleContributions = false;

    /** True when identity, formatting, direction, or numeric validity conflicts and color verdict must remain neutral. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison")
    bool bSemanticConflict = false;

    /** Compatibility benefit flag retained until existing comparison call sites migrate to Verdict. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison",
              meta = (DeprecatedProperty, DeprecationMessage = "Use Verdict == Better."))
    bool bIsUpgrade = false;

    bool operator==(const FAttributeDiff &Other) const {
        return ComparisonTag == Other.ComparisonTag
            && AttributeName.EqualTo(Other.AttributeName)
            && FMath::IsNearlyEqual(CurrentValue, Other.CurrentValue)
            && FMath::IsNearlyEqual(NewValue, Other.NewValue)
            && FMath::IsNearlyEqual(Delta, Other.Delta)
            && FMath::IsNearlyEqual(ContributionIdentity, Other.ContributionIdentity)
            && FMath::IsNearlyEqual(NeutralValue, Other.NeutralValue)
            && ComparisonDirection == Other.ComparisonDirection
            && MythicItemComparison::AreNumberPresentationsEquivalent(
                NumberPresentation, Other.NumberPresentation)
            && Movement == Other.Movement
            && Verdict == Other.Verdict
            && FormattedCurrentValue.EqualTo(Other.FormattedCurrentValue)
            && FormattedNewValue.EqualTo(Other.FormattedNewValue)
            && FormattedDelta.EqualTo(Other.FormattedDelta)
            && AccessibleSummary.EqualTo(Other.AccessibleSummary)
            && bCandidateOnly == Other.bCandidateOnly
            && bBaselineOnly == Other.bBaselineOnly
            && bAggregatedFromMultipleContributions == Other.bAggregatedFromMultipleContributions
            && bSemanticConflict == Other.bSemanticConflict
            && bIsUpgrade == Other.bIsUpgrade;
    }
};

/** Multi-channel-ready presentation payload for one pooled affix row in the existing ItemDetails card. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicAffixRowPresentation {
    GENERATED_BODY()

    /** Candidate affix presentation, or the baseline affix used to synthesize a baseline-only loss row. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison|Affix")
    FAffixDisplayData DisplayData;

    /** Zero or more canonical per-value deltas; multi-channel affixes retain one delta per displayed stat channel. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison|Affix")
    TArray<FAttributeDiff> ValueDiffs;

    /** True while this row is rendered inside an active candidate-versus-equipped comparison. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison|Affix")
    bool bComparisonActive = false;

    /** True when this row exists only to disclose a contribution lost from the equipped baseline. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison|Affix")
    bool bBaselineOnly = false;

    /** Localized non-color summary for the complete multi-channel row. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison|Affix")
    FText AccessibleSummary;
};

/** Exact transient comparison context rendered inside the one persistent ItemDetails card. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicItemDetailsComparisonContext {
    GENERATED_BODY()

    /** True when ItemDetails should project candidate-versus-equipped inline deltas. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison|Item Details")
    bool bComparisonActive = false;

    /** Localized label of the exact equipment target used by both comparison and equip. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison|Item Details")
    FText TargetLabel;

    /** True when the exact target is empty; callers suppress zero-baseline deltas in this state. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison|Item Details")
    bool bTargetEmpty = true;

    /** Item occupying the exact equipment target when the context was resolved. */
    UPROPERTY(BlueprintReadOnly, Transient, Category = "Mythic|Comparison|Item Details")
    TObjectPtr<UMythicItemInstance> BaselineItem = nullptr;

    /** Physical identity expected from BaselineItem when asynchronous presentation completes. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison|Item Details")
    FGuid ExpectedBaselineGuid;

    /** Absolute inventory slot index of the exact equipment target. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison|Item Details")
    int32 TargetSlotIndex = INDEX_NONE;

    /** True when another compatible target exists and the target-cycle affordance should be visible. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison|Item Details")
    bool bCanCycleTarget = false;
};

/** Atomic weapon comparison projection for the existing ItemDetails attack block. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicWeaponAttackComparisonViewData {
    GENERATED_BODY()

    /** True when the inspected item supplied a complete canonical weapon attack projection. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison|Weapon Attack")
    bool bIsValid = false;

    /** True when the replacement slot contains a weapon with a complete canonical attack projection. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison|Weapon Attack")
    bool bHasEquippedWeaponAttack = false;

    /** True when metric deltas were built; false for a caller-suppressed empty baseline. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison|Weapon Attack")
    bool bHasComparisonDeltas = false;

    /** Canonical attack projection for the inspected candidate. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison|Weapon Attack")
    FMythicWeaponAttackViewData InspectedAttack;

    /** Canonical attack projection for the equipped item, or an invalid projection when the target is empty. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison|Weapon Attack")
    FMythicWeaponAttackViewData EquippedAttack;

    /** Expected damage-per-hit comparison derived from the same uniform range used by combat. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison|Weapon Attack")
    FAttributeDiff AverageDamagePerHitComparison;

    /** Sustained-DPS comparison using the inspected candidate as the new value. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison|Weapon Attack")
    FAttributeDiff DamagePerSecondComparison;

    /** Effective-APS comparison after the exact combat AttackSpeed clamp. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Comparison|Weapon Attack")
    FAttributeDiff EffectiveAttacksPerSecondComparison;

    bool operator==(const FMythicWeaponAttackComparisonViewData &Other) const {
        return bIsValid == Other.bIsValid
            && bHasEquippedWeaponAttack == Other.bHasEquippedWeaponAttack
            && bHasComparisonDeltas == Other.bHasComparisonDeltas
            && InspectedAttack == Other.InspectedAttack
            && EquippedAttack == Other.EquippedAttack
            && AverageDamagePerHitComparison == Other.AverageDamagePerHitComparison
            && DamagePerSecondComparison == Other.DamagePerSecondComparison
            && EffectiveAttacksPerSecondComparison == Other.EffectiveAttacksPerSecondComparison;
    }
};
