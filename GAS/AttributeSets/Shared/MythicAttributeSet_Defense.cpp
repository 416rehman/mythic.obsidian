// 

#include "MythicAttributeSet_Defense.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h" // avatar pawn -> owning player controller
#include "Player/MythicPlayerController.h" // ClientShowShieldAbsorbed (shield combat feedback RPC)

void UMythicAttributeSet_Defense::PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue) {
    Super::PreAttributeChange(Attribute, NewValue);

    if (Attribute == GetShieldAttribute()) {
        ShieldBeforeChange = GetShield(); // cache the pre-change value for the EXACT absorbed-amount feedback
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
        // Probabilities / damage-reduction fractions: capped at [0,1] so stacked buffs can't make a target permanently
        // un-hittable / fully immune, and a NEGATIVE reduction can't invert into a damage amplifier.
        NewValue = FMath::Clamp(NewValue, 0.0f, 1.0f);
    }
    else if (Attribute == GetArmorAttribute()) {
        NewValue = FMath::Max(0.0f, NewValue);
    }
}

void UMythicAttributeSet_Defense::PostGameplayEffectExecute(const FGameplayEffectModCallbackData &Data) {
    Super::PostGameplayEffectExecute(Data);

    // Damage routes to Shield as a negative Additive modifier (clamped by PreAttributeChange to >= 0).
    // When MaxShield is reduced, re-clamp current Shield down to the new maximum.
    if (Data.EvaluatedData.Attribute == GetMaxShieldAttribute() && GetShield() > GetMaxShield()) {
        SetShield(GetMaxShield());
    }
    // Player-facing shield feedback (Rule-4 combat) — fired ONLY from the authority's real-damage path. The MaxShield
    // re-clamp above is the `if` branch, so a clamp-driven Shield drop (debuff expiry, gear unequip) never reaches here;
    // that means we never infer "damage vs clamp" from a raw attribute delta — the prior OnRep approach could not, and
    // floated a spurious "absorbed N" on clients (batch 147-150 review MEDIUM). The server knows it's real damage, so it
    // fires a Client RPC to the owning player (the established callout idiom; covers standalone/listen-host/dedicated).
    else if (Data.EvaluatedData.Attribute == GetShieldAttribute()) {
        const float Absorbed = ShieldBeforeChange - GetShield();
        if (Absorbed > 0.0f) { // a DECREASE = damage the shield ate (regen / initial replication INCREASE → skip)
            const UAbilitySystemComponent *ASC = GetOwningAbilitySystemComponent();
            const APawn *Avatar = ASC ? Cast<APawn>(ASC->GetAvatarActor()) : nullptr;
            if (AMythicPlayerController *PC = Avatar ? Cast<AMythicPlayerController>(Avatar->GetController()) : nullptr) {
                const bool bBroke = (GetShield() <= 0.0f && ShieldBeforeChange > 0.0f);
                PC->ClientShowShieldAbsorbed(FMath::RoundToInt(Absorbed), bBroke);
            }
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

void UMythicAttributeSet_Defense::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, Armor, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, DodgeChance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, BurnResistance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, BleedResistance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, PoisonResistance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, SlowResistance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, FreezeResistance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, StunResistance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, DecreasedDamageFromEnemiesUnderStatusEffects, COND_None,
                                   REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, HealthRegenRate, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, MaxShield, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, Shield, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, ShieldRegenRate, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, LifePerHit, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, LifePerKill, COND_None, REPNOTIFY_Always);
}
