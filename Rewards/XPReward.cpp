

#include "XPReward.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/PlayerController.h"
#include "World/Camping/MythicRestedXp.h"

bool UXPReward::Give(FRewardContext &Context) const {
    FXPRewardContext *XPContext = static_cast<FXPRewardContext *>(&Context);
    checkf(XPContext, TEXT("XPContext is null"));

    UAbilitySystemComponent *AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Context.PlayerController);
    checkf(AbilitySystemComponent, TEXT("AbilitySystemComponent is null"));

    auto Proficiency = this->ProficiencyDef;
    checkf(Proficiency, TEXT("ProficiencyDef is null"));

    auto TargetLvl = XPContext->Level;
    auto OverlevelBonus = this->OverlevelXPBonus;
    auto PercentageOfActionXPtoGive = this->Percentage;

    float XPToReward = CalculateXP(AbilitySystemComponent, Proficiency, TargetLvl, OverlevelBonus, PercentageOfActionXPtoGive);

    XPToReward *= MythicCampsite::ReadRestedXpMultiplier(AbilitySystemComponent);

    AbilitySystemComponent->ApplyModToAttribute(Proficiency->ProgressAttribute, EGameplayModOp::Additive, XPToReward);

    return true;
}

float UXPReward::CalculateXP(UAbilitySystemComponent *AbilitySystemComponent, UProficiencyDefinition *Proficiency, int32 TargetLvl, float OverlevelBonus,
                             float PercentageOfActionXPtoGive) {
    auto PreScaledXP = Proficiency->BaseXPPerAction * PercentageOfActionXPtoGive;

    if (TargetLvl <= 0) {
        return PreScaledXP;
    }

    float CurrentProgress = AbilitySystemComponent->GetNumericAttribute(Proficiency->ProgressAttribute);
    int32 CurrentLevel = UProficiencyDefinition::CalcLevelAtXP(CurrentProgress, Proficiency);

    int32 LevelDifference = TargetLvl - CurrentLevel;

    return FMath::Max(0.0f, PreScaledXP + (LevelDifference * PreScaledXP * OverlevelBonus));
}

FText UXPReward::GetPreviewText() const {
    if (!ProficiencyDef) {
        return FText::GetEmpty();
    }
    return FText::FromString(FString::Printf(TEXT("%s XP"), *ProficiencyDef->Name.ToString()));
}
