
#include "MythicStatusEffects.h"

#include "ScalableFloat.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"
#include "GAS/MythicTags_GAS.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/Effects/MythicCrowdControl.h"

namespace {
    void ConfigureDoT(UGameplayEffect *GE, float DurationSeconds, float DamagePerTick) {
        GE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
        GE->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(DurationSeconds));
        GE->Period = FScalableFloat(1.0f);
        GE->bExecutePeriodicEffectOnApplication = false;

        FGameplayModifierInfo Mod;
        Mod.Attribute = UMythicAttributeSet_Life::GetDamageAttribute();
        Mod.ModifierOp = EGameplayModOp::Additive;
        Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(DamagePerTick));
        GE->Modifiers.Add(Mod);
    }

    void ConfigureTagOnly(UGameplayEffect *GE, float DurationSeconds) {
        GE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
        GE->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(DurationSeconds));
    }
}

void UMythicDebuffGameplayEffect::PostInitProperties() {
    Super::PostInitProperties();

    if (!GrantedDebuffTags.IsEmpty()) {
        UTargetTagsGameplayEffectComponent &TagComp = FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
        FInheritedTagContainer Container;
        for (const FGameplayTag &T : GrantedDebuffTags) {
            Container.AddTag(T);
        }
        TagComp.SetAndApplyTargetTagChanges(Container);
    }

    if (!BlockedApplicationTags.IsEmpty()) {
        UTargetTagRequirementsGameplayEffectComponent &ReqComp = FindOrAddComponent<UTargetTagRequirementsGameplayEffectComponent>();
        ReqComp.ApplicationTagRequirements.IgnoreTags.AppendTags(BlockedApplicationTags);
    }
}

UMythicGE_Burn::UMythicGE_Burn() {
    ConfigureDoT(this, 5.0f, 3.0f);
    GrantedDebuffTags.AddTag(GAS_DEBUFF_BURNING);
}

UMythicGE_Poison::UMythicGE_Poison() {
    ConfigureDoT(this, 5.0f, 3.0f);
    GrantedDebuffTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Status.State.Poisoned")));
    GrantedDebuffTags.AddTag(GAS_DEBUFF_POISONED);
}

UMythicGE_Bleed::UMythicGE_Bleed() {
    ConfigureDoT(this, 5.0f, 3.0f);
    GrantedDebuffTags.AddTag(GAS_DEBUFF_BLEEDING);
}

UMythicGE_Slow::UMythicGE_Slow() {
    ConfigureTagOnly(this, 4.0f);
    GrantedDebuffTags.AddTag(GAS_DEBUFF_SLOWED);
}

UMythicGE_Freeze::UMythicGE_Freeze() {
    ConfigureTagOnly(this, 2.0f);
    GrantedDebuffTags.AddTag(GAS_DEBUFF_FROZEN);
    BlockedApplicationTags.AddTag(GAS_IMMUNE_HARDCC);
}

UMythicGE_Stun::UMythicGE_Stun() {
    ConfigureTagOnly(this, 2.0f);
    GrantedDebuffTags.AddTag(GAS_DEBUFF_STUNNED);
    BlockedApplicationTags.AddTag(GAS_IMMUNE_HARDCC);
}

UMythicGE_Weaken::UMythicGE_Weaken() {
    ConfigureTagOnly(this, 5.0f);
    GrantedDebuffTags.AddTag(GAS_DEBUFF_WEAKENED);
}

UMythicGE_Terrify::UMythicGE_Terrify() {
    ConfigureTagOnly(this, 5.0f);
    GrantedDebuffTags.AddTag(GAS_DEBUFF_TERRIFIED);
}

TSubclassOf<UGameplayEffect> FMythicStatusEffectResolver::ResolveDebuffGEForStatus(const FGameplayTag &StatusType) {
    static const FGameplayTag TypeBurn = FGameplayTag::RequestGameplayTag(FName("Status.Type.Burn"));
    static const FGameplayTag TypePoison = FGameplayTag::RequestGameplayTag(FName("Status.Type.Poison"));
    static const FGameplayTag TypeBleed = FGameplayTag::RequestGameplayTag(FName("Status.Type.Bleed"));
    static const FGameplayTag TypeSlow = FGameplayTag::RequestGameplayTag(FName("Status.Type.Slow"));
    static const FGameplayTag TypeFreeze = FGameplayTag::RequestGameplayTag(FName("Status.Type.Freeze"));
    static const FGameplayTag TypeStun = FGameplayTag::RequestGameplayTag(FName("Status.Type.Stun"));

    if (StatusType == TypeBurn) { return UMythicGE_Burn::StaticClass(); }
    if (StatusType == TypePoison) { return UMythicGE_Poison::StaticClass(); }
    if (StatusType == TypeBleed) { return UMythicGE_Bleed::StaticClass(); }
    if (StatusType == TypeSlow) { return UMythicGE_Slow::StaticClass(); }
    if (StatusType == TypeFreeze) { return UMythicGE_Freeze::StaticClass(); }
    if (StatusType == TypeStun) { return UMythicGE_Stun::StaticClass(); }
    return nullptr;
}
