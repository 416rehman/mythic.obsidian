
#include "Misc/AutomationTest.h"
#include "World/Farming/MythicLivestockGenome.h"

namespace {
FMythicLivestockGenome LSG_MakeGenome(float Yield, float Quality) {
    FMythicLivestockGenome G;
    G.Yield = Yield;
    G.Quality = Quality;
    return G;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicLivestockGenomeTest,
    "Mythic.Farming.Breeding",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicLivestockGenomeTest::RunTest(const FString &Parameters) {
    using Genome = FMythicLivestockGenome;
    using Statics = FMythicLivestockGenomeStatics;
    const FMythicBreedingParams Params;

    {
        const Genome Neutral;
        TestTrue(TEXT("default genome is neutral"), Neutral.IsNeutral());
        TestEqual(TEXT("neutral -> 0 tier bonus (parity)"), Statics::ProduceTierBonusFromGenome(Neutral, Params), 0);
        TestEqual(TEXT("neutral -> 1.0x rate exactly"), Statics::ProductionRateMultiplierFromGenome(Neutral, Params), 1.0f);
        TestEqual(TEXT("neutral -> base interval unchanged"), Statics::EffectiveProduceIntervalSeconds(1800.0f, Neutral, Params), 1800.0f);
        TestFalse(TEXT("non-neutral detected"), LSG_MakeGenome(0.0f, 0.3f).IsNeutral());
    }

    {
        const Genome A = LSG_MakeGenome(0.2f, 0.8f);
        const Genome B = LSG_MakeGenome(0.8f, 0.2f);
        const Genome Mid = Statics::Breed(A, B, 0.5f, Params);
        TestTrue(TEXT("blend Yield -> midpoint"), FMath::IsNearlyEqual(Mid.Yield, 0.5f, 1e-4f));
        TestTrue(TEXT("blend Quality -> midpoint"), FMath::IsNearlyEqual(Mid.Quality, 0.5f, 1e-4f));

        const Genome P = LSG_MakeGenome(0.4f, 0.6f);
        const Genome Clone = Statics::Breed(P, P, 0.5f, Params);
        TestTrue(TEXT("identical parents, no mut -> Yield == parent"), FMath::IsNearlyEqual(Clone.Yield, 0.4f, 1e-4f));
        TestTrue(TEXT("identical parents, no mut -> Quality == parent"), FMath::IsNearlyEqual(Clone.Quality, 0.6f, 1e-4f));

        TestTrue(TEXT("blend between parents (Yield)"), Mid.Yield >= 0.2f - 1e-4f && Mid.Yield <= 0.8f + 1e-4f);
    }

    {
        const Genome P = LSG_MakeGenome(0.4f, 0.4f);
        TestTrue(TEXT("roll 1.0 -> +mutation"), FMath::IsNearlyEqual(Statics::Breed(P, P, 1.0f, Params).Yield, 0.45f, 1e-4f));
        TestTrue(TEXT("roll 0.0 -> -mutation"), FMath::IsNearlyEqual(Statics::Breed(P, P, 0.0f, Params).Yield, 0.35f, 1e-4f));

        for (int32 i = 0; i <= 10; ++i) {
            const float Roll = static_cast<float>(i) / 10.0f;
            const float Trait = Statics::Breed(P, P, Roll, Params).Yield;
            TestTrue(TEXT("mutation bounded by magnitude"), FMath::Abs(Trait - 0.4f) <= Params.MutationMagnitude + 1e-4f);
        }

        const Genome Maxed = LSG_MakeGenome(1.0f, 1.0f);
        TestTrue(TEXT("clamp at TraitMax"), Statics::Breed(Maxed, Maxed, 1.0f, Params).Yield <= 1.0f + 1e-6f);
        TestTrue(TEXT("clamp keeps <= 1"), FMath::IsNearlyEqual(Statics::Breed(Maxed, Maxed, 1.0f, Params).Yield, 1.0f, 1e-4f));
        const Genome Floored;
        TestTrue(TEXT("clamp at TraitMin"), Statics::Breed(Floored, Floored, 0.0f, Params).Yield >= 0.0f);
        TestTrue(TEXT("clamp keeps >= 0"), FMath::IsNearlyEqual(Statics::Breed(Floored, Floored, 0.0f, Params).Quality, 0.0f, 1e-4f));
    }

    {
        const Genome P = LSG_MakeGenome(0.4f, 0.4f);
        const float Rolls[2] = {1.0f, 0.0f};
        const Genome Child = Statics::Breed(P, P, TConstArrayView<float>(Rolls, 2), Params);
        TestTrue(TEXT("array roll: Yield up"), FMath::IsNearlyEqual(Child.Yield, 0.45f, 1e-4f));
        TestTrue(TEXT("array roll: Quality down"), FMath::IsNearlyEqual(Child.Quality, 0.35f, 1e-4f));
        const Genome Broadcast = Statics::Breed(P, P, 1.0f, Params);
        const float Same[2] = {1.0f, 1.0f};
        const Genome ArrayBroadcast = Statics::Breed(P, P, TConstArrayView<float>(Same, 2), Params);
        TestTrue(TEXT("single-float == broadcast (Yield)"), FMath::IsNearlyEqual(Broadcast.Yield, ArrayBroadcast.Yield, 1e-5f));
        TestTrue(TEXT("single-float == broadcast (Quality)"), FMath::IsNearlyEqual(Broadcast.Quality, ArrayBroadcast.Quality, 1e-5f));
    }

    {
        TestEqual(TEXT("Quality 0 -> +0 (parity)"), Statics::ProduceTierBonusFromGenome(LSG_MakeGenome(0.0f, 0.0f), Params), 0);
        TestEqual(TEXT("Quality 0.49 -> +0 (below step)"), Statics::ProduceTierBonusFromGenome(LSG_MakeGenome(0.0f, 0.49f), Params), 0);
        TestEqual(TEXT("Quality 0.5 -> +1 (one step)"), Statics::ProduceTierBonusFromGenome(LSG_MakeGenome(0.0f, 0.5f), Params), 1);
        TestEqual(TEXT("Quality 0.99 -> +1"), Statics::ProduceTierBonusFromGenome(LSG_MakeGenome(0.0f, 0.99f), Params), 1);
        TestEqual(TEXT("Quality 1.0 -> +2 (two steps)"), Statics::ProduceTierBonusFromGenome(LSG_MakeGenome(0.0f, 1.0f), Params), 2);

        int32 Prev = -1;
        for (int32 i = 0; i <= 20; ++i) {
            const float Q = static_cast<float>(i) / 20.0f;
            const int32 Bonus = Statics::ProduceTierBonusFromGenome(LSG_MakeGenome(0.0f, Q), Params);
            TestTrue(TEXT("tier bonus monotonic in Quality"), Bonus >= Prev);
            TestTrue(TEXT("tier bonus within cap"), Bonus <= Params.MaxTierBonus);
            Prev = Bonus;
        }
    }

    {
        TestEqual(TEXT("Yield 0 -> 1.0x (parity)"), Statics::ProductionRateMultiplierFromGenome(LSG_MakeGenome(0.0f, 0.0f), Params), 1.0f);
        TestTrue(TEXT("Yield 0.5 -> 1.25x"), FMath::IsNearlyEqual(Statics::ProductionRateMultiplierFromGenome(LSG_MakeGenome(0.5f, 0.0f), Params), 1.25f, 1e-4f));
        TestTrue(TEXT("Yield 1.0 -> 1.5x"), FMath::IsNearlyEqual(Statics::ProductionRateMultiplierFromGenome(LSG_MakeGenome(1.0f, 0.0f), Params), 1.5f, 1e-4f));

        TestTrue(TEXT("Yield 1.0 -> interval / 1.5"), FMath::IsNearlyEqual(Statics::EffectiveProduceIntervalSeconds(1800.0f, LSG_MakeGenome(1.0f, 0.0f), Params), 1200.0f, 1e-2f));
        TestEqual(TEXT("unauthored interval stays 0"), Statics::EffectiveProduceIntervalSeconds(0.0f, LSG_MakeGenome(1.0f, 0.0f), Params), 0.0f);

        float PrevInterval = 1.0e9f;
        for (int32 i = 0; i <= 10; ++i) {
            const float Y = static_cast<float>(i) / 10.0f;
            const float Interval = Statics::EffectiveProduceIntervalSeconds(1800.0f, LSG_MakeGenome(Y, 0.0f), Params);
            TestTrue(TEXT("effective interval non-increasing in Yield"), Interval <= PrevInterval + 1e-2f);
            PrevInterval = Interval;
        }
    }

    {
        Genome Line;
        const int32 Gen10 = 10, GenMany = 40;
        for (int32 g = 0; g < Gen10; ++g) {
            Line = Statics::Breed(Line, Line, 1.0f, Params);
        }
        TestTrue(TEXT("10 gens of selection -> Quality ~0.5"), Line.Quality >= 0.5f - 1e-3f);
        TestTrue(TEXT("10 gens -> +1 produce tier"), Statics::ProduceTierBonusFromGenome(Line, Params) >= 1);

        for (int32 g = Gen10; g < GenMany; ++g) {
            Line = Statics::Breed(Line, Line, 1.0f, Params);
        }
        TestTrue(TEXT("many gens -> Quality clamps at 1.0"), FMath::IsNearlyEqual(Line.Quality, 1.0f, 1e-3f));
        TestEqual(TEXT("many gens -> +2 tier (cap)"), Statics::ProduceTierBonusFromGenome(Line, Params), 2);
        TestTrue(TEXT("many gens -> 1.5x rate (ceiling)"), FMath::IsNearlyEqual(Statics::ProductionRateMultiplierFromGenome(Line, Params), 1.5f, 1e-3f));
    }

    return true;
}
