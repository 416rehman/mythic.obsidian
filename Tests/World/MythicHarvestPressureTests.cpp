
#include "Misc/AutomationTest.h"
#include "World/Gathering/MythicHarvestPressureRules.h"
#include "World/Gathering/MythicYieldQuality.h"

namespace {
static FMythicHarvestPressureConfig MakeTunedHarvestConfig() {
    FMythicHarvestPressureConfig Cfg;
    Cfg.YieldDepletionPerPressure = 0.1f;
    Cfg.MinYieldMultiplier = 0.35f;
    Cfg.RespawnGateThreshold = 5.0f;
    Cfg.RespawnLengthenPerPressure = 0.2f;
    Cfg.MaxRespawnDelayMultiplier = 4.0f;
    Cfg.PressurePerQualityTierDrop = 3.0f;
    Cfg.FallowRecoveryPerSecond = 0.5f;
    return Cfg;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestPressureTest,
    "Mythic.World.HarvestPressure",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestPressureTest::RunTest(const FString &Parameters) {
    using HP = FMythicHarvestPressureRules;
    const FMythicHarvestPressureConfig Tuned = MakeTunedHarvestConfig();

    {
        const FMythicHarvestPressureConfig Def;
        for (const float P : {0.0f, 1.0f, 50.0f, 1000.0f}) {
            TestEqual(TEXT("inert: yield mult 1.0 at any pressure"), HP::DepletionYieldMultiplier(P, Def), 1.0f);
            TestEqual(TEXT("inert: respawn-delay mult 1.0 at any pressure"), HP::RespawnDelayMultiplier(P, Def), 1.0f);
            TestFalse(TEXT("inert: never gated"), HP::IsRespawnGated(P, Def.RespawnGateThreshold));
            TestEqual(TEXT("inert: 0 quality-tier drops"), HP::QualityTierDropSteps(P, Def.PressurePerQualityTierDrop), 0);
        }
    }

    {
        TestEqual(TEXT("P0: yield mult 1.0"), HP::DepletionYieldMultiplier(0.0f, Tuned), 1.0f);
        TestEqual(TEXT("P0: respawn-delay mult 1.0"), HP::RespawnDelayMultiplier(0.0f, Tuned), 1.0f);
        TestFalse(TEXT("P0: not gated"), HP::IsRespawnGated(0.0f, Tuned.RespawnGateThreshold));
        TestEqual(TEXT("P0: 0 quality drops"), HP::QualityTierDropSteps(0.0f, Tuned.PressurePerQualityTierDrop), 0);
        TestEqual(TEXT("negative pressure clamps to neutral yield"), HP::DepletionYieldMultiplier(-10.0f, Tuned), 1.0f);
    }

    {
        TestEqual(TEXT("P1 → 0.9"), HP::DepletionYieldMultiplier(1.0f, Tuned), 0.9f);
        TestEqual(TEXT("P5 → 0.5"), HP::DepletionYieldMultiplier(5.0f, Tuned), 0.5f);
        TestEqual(TEXT("P6.5 → floor 0.35"), HP::DepletionYieldMultiplier(6.5f, Tuned), 0.35f);
        TestEqual(TEXT("P100 → clamped at floor"), HP::DepletionYieldMultiplier(100.0f, Tuned), 0.35f);
        float Prev = HP::DepletionYieldMultiplier(0.0f, Tuned);
        for (float P = 0.5f; P <= 12.0f; P += 0.5f) {
            const float Cur = HP::DepletionYieldMultiplier(P, Tuned);
            TestTrue(TEXT("yield multiplier is monotonic non-increasing in pressure"), Cur <= Prev + KINDA_SMALL_NUMBER);
            TestTrue(TEXT("yield multiplier never below floor"), Cur >= Tuned.MinYieldMultiplier - KINDA_SMALL_NUMBER);
            Prev = Cur;
        }
    }

    {
        TestFalse(TEXT("below threshold → not gated"), HP::IsRespawnGated(4.99f, Tuned.RespawnGateThreshold));
        TestTrue(TEXT("at threshold → gated (inclusive)"), HP::IsRespawnGated(5.0f, Tuned.RespawnGateThreshold));
        TestTrue(TEXT("past threshold → gated"), HP::IsRespawnGated(50.0f, Tuned.RespawnGateThreshold));
        TestFalse(TEXT("threshold 0 → disabled"), HP::IsRespawnGated(1000.0f, 0.0f));
    }

    {
        TestEqual(TEXT("P0 → ×1.0"), HP::RespawnDelayMultiplier(0.0f, Tuned), 1.0f);
        TestEqual(TEXT("P5 → ×2.0"), HP::RespawnDelayMultiplier(5.0f, Tuned), 2.0f);
        TestEqual(TEXT("huge pressure → capped at Max"), HP::RespawnDelayMultiplier(1000.0f, Tuned), 4.0f);
        float Prev = HP::RespawnDelayMultiplier(0.0f, Tuned);
        for (float P = 1.0f; P <= 40.0f; P += 1.0f) {
            const float Cur = HP::RespawnDelayMultiplier(P, Tuned);
            TestTrue(TEXT("respawn-delay multiplier monotonic non-decreasing"), Cur >= Prev - KINDA_SMALL_NUMBER);
            TestTrue(TEXT("respawn-delay multiplier capped"), Cur <= Tuned.MaxRespawnDelayMultiplier + KINDA_SMALL_NUMBER);
            Prev = Cur;
        }
    }

    {
        TestEqual(TEXT("P3 → 1 tier drop"), HP::QualityTierDropSteps(3.0f, Tuned.PressurePerQualityTierDrop), 1);
        TestEqual(TEXT("P6 → 2 tier drops"), HP::QualityTierDropSteps(6.0f, Tuned.PressurePerQualityTierDrop), 2);
        TestEqual(TEXT("P9 → 3 tier drops"), HP::QualityTierDropSteps(9.0f, Tuned.PressurePerQualityTierDrop), 3);

        using YQ = FMythicYieldQuality;
        TestTrue(TEXT("Pristine, 0 drops → Pristine"),
                 YQ::DepleteTier(EMythicYieldQuality::Pristine, 0) == EMythicYieldQuality::Pristine);
        TestTrue(TEXT("Pristine, 1 drop → Fine"),
                 YQ::DepleteTier(EMythicYieldQuality::Pristine, 1) == EMythicYieldQuality::Fine);
        TestTrue(TEXT("Pristine, 2 drops → Common"),
                 YQ::DepleteTier(EMythicYieldQuality::Pristine, 2) == EMythicYieldQuality::Common);
        TestTrue(TEXT("Pristine, 5 drops → floored at Common"),
                 YQ::DepleteTier(EMythicYieldQuality::Pristine, 5) == EMythicYieldQuality::Common);
        TestTrue(TEXT("Fine, 1 drop → Common"),
                 YQ::DepleteTier(EMythicYieldQuality::Fine, 1) == EMythicYieldQuality::Common);
    }

    {
        const float Rate = Tuned.FallowRecoveryPerSecond;
        const float P0 = 10.0f;
        const float P10 = HP::FallowRecover(P0, 10.0f, Rate);
        const float P20 = HP::FallowRecover(P0, 20.0f, Rate);
        TestEqual(TEXT("fallow 10s sheds 5"), P10, 5.0f);
        TestEqual(TEXT("fallow 20s → back to 0"), P20, 0.0f);
        TestEqual(TEXT("fallow never underflows past 0"), HP::FallowRecover(P0, 1000.0f, Rate), 0.0f);

        const float MStart = HP::DepletionYieldMultiplier(P0, Tuned);
        const float MMid = HP::DepletionYieldMultiplier(P10, Tuned);
        const float MEnd = HP::DepletionYieldMultiplier(P20, Tuned);
        TestTrue(TEXT("multiplier recovers upward as pressure fallows"), MMid > MStart && MEnd > MMid);
        TestEqual(TEXT("fully-recovered cell is byte-identical (1.0)"), MEnd, 1.0f);
        TestFalse(TEXT("recovered cell no longer respawn-gated"), HP::IsRespawnGated(P20, Tuned.RespawnGateThreshold));
    }

    return true;
}
