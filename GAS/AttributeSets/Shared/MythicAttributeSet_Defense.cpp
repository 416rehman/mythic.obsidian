
#include "MythicAttributeSet_Defense.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "Player/MythicPlayerController.h"
#include "NativeGameplayTags.h"
#include "Engine/GameInstance.h"
#include "GAS/Effects/MythicStatusEffectDefinition.h"
#include "GAS/Effects/MythicStatusRegistry.h"
#include "Settings/MythicCombatSettings.h"
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
        || Attribute == GetWeakenResistanceAttribute() || Attribute == GetTerrifyResistanceAttribute()
        || Attribute == GetDecreasedDamageFromEnemiesUnderStatusEffectsAttribute()) {
        NewValue = FMath::Clamp(NewValue, 0.0f, 1.0f);
    }
    else if (Attribute == GetBurnBuildupAttribute() || Attribute == GetBleedBuildupAttribute()
        || Attribute == GetPoisonBuildupAttribute() || Attribute == GetSlowBuildupAttribute()
        || Attribute == GetFreezeBuildupAttribute() || Attribute == GetStunBuildupAttribute()
        || Attribute == GetWeakenBuildupAttribute() || Attribute == GetTerrifyBuildupAttribute()) {
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

    if (!Data.EvaluatedData.Attribute.IsValid()) {
        return;
    }

    UAbilitySystemComponent *TargetASC = GetOwningAbilitySystemComponent();
    if (!TargetASC) {
        return;
    }

    const UWorld *World = TargetASC->GetWorld();
    const UGameInstance *GameInstance = World ? World->GetGameInstance() : nullptr;
    const UMythicStatusRegistry *Registry = GameInstance ? GameInstance->GetSubsystem<UMythicStatusRegistry>() : nullptr;
    if (!Registry) {
        return;
    }

    const UMythicStatusEffectDefinition *Definition = Registry->FindStatusByBuildupAttribute(Data.EvaluatedData.Attribute);
    if (!Definition) {
        return;
    }

    const float CurrentBuildup = TargetASC->GetNumericAttribute(Data.EvaluatedData.Attribute);
    const float Resistance = TargetASC->HasAttributeSetForAttribute(Definition->ResistanceAttribute)
                                 ? TargetASC->GetNumericAttribute(Definition->ResistanceAttribute)
                                 : 0.0f;
    const float Threshold = ComputeBuildupThreshold(Resistance);
    const bool bHardCC = Definition->bHardCrowdControl;

    if (bHardCC && TargetASC->HasMatchingGameplayTag(GAS_IMMUNE_HARDCC)) {
        if (CurrentBuildup >= Threshold) {
            TargetASC->SetNumericAttributeBase(Data.EvaluatedData.Attribute, CurrentBuildup - Threshold);
            UMythicStatusRegistry::PlayStatusCue(TargetASC, TAG_GameplayCue_Combat_Immune);
        }
        return;
    }

    float EffectiveThreshold = Threshold;
    FMythicCcEscalationConfig CcCfg;
    FMythicCcResolution CcRes;
    if (bHardCC) {
        static const TArray<FMythicCcTierEscalation> EmptyCcTable;
        const UMythicCombatSettings *CcSettings = GetDefault<UMythicCombatSettings>();
        CcCfg = FMythicCrowdControlRules::ConfigForTier(
            CcSettings ? CcSettings->CcEscalationByTier : EmptyCcTable, ResolveTargetTierInt(TargetASC));
        const float Now = World ? World->GetTimeSeconds() : 0.0f;
        CcRes = FMythicCrowdControlRules::ResolveCcTrigger(CcHardTrackStates.FindRef(Definition->StatusType), CcCfg, Now);
        EffectiveThreshold = Threshold * CcRes.EffectiveThresholdMultiplier;
    }

    if (CurrentBuildup < EffectiveThreshold) {
        return;
    }

    TargetASC->SetNumericAttributeBase(Data.EvaluatedData.Attribute, CurrentBuildup - EffectiveThreshold);

    if (bHardCC) {
        CcHardTrackStates.Add(Definition->StatusType, CcRes.NextState);
    }

    AActor *EffectInstigator = Data.EffectSpec.GetContext().GetInstigator();
    AActor *EffectCauser = Data.EffectSpec.GetContext().GetEffectCauser();

    const FMythicStatusReaction *Reaction = nullptr;
    for (const FMythicStatusReaction &Candidate : Definition->Reactions) {
        if (Candidate.RequiredTargetTag.IsValid() && TargetASC->HasMatchingGameplayTag(Candidate.RequiredTargetTag)) {
            Reaction = &Candidate;
            break;
        }
    }

    if (Reaction) {
        if (Reaction->bConsumeExistingStatus) {
            TargetASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(Reaction->RequiredTargetTag));
        }
        if (Reaction->ReactionEventTag.IsValid()) {
            FGameplayEventData ReactionEvent;
            ReactionEvent.EventTag = Reaction->ReactionEventTag;
            ReactionEvent.Instigator = EffectInstigator;
            ReactionEvent.Target = TargetASC->GetAvatarActor();
            ReactionEvent.OptionalObject = Definition;
            TargetASC->HandleGameplayEvent(ReactionEvent.EventTag, &ReactionEvent);
        }
        UMythicStatusRegistry::PlayStatusCue(TargetASC, Reaction->ReactionCueTag);
    }

    if (!Reaction || !Reaction->bSuppressStatusApplication) {
        FGameplayEventData ApplyEvent;
        ApplyEvent.EventTag = TAG_Event_ApplyStatus;
        ApplyEvent.Instigator = EffectInstigator;
        ApplyEvent.Target = TargetASC->GetAvatarActor();
        ApplyEvent.EventMagnitude = 1.0f;
        ApplyEvent.OptionalObject = Definition;
        TargetASC->HandleGameplayEvent(TAG_Event_ApplyStatus, &ApplyEvent);

        UMythicStatusRegistry::ApplyStatusEffect(TargetASC, Definition, EffectInstigator, EffectCauser);
    }

    if (bHardCC && CcRes.bGrantImmunity) {
        FGameplayEffectContextHandle ImmuneCtx = TargetASC->MakeEffectContext();
        ImmuneCtx.AddInstigator(EffectInstigator, EffectCauser);
        FGameplayEffectSpecHandle ImmuneSpec = TargetASC->MakeOutgoingSpec(UMythicGE_CCImmune::StaticClass(), 1.0f, ImmuneCtx);
        if (ImmuneSpec.IsValid()) {
            ImmuneSpec.Data->SetSetByCallerMagnitude(GAS_SETBYCALLER_CCIMMUNE_DURATION, CcCfg.ImmuneSeconds);
            TargetASC->ApplyGameplayEffectSpecToSelf(*ImmuneSpec.Data.Get());
        }
    }
}

