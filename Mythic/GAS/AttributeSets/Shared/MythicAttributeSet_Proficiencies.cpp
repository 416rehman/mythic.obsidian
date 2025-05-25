// 
#include "MythicAttributeSet_Proficiencies.h"
#include "Net/UnrealNetwork.h"

void UMythicAttributeSet_Proficiencies::OnRep_CombatProficiency(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, CombatProficiency, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_WoodcuttingProficiency(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, WoodcuttingProficiency, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_ConstructionProficiency(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, ConstructionProficiency, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_TradingProficiency(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, TradingProficiency, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_HuntingProficiency(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, HuntingProficiency, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_CraftingProficiency(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, CraftingProficiency, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_AlchemyProficiency(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, AlchemyProficiency, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_CookingProficiency(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, CookingProficiency, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_MiningProficiency(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, MiningProficiency, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_FarmingProficiency(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, FarmingProficiency, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_FishingProficiency(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, FishingProficiency, OldValue);
}

void UMythicAttributeSet_Proficiencies::OnRep_HarvestingProficiency(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Proficiencies, HarvestingProficiency, OldValue);
}

void UMythicAttributeSet_Proficiencies::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, CombatProficiency, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, WoodcuttingProficiency, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, MiningProficiency, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, ConstructionProficiency, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, TradingProficiency, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, HuntingProficiency, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, FishingProficiency, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, FarmingProficiency, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, HarvestingProficiency, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, CraftingProficiency, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, AlchemyProficiency, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Proficiencies, CookingProficiency, COND_None, REPNOTIFY_Always);
}

void UMythicAttributeSet_Proficiencies::PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue) {
    Super::PreAttributeChange(Attribute, NewValue);

    auto Max = 0;
    if (this->ProficiencyMaximums.Contains(Attribute.AttributeName)) {
        Max = this->ProficiencyMaximums[Attribute.AttributeName];
    }

    NewValue = FMath::Clamp(NewValue, 0.0f, Max);
}
