
#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "ScalableFloat.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicHealthBands.h"
#include "GAS/MythicTags_GAS.h"
#include "Settings/MythicCombatSettings.h"

namespace {
FMythicHealthBand Band(const FGameplayTag &Tag, float Min, float Max) {
    FMythicHealthBand Result;
    Result.Tag = Tag;
    Result.MinFraction = Min;
    Result.MaxFraction = Max;
    return Result;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHealthBandRulesTest,
    "Mythic.Combat.HealthBandRules",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicHealthBandRulesTest::RunTest(const FString &Parameters) {
    using Rules = FMythicHealthBandRules;

    TestEqual(TEXT("no maximum reads as unhurt"), Rules::FractionOf(0.0f, 0.0f), 1.0f);
    TestEqual(TEXT("half health is half"), Rules::FractionOf(50.0f, 100.0f), 0.5f);
    TestEqual(TEXT("overheal cannot exceed full"), Rules::FractionOf(300.0f, 100.0f), 1.0f);

    const FMythicHealthBand Low = Band(GAS_STATE_HEALTH_LOW, 0.0f, 0.5f);
    TestTrue(TEXT("exactly at the edge is inside"), Rules::BandContains(Low, 0.5f));
    TestFalse(TEXT("just past the edge is outside"), Rules::BandContains(Low, 0.5001f));
    TestTrue(TEXT("empty health is inside a low band"), Rules::BandContains(Low, 0.0f));

    // A row with no tag would publish nothing, so treating it as a match would only hide the authoring slip.
    TestFalse(TEXT("an untagged row never matches"), Rules::BandContains(Band(FGameplayTag(), 0.0f, 1.0f), 0.5f));

    // An inverted row is an authoring slip, not a request for a band that matches nothing.
    TestTrue(TEXT("an inverted row still covers its span"), Rules::BandContains(Band(GAS_STATE_HEALTH_LOW, 0.5f, 0.0f), 0.25f));

    const TArray<FMythicHealthBand> Bands = {
        Band(GAS_STATE_HEALTH_CRITICAL, 0.0f, 0.20f),
        Band(GAS_STATE_HEALTH_LOW, 0.0f, 0.50f),
        Band(GAS_STATE_HEALTH_WOUNDED, 0.0f, 0.90f),
        Band(GAS_STATE_HEALTH_UNHURT, 0.90f, 1.0f),
    };

    FGameplayTagContainer Active;

    // Nesting is the whole reason a two-tier talent is two modifiers instead of a special case.
    Rules::ResolveBands(Bands, 0.10f, Active);
    TestEqual(TEXT("a dying actor is in all three low bands"), Active.Num(), 3);
    TestTrue(TEXT("critical"), Active.HasTagExact(GAS_STATE_HEALTH_CRITICAL));
    TestTrue(TEXT("low"), Active.HasTagExact(GAS_STATE_HEALTH_LOW));
    TestTrue(TEXT("wounded"), Active.HasTagExact(GAS_STATE_HEALTH_WOUNDED));

    Rules::ResolveBands(Bands, 0.40f, Active);
    TestFalse(TEXT("40% is not critical"), Active.HasTagExact(GAS_STATE_HEALTH_CRITICAL));
    TestTrue(TEXT("40% is low"), Active.HasTagExact(GAS_STATE_HEALTH_LOW));

    Rules::ResolveBands(Bands, 1.0f, Active);
    TestEqual(TEXT("full health is unhurt and nothing else"), Active.Num(), 1);
    TestTrue(TEXT("unhurt"), Active.HasTagExact(GAS_STATE_HEALTH_UNHURT));

    // 0.90 sits on the shared edge of two bands and must belong to both, or a talent gated on one of them
    // silently misses the boundary case a designer is most likely to test.
    Rules::ResolveBands(Bands, 0.90f, Active);
    TestTrue(TEXT("the shared edge is wounded"), Active.HasTagExact(GAS_STATE_HEALTH_WOUNDED));
    TestTrue(TEXT("the shared edge is also unhurt"), Active.HasTagExact(GAS_STATE_HEALTH_UNHURT));

    // Resolve must clear what it found last time, or the container grows forever as health moves.
    Rules::ResolveBands(Bands, 0.10f, Active);
    Rules::ResolveBands(Bands, 1.0f, Active);
    TestEqual(TEXT("resolve replaces rather than accumulates"), Active.Num(), 1);

    const TArray<FMythicHealthBand> None;
    Rules::ResolveBands(None, 0.5f, Active);
    TestEqual(TEXT("no bands authored, nothing published"), Active.Num(), 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHealthBandTagTest,
    "Mythic.Combat.HealthBandTags",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicHealthBandTagTest::RunTest(const FString &Parameters) {
    if (!TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }

    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    if (!TestNotNull(TEXT("combat settings exist"), Settings)) {
        return false;
    }
    // Everything below reads the shipped band set, so a project that cleared it would pass vacuously.
    if (!TestTrue(TEXT("bands ship configured"), Settings->HealthBands.IsConfigured())) {
        return false;
    }

    UGameInstance *GameInstance = NewObject<UGameInstance>(GEngine);
    GameInstance->InitializeStandalone();
    UWorld *World = GameInstance->GetWorld();
    if (!TestNotNull(TEXT("standalone world exists"), World)) {
        return false;
    }
    ON_SCOPE_EXIT {
        GameInstance->Shutdown();
    };

    AActor *Actor = World->SpawnActor<AActor>();
    UMythicAbilitySystemComponent *ASC = NewObject<UMythicAbilitySystemComponent>(Actor);
    ASC->RegisterComponent();
    ASC->InitAbilityActorInfo(Actor, Actor);
    ASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Life>(Actor));
    ASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Defense>(Actor));

    ASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetMaxHealthAttribute(), 100.0f);
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(), 100.0f);

