
#include "MythicAttributeSet_Defense.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "Player/MythicPlayerController.h"
#include "NativeGameplayTags.h"
#include "GAS/Effects/MythicStatusEffects.h"
#include "GAS/Effects/MythicCrowdControl.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/Feedback/MythicTags_FeedbackCues.h"
#include "AI/MythicTags_AI.h"
#include "GAS/MythicGameplayEffectContext.h"

namespace {
    int32 ResolveTargetTierInt(const UAbilitySystemComponent *ASC) {
        if (!ASC) {
            return -1;
        }
        FGameplayTagContainer Owned;
        ASC->GetOwnedGameplayTags(Owned);
        for (const FGameplayTag &T : Owned) {
            const int32 TierInt = GetAITierInt(T);
            if (TierInt > 0) {
                return TierInt;
            }
        }
        return -1;
    }
}

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
    InitIncomingDamageMultiplier(1.0f);
}

void UMythicAttributeSet_Defense::PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue) {
    Super::PreAttributeChange(Attribute, NewValue);

    if (Attribute == GetShieldAttribute()) {
        ShieldBeforeChange = GetShield();
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
        NewValue = FMath::Clamp(NewValue, 0.0f, 1.0f);
    }
    else if (Attribute == GetBurnBuildupAttribute() || Attribute == GetBleedBuildupAttribute()
        || Attribute == GetPoisonBuildupAttribute() || Attribute == GetSlowBuildupAttribute()
        || Attribute == GetFreezeBuildupAttribute() || Attribute == GetStunBuildupAttribute()) {
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
        if (Absorbed > 0.0f) {
            const UAbilitySystemComponent *ASC = GetOwningAbilitySystemComponent();
            const APawn *Avatar = ASC ? Cast<APawn>(ASC->GetAvatarActor()) : nullptr;
            if (AMythicPlayerController *PC = Avatar ? Cast<AMythicPlayerController>(Avatar->GetController()) : nullptr) {
                const bool bBroke = (GetShield() <= 0.0f && ShieldBeforeChange > 0.0f);
                PC->ClientShowShieldAbsorbed(FMath::RoundToInt(Absorbed), bBroke);
            }
        }
    }

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
                    float Threshold = ComputeBuildupThreshold(Resistance);

                    const bool bHardCC = (StatusTag == TAG_Status_Type_Stun || StatusTag == TAG_Status_Type_Freeze);

                    if (bHardCC && TargetASC->HasMatchingGameplayTag(GAS_IMMUNE_HARDCC))
                    {
                        if (CurrentBuildup >= Threshold)
                        {
                            TargetASC->SetNumericAttributeBase(BuildupAttr, CurrentBuildup - Threshold);

                            if (UMythicAbilitySystemComponent *TargetMythicASC = Cast<UMythicAbilitySystemComponent>(TargetASC)) {
                                FGameplayCueParameters CueParams;
                                if (const AActor *TargetActor = TargetASC->GetAvatarActor()) {
                                    CueParams.Location = TargetActor->GetActorLocation();
                                }
                                TargetMythicASC->ExecuteGameplayCueMulticast(TAG_GameplayCue_Combat_Immune, CueParams);
                            }
                        }
                        return;
                    }

                    float EffectiveThreshold = Threshold;
                    FMythicCcEscalationConfig CcCfg;
                    FMythicCcResolution CcRes;
                    if (bHardCC)
                    {
                        CcCfg = FMythicCrowdControlRules::ConfigForTier(ResolveTargetTierInt(TargetASC));
                        const UWorld *CcWorld = TargetASC->GetWorld();
                        const float Now = CcWorld ? CcWorld->GetTimeSeconds() : 0.0f;
                        CcRes = FMythicCrowdControlRules::ResolveCcTrigger(CcHardTrackStates.FindRef(StatusTag), CcCfg, Now);
                        EffectiveThreshold = Threshold * CcRes.EffectiveThresholdMultiplier;
                    }

                    if (CurrentBuildup >= EffectiveThreshold)
                    {
                        TargetASC->SetNumericAttributeBase(BuildupAttr, CurrentBuildup - EffectiveThreshold);

                        if (bHardCC)
                        {
                            CcHardTrackStates.Add(StatusTag, CcRes.NextState);
                        }

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

                            if (UMythicAbilitySystemComponent *ReactAsc = Cast<UMythicAbilitySystemComponent>(TargetASC)) {
                                FGameplayCueParameters CueParams;
                                if (const AActor *TargetActor = TargetASC->GetAvatarActor()) {
                                    CueParams.Location = TargetActor->GetActorLocation();
                                }
                                ReactAsc->ExecuteGameplayCueMulticast(TAG_GameplayCue_Combat_Reaction, CueParams);
                            }
                        }

                        if (!bReactionTriggered)
                        {
                            FGameplayEventData EventData;
                            EventData.EventTag = ApplyTag;
                            EventData.Instigator = Data.EffectSpec.GetContext().GetInstigator();
                            EventData.Target = TargetASC->GetAvatarActor();
                            EventData.EventMagnitude = 1.0f;
                            TargetASC->HandleGameplayEvent(ApplyTag, &EventData);

                            if (TSubclassOf<UGameplayEffect> DebuffGE = FMythicStatusEffectResolver::ResolveDebuffGEForStatus(StatusTag))
                            {
                                const FGameplayEffectContextHandle &SourceCtx = Data.EffectSpec.GetContext();
                                FGameplayEffectContextHandle DebuffCtx = TargetASC->MakeEffectContext();
                                DebuffCtx.AddInstigator(SourceCtx.GetInstigator(), SourceCtx.GetEffectCauser());
                                FGameplayEffectSpecHandle DebuffSpec = TargetASC->MakeOutgoingSpec(DebuffGE, 1.0f, DebuffCtx);
                                if (DebuffSpec.IsValid())
                                {
                                    TargetASC->ApplyGameplayEffectSpecToSelf(*DebuffSpec.Data.Get());
                                }
                            }

                            FGameplayTag OnsetCue;
                            if (StatusTag == TAG_Status_Type_Burn) { OnsetCue = TAG_GameplayCue_Status_Burn_Onset; }
                            else if (StatusTag == TAG_Status_Type_Bleed) { OnsetCue = TAG_GameplayCue_Status_Bleed_Onset; }
                            else if (StatusTag == TAG_Status_Type_Poison) { OnsetCue = TAG_GameplayCue_Status_Poison_Onset; }
                            else if (StatusTag == TAG_Status_Type_Slow) { OnsetCue = TAG_GameplayCue_Status_Slow_Onset; }
                            else if (StatusTag == TAG_Status_Type_Freeze) { OnsetCue = TAG_GameplayCue_Status_Freeze_Onset; }
                            else if (StatusTag == TAG_Status_Type_Stun) { OnsetCue = TAG_GameplayCue_Status_Stun_Onset; }
                            if (OnsetCue.IsValid()) {
                                if (UMythicAbilitySystemComponent *OnsetAsc = Cast<UMythicAbilitySystemComponent>(TargetASC)) {
                                    FGameplayCueParameters CueParams;
                                    if (const AActor *TargetActor = TargetASC->GetAvatarActor()) {
                                        CueParams.Location = TargetActor->GetActorLocation();
                                    }
                                    OnsetAsc->ExecuteGameplayCueMulticast(OnsetCue, CueParams);
                                }
                            }
                        }

                        if (bHardCC && CcRes.bGrantImmunity)
                        {
                            const FGameplayEffectContextHandle &SrcCtx = Data.EffectSpec.GetContext();
                            FGameplayEffectContextHandle ImmuneCtx = TargetASC->MakeEffectContext();
                            ImmuneCtx.AddInstigator(SrcCtx.GetInstigator(), SrcCtx.GetEffectCauser());
                            FGameplayEffectSpecHandle ImmuneSpec = TargetASC->MakeOutgoingSpec(UMythicGE_CCImmune::StaticClass(), 1.0f, ImmuneCtx);
                            if (ImmuneSpec.IsValid())
                            {
                                ImmuneSpec.Data->SetSetByCallerMagnitude(GAS_SETBYCALLER_CCIMMUNE_DURATION, CcCfg.ImmuneSeconds);
                                TargetASC->ApplyGameplayEffectSpecToSelf(*ImmuneSpec.Data.Get());
                            }
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

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, Armor, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, DodgeChance, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, BurnResistance, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, BleedResistance, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, PoisonResistance, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, SlowResistance, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, FreezeResistance, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, StunResistance, COND_OwnerOnly, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, BurnBuildup, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, BleedBuildup, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, PoisonBuildup, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, SlowBuildup, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, FreezeBuildup, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, StunBuildup, COND_OwnerOnly, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, DecreasedDamageFromEnemiesUnderStatusEffects, COND_OwnerOnly,
                                   REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, HealthRegenRate, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, MaxShield, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, Shield, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, ShieldRegenRate, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, LifePerHit, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, LifePerKill, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, IncomingDamageMultiplier, COND_OwnerOnly, REPNOTIFY_Always);
}

