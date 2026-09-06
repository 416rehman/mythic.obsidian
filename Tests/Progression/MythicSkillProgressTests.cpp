
#include "Tests/Progression/MythicAbilityWorldFixture.h"
#include "Misc/AutomationTest.h"
#include "Tests/Progression/MythicAbilityTestFixture.h"

#include "Abilities/GameplayAbility.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"
#include "Player/MythicPlayerState.h"
#include "Progression/Skills/MythicSkillComponent.h"
#include "Progression/Skills/MythicSkillDefinition.h"
#include "Subsystem/SaveSystem/Character/CharacterData.h"
#include "Subsystem/SaveSystem/MythicSaveGame.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace {

struct FMythicSkillProgressFixture {
    UGameInstance *GameInstance = nullptr;
    AMythicPlayerState *PlayerState = nullptr;
    UMythicSkillComponent *Skills = nullptr;
};

// Every progress verb is authority-gated on the owner, and the player state is that owner in the game, so spawning
// one is the honest fixture rather than a stand-in.
bool BuildSkillProgressFixture(FAutomationTestBase &Test, FMythicSkillProgressFixture &Out) {
    if (!Test.TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }
    Out.GameInstance = NewObject<UGameInstance>(GEngine);
    Out.GameInstance->InitializeStandalone();
    UWorld *World = Out.GameInstance->GetWorld();
    if (!Test.TestNotNull(TEXT("standalone world exists"), World)) {
        return false;
    }
    Out.PlayerState = World->SpawnActor<AMythicPlayerState>();
    if (!Test.TestNotNull(TEXT("the player state spawned"), Out.PlayerState)) {
        return false;
    }
    if (!Test.TestTrue(TEXT("the owner holds authority"), Out.PlayerState->HasAuthority())) {
        return false;
    }
    Out.Skills = Out.PlayerState->GetSkillComponent();
    if (!Test.TestNotNull(TEXT("the player state owns a skill component"), Out.Skills)) {
        return false;
    }
    if (!Out.Skills->IsRegistered()) {
        Out.Skills->RegisterComponent();
    }
    InitializeAbilityTestPlayer(Out.PlayerState);
    return true;
}

FMythicSkillModifier MakeModifier(float RadiusDelta, int32 PointCost) {
    FMythicSkillModifier Modifier;
    Modifier.RadiusDelta = RadiusDelta;
    Modifier.PointCost = PointCost;
    return Modifier;
}

// No RequiredTag, so nothing but the gate under test can refuse the modifier.
UMythicSkillDefinition *MakeSkillWithModifiers(UMythicSkillComponent* Skills,std::initializer_list<FMythicSkillModifier> Modifiers) {
    UMythicSkillDefinition *Skill = NewObject<UMythicSkillDefinition>();
    Skill->Ability = UGameplayAbility::StaticClass();
    Skill->Modifiers = Modifiers;
    RegisterAbilityTestDefinition(Skills,Skill);
    return Skill;
}

// A skill's ceiling is how many modifiers it authors, so a fixture that wants to reach level N has to offer N things
// to buy. Each row is real and priced at one point, leaving only the gate under test able to refuse it.
UMythicSkillDefinition *MakeSkillWithNModifiers(UMythicSkillComponent* Skills,int32 Count) {
    UMythicSkillDefinition *Skill = NewObject<UMythicSkillDefinition>();
    Skill->Ability = UGameplayAbility::StaticClass();
    for (int32 Index = 0; Index < Count; Index++) {
        FMythicSkillModifier Modifier;
        Modifier.RadiusDelta = 25.0f * (Index + 1);
        Modifier.PointCost = 1;
        Skill->Modifiers.Add(Modifier);
    }
    RegisterAbilityTestDefinition(Skills,Skill);
    return Skill;
}

void RaiseToLevel(UMythicSkillComponent *Skills, UMythicSkillDefinition *Skill, int32 TargetLevel) {
    for (int32 Step = Skills->GetSkillLevel(Skill); Step < TargetLevel; Step++) {
        Skills->GrantSkillLevel(Skill);
    }
}

