
#include "Misc/AutomationTest.h"
#include "World/LivingWorld/Spawn/DesignerSpawnerTypes.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"

namespace {
    FMythicTimeWindow MakeWindow(float Start, float End, bool bEnabled = true) {
        FMythicTimeWindow W;
        W.bEnabled = bEnabled;
        W.StartHour = Start;
        W.EndHour = End;
        return W;
    }
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDesignerSpawnerHourWindowTest,
    "Mythic.LivingWorld.DesignerSpawner.IsHourInWindow",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicDesignerSpawnerHourWindowTest::RunTest(const FString& Parameters) {
    using namespace MythicDesignerSpawner;

    TestTrue(TEXT("Disabled window always satisfied (hour 3)"), IsHourInWindow(MakeWindow(8, 18, false), 3.0f));
    TestTrue(TEXT("Disabled window always satisfied (hour 23)"), IsHourInWindow(MakeWindow(8, 18, false), 23.0f));

    const FMythicTimeWindow Day = MakeWindow(8, 18);
    TestTrue(TEXT("Start boundary is inclusive (8.0 in [8,18))"), IsHourInWindow(Day, 8.0f));
    TestTrue(TEXT("Mid window (12.0) in [8,18)"), IsHourInWindow(Day, 12.0f));
    TestFalse(TEXT("End boundary is exclusive (18.0 NOT in [8,18))"), IsHourInWindow(Day, 18.0f));
    TestFalse(TEXT("Before window (7.99) NOT in [8,18)"), IsHourInWindow(Day, 7.99f));
    TestFalse(TEXT("After window (18.01) NOT in [8,18)"), IsHourInWindow(Day, 18.01f));

    const FMythicTimeWindow Night = MakeWindow(22, 6);
    TestTrue(TEXT("Wrap: 23.0 in [22,6)"), IsHourInWindow(Night, 23.0f));
    TestTrue(TEXT("Wrap: 22.0 (start) in [22,6)"), IsHourInWindow(Night, 22.0f));
    TestTrue(TEXT("Wrap: 0.0 in [22,6)"), IsHourInWindow(Night, 0.0f));
    TestTrue(TEXT("Wrap: 5.99 in [22,6)"), IsHourInWindow(Night, 5.99f));
    TestFalse(TEXT("Wrap: 6.0 (end) NOT in [22,6)"), IsHourInWindow(Night, 6.0f));
    TestFalse(TEXT("Wrap: 12.0 NOT in [22,6)"), IsHourInWindow(Night, 12.0f));

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDesignerSpawnerEvalBaselineTest,
    "Mythic.LivingWorld.DesignerSpawner.Evaluate.Baseline",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicDesignerSpawnerEvalBaselineTest::RunTest(const FString& Parameters) {
    using namespace MythicDesignerSpawner;

    FMythicDesignerConditionSet Empty;
    FMythicDesignerConditionInputs In;
    In.GameHour = 13.0f;
    TestTrue(TEXT("Empty condition set always met"), EvaluateConditions(Empty, In));

    FMythicDesignerConditionSet C;
    C.TimeWindow = MakeWindow(8, 18);
    C.RequiredPlayerTags.AddTag(TAG_NPC_ROLE_CIVILIAN);
    C.bRequireAnyPlayerInRange = true;
    C.GatingFactionTag = TAG_NPC_ROLE_SOLDIER;
    C.FactionState = EMythicDesignerFactionStatePredicate::Alive;
    C.RelationFactionA = TAG_NPC_ROLE_SOLDIER;
    C.RelationFactionB = TAG_NPC_ROLE_SOLDIER;
    C.Relation = EMythicDesignerRelationPredicate::AtWar;

    FMythicDesignerConditionInputs Pass;
    Pass.GameHour = 12.0f;
    Pass.bAnyPlayerSatisfiesTags = true;
    Pass.GatingFactionStatus = EMythicFactionStatus::Active;
    Pass.bGatingFactionResolved = true;
    Pass.RelationAB = EMythicFactionRelation::Hostile;
    Pass.bRelationResolved = true;
    TestTrue(TEXT("All gates satisfied => met"), EvaluateConditions(C, Pass));

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDesignerSpawnerEvalGatesTest,
    "Mythic.LivingWorld.DesignerSpawner.Evaluate.Gates",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicDesignerSpawnerEvalGatesTest::RunTest(const FString& Parameters) {
    using namespace MythicDesignerSpawner;

    const FGameplayTag FactionTag = TAG_NPC_ROLE_SOLDIER;
    const FGameplayTag PlayerTag = TAG_NPC_ROLE_CIVILIAN;

    auto MakePassingSet = [&]() {
        FMythicDesignerConditionSet C;
        C.TimeWindow = MakeWindow(8, 18);
        C.RequiredPlayerTags.AddTag(PlayerTag);
        C.bRequireAnyPlayerInRange = true;
        C.GatingFactionTag = FactionTag;
        C.FactionState = EMythicDesignerFactionStatePredicate::Alive;
        C.RelationFactionA = FactionTag;
        C.RelationFactionB = FactionTag;
        C.Relation = EMythicDesignerRelationPredicate::AtWar;
        return C;
    };
    auto MakePassingInputs = []() {
        FMythicDesignerConditionInputs In;
        In.GameHour = 12.0f;
        In.bAnyPlayerSatisfiesTags = true;
        In.GatingFactionStatus = EMythicFactionStatus::Active;
        In.bGatingFactionResolved = true;
        In.RelationAB = EMythicFactionRelation::Hostile;
        In.bRelationResolved = true;
        return In;
    };

    {
        FMythicDesignerConditionInputs In = MakePassingInputs();
        In.GameHour = 20.0f;
        TestFalse(TEXT("Time outside window rejects"), EvaluateConditions(MakePassingSet(), In));
    }

    {
        FMythicDesignerConditionInputs In = MakePassingInputs();
        In.bAnyPlayerSatisfiesTags = false;
        TestFalse(TEXT("Player gate unsatisfied rejects"), EvaluateConditions(MakePassingSet(), In));
    }

    {
        FMythicDesignerConditionInputs In = MakePassingInputs();
        In.GatingFactionStatus = EMythicFactionStatus::Dormant;
        TestFalse(TEXT("Faction-state mismatch rejects"), EvaluateConditions(MakePassingSet(), In));
    }

    {
        FMythicDesignerConditionInputs In = MakePassingInputs();
        In.RelationAB = EMythicFactionRelation::Neutral;
        TestFalse(TEXT("Relation AtWar but Neutral rejects"), EvaluateConditions(MakePassingSet(), In));
    }

    {
        FMythicDesignerConditionSet C = MakePassingSet();
        C.Relation = EMythicDesignerRelationPredicate::AtPeace;
        FMythicDesignerConditionInputs Hostile = MakePassingInputs();
        Hostile.RelationAB = EMythicFactionRelation::Hostile;
        TestFalse(TEXT("AtPeace but Hostile rejects"), EvaluateConditions(C, Hostile));

        FMythicDesignerConditionInputs Peace = MakePassingInputs();
        Peace.RelationAB = EMythicFactionRelation::Friendly;
        TestTrue(TEXT("AtPeace and Friendly passes"), EvaluateConditions(C, Peace));
    }

    {
        FMythicDesignerConditionSet TagsOnly;
        TagsOnly.RequiredPlayerTags.AddTag(PlayerTag);
        FMythicDesignerConditionInputs In;
        In.GameHour = 12.0f;
        In.bAnyPlayerSatisfiesTags = false;
        TestFalse(TEXT("Player-tags-only gate unsatisfied rejects"), EvaluateConditions(TagsOnly, In));
        In.bAnyPlayerSatisfiesTags = true;
        TestTrue(TEXT("Player-tags-only gate satisfied passes"), EvaluateConditions(TagsOnly, In));

        FMythicDesignerConditionSet RangeOnly;
        RangeOnly.bRequireAnyPlayerInRange = true;
        FMythicDesignerConditionInputs In2;
        In2.GameHour = 12.0f;
        In2.bAnyPlayerSatisfiesTags = false;
        TestFalse(TEXT("Player-range-only gate unsatisfied rejects"), EvaluateConditions(RangeOnly, In2));

        FMythicDesignerConditionSet NoPlayerGate;
        FMythicDesignerConditionInputs In3;
        In3.GameHour = 12.0f;
        In3.bAnyPlayerSatisfiesTags = false;
        TestTrue(TEXT("No player gate -> false flag is ignored"), EvaluateConditions(NoPlayerGate, In3));
    }

    {
        FMythicDesignerConditionSet C;
        C.GatingFactionTag = FactionTag;
        C.FactionState = EMythicDesignerFactionStatePredicate::Any;
        FMythicDesignerConditionInputs In;
        In.GameHour = 12.0f;
        In.bGatingFactionResolved = true;
        In.GatingFactionStatus = EMythicFactionStatus::Annihilated;
        TestTrue(TEXT("FactionState Any ignores resolved status"), EvaluateConditions(C, In));
    }

    {
        FMythicDesignerConditionSet C;
        C.RelationFactionA = FactionTag;
        C.RelationFactionB = FactionTag;
        C.Relation = EMythicDesignerRelationPredicate::Ignore;
        FMythicDesignerConditionInputs In;
        In.GameHour = 12.0f;
        In.bRelationResolved = true;
        In.RelationAB = EMythicFactionRelation::Hostile;
        TestTrue(TEXT("Relation Ignore ignores resolved relation"), EvaluateConditions(C, In));
    }

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDesignerSpawnerEvalFailSafeTest,
    "Mythic.LivingWorld.DesignerSpawner.Evaluate.FailSafe",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicDesignerSpawnerEvalFailSafeTest::RunTest(const FString& Parameters) {
    using namespace MythicDesignerSpawner;

    const FGameplayTag FactionTag = TAG_NPC_ROLE_SOLDIER;

    {
        FMythicDesignerConditionSet C;
        C.GatingFactionTag = FactionTag;
        C.FactionState = EMythicDesignerFactionStatePredicate::Alive;

        FMythicDesignerConditionInputs In;
        In.GameHour = 12.0f;
        In.bGatingFactionResolved = false;
        TestFalse(TEXT("Unresolved required faction => false (fail-safe)"), EvaluateConditions(C, In));

        In.bGatingFactionResolved = true;
        In.GatingFactionStatus = EMythicFactionStatus::Active;
        TestTrue(TEXT("Resolved + matching faction-state => true"), EvaluateConditions(C, In));
    }

    {
        FMythicDesignerConditionSet C;
        C.RelationFactionA = FactionTag;
        C.RelationFactionB = FactionTag;
        C.Relation = EMythicDesignerRelationPredicate::AtWar;

        FMythicDesignerConditionInputs In;
        In.GameHour = 12.0f;
        In.bRelationResolved = false;
        TestFalse(TEXT("Unresolved required relation => false (fail-safe)"), EvaluateConditions(C, In));

        In.bRelationResolved = true;
        In.RelationAB = EMythicFactionRelation::Hostile;
        TestTrue(TEXT("Resolved + hostile relation for AtWar => true"), EvaluateConditions(C, In));
    }

    {
        FMythicDesignerConditionSet C;
        C.GatingFactionTag = FactionTag;
        C.FactionState = EMythicDesignerFactionStatePredicate::Any;

        FMythicDesignerConditionInputs In;
        In.GameHour = 12.0f;
        In.bGatingFactionResolved = false;
        TestTrue(TEXT("FactionState Any ignores the (unresolved) faction gate"), EvaluateConditions(C, In));
    }

    return true;
}
