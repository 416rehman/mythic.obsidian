
#include "MythicCombatBuffs.h"

#include "ScalableFloat.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GAS/MythicTags_GAS.h"

void UMythicBuffGameplayEffect::PostInitProperties() {
    Super::PostInitProperties();

    if (!GrantedBuffTags.IsEmpty()) {
        UTargetTagsGameplayEffectComponent &TagComp = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
        FInheritedTagContainer Container;
        for (const FGameplayTag &T : GrantedBuffTags) {
            Container.AddTag(T);
        }
        TagComp.SetAndApplyTargetTagChanges(Container);
    }
}

UMythicGE_EvadeIFrames::UMythicGE_EvadeIFrames() {
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(IFrameDuration));
    GrantedBuffTags.AddTag(GAS_BUFF_INVINCIBLE);
}

UMythicGE_Block::UMythicGE_Block() {
    DurationPolicy = EGameplayEffectDurationType::Infinite;
    GrantedBuffTags.AddTag(GAS_BUFF_FORTIFY);
}
