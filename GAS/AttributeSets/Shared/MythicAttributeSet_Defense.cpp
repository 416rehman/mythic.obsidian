// 

#include "MythicAttributeSet_Defense.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h" // avatar pawn -> owning player controller
#include "Player/MythicPlayerController.h" // ClientShowShieldAbsorbed (shield combat feedback RPC)
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Type_Burn, "Status.Type.Burn");
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Type_Poison, "Status.Type.Poison");
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Type_Bleed, "Status.Type.Bleed");
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Type_Slow, "Status.Type.Slow");
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Type_Freeze, "Status.Type.Freeze");
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Type_Stun, "Status.Type.Stun");
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_State_Poisoned, "Status.State.Poisoned");
UE_DEFINE_GAMEPLAY_TAG(TAG_Reaction_ExplosiveToxin, "Reaction.ExplosiveToxin");
UE_DEFINE_GAMEPLAY_TAG(TAG_Event_ApplyStatus, "Event.ApplyStatus");

UMythicAttributeSet_Defense::UMythicAttributeSet_Defense() {
    // initialize incoming damage multiplier to standard scale
    InitIncomingDamageMultiplier(1.0f);
}

void UMythicAttributeSet_Defense::PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue) {
    Super::PreAttributeChange(Attribute, NewValue);

    if (Attribute == GetShieldAttribute()) {
        ShieldBeforeChange = GetShield(); // cache prechange value to compute exact shield damage absorbed
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxShield());
    }
    else if (Attribute == GetMaxShieldAttribute()) {
        NewValue = FMath::Max(0.0f, NewValue);
    }
    else if (Attribute == GetDodgeChanceAttribute()
        || Attribute == GetBurnResistanceAttribute() || Attribute == GetBleedResistanceAttribute()
        || Attribute == GetPoisonResistanceAttribute() || Attribute == GetSlowResistanceAttribute()
        || Attribute == GetFreezeResistanceAttribute() || Attribute == GetStunResistanceAttribute()
        || Attribute == GetDecreasedDamageFromEnemiesUnderStatusEffectsAttribute()) {
        // clamp probability and reduction fraction attributes to valid range
        NewValue = FMath::Clamp(NewValue, 0.0f, 1.0f);
    }
    else if (Attribute == GetBurnBuildupAttribute() || Attribute == GetBleedBuildupAttribute()
        || Attribute == GetPoisonBuildupAttribute() || Attribute == GetSlowBuildupAttribute()
        || Attribute == GetFreezeBuildupAttribute() || Attribute == GetStunBuildupAttribute()) {
        // clamp buildup so it never goes below 0 (no permanent immunity black holes)
        NewValue = FMath::Max(0.0f, NewValue);
    }
    else if (Attribute == GetArmorAttribute()) {
        NewValue = FMath::Max(0.0f, NewValue);
    }
    else if (Attribute == GetIncomingDamageMultiplierAttribute()) {
        NewValue = FMath::Max(0.0f, NewValue);
    }
}

