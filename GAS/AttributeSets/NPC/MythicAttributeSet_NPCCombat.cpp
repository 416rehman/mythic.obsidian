#include "MythicAttributeSet_NPCCombat.h"

#include "Net/UnrealNetwork.h"

void UMythicAttributeSet_NPCCombat::OnRep_Damage(const FGameplayAttributeData &OldDamage) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_NPCCombat, Damage, OldDamage);
}

void UMythicAttributeSet_NPCCombat::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_NPCCombat, Damage, COND_None, REPNOTIFY_Always);
}
