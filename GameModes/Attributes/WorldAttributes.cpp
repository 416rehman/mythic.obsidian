// 
#include "WorldAttributes.h"

#include "GameModes/GameState/MythicGameState.h"
#include "Net/UnrealNetwork.h"


class AMythicGameState;

void UWorldTierAttributes::OnRep_GoldDropRateMultiplier(const FGameplayAttributeData &OldGoldDropRate) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UWorldTierAttributes, GoldDropRateMultiplier, OldGoldDropRate);
}

void UWorldTierAttributes::OnRep_ExperienceGainMultiplier(const FGameplayAttributeData &OldExperienceGain) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UWorldTierAttributes, ExperienceGainMultiplier, OldExperienceGain);
}

void UWorldTierAttributes::OnRep_LegendaryDropRateMultiplier(const FGameplayAttributeData &OldLegendaryDropRate) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UWorldTierAttributes, LegendaryDropRateMultiplier, OldLegendaryDropRate);
}

void UWorldTierAttributes::OnRep_MythicDropRateMultiplier(const FGameplayAttributeData &OldMythicDropRate) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UWorldTierAttributes, ExoticDropRateMultiplier, OldMythicDropRate);
}

void UWorldTierAttributes::OnRep_EnemyHealthMultiplier(const FGameplayAttributeData &OldEnemyHealth) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UWorldTierAttributes, EnemyHealthMultiplier, OldEnemyHealth);
}

void UWorldTierAttributes::OnRep_EnemyDamageMultiplier(const FGameplayAttributeData &OldEnemyDamage) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UWorldTierAttributes, EnemyDamageMultiplier, OldEnemyDamage);
}

void UWorldTierAttributes::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UWorldTierAttributes, GoldDropRateMultiplier, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UWorldTierAttributes, ExperienceGainMultiplier, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UWorldTierAttributes, LegendaryDropRateMultiplier, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UWorldTierAttributes, ExoticDropRateMultiplier, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UWorldTierAttributes, EnemyHealthMultiplier, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UWorldTierAttributes, EnemyDamageMultiplier, COND_None, REPNOTIFY_Always);
}