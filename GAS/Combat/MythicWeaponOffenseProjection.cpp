// Copyright Stellar Games. All Rights Reserved.

#include "GAS/Combat/MythicWeaponOffenseProjection.h"

#include "AbilitySystemComponent.h"
#include "GAS/Abilities/MythicWeaponAttackAbility.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/MythicStatContribution.h"
#include "GAS/MythicStatDiminishing.h"
#include "Itemization/Inventory/Fragments/Actionable/AttackFragment.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/MythicTags_Inventory.h"
#include "Settings/MythicCombatSettings.h"

namespace {
bool ResolveWeaponClassBonusAttribute(
    const FGameplayTagContainer &SourceTags,
    FGameplayTag &OutWeaponClassTag,
    FGameplayAttribute &OutBonusAttribute) {
    using Offense = UMythicAttributeSet_Offense;
    OutWeaponClassTag = FGameplayTag();
    OutBonusAttribute = FGameplayAttribute();

    if (!SourceTags.HasTag(ITEMIZATION_TYPE_EQUIPMENT_WEAPON)) {
        return true;
    }

    struct FWeaponClassBinding {
        FGameplayTag Tag;
        FGameplayAttribute BonusAttribute;
    };
    const FWeaponClassBinding Bindings[] = {
        {ITEMIZATION_TYPE_EQUIPMENT_WEAPON_SWORD, Offense::GetBonusSwordDamageAttribute()},
        {ITEMIZATION_TYPE_EQUIPMENT_WEAPON_AXE, Offense::GetBonusAxeDamageAttribute()},
        {ITEMIZATION_TYPE_EQUIPMENT_WEAPON_DAGGERS, Offense::GetBonusDaggerDamageAttribute()},
        {ITEMIZATION_TYPE_EQUIPMENT_WEAPON_SICKLE, Offense::GetBonusSickleDamageAttribute()},
        {ITEMIZATION_TYPE_EQUIPMENT_WEAPON_SPEAR, Offense::GetBonusSpearDamageAttribute()},
        {ITEMIZATION_TYPE_EQUIPMENT_WEAPON_HAMMER, Offense::GetBonusHammerDamageAttribute()},
    };

    int32 MatchCount = 0;
    for (const FWeaponClassBinding &Binding : Bindings) {
        if (!SourceTags.HasTagExact(Binding.Tag)) {
            continue;
        }
        ++MatchCount;
        OutWeaponClassTag = Binding.Tag;
        OutBonusAttribute = Binding.BonusAttribute;
    }
    if (MatchCount != 1) {
        OutWeaponClassTag = FGameplayTag();
        OutBonusAttribute = FGameplayAttribute();
        return false;
    }
    return true;
}
}

