#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/ScopeExit.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Proficiencies.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Survival.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "GAS/MythicAbilitySystemComponent.h"

#include <limits>

namespace {

struct FMaxInvariantCase {
    const TCHAR *Name;
    FGameplayAttribute Current;
    FGameplayAttribute Maximum;
    float MaximumFloor;
    bool bPreserveBaseOverflow;
};

bool MaxInvariantNearlyEqual(const float Actual, const float Expected) {
    return FMath::IsFinite(Actual)
        && FMath::IsNearlyEqual(Actual, Expected, KINDA_SMALL_NUMBER);
}

} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicMaxAttributeInvariantTest,
    "Mythic.Combat.Attributes.MaximumInvariant",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicMaxAttributeInvariantTest::RunTest(
    const FString &Parameters) {
    if (!TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }

    UGameInstance *GameInstance = NewObject<UGameInstance>(GEngine);
    GameInstance->InitializeStandalone();
    ON_SCOPE_EXIT { GameInstance->Shutdown(); };

    UWorld *World = GameInstance->GetWorld();
    if (!TestNotNull(TEXT("standalone world exists"), World)) {
        return false;
    }

    AActor *Owner = World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("authoritative attribute owner spawned"), Owner)) {
        return false;
    }

    UMythicAbilitySystemComponent *AbilitySystem =
        NewObject<UMythicAbilitySystemComponent>(Owner);
    if (!TestNotNull(TEXT("ability system was created"), AbilitySystem)) {
        return false;
    }
    AbilitySystem->RegisterComponent();
    AbilitySystem->AddAttributeSetSubobject(
        NewObject<UMythicAttributeSet_Life>(Owner));
    AbilitySystem->AddAttributeSetSubobject(
        NewObject<UMythicAttributeSet_Defense>(Owner));
    AbilitySystem->AddAttributeSetSubobject(
        NewObject<UMythicAttributeSet_Utility>(Owner));
    AbilitySystem->AddAttributeSetSubobject(
        NewObject<UMythicAttributeSet_Survival>(Owner));
    AbilitySystem->AddAttributeSetSubobject(
        NewObject<UMythicAttributeSet_Proficiencies>(Owner));
    AbilitySystem->InitAbilityActorInfo(Owner, Owner);

    const TArray<FMaxInvariantCase> Cases = {
        {TEXT("Health"),
         UMythicAttributeSet_Life::GetHealthAttribute(),
         UMythicAttributeSet_Life::GetMaxHealthAttribute(), 1.0f, false},
        {TEXT("Shield"),
         UMythicAttributeSet_Defense::GetShieldAttribute(),
         UMythicAttributeSet_Defense::GetMaxShieldAttribute(), 0.0f, false},
        {TEXT("Stamina"),
         UMythicAttributeSet_Utility::GetCurrentStaminaAttribute(),
         UMythicAttributeSet_Utility::GetMaxStaminaAttribute(), 0.0f, false},
        {TEXT("CooldownReduction"),
         UMythicAttributeSet_Utility::GetCooldownReductionAttribute(),
         UMythicAttributeSet_Utility::GetMaxCooldownReductionAttribute(),
         0.0f, true},
        {TEXT("Nourishment"),
         UMythicAttributeSet_Survival::GetNourishmentAttribute(),
         UMythicAttributeSet_Survival::GetMaxNourishmentAttribute(), 0.0f,
         false},
        {TEXT("Hydration"),
         UMythicAttributeSet_Survival::GetHydrationAttribute(),
         UMythicAttributeSet_Survival::GetMaxHydrationAttribute(), 0.0f,
         false},
        {TEXT("Warmth"),
         UMythicAttributeSet_Survival::GetWarmthAttribute(),
         UMythicAttributeSet_Survival::GetMaxWarmthAttribute(), 0.0f, false},
        {TEXT("Wetness"),
         UMythicAttributeSet_Survival::GetWetnessAttribute(),
         UMythicAttributeSet_Survival::GetMaxWetnessAttribute(), 0.0f, false},
        {TEXT("CombatProficiency"),
         UMythicAttributeSet_Proficiencies::GetCombatProficiencyAttribute(),
         UMythicAttributeSet_Proficiencies::GetCombatProficiencyMaxAttribute(),
         0.0f, false},
        {TEXT("WoodcuttingProficiency"),
         UMythicAttributeSet_Proficiencies::GetWoodcuttingProficiencyAttribute(),
         UMythicAttributeSet_Proficiencies::GetWoodcuttingProficiencyMaxAttribute(),
         0.0f, false},
        {TEXT("MiningProficiency"),
         UMythicAttributeSet_Proficiencies::GetMiningProficiencyAttribute(),
         UMythicAttributeSet_Proficiencies::GetMiningProficiencyMaxAttribute(),
         0.0f, false},
        {TEXT("ConstructionProficiency"),
         UMythicAttributeSet_Proficiencies::GetConstructionProficiencyAttribute(),
         UMythicAttributeSet_Proficiencies::GetConstructionProficiencyMaxAttribute(),
         0.0f, false},
        {TEXT("TradingProficiency"),
         UMythicAttributeSet_Proficiencies::GetTradingProficiencyAttribute(),
         UMythicAttributeSet_Proficiencies::GetTradingProficiencyMaxAttribute(),
         0.0f, false},
        {TEXT("HuntingProficiency"),
         UMythicAttributeSet_Proficiencies::GetHuntingProficiencyAttribute(),
         UMythicAttributeSet_Proficiencies::GetHuntingProficiencyMaxAttribute(),
         0.0f, false},
        {TEXT("FishingProficiency"),
         UMythicAttributeSet_Proficiencies::GetFishingProficiencyAttribute(),
         UMythicAttributeSet_Proficiencies::GetFishingProficiencyMaxAttribute(),
         0.0f, false},
        {TEXT("FarmingProficiency"),
         UMythicAttributeSet_Proficiencies::GetFarmingProficiencyAttribute(),
         UMythicAttributeSet_Proficiencies::GetFarmingProficiencyMaxAttribute(),
         0.0f, false},
        {TEXT("HarvestingProficiency"),
         UMythicAttributeSet_Proficiencies::GetHarvestingProficiencyAttribute(),
         UMythicAttributeSet_Proficiencies::GetHarvestingProficiencyMaxAttribute(),
         0.0f, false},
        {TEXT("CraftingProficiency"),
         UMythicAttributeSet_Proficiencies::GetCraftingProficiencyAttribute(),
         UMythicAttributeSet_Proficiencies::GetCraftingProficiencyMaxAttribute(),
         0.0f, false},
        {TEXT("AlchemyProficiency"),
         UMythicAttributeSet_Proficiencies::GetAlchemyProficiencyAttribute(),
         UMythicAttributeSet_Proficiencies::GetAlchemyProficiencyMaxAttribute(),
         0.0f, false},
        {TEXT("CookingProficiency"),
         UMythicAttributeSet_Proficiencies::GetCookingProficiencyAttribute(),
         UMythicAttributeSet_Proficiencies::GetCookingProficiencyMaxAttribute(),
         0.0f, false},
        {TEXT("OverallXp"),
         UMythicAttributeSet_Proficiencies::GetOverallXpAttribute(),
         UMythicAttributeSet_Proficiencies::GetOverallXpMaxAttribute(), 0.0f,
         false},
    };

    TestEqual(TEXT("the typed invariant roster covers every current/max pair"),
              Cases.Num(), 21);

    for (const FMaxInvariantCase &Entry : Cases) {
        const float InitialMaximum =
            Entry.bPreserveBaseOverflow ? 0.80f : 100.0f;
        const float InitialCurrent =
            Entry.bPreserveBaseOverflow ? 0.75f : 80.0f;
        const float LowerMaximum =
            Entry.bPreserveBaseOverflow ? 0.50f : 50.0f;
        const float RaisedMaximum =
            Entry.bPreserveBaseOverflow ? 0.80f : 120.0f;

        AbilitySystem->SetNumericAttributeBase(
            Entry.Maximum, InitialMaximum);
        AbilitySystem->SetNumericAttributeBase(
            Entry.Current, InitialCurrent);
        TestTrue(
            *FString::Printf(TEXT("%s accepts a current value below its max"),
                             Entry.Name),
            MaxInvariantNearlyEqual(
                AbilitySystem->GetNumericAttribute(Entry.Current),
                InitialCurrent));

        AbilitySystem->SetNumericAttributeBase(
            Entry.Maximum, LowerMaximum);
        TestTrue(
            *FString::Printf(
                TEXT("lowering %s max immediately clips its current value"),
                Entry.Name),
            MaxInvariantNearlyEqual(
                AbilitySystem->GetNumericAttribute(Entry.Current),
                LowerMaximum));

        const float ExpectedBaseAfterClip =
            Entry.bPreserveBaseOverflow ? InitialCurrent : LowerMaximum;
        TestTrue(
            *FString::Printf(TEXT("%s applies its authored overflow policy"),
                             Entry.Name),
            MaxInvariantNearlyEqual(
                AbilitySystem->GetNumericAttributeBase(Entry.Current),
                ExpectedBaseAfterClip));

        AbilitySystem->SetNumericAttributeBase(
            Entry.Maximum, RaisedMaximum);
        const float ExpectedAfterRaise =
            Entry.bPreserveBaseOverflow ? InitialCurrent : LowerMaximum;
        const FString RaiseExpectation = Entry.bPreserveBaseOverflow
            ? FString::Printf(
                  TEXT("raising %s max reveals preserved stat investment"),
                  Entry.Name)
            : FString::Printf(
                  TEXT("raising %s max does not refill its current value"),
                  Entry.Name);
        TestTrue(
            *RaiseExpectation,
            MaxInvariantNearlyEqual(
                AbilitySystem->GetNumericAttribute(Entry.Current),
                ExpectedAfterRaise));
    }

    // Floors are part of the same contract. Health retains a one-point valid
    // capacity; every other gauge, stat cap, and progression capacity may be
    // disabled at zero.
    for (const FMaxInvariantCase &Entry : Cases) {
        AbilitySystem->SetNumericAttributeBase(
            Entry.Maximum,
            Entry.bPreserveBaseOverflow ? 0.80f : 100.0f);
        AbilitySystem->SetNumericAttributeBase(
            Entry.Current,
            Entry.bPreserveBaseOverflow ? 0.25f : 25.0f);
        AbilitySystem->SetNumericAttributeBase(Entry.Maximum, -10.0f);
        TestTrue(
            *FString::Printf(TEXT("%s max respects its authored floor"),
                             Entry.Name),
            MaxInvariantNearlyEqual(
                AbilitySystem->GetNumericAttribute(Entry.Maximum),
                Entry.MaximumFloor));
        TestTrue(
            *FString::Printf(
                TEXT("%s does not retain an invalid maximum base"),
                Entry.Name),
            MaxInvariantNearlyEqual(
                AbilitySystem->GetNumericAttributeBase(Entry.Maximum),
                Entry.MaximumFloor));
        TestTrue(
            *FString::Printf(
                TEXT("%s current follows a max sanitized to its floor"),
                Entry.Name),
            MaxInvariantNearlyEqual(
                AbilitySystem->GetNumericAttribute(Entry.Current),
                Entry.MaximumFloor));
    }

    const float NaN = std::numeric_limits<float>::quiet_NaN();
    const float Infinity = std::numeric_limits<float>::infinity();

    const FMaxInvariantCase &Health = Cases[0];
    AbilitySystem->SetNumericAttributeBase(Health.Maximum, NaN);
    TestTrue(TEXT("a NaN health max sanitizes to one"),
             MaxInvariantNearlyEqual(
                 AbilitySystem->GetNumericAttribute(Health.Maximum), 1.0f));
    TestTrue(TEXT("a NaN health max cannot poison its stored base"),
             MaxInvariantNearlyEqual(
                 AbilitySystem->GetNumericAttributeBase(Health.Maximum),
                 1.0f));

    const FMaxInvariantCase &Shield = Cases[1];
    AbilitySystem->SetNumericAttributeBase(Shield.Maximum, Infinity);
    TestTrue(TEXT("an infinite non-health max sanitizes to zero"),
             MaxInvariantNearlyEqual(
                 AbilitySystem->GetNumericAttribute(Shield.Maximum), 0.0f));
    TestTrue(TEXT("an infinite non-health max cannot poison its stored base"),
             MaxInvariantNearlyEqual(
                 AbilitySystem->GetNumericAttributeBase(Shield.Maximum),
                 0.0f));

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