void UMythicAttributeSet_Defense::OnRep_Armor(const FGameplayAttributeData &OldArmor) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, Armor, OldArmor);
}

void UMythicAttributeSet_Defense::OnRep_Strength(const FGameplayAttributeData &OldStrength) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, Strength, OldStrength);
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

void UMythicAttributeSet_Defense::OnRep_WeakenResistance(const FGameplayAttributeData &OldWeakenResistance) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, WeakenResistance, OldWeakenResistance);
}

void UMythicAttributeSet_Defense::OnRep_TerrifyResistance(const FGameplayAttributeData &OldTerrifyResistance) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, TerrifyResistance, OldTerrifyResistance);
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

void UMythicAttributeSet_Defense::OnRep_WeakenBuildup(const FGameplayAttributeData &OldWeakenBuildup) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, WeakenBuildup, OldWeakenBuildup);
}

void UMythicAttributeSet_Defense::OnRep_TerrifyBuildup(const FGameplayAttributeData &OldTerrifyBuildup) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, TerrifyBuildup, OldTerrifyBuildup);
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
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, Strength, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, DodgeChance, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, BurnResistance, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, BleedResistance, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, PoisonResistance, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, SlowResistance, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, FreezeResistance, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, StunResistance, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, WeakenResistance, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, TerrifyResistance, COND_OwnerOnly, REPNOTIFY_Always);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, BurnBuildup, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, BleedBuildup, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, PoisonBuildup, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, SlowBuildup, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, FreezeBuildup, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, StunBuildup, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, WeakenBuildup, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, TerrifyBuildup, COND_OwnerOnly, REPNOTIFY_Always);

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
        {GetWeakenBuildupAttribute(), Def->GetWeakenBuildup()},
        {GetTerrifyBuildupAttribute(), Def->GetTerrifyBuildup()},
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
        ASC->SetNumericAttributeBase(GetWeakenBuildupAttribute(), 0.0f);
        ASC->SetNumericAttributeBase(GetTerrifyBuildupAttribute(), 0.0f);

        ASC->SetNumericAttributeBase(GetShieldAttribute(), 0.0f);
    }
}
