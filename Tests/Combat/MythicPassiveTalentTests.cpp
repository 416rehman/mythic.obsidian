
#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "ScalableFloat.h"

#include "GAS/Abilities/MythicGA_Passive.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicTags_GAS.h"
#include "Tests/Combat/MythicTestEffects.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPassiveTalentTest,
    "Mythic.Combat.PassiveTalent",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPassiveTalentTest::RunTest(const FString &Parameters) {
    TestFalse(TEXT("a clause with no effect is inert"), UMythicGA_Passive::HasPayload(FMythicPassiveClause()));
    FMythicPassiveClause Payload;
    Payload.EffectToApply = UMythicTestPassiveEffect::StaticClass();
    TestTrue(TEXT("a clause with an effect is not"), UMythicGA_Passive::HasPayload(Payload));

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
    UMythicAbilitySystemComponent *ASC = NewObject<UMythicAbilitySystemComponent>(Actor);
    ASC->RegisterComponent();
    ASC->InitAbilityActorInfo(Actor, Actor);
    ASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Offense>(Actor));

    const FGameplayAttribute Outgoing = UMythicAttributeSet_Offense::GetOutgoingDamageMultiplierAttribute();
    const float Base = ASC->GetNumericAttribute(Outgoing);

    FGameplayAbilitySpec Spec(UMythicTestPassiveAbility::StaticClass(), 1, INDEX_NONE, Actor);
    const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
    if (!TestTrue(TEXT("the ability was granted"), Handle.IsValid())) {
        return false;
    }
    ASC->TryActivateAbility(Handle);

    // Nothing rolled a value, so the clause's own magnitude is what has to reach the attribute.
    TestEqual(TEXT("a granted passive raises the attribute by its fallback magnitude"),
              ASC->GetNumericAttribute(Outgoing), Base + 0.4f);

    // Unequipping the item that granted it has to take the bonus with it.
    ASC->ClearAbility(Handle);
    TestEqual(TEXT("clearing the ability retracts the bonus"), ASC->GetNumericAttribute(Outgoing), Base);

    return true;
}
