#include "MythicAttributeSet_Tool.h"

#include "Net/UnrealNetwork.h"

void UMythicAttributeSet_Tool::OnRep_PickaxeSpeedPercent(const FGameplayAttributeData& OldPickaxeSpeedPercent) {
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Tool, PickaxeSpeedPercent, OldPickaxeSpeedPercent);
}
void UMythicAttributeSet_Tool::OnRep_ScytheSpeedPercent(const FGameplayAttributeData& OldScytheSpeedPercent) {
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Tool, ScytheSpeedPercent, OldScytheSpeedPercent);
}
void UMythicAttributeSet_Tool::OnRep_HatchetSpeedPercent(const FGameplayAttributeData& OldHatchetSpeedPercent) {
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Tool, HatchetSpeedPercent, OldHatchetSpeedPercent);
}

void UMythicAttributeSet_Tool::OnRep_PickaxeYield(const FGameplayAttributeData& OldPickaxeYield) {
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Tool, PickaxeYieldPercent, OldPickaxeYield);
}
void UMythicAttributeSet_Tool::OnRep_ScytheYield(const FGameplayAttributeData& OldScytheYield) {
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Tool, ScytheYieldPercent, OldScytheYield);
}
void UMythicAttributeSet_Tool::OnRep_HatchetYield(const FGameplayAttributeData& OldHatchetYield) {
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Tool, HatchetYieldPercent, OldHatchetYield);
}

void UMythicAttributeSet_Tool::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Tool, PickaxeSpeedPercent, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Tool, ScytheSpeedPercent, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Tool, HatchetSpeedPercent, COND_None, REPNOTIFY_Always);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Tool, PickaxeYieldPercent, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Tool, ScytheYieldPercent, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Tool, HatchetYieldPercent, COND_None, REPNOTIFY_Always);
}


