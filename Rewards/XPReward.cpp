

#include "XPReward.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/PlayerController.h"
#include "Player/MythicPlayerController.h"
#include "Player/Proficiency/ProficiencyComponent.h"

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

    const float XPToReward = CalculateXP(AbilitySystemComponent, Proficiency, TargetLvl, OverlevelBonus, PercentageOfActionXPtoGive);

    /**
     * Granted through the proficiency component rather than written straight onto the attribute. That path writes
     * the BASE value, which is what progression persists, and it is the only one that runs the scaling in
     * PreAttributeBaseChange - the XP bonus stat, Enlighten, rested and the world tier multiplier. It also emits
     * GAS.Event.Proficiency.Gained, so a talent that rewards work sees a kill's XP like any other.
     */
    AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(Context.PlayerController);
    UProficiencyComponent *ProfComp = MythicPC
        ? const_cast<UProficiencyComponent *>(MythicPC->GetProficiencyComponent())
        : nullptr;
    if (!ProfComp) {
        return false;
    }

    ProfComp->GrantProficiencyXP(Proficiency, XPToReward);
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
