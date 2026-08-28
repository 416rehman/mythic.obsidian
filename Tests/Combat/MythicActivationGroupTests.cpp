// Copyright Stellar Games. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GAS/Abilities/MythicGameplayAbility.h"
#include "GAS/MythicAbilitySystemComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicActivationGroupLifecycleTest,
    "Mythic.Combat.AbilityActivationGroups.Lifecycle",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicActivationGroupLifecycleTest::RunTest(const FString &Parameters) {
    UMythicAbilitySystemComponent *AbilitySystem = NewObject<UMythicAbilitySystemComponent>();
    UMythicGameplayAbility *Ability = NewObject<UMythicGameplayAbility>(AbilitySystem);
    if (!TestNotNull(TEXT("a Mythic ability can be created for lifecycle notification coverage"), Ability)) {
        return false;
    }

    const uint8 IndependentIndex = static_cast<uint8>(EMythicAbilityActivationGroup::Independent);
    const uint8 ReplaceableIndex = static_cast<uint8>(EMythicAbilityActivationGroup::Exclusive_Replaceable);
    const uint8 BlockingIndex = static_cast<uint8>(EMythicAbilityActivationGroup::Exclusive_Blocking);

    TestEqual(TEXT("independent activation groups start empty"),
              AbilitySystem->ActivationGroupCounts[IndependentIndex], 0);
    TestEqual(TEXT("replaceable activation groups start empty"),
              AbilitySystem->ActivationGroupCounts[ReplaceableIndex], 0);
    TestEqual(TEXT("blocking activation groups start empty"),
              AbilitySystem->ActivationGroupCounts[BlockingIndex], 0);

    bool bActivationDelegateObservedRegistration = false;
    AbilitySystem->AbilityActivatedCallbacks.AddLambda(
        [AbilitySystem, IndependentIndex, &bActivationDelegateObservedRegistration](UGameplayAbility *) {
            bActivationDelegateObservedRegistration =
                AbilitySystem->ActivationGroupCounts[IndependentIndex] == 1;
        });

    const FGameplayAbilitySpecHandle Handle;
    AbilitySystem->NotifyAbilityActivated(Handle, Ability);
    TestTrue(TEXT("activation callbacks observe the ability already registered"),
             bActivationDelegateObservedRegistration);
    TestEqual(TEXT("activation registers exactly one independent ability"),
              AbilitySystem->ActivationGroupCounts[IndependentIndex], 1);

    AbilitySystem->NotifyAbilityEnded(Handle, Ability, false);
    TestEqual(TEXT("ending unregisters the independent ability"),
              AbilitySystem->ActivationGroupCounts[IndependentIndex], 0);
    TestEqual(TEXT("the lifecycle does not alter replaceable groups"),
              AbilitySystem->ActivationGroupCounts[ReplaceableIndex], 0);
    TestEqual(TEXT("the lifecycle does not alter blocking groups"),
              AbilitySystem->ActivationGroupCounts[BlockingIndex], 0);

    return true;
}

#endif
