#include "MythicRuneDefinition.h"

#include "GAS/Abilities/MythicGA_Rune.h"

bool UMythicRuneDefinition::HasPayload() const {
    return Ability.Get() && Ability->IsChildOf(UMythicGA_Rune::StaticClass());
}

float UMythicRuneDefinition::GetParameterMidpoint(FGameplayTag Parameter, float Fallback) const {
    const FRollDefinition *Roll = Parameters.Find(Parameter);
    if (!Roll) {
        return Fallback;
    }
    const float Midpoint = (Roll->Min + Roll->Max) * 0.5f;
    return Roll->bWholeNumber ? FMath::RoundToFloat(Midpoint) : Midpoint;
}
