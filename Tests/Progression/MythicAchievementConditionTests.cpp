
#include "Misc/AutomationTest.h"
#include "Containers/Map.h"
#include "Progression/MythicAchievementCondition.h"
#include "Progression/MythicTags_MetaProgression.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAchievementConditionTest,
    "Mythic.Progression.AchievementCondition",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAchievementConditionTest::RunTest(const FString &Parameters) {
    const FGameplayTag KillGeneric = STAT_KILL_GENERIC;
    const FGameplayTag KillBoss = STAT_KILL_BOSS;
    const FGameplayTag KillPrefix = KillGeneric.RequestDirectParent();
    const FGameplayTag Gold = STAT_GOLD_EARNED;
    const FGameplayTag OwnedTagA = ACHIEVEMENT_SLAYER;
    const FGameplayTag OwnedTagB = TITLE_SLAYER;

    TMap<FGameplayTag, int64> Exact;
    Exact.Add(KillGeneric, 3);
    Exact.Add(KillBoss, 2);
    TMap<FGameplayTag, int64> Rollup;
    Rollup.Add(KillPrefix, 5);
    Rollup.Add(KillBoss, 2);

    auto Lookup = [&](FGameplayTag Tag, bool bHierarchical) -> int64 {
        const TMap<FGameplayTag, int64> &Src = bHierarchical ? Rollup : Exact;
        const int64 *Found = Src.Find(Tag);
        return Found ? *Found : 0;
    };

    FGameplayTagContainer OwnedAB;
    OwnedAB.AddTag(OwnedTagA);
    OwnedAB.AddTag(OwnedTagB);

    auto MakeReq = [](FGameplayTag Tag, int64 Min, bool bHier) {
        FMythicStatRequirement R;
        R.StatTag = Tag;
        R.MinValue = Min;
        R.bHierarchical = bHier;
        return R;
    };

    {
        FMythicAchievementCondition C;
        TestTrue(TEXT("all-empty condition passes"), FMythicAchievementCondition::Evaluate(C, OwnedAB, Lookup));
        TestTrue(TEXT("all-empty passes with empty owned set"),
                 FMythicAchievementCondition::Evaluate(C, FGameplayTagContainer(), Lookup));
    }

    {
        FMythicAchievementCondition C;
        C.TagCondition.RequireAll.AddTag(OwnedTagA);
        C.StatRequirements.Add(MakeReq(KillGeneric, 3, false));

        TestTrue(TEXT("tag satisfied + exact stat met passes"), FMythicAchievementCondition::Evaluate(C, OwnedAB, Lookup));

        C.StatRequirements[0].MinValue = 4;
        TestFalse(TEXT("exact stat below MinValue fails"), FMythicAchievementCondition::Evaluate(C, OwnedAB, Lookup));
    }

    {
        FMythicAchievementCondition C;
        C.TagCondition.RequireAll.AddTag(TITLE_BOSS_HUNTER);
        C.StatRequirements.Add(MakeReq(KillGeneric, 1, false));
        TestFalse(TEXT("missing required tag fails even when stats pass"),
                  FMythicAchievementCondition::Evaluate(C, OwnedAB, Lookup));
    }

    {
        FMythicAchievementCondition Exactly;
        Exactly.StatRequirements.Add(MakeReq(KillBoss, 5, false));
        TestFalse(TEXT("exact leaf below 5 fails"), FMythicAchievementCondition::Evaluate(Exactly, OwnedAB, Lookup));

        FMythicAchievementCondition Hier;
        Hier.StatRequirements.Add(MakeReq(KillPrefix, 5, true));
        TestTrue(TEXT("hierarchical rollup meets 5"), FMythicAchievementCondition::Evaluate(Hier, OwnedAB, Lookup));
    }

    {
        FMythicAchievementCondition C;
        C.StatRequirements.Add(MakeReq(Gold, 1, false));
        TestFalse(TEXT("never-recorded stat fails a positive threshold"),
                  FMythicAchievementCondition::Evaluate(C, OwnedAB, Lookup));

        C.StatRequirements[0].MinValue = 0;
        TestTrue(TEXT("missing stat meets a zero threshold"), FMythicAchievementCondition::Evaluate(C, OwnedAB, Lookup));
    }

    {
        FMythicAchievementCondition C;
        C.StatRequirements.Add(MakeReq(KillGeneric, 3, false));
        C.StatRequirements.Add(MakeReq(KillBoss, 2, false));
        TestTrue(TEXT("all reqs met passes"), FMythicAchievementCondition::Evaluate(C, OwnedAB, Lookup));

        C.StatRequirements.Add(MakeReq(Gold, 1, false));
        TestFalse(TEXT("one failing req fails the conjunction"), FMythicAchievementCondition::Evaluate(C, OwnedAB, Lookup));
    }

    return true;
}