void UMythicAttributeSet_Defense::PostGameplayEffectExecute(const FGameplayEffectModCallbackData &Data) {
    Super::PostGameplayEffectExecute(Data);

    if (Data.EvaluatedData.Attribute == GetMaxShieldAttribute() && GetShield() > GetMaxShield()) {
        SetShield(GetMaxShield());
    }
    else if (Data.EvaluatedData.Attribute == GetShieldAttribute()) {
        const float Absorbed = ShieldBeforeChange - GetShield();
        if (Absorbed > 0.0f) { // check if shield decreased indicating damage absorbed
            const UAbilitySystemComponent *ASC = GetOwningAbilitySystemComponent();
            const APawn *Avatar = ASC ? Cast<APawn>(ASC->GetAvatarActor()) : nullptr;
            if (AMythicPlayerController *PC = Avatar ? Cast<AMythicPlayerController>(Avatar->GetController()) : nullptr) {
                const bool bBroke = (GetShield() <= 0.0f && ShieldBeforeChange > 0.0f);
                PC->ClientShowShieldAbsorbed(FMath::RoundToInt(Absorbed), bBroke);
            }
        }
    }
    
    // Status Effect Buildup Threshold Checks
    if (Data.EvaluatedData.Attribute == GetBurnBuildupAttribute() || Data.EvaluatedData.Attribute == GetPoisonBuildupAttribute() || 
        Data.EvaluatedData.Attribute == GetBleedBuildupAttribute() || Data.EvaluatedData.Attribute == GetSlowBuildupAttribute() || 
        Data.EvaluatedData.Attribute == GetFreezeBuildupAttribute() || Data.EvaluatedData.Attribute == GetStunBuildupAttribute())
    {
        UAbilitySystemComponent* TargetASC = GetOwningAbilitySystemComponent();
        if (TargetASC)
        {
            auto CheckAndTriggerStatus = [this, TargetASC, &Data](const FGameplayAttribute& BuildupAttr, const FGameplayAttribute& ResistAttr, const FGameplayTag& StatusTag, const FGameplayTag& ApplyTag)
            {
                if (Data.EvaluatedData.Attribute == BuildupAttr)
                {
                    float CurrentBuildup = BuildupAttr.GetNumericValue(this);
                    float Resistance = ResistAttr.GetNumericValue(this);
                    float Threshold = 100.0f + (Resistance * 2.0f);

                    if (CurrentBuildup >= Threshold)
                    {
                        // Deduct threshold from buildup (rollover)
                        TargetASC->SetNumericAttributeBase(BuildupAttr, CurrentBuildup - Threshold);
                        
                        // Check for Systemic Reaction: Explosive Toxin
                        bool bReactionTriggered = false;
                        if (StatusTag == TAG_Status_Type_Burn && TargetASC->HasMatchingGameplayTag(TAG_Status_State_Poisoned))
                        {
                            bReactionTriggered = true;
                            TargetASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(TAG_Status_State_Poisoned));
                            
                            FGameplayEventData EventData;
                            EventData.EventTag = TAG_Reaction_ExplosiveToxin;
                            EventData.Instigator = Data.EffectSpec.GetContext().GetInstigator();
                            EventData.Target = TargetASC->GetAvatarActor();
                            TargetASC->HandleGameplayEvent(EventData.EventTag, &EventData);
                        }

                        if (!bReactionTriggered)
                        {
                            FGameplayEventData EventData;
                            EventData.EventTag = ApplyTag;
                            EventData.Instigator = Data.EffectSpec.GetContext().GetInstigator();
                            EventData.Target = TargetASC->GetAvatarActor();
                            EventData.EventMagnitude = 1.0f;
                            TargetASC->HandleGameplayEvent(ApplyTag, &EventData);
                        }
                    }
                }
            };

            CheckAndTriggerStatus(GetBurnBuildupAttribute(), GetBurnResistanceAttribute(), TAG_Status_Type_Burn, TAG_Event_ApplyStatus);
            CheckAndTriggerStatus(GetPoisonBuildupAttribute(), GetPoisonResistanceAttribute(), TAG_Status_Type_Poison, TAG_Event_ApplyStatus);
            CheckAndTriggerStatus(GetBleedBuildupAttribute(), GetBleedResistanceAttribute(), TAG_Status_Type_Bleed, TAG_Event_ApplyStatus);
            CheckAndTriggerStatus(GetSlowBuildupAttribute(), GetSlowResistanceAttribute(), TAG_Status_Type_Slow, TAG_Event_ApplyStatus);
            CheckAndTriggerStatus(GetFreezeBuildupAttribute(), GetFreezeResistanceAttribute(), TAG_Status_Type_Freeze, TAG_Event_ApplyStatus);
            CheckAndTriggerStatus(GetStunBuildupAttribute(), GetStunResistanceAttribute(), TAG_Status_Type_Stun, TAG_Event_ApplyStatus);
        }
    }
}

void UMythicAttributeSet_Defense::OnRep_Armor(const FGameplayAttributeData &OldArmor) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, Armor, OldArmor);
}

void UMythicAttributeSet_Defense::OnRep_DodgeChance(const FGameplayAttributeData &OldDodgeChance) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, DodgeChance, OldDodgeChance);
}

void UMythicAttributeSet_Defense::OnRep_BurnResistance(const FGameplayAttributeData &OldBurnResistance) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, BurnResistance, OldBurnResistance);
}

void UMythicAttributeSet_Defense::OnRep_BleedResistance(const FGameplayAttributeData &OldBleedResistance) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, BleedResistance, OldBleedResistance);
}

