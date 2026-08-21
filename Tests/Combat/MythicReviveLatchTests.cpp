
#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "ScalableFloat.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/MythicAbilitySystemComponent.h"

namespace {
void ApplyDamage(UAbilitySystemComponent *ASC, float Amount) {
    UGameplayEffect *Effect = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("Test_Damage")));
    Effect->DurationPolicy = EGameplayEffectDurationType::Instant;

    FGameplayModifierInfo Mod;
    Mod.Attribute = UMythicAttributeSet_Life::GetDamageAttribute();
    Mod.ModifierOp = EGameplayModOp::Additive;
    Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Amount));
    Effect->Modifiers.Add(Mod);

    ASC->ApplyGameplayEffectToSelf(Effect, 1.0f, ASC->MakeEffectContext());
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicReviveLatchTest,
    "Mythic.Combat.ReviveLatch",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicReviveLatchTest::RunTest(const FString &Parameters) {
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

    AActor *Target = World->SpawnActor<AActor>();
    UMythicAbilitySystemComponent *ASC = NewObject<UMythicAbilitySystemComponent>(Target);
    ASC->RegisterComponent();
    ASC->InitAbilityActorInfo(Target, Target);
    UMythicAttributeSet_Life *Life = NewObject<UMythicAttributeSet_Life>(Target);
    ASC->AddAttributeSetSubobject(Life);
    ASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Defense>(Target));

    ASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetMaxHealthAttribute(), 100.0f);
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(), 100.0f);
    Life->RefreshOutOfHealthLatch();

    ApplyDamage(ASC, 30.0f);
    TestEqual(TEXT("a normal hit takes health"), ASC->GetNumericAttribute(UMythicAttributeSet_Life::GetHealthAttribute()), 70.0f);

    // Drop to zero: the latch is what stops a corpse from being hit again.
    ApplyDamage(ASC, 200.0f);
    TestEqual(TEXT("lethal damage empties health"), ASC->GetNumericAttribute(UMythicAttributeSet_Life::GetHealthAttribute()), 0.0f);

    // Revive restores health with SetNumericAttributeBase, which skips PostGameplayEffectExecute. Without
    // refreshing the latch the target stays invulnerable for the rest of the session.
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(), 50.0f);
    Life->RefreshOutOfHealthLatch();

    ApplyDamage(ASC, 20.0f);
    TestEqual(TEXT("a revived target can be damaged again"),
              ASC->GetNumericAttribute(UMythicAttributeSet_Life::GetHealthAttribute()), 30.0f);

    // Restoring to zero must NOT clear the latch: that would make a corpse damageable.
    ApplyDamage(ASC, 500.0f);
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(), 0.0f);
    Life->RefreshOutOfHealthLatch();
    ApplyDamage(ASC, 10.0f);
    TestEqual(TEXT("a target still at zero stays latched"),
              ASC->GetNumericAttribute(UMythicAttributeSet_Life::GetHealthAttribute()), 0.0f);

    return true;
}
