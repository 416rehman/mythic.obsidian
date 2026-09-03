
#include "GAS/Effects/MythicCrowdControl.h"

#include "ScalableFloat.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_IMMUNE_HARDCC, "GAS.Immune.HardCC",
                               "Target is temporarily immune to hard crowd control (Stun/Freeze); granted by the CC-escalation framework");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_IMMUNE_FALLDAMAGE, "GAS.Immune.FallDamage",
                               "Target takes no fall damage while this is on its ASC; held by a rune as a loose server tag");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_SETBYCALLER_CCIMMUNE_DURATION, "SetByCaller.CCImmuneDuration",
                               "SetByCaller duration (seconds) fed to UMythicGE_CCImmune at apply time (per-tier ImmuneSeconds)");

UMythicGE_CCImmune::UMythicGE_CCImmune() {
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    FSetByCallerFloat DurationSBC;
    DurationSBC.DataTag = GAS_SETBYCALLER_CCIMMUNE_DURATION;
    DurationMagnitude = FGameplayEffectModifierMagnitude(DurationSBC);
}

void UMythicGE_CCImmune::PostInitProperties() {
    Super::PostInitProperties();

    UTargetTagsGameplayEffectComponent &TagComp = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
    FInheritedTagContainer Container;
    Container.AddTag(GAS_IMMUNE_HARDCC);
    TagComp.SetAndApplyTargetTagChanges(Container);
}
