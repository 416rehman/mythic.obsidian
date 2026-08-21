
#include "Misc/AutomationTest.h"
#include "World/Fishing/MythicFishingMinigameRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicFishingMinigameTest,
    "Mythic.Fishing.Minigame",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicFishingMinigameTest::RunTest(const FString &Parameters) {
    using Rules = FMythicFishingMinigameRules;

    FMythicFishingMinigameConfig Config;

    {
        const FMythicFishingMinigameParams A = Rules::MakeParams(Config, 12345);
        const FMythicFishingMinigameParams B = Rules::MakeParams(Config, 12345);
        TestEqual(TEXT("same seed → same wait"), A.WaitSeconds, B.WaitSeconds);
        TestEqual(TEXT("same seed → same surge delay"), A.SurgeFirstDelay, B.SurgeFirstDelay);

        const FMythicFishingMinigameParams C = Rules::MakeParams(Config, 54321);
        TestTrue(TEXT("different seed → (generally) different timeline"),
                 A.WaitSeconds != C.WaitSeconds || A.SurgeFirstDelay != C.SurgeFirstDelay);

        TestTrue(TEXT("wait within config range"), A.WaitSeconds >= Config.WaitMinSeconds && A.WaitSeconds <= Config.WaitMaxSeconds);
        TestTrue(TEXT("surge leaves a reel gap"), A.SurgeDuration <= A.SurgePeriod * 0.75f + KINDA_SMALL_NUMBER);

        FMythicFishingMinigameConfig Bad;
        Bad.WaitMinSeconds = 5.0f;
        Bad.WaitMaxSeconds = 1.0f;
        Bad.SurgePeriodSeconds = 0.0f;
        Bad.SurgeDurationSeconds = 99.0f;
        Bad.PullsToLand = 0;
        const FMythicFishingMinigameParams D = Rules::MakeParams(Bad, 7);
        TestEqual(TEXT("inverted wait range clamps to min"), D.WaitSeconds, 5.0f);
        TestTrue(TEXT("period floors at 0.5"), D.SurgePeriod >= 0.5f);
        TestTrue(TEXT("surge duration clamped under the period"), D.SurgeDuration <= D.SurgePeriod * 0.75f + KINDA_SMALL_NUMBER);
        TestEqual(TEXT("pulls floor at 1"), D.PullsToLand, 1);
    }

    FMythicFishingMinigameParams P;
    P.WaitSeconds = 3.0f;
    P.BiteWindowSeconds = 1.0f;
    P.FightMaxSeconds = 20.0f;
    P.SurgeFirstDelay = 2.0f;
    P.SurgePeriod = 4.0f;
    P.SurgeDuration = 1.0f;
    P.PullsToLand = 3;

    {
        TestTrue(TEXT("t=0 → Wait"), Rules::PhaseAtTime(P, 0.0f, false, 0.0f) == EMythicFishingPhase::Wait);
        TestTrue(TEXT("t just before window → Wait"), Rules::PhaseAtTime(P, 2.99f, false, 0.0f) == EMythicFishingPhase::Wait);
        TestTrue(TEXT("t at window start → Bite"), Rules::PhaseAtTime(P, 3.0f, false, 0.0f) == EMythicFishingPhase::Bite);
        TestTrue(TEXT("t inside window → Bite"), Rules::PhaseAtTime(P, 3.5f, false, 0.0f) == EMythicFishingPhase::Bite);
        TestTrue(TEXT("t past window unhooked → MissedBite"), Rules::PhaseAtTime(P, 4.0f, false, 0.0f) == EMythicFishingPhase::MissedBite);
        TestTrue(TEXT("hooked → Fight"), Rules::PhaseAtTime(P, 5.0f, true, 3.5f) == EMythicFishingPhase::Fight);
        TestTrue(TEXT("fight cap outlasted → Escaped"), Rules::PhaseAtTime(P, 3.5f + 20.0f, true, 3.5f) == EMythicFishingPhase::Escaped);
    }

    {
        TestFalse(TEXT("early hook is NOT in window"), Rules::IsInBiteWindow(P, 1.0f));
        TestTrue(TEXT("window start scores"), Rules::IsInBiteWindow(P, 3.0f));
        TestTrue(TEXT("mid-window scores"), Rules::IsInBiteWindow(P, 3.9f));
        TestFalse(TEXT("window end is exclusive"), Rules::IsInBiteWindow(P, 4.0f));

        TestFalse(TEXT("no miss before the window closes"), Rules::IsBiteMissed(P, 3.9f, false));
        TestTrue(TEXT("miss after the window (unhooked)"), Rules::IsBiteMissed(P, 4.0f, false));
        TestFalse(TEXT("hooked can't miss"), Rules::IsBiteMissed(P, 10.0f, true));
    }

    {
        TestFalse(TEXT("pre-first-surge is calm"), Rules::IsInSurge(P, 1.9f));
        TestTrue(TEXT("first surge start"), Rules::IsInSurge(P, 2.0f));
        TestTrue(TEXT("inside first surge"), Rules::IsInSurge(P, 2.9f));
        TestFalse(TEXT("after first surge is calm"), Rules::IsInSurge(P, 3.1f));
        TestTrue(TEXT("second surge (periodic)"), Rules::IsInSurge(P, 6.5f));
        TestFalse(TEXT("between second and third"), Rules::IsInSurge(P, 8.0f));

        TestTrue(TEXT("pull between surges → Reel"), Rules::ScorePull(P, 1.0f, 0, true) == EMythicFishingPullResult::Reel);
        TestTrue(TEXT("pull during surge → LineBreak"), Rules::ScorePull(P, 2.5f, 0, true) == EMythicFishingPullResult::LineBreak);
        TestTrue(TEXT("the PullsToLand-th reel → Landed"), Rules::ScorePull(P, 3.5f, 2, true) == EMythicFishingPullResult::Landed);
        TestTrue(TEXT("unhooked pull → Ignored"), Rules::ScorePull(P, 1.0f, 0, false) == EMythicFishingPullResult::Ignored);

        TestFalse(TEXT("no escape before the cap"), Rules::HasFishEscaped(P, 19.9f));
        TestTrue(TEXT("escape at the cap"), Rules::HasFishEscaped(P, 20.0f));

        TestTrue(TEXT("ceiling > wait+window+fight"), Rules::ChannelCeiling(P) > P.WaitSeconds + P.BiteWindowSeconds + P.FightMaxSeconds);
    }

    {
        TestFalse(TEXT("valve disabled (level 0 config) never auto-resolves"), Rules::ShouldAutoResolve(99, 0, true));
        TestFalse(TEXT("below mastery keeps the minigame"), Rules::ShouldAutoResolve(9, 10, true));
        TestTrue(TEXT("at mastery, trash auto-resolves"), Rules::ShouldAutoResolve(10, 10, true));
        TestTrue(TEXT("above mastery, trash auto-resolves"), Rules::ShouldAutoResolve(50, 10, true));
        TestFalse(TEXT("a REAL catch always plays the beats"), Rules::ShouldAutoResolve(50, 10, false));
    }

    return true;
}