    UMythicLifeComponent *Life = NewObject<UMythicLifeComponent>(Actor);
    Life->RegisterComponent();
    Life->InitializeWithAbilitySystem(ASC);

    TestTrue(TEXT("a full-health actor advertises unhurt"), ASC->HasMatchingGameplayTag(GAS_STATE_HEALTH_UNHURT));
    TestFalse(TEXT("and nothing worse"), ASC->HasMatchingGameplayTag(GAS_STATE_HEALTH_LOW));

    ASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(), 30.0f);
    TestTrue(TEXT("a hurt actor advertises low"), ASC->HasMatchingGameplayTag(GAS_STATE_HEALTH_LOW));
    TestFalse(TEXT("30% is not critical"), ASC->HasMatchingGameplayTag(GAS_STATE_HEALTH_CRITICAL));
    TestFalse(TEXT("and is no longer unhurt"), ASC->HasMatchingGameplayTag(GAS_STATE_HEALTH_UNHURT));

    ASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(), 10.0f);
    TestTrue(TEXT("a dying actor advertises critical"), ASC->HasMatchingGameplayTag(GAS_STATE_HEALTH_CRITICAL));

    // Healing has to retract the band, or an execute talent keeps firing on a target that recovered.
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(), 100.0f);
    TestFalse(TEXT("healing clears critical"), ASC->HasMatchingGameplayTag(GAS_STATE_HEALTH_CRITICAL));
    TestFalse(TEXT("healing clears low"), ASC->HasMatchingGameplayTag(GAS_STATE_HEALTH_LOW));
    TestTrue(TEXT("healing restores unhurt"), ASC->HasMatchingGameplayTag(GAS_STATE_HEALTH_UNHURT));

    // A raised maximum makes the same health a smaller fraction, so the bands must follow MaxHealth too.
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetMaxHealthAttribute(), 1000.0f);
    TestTrue(TEXT("a raised maximum drops the actor into critical"), ASC->HasMatchingGameplayTag(GAS_STATE_HEALTH_CRITICAL));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHealthBandGatesModifierTest,
    "Mythic.Combat.HealthBandGatesModifier",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

/**
 * The point of publishing band tags is that a gameplay effect can gate a modifier on one, so "hits harder when the
 * target is nearly dead" is authored rather than coded. This drives that end to end: the band tag must reach the
 * spec's captured target tags and qualify a modifier that only counts against a dying target.
 */
