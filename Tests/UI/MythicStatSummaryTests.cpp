// Copyright Stellar Games. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GAS/Abilities/MythicWeaponAttackAbility.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicStatContribution.h"
#include "GAS/MythicStatSummary.h"
#include "Itemization/Inventory/Fragments/Actionable/AttackFragment.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/MythicTags_Inventory.h"
#include "Itemization/Storage/MythicStorageContainer.h"
#include "Misc/ScopeExit.h"
#include "Settings/MythicCombatSettings.h"
#include "Tests/UI/MythicStatSummaryTestTypes.h"
#include "UI/ViewModels/MythicStatDisplay.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatSummaryComputeTest,
    "Mythic.UI.StatSummaryCompute",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatSummaryComputeTest::RunTest(const FString &Parameters) {
    UMythicStatSummaryDefinition *Definition = NewObject<UMythicStatSummaryDefinition>(GetTransientPackage());

    // A half-authored row reads as a plain zero, never a crash or a stale number.
    TestEqual(TEXT("no calculation class computes zero"), Definition->Compute(nullptr), 0.0f);

    Definition->CalculationClass = UMythicStatSummaryCalculation_Fixed::StaticClass();
    TestEqual(TEXT("compute runs the authored calculation"), Definition->Compute(nullptr), 1234.5f);

    return true;
}

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicBasicAttackDamageSummaryTest,
    "Mythic.UI.BasicAttackDamageSummary",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicBasicAttackDamageSummaryTest::RunTest(const FString &Parameters) {
    UGameInstance *GameInstance = NewObject<UGameInstance>(GEngine);
    GameInstance->InitializeStandalone();
    ON_SCOPE_EXIT {
        GameInstance->Shutdown();
    };
    UWorld *World = GameInstance->GetWorld();
    if (!TestNotNull(TEXT("standalone world exists"), World)) {
        return false;
    }

    AActor *Owner = World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("authoritative fixture owner exists"), Owner)) {
        return false;
    }
    UMythicAbilitySystemComponent *AbilitySystem =
        NewObject<UMythicAbilitySystemComponent>(Owner);
    AbilitySystem->RegisterComponent();
    AbilitySystem->InitAbilityActorInfo(Owner, Owner);
    AbilitySystem->AddAttributeSetSubobject(
        NewObject<UMythicAttributeSet_Offense>(Owner));
    AbilitySystem->SetNumericAttributeBase(
        UMythicAttributeSet_Offense::GetDamagePerHitAttribute(), 4.0f);
    AbilitySystem->SetNumericAttributeBase(
        UMythicAttributeSet_Offense::GetPowerAttribute(), 59.0f);
    AbilitySystem->SetNumericAttributeBase(
        UMythicAttributeSet_Offense::GetBonusAxeDamageAttribute(), 0.73648f);

    AMythicStorageContainer *ItemOwner =
        NewObject<AMythicStorageContainer>(GetTransientPackage());
    UItemDefinition *WeaponDefinition = NewObject<UItemDefinition>(ItemOwner);
    WeaponDefinition->StackSizeMax = 1;
    WeaponDefinition->ItemType = ITEMIZATION_TYPE_EQUIPMENT_WEAPON_AXE;
    UMythicItemInstance *Weapon = NewObject<UMythicItemInstance>(ItemOwner);
    Weapon->SetOwner(ItemOwner);
    Weapon->InitializeFixtureForTests(WeaponDefinition, 1, 1);
    UAttackFragment *AttackFragment = NewObject<UAttackFragment>(Weapon);
    AttackFragment->SetOwnerItemInstance(Weapon);
    const FGameplayAbilitySpecHandle WeaponHandle = AbilitySystem->GiveAbility(
        FGameplayAbilitySpec(
            UMythicStatSummaryWeaponAbilityFixture::StaticClass(),
            1,
            INDEX_NONE,
            AttackFragment));
    TestTrue(TEXT("the fixture owns one canonical weapon source"), WeaponHandle.IsValid());

    UMythicStatSummaryDefinition *Definition =
        NewObject<UMythicStatSummaryDefinition>(GetTransientPackage());
    Definition->CalculationClass =
        UMythicBasicAttackDamageSummaryCalculation::StaticClass();

    float Minimum = 0.0f;
    float Maximum = 0.0f;
    TestTrue(TEXT("the native summary supplies a character-effective range"),
             Definition->ComputeRange(AbilitySystem, Minimum, Maximum));
    TestTrue(TEXT("the summary includes Power and the exact axe bonus in its fourteen-to-twenty-one source band"),
             Minimum > 14.0f && Minimum < 14.1f
             && Maximum > 21.0f && Maximum < 21.1f);
    TestTrue(TEXT("the scalar summary remains the expected midpoint for sorting and telemetry"),
             FMath::IsNearlyEqual(
                 Definition->Compute(AbilitySystem),
                 (Minimum + Maximum) * 0.5f,
                 UE_KINDA_SMALL_NUMBER));
    return true;
}
#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatSummaryLibraryAuditTest,
    "Mythic.UI.StatSummaryLibraryAudit",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatSummaryLibraryAuditTest::RunTest(const FString &Parameters) {
    const UMythicStatDisplaySettings *Display = GetDefault<UMythicStatDisplaySettings>();
    if (!Display || Display->SummaryLibrary.IsNull()) {
        AddInfo(TEXT("0 summaries audited: no summary library authored yet - the cards section stays absent."));
        return true;
    }

    const UMythicStatSummaryLibrary *Library = Display->SummaryLibrary.LoadSynchronous();
    if (!Library) {
        AddError(TEXT("a summary library is configured but does not load"));
        return false;
    }

    const FGameplayTag SummaryRoot = FGameplayTag::RequestGameplayTag(TEXT("Stat.Summary"), false);
    int32 Audited = 0;
    for (const UMythicStatSummaryDefinition *Definition : Library->Summaries) {
        if (!Definition) {
            AddError(TEXT("the library holds a null definition row"));
            continue;
        }
        ++Audited;
        const FString Name = Definition->GetName();
        TestTrue(FString::Printf(TEXT("%s: SummaryId sits under Stat.Summary"), *Name),
                 SummaryRoot.IsValid() && Definition->SummaryId.MatchesTag(SummaryRoot));
        TestFalse(FString::Printf(TEXT("%s: label is authored"), *Name), Definition->Label.IsEmpty());
        TestTrue(FString::Printf(TEXT("%s: a calculation is authored"), *Name),
                 Definition->CalculationClass != nullptr);
    }
    AddInfo(FString::Printf(TEXT("%d of %d summaries audited."), Audited, Library->Summaries.Num()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPrimaryAttributeRoundTripTest,
    "Mythic.UI.PrimaryAttributeRoundTrip",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPrimaryAttributeRoundTripTest::RunTest(const FString &Parameters) {
    // The tooltip asks for contributions BY the row's attribute; a primary whose attribute finds no rows
    // renders an empty hover, which reads as a broken feature rather than a missing one.
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    if (!Settings) {
        AddError(TEXT("no combat settings"));
        return false;
    }
    const TArray<FMythicStatContribution> &Rows = Settings->StatContributions.Contributions;

    int32 PowerRows = 0;
    int32 StrengthRows = 0;
    for (const FMythicStatContribution &Row : Rows) {
        if (Row.SourceStat == UMythicAttributeSet_Offense::GetPowerAttribute()) {
            ++PowerRows;
        }
        if (Row.SourceStat == UMythicAttributeSet_Defense::GetStrengthAttribute()) {
            ++StrengthRows;
        }
    }
    AddInfo(FString::Printf(TEXT("%d rows total: %d from Power, %d from Strength."), Rows.Num(), PowerRows, StrengthRows));
    TestTrue(TEXT("Power feeds at least one contribution row"), PowerRows > 0);
    TestTrue(TEXT("Strength feeds at least one contribution row"), StrengthRows > 0);
    return true;
}
