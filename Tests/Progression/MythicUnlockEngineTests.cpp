
#include "Misc/AutomationTest.h"
#include "Progression/MythicUnlockEngine.h"
#include "Narrative/MythicStoryCondition.h"
#include "Progression/MythicTags_MetaProgression.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicUnlockEngineTest,
    "Mythic.Progression.UnlockEngine",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicUnlockEngineTest::RunTest(const FString &Parameters) {
    const FGameplayTag TagA = ACHIEVEMENT_SLAYER;
    const FGameplayTag TagB = ACHIEVEMENT_BOSS_HUNTER;

    TArray<FMythicStoryCondition> Rules;
    {
        FMythicStoryCondition R0;
        R0.RequireAll.AddTag(TagA);
        Rules.Add(R0);
        FMythicStoryCondition R1;
        R1.RequireAll.AddTag(TagB);
        Rules.Add(R1);
        Rules.Add(FMythicStoryCondition());
    }

    FGameplayTagContainer OwnedA;
    OwnedA.AddTag(TagA);

    {
        TSet<int32> AlreadyApplied;
        TArray<int32> Fire;
        FMythicUnlockEngine::CollectNewlySatisfied(Rules, OwnedA, AlreadyApplied, Fire);

        TestEqual(TEXT("two rules fire in one pass"), Fire.Num(), 2);
        TestTrue(TEXT("rule 0 (TagA) fires"), Fire.Contains(0));
        TestFalse(TEXT("rule 1 (TagB not owned) does not fire"), Fire.Contains(1));
        TestTrue(TEXT("rule 2 (empty precond) fires"), Fire.Contains(2));
    }

    {
        TSet<int32> AlreadyApplied;
        AlreadyApplied.Add(0);
        AlreadyApplied.Add(2);
        TArray<int32> Fire;
        FMythicUnlockEngine::CollectNewlySatisfied(Rules, OwnedA, AlreadyApplied, Fire);

        TestEqual(TEXT("already-applied rules excluded → nothing new fires"), Fire.Num(), 0);
    }

    {
        TArray<FMythicStoryCondition> OnlyEmpty;
        OnlyEmpty.Add(FMythicStoryCondition());

        FGameplayTagContainer Empty;
        TSet<int32> Applied;
        TArray<int32> Fire1;
        FMythicUnlockEngine::CollectNewlySatisfied(OnlyEmpty, Empty, Applied, Fire1);
        TestEqual(TEXT("empty precond fires on the first pass"), Fire1.Num(), 1);
        TestTrue(TEXT("...as rule index 0"), Fire1.Contains(0));

        Applied.Add(0);
        TArray<int32> Fire2;
        FMythicUnlockEngine::CollectNewlySatisfied(OnlyEmpty, Empty, Applied, Fire2);
        TestEqual(TEXT("empty precond does NOT re-fire once applied"), Fire2.Num(), 0);
    }

    {
        TSet<int32> AlreadyApplied;
        TArray<int32> Fire;
        Fire.Add(99);
        FMythicUnlockEngine::CollectNewlySatisfied(Rules, OwnedA, AlreadyApplied, Fire);
        TestTrue(TEXT("pre-existing OutFire entry is preserved"), Fire.Contains(99));
        TestTrue(TEXT("new fires are appended"), Fire.Contains(0) && Fire.Contains(2));
    }

    return true;
}
