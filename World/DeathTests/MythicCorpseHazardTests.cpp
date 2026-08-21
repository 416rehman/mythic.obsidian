
#include "Misc/AutomationTest.h"
#include "World/Death/MythicCorpseHazard.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCorpseHazardTest,
    "Mythic.Death.CorpseHazard",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCorpseHazardTest::RunTest(const FString &Parameters) {
    using Rules = FMythicCorpseHazardRules;

    constexpr int32 Fresh = 0, Bloated = 1, Decayed = 2, Skeletal = 3;

    {
        FMythicCorpseHazardConfig Cfg;

        const float FreshNear = Rules::SanitationPenalty(Fresh, 0.0f, Cfg);
        TestTrue(TEXT("fresh + near → high penalty"), FreshNear > 0.0f);
        TestEqual(TEXT("fresh + zero distance → full SanitationPerFreshCorpse"), FreshNear, Cfg.SanitationPerFreshCorpse);

        TestEqual(TEXT("fresh at radius → 0"), Rules::SanitationPenalty(Fresh, Cfg.SanitationRadius, Cfg), 0.0f);
        TestEqual(TEXT("fresh beyond radius → 0"), Rules::SanitationPenalty(Fresh, Cfg.SanitationRadius * 2.0f, Cfg), 0.0f);

        const float SkeletalNear = Rules::SanitationPenalty(Skeletal, 0.0f, Cfg);
        TestTrue(TEXT("skeletal near < fresh near"), SkeletalNear < FreshNear);
        TestTrue(TEXT("skeletal near still > 0"), SkeletalNear > 0.0f);

        float PrevPenalty = Rules::SanitationPenalty(Fresh, 0.0f, Cfg);
        for (float D = 0.0f; D <= Cfg.SanitationRadius + 500.0f; D += 100.0f) {
            const float Cur = Rules::SanitationPenalty(Fresh, D, Cfg);
            TestTrue(*FString::Printf(TEXT("penalty non-increasing @dist %.0f"), D), Cur <= PrevPenalty + KINDA_SMALL_NUMBER);
            PrevPenalty = Cur;
        }

        float PrevStagePenalty = Rules::SanitationPenalty(Fresh, 100.0f, Cfg);
        for (int32 S = Bloated; S <= Skeletal; ++S) {
            const float Cur = Rules::SanitationPenalty(S, 100.0f, Cfg);
            TestTrue(*FString::Printf(TEXT("penalty non-increasing @stage %d"), S), Cur <= PrevStagePenalty + KINDA_SMALL_NUMBER);
            PrevStagePenalty = Cur;
        }

        FMythicCorpseHazardConfig NoRadius; NoRadius.SanitationRadius = 0.0f;
        TestEqual(TEXT("zero radius → 0 penalty"), Rules::SanitationPenalty(Fresh, 0.0f, NoRadius), 0.0f);
    }

    {
        FMythicCorpseHazardConfig Cfg;

        TestEqual(TEXT("Fresh → 0 severity"), Rules::DiseaseSeverity(Fresh, Cfg), 0.0f);
        TestEqual(TEXT("Bloated → 0 severity"), Rules::DiseaseSeverity(Bloated, Cfg), 0.0f);
        TestFalse(TEXT("Fresh → no disease emit"), Rules::ShouldEmitDisease(Fresh, Cfg));
        TestFalse(TEXT("Bloated → no disease emit"), Rules::ShouldEmitDisease(Bloated, Cfg));

        const float DecayedSev = Rules::DiseaseSeverity(Decayed, Cfg);
        TestTrue(TEXT("Decayed → emits disease"), Rules::ShouldEmitDisease(Decayed, Cfg));
        TestEqual(TEXT("Decayed severity == PerStage"), DecayedSev, Cfg.DiseaseSeverityPerStage);

        const float SkeletalSev = Rules::DiseaseSeverity(Skeletal, Cfg);
        TestTrue(TEXT("Skeletal severity > Decayed severity (rising)"), SkeletalSev > DecayedSev);
        TestTrue(TEXT("Skeletal → emits disease"), Rules::ShouldEmitDisease(Skeletal, Cfg));

        float PrevSev = Rules::DiseaseSeverity(Fresh, Cfg);
        for (int32 S = Bloated; S <= Skeletal; ++S) {
            const float Cur = Rules::DiseaseSeverity(S, Cfg);
            TestTrue(*FString::Printf(TEXT("severity non-decreasing @stage %d"), S), Cur >= PrevSev - KINDA_SMALL_NUMBER);
            PrevSev = Cur;
        }

        FMythicCorpseHazardConfig EarlyStart; EarlyStart.DiseaseStartStageInt = 1.0f;
        TestTrue(TEXT("start=Bloated → Bloated emits"), Rules::ShouldEmitDisease(Bloated, EarlyStart));
        TestFalse(TEXT("start=Bloated → Fresh still doesn't emit"), Rules::ShouldEmitDisease(Fresh, EarlyStart));
    }

    {
        FMythicCorpseHazardConfig Cfg;

        const float A_Fresh = Rules::CarrionAttractiveness(Fresh, Cfg);
        const float A_Bloated = Rules::CarrionAttractiveness(Bloated, Cfg);
        const float A_Decayed = Rules::CarrionAttractiveness(Decayed, Cfg);
        const float A_Skeletal = Rules::CarrionAttractiveness(Skeletal, Cfg);

        const float MidPeak = FMath::Max(A_Bloated, A_Decayed);
        TestTrue(TEXT("mid-decay peak > fresh"), MidPeak > A_Fresh);
        TestTrue(TEXT("mid-decay peak > skeletal"), MidPeak > A_Skeletal);

        TestEqual(TEXT("skeletal → 0 attractiveness"), A_Skeletal, 0.0f);
        TestTrue(TEXT("bloated is the single peak"), A_Bloated >= A_Decayed && A_Bloated > A_Fresh);

        FMythicCorpseHazardConfig Scaled; Scaled.CarrionAttractPerStage = 2.0f;
        TestEqual(TEXT("attractiveness scales with CarrionAttractPerStage"),
                  Rules::CarrionAttractiveness(Bloated, Scaled), 2.0f * A_Bloated);
    }

    {
        TestFalse(TEXT("under cap → no evict"), Rules::ShouldEvictForCap(63, 64));
        TestFalse(TEXT("at cap → no evict"), Rules::ShouldEvictForCap(64, 64));
        TestTrue(TEXT("over cap → evict"), Rules::ShouldEvictForCap(65, 64));
        TestFalse(TEXT("cap 0 disables"), Rules::ShouldEvictForCap(1000, 0));
        TestFalse(TEXT("negative cap disables"), Rules::ShouldEvictForCap(1000, -5));

        {
            const TArray<float> Ages = {10.0f, 300.0f, 45.0f};
            const TArray<bool> NoneLocked = {false, false, false};
            TestEqual(TEXT("picks the oldest (largest age)"), Rules::PickEvictIndex(Ages, NoneLocked), 1);
        }

        {
            const TArray<float> Ages = {10.0f, 300.0f, 45.0f};
            const TArray<bool> OldestLocked = {false, true, false};
            TestEqual(TEXT("skips a locked oldest → next-oldest unlocked"), Rules::PickEvictIndex(Ages, OldestLocked), 2);
        }

        {
            const TArray<float> Ages = {10.0f, 300.0f};
            const TArray<bool> AllLocked = {true, true};
            TestEqual(TEXT("all locked → INDEX_NONE"), Rules::PickEvictIndex(Ages, AllLocked), static_cast<int32>(INDEX_NONE));
        }

        {
            const TArray<float> Empty;
            const TArray<bool> EmptyLocks;
            TestEqual(TEXT("empty → INDEX_NONE"), Rules::PickEvictIndex(Empty, EmptyLocks), static_cast<int32>(INDEX_NONE));
        }

        {
            const TArray<float> Ties = {50.0f, 50.0f, 50.0f};
            const TArray<bool> NoneLocked = {false, false, false};
            TestEqual(TEXT("age ties → lowest index"), Rules::PickEvictIndex(Ties, NoneLocked), 0);
        }

        {
            const TArray<float> Ages = {5.0f, 900.0f};
            const TArray<bool> ShortLocks;
            TestEqual(TEXT("missing lock entries → unlocked"), Rules::PickEvictIndex(Ages, ShortLocks), 1);
        }

        {
            const bool bOpen1 = false, bChannelLocked1 = true;
            const TArray<float> Ages = {10.0f, 900.0f, 300.0f};
            const TArray<bool> Locked = {false, bOpen1 || bChannelLocked1, false};
            TestEqual(TEXT("channel-locked (skinning) oldest is protected → next-oldest unlocked evicts"),
                      Rules::PickEvictIndex(Ages, Locked), 2);
        }
        {
            const TArray<float> Ages = {120.0f, 900.0f};
            const TArray<bool> Locked = {true, false || true};
            TestEqual(TEXT("all bodies open-or-channel-locked → INDEX_NONE (cap yields)"),
                      Rules::PickEvictIndex(Ages, Locked), static_cast<int32>(INDEX_NONE));
        }
    }

    return true;
}