namespace MythicCombat {
bool ResolveWeaponDamageProjection(
    const float DamagePerHit,
    const FGameplayTagContainer &AttackSourceTags,
    TFunctionRef<float(const FGameplayAttribute &)> ReadSourceStat,
    FMythicWeaponDamageProjection &OutProjection) {
    OutProjection = FMythicWeaponDamageProjection();

    FMythicWeaponDamageProjection Candidate;
    if (!ResolveWeaponDamageRange(
            DamagePerHit,
            Candidate.BaseMinimumDamage,
            Candidate.BaseMaximumDamage,
            Candidate.BaseAverageDamage)) {
        return false;
    }

    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    if (!Settings) {
        return false;
    }

    bool bReadInvalidSourceStat = false;
    const auto ReadFiniteSourceStat =
        [&ReadSourceStat, &bReadInvalidSourceStat](const FGameplayAttribute &Attribute) {
            const float Value = ReadSourceStat(Attribute);
            if (!FMath::IsFinite(Value)) {
                bReadInvalidSourceStat = true;
                return 0.0f;
            }
            return Value;
        };

    Candidate.PrimaryStatBonusFraction = FMythicStatContributionRules::ResolveTarget(
        Settings->StatContributions.Contributions,
        UMythicAttributeSet_Offense::GetDamagePerHitAttribute(),
        ReadFiniteSourceStat);
    const float PrimaryFactor = 1.0f + Candidate.PrimaryStatBonusFraction;
    if (bReadInvalidSourceStat || !FMath::IsFinite(PrimaryFactor) || PrimaryFactor < 0.0f) {
        return false;
    }

    Candidate.PrimaryAdjustedMinimumDamage = Candidate.BaseMinimumDamage * PrimaryFactor;
    Candidate.PrimaryAdjustedMaximumDamage = Candidate.BaseMaximumDamage * PrimaryFactor;
    Candidate.PrimaryAdjustedAverageDamage = Candidate.BaseAverageDamage * PrimaryFactor;

    FGameplayAttribute WeaponBonusAttribute;
    if (!ResolveWeaponClassBonusAttribute(
            AttackSourceTags,
            Candidate.WeaponClassTag,
            WeaponBonusAttribute)) {
        return false;
    }
    if (WeaponBonusAttribute.IsValid()) {
        const float RawWeaponClassBonus = ReadFiniteSourceStat(WeaponBonusAttribute);
        Candidate.WeaponClassBonusMultiplier = FMythicStatDiminishingRules::ApplyToBonus(
            Settings->StatDiminishing, WeaponBonusAttribute, RawWeaponClassBonus);
    }
    if (bReadInvalidSourceStat
        || !FMath::IsFinite(Candidate.WeaponClassBonusMultiplier)
        || Candidate.WeaponClassBonusMultiplier < 0.0f) {
        return false;
    }

    Candidate.EffectiveMinimumDamage =
        Candidate.PrimaryAdjustedMinimumDamage * Candidate.WeaponClassBonusMultiplier;
    Candidate.EffectiveMaximumDamage =
        Candidate.PrimaryAdjustedMaximumDamage * Candidate.WeaponClassBonusMultiplier;
    Candidate.EffectiveAverageDamage =
        Candidate.PrimaryAdjustedAverageDamage * Candidate.WeaponClassBonusMultiplier;
    if (!FMath::IsFinite(Candidate.PrimaryAdjustedMinimumDamage)
        || !FMath::IsFinite(Candidate.PrimaryAdjustedMaximumDamage)
        || !FMath::IsFinite(Candidate.PrimaryAdjustedAverageDamage)
        || !FMath::IsFinite(Candidate.EffectiveMinimumDamage)
        || !FMath::IsFinite(Candidate.EffectiveMaximumDamage)
        || !FMath::IsFinite(Candidate.EffectiveAverageDamage)) {
        return false;
    }

    OutProjection = Candidate;
    return true;
}

bool ResolveWeaponTypeTags(
    const UAttackFragment *AttackFragment,
    FGameplayTagContainer &OutWeaponTypeTags) {
    OutWeaponTypeTags.Reset();
    const UMythicItemInstance *Item = AttackFragment
        ? AttackFragment->GetOwningItemInstance() : nullptr;
    if (!IsValid(Item)) {
        return false;
    }

    FGameplayTagContainer TypeProbe;
    Item->GetTypeProbe(TypeProbe);
    OutWeaponTypeTags = TypeProbe.Filter(
        FGameplayTagContainer(ITEMIZATION_TYPE_EQUIPMENT_WEAPON));

    FGameplayTag WeaponClassTag;
    FGameplayAttribute BonusAttribute;
    if (!ResolveWeaponClassBonusAttribute(
            OutWeaponTypeTags,
            WeaponClassTag,
            BonusAttribute)
        || !WeaponClassTag.IsValid()
        || !BonusAttribute.IsValid()) {
        OutWeaponTypeTags.Reset();
        return false;
    }
    return true;
}

bool ResolveActiveWeaponTypeTags(
    const UAbilitySystemComponent *AbilitySystem,
    FGameplayTagContainer &OutWeaponTypeTags) {
    OutWeaponTypeTags.Reset();
    if (!AbilitySystem) {
        return false;
    }

    const UAttackFragment *ResolvedWeapon = nullptr;
    for (const FGameplayAbilitySpec &Spec : AbilitySystem->GetActivatableAbilities()) {
        if (Spec.PendingRemove || !Cast<UMythicWeaponAttackAbility>(Spec.Ability)) {
            continue;
        }
        const UAttackFragment *AttackFragment = Cast<UAttackFragment>(Spec.SourceObject.Get());
        if (!AttackFragment
            || UMythicWeaponAttackAbility::ResolveAttackSourceDomain(AttackFragment)
                != EMythicAttackSourceDomain::Weapon) {
            continue;
        }
        if (ResolvedWeapon && ResolvedWeapon != AttackFragment) {
            OutWeaponTypeTags.Reset();
            return false;
        }
        ResolvedWeapon = AttackFragment;
    }

    return ResolveWeaponTypeTags(ResolvedWeapon, OutWeaponTypeTags);
}

bool BuildWeaponDamageProjection(
    const UAbilitySystemComponent *AbilitySystem,
    FMythicWeaponDamageProjection &OutProjection) {
    OutProjection = FMythicWeaponDamageProjection();
    const FGameplayAttribute DamageAttribute =
        UMythicAttributeSet_Offense::GetDamagePerHitAttribute();
    if (!AbilitySystem
        || !AbilitySystem->HasAttributeSetForAttribute(DamageAttribute)) {
        return false;
    }

    FGameplayTagContainer WeaponTypeTags;
    if (!ResolveActiveWeaponTypeTags(AbilitySystem, WeaponTypeTags)) {
        return false;
    }
    return ResolveWeaponDamageProjection(
        AbilitySystem->GetNumericAttribute(DamageAttribute),
        WeaponTypeTags,
        [AbilitySystem](const FGameplayAttribute &Attribute) -> float {
            return Attribute.IsValid()
                && AbilitySystem->HasAttributeSetForAttribute(Attribute)
                ? AbilitySystem->GetNumericAttribute(Attribute) : 0.0f;
        },
        OutProjection);
}
}
