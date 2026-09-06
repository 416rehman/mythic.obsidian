#pragma once
#include "GAS/Abilities/MythicGA_AbilityWindow.h"
#include "GAS/Abilities/MythicAbilityCost_Stamina.h"
#include "MythicAbilityWindowTestProbe.generated.h"

/** Concrete native automation probe. Production activation, costs and target admission remain unchanged. */
UCLASS(NotBlueprintable, Transient, Hidden)
class UMythicAbilityWindowTestProbe : public UMythicGA_AbilityWindow {
    GENERATED_BODY()
public:
    UMythicAbilityWindowTestProbe() {
        AdditionalCosts.Add(CreateDefaultSubobject<UMythicAbilityCost_Stamina>(TEXT("Stamina")));
    }
    bool bCancelDuringValidation=false;
protected:
    virtual bool ValidateAbilityIntent_Implementation() override {
        if (bCancelDuringValidation) EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,true,true);
        return true;
    }
};

UCLASS(NotBlueprintable, Transient, Hidden)
class UMythicAbilityTestCooldown : public UGameplayEffect {
    GENERATED_BODY()
public:
    UMythicAbilityTestCooldown() {
        DurationPolicy=EGameplayEffectDurationType::HasDuration;
        DurationMagnitude=FGameplayEffectModifierMagnitude(FScalableFloat(1));
    }
};
