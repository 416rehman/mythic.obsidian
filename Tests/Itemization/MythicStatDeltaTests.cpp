#include "Misc/AutomationTest.h"
#include "Itemization/Inventory/ViewModels/MythicStatDelta.h"
#include "Itemization/Inventory/ViewModels/MythicTags_ItemMetrics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatDeltaTest,
    "Mythic.Itemization.StatDelta",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatDeltaTest::RunTest(const FString &Parameters) {
    using Core = FMythicStatDeltaCore;

    const auto MakePresentation = [](
        const EMythicStatFormat Format,
        const int32 DecimalPlaces,
        const FText &UnitSuffix = FText::GetEmpty()) {
        FMythicStatNumberPresentation Presentation;
        Presentation.Format = Format;
        Presentation.DecimalPlaces = DecimalPlaces;
        Presentation.UnitSuffix = UnitSuffix;
        return Presentation;
    };

    const FGameplayTag KeyPrimary = ITEM_METRIC_WEAPON_DAMAGE_PER_SECOND.GetTag();
    const FGameplayTag KeyAttackSpeed = ITEM_METRIC_WEAPON_ATTACKS_PER_SECOND.GetTag();
    const FGameplayTag KeyAverageHit = ITEM_METRIC_WEAPON_AVERAGE_DAMAGE_PER_HIT.GetTag();
    const FGameplayTag KeySecondary = ITEM_METRIC_DURABILITY.GetTag();
    const FText Label = FText::FromString(TEXT("Stat"));
    const FMythicStatNumberPresentation FlatOne = MakePresentation(
        EMythicStatFormat::Flat, 1);
    if (!TestTrue(TEXT("native comparison identities are registered"),
                  KeyPrimary.IsValid() && KeyAttackSpeed.IsValid()
                      && KeyAverageHit.IsValid() && KeySecondary.IsValid())) {
        return false;
    }

    {
        TArray<FMythicComparableStat> New, Current;
        New.Emplace(KeyPrimary, Label, 10.0f, 0.0f,
                    EMythicStatComparisonDirection::HigherIsBetter, FlatOne);
        Current.Emplace(KeyPrimary, Label, 6.0f, 0.0f,
                        EMythicStatComparisonDirection::HigherIsBetter, FlatOne);
        const TArray<FAttributeDiff> Diffs = Core::ComputeDiffs(New, Current);
        TestEqual(TEXT("one shared tag produces one row"), Diffs.Num(), 1);
        if (Diffs.Num() == 1) {
            const FAttributeDiff &Diff = Diffs[0];
            TestEqual(TEXT("identity remains the canonical tag"), Diff.ComparisonTag, KeyPrimary);
            TestEqual(TEXT("new value carried"), Diff.NewValue, 10.0f);
            TestEqual(TEXT("current value carried"), Diff.CurrentValue, 6.0f);
            TestEqual(TEXT("delta is new minus current"), Diff.Delta, 4.0f);
            TestEqual(TEXT("positive numeric change is an increase"), Diff.Movement,
                      EMythicStatValueMovement::Increase);
            TestEqual(TEXT("higher-is-better increase is better"), Diff.Verdict,
                      EMythicComparisonVerdict::Better);
            TestTrue(TEXT("compatibility upgrade flag mirrors the verdict"), Diff.bIsUpgrade);
            TestTrue(TEXT("canonical current formatting is retained"),
                     Diff.FormattedCurrentValue.EqualTo(
                         MythicStatDisplay::FormatValue(6.0f, FlatOne)));
            TestTrue(TEXT("canonical signed delta formatting is retained"),
                     Diff.FormattedDelta.EqualTo(
                         MythicStatDisplay::FormatBonus(4.0f, FlatOne)));
            TestFalse(TEXT("accessible comparison summary is populated"),
                      Diff.AccessibleSummary.IsEmpty());
        }
    }

    {
        TArray<FMythicComparableStat> New, Current;
        New.Emplace(KeyPrimary, Label, 4.0f, 0.0f,
                    EMythicStatComparisonDirection::LowerIsBetter, FlatOne);
        Current.Emplace(KeyPrimary, Label, 6.0f, 0.0f,
                        EMythicStatComparisonDirection::LowerIsBetter, FlatOne);
        const TArray<FAttributeDiff> Diffs = Core::ComputeDiffs(New, Current);
        TestEqual(TEXT("lower-is-better produces one row"), Diffs.Num(), 1);
        if (Diffs.Num() == 1) {
            TestEqual(TEXT("numeric movement remains a decrease"), Diffs[0].Movement,
                      EMythicStatValueMovement::Decrease);
            TestEqual(TEXT("lower-is-better decrease is independently better"), Diffs[0].Verdict,
                      EMythicComparisonVerdict::Better);
        }
    }

    {
        TArray<FMythicComparableStat> New, Current;
        New.Emplace(KeyPrimary, Label, 9.0f, 0.0f,
                    EMythicStatComparisonDirection::Neutral, FlatOne);
        Current.Emplace(KeyPrimary, Label, 6.0f, 0.0f,
                        EMythicStatComparisonDirection::Neutral, FlatOne);
        const TArray<FAttributeDiff> Diffs = Core::ComputeDiffs(New, Current);
        if (TestEqual(TEXT("neutral-direction change produces one row"), Diffs.Num(), 1)) {
            TestEqual(TEXT("neutral direction does not erase numeric movement"), Diffs[0].Movement,
                      EMythicStatValueMovement::Increase);
            TestEqual(TEXT("neutral direction never manufactures green or red"), Diffs[0].Verdict,
                      EMythicComparisonVerdict::Neutral);
        }
    }

    {
        TArray<FMythicComparableStat> New, Current;
        New.Emplace(KeyPrimary, Label, 12.0f, 0.0f,
                    EMythicStatComparisonDirection::HigherIsBetter, FlatOne);
        Current.Emplace(KeySecondary, Label, 8.0f, 0.0f,
                        EMythicStatComparisonDirection::HigherIsBetter, FlatOne);
        const TArray<FAttributeDiff> Diffs = Core::ComputeDiffs(New, Current);
        TestEqual(TEXT("tag union emits both one-sided rows"), Diffs.Num(), 2);
        if (Diffs.Num() == 2) {
            TestTrue(TEXT("gained stat is candidate-only"), Diffs[0].bCandidateOnly);
            TestFalse(TEXT("gained stat is not baseline-only"), Diffs[0].bBaselineOnly);
            TestEqual(TEXT("gained stat baselines at contribution identity"),
                      Diffs[0].CurrentValue, 0.0f);
            TestEqual(TEXT("gained stat displays a missing equipped value"),
                      Diffs[0].FormattedCurrentValue.ToString(), FString(TEXT("\u2014")));
            TestTrue(TEXT("lost stat is baseline-only"), Diffs[1].bBaselineOnly);
            TestFalse(TEXT("lost stat is not candidate-only"), Diffs[1].bCandidateOnly);
            TestEqual(TEXT("lost stat ends at contribution identity"),
                      Diffs[1].NewValue, 0.0f);
            TestEqual(TEXT("lost stat displays a missing candidate value"),
                      Diffs[1].FormattedNewValue.ToString(), FString(TEXT("\u2014")));
            TestEqual(TEXT("lost higher-is-better stat is worse"), Diffs[1].Verdict,
                      EMythicComparisonVerdict::Worse);
        }
    }

    {
        TArray<FMythicComparableStat> New, Current;
        New.Emplace(KeySecondary, Label, 5.0f, 0.0f,
                    EMythicStatComparisonDirection::HigherIsBetter, FlatOne);
        New.Emplace(KeySecondary, Label, 5.0f, 0.0f,
                    EMythicStatComparisonDirection::HigherIsBetter, FlatOne);
        Current.Emplace(KeySecondary, Label, 7.0f, 0.0f,
                        EMythicStatComparisonDirection::HigherIsBetter, FlatOne);
        const TArray<FAttributeDiff> Diffs = Core::ComputeDiffs(New, Current);
        if (TestEqual(TEXT("duplicate same-side tags fold to one row"), Diffs.Num(), 1)) {
            TestEqual(TEXT("zero-neutral contributions sum"), Diffs[0].NewValue, 10.0f);
            TestEqual(TEXT("summed delta"), Diffs[0].Delta, 3.0f);
            TestTrue(TEXT("folded row is marked as a net aggregate"),
                     Diffs[0].bAggregatedFromMultipleContributions);
        }
    }

    {
        const FMythicStatNumberPresentation Multiplier = MakePresentation(
            EMythicStatFormat::Multiplier, 1);
        TArray<FMythicComparableStat> New;
        New.Emplace(KeyPrimary, Label, 0.97f, 1.0f,
                    EMythicStatComparisonDirection::LowerIsBetter, Multiplier);
        const TArray<FAttributeDiff> Diffs = Core::ComputeDiffs(
            New, TArray<FMythicComparableStat>());
        if (TestEqual(TEXT("one-neutral multiplier produces one row"), Diffs.Num(), 1)) {
            TestEqual(TEXT("missing multiplier side baselines at authored one"),
                      Diffs[0].CurrentValue, 1.0f);
            TestEqual(TEXT("multiplier delta preserves the three-percent reduction"),
                      Diffs[0].Delta, -0.03f, 0.0001f);
            TestEqual(TEXT("lower incoming multiplier is better"), Diffs[0].Verdict,
                      EMythicComparisonVerdict::Better);
            TestTrue(TEXT("multiplier delta uses canonical percent-point formatting"),
                     Diffs[0].FormattedDelta.EqualTo(
                         MythicStatDisplay::FormatBonus(-0.03f, Multiplier)));
        }
    }

    {
        const FMythicStatNumberPresentation PercentOne = MakePresentation(
            EMythicStatFormat::Percent, 1);
        TArray<FMythicComparableStat> New;
        // Additive contributions retain a zero operation identity even when displayed as percent.
        New.Emplace(KeyPrimary, Label, 0.10f, 0.0f,
                    EMythicStatComparisonDirection::HigherIsBetter, PercentOne);
        const TArray<FAttributeDiff> Diffs = Core::ComputeDiffs(
            New, TArray<FMythicComparableStat>());
        if (TestEqual(TEXT("additive percent contribution produces one row"), Diffs.Num(), 1)) {
            TestEqual(TEXT("additive percent contribution retains zero identity"),
                      Diffs[0].ContributionIdentity, 0.0f);
            TestEqual(TEXT("additive percent contribution retains its numeric offset"),
                      Diffs[0].Delta, 0.10f, 0.0001f);
            TestTrue(TEXT("additive percent delta uses canonical scaled formatting"),
                     Diffs[0].FormattedDelta.EqualTo(
                         MythicStatDisplay::FormatBonus(0.10f, PercentOne)));
        }
    }

    {
        const FMythicStatNumberPresentation FlatZero = MakePresentation(
            EMythicStatFormat::Flat, 0);
        TArray<FMythicComparableStat> New, Current;
        New.Emplace(KeyPrimary, Label, 10.49f, 0.0f,
                    EMythicStatComparisonDirection::HigherIsBetter, FlatZero);
        Current.Emplace(KeyPrimary, Label, 10.0f, 0.0f,
                        EMythicStatComparisonDirection::HigherIsBetter, FlatZero);
        const TArray<FAttributeDiff> Diffs = Core::ComputeDiffs(New, Current);
        if (TestEqual(TEXT("sub-display-precision delta produces one row"), Diffs.Num(), 1)) {
            TestEqual(TEXT("epsilon-equal values have equal movement"), Diffs[0].Movement,
                      EMythicStatValueMovement::Equal);
            TestEqual(TEXT("epsilon-equal values have neutral verdict"), Diffs[0].Verdict,
                      EMythicComparisonVerdict::Neutral);
            TestTrue(TEXT("epsilon-equal signed delta is collapsed"),
                     Diffs[0].FormattedDelta.IsEmpty());
        }
    }

    {
        const FMythicStatNumberPresentation FlatZero = MakePresentation(
            EMythicStatFormat::Flat, 0);
        TArray<FMythicComparableStat> New, Current;
        New.Emplace(KeyPrimary, Label, 1.49f, 0.0f,
                    EMythicStatComparisonDirection::HigherIsBetter, FlatZero);
        Current.Emplace(KeyPrimary, Label, 0.51f, 0.0f,
                        EMythicStatComparisonDirection::HigherIsBetter, FlatZero);
        const TArray<FAttributeDiff> Diffs = Core::ComputeDiffs(New, Current);
        if (TestEqual(TEXT("same display bucket produces one row"), Diffs.Num(), 1)) {
            TestTrue(TEXT("both endpoints visibly render as one"),
                     Diffs[0].FormattedNewValue.EqualTo(Diffs[0].FormattedCurrentValue));
            TestEqual(TEXT("same displayed endpoints have zero display delta"),
                      Diffs[0].Delta, 0.0f);
            TestEqual(TEXT("same displayed endpoints have equal movement"),
                      Diffs[0].Movement, EMythicStatValueMovement::Equal);
            TestTrue(TEXT("same displayed endpoints suppress the delta chip"),
                     Diffs[0].FormattedDelta.IsEmpty());
        }
    }

    {
        const FMythicStatNumberPresentation FlatZero = MakePresentation(
            EMythicStatFormat::Flat, 0);
        TArray<FMythicComparableStat> New, Current;
        New.Emplace(KeyPrimary, Label, 0.51f, 0.0f,
                    EMythicStatComparisonDirection::HigherIsBetter, FlatZero);
        Current.Emplace(KeyPrimary, Label, 0.49f, 0.0f,
                        EMythicStatComparisonDirection::HigherIsBetter, FlatZero);
        const TArray<FAttributeDiff> Diffs = Core::ComputeDiffs(New, Current);
        if (TestEqual(TEXT("display bucket crossing produces one row"), Diffs.Num(), 1)) {
            TestFalse(TEXT("bucket-crossing endpoints visibly differ"),
                      Diffs[0].FormattedNewValue.EqualTo(Diffs[0].FormattedCurrentValue));
            TestEqual(TEXT("display delta matches the rendered endpoints"),
                      Diffs[0].Delta, 1.0f);
            TestEqual(TEXT("visible bucket crossing is an increase"),
                      Diffs[0].Movement, EMythicStatValueMovement::Increase);
            TestTrue(TEXT("bucket crossing renders the canonical signed delta"),
                     Diffs[0].FormattedDelta.EqualTo(
                         MythicStatDisplay::FormatBonus(1.0f, FlatZero)));
        }
    }

    {
        TArray<FMythicComparableStat> New, Current;
        New.Emplace(KeyPrimary, Label, 0.2f, 0.0f,
                    EMythicStatComparisonDirection::HigherIsBetter, FlatOne);
        New.Emplace(KeyPrimary, Label, 1.3f, 1.0f,
                    EMythicStatComparisonDirection::HigherIsBetter, FlatOne);
        Current.Emplace(KeyPrimary, Label, 1.3f, 1.0f,
                        EMythicStatComparisonDirection::HigherIsBetter, FlatOne);
        Current.Emplace(KeyPrimary, Label, 0.2f, 0.0f,
                        EMythicStatComparisonDirection::HigherIsBetter, FlatOne);
        const TArray<FAttributeDiff> Diffs = Core::ComputeDiffs(New, Current);
        if (TestEqual(TEXT("reordered mixed identities still produce one disclosure row"),
                      Diffs.Num(), 1)) {
            TestTrue(TEXT("mixed contribution identities are recorded as a conflict"),
                     Diffs[0].bSemanticConflict);
            TestEqual(TEXT("equivalent net offsets are independent of encounter order"),
                      Diffs[0].Delta, 0.0f, 0.0001f);
            TestEqual(TEXT("equivalent conflicted offsets have equal movement"),
                      Diffs[0].Movement, EMythicStatValueMovement::Equal);
            TestEqual(TEXT("conflicted offsets retain a neutral verdict"),
                      Diffs[0].Verdict, EMythicComparisonVerdict::Neutral);
        }
    }

    {
        const FMythicStatNumberPresentation FlatTwo = MakePresentation(
            EMythicStatFormat::Flat, 2);
        const FMythicStatNumberPresentation PercentTwo = MakePresentation(
            EMythicStatFormat::Percent, 2);
        TArray<FMythicComparableStat> New, Current;
        New.Emplace(KeyPrimary, Label, 8.0f, 0.0f,
                    EMythicStatComparisonDirection::HigherIsBetter, FlatTwo);
        Current.Emplace(KeyPrimary, Label, 6.0f, 1.0f,
                        EMythicStatComparisonDirection::LowerIsBetter, PercentTwo);
        const TArray<FAttributeDiff> Diffs = Core::ComputeDiffs(New, Current);
        if (TestEqual(TEXT("conflicting semantics still produce one disclosure row"),
                      Diffs.Num(), 1)) {
            TestTrue(TEXT("identity, format, and direction conflict is recorded"),
                     Diffs[0].bSemanticConflict);
            TestEqual(TEXT("conflict preserves numeric movement"), Diffs[0].Movement,
                      EMythicStatValueMovement::Increase);
            TestEqual(TEXT("conflict forces neutral benefit verdict"), Diffs[0].Verdict,
                      EMythicComparisonVerdict::Neutral);
        }
    }

    {
        FMythicAffixRowPresentation Row;
        Row.ValueDiffs.AddDefaulted(2);
        Row.ValueDiffs[0].ComparisonTag = KeyPrimary;
        Row.ValueDiffs[1].ComparisonTag = KeyAttackSpeed;
        TestEqual(TEXT("affix-row projection retains multiple value-channel deltas"),
                  Row.ValueDiffs.Num(), 2);
    }

    return true;
}