void UMythicAttributeSet_Defense::OnRep_PoisonResistance(const FGameplayAttributeData &OldPoisonResistance) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, PoisonResistance, OldPoisonResistance);
}

void UMythicAttributeSet_Defense::OnRep_SlowResistance(const FGameplayAttributeData &OldSlowResistance) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, SlowResistance, OldSlowResistance);
}

void UMythicAttributeSet_Defense::OnRep_FreezeResistance(const FGameplayAttributeData &OldFreezeResistance) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, FreezeResistance, OldFreezeResistance);
}

void UMythicAttributeSet_Defense::OnRep_StunResistance(const FGameplayAttributeData &OldStunResistance) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, StunResistance, OldStunResistance);
}

void UMythicAttributeSet_Defense::OnRep_BurnBuildup(const FGameplayAttributeData &OldBurnBuildup) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, BurnBuildup, OldBurnBuildup);
}

void UMythicAttributeSet_Defense::OnRep_BleedBuildup(const FGameplayAttributeData &OldBleedBuildup) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, BleedBuildup, OldBleedBuildup);
}

void UMythicAttributeSet_Defense::OnRep_PoisonBuildup(const FGameplayAttributeData &OldPoisonBuildup) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, PoisonBuildup, OldPoisonBuildup);
}

void UMythicAttributeSet_Defense::OnRep_SlowBuildup(const FGameplayAttributeData &OldSlowBuildup) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, SlowBuildup, OldSlowBuildup);
}

void UMythicAttributeSet_Defense::OnRep_FreezeBuildup(const FGameplayAttributeData &OldFreezeBuildup) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, FreezeBuildup, OldFreezeBuildup);
}

void UMythicAttributeSet_Defense::OnRep_StunBuildup(const FGameplayAttributeData &OldStunBuildup) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, StunBuildup, OldStunBuildup);
}

void UMythicAttributeSet_Defense::OnRep_DecreasedDamageFromEnemiesUnderStatusEffects(
    const FGameplayAttributeData &OldDecreasedDamageFromEnemiesUnderStatusEffects) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, DecreasedDamageFromEnemiesUnderStatusEffects,
                                OldDecreasedDamageFromEnemiesUnderStatusEffects);
}

void UMythicAttributeSet_Defense::OnRep_HealthRegenRate(const FGameplayAttributeData &OldHealthRegenRate) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, HealthRegenRate, OldHealthRegenRate);
}

void UMythicAttributeSet_Defense::OnRep_MaxShield(const FGameplayAttributeData &OldMaxShield) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, MaxShield, OldMaxShield);
}

void UMythicAttributeSet_Defense::OnRep_Shield(const FGameplayAttributeData &OldShield) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, Shield, OldShield);
}

void UMythicAttributeSet_Defense::OnRep_ShieldRegenRate(const FGameplayAttributeData &OldShieldRegenRate) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, ShieldRegenRate, OldShieldRegenRate);
}

void UMythicAttributeSet_Defense::OnRep_LifePerHit(const FGameplayAttributeData &OldLifePerHit) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, LifePerHit, OldLifePerHit);
}

void UMythicAttributeSet_Defense::OnRep_LifePerKill(const FGameplayAttributeData &OldLifePerKill) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, LifePerKill, OldLifePerKill);
}

void UMythicAttributeSet_Defense::OnRep_IncomingDamageMultiplier(const FGameplayAttributeData &OldIncomingDamageMultiplier) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, IncomingDamageMultiplier, OldIncomingDamageMultiplier);
}

void UMythicAttributeSet_Defense::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // register defensive attributes for network replication
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, Armor, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, DodgeChance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, BurnResistance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, BleedResistance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, PoisonResistance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, SlowResistance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, FreezeResistance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, StunResistance, COND_None, REPNOTIFY_Always);
    
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, BurnBuildup, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, BleedBuildup, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, PoisonBuildup, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, SlowBuildup, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, FreezeBuildup, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, StunBuildup, COND_None, REPNOTIFY_Always);
    
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, DecreasedDamageFromEnemiesUnderStatusEffects, COND_None,
                                   REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, HealthRegenRate, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, MaxShield, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, Shield, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, ShieldRegenRate, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, LifePerHit, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, LifePerKill, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, IncomingDamageMultiplier, COND_None, REPNOTIFY_Always);
}
