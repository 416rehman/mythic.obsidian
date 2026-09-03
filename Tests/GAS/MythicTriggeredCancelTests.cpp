
#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

#include "GAS/Abilities/MythicGA_Triggered.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicTags_GAS.h"
#include "Tests/Combat/MythicTestEffects.h"

namespace {

bool IsTriggeredSpecActive(const UAbilitySystemComponent *ASC, const FGameplayAbilitySpecHandle Handle) {
    const FGameplayAbilitySpec *Spec = ASC->FindAbilitySpecFromHandle(Handle);
    return Spec && Spec->IsActive();
}

bool IsTriggeredEventBound(const UAbilitySystemComponent *ASC, const FGameplayTag Event) {
    const FGameplayEventMulticastDelegate *Delegate = ASC->GenericGameplayEventCallbacks.Find(Event);
    return Delegate && Delegate->IsBound();
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTriggeredSurvivesCancelAllTest,
    "Mythic.GAS.Triggered.SurvivesCancelAll",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicTriggeredSurvivesCancelAllTest::RunTest(const FString &Parameters) {
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

    AActor *Actor = World->SpawnActor<AActor>();
    UMythicAbilitySystemComponent *ASC = NewObject<UMythicAbilitySystemComponent>(Actor);
    ASC->RegisterComponent();
    ASC->InitAbilityActorInfo(Actor, Actor);
    ASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Offense>(Actor));
    if (!TestTrue(TEXT("the ability system answers as authority"), ASC->IsOwnerActorAuthoritative())) {
        return false;
    }

    // The control keeps the engine's default cancelability. It has to fall, or the sweep never ran and the
    // trigger ability surviving proves nothing.
    FGameplayAbilitySpec ControlSpec(UMythicTestPassiveAbility::StaticClass(), 1, INDEX_NONE, Actor);
    const FGameplayAbilitySpecHandle ControlHandle = ASC->GiveAbility(ControlSpec);
    FGameplayAbilitySpec TriggeredSpec(UMythicTestTriggeredAbility::StaticClass(), 1, INDEX_NONE, Actor);
    const FGameplayAbilitySpecHandle TriggeredHandle = ASC->GiveAbility(TriggeredSpec);
    if (!TestTrue(TEXT("both abilities were granted"), ControlHandle.IsValid() && TriggeredHandle.IsValid())) {
        return false;
    }
    ASC->TryActivateAbility(ControlHandle);
    ASC->TryActivateAbility(TriggeredHandle);

    if (!TestTrue(TEXT("the control passive is active before the cancel"), IsTriggeredSpecActive(ASC, ControlHandle))) {
        return false;
    }
    if (!TestTrue(TEXT("the trigger ability is active before the cancel"), IsTriggeredSpecActive(ASC, TriggeredHandle))) {
        return false;
    }
    if (!TestTrue(TEXT("its event is bound before the cancel"), IsTriggeredEventBound(ASC, GAS_EVENT_KILL))) {
        return false;
    }

    // What EnterDownedState and StartDeath both do.
    ASC->CancelAllAbilities();

    TestFalse(TEXT("CancelAllAbilities ended the control passive, so the sweep really ran"),
              IsTriggeredSpecActive(ASC, ControlHandle));
    TestTrue(TEXT("the trigger ability is still active after CancelAllAbilities"), IsTriggeredSpecActive(ASC, TriggeredHandle));
    TestTrue(TEXT("its event callback is still bound after CancelAllAbilities"), IsTriggeredEventBound(ASC, GAS_EVENT_KILL));

    // Refusing cancellation must not refuse removal, or a rune could never leave its socket.
    ASC->ClearAbility(TriggeredHandle);
    TestNull(TEXT("clearing the ability still removes it"), ASC->FindAbilitySpecFromHandle(TriggeredHandle));
    TestFalse(TEXT("and unbinds its event"), IsTriggeredEventBound(ASC, GAS_EVENT_KILL));

    AddInfo(FString::Printf(TEXT("abilities granted: 2, cancelled by the sweep: %d, survived: %d"),
                            IsTriggeredSpecActive(ASC, ControlHandle) ? 0 : 1, IsTriggeredSpecActive(ASC, TriggeredHandle) ? 1 : 0));
    return true;
}