bool FMythicHealthBandGatesModifierTest::RunTest(const FString &Parameters) {
    if (!TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }

    UGameInstance *GameInstance = NewObject<UGameInstance>(GEngine);
    GameInstance->InitializeStandalone();
    UWorld *World = GameInstance->GetWorld();
    if (!TestNotNull(TEXT("standalone world exists"), World)) {
        return false;
    }
    ON_SCOPE_EXIT {
        GameInstance->Shutdown();
    };

    AActor *AttackerActor = World->SpawnActor<AActor>();
    UMythicAbilitySystemComponent *Attacker = NewObject<UMythicAbilitySystemComponent>(AttackerActor);
    Attacker->RegisterComponent();
    Attacker->InitAbilityActorInfo(AttackerActor, AttackerActor);
    Attacker->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Offense>(AttackerActor));

    AActor *VictimActor = World->SpawnActor<AActor>();
    UMythicAbilitySystemComponent *Victim = NewObject<UMythicAbilitySystemComponent>(VictimActor);
    Victim->RegisterComponent();
    Victim->InitAbilityActorInfo(VictimActor, VictimActor);
    Victim->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Life>(VictimActor));
    Victim->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Defense>(VictimActor));
    Victim->SetNumericAttributeBase(UMythicAttributeSet_Life::GetMaxHealthAttribute(), 1000.0f);
    Victim->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(), 1000.0f);

    UMythicLifeComponent *Life = NewObject<UMythicLifeComponent>(VictimActor);
    Life->RegisterComponent();
    Life->InitializeWithAbilitySystem(Victim);

    // The talent: +0.75 outgoing damage, but only against something already in its last fifth.
    UGameplayEffect *Talent = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("Test_ExecuteTalent")));
    Talent->DurationPolicy = EGameplayEffectDurationType::Infinite;
    FGameplayModifierInfo TalentMod;
    TalentMod.Attribute = UMythicAttributeSet_Offense::GetOutgoingDamageMultiplierAttribute();
    TalentMod.ModifierOp = EGameplayModOp::Additive;
    TalentMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.75f));
    TalentMod.TargetTags.RequireTags.AddTag(GAS_STATE_HEALTH_CRITICAL);
    Talent->Modifiers.Add(TalentMod);
    Attacker->ApplyGameplayEffectToSelf(Talent, 1.0f, Attacker->MakeEffectContext());

    // A hit whose size is exactly the attacker's outgoing multiplier, so health lost reads the multiplier back.
    UGameplayEffect *Hit = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("Test_Hit")));
    Hit->DurationPolicy = EGameplayEffectDurationType::Instant;
    FGameplayModifierInfo HitMod;
    HitMod.Attribute = UMythicAttributeSet_Life::GetDamageAttribute();
    HitMod.ModifierOp = EGameplayModOp::Additive;
    FAttributeBasedFloat Scaled;
    Scaled.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
        UMythicAttributeSet_Offense::GetOutgoingDamageMultiplierAttribute(), EGameplayEffectAttributeCaptureSource::Source, false);
    Scaled.AttributeCalculationType = EAttributeBasedFloatCalculationType::AttributeMagnitude;
    Scaled.Coefficient = FScalableFloat(10.0f);
    HitMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(Scaled);
    Hit->Modifiers.Add(HitMod);

    const auto Strike = [&]() -> float {
        const float Before = Victim->GetNumericAttribute(UMythicAttributeSet_Life::GetHealthAttribute());
        FGameplayEffectSpec Spec(Hit, Attacker->MakeEffectContext(), 1.0f);
        Attacker->ApplyGameplayEffectSpecToTarget(Spec, Victim);
        return Before - Victim->GetNumericAttribute(UMythicAttributeSet_Life::GetHealthAttribute());
    };

    TestEqual(TEXT("a healthy target takes the unmodified hit"), Strike(), 10.0f);

    Victim->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(), 100.0f);
    if (!TestTrue(TEXT("the victim is now critical"), Victim->HasMatchingGameplayTag(GAS_STATE_HEALTH_CRITICAL))) {
        return false;
    }
    TestEqual(TEXT("a dying target takes the gated bonus"), Strike(), 17.5f);

    // And the gate has to close again, or the bonus is unconditional with extra steps.
    Victim->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(), 1000.0f);
    TestEqual(TEXT("healing out of the band retracts the bonus"), Strike(), 10.0f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDuplicateLifeComponentInitializationTest,
    "Mythic.Combat.LifeComponent.RejectsDuplicateInitialization",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicDuplicateLifeComponentInitializationTest::RunTest(
    const FString &Parameters) {
    if (!TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }

    UGameInstance *GameInstance = NewObject<UGameInstance>(GEngine);
    GameInstance->InitializeStandalone();
    UWorld *World = GameInstance->GetWorld();
    if (!TestNotNull(TEXT("standalone world exists"), World)) {
        return false;
    }
    ON_SCOPE_EXIT {
        GameInstance->Shutdown();
    };

    AActor *Actor = World->SpawnActor<AActor>();
    UMythicAbilitySystemComponent *ASC =
        NewObject<UMythicAbilitySystemComponent>(Actor);
    ASC->RegisterComponent();
    ASC->InitAbilityActorInfo(Actor, Actor);
    ASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Life>(Actor));
    ASC->AddAttributeSetSubobject(
        NewObject<UMythicAttributeSet_Defense>(Actor));

    UMythicLifeComponent *Canonical =
        NewObject<UMythicLifeComponent>(Actor, TEXT("CanonicalLife"));
    Canonical->CreationMethod = EComponentCreationMethod::Native;
    Canonical->RegisterComponent();

    UMythicLifeComponent *Duplicate =
        NewObject<UMythicLifeComponent>(Actor, TEXT("BlueprintDuplicateLife"));
    Duplicate->CreationMethod =
        EComponentCreationMethod::SimpleConstructionScript;
    Duplicate->RegisterComponent();

    AddExpectedError(TEXT("rejected duplicate life component"),
                     EAutomationExpectedErrorFlags::Contains, 1);
    Duplicate->InitializeWithAbilitySystem(ASC);
    TestFalse(TEXT("a Blueprint duplicate cannot bind the ASC"),
              Duplicate->IsInitialized());

    Canonical->InitializeWithAbilitySystem(ASC);
    TestTrue(TEXT("the native canonical component owns the ASC"),
             Canonical->IsInitialized());
    Canonical->UninitializeFromAbilitySystem();

    return true;
}
