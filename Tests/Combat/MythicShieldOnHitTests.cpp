
#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/Executions/MythicDamageApplication.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "GAS/MythicTags_GAS.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicShieldOnHitTest,
    "Mythic.Combat.ShieldOnHit",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicShieldOnHitTest::RunTest(const FString &Parameters) {
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

    AActor *Source = World->SpawnActor<AActor>();
    UMythicAbilitySystemComponent *SourceASC = NewObject<UMythicAbilitySystemComponent>(Source);
    SourceASC->RegisterComponent();
    SourceASC->InitAbilityActorInfo(Source, Source);
    SourceASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Offense>(Source));
    SourceASC->SetNumericAttributeBase(UMythicAttributeSet_Offense::GetPowerAttribute(), 1.0f);
    SourceASC->SetNumericAttributeBase(UMythicAttributeSet_Offense::GetDamagePerHitAttribute(), 10.0f);

    AActor *Target = World->SpawnActor<AActor>();
    UMythicAbilitySystemComponent *TargetASC = NewObject<UMythicAbilitySystemComponent>(Target);
    TargetASC->RegisterComponent();
    TargetASC->InitAbilityActorInfo(Target, Target);
    TargetASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Life>(Target));
    TargetASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Defense>(Target));
    TargetASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetMaxHealthAttribute(), 100.0f);
    TargetASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(), 100.0f);
    // Far more shield than the hit can possibly be, so nothing reaches health.
    TargetASC->SetNumericAttributeBase(UMythicAttributeSet_Defense::GetMaxShieldAttribute(), 100000.0f);
    TargetASC->SetNumericAttributeBase(UMythicAttributeSet_Defense::GetShieldAttribute(), 100000.0f);

    int32 ReceivedCount = 0;
    float ReceivedMagnitude = 0.0f;
    TargetASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_DMG_RECEIVED).AddLambda(
        [&ReceivedCount, &ReceivedMagnitude](const FGameplayEventData *Payload) {
            ++ReceivedCount;
            ReceivedMagnitude = Payload ? Payload->EventMagnitude : 0.0f;
        });

    UGameplayEffect *Damage = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("Test_ShieldedHit")));
    Damage->DurationPolicy = EGameplayEffectDurationType::Instant;
    FGameplayEffectExecutionDefinition ExecDef;
    ExecDef.CalculationClass = UMythicDamageApplication::StaticClass();
    Damage->Executions.Add(ExecDef);

    const FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
    if (!TestNotNull(TEXT("the effect context is a Mythic context — the execution aborts on anything else"),
                     FMythicGameplayEffectContext::ExtractEffectContext(Context))) {
        return false;
    }

    SourceASC->ApplyGameplayEffectToTarget(Damage, TargetASC, 1.0f, Context);

    const float ShieldLeft = TargetASC->GetNumericAttribute(UMythicAttributeSet_Defense::GetShieldAttribute());
    const float HealthLeft = TargetASC->GetNumericAttribute(UMythicAttributeSet_Life::GetHealthAttribute());
    const float Absorbed = 100000.0f - ShieldLeft;

    TestTrue(TEXT("the shield absorbed the hit"), Absorbed > 0.0f);
    TestEqual(TEXT("no health was lost"), HealthLeft, 100.0f);

    // The whole on-hit chain lives in the life set's Damage branch. If a fully absorbed hit skips it, the
    // target never aggros and no on-hit effect of the attacker's build fires.
    TestEqual(TEXT("a fully absorbed hit still reports damage received"), ReceivedCount, 1);
    TestEqual(TEXT("the reported magnitude is the damage the hit dealt, not the health it took"),
              ReceivedMagnitude, Absorbed, 0.01f);

    return true;
}
