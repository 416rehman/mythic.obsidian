
#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/Effects/MythicStatusRegistry.h"
#include "GAS/Executions/MythicCombatRoll.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicStatDiminishing.h"
#include "Settings/MythicCombatSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatDiminishingTest,
    "Mythic.Combat.StatDiminishing",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatDiminishingTest::RunTest(const FString &Parameters) {
    using MythicCombat::DiminishMultiplier;

    // A stat is 1.0-based, so a character with no gear must come out exactly unscaled. Bending the whole value
    // instead of the bonus would quietly nerf everyone wearing nothing.
    TestEqual(TEXT("no bonus is untouched"), DiminishMultiplier(1.0f, 1.0f, 4.0f), 1.0f);
    TestEqual(TEXT("a bonus under the soft cap is face value"), DiminishMultiplier(1.5f, 1.0f, 4.0f), 1.5f);
    TestEqual(TEXT("a bonus exactly at the soft cap is face value"), DiminishMultiplier(2.0f, 1.0f, 4.0f), 2.0f);

    const float Stacked = DiminishMultiplier(3.0f, 1.0f, 4.0f);
    TestTrue(TEXT("past the soft cap the bonus is bent"), Stacked < 3.0f);
    TestTrue(TEXT("but still beats the soft cap"), Stacked > 2.0f);
    TestTrue(TEXT("and never reaches the ceiling"), DiminishMultiplier(1000.0f, 1.0f, 4.0f) < 5.0f);

    // Stacking must always pay, or the top of the loot curve stops mattering.
    TestTrue(TEXT("more gear is always more"), DiminishMultiplier(4.0f, 1.0f, 4.0f) > DiminishMultiplier(3.0f, 1.0f, 4.0f));

    // A stat with no authored curve must pass through, so adding a stat never silently caps it.
    TestEqual(TEXT("no ceiling authored means no curve"), DiminishMultiplier(9.0f, 1.0f, 0.0f), 9.0f);

    // A negative stat would heal the target with a debuff.
    TestEqual(TEXT("below zero floors at no bonus"), DiminishMultiplier(-2.0f, 1.0f, 4.0f), 1.0f);

    FMythicStatDiminishingConfig Config;
    FMythicStatDiminishing Row;
    Row.Attribute = UMythicAttributeSet_Offense::GetBurnDamageMultiplierAttribute();
    Row.SoftCapBonus = 0.5f;
    Row.CeilingBonus = 2.0f;
    Config.Stats.Add(Row);
    Config.DefaultCeilingBonus = 0.0f;

    float Soft = -1.0f, Ceiling = -1.0f;
    FMythicStatDiminishingRules::FindCurve(Config, UMythicAttributeSet_Offense::GetBurnDamageMultiplierAttribute(), Soft, Ceiling);
    TestEqual(TEXT("a named stat finds its own curve"), Soft, 0.5f);
    TestEqual(TEXT("and its own ceiling"), Ceiling, 2.0f);

    FMythicStatDiminishingRules::FindCurve(Config, UMythicAttributeSet_Offense::GetPoisonDamageMultiplierAttribute(), Soft, Ceiling);
    TestEqual(TEXT("an unnamed stat falls back to the default"), Ceiling, 0.0f);
    TestEqual(TEXT("so an unnamed stat is uncurved"),
              FMythicStatDiminishingRules::Apply(Config, UMythicAttributeSet_Offense::GetPoisonDamageMultiplierAttribute(), 6.0f), 6.0f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatusApplierScaleTest,
    "Mythic.Combat.StatusApplierScale",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatusApplierScaleTest::RunTest(const FString &Parameters) {
    if (!TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }

    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    // Everything below reads the shipped curves, so an empty list would pass for the wrong reason.
    if (!TestTrue(TEXT("status curves ship authored"), Settings && Settings->StatDiminishing.Stats.Num() > 0)) {
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

    const FGameplayAttribute BurnDamage = UMythicAttributeSet_Offense::GetBurnDamageMultiplierAttribute();

    // An applier with no ability system at all - a trap, a hazard, a script. It must inflict the authored band,
    // not nothing, or every environmental status in the game silently deals zero.
    AActor *Bare = World->SpawnActor<AActor>();
    TestEqual(TEXT("an applier with no ability system scales by one"),
              UMythicStatusRegistry::ResolveApplierScale(Bare, BurnDamage), 1.0f);
    TestEqual(TEXT("no applier at all scales by one"),
              UMythicStatusRegistry::ResolveApplierScale(nullptr, BurnDamage), 1.0f);

    AActor *Applier = World->SpawnActor<AActor>();
    UMythicAbilitySystemComponent *ASC = NewObject<UMythicAbilitySystemComponent>(Applier);
    ASC->RegisterComponent();
    ASC->InitAbilityActorInfo(Applier, Applier);
    ASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Offense>(Applier));

    // A status that names no stat is authored-band-only, whatever the applier is wearing.
    TestEqual(TEXT("a status naming no stat scales by one"),
              UMythicStatusRegistry::ResolveApplierScale(Applier, FGameplayAttribute()), 1.0f);

    TestEqual(TEXT("an applier with default gear scales by one"),
              UMythicStatusRegistry::ResolveApplierScale(Applier, BurnDamage), 1.0f);

    // +100% is exactly the shipped soft cap for damage over time, so it should arrive intact.
    ASC->SetNumericAttributeBase(BurnDamage, 2.0f);
    TestEqual(TEXT("a bonus at the soft cap arrives whole"),
              UMythicStatusRegistry::ResolveApplierScale(Applier, BurnDamage), 2.0f);

    // Past it, gear still pays but not at face value.
    ASC->SetNumericAttributeBase(BurnDamage, 3.5f);
    const float Deep = UMythicStatusRegistry::ResolveApplierScale(Applier, BurnDamage);
    TestTrue(TEXT("a deep stack is bent down"), Deep < 3.5f);
    TestTrue(TEXT("but still beats the soft cap"), Deep > 2.0f);

    // The attribute set clamps at zero, so cursed gear cannot invert a debuff into a benefit.
    ASC->SetNumericAttributeBase(BurnDamage, -5.0f);
    TestTrue(TEXT("a negative stat cannot heal the target"),
             UMythicStatusRegistry::ResolveApplierScale(Applier, BurnDamage) >= 0.0f);

    // Hard crowd control is on a far tighter curve than damage, deliberately.
    const FGameplayAttribute StunDuration = UMythicAttributeSet_Offense::GetStunDurationMultiplierAttribute();
    ASC->SetNumericAttributeBase(StunDuration, 5.0f);
    ASC->SetNumericAttributeBase(BurnDamage, 5.0f);
    TestTrue(TEXT("stun duration is capped harder than burn damage"),
             UMythicStatusRegistry::ResolveApplierScale(Applier, StunDuration)
             < UMythicStatusRegistry::ResolveApplierScale(Applier, BurnDamage));
    TestTrue(TEXT("and cannot reach a stun the player cannot act out of"),
             UMythicStatusRegistry::ResolveApplierScale(Applier, StunDuration) < 1.5f);

    return true;
}
