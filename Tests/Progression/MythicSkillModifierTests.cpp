
#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameplayAbilitySpec.h"
#include "GAS/Abilities/MythicGA_Skill.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "Misc/ScopeExit.h"
#include "Player/MythicPlayerState.h"
#include "Progression/Skills/MythicSkillComponent.h"
#include "Progression/Skills/MythicSkillDefinition.h"

namespace {

// The fold reads the caster's skill component through the ability's actor info and its definition off the granted
// spec, so nothing short of a real owner carrying both can exercise it. The player state is that owner in the game.
struct FMythicSkillModifierFixture {
    UGameInstance *GameInstance = nullptr;
    AMythicPlayerState *PlayerState = nullptr;
    UMythicSkillComponent *Skills = nullptr;
    UMythicAbilitySystemComponent *ASC = nullptr;
    UMythicSkillDefinition *Definition = nullptr;
    UMythicGA_Skill *Ability = nullptr;
};

FMythicSkillModifier MakeModifier(const int32 PointCost = 1) {
    FMythicSkillModifier Modifier;
    Modifier.PointCost = PointCost;
    return Modifier;
}

bool BuildModifierFixture(FAutomationTestBase &Test, FMythicSkillModifierFixture &Out,
                          const TArray<FMythicSkillModifier> &Modifiers) {
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
    Out.ASC = Out.PlayerState->GetMythicAbilitySystemComponent();
    if (!Test.TestNotNull(TEXT("the player state owns an ability system"), Out.ASC)) {
        return false;
    }
    if (!Out.Skills->IsRegistered()) {
        Out.Skills->RegisterComponent();
    }
    if (!Out.ASC->IsRegistered()) {
        Out.ASC->RegisterComponent();
    }
    Out.ASC->InitAbilityActorInfo(Out.PlayerState, Out.PlayerState);

    // A standalone world never initialises actor components, so the ability system never runs the pass that adopts
    // the player state's own attribute sets. The set has to be handed to it by name or every bonus reads as zero.
    if (!Out.ASC->GetSet<UMythicAttributeSet_Offense>()) {
        Out.ASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Offense>(Out.PlayerState));
    }

    Out.Definition = NewObject<UMythicSkillDefinition>();
    Out.Definition->Ability = UMythicGA_Skill::StaticClass();
    Out.Definition->Modifiers = Modifiers;

    // The definition is the spec's source object, which is the only thread from a fired skill back to the authored
    // modifiers it was granted from.
    const FGameplayAbilitySpec Spec(UMythicGA_Skill::StaticClass(), 1, INDEX_NONE, Out.Definition);
    const FGameplayAbilitySpecHandle Handle = Out.ASC->GiveAbility(Spec);
    if (!Test.TestTrue(TEXT("the skill was granted"), Handle.IsValid())) {
        return false;
    }
    const FGameplayAbilitySpec *Granted = Out.ASC->FindAbilitySpecFromHandle(Handle);
    if (!Test.TestNotNull(TEXT("the granted spec is found"), Granted)) {
        return false;
    }
    Out.Ability = Cast<UMythicGA_Skill>(Granted->GetPrimaryInstance());
    if (!Test.TestNotNull(TEXT("the skill is instanced per actor, so it has an owner to read its build off"), Out.Ability)) {
        return false;
    }
    return Test.TestNotNull(TEXT("the owner carries the offense attributes the fold sits beside"),
                            Out.ASC->GetSet<UMythicAttributeSet_Offense>());
}

// Capacity is one by default, so a build is proved one modifier at a time: switch on, read, switch off, and the
// point comes back for the next one.
bool ToggleModifier(UMythicSkillComponent *Skills, UMythicSkillDefinition *Skill, const int32 Index, const bool bActive) {
    Skills->ServerSetModifierActive(Skill, Index, bActive);
    return Skills->IsModifierActive(Skill, Index) == bActive;
}

