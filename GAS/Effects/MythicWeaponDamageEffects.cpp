#include "GAS/Effects/MythicWeaponDamageEffects.h"

#include "GAS/Executions/MythicDamageApplication.h"
#include "GAS/Executions/MythicDamageCalculation.h"
#include "GAS/Feedback/MythicTags_FeedbackCues.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MythicWeaponDamageEffects)

namespace {
template <typename TExecution>
void ConfigureExactInstantExecution(UGameplayEffect &Effect) {
    Effect.DurationPolicy = EGameplayEffectDurationType::Instant;
    Effect.Modifiers.Reset();
    Effect.Executions.Reset();

    FGameplayEffectExecutionDefinition &Execution =
        Effect.Executions.AddDefaulted_GetRef();
    Execution.CalculationClass = TExecution::StaticClass();
}
}

UMythicGE_WeaponDamageCalculation::UMythicGE_WeaponDamageCalculation() {
    ConfigureExactInstantExecution<UMythicDamageCalculation>(*this);
    GameplayCues.Reset();
}

UMythicGE_WeaponDamageApplication::UMythicGE_WeaponDamageApplication() {
    ConfigureExactInstantExecution<UMythicDamageApplication>(*this);
    GameplayCues.Reset();
    GameplayCues.Emplace(TAG_GameplayCue_Damage_Hit, 0.0f, 0.0f);
}

TSubclassOf<UGameplayEffect> MythicWeaponDamage::GetCalculationEffectClass() {
    return UMythicGE_WeaponDamageCalculation::StaticClass();
}

TSubclassOf<UGameplayEffect> MythicWeaponDamage::GetApplicationEffectClass() {
    return UMythicGE_WeaponDamageApplication::StaticClass();
}

FMythicDamageContainer MythicWeaponDamage::MakeDamageContainer() {
    FMythicDamageContainer Container;
    Container.DamageCalculationEffect = GetCalculationEffectClass();
    Container.DamageApplicationEffect = GetApplicationEffectClass();
    return Container;
}
