
#include "Misc/AutomationTest.h"
#include "World/Hunting/MythicSkinningRules.h"
#include "World/Gathering/MythicYieldQuality.h"
#include "World/Death/MythicCorpseTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPeltQualityTest,
    "Mythic.Hunting.PeltQuality",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPeltQualityTest::RunTest(const FString &Parameters) {
    using Rules = FMythicSkinningRules;

    const FMythicPeltQualityConfig Config;
    const FMythicYieldQualityRules YieldRules;

    constexpr float NoUpgradeRoll = 0.99f;
    constexpr float PristineRoll = 0.001f;

    FMythicKillContext Clean;
    Clean.bCriticalKill = true;
    Clean.HitsTaken = 1;

    FMythicKillContext Ordinary;

    FMythicKillContext Botched;
    Botched.bBurnKill = true;
    Botched.bPoisonKill = true;

    {
        TestEqual(TEXT("no info → 0 points"), Rules::BotchScore(Ordinary, Config), 0);

        FMythicKillContext C;
        C.bBurnKill = true;
        TestEqual(TEXT("burn → 1 point"), Rules::BotchScore(C, Config), 1);
        C.bPoisonKill = true;
        TestEqual(TEXT("burn+poison → 2"), Rules::BotchScore(C, Config), 2);
        C.OverkillFraction = 0.5f;
        TestEqual(TEXT("+obliterating overkill → 3"), Rules::BotchScore(C, Config), 3);
        C.HitsTaken = 8;
        TestEqual(TEXT("+riddled carcass → 4"), Rules::BotchScore(C, Config), 4);

        FMythicKillContext UnderThresholds;
        UnderThresholds.OverkillFraction = 0.49f;
        UnderThresholds.HitsTaken = 7;
        TestEqual(TEXT("sub-threshold overkill/hits add nothing"), Rules::BotchScore(UnderThresholds, Config), 0);

        FMythicKillContext Bleedy;
        Bleedy.bBleedKill = true;
        TestEqual(TEXT("bleed alone adds no botch point"), Rules::BotchScore(Bleedy, Config), 0);
    }

    {
        TestTrue(TEXT("clean crit kill bases at Fine"), Rules::BaseTierForKill(Clean, Config) == EMythicYieldQuality::Fine);
        TestTrue(TEXT("ordinary kill bases at Common"), Rules::BaseTierForKill(Ordinary, Config) == EMythicYieldQuality::Common);
        TestTrue(TEXT("botched kill bases at Ragged"), Rules::BaseTierForKill(Botched, Config) == EMythicYieldQuality::Ragged);

        FMythicKillContext NoCrit = Clean;
        NoCrit.bCriticalKill = false;
        TestTrue(TEXT("no crit → Common base"), Rules::BaseTierForKill(NoCrit, Config) == EMythicYieldQuality::Common);

        FMythicKillContext TooManyHits = Clean;
        TooManyHits.HitsTaken = 4;
        TestTrue(TEXT("too many hits → Common base"), Rules::BaseTierForKill(TooManyHits, Config) == EMythicYieldQuality::Common);

        FMythicKillContext BleedClean = Clean;
        BleedClean.bBleedKill = true;
        TestTrue(TEXT("a torn hide voids the clean bonus"), Rules::BaseTierForKill(BleedClean, Config) == EMythicYieldQuality::Common);

        FMythicKillContext OneBotch = Clean;
        OneBotch.bBurnKill = true;
        TestTrue(TEXT("one botch point → Common base"), Rules::BaseTierForKill(OneBotch, Config) == EMythicYieldQuality::Common);
    }

    {
        TestTrue(TEXT("botched → Ragged despite a Pristine roll"),
                 Rules::ResolveQuality(Botched, EMythicDecompStage::Fresh, 50, YieldRules, Config, PristineRoll) == EMythicYieldQuality::Ragged);

        TestTrue(TEXT("ordinary, fresh, no upgrade → Common"),
                 Rules::ResolveQuality(Ordinary, EMythicDecompStage::Fresh, 0, YieldRules, Config, NoUpgradeRoll) == EMythicYieldQuality::Common);
        TestTrue(TEXT("clean, fresh, no upgrade → Fine"),
                 Rules::ResolveQuality(Clean, EMythicDecompStage::Fresh, 0, YieldRules, Config, NoUpgradeRoll) == EMythicYieldQuality::Fine);

        TestTrue(TEXT("clean, fresh, pristine roll → Pristine (the ceiling)"),
                 Rules::ResolveQuality(Clean, EMythicDecompStage::Fresh, 0, YieldRules, Config, PristineRoll) == EMythicYieldQuality::Pristine);

        TestTrue(TEXT("bloated drops Fine → Common"),
                 Rules::ResolveQuality(Clean, EMythicDecompStage::Bloated, 0, YieldRules, Config, NoUpgradeRoll) == EMythicYieldQuality::Common);
        TestTrue(TEXT("decayed drops Fine → Ragged"),
                 Rules::ResolveQuality(Clean, EMythicDecompStage::Decayed, 0, YieldRules, Config, NoUpgradeRoll) == EMythicYieldQuality::Ragged);
        TestTrue(TEXT("skeletal clamps at Ragged"),
                 Rules::ResolveQuality(Clean, EMythicDecompStage::Skeletal, 0, YieldRules, Config, NoUpgradeRoll) == EMythicYieldQuality::Ragged);
        TestTrue(TEXT("a Pristine roll on a BLOATED corpse is only Fine (no fresh, no ceiling)"),
                 Rules::ResolveQuality(Clean, EMythicDecompStage::Bloated, 0, YieldRules, Config, PristineRoll) == EMythicYieldQuality::Fine);

        int32 PrevTier = FMythicYieldQuality::TierIndex(
            Rules::ResolveQuality(Clean, EMythicDecompStage::Fresh, 10, YieldRules, Config, 0.5f));
        for (uint8 Stage = 1; Stage <= static_cast<uint8>(EMythicDecompStage::Skeletal); ++Stage) {
            const int32 Cur = FMythicYieldQuality::TierIndex(
                Rules::ResolveQuality(Clean, static_cast<EMythicDecompStage>(Stage), 10, YieldRules, Config, 0.5f));
            TestTrue(*FString::Printf(TEXT("decomp stage %d never improves the tier"), Stage), Cur <= PrevTier);
            PrevTier = Cur;
        }
    }

    {
        const FMythicKillContext Unstamped;
        TestTrue(TEXT("no kill info → Common (never Ragged, never a freebie)"),
                 Rules::ResolveQuality(Unstamped, EMythicDecompStage::Fresh, 0, YieldRules, Config, NoUpgradeRoll) == EMythicYieldQuality::Common);
    }

    return true;
}
