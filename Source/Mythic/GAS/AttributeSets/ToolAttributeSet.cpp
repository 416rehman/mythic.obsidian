#include "ToolAttributeSet.h"

#include "Net/UnrealNetwork.h"

void UToolAttributeSet::OnRep_PickaxeSpeedPercent(const FGameplayAttributeData& OldPickaxeSpeedPercent) {
	GAMEPLAYATTRIBUTE_REPNOTIFY(UToolAttributeSet, PickaxeSpeedPercent, OldPickaxeSpeedPercent);
}
void UToolAttributeSet::OnRep_ScytheSpeedPercent(const FGameplayAttributeData& OldScytheSpeedPercent) {
	GAMEPLAYATTRIBUTE_REPNOTIFY(UToolAttributeSet, ScytheSpeedPercent, OldScytheSpeedPercent);
}
void UToolAttributeSet::OnRep_HatchetSpeedPercent(const FGameplayAttributeData& OldHatchetSpeedPercent) {
	GAMEPLAYATTRIBUTE_REPNOTIFY(UToolAttributeSet, HatchetSpeedPercent, OldHatchetSpeedPercent);
}

void UToolAttributeSet::OnRep_PickaxeYield(const FGameplayAttributeData& OldPickaxeYield) {
	GAMEPLAYATTRIBUTE_REPNOTIFY(UToolAttributeSet, PickaxeYieldPercent, OldPickaxeYield);
}
void UToolAttributeSet::OnRep_ScytheYield(const FGameplayAttributeData& OldScytheYield) {
	GAMEPLAYATTRIBUTE_REPNOTIFY(UToolAttributeSet, ScytheYieldPercent, OldScytheYield);
}
void UToolAttributeSet::OnRep_HatchetYield(const FGameplayAttributeData& OldHatchetYield) {
	GAMEPLAYATTRIBUTE_REPNOTIFY(UToolAttributeSet, HatchetYieldPercent, OldHatchetYield);
}

void UToolAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UToolAttributeSet, PickaxeSpeedPercent, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UToolAttributeSet, ScytheSpeedPercent, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UToolAttributeSet, HatchetSpeedPercent, COND_None, REPNOTIFY_Always);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UToolAttributeSet, PickaxeYieldPercent, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UToolAttributeSet, ScytheYieldPercent, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UToolAttributeSet, HatchetYieldPercent, COND_None, REPNOTIFY_Always);
}