FGameplayTag ProgressTestTag(const TCHAR *Name) {
    return FGameplayTag::RequestGameplayTag(FName(Name), false);
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSkillLevelGrantsPointTest,
    "Mythic.Progression.Skills.LevelGrantsPoint",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillLevelGrantsPointTest::RunTest(const FString &Parameters) {
    FMythicSkillProgressFixture Fixture;
    const bool bReady = BuildSkillProgressFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            DestroyOwnedAbilityTestWorld(Fixture.GameInstance);
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicSkillComponent *Skills = Fixture.Skills;
    const int32 Ceiling = Skills->MaxSkillLevel;
    if (!TestTrue(TEXT("the authored MaxSkillLevel leaves room to grow"), Ceiling >= 2)) {
        return false;
    }

    // One thing to buy per level, so the safety clamp is what stops this skill rather than its own content.
    UMythicSkillDefinition *Skill = MakeSkillWithNModifiers(Skills,Ceiling);
    if (!TestEqual(TEXT("a skill authoring a modifier per level climbs to the clamp"),
                   Skills->GetMaxSkillLevel(Skill), Ceiling)) {
        return false;
    }

    // Learning is durable even before practice or modifier selection.
    TestEqual(TEXT("an untouched skill is level one"), Skills->GetSkillLevel(Skill), 1);
    TestEqual(TEXT("and holds the one point that level granted"), Skills->GetAvailablePoints(Skill), 1);
    TestEqual(TEXT("learning stores one ownership row"), Skills->GetSkillProgress().Num(), 1);
    TestTrue(TEXT("the saved row records ownership"), Skills->GetSkillProgress()[0].bLearned);
    TestEqual(TEXT("learning does not fabricate practice"), Skills->GetSkillUses(Skill), 0);
    Skills->LearnSkill(Skill);
    TestEqual(TEXT("a repeated reward does not duplicate ownership"), Skills->GetSkillProgress().Num(), 1);

    // Every one of the Ceiling-1 level-ups available must move both numbers by exactly one, or the budget drifts.
    for (int32 Expected = 2; Expected <= Ceiling; Expected++) {
        const int32 Before = Skills->GetAvailablePoints(Skill);
        Skills->GrantSkillLevel(Skill);
        TestEqual(FString::Printf(TEXT("level-up %d lands on level %d"), Expected - 1, Expected),
                  Skills->GetSkillLevel(Skill), Expected);
        TestEqual(FString::Printf(TEXT("and grants exactly one point (%d of %d)"), Expected - 1, Ceiling - 1),
                  Skills->GetAvailablePoints(Skill), Before + 1);
    }

    // The ceiling has to hold, or a grant repeated in a loop levels past what the design allows.
    Skills->GrantSkillLevel(Skill);
    TestEqual(TEXT("nothing levels past MaxSkillLevel"), Skills->GetSkillLevel(Skill), Ceiling);
    TestEqual(TEXT("and no point is granted for the refusal"), Skills->GetAvailablePoints(Skill), Ceiling);

    // Growth is per skill: one skill's levels must never pay for another's modifiers.
    UMythicSkillDefinition *Untouched = MakeSkillWithNModifiers(Skills,1);
    TestEqual(TEXT("a second skill is still level one"), Skills->GetSkillLevel(Untouched), 1);
    TestEqual(TEXT("with its own single point"), Skills->GetAvailablePoints(Untouched), 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSkillModifierCapacityGateTest,
    "Mythic.Progression.Skills.ModifierCapacityGate",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillModifierCapacityGateTest::RunTest(const FString &Parameters) {
    FMythicSkillProgressFixture Fixture;
    const bool bReady = BuildSkillProgressFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            DestroyOwnedAbilityTestWorld(Fixture.GameInstance);
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicSkillComponent *Skills = Fixture.Skills;
    if (!TestTrue(TEXT("the authored MaxModifierCapacity leaves room to grow"), Skills->MaxModifierCapacity >= 2)) {
        return false;
    }

    UMythicSkillDefinition *Skill = MakeSkillWithModifiers(Skills,{
        MakeModifier(100.0f, 1), MakeModifier(50.0f, 1), MakeModifier(25.0f, 1)});
    RaiseToLevel(Skills, Skill, 3);

    // Points must not be what refuses the second modifier, or this test would pass with the capacity gate deleted.
    if (!TestEqual(TEXT("three levels bought three points"), Skills->GetAvailablePoints(Skill), 3)) {
        return false;
    }
    if (!TestEqual(TEXT("a fresh character carries one modifier at once"), Skills->GetModifierCapacity(), 1)) {
        return false;
    }

    Skills->ServerSetModifierActive(Skill, 0, true);
    TestTrue(TEXT("the first modifier switches on"), Skills->IsModifierActive(Skill, 0));
    TestEqual(TEXT("and spends its point"), Skills->GetAvailablePoints(Skill), 2);

    Skills->ServerSetModifierActive(Skill, 1, true);
    TestFalse(TEXT("the second is refused with the capacity full"), Skills->IsModifierActive(Skill, 1));
    TestEqual(TEXT("and costs nothing for the refusal"), Skills->GetAvailablePoints(Skill), 2);
    TestEqual(TEXT("one of one carried"), Skills->GetActiveModifiers(Skill).Num(), 1);

    // The gate has to be the capacity, not a blanket refusal: raise it and the same call must succeed.
    Skills->GrantModifierCapacity();
    TestEqual(TEXT("the unlock raises the capacity by one"), Skills->GetModifierCapacity(), 2);
    Skills->ServerSetModifierActive(Skill, 1, true);
    TestTrue(TEXT("the second modifier now switches on"), Skills->IsModifierActive(Skill, 1));
    TestEqual(TEXT("two of two carried"), Skills->GetActiveModifiers(Skill).Num(), 2);
    TestEqual(TEXT("and it spent the second point"), Skills->GetAvailablePoints(Skill), 1);

    // Repeated unlocks stop at the ceiling rather than running away behind the clamp.
    for (int32 Attempt = 0; Attempt < Skills->MaxModifierCapacity + 4; Attempt++) {
        Skills->GrantModifierCapacity();
    }
    TestEqual(TEXT("capacity stops at MaxModifierCapacity"), Skills->GetModifierCapacity(), Skills->MaxModifierCapacity);

    // Switching off hands the point and the carried slot back, or a build could never be changed.
    Skills->ServerSetModifierActive(Skill, 0, false);
    TestFalse(TEXT("the modifier switches off"), Skills->IsModifierActive(Skill, 0));
    TestEqual(TEXT("and refunds its point"), Skills->GetAvailablePoints(Skill), 2);
    TestEqual(TEXT("leaving one carried"), Skills->GetActiveModifiers(Skill).Num(), 1);

    // An index the definition does not have is refused rather than stored for the ability to read past the end.
    Skills->ServerSetModifierActive(Skill, 9, true);
    TestEqual(TEXT("an out-of-range modifier is not carried"), Skills->GetActiveModifiers(Skill).Num(), 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSkillModifierPointGateTest,
    "Mythic.Progression.Skills.ModifierPointGate",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillModifierPointGateTest::RunTest(const FString &Parameters) {
    FMythicSkillProgressFixture Fixture;
    const bool bReady = BuildSkillProgressFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            DestroyOwnedAbilityTestWorld(Fixture.GameInstance);
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicSkillComponent *Skills = Fixture.Skills;

    const int32 DearCost = 4;
    if (!TestTrue(TEXT("the authored MaxSkillLevel can pay for the dear modifier and its cheap sibling"),
                  Skills->MaxSkillLevel >= DearCost + 1)) {
        return false;
    }

    // The ceiling is the modifier count, so the skill has to offer DearCost + 1 things to buy before it can ever
    // hold DearCost + 1 points. The rows past the two under test are never bought.
    UMythicSkillDefinition *Skill = MakeSkillWithNModifiers(Skills,DearCost + 1);
    Skill->Modifiers[0] = MakeModifier(100.0f, DearCost);
    Skill->Modifiers[1] = MakeModifier(50.0f, 1);
    RegisterAbilityTestDefinition(Skills,Skill);

    // Capacity must not be what refuses the buy, or this test would pass with the point gate deleted.
    for (int32 Attempt = 0; Attempt < Skills->MaxModifierCapacity; Attempt++) {
        Skills->GrantModifierCapacity();
    }
    if (!TestTrue(TEXT("there is room to carry both modifiers"), Skills->GetModifierCapacity() >= 2)) {
        return false;
    }
    if (!TestEqual(TEXT("a level-one skill holds one point"), Skills->GetAvailablePoints(Skill), 1)) {
        return false;
    }

    Skills->ServerSetModifierActive(Skill, 0, true);
    TestFalse(FString::Printf(TEXT("a %d-point modifier is refused on one point"), DearCost),
              Skills->IsModifierActive(Skill, 0));
    TestEqual(TEXT("and nothing is spent for the refusal"), Skills->GetAvailablePoints(Skill), 1);

    // The gate has to be the points, not the price: the cheap sibling must go on with the same budget.
    Skills->ServerSetModifierActive(Skill, 1, true);
    TestTrue(TEXT("the one-point modifier goes on"), Skills->IsModifierActive(Skill, 1));
    TestEqual(TEXT("spending the only point"), Skills->GetAvailablePoints(Skill), 0);

    // Levelling pays for it. Exactly enough, so an off-by-one in the budget shows up here.
    RaiseToLevel(Skills, Skill, DearCost);
    TestEqual(TEXT("levelling leaves the price short by one"), Skills->GetAvailablePoints(Skill), DearCost - 1);
    Skills->ServerSetModifierActive(Skill, 0, true);
    TestFalse(TEXT("one point short is still refused"), Skills->IsModifierActive(Skill, 0));

    Skills->GrantSkillLevel(Skill);
    TestEqual(TEXT("one more level pays the price exactly"), Skills->GetAvailablePoints(Skill), DearCost);
    Skills->ServerSetModifierActive(Skill, 0, true);
    TestTrue(TEXT("and the dear modifier goes on"), Skills->IsModifierActive(Skill, 0));
    TestEqual(TEXT("spending every point it had"), Skills->GetAvailablePoints(Skill), 0);
    TestEqual(TEXT("the spend adds up to both prices"), Skills->GetSpentPoints(Skill), DearCost + 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSkillInertModifierTest,
    "Mythic.Progression.Skills.InertModifier",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillInertModifierTest::RunTest(const FString &Parameters) {
    // A modifier that costs a point and changes nothing is a content defect. Each of the six deltas on its own has to
    // be enough to make it real, or authoring one of them would still read as inert.
    const FGameplayTag Bleed = ProgressTestTag(TEXT("Status.Type.Bleed"));

    TestFalse(TEXT("a modifier with no deltas is inert"), FMythicSkillModifier().HasEffect());

    int32 Proved = 0;
    const int32 Denominator = Bleed.IsValid() ? 6 : 5;
    {
        FMythicSkillModifier M;
        M.RadiusDelta = 50.0f;
        Proved += TestTrue(TEXT("a radius delta makes it real"), M.HasEffect()) ? 1 : 0;
    }
    {
        FMythicSkillModifier M;
        M.TargetCountDelta = 1;
        Proved += TestTrue(TEXT("a target-count delta makes it real"), M.HasEffect()) ? 1 : 0;
    }
    {
        FMythicSkillModifier M;
        M.DurationDelta = 2.0f;
        Proved += TestTrue(TEXT("a duration delta makes it real"), M.HasEffect()) ? 1 : 0;
    }
    {
        FMythicSkillModifier M;
        M.MovementDistanceDelta = 200.0f;
        Proved += TestTrue(TEXT("a movement delta makes it real"), M.HasEffect()) ? 1 : 0;
    }
    {
        FMythicSkillModifier M;
        M.StatusChanceDelta = 0.25f;
        Proved += TestTrue(TEXT("a status-chance delta makes it real"), M.HasEffect()) ? 1 : 0;
    }
    if (Bleed.IsValid()) {
        FMythicSkillModifier M;
        M.StatusOverride = Bleed;
        Proved += TestTrue(TEXT("a status override makes it real"), M.HasEffect()) ? 1 : 0;
    }
    else {
        AddInfo(TEXT("Status.Type.Bleed is not registered; the status-override field is untested"));
    }
    TestEqual(FString::Printf(TEXT("%d of %d delta fields answer for the modifier"), Proved, Denominator),
              Proved, Denominator);

    // A negative delta is a trade, not an absence: shrinking the skill still counts as doing something.
    FMythicSkillModifier Traded;
    Traded.RadiusDelta = -100.0f;
    TestTrue(TEXT("a negative delta is still a real modifier"), Traded.HasEffect());

    // And the server must not sell one. The sibling with a delta proves the refusal is about the inert row.
    FMythicSkillProgressFixture Fixture;
    const bool bReady = BuildSkillProgressFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            DestroyOwnedAbilityTestWorld(Fixture.GameInstance);
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicSkillComponent *Skills = Fixture.Skills;

    FMythicSkillModifier Inert;
    Inert.PointCost = 1;
    UMythicSkillDefinition *Skill = MakeSkillWithModifiers(Skills,{Inert, MakeModifier(100.0f, 1)});
    RaiseToLevel(Skills, Skill, 2);

    TestEqual(TEXT("the inert choice reports a structured refusal"), Skills->Preflight(EMythicAbilityAction::Modifier,0,Skill,Skill->Modifiers[0].ModifierId,true), EMythicAbilityResult::InvalidModifier);
    Skills->ServerSetModifierActive(Skill, 0, true);
    TestFalse(TEXT("the inert modifier is refused"), Skills->IsModifierActive(Skill, 0));
    TestEqual(TEXT("and costs nothing"), Skills->GetAvailablePoints(Skill), 2);

    Skills->ServerSetModifierActive(Skill, 1, true);
    TestTrue(TEXT("its sibling with a delta goes on"), Skills->IsModifierActive(Skill, 1));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSkillProgressRoundTripTest,
    "Mythic.Progression.Skills.ProgressRoundTrip",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillProgressRoundTripTest::RunTest(const FString &Parameters) {
    FMythicSkillProgressFixture Fixture;
    const bool bReady = BuildSkillProgressFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            DestroyOwnedAbilityTestWorld(Fixture.GameInstance);
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicSkillComponent *Skills = Fixture.Skills;
    if (!TestTrue(TEXT("the authored ceilings leave room for a real build"),
                  Skills->MaxSkillLevel >= 3 && Skills->MaxModifierCapacity >= 2)) {
        return false;
    }

    UMythicSkillDefinition *Built = MakeSkillWithModifiers(Skills,{
        MakeModifier(100.0f, 1), MakeModifier(50.0f, 1), MakeModifier(25.0f, 1)});
    UMythicSkillDefinition *Levelled = MakeSkillWithNModifiers(Skills,2);

    Skills->GrantModifierCapacity();
    RaiseToLevel(Skills, Built, 3);
    Skills->ServerSetModifierActive(Built, 0, true);
    Skills->ServerSetModifierActive(Built, 2, true);
    RaiseToLevel(Skills, Levelled, 2);

    if (!TestEqual(TEXT("the build is two modifiers deep"), Skills->GetActiveModifiers(Built).Num(), 2)) {
        return false;
    }

    FSerializedCharacterData Data;
    if (!TestTrue(TEXT("the character serialises"), FSerializedCharacterData::Serialize(Fixture.PlayerState, Data))) {
        return false;
    }

    // The component's SaveGame flag is inert here: this project persists an explicit field list, so growth only
    // survives a reload if the character save writes it itself.
    TestEqual(TEXT("the save carries the modifier capacity"), Data.SkillModifierCapacity, Skills->GetModifierCapacity());
    if (!TestEqual(TEXT("the save carries one row per levelled skill"), Data.SkillProgress.Num(), 2)) {
        return false;
    }
    TestEqual(TEXT("the save is stamped at the current version"), Data.DataVersion,
              static_cast<int32>(CurrentCharacterSaveVersion));

    // A second owner in the same isolated world exercises a real restoration, then a repeated load.
    auto* RestoredPlayer=Fixture.GameInstance->GetWorld()->SpawnActor<AMythicPlayerState>();
    RestoredPlayer->AuthoritySetPersistentEntityId(Fixture.PlayerState->GetPersistentEntityId());
    RestoredPlayer->SetPersistentCharacterId(Fixture.PlayerState->GetPersistentCharacterId());
    RestoredPlayer->GetMythicAbilitySystemComponent()->InitAbilityActorInfo(RestoredPlayer,RestoredPlayer);
    Skills=RestoredPlayer->GetSkillComponent();
    Skills->SetRestoring(true);
    Skills->RestoreSkillProgress(Data.SkillProgress,Data.SkillModifierCapacity);
    TArray<TSoftObjectPtr<UMythicSkillDefinition>> RestoredSlots;
    for (const auto& Path:Data.EquippedSkills) RestoredSlots.Add(TSoftObjectPtr<UMythicSkillDefinition>(Path));
    Skills->RestoreSkills(RestoredSlots,Data.UnlockedSkillSlots);
    Skills->SetRestoring(false);
    Skills->RestoreSkillProgress(Data.SkillProgress,Data.SkillModifierCapacity);

    TestEqual(TEXT("the capacity comes back"), Skills->GetModifierCapacity(), 2);
    TestEqual(TEXT("the built skill keeps its level"), Skills->GetSkillLevel(Built), 3);
    TestEqual(TEXT("the second skill keeps its own"), Skills->GetSkillLevel(Levelled), 2);
    TestTrue(TEXT("the first chosen modifier comes back"), Skills->IsModifierActive(Built, 0));
    TestTrue(TEXT("so does the third"), Skills->IsModifierActive(Built, 2));
    TestFalse(TEXT("and the one never chosen stays off"), Skills->IsModifierActive(Built, 1));
    TestEqual(TEXT("the spend adds up as it did before the save"), Skills->GetSpentPoints(Built), 2);
    TestEqual(TEXT("leaving the same point unspent"), Skills->GetAvailablePoints(Built), 1);

    // A file must not be trusted over the live rules. Shrink what a character may carry and the overflow has to go,
    // or a save could smuggle a build past the capacity gate.
    Skills->RestoreSkillProgress(Data.SkillProgress, 1);
    TestEqual(TEXT("a shrunk capacity drops the overflow"), Skills->GetActiveModifiers(Built).Num(), 1);
    TestEqual(TEXT("and keeps the level that paid for it"), Skills->GetSkillLevel(Built), 3);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSkillModifierCapacityMigrationTest,
    "Mythic.Progression.Skills.ModifierCapacityMigration",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillModifierCapacityMigrationTest::RunTest(const FString &Parameters) {
    const FGameplayTag Modifier2 = ProgressTestTag(TEXT("Unlock.Rule.SkillModifier2"));
    const FGameplayTag Modifier3 = ProgressTestTag(TEXT("Unlock.Rule.SkillModifier3"));
    const FGameplayTag SkillSlot2 = ProgressTestTag(TEXT("Unlock.Rule.SkillSlot2"));
    const FGameplayTag RuneSlot2 = ProgressTestTag(TEXT("Unlock.Rule.RuneSlot2"));
    if (!Modifier2.IsValid() || !Modifier3.IsValid()) {
        AddError(TEXT("Unlock.Rule.SkillModifier2 / SkillModifier3 are not registered; the migration cannot be counted"));
        return false;
    }

    using Migration = FMythicCharacterSaveMigration;

    TestEqual(TEXT("a save that earned nothing still carries one modifier"),
              Migration::SkillModifierCapacityFromAppliedRules({}), 1);
    TestEqual(TEXT("one earned rule carries two"),
              Migration::SkillModifierCapacityFromAppliedRules({Modifier2}), 2);
    TestEqual(TEXT("both earned rules carry three"),
              Migration::SkillModifierCapacityFromAppliedRules({Modifier2, Modifier3}), 3);
    TestEqual(TEXT("an invalid entry is ignored rather than counted"),
              Migration::SkillModifierCapacityFromAppliedRules({FGameplayTag(), Modifier2}), 2);

    // The three counts share the Unlock.Rule parent, and Unlock.Rule.Skill* is a prefix of two of them. A loose scan
    // would hand out a modifier for every ability slot, so each pair has to stay blind to the other.
    if (SkillSlot2.IsValid()) {
        TestEqual(TEXT("an ability slot rule carries no extra modifier"),
                  Migration::SkillModifierCapacityFromAppliedRules({SkillSlot2}), 1);
        TestEqual(TEXT("and a modifier rule opens no ability slot"),
                  Migration::SkillSlotsFromAppliedRules({Modifier2, Modifier3}), 1);
    }
    if (RuneSlot2.IsValid()) {
        TestEqual(TEXT("a rune socket rule carries no extra modifier"),
                  Migration::SkillModifierCapacityFromAppliedRules({RuneSlot2}), 1);
        TestEqual(TEXT("and a modifier rule opens no rune socket"),
                  Migration::RuneSlotsFromAppliedRules({Modifier2}), 1);
    }

    // A save written before modifiers existed: it carries the rules and no count, and RestoreUnlockState re-latches
    // those rules so they never fire again. Everything else in it has to survive the repair untouched.
    UMythicSaveGame *Old = NewObject<UMythicSaveGame>();
    Old->CharacterData.DataVersion = static_cast<int32>(EMythicCharacterSaveVersion::PreSkillModifiers);
    Old->CharacterData.CharacterName = TEXT("Aldric");
    Old->CharacterData.AppliedUnlockRules = {Modifier2};
    if (SkillSlot2.IsValid()) {
        Old->CharacterData.AppliedUnlockRules.Add(SkillSlot2);
    }
    Old->CharacterData.UnlockedSkillSlots = 2;
    Old->CharacterData.UnlockedRuneSlots = 3;
    Old->CharacterData.StoryTags = {Modifier2};
    Old->FixupData();

    TestEqual(TEXT("the earned capacity is rebuilt from the applied rule"), Old->CharacterData.SkillModifierCapacity, 2);
    TestEqual(TEXT("and the save is stamped forward"), Old->CharacterData.DataVersion,
              static_cast<int32>(CurrentCharacterSaveVersion));
    TestEqual(TEXT("the name survives the repair"), Old->CharacterData.CharacterName, FString(TEXT("Aldric")));
    TestEqual(TEXT("the old save receives all four baseline ability sockets"), Old->CharacterData.UnlockedSkillSlots, 4);
    TestEqual(TEXT("and the rune sockets"), Old->CharacterData.UnlockedRuneSlots, 3);
    TestEqual(TEXT("and its story ledger"), Old->CharacterData.StoryTags.Num(), 1);
    TestEqual(TEXT("a save from before modifiers holds no build to lose"), Old->CharacterData.SkillProgress.Num(), 0);

    // A save from before either system: every repair has to run, not just the newest one.
    if (SkillSlot2.IsValid() && RuneSlot2.IsValid()) {
        UMythicSaveGame *Ancient = NewObject<UMythicSaveGame>();
        Ancient->CharacterData.DataVersion = static_cast<int32>(EMythicCharacterSaveVersion::PreRunes);
        Ancient->CharacterData.AppliedUnlockRules = {RuneSlot2, SkillSlot2, Modifier2};
        Ancient->FixupData();
        TestEqual(TEXT("an older save still recovers its rune sockets"), Ancient->CharacterData.UnlockedRuneSlots, 2);
        TestEqual(TEXT("and all four baseline ability sockets"), Ancient->CharacterData.UnlockedSkillSlots, 4);
        TestEqual(TEXT("and its modifier capacity in the same pass"), Ancient->CharacterData.SkillModifierCapacity, 2);
    }

    // The version marker has to be read, not merely written. A current save that spent its capacity back down must
    // keep that, or every load would re-derive a number the player no longer has.
    UMythicSaveGame *Current = NewObject<UMythicSaveGame>();
    Current->CharacterData.DataVersion = static_cast<int32>(CurrentCharacterSaveVersion);
    Current->CharacterData.AppliedUnlockRules = {Modifier2, Modifier3};
    Current->CharacterData.SkillModifierCapacity = 1;
    Current->FixupData();
    TestEqual(TEXT("a save already at the current version is left alone"), Current->CharacterData.SkillModifierCapacity, 1);

    // A brand-new save writes no capacity, and the restore seam is what treats that as "leave the component alone".
    UMythicSaveGame *Fresh = NewObject<UMythicSaveGame>();
    Fresh->CharacterData.DataVersion = static_cast<int32>(CurrentCharacterSaveVersion);
    Fresh->FixupData();
    TestEqual(TEXT("a fresh save carries no capacity for the restore to apply"),
              Fresh->CharacterData.SkillModifierCapacity, 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityIndependentMasteryTest,"Mythic.Progression.Skills.IndependentMastery",EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FAbilityIndependentMasteryTest::RunTest(const FString&) {
    FMythicSkillProgressFixture F;
    if (!BuildSkillProgressFixture(*this,F)) return false;
    ON_SCOPE_EXIT { DestroyOwnedAbilityTestWorld(F.GameInstance); };
    auto* Skill=NewObject<UMythicSkillDefinition>(); Skill->Ability=UGameplayAbility::StaticClass();
    Skill->bHasMasteryTrack=true; Skill->MasteryUseThresholds={2,5};
    RegisterAbilityTestDefinition(F.Skills,Skill,true,false);
    TestEqual(TEXT("zero behavior rows can have three ranks"),F.Skills->GetMaxSkillLevel(Skill),3);
    for (int32 i=0;i<5;++i) F.Skills->RecordSkillUse(Skill);
    TestEqual(TEXT("authored cumulative threshold reached"),F.Skills->GetSkillLevel(Skill),3);
    TestEqual(TEXT("rank is not refinement currency"),F.Skills->GetGrantedPoints(Skill),0);
    auto M=MakeModifier(25,1); M.ModifierId=TEXT("NewBehavior"); Skill->Modifiers.Add(M);
    TestEqual(TEXT("adding a behavior cannot change the track"),F.Skills->GetMaxSkillLevel(Skill),3);
    const auto Saved=F.Skills->GetSkillProgress();
    Skill->bHasMasteryTrack=false; Skill->MasteryUseThresholds.Reset();
    F.Skills->RestoreSkillProgress(Saved,1);
    TestEqual(TEXT("disabled track retains practice for save roundtrip"),F.Skills->GetSkillUses(Skill),5);
    TestEqual(TEXT("a behavior alone creates no mastery ranks"),F.Skills->GetMaxSkillLevel(Skill),1);
    Skill->bHasMasteryTrack=true; Skill->MasteryUseThresholds={2,5};
    F.Skills->RestoreSkillProgress(F.Skills->GetSkillProgress(),1);
    TestEqual(TEXT("second restore recovers retained rank"),F.Skills->GetSkillLevel(Skill),3);
    return true;
}

#endif
