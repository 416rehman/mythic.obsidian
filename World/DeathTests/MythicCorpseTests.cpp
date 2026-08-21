
#include "Misc/AutomationTest.h"
#include "World/Death/MythicCorpseTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCorpseTest,
    "Mythic.Death.Corpse",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCorpseTest::RunTest(const FString &Parameters) {
    using Rules = FMythicCorpseRules;

    {
        const float Thresholds[3] = {60.0f, 150.0f, 240.0f};
        const TConstArrayView<float> View(Thresholds, 3);

        TestEqual(TEXT("age 0 → Fresh"), Rules::StageForAge(0.0f, View), EMythicDecompStage::Fresh);
        TestEqual(TEXT("age 59 → Fresh"), Rules::StageForAge(59.0f, View), EMythicDecompStage::Fresh);

        TestEqual(TEXT("age 60 → Bloated"), Rules::StageForAge(60.0f, View), EMythicDecompStage::Bloated);
        TestEqual(TEXT("age 149 → Bloated"), Rules::StageForAge(149.0f, View), EMythicDecompStage::Bloated);
        TestEqual(TEXT("age 150 → Decayed"), Rules::StageForAge(150.0f, View), EMythicDecompStage::Decayed);
        TestEqual(TEXT("age 239 → Decayed"), Rules::StageForAge(239.0f, View), EMythicDecompStage::Decayed);
        TestEqual(TEXT("age 240 → Skeletal"), Rules::StageForAge(240.0f, View), EMythicDecompStage::Skeletal);

        TestEqual(TEXT("age 10000 → Skeletal (clamped)"), Rules::StageForAge(10000.0f, View), EMythicDecompStage::Skeletal);

        const TConstArrayView<float> Empty;
        TestEqual(TEXT("empty thresholds, age 0 → Fresh"), Rules::StageForAge(0.0f, Empty), EMythicDecompStage::Fresh);
        TestEqual(TEXT("empty thresholds, age 9999 → Fresh"), Rules::StageForAge(9999.0f, Empty), EMythicDecompStage::Fresh);

        EMythicDecompStage Prev = Rules::StageForAge(0.0f, View);
        for (float Age = 0.0f; Age <= 400.0f; Age += 5.0f) {
            const EMythicDecompStage Cur = Rules::StageForAge(Age, View);
            TestTrue(*FString::Printf(TEXT("stage non-decreasing @age %.0f"), Age),
                     static_cast<uint8>(Cur) >= static_cast<uint8>(Prev));
            Prev = Cur;
        }
    }

    {
        const EMythicDecompStage MaxRaisable = EMythicDecompStage::Decayed;

        TestTrue(TEXT("Fresh, not raised → raisable"),
                 Rules::IsRaisable(EMythicDecompStage::Fresh, MaxRaisable, false));
        TestTrue(TEXT("Decayed (== max), not raised → raisable"),
                 Rules::IsRaisable(EMythicDecompStage::Decayed, MaxRaisable, false));
        TestFalse(TEXT("Skeletal (> max) → not raisable"),
                  Rules::IsRaisable(EMythicDecompStage::Skeletal, MaxRaisable, false));
        TestFalse(TEXT("Fresh but already raised → not raisable"),
                  Rules::IsRaisable(EMythicDecompStage::Fresh, MaxRaisable, true));
        TestFalse(TEXT("Decayed but already raised → not raisable"),
                  Rules::IsRaisable(EMythicDecompStage::Decayed, MaxRaisable, true));
    }

    {
        const float Base = 300.0f;
        const float PerTier = 120.0f;

        TestEqual(TEXT("tier 1 (Normal) → base"), Rules::DecayLifetimeForTier(1, Base, PerTier), Base);
        TestEqual(TEXT("tier 0 (unknown) → base"), Rules::DecayLifetimeForTier(0, Base, PerTier), Base);

        float PrevLifetime = Rules::DecayLifetimeForTier(1, Base, PerTier);
        for (int32 Tier = 2; Tier <= 5; ++Tier) {
            const float Cur = Rules::DecayLifetimeForTier(Tier, Base, PerTier);
            TestTrue(*FString::Printf(TEXT("tier %d lifetime > tier %d"), Tier, Tier - 1), Cur > PrevLifetime);
            PrevLifetime = Cur;
        }

        TestEqual(TEXT("Boss lifetime = base + 4*perTier"),
                  Rules::DecayLifetimeForTier(5, Base, PerTier), Base + 4.0f * PerTier);

        TestEqual(TEXT("zero perTier → base at Boss"), Rules::DecayLifetimeForTier(5, Base, 0.0f), Base);
    }

    return true;
}
