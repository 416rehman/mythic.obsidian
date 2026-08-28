#include "Misc/AutomationTest.h"

#include <limits>

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GAS/Abilities/MythicGameplayAbility.h"
#include "GAS/Effects/MythicWeaponDamageEffects.h"
#include "GAS/Executions/MythicDamageApplication.h"
#include "GAS/Executions/MythicDamageCalculation.h"
#include "GAS/Feedback/MythicTags_FeedbackCues.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "Itemization/Inventory/Fragments/Actionable/AttackFragment.h"
#include "Misc/ScopeExit.h"

namespace {
bool TestExactExecution(FAutomationTestBase &Test, const UGameplayEffect *Effect,
                        const UClass *ExpectedExecutionClass,
                        const TCHAR *EffectLabel) {
    if (!Test.TestNotNull(
            *FString::Printf(TEXT("%s CDO resolves"), EffectLabel), Effect)) {
        return false;
    }

    Test.TestTrue(
        *FString::Printf(TEXT("%s is instant"), EffectLabel),
        Effect->DurationPolicy == EGameplayEffectDurationType::Instant);
    Test.TestEqual(
        *FString::Printf(TEXT("%s has no drift-prone modifiers"), EffectLabel),
        Effect->Modifiers.Num(), 0);
    if (!Test.TestEqual(
            *FString::Printf(TEXT("%s has exactly one execution"), EffectLabel),
            Effect->Executions.Num(), 1)) {
        return false;
    }

    Test.TestTrue(
        *FString::Printf(TEXT("%s owns the exact native execution"), EffectLabel),
        Effect->Executions[0].CalculationClass.Get() == ExpectedExecutionClass);
    Test.TestTrue(
        *FString::Printf(TEXT("%s execution has no mutable scoped modifiers"), EffectLabel),
        Effect->Executions[0].CalculationModifiers.IsEmpty());
    Test.TestTrue(
        *FString::Printf(TEXT("%s execution has no hidden passed-in tags"), EffectLabel),
        Effect->Executions[0].PassedInTags.IsEmpty());
    Test.TestTrue(
        *FString::Printf(TEXT("%s execution has no conditional child effects"), EffectLabel),
        Effect->Executions[0].ConditionalGameplayEffects.IsEmpty());
    return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWeaponDamageEffectsContractTest,
    "Mythic.Combat.WeaponDamage.NativeEffects",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWeaponDamageEffectsContractTest::RunTest(const FString &Parameters) {
    const UMythicGE_WeaponDamageCalculation *Calculation =
        GetDefault<UMythicGE_WeaponDamageCalculation>();
    const UMythicGE_WeaponDamageApplication *Application =
        GetDefault<UMythicGE_WeaponDamageApplication>();

    const bool bCalculationValid = TestExactExecution(
        *this, Calculation, UMythicDamageCalculation::StaticClass(),
        TEXT("weapon damage calculation"));
    const bool bApplicationValid = TestExactExecution(
        *this, Application, UMythicDamageApplication::StaticClass(),
        TEXT("weapon damage application"));

    if (bCalculationValid) {
        TestTrue(TEXT("calculation stage never emits target hit feedback"),
                 Calculation->GameplayCues.IsEmpty());
    }
    if (bApplicationValid) {
        if (TestEqual(TEXT("application stage has exactly one Gameplay Cue"),
                      Application->GameplayCues.Num(), 1)) {
            TestTrue(TEXT("application stage emits the native Damage.Hit cue"),
                     Application->GameplayCues[0].GameplayCueTags.HasTagExact(
                         TAG_GameplayCue_Damage_Hit));
            TestEqual(TEXT("application cue contains no unrelated feedback tags"),
                      Application->GameplayCues[0].GameplayCueTags.Num(), 1);
        }
    }

    TestTrue(TEXT("the canonical Damage.Hit tag is registered natively"),
             TAG_GameplayCue_Damage_Hit.GetTag().IsValid());

    const FMythicDamageContainer Container =
        MythicWeaponDamage::MakeDamageContainer();
    TestTrue(TEXT("container exposes the exact native calculation class"),
             Container.DamageCalculationEffect.Get()
                 == UMythicGE_WeaponDamageCalculation::StaticClass());
    TestTrue(TEXT("container exposes the exact native application class"),
             Container.DamageApplicationEffect.Get()
                 == UMythicGE_WeaponDamageApplication::StaticClass());
    TestTrue(TEXT("calculation accessor cannot select an authored asset"),
             MythicWeaponDamage::GetCalculationEffectClass().Get()
                 == UMythicGE_WeaponDamageCalculation::StaticClass());
    TestTrue(TEXT("application accessor cannot select an authored asset"),
             MythicWeaponDamage::GetApplicationEffectClass().Get()
                  == UMythicGE_WeaponDamageApplication::StaticClass());

    FGameplayEffectCustomExecutionOutput AbortedOutput;
    TestFalse(TEXT("a fresh execution output permits automatic effect cues"),
              AbortedOutput.AreGameplayCuesHandledManually());
    UMythicDamageApplication::MarkDamageExecutionAborted(AbortedOutput);
    TestTrue(TEXT("an aborted damage execution suppresses automatic Damage.Hit"),
             AbortedOutput.AreGameplayCuesHandledManually());

    FGameplayEffectCustomExecutionOutput ZeroMultiplierOutput;
    const float ZeroMultiplierDamage = 25.0f * 0.0f * 1.0f;
    TestTrue(TEXT("a zero damage multiplier enters the null-damage cue branch"),
             UMythicDamageApplication::HandleResolvedDamageCuePolicy(
                 ZeroMultiplierDamage, ZeroMultiplierOutput));
    TestTrue(TEXT("multiplier immunity suppresses automatic Damage.Hit"),
             ZeroMultiplierOutput.AreGameplayCuesHandledManually());

    FGameplayEffectCustomExecutionOutput PositiveDamageOutput;
    TestFalse(TEXT("positive resolved damage keeps the automatic Damage.Hit cue"),
              UMythicDamageApplication::HandleResolvedDamageCuePolicy(
                  25.0f, PositiveDamageOutput));
    TestFalse(TEXT("positive damage does not mark cues handled manually"),
              PositiveDamageOutput.AreGameplayCuesHandledManually());

    FGameplayEffectCustomExecutionOutput InvalidDamageOutput;
    TestTrue(TEXT("non-finite resolved damage also fails the cue branch closed"),
             UMythicDamageApplication::HandleResolvedDamageCuePolicy(
                 std::numeric_limits<float>::quiet_NaN(), InvalidDamageOutput));
    TestTrue(TEXT("invalid combat math cannot emit automatic Damage.Hit"),
             InvalidDamageOutput.AreGameplayCuesHandledManually());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWeaponDamageSourceProvenanceTest,
    "Mythic.Combat.WeaponDamage.SourceProvenance",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWeaponDamageSourceProvenanceTest::RunTest(
    const FString &Parameters) {
    if (!TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }

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
    UMythicAbilitySystemComponent *ASC =
        NewObject<UMythicAbilitySystemComponent>(Owner);
    ASC->RegisterComponent();
    ASC->InitAbilityActorInfo(Owner, Owner);

    UAttackFragment *ExactSource = NewObject<UAttackFragment>(Owner);
    const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(
        FGameplayAbilitySpec(UMythicGameplayAbility::StaticClass(), 1,
                             INDEX_NONE, ExactSource));
    if (!TestTrue(TEXT("provenance test ability is granted"),
                  Handle.IsValid())) {
        return false;
    }
    if (!TestTrue(TEXT("provenance test ability activates"),
                  ASC->TryActivateAbility(Handle))) {
        return false;
    }
    FGameplayAbilitySpec *Granted = ASC->FindAbilitySpecFromHandle(Handle);
    UMythicGameplayAbility *Ability = Granted
        ? Cast<UMythicGameplayAbility>(Granted->GetPrimaryInstance())
        : nullptr;
    if (!TestNotNull(TEXT("provenance test ability is instanced"), Ability)) {
        return false;
    }

    const FMythicDamageContainerSpec Spec = Ability->MakeDamageContainerSpec(
        MythicWeaponDamage::MakeDamageContainer());
    TestTrue(TEXT("damage context is valid"), Spec.EffectContextHandle.IsValid());
    TestTrue(TEXT("damage context records the exact local ability instance"),
             Spec.EffectContextHandle.GetAbilityInstance_NotReplicated()
                 == Ability);
    TestTrue(TEXT("damage context records the stable ability CDO"),
             Spec.EffectContextHandle.GetAbility()
                 == Ability->GetClass()->GetDefaultObject<UGameplayAbility>());
    TestTrue(TEXT("damage context records the exact granting fragment"),
             Spec.EffectContextHandle.GetSourceObject() == ExactSource);
    TestTrue(TEXT("damage context records the owning actor as instigator"),
             Spec.EffectContextHandle.GetInstigator() == Owner);
    TestTrue(TEXT("damage context records the avatar as effect causer"),
             Spec.EffectContextHandle.GetEffectCauser() == Owner);

    if (TestTrue(TEXT("calculation spec was created"),
                 Spec.DamageCalculationEffectSpec.IsValid())) {
        TestTrue(TEXT("calculation spec preserves the granting fragment"),
                 Spec.DamageCalculationEffectSpec.Data->GetContext()
                         .GetSourceObject()
                     == ExactSource);
    }
    if (TestTrue(TEXT("application spec was created"),
                 Spec.DamageApplicationEffectSpec.IsValid())) {
        TestTrue(TEXT("application spec preserves the granting fragment"),
                 Spec.DamageApplicationEffectSpec.Data->GetContext()
                         .GetSourceObject()
                     == ExactSource);
    }

    ASC->ClearAbility(Handle);
    return true;
}
