#include "Misc/AutomationTest.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "Itemization/Inventory/ViewModels/ItemComparisonVM.h"
#include "Itemization/Inventory/ViewModels/ItemTooltipVM.h"
#include "Itemization/Inventory/ViewModels/MythicTags_ItemMetrics.h"
#include "Settings/MythicCombatSettings.h"
#include "UI/ViewModels/MythicStatDisplay.h"

#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWeaponDamageRangeTest,
    "Mythic.Combat.WeaponDamageRange",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWeaponDamageRangeTest::RunTest(const FString &Parameters) {
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    const float MaximumMultiplier = FMath::Max(
        1.0f, Settings ? Settings->WeaponDamageMaximumMultiplier : 1.5f);

    float MinimumDamage = -1.0f;
    float MaximumDamage = -1.0f;
    float AverageDamage = -1.0f;
    TestTrue(TEXT("a finite non-negative DamagePerHit resolves"),
             MythicCombat::ResolveWeaponDamageRange(
                 60.0f, MinimumDamage, MaximumDamage, AverageDamage));
    TestTrue(TEXT("DamagePerHit is the lower endpoint"),
             FMath::IsNearlyEqual(MinimumDamage, 60.0f, UE_KINDA_SMALL_NUMBER));
    TestTrue(TEXT("the upper endpoint uses the one global authored multiplier"),
             FMath::IsNearlyEqual(
                 MaximumDamage, 60.0f * MaximumMultiplier, UE_KINDA_SMALL_NUMBER));
    TestTrue(TEXT("the expected value is the mean of the uniform range"),
             FMath::IsNearlyEqual(
                 AverageDamage,
                 (MinimumDamage + MaximumDamage) * 0.5f,
                 UE_KINDA_SMALL_NUMBER));

    const auto ExpectRejectedAndZeroed = [this](const TCHAR *Label, const float DamagePerHit) {
        float RejectedMinimum = 11.0f;
        float RejectedMaximum = 22.0f;
        float RejectedAverage = 33.0f;
        TestFalse(Label, MythicCombat::ResolveWeaponDamageRange(
                             DamagePerHit,
                             RejectedMinimum,
                             RejectedMaximum,
                             RejectedAverage));
        TestEqual(TEXT("a rejected range clears its minimum"), RejectedMinimum, 0.0f);
        TestEqual(TEXT("a rejected range clears its maximum"), RejectedMaximum, 0.0f);
        TestEqual(TEXT("a rejected range clears its expected value"), RejectedAverage, 0.0f);
    };

    ExpectRejectedAndZeroed(TEXT("negative DamagePerHit is rejected"), -1.0f);
    ExpectRejectedAndZeroed(
        TEXT("NaN DamagePerHit is rejected"),
        std::numeric_limits<float>::quiet_NaN());
    ExpectRejectedAndZeroed(
        TEXT("infinite DamagePerHit is rejected"),
        std::numeric_limits<float>::infinity());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWeaponAttackMetricsTest,
    "Mythic.UI.ItemDetails.WeaponAttackMetrics",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWeaponAttackMetricsTest::RunTest(const FString &Parameters) {
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    const float MaximumMultiplier = FMath::Max(
        1.0f, Settings ? Settings->WeaponDamageMaximumMultiplier : 1.5f);
    const float ExpectedMinimumDamage = 60.0f;
    const float ExpectedMaximumDamage = ExpectedMinimumDamage * MaximumMultiplier;
    const float ExpectedAverageDamage =
        (ExpectedMinimumDamage + ExpectedMaximumDamage) * 0.5f;

    FMythicWeaponAttackViewData Metrics;
    TestTrue(TEXT("valid weapon inputs produce an attack projection"),
             UItemTooltipVM::CalculateWeaponAttackMetrics(
                 60.0f, 0.2f, 0.75f, 0.8f, 1.4f, Metrics));
    TestTrue(TEXT("metrics retain the shared minimum weapon damage"),
             FMath::IsNearlyEqual(
                 Metrics.MinimumDamagePerHit,
                 ExpectedMinimumDamage,
                 UE_KINDA_SMALL_NUMBER));
    TestTrue(TEXT("metrics retain the shared maximum weapon damage"),
             FMath::IsNearlyEqual(
                 Metrics.MaximumDamagePerHit,
                 ExpectedMaximumDamage,
                 UE_KINDA_SMALL_NUMBER));
    TestTrue(TEXT("metrics retain the uniform damage expectation"),
             FMath::IsNearlyEqual(
                 Metrics.AverageDamagePerHit,
                 ExpectedAverageDamage,
                 UE_KINDA_SMALL_NUMBER));
    TestTrue(TEXT("authored cadence is the reciprocal of the nominal attack cycle"),
             FMath::IsNearlyEqual(
                 Metrics.BaseAttacksPerSecond,
                 4.0f / 3.0f,
                 UE_KINDA_SMALL_NUMBER));
    TestTrue(TEXT("a twenty-percent bonus applies the exact combat play rate"),
             FMath::IsNearlyEqual(
                 Metrics.AttacksPerSecond, 1.6f, UE_KINDA_SMALL_NUMBER));
    TestTrue(TEXT("effective attack time uses the same combat play rate"),
             FMath::IsNearlyEqual(
                 Metrics.AttackTimeSeconds, 0.625f, UE_KINDA_SMALL_NUMBER));
    TestTrue(TEXT("DPS uses expected damage rather than either range endpoint"),
             FMath::IsNearlyEqual(
                 Metrics.DamagePerSecond,
                 ExpectedAverageDamage * 1.6f,
                 UE_KINDA_SMALL_NUMBER));

    FMythicWeaponAttackViewData HighClamp;
    TestTrue(TEXT("an over-cap attack-speed bonus still produces metrics"),
             UItemTooltipVM::CalculateWeaponAttackMetrics(
                 60.0f, 3.0f, 0.75f, 0.8f, 1.4f, HighClamp));
    TestTrue(TEXT("the upper combat play-rate clamp is reflected in APS"),
             FMath::IsNearlyEqual(
                 HighClamp.AttacksPerSecond,
                 (1.0f / 0.75f) * 1.4f,
                 UE_KINDA_SMALL_NUMBER));
    TestTrue(TEXT("the upper combat play-rate clamp is reflected in attack time"),
             FMath::IsNearlyEqual(
                 HighClamp.AttackTimeSeconds,
                 0.75f / 1.4f,
                 UE_KINDA_SMALL_NUMBER));

    FMythicWeaponAttackViewData LowClamp;
    TestTrue(TEXT("an over-cap slow still produces metrics"),
             UItemTooltipVM::CalculateWeaponAttackMetrics(
                 60.0f, -0.9f, 0.75f, 0.8f, 1.4f, LowClamp));
    TestTrue(TEXT("the lower combat play-rate clamp is reflected in APS"),
             FMath::IsNearlyEqual(
                 LowClamp.AttacksPerSecond,
                 (1.0f / 0.75f) * 0.8f,
                 UE_KINDA_SMALL_NUMBER));
    TestTrue(TEXT("the lower combat play-rate clamp is reflected in attack time"),
             FMath::IsNearlyEqual(
                 LowClamp.AttackTimeSeconds,
                 0.75f / 0.8f,
                 UE_KINDA_SMALL_NUMBER));

    const auto ExpectRejectedAndReset = [this](
        const TCHAR *Label,
        const float DamagePerHit,
        const float AttackSpeedBonus,
        const float Duration,
        const float MinimumPlayRate,
        const float MaximumPlayRate) {
        FMythicWeaponAttackViewData Rejected;
        Rejected.bIsValid = true;
        Rejected.MinimumDamagePerHit = 1.0f;
        Rejected.MaximumDamagePerHit = 2.0f;
        Rejected.AverageDamagePerHit = 1.5f;
        Rejected.AttacksPerSecond = 3.0f;
        Rejected.DamagePerSecond = 4.0f;
        Rejected.DamagePerHitText = FText::FromString(TEXT("stale"));
        TestFalse(Label, UItemTooltipVM::CalculateWeaponAttackMetrics(
                             DamagePerHit,
                             AttackSpeedBonus,
                             Duration,
                             MinimumPlayRate,
                             MaximumPlayRate,
                             Rejected));
        TestFalse(TEXT("a rejected metric projection is invalid"), Rejected.bIsValid);
        TestEqual(TEXT("a rejected metric projection clears minimum damage"),
                  Rejected.MinimumDamagePerHit, 0.0f);
        TestEqual(TEXT("a rejected metric projection clears maximum damage"),
                  Rejected.MaximumDamagePerHit, 0.0f);
        TestEqual(TEXT("a rejected metric projection clears average damage"),
                  Rejected.AverageDamagePerHit, 0.0f);
        TestEqual(TEXT("a rejected metric projection clears APS"),
                  Rejected.AttacksPerSecond, 0.0f);
        TestEqual(TEXT("a rejected metric projection clears DPS"),
                  Rejected.DamagePerSecond, 0.0f);
        TestTrue(TEXT("a rejected metric projection clears display text"),
                 Rejected.DamagePerHitText.IsEmpty());
    };

    ExpectRejectedAndReset(TEXT("zero attack-cycle duration is rejected"),
                           60.0f, 0.2f, 0.0f, 0.8f, 1.4f);
    ExpectRejectedAndReset(TEXT("negative attack-cycle duration is rejected"),
                           60.0f, 0.2f, -0.75f, 0.8f, 1.4f);
    ExpectRejectedAndReset(
        TEXT("NaN attack-cycle duration is rejected"),
        60.0f, 0.2f, std::numeric_limits<float>::quiet_NaN(), 0.8f, 1.4f);
    ExpectRejectedAndReset(
        TEXT("infinite attack-cycle duration is rejected"),
        60.0f, 0.2f, std::numeric_limits<float>::infinity(), 0.8f, 1.4f);
    ExpectRejectedAndReset(
        TEXT("non-finite damage is rejected"),
        std::numeric_limits<float>::quiet_NaN(), 0.2f, 0.75f, 0.8f, 1.4f);
    ExpectRejectedAndReset(
        TEXT("non-finite attack-speed bonus is rejected"),
        60.0f, std::numeric_limits<float>::infinity(), 0.75f, 0.8f, 1.4f);
    ExpectRejectedAndReset(
        TEXT("non-finite minimum play rate is rejected"),
        60.0f, 0.2f, 0.75f, std::numeric_limits<float>::quiet_NaN(), 1.4f);
    ExpectRejectedAndReset(
        TEXT("non-finite maximum play rate is rejected"),
        60.0f, 0.2f, 0.75f, 0.8f, std::numeric_limits<float>::infinity());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWeaponAttackAttributeTest,
    "Mythic.UI.ItemDetails.WeaponAttackAttributes",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWeaponAttackAttributeTest::RunTest(const FString &Parameters) {
    TestTrue(TEXT("DamagePerHit belongs to the dedicated weapon attack block"),
             UItemTooltipVM::IsWeaponAttackAttribute(
                 UMythicAttributeSet_Offense::GetDamagePerHitAttribute()));
    TestTrue(TEXT("AttackSpeed belongs to the dedicated weapon attack block"),
             UItemTooltipVM::IsWeaponAttackAttribute(
                 UMythicAttributeSet_Offense::GetAttackSpeedAttribute()));
    TestFalse(TEXT("a normal defensive stat remains in the regular stat rows"),
              UItemTooltipVM::IsWeaponAttackAttribute(
                  UMythicAttributeSet_Defense::GetArmorAttribute()));
    TestFalse(TEXT("an invalid GAS attribute is never claimed by the attack block"),
              UItemTooltipVM::IsWeaponAttackAttribute(FGameplayAttribute()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWeaponAttackComparisonTest,
    "Mythic.UI.ItemComparison.WeaponAttack",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWeaponAttackComparisonTest::RunTest(const FString &Parameters) {
    FMythicWeaponAttackViewData InspectedAttack;
    TestTrue(TEXT("the inspected weapon metrics can be composed"),
             UItemTooltipVM::CalculateWeaponAttackMetrics(
                 120.0f, 0.3f, 1.0f, 0.8f, 1.4f, InspectedAttack));
    InspectedAttack.bIsValid = true;

    FMythicWeaponAttackViewData EquippedAttack;
    TestTrue(TEXT("the equipped weapon metrics can be composed"),
             UItemTooltipVM::CalculateWeaponAttackMetrics(
                 50.0f, 0.1f, 0.5f, 0.8f, 1.4f, EquippedAttack));
    EquippedAttack.bIsValid = true;

    UItemTooltipVM *Tooltip = NewObject<UItemTooltipVM>(GetTransientPackage());
    Tooltip->SetWeaponAttack(InspectedAttack);
    TestTrue(TEXT("the tooltip stores the canonical weapon projection atomically"),
             Tooltip->GetWeaponAttack() == InspectedAttack);

    const FMythicWeaponAttackComparisonViewData Comparison =
        UItemComparisonVM::BuildWeaponAttackComparison(InspectedAttack, EquippedAttack);
    TestTrue(TEXT("two valid weapon projections produce a typed comparison"),
             Comparison.bIsValid);
    TestTrue(TEXT("the comparison records that the replacement slot has a weapon"),
             Comparison.bHasEquippedWeaponAttack);
    TestTrue(TEXT("an occupied weapon target produces inline metric deltas"),
             Comparison.bHasComparisonDeltas);
    TestTrue(TEXT("the inspected canonical projection is retained"),
             Comparison.InspectedAttack == InspectedAttack);
    TestTrue(TEXT("the equipped canonical projection is retained"),
             Comparison.EquippedAttack == EquippedAttack);

    TestEqual(TEXT("average hit uses the canonical item-metric identity"),
              Comparison.AverageDamagePerHitComparison.ComparisonTag,
              ITEM_METRIC_WEAPON_AVERAGE_DAMAGE_PER_HIT.GetTag());
    TestTrue(TEXT("average hit uses the combat range expectation"),
             FMath::IsNearlyEqual(
                 Comparison.AverageDamagePerHitComparison.NewValue,
                 InspectedAttack.AverageDamagePerHit,
                 UE_KINDA_SMALL_NUMBER));
    TestTrue(TEXT("average-hit baseline comes from the equipped combat range expectation"),
             FMath::IsNearlyEqual(
                 Comparison.AverageDamagePerHitComparison.CurrentValue,
                 EquippedAttack.AverageDamagePerHit,
                 UE_KINDA_SMALL_NUMBER));
    TestTrue(TEXT("average-hit delta is candidate minus equipped"),
             FMath::IsNearlyEqual(
                 Comparison.AverageDamagePerHitComparison.Delta,
                 MythicStatDisplay::QuantizeValueToDisplayPrecision(
                     InspectedAttack.AverageDamagePerHit,
                     InspectedAttack.DamageNumberPresentation)
                     - MythicStatDisplay::QuantizeValueToDisplayPrecision(
                         EquippedAttack.AverageDamagePerHit,
                         InspectedAttack.DamageNumberPresentation),
                 UE_KINDA_SMALL_NUMBER));
    TestEqual(TEXT("the higher average hit moves upward"),
              Comparison.AverageDamagePerHitComparison.Movement,
              EMythicStatValueMovement::Increase);
    TestEqual(TEXT("the higher average hit is beneficial"),
              Comparison.AverageDamagePerHitComparison.Verdict,
              EMythicComparisonVerdict::Better);

    TestEqual(TEXT("DPS uses the canonical item-metric identity"),
              Comparison.DamagePerSecondComparison.ComparisonTag,
              ITEM_METRIC_WEAPON_DAMAGE_PER_SECOND.GetTag());
    TestTrue(TEXT("DPS new value comes from the inspected canonical projection"),
             FMath::IsNearlyEqual(
                 Comparison.DamagePerSecondComparison.NewValue,
                 InspectedAttack.DamagePerSecond,
                 UE_KINDA_SMALL_NUMBER));
    TestTrue(TEXT("DPS current value comes from the equipped canonical projection"),
             FMath::IsNearlyEqual(
                 Comparison.DamagePerSecondComparison.CurrentValue,
                 EquippedAttack.DamagePerSecond,
                 UE_KINDA_SMALL_NUMBER));
    TestTrue(TEXT("DPS delta is inspected minus equipped"),
             FMath::IsNearlyEqual(
                 Comparison.DamagePerSecondComparison.Delta,
                 MythicStatDisplay::QuantizeValueToDisplayPrecision(
                     InspectedAttack.DamagePerSecond,
                     InspectedAttack.DamageNumberPresentation)
                     - MythicStatDisplay::QuantizeValueToDisplayPrecision(
                         EquippedAttack.DamagePerSecond,
                         InspectedAttack.DamageNumberPresentation),
                 UE_KINDA_SMALL_NUMBER));
    TestEqual(TEXT("the higher-DPS candidate has increasing numeric movement"),
              Comparison.DamagePerSecondComparison.Movement,
              EMythicStatValueMovement::Increase);
    TestEqual(TEXT("the higher-DPS candidate is independently better"),
              Comparison.DamagePerSecondComparison.Verdict,
              EMythicComparisonVerdict::Better);
    TestFalse(TEXT("DPS comparison carries a canonical formatted delta"),
              Comparison.DamagePerSecondComparison.FormattedDelta.IsEmpty());

    TestEqual(TEXT("effective APS uses the canonical item-metric identity"),
              Comparison.EffectiveAttacksPerSecondComparison.ComparisonTag,
              ITEM_METRIC_WEAPON_ATTACKS_PER_SECOND.GetTag());
    TestTrue(TEXT("APS new value uses post-clamp effective cadence"),
             FMath::IsNearlyEqual(
                 Comparison.EffectiveAttacksPerSecondComparison.NewValue,
                 InspectedAttack.AttacksPerSecond,
                 UE_KINDA_SMALL_NUMBER));
    TestFalse(TEXT("APS comparison never falls back to raw authored cadence"),
              FMath::IsNearlyEqual(
                  Comparison.EffectiveAttacksPerSecondComparison.NewValue,
                  InspectedAttack.BaseAttacksPerSecond,
                  UE_KINDA_SMALL_NUMBER));
    TestTrue(TEXT("APS delta is inspected minus equipped"),
             FMath::IsNearlyEqual(
                 Comparison.EffectiveAttacksPerSecondComparison.Delta,
                 MythicStatDisplay::QuantizeValueToDisplayPrecision(
                     InspectedAttack.AttacksPerSecond,
                     InspectedAttack.AttacksPerSecondNumberPresentation)
                     - MythicStatDisplay::QuantizeValueToDisplayPrecision(
                         EquippedAttack.AttacksPerSecond,
                         InspectedAttack.AttacksPerSecondNumberPresentation),
                 UE_KINDA_SMALL_NUMBER));
    TestEqual(TEXT("a slower effective cadence has decreasing numeric movement"),
              Comparison.EffectiveAttacksPerSecondComparison.Movement,
              EMythicStatValueMovement::Decrease);
    TestEqual(TEXT("a slower effective cadence is independently worse"),
              Comparison.EffectiveAttacksPerSecondComparison.Verdict,
              EMythicComparisonVerdict::Worse);

    const FMythicWeaponAttackComparisonViewData EmptySlotComparison =
        UItemComparisonVM::BuildWeaponAttackComparison(
            InspectedAttack, FMythicWeaponAttackViewData(), false);
    TestTrue(TEXT("a core caller can explicitly project candidate-only weapon metrics"),
             EmptySlotComparison.bIsValid);
    TestFalse(TEXT("an empty slot is not reported as an equipped weapon"),
              EmptySlotComparison.bHasEquippedWeaponAttack);
    TestTrue(TEXT("an explicit zero-baseline request emits inline metric deltas"),
             EmptySlotComparison.bHasComparisonDeltas);
    TestEqual(TEXT("empty-slot DPS uses the zero item-metric identity"),
              EmptySlotComparison.DamagePerSecondComparison.CurrentValue, 0.0f);
    TestEqual(TEXT("empty-slot APS uses the zero item-metric identity"),
              EmptySlotComparison.EffectiveAttacksPerSecondComparison.CurrentValue, 0.0f);
    TestTrue(TEXT("positive candidate DPS upgrades an empty slot"),
             EmptySlotComparison.DamagePerSecondComparison.bIsUpgrade);
    TestTrue(TEXT("positive candidate APS upgrades an empty slot"),
             EmptySlotComparison.EffectiveAttacksPerSecondComparison.bIsUpgrade);

    const FMythicWeaponAttackComparisonViewData SuppressedEmptySlotComparison =
        UItemComparisonVM::BuildWeaponAttackComparison(
            InspectedAttack, FMythicWeaponAttackViewData());
    TestTrue(TEXT("default empty-baseline suppression retains the valid candidate attack block"),
             SuppressedEmptySlotComparison.bIsValid);
    TestFalse(TEXT("empty-baseline suppression still reports no equipped weapon"),
              SuppressedEmptySlotComparison.bHasEquippedWeaponAttack);
    TestFalse(TEXT("empty-baseline suppression emits no misleading green deltas"),
              SuppressedEmptySlotComparison.bHasComparisonDeltas);
    TestFalse(TEXT("suppressed average-hit comparison has no metric identity"),
              SuppressedEmptySlotComparison.AverageDamagePerHitComparison.ComparisonTag.IsValid());
    TestFalse(TEXT("suppressed DPS comparison has no metric identity"),
              SuppressedEmptySlotComparison.DamagePerSecondComparison.ComparisonTag.IsValid());
    TestFalse(TEXT("suppressed APS comparison has no metric identity"),
              SuppressedEmptySlotComparison.EffectiveAttacksPerSecondComparison.ComparisonTag.IsValid());

    FMythicWeaponAttackViewData InvalidInspected = InspectedAttack;
    InvalidInspected.bIsValid = false;
    TestFalse(TEXT("an invalid inspected projection rejects the whole typed comparison"),
              UItemComparisonVM::BuildWeaponAttackComparison(
                  InvalidInspected, EquippedAttack).bIsValid);

    FMythicWeaponAttackViewData NonFiniteInspected = InspectedAttack;
    NonFiniteInspected.DamagePerSecond = std::numeric_limits<float>::infinity();
    TestFalse(TEXT("a corrupt non-finite inspected projection cannot leak into comparison UI"),
              UItemComparisonVM::BuildWeaponAttackComparison(
                  NonFiniteInspected, EquippedAttack).bIsValid);

    FMythicWeaponAttackViewData CorruptRange = InspectedAttack;
    CorruptRange.MaximumDamagePerHit = CorruptRange.MinimumDamagePerHit - 1.0f;
    TestFalse(TEXT("a corrupt inverted damage range rejects the atomic comparison projection"),
              UItemComparisonVM::BuildWeaponAttackComparison(
                  CorruptRange, EquippedAttack).bIsValid);
    return true;
}

#if WITH_EDITOR

#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Itemization/Inventory/Fragments/Actionable/AttackFragment.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAttackFragmentNominalCycleDurationTest,
    "Mythic.Itemization.AttackFragment.NominalCycleDuration",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAttackFragmentNominalCycleDurationTest::RunTest(const FString &Parameters) {
    TestEqual(TEXT("a null montage has no attack cycle"),
              UAttackFragment::GetNominalAttackCycleDuration(nullptr), 0.0f);

    UAnimMontage *Montage = NewObject<UAnimMontage>(GetTransientPackage());
    if (!TestNotNull(TEXT("a transient montage can be constructed"), Montage)) {
        return false;
    }
    TestEqual(TEXT("a montage without sections has no attack cycle"),
              UAttackFragment::GetNominalAttackCycleDuration(Montage), 0.0f);

    FFloatProperty *SequenceLengthProperty = FindFProperty<FFloatProperty>(
        UAnimSequenceBase::StaticClass(), TEXT("SequenceLength"));
    if (!TestNotNull(TEXT("the animation sequence-length property exists"),
                     SequenceLengthProperty)) {
        return false;
    }
    SequenceLengthProperty->SetPropertyValue_InContainer(Montage, 3.0f);
    Montage->AddSlot(FName(TEXT("MythicTestSlot")));

    const int32 Opening = Montage->AddAnimCompositeSection(
        FName(TEXT("OpeningArc")), 0.0f);
    const int32 Backhand = Montage->AddAnimCompositeSection(
        FName(TEXT("Backhand_Heavy")), 0.5f);
    const int32 Finale = Montage->AddAnimCompositeSection(
        FName(TEXT("Finale")), 1.5f);
    if (!TestTrue(TEXT("arbitrarily named attack sections were authored"),
                  Opening != INDEX_NONE && Backhand != INDEX_NONE && Finale != INDEX_NONE)) {
        return false;
    }

    // AddAnimCompositeSection links adjacent sections for editor convenience. Attack variants are deliberately
    // standalone, so clear those links before checking the uniformly weighted cycle contract.
    for (int32 SectionIndex = 0; SectionIndex < Montage->GetNumSections(); ++SectionIndex) {
        Montage->GetAnimCompositeSection(SectionIndex).NextSectionName = NAME_None;
    }
    TestTrue(TEXT("arbitrary section names resolve through the live montage table"),
             FMath::IsNearlyEqual(
                 UAttackFragment::GetNominalAttackCycleDuration(Montage),
                 1.0f,
                 UE_KINDA_SMALL_NUMBER));

    Montage->RateScale = 0.0f;
    TestEqual(TEXT("zero asset Rate Scale cannot create a stalled tooltip cadence"),
              UAttackFragment::GetNominalAttackCycleDuration(Montage), 0.0f);
    Montage->RateScale = 0.5f;
    TestEqual(TEXT("a hidden finite Rate Scale multiplier invalidates the tooltip cadence"),
              UAttackFragment::GetNominalAttackCycleDuration(Montage), 0.0f);
    Montage->RateScale = std::numeric_limits<float>::quiet_NaN();
    TestEqual(TEXT("NaN asset Rate Scale invalidates the tooltip cadence"),
              UAttackFragment::GetNominalAttackCycleDuration(Montage), 0.0f);
    Montage->RateScale = std::numeric_limits<float>::infinity();
    TestEqual(TEXT("infinite asset Rate Scale invalidates the tooltip cadence"),
              UAttackFragment::GetNominalAttackCycleDuration(Montage), 0.0f);

    FText RateScaleError;
    TestFalse(TEXT("authoring validation rejects a non-unit global Rate Scale"),
              UAttackFragment::IsAttackMontageContractValid(Montage,
                                                            &RateScaleError));
    TestTrue(TEXT("authoring validation explains the single cadence owner"),
             RateScaleError.ToString().Contains(TEXT("Rate Scale")));

    Montage->RateScale = 1.0f;
    TestTrue(TEXT("restoring unit Rate Scale restores the shared raw cycle"),
             FMath::IsNearlyEqual(
                 UAttackFragment::GetNominalAttackCycleDuration(Montage),
                 1.0f,
                 UE_KINDA_SMALL_NUMBER));

    Montage->GetAnimCompositeSection(0).NextSectionName = Montage->GetSectionName(1);
    TestEqual(TEXT("a linked section is rejected because it is not one complete attack variant"),
              UAttackFragment::GetNominalAttackCycleDuration(Montage), 0.0f);
    return true;
}

#endif // WITH_EDITOR