FGameplayTag ModifierTestTag(const TCHAR *Name) {
    return FGameplayTag::RequestGameplayTag(FName(Name), false);
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSkillModifierTotalsTest,
    "Mythic.Progression.Skills.ModifierTotals",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillModifierTotalsTest::RunTest(const FString &Parameters) {
    // What several modifiers add up to, with no world in it. Every field has to sum, or a build two modifiers deep
    // would quietly count only one of them.
    TArray<FMythicSkillModifier> Modifiers;
    Modifiers.Add(MakeModifier());
    Modifiers[0].RadiusDelta = 100.0f;
    Modifiers[0].TargetCountDelta = 1;
    Modifiers[0].DurationDelta = 2.0f;
    Modifiers[0].MovementDistanceDelta = 150.0f;
    Modifiers[0].StatusChanceDelta = 0.1f;

    Modifiers.Add(MakeModifier());
    Modifiers[1].RadiusDelta = 50.0f;
    Modifiers[1].TargetCountDelta = 2;
    Modifiers[1].DurationDelta = 1.0f;
    Modifiers[1].MovementDistanceDelta = 50.0f;
    Modifiers[1].StatusChanceDelta = 0.15f;

    // A trade: this one gives radius back to buy nothing else, so a sum that took the largest or the last would read
    // differently from one that added.
    Modifiers.Add(MakeModifier());
    Modifiers[2].RadiusDelta = -30.0f;

    const FMythicSkillModifierTotals None = FMythicSkillModifierTotals::Sum(Modifiers, {});
    TestEqual(TEXT("no modifier switched on changes no radius"), None.RadiusDelta, 0.0f);
    TestEqual(TEXT("nor any target count"), None.TargetCountDelta, 0);
    TestEqual(TEXT("nor any duration"), None.DurationDelta, 0.0f);
    TestEqual(TEXT("nor any movement"), None.MovementDistanceDelta, 0.0f);
    TestEqual(TEXT("nor any status chance"), None.StatusChanceDelta, 0.0f);
    TestFalse(TEXT("and overrides no status"), None.StatusOverride.IsValid());

    const FMythicSkillModifierTotals Both = FMythicSkillModifierTotals::Sum(Modifiers, {0, 1});
    AddInfo(FString::Printf(TEXT("summing 2 of %d authored modifiers across 5 numeric fields"), Modifiers.Num()));
    TestEqual(TEXT("two radius deltas add"), Both.RadiusDelta, 150.0f);
    TestEqual(TEXT("two target-count deltas add"), Both.TargetCountDelta, 3);
    TestEqual(TEXT("two duration deltas add"), Both.DurationDelta, 3.0f);
    TestEqual(TEXT("two movement deltas add"), Both.MovementDistanceDelta, 200.0f);
    TestEqual(TEXT("two status-chance deltas add"), Both.StatusChanceDelta, 0.25f);

    const FMythicSkillModifierTotals Traded = FMythicSkillModifierTotals::Sum(Modifiers, {0, 1, 2});
    TestEqual(TEXT("a negative delta is subtracted, not ignored"), Traded.RadiusDelta, 120.0f);

    // Content that dropped a modifier leaves saved characters holding an index that no longer exists. It has to
    // contribute nothing rather than read past the end.
    const FMythicSkillModifierTotals Stale = FMythicSkillModifierTotals::Sum(Modifiers, {0, 9, -1});
    TestEqual(TEXT("an index the definition no longer has contributes nothing"), Stale.RadiusDelta, 100.0f);
    TestEqual(TEXT("and nothing to the count either"), Stale.TargetCountDelta, 1);

    // A status is a choice between modifiers, not a sum, so the later one has to win outright.
    const FGameplayTag Bleed = ModifierTestTag(TEXT("Status.Type.Bleed"));
    const FGameplayTag Freeze = ModifierTestTag(TEXT("Status.Type.Freeze"));
    if (Bleed.IsValid() && Freeze.IsValid()) {
        TArray<FMythicSkillModifier> Statuses;
        Statuses.Add(MakeModifier());
        Statuses[0].StatusOverride = Bleed;
        Statuses.Add(MakeModifier());
        Statuses[1].StatusOverride = Freeze;
        Statuses.Add(MakeModifier());
        Statuses[2].RadiusDelta = 10.0f;

        TestEqual(TEXT("one status modifier names the status"),
                  FMythicSkillModifierTotals::Sum(Statuses, {0}).StatusOverride, Bleed);
        TestEqual(TEXT("two of them settle on the later one"),
                  FMythicSkillModifierTotals::Sum(Statuses, {0, 1}).StatusOverride, Freeze);
        TestEqual(TEXT("and a modifier that names none leaves the choice standing"),
                  FMythicSkillModifierTotals::Sum(Statuses, {0, 2}).StatusOverride, Bleed);
    }
    else {
        AddError(TEXT("Status.Type.Bleed / Status.Type.Freeze are not registered; the status override cannot be counted"));
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSkillModifierResolvedShapeTest,
    "Mythic.Progression.Skills.ModifierResolvedShape",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillModifierResolvedShapeTest::RunTest(const FString &Parameters) {
    const FGameplayTag Burn = ModifierTestTag(TEXT("Status.Type.Burn"));
    const FGameplayTag Bleed = ModifierTestTag(TEXT("Status.Type.Bleed"));
    if (!Burn.IsValid() || !Bleed.IsValid()) {
        AddError(TEXT("Status.Type.Burn / Status.Type.Bleed are not registered; the status quantifier cannot be proved"));
        return false;
    }

    // Six quantifiers, one modifier each, so a delta wired to the wrong resolver shows up as the wrong number moving.
    TArray<FMythicSkillModifier> Modifiers;
    Modifiers.Add(MakeModifier());
    Modifiers[0].RadiusDelta = 150.0f;
    Modifiers.Add(MakeModifier());
    Modifiers[1].TargetCountDelta = 2;
    Modifiers.Add(MakeModifier());
    Modifiers[2].DurationDelta = 3.0f;
    Modifiers.Add(MakeModifier());
    Modifiers[3].MovementDistanceDelta = 200.0f;
    Modifiers.Add(MakeModifier());
    Modifiers[4].StatusChanceDelta = 0.25f;
    Modifiers.Add(MakeModifier());
    Modifiers[5].StatusOverride = Bleed;

    FMythicSkillModifierFixture Fixture;
    const bool bReady = BuildModifierFixture(*this, Fixture, Modifiers);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicSkillComponent *Skills = Fixture.Skills;
    UMythicSkillDefinition *Skill = Fixture.Definition;
    UMythicGA_Skill *Ability = Fixture.Ability;

    Ability->Shape.Shape = EMythicSkillShape::Sphere;
    Ability->Shape.Radius = 400.0f;
    Ability->Shape.MaxTargets = 2;
    Ability->SelfEffectDuration = 6.0f;
    Ability->MovementDistance = 500.0f;
    Ability->StatusChance = 0.5f;
    Ability->StatusToApply = Burn;

    // No gear on: the fold must start from the authored numbers, or every assertion below would measure a stat
    // instead of a modifier.
    Fixture.ASC->SetNumericAttributeBase(UMythicAttributeSet_Offense::GetSkillRadiusBonusAttribute(), 0.0f);
    Fixture.ASC->SetNumericAttributeBase(UMythicAttributeSet_Offense::GetSkillTargetCountBonusAttribute(), 0.0f);
    Fixture.ASC->SetNumericAttributeBase(UMythicAttributeSet_Offense::GetSkillDurationBonusAttribute(), 0.0f);

    if (!TestEqual(TEXT("a character with nothing switched on queries the authored radius"), Ability->ResolveShape().Radius, 400.0f)) {
        return false;
    }
    TestEqual(TEXT("and the authored cap"), Ability->ResolveShape().MaxTargets, 2);
    TestEqual(TEXT("and the authored duration"), Ability->ResolveSelfEffectDuration(), 6.0f);
    TestEqual(TEXT("and the authored distance"), Ability->ResolveMovementDistance(), 500.0f);
    TestEqual(TEXT("and the authored chance"), Ability->ResolveStatusChance(), 0.5f);
    TestEqual(TEXT("and inflicts the authored status"), Ability->ResolveStatusToApply(), Burn);

    int32 Moved = 0;
    const int32 Quantifiers = 6;

    if (TestTrue(TEXT("the radius modifier switches on"), ToggleModifier(Skills, Skill, 0, true))) {
        Moved += TestEqual(TEXT("a radius delta widens the query"), Ability->ResolveShape().Radius, 550.0f) ? 1 : 0;
        // Each delta owns one quantifier. Crossed wiring would show up nowhere else.
        TestEqual(TEXT("and leaves the cap alone"), Ability->ResolveShape().MaxTargets, 2);
        TestEqual(TEXT("and the duration alone"), Ability->ResolveSelfEffectDuration(), 6.0f);
        // Switching off has to give the reach back, or a build could never be changed.
        TestTrue(TEXT("the radius modifier switches off"), ToggleModifier(Skills, Skill, 0, false));
        TestEqual(TEXT("and the query narrows back to the authored reach"), Ability->ResolveShape().Radius, 400.0f);
    }

    if (TestTrue(TEXT("the target-count modifier switches on"), ToggleModifier(Skills, Skill, 1, true))) {
        Moved += TestEqual(TEXT("a target-count delta raises the queried cap"), Ability->ResolveShape().MaxTargets, 4) ? 1 : 0;
        TestEqual(TEXT("and leaves the radius alone"), Ability->ResolveShape().Radius, 400.0f);
        TestTrue(TEXT("the target-count modifier switches off"), ToggleModifier(Skills, Skill, 1, false));
        TestEqual(TEXT("and the cap falls back"), Ability->ResolveShape().MaxTargets, 2);
    }

    if (TestTrue(TEXT("the duration modifier switches on"), ToggleModifier(Skills, Skill, 2, true))) {
        Moved += TestEqual(TEXT("a duration delta lengthens the stance"), Ability->ResolveSelfEffectDuration(), 9.0f) ? 1 : 0;
        TestEqual(TEXT("and leaves the radius alone"), Ability->ResolveShape().Radius, 400.0f);
        TestTrue(TEXT("the duration modifier switches off"), ToggleModifier(Skills, Skill, 2, false));
        TestEqual(TEXT("and the stance shortens back"), Ability->ResolveSelfEffectDuration(), 6.0f);
    }

    if (TestTrue(TEXT("the movement modifier switches on"), ToggleModifier(Skills, Skill, 3, true))) {
        Moved += TestEqual(TEXT("a movement delta lengthens the dash"), Ability->ResolveMovementDistance(), 700.0f) ? 1 : 0;
        TestTrue(TEXT("the movement modifier switches off"), ToggleModifier(Skills, Skill, 3, false));
        TestEqual(TEXT("and the dash shortens back"), Ability->ResolveMovementDistance(), 500.0f);
    }

    if (TestTrue(TEXT("the status-chance modifier switches on"), ToggleModifier(Skills, Skill, 4, true))) {
        Moved += TestEqual(TEXT("a status-chance delta makes the status land more often"), Ability->ResolveStatusChance(), 0.75f) ? 1 : 0;
        TestEqual(TEXT("and does not change which status lands"), Ability->ResolveStatusToApply(), Burn);
        TestTrue(TEXT("the status-chance modifier switches off"), ToggleModifier(Skills, Skill, 4, false));
        TestEqual(TEXT("and the chance falls back"), Ability->ResolveStatusChance(), 0.5f);
    }

    if (TestTrue(TEXT("the status modifier switches on"), ToggleModifier(Skills, Skill, 5, true))) {
        Moved += TestEqual(TEXT("a status override changes what the skill inflicts"), Ability->ResolveStatusToApply(), Bleed) ? 1 : 0;
        TestEqual(TEXT("and leaves how often it lands alone"), Ability->ResolveStatusChance(), 0.5f);
        TestTrue(TEXT("the status modifier switches off"), ToggleModifier(Skills, Skill, 5, false));
        TestEqual(TEXT("and the authored status comes back"), Ability->ResolveStatusToApply(), Burn);
    }

    AddInfo(FString::Printf(TEXT("%d of %d quantifiers a modifier can move actually moved, and fell back when it was switched off"),
                            Moved, Quantifiers));
    TestEqual(FString::Printf(TEXT("%d of %d quantifiers a modifier can move actually moved"), Moved, Quantifiers),
              Moved, Quantifiers);

    // The clamps still run after the fold. A modifier that takes more than the skill has must shrink it, never invert
    // it, and must not turn a capped skill into one that hits the whole room.
    TArray<FMythicSkillModifier> Cursed;
    Cursed.Add(MakeModifier());
    Cursed[0].RadiusDelta = -1000.0f;
    Cursed[0].TargetCountDelta = -10;
    Cursed[0].DurationDelta = -100.0f;
    Cursed[0].MovementDistanceDelta = -9000.0f;
    Cursed[0].StatusChanceDelta = -5.0f;
    Skill->Modifiers = Cursed;

    if (TestTrue(TEXT("the cursed modifier switches on"), ToggleModifier(Skills, Skill, 0, true))) {
        TestEqual(TEXT("a crushing radius delta shrinks the skill rather than inverting it"), Ability->ResolveShape().Radius, 0.0f);
        TestEqual(TEXT("a crushing count delta never falls below a single target"), Ability->ResolveShape().MaxTargets, 1);
        const float Cut = Ability->ResolveSelfEffectDuration();
        TestEqual(TEXT("a crushing duration delta leaves a sliver, never a zero that means instant"),
                  Cut, UMythicGA_Skill::MinScaledDuration);
        TestTrue(TEXT("and that sliver is shorter than the stance it cut down"), Cut < 6.0f);
        TestEqual(TEXT("a crushing movement delta never dashes backwards"), Ability->ResolveMovementDistance(), 0.0f);
        TestEqual(TEXT("and a crushing chance delta stays a probability"), Ability->ResolveStatusChance(), 0.0f);
    }

    // The 0 sentinel means "whatever the effect authors" and "this skill does not move you". A modifier must not be
    // able to talk either one into a real number.
    Ability->SelfEffectDuration = 0.0f;
    Ability->MovementDistance = 0.0f;
    Ability->Shape.MaxTargets = 0;
    Skill->Modifiers = Modifiers;
    Skills->ServerSetModifierActive(Skill, 0, false);

    if (TestTrue(TEXT("the duration modifier switches on against the sentinel"), ToggleModifier(Skills, Skill, 2, true))) {
        TestEqual(TEXT("a duration modifier leaves the authored-0 sentinel alone"), Ability->ResolveSelfEffectDuration(), 0.0f);
        TestTrue(TEXT("it switches off again"), ToggleModifier(Skills, Skill, 2, false));
    }
    if (TestTrue(TEXT("the movement modifier switches on against the sentinel"), ToggleModifier(Skills, Skill, 3, true))) {
        TestEqual(TEXT("no modifier makes a standing skill dash"), Ability->ResolveMovementDistance(), 0.0f);
        TestTrue(TEXT("it switches off again"), ToggleModifier(Skills, Skill, 3, false));
    }
    if (TestTrue(TEXT("the target-count modifier switches on against the sentinel"), ToggleModifier(Skills, Skill, 1, true))) {
        TestEqual(TEXT("an uncapped skill stays uncapped however many targets a modifier adds"),
                  Ability->ResolveShape().MaxTargets, 0);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSkillModifierAttributeStackingTest,
    "Mythic.Progression.Skills.ModifierAttributeStacking",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillModifierAttributeStackingTest::RunTest(const FString &Parameters) {
    /**
     * A modifier folds in BESIDE the caster's gear, never instead of it. Reading either contribution alone would look
     * right in isolation and quietly halve every levelled skill on a geared character, so each quantifier is measured
     * four ways: neither, the modifier alone, the gear alone, and both.
     */
    TArray<FMythicSkillModifier> Modifiers;
    Modifiers.Add(MakeModifier());
    Modifiers[0].RadiusDelta = 150.0f;
    Modifiers[0].TargetCountDelta = 2;
    Modifiers[0].DurationDelta = 3.0f;

    Modifiers.Add(MakeModifier());
    Modifiers[1].RadiusDelta = -1000.0f;

    FMythicSkillModifierFixture Fixture;
    const bool bReady = BuildModifierFixture(*this, Fixture, Modifiers);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicSkillComponent *Skills = Fixture.Skills;
    UMythicSkillDefinition *Skill = Fixture.Definition;
    UMythicGA_Skill *Ability = Fixture.Ability;
    UMythicAbilitySystemComponent *ASC = Fixture.ASC;

    const FGameplayAttribute RadiusBonus = UMythicAttributeSet_Offense::GetSkillRadiusBonusAttribute();
    const FGameplayAttribute CountBonus = UMythicAttributeSet_Offense::GetSkillTargetCountBonusAttribute();
    const FGameplayAttribute DurationBonus = UMythicAttributeSet_Offense::GetSkillDurationBonusAttribute();

    Ability->Shape.Shape = EMythicSkillShape::Sphere;
    Ability->Shape.Radius = 400.0f;
    Ability->Shape.MaxTargets = 2;
    Ability->SelfEffectDuration = 6.0f;

    ASC->SetNumericAttributeBase(RadiusBonus, 0.0f);
    ASC->SetNumericAttributeBase(CountBonus, 0.0f);
    ASC->SetNumericAttributeBase(DurationBonus, 0.0f);

    const float BareRadius = Ability->ResolveShape().Radius;
    const int32 BareCount = Ability->ResolveShape().MaxTargets;
    const float BareDuration = Ability->ResolveSelfEffectDuration();
    if (!TestEqual(TEXT("bare: the authored radius"), BareRadius, 400.0f)) {
        return false;
    }
    TestEqual(TEXT("bare: the authored cap"), BareCount, 2);
    TestEqual(TEXT("bare: the authored duration"), BareDuration, 6.0f);

    // The modifier alone.
    if (!TestTrue(TEXT("the modifier switches on"), ToggleModifier(Skills, Skill, 0, true))) {
        return false;
    }
    const float ModifierRadius = Ability->ResolveShape().Radius;
    const int32 ModifierCount = Ability->ResolveShape().MaxTargets;
    const float ModifierDuration = Ability->ResolveSelfEffectDuration();
    TestEqual(TEXT("the modifier alone adds its radius"), ModifierRadius, 550.0f);
    TestEqual(TEXT("the modifier alone adds its targets"), ModifierCount, 4);
    TestEqual(TEXT("the modifier alone adds its seconds"), ModifierDuration, 9.0f);

    // The gear alone.
    TestTrue(TEXT("the modifier switches off"), ToggleModifier(Skills, Skill, 0, false));
    ASC->SetNumericAttributeBase(RadiusBonus, 50.0f);
    ASC->SetNumericAttributeBase(CountBonus, 3.0f);
    ASC->SetNumericAttributeBase(DurationBonus, 2.0f);
    const float GearRadius = Ability->ResolveShape().Radius;
    const int32 GearCount = Ability->ResolveShape().MaxTargets;
    const float GearDuration = Ability->ResolveSelfEffectDuration();
    TestEqual(TEXT("the gear alone adds its radius"), GearRadius, 450.0f);
    TestEqual(TEXT("the gear alone adds its targets"), GearCount, 5);
    TestEqual(TEXT("the gear alone adds its seconds"), GearDuration, 8.0f);

    // Both. This is the assertion the whole design rests on.
    if (!TestTrue(TEXT("the modifier switches back on over the gear"), ToggleModifier(Skills, Skill, 0, true))) {
        return false;
    }
    const float BothRadius = Ability->ResolveShape().Radius;
    const int32 BothCount = Ability->ResolveShape().MaxTargets;
    const float BothDuration = Ability->ResolveSelfEffectDuration();

    int32 Stacked = 0;
    const int32 Quantifiers = 3;

    AddInfo(FString::Printf(TEXT("%d quantifiers take both a modifier and an attribute; each measured bare, modifier-only, gear-only, both"),
                            Quantifiers));

    Stacked += TestEqual(TEXT("radius: modifier and gear both count"), BothRadius, 600.0f) ? 1 : 0;
    TestNotEqual(TEXT("radius: the gear did not replace the modifier"), BothRadius, GearRadius);
    TestNotEqual(TEXT("radius: nor the modifier the gear"), BothRadius, ModifierRadius);
    TestEqual(TEXT("radius: what both add is what each adds, summed"),
              BothRadius - BareRadius, (ModifierRadius - BareRadius) + (GearRadius - BareRadius));

    Stacked += TestEqual(TEXT("targets: modifier and gear both count"), BothCount, 7) ? 1 : 0;
    TestNotEqual(TEXT("targets: the gear did not replace the modifier"), BothCount, GearCount);
    TestNotEqual(TEXT("targets: nor the modifier the gear"), BothCount, ModifierCount);
    TestEqual(TEXT("targets: what both add is what each adds, summed"),
              BothCount - BareCount, (ModifierCount - BareCount) + (GearCount - BareCount));

    Stacked += TestEqual(TEXT("duration: modifier and gear both count"), BothDuration, 11.0f) ? 1 : 0;
    TestNotEqual(TEXT("duration: the gear did not replace the modifier"), BothDuration, GearDuration);
    TestNotEqual(TEXT("duration: nor the modifier the gear"), BothDuration, ModifierDuration);
    TestEqual(TEXT("duration: what both add is what each adds, summed"),
              BothDuration - BareDuration, (ModifierDuration - BareDuration) + (GearDuration - BareDuration));

    AddInfo(FString::Printf(TEXT("%d of %d quantifiers stack the modifier with the attribute rather than one replacing the other"),
                            Stacked, Quantifiers));
    TestEqual(FString::Printf(TEXT("%d of %d quantifiers stack the modifier with the attribute"), Stacked, Quantifiers),
              Stacked, Quantifiers);

    // A modifier and gear pulling opposite ways settle on the sum, and the clamp runs after that sum rather than
    // after either half — a floor applied per contribution would hand back a wider skill than the numbers asked for.
    TestTrue(TEXT("the giving modifier switches off"), ToggleModifier(Skills, Skill, 0, false));
    if (TestTrue(TEXT("the taking modifier switches on"), ToggleModifier(Skills, Skill, 1, true))) {
        TestEqual(TEXT("a crushing modifier over positive gear floors at zero, not below it"),
                  Ability->ResolveShape().Radius, 0.0f);
        ASC->SetNumericAttributeBase(RadiusBonus, 900.0f);
        TestEqual(TEXT("and enough gear pays the modifier back exactly"), Ability->ResolveShape().Radius, 300.0f);
    }

    return true;
}