float UMythicAttributeSet_Defense::ComputeBuildupThreshold(float Resistance) {
    return 100.0f + FMath::Clamp(Resistance, 0.0f, 1.0f) * 2.0f;
}

bool UMythicAttributeSet_Defense::BuildupCrossesThreshold(float NewBuildup, float Resistance) {
    return NewBuildup >= ComputeBuildupThreshold(Resistance);
}

float UMythicAttributeSet_Defense::ComputeBuildupAfterDecay(float Cur, float DecayPerSecond, float DeltaSeconds) {
    const float Decay = FMath::Max(0.0f, DecayPerSecond) * FMath::Max(0.0f, DeltaSeconds);
    return FMath::Max(0.0f, Cur - Decay);
}

void UMythicAttributeSet_Defense::DecayAllBuildups(UAbilitySystemComponent *ASC, float DecayPerSecond, float DeltaSeconds) {
    if (!ASC || DecayPerSecond <= 0.0f || DeltaSeconds <= 0.0f) {
        return;
    }
    const UMythicAttributeSet_Defense *Def = ASC->GetSet<UMythicAttributeSet_Defense>();
    if (!Def) {
        return;
    }

    const TPair<FGameplayAttribute, float> Buildups[] = {
        {GetBurnBuildupAttribute(), Def->GetBurnBuildup()},
        {GetBleedBuildupAttribute(), Def->GetBleedBuildup()},
        {GetPoisonBuildupAttribute(), Def->GetPoisonBuildup()},
        {GetSlowBuildupAttribute(), Def->GetSlowBuildup()},
        {GetFreezeBuildupAttribute(), Def->GetFreezeBuildup()},
        {GetStunBuildupAttribute(), Def->GetStunBuildup()},
    };
    for (const TPair<FGameplayAttribute, float> &B : Buildups) {
        if (B.Value > 0.0f) {
            const float NewV = ComputeBuildupAfterDecay(B.Value, DecayPerSecond, DeltaSeconds);
            if (NewV != B.Value) {
                ASC->SetNumericAttributeBase(B.Key, NewV);
            }
        }
    }
}

void UMythicAttributeSet_Defense::ResetCcAndBuildupState() {
    CcHardTrackStates.Reset();

    if (UAbilitySystemComponent *ASC = GetOwningAbilitySystemComponent()) {
        ASC->SetNumericAttributeBase(GetBurnBuildupAttribute(), 0.0f);
        ASC->SetNumericAttributeBase(GetBleedBuildupAttribute(), 0.0f);
        ASC->SetNumericAttributeBase(GetPoisonBuildupAttribute(), 0.0f);
        ASC->SetNumericAttributeBase(GetSlowBuildupAttribute(), 0.0f);
        ASC->SetNumericAttributeBase(GetFreezeBuildupAttribute(), 0.0f);
        ASC->SetNumericAttributeBase(GetStunBuildupAttribute(), 0.0f);

        ASC->SetNumericAttributeBase(GetShieldAttribute(), 0.0f);
    }
}
