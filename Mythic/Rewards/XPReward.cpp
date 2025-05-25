// 


#include "XPReward.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

bool UXPReward::Give(FRewardContext &Context) const {
    // Cast the context to FXpRewardContext
    FXPRewardContext *XPContext = static_cast<FXPRewardContext *>(&Context);
    checkf(XPContext, TEXT("XPContext is null"));

    // Get the ability system component from the player controller
    UAbilitySystemComponent *AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Context.PlayerController);
    checkf(AbilitySystemComponent, TEXT("AbilitySystemComponent is null"));

    auto Proficiency = this->ProficiencyDef;
    checkf(Proficiency, TEXT("ProficiencyDef is null"));

    auto TargetLvl = XPContext->Level;
    auto OverlevelBonus = this->OverlevelXPBonus;
    auto PercentageOfActionXPtoGive = this->Percentage;

    float XPToReward = CalculateXP(AbilitySystemComponent, Proficiency, TargetLvl, OverlevelBonus, PercentageOfActionXPtoGive);

    AbilitySystemComponent->ApplyModToAttribute(Proficiency->ProgressAttribute, EGameplayModOp::Additive, XPToReward);

    return true;
}

float UXPReward::CalculateXP(UAbilitySystemComponent *AbilitySystemComponent, UProficiencyDefinition *Proficiency, int32 TargetLvl, float OverlevelBonus,
                             float PercentageOfActionXPtoGive) {
    auto PreScaledXP = Proficiency->BaseXPPerAction * PercentageOfActionXPtoGive;

    // If the target level is 0, then no scaling is done
    if (TargetLvl <= 0) {
        return PreScaledXP;
    }

    // Get the current proficiency level
    float CurrentProgress = AbilitySystemComponent->GetNumericAttribute(Proficiency->ProgressAttribute);
    int32 CurrentLevel = UProficiencyDefinition::CalcLevelAtXP(CurrentProgress, Proficiency);

    // Level Difference
    int32 LevelDifference = TargetLvl - CurrentLevel;

    // Scaled XP
    return PreScaledXP + (LevelDifference * PreScaledXP * OverlevelBonus);
}
