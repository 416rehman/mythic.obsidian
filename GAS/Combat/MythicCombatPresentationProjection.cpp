#include "GAS/Combat/MythicCombatPresentationProjection.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AI/MythicTags_AI.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "GAS/Combat/MythicEntityCombatPresentationComponent.h"
#include "GAS/Combat/MythicWeaponOffenseProjection.h"
#include "GAS/Abilities/MythicWeaponAttackAbility.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Proficiencies.h"
#include "GAS/Executions/MythicCombatRoll.h"
#include "GAS/MythicStatDiminishing.h"
#include "GAS/MythicTags_GAS.h"
#include "GameModes/GameState/MythicGameState.h"
#include "Itemization/Inventory/Fragments/Actionable/AttackFragment.h"
#include "Settings/MythicCombatSettings.h"

namespace {
float ReadCombatAttribute(const UAbilitySystemComponent *AbilitySystem,
                          const FGameplayAttribute &Attribute,
                          const float Fallback = 0.0f) {
    return AbilitySystem && Attribute.IsValid()
        && AbilitySystem->HasAttributeSetForAttribute(Attribute)
        ? AbilitySystem->GetNumericAttribute(Attribute) : Fallback;
}

bool IsCombatCapable(AActor *Actor,
                     const UAbilitySystemComponent *AbilitySystem) {
    if (const AMythicNPCCharacter *NPC = Cast<AMythicNPCCharacter>(Actor)) {
        return NPC->GetAttackAbility() != nullptr;
    }
    if (const APawn *Pawn = Cast<APawn>(Actor);
        Pawn && Pawn->IsPlayerControlled()) {
        return true;
    }
    return AbilitySystem
        && AbilitySystem->HasAttributeSetForAttribute(
            UMythicAttributeSet_Life::GetMaxHealthAttribute())
        && AbilitySystem->HasAttributeSetForAttribute(
            UMythicAttributeSet_Offense::GetDamagePerHitAttribute())
        && ReadCombatAttribute(
            AbilitySystem,
            UMythicAttributeSet_Life::GetMaxHealthAttribute()) > 0.0f
        && ReadCombatAttribute(
            AbilitySystem,
            UMythicAttributeSet_Offense::GetDamagePerHitAttribute()) > 0.0f;
}

float ResolveBasicAttacksPerSecond(
    UAbilitySystemComponent *AbilitySystem,
    const UMythicCombatSettings *Settings) {
    const float Fallback = Settings
        && FMath::IsFinite(Settings->CombatRatingFallbackAttacksPerSecond)
        ? FMath::Clamp(Settings->CombatRatingFallbackAttacksPerSecond,
                       0.01f, 20.0f)
        : 1.0f;
    if (!AbilitySystem) {
        return Fallback;
    }

    float BestCanonicalWeaponCadence = 0.0f;
    for (const FGameplayAbilitySpec &Spec :
         AbilitySystem->GetActivatableAbilities()) {
        if (!Cast<UMythicWeaponAttackAbility>(Spec.Ability)) {
            continue;
        }
        const UAttackFragment *AttackFragment =
            Cast<UAttackFragment>(Spec.SourceObject.Get());
        if (!AttackFragment
            || UMythicWeaponAttackAbility::ResolveAttackSourceDomain(
                   AttackFragment) != EMythicAttackSourceDomain::Weapon) {
            continue;
        }
        const float CycleSeconds =
            AttackFragment->GetRuntimeNominalAttackCycleDuration();
        if (FMath::IsFinite(CycleSeconds)
            && CycleSeconds > UE_SMALL_NUMBER) {
            BestCanonicalWeaponCadence = FMath::Max(
                BestCanonicalWeaponCadence, 1.0f / CycleSeconds);
        }
    }
    return BestCanonicalWeaponCadence > 0.0f
        ? BestCanonicalWeaponCadence : Fallback;
}
}

UAbilitySystemComponent *
FMythicCombatPresentationProjectionRules::ResolveAbilitySystem(AActor *Actor) {
    const IAbilitySystemInterface *AbilitySystemOwner =
        Cast<IAbilitySystemInterface>(Actor);
    return AbilitySystemOwner
        ? AbilitySystemOwner->GetAbilitySystemComponent() : nullptr;
}

EMythicPresentedCombatRank
FMythicCombatPresentationProjectionRules::ResolveAuthorityNpcPresentedCombatRank(
    const AMythicNPCCharacter *NPC) {
    if (!IsValid(NPC) || !NPC->HasAuthority()) {
        return EMythicPresentedCombatRank::Unknown;
    }

    const FGameplayTag EnemyTier = NPC->GetEnemyTier();
    if (EnemyTier.MatchesTagExact(AI_TIER_NORMAL)) {
        return EMythicPresentedCombatRank::Standard;
    }
    if (EnemyTier.MatchesTagExact(AI_TIER_SUPERIOR)) {
        return EMythicPresentedCombatRank::Superior;
    }
    if (EnemyTier.MatchesTagExact(AI_TIER_ELITE)) {
        return EMythicPresentedCombatRank::Elite;
    }
    if (EnemyTier.MatchesTagExact(AI_TIER_CHAMPION)) {
        return EMythicPresentedCombatRank::Champion;
    }
    if (EnemyTier.MatchesTagExact(AI_TIER_BOSS)) {
        return EMythicPresentedCombatRank::Boss;
    }
    return EMythicPresentedCombatRank::Unknown;
}

bool FMythicCombatPresentationProjectionRules::HasPublicCombatCommitment(
    const AActor *SubjectActor,
    const UAbilitySystemComponent *SubjectAbilitySystem,
    const APawn *ViewerPawn) {
    if (!IsValid(SubjectActor) || !IsValid(ViewerPawn)
        || SubjectActor == ViewerPawn) {
        return false;
    }
    if (SubjectAbilitySystem
        && SubjectAbilitySystem->HasMatchingGameplayTag(GAS_STATE_INCOMBAT)) {
        return true;
    }
    const AMythicNPCCharacter *NPC = Cast<AMythicNPCCharacter>(SubjectActor);
    return NPC && NPC->HasAuthority() && NPC->GetEngagedTarget() == ViewerPawn;
}

FMythicCombatPressureSnapshot
FMythicCombatPresentationProjectionRules::BuildAuthorityPressureSnapshot(
    const UObject *WorldContext, AActor *Actor,
    UAbilitySystemComponent *AbilitySystem) {
    FMythicCombatPressureSnapshot Snapshot;
    Snapshot.bCombatCapable = IsCombatCapable(Actor, AbilitySystem);
    if (!AbilitySystem) {
        return Snapshot;
    }

    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    const float DamagePerHit = ReadCombatAttribute(
        AbilitySystem,
        UMythicAttributeSet_Offense::GetDamagePerHitAttribute());
    FGameplayTagContainer WeaponTypeTags;
    const bool bCanProjectWeaponDamage = Cast<AMythicNPCCharacter>(Actor)
        || MythicCombat::ResolveActiveWeaponTypeTags(
            AbilitySystem, WeaponTypeTags);
    FMythicWeaponDamageProjection WeaponDamage;
    if (bCanProjectWeaponDamage
        && MythicCombat::ResolveWeaponDamageProjection(
            DamagePerHit,
            WeaponTypeTags,
            [AbilitySystem](const FGameplayAttribute &Attribute) {
                return ReadCombatAttribute(AbilitySystem, Attribute);
            },
            WeaponDamage)) {
        Snapshot.ExpectedDamagePerHit = WeaponDamage.EffectiveAverageDamage;
    }

    const float AttackSpeedBonus = ReadCombatAttribute(
        AbilitySystem,
        UMythicAttributeSet_Offense::GetAttackSpeedAttribute());
    const float MinimumPlayRate = Settings
        ? FMath::Max(0.01f, Settings->MinAttackSpeedPlayRate) : 0.80f;
    const float MaximumPlayRate = Settings
        ? FMath::Max(MinimumPlayRate, Settings->MaxAttackSpeedPlayRate)
        : 1.40f;
    const float PlayRate = FMath::Clamp(
        1.0f + AttackSpeedBonus, MinimumPlayRate, MaximumPlayRate);
    Snapshot.AttacksPerSecond =
        ResolveBasicAttacksPerSecond(AbilitySystem, Settings) * PlayRate;
    const float RawCriticalHitChance = ReadCombatAttribute(
        AbilitySystem,
        UMythicAttributeSet_Offense::GetCriticalHitChanceAttribute());
    Snapshot.CriticalHitChance = Settings
        ? MythicCombat::DiminishProbability(
              RawCriticalHitChance, Settings->ProbabilitySoftCap)
        : FMath::Clamp(RawCriticalHitChance, 0.0f, 1.0f);
    const float RawCriticalMultiplier = ReadCombatAttribute(
        AbilitySystem,
        UMythicAttributeSet_Offense::GetCriticalHitDamageAttribute(), 1.0f);
    Snapshot.CriticalDamageMultiplier = Settings
        ? FMath::Max(1.0f, FMythicStatDiminishingRules::ApplyToBonus(
              Settings->StatDiminishing,
              UMythicAttributeSet_Offense::GetCriticalHitDamageAttribute(),
              RawCriticalMultiplier))
        : FMath::Max(1.0f, RawCriticalMultiplier);
    Snapshot.OutgoingDamageMultiplier = FMath::Max(
        0.0f, ReadCombatAttribute(
            AbilitySystem,
            UMythicAttributeSet_Offense::GetOutgoingDamageMultiplierAttribute(),
            1.0f));
    Snapshot.MaximumHealth = ReadCombatAttribute(
        AbilitySystem,
        UMythicAttributeSet_Life::GetMaxHealthAttribute());
    Snapshot.MaximumShield = ReadCombatAttribute(
        AbilitySystem,
        UMythicAttributeSet_Defense::GetMaxShieldAttribute());
    Snapshot.ArmorMitigationFraction =
        AMythicGameState::EvaluateArmorMitigation(
            WorldContext, ReadCombatAttribute(
                AbilitySystem,
                UMythicAttributeSet_Defense::GetArmorAttribute()));
    Snapshot.DodgeChance = ReadCombatAttribute(
        AbilitySystem,
        UMythicAttributeSet_Defense::GetDodgeChanceAttribute());
    if (Settings) {
        Snapshot.DodgeChance = FMath::Clamp(
            Snapshot.DodgeChance, 0.0f,
            FMath::Clamp(Settings->MaxDodgeChance, 0.0f, 0.95f));
    }
    return Snapshot;
}

int32 FMythicCombatPresentationProjectionRules::ResolveAuthorityCombatLevel(
    AActor *Actor, const UAbilitySystemComponent *AbilitySystem) {
    if (const AMythicNPCCharacter *NPC = Cast<AMythicNPCCharacter>(Actor)) {
        return FMath::Max(1, NPC->GetNPCDataRef().CombatLevel);
    }
    bool bFoundLevel = false;
    const int32 Level = UMythicAttributeSet_Proficiencies::GetLevel(
        AbilitySystem, bFoundLevel);
    return bFoundLevel ? FMath::Max(1, Level) : 0;
}

bool FMythicCombatPresentationProjectionPolicy::IsValid() const {
    return FMath::IsFinite(MaximumFocusRangeCentimeters)
        && MaximumFocusRangeCentimeters >= 100.0f
        && MaximumFocusRangeCentimeters <= 20000.0f
        && FMath::IsFinite(MinimumFocusViewDot)
        && MinimumFocusViewDot >= -1.0f && MinimumFocusViewDot <= 1.0f
        && LineOfSightTraceChannel >= ECC_WorldStatic
        && LineOfSightTraceChannel < ECC_MAX
        && FMath::IsFinite(MinimumClientRequestIntervalSeconds)
        && MinimumClientRequestIntervalSeconds >= 0.02f
        && MinimumClientRequestIntervalSeconds <= 0.50f
        && FMath::IsFinite(AuthorityRefreshIntervalSeconds)
        && AuthorityRefreshIntervalSeconds >= 0.10f
        && AuthorityRefreshIntervalSeconds <= 2.0f
        && FMath::IsFinite(PresentationLeaseDurationSeconds)
        && PresentationLeaseDurationSeconds > AuthorityRefreshIntervalSeconds
        && PresentationLeaseDurationSeconds <= 10.0f;
}

double FMythicCombatPresentationProjectionRules::ComputeEffectivePressure(
    const FMythicCombatPressureSnapshot &Snapshot) {
    const float Values[] = {
        Snapshot.ExpectedDamagePerHit,
        Snapshot.AttacksPerSecond,
        Snapshot.CriticalHitChance,
        Snapshot.CriticalDamageMultiplier,
        Snapshot.OutgoingDamageMultiplier,
        Snapshot.MaximumHealth,
        Snapshot.MaximumShield,
        Snapshot.ArmorMitigationFraction,
        Snapshot.DodgeChance,
    };
    for (const float Value : Values) {
        if (!FMath::IsFinite(Value)) {
            return 0.0f;
        }
    }
    if (!Snapshot.bCombatCapable || Snapshot.ExpectedDamagePerHit <= 0.0f
        || Snapshot.AttacksPerSecond <= 0.0f
        || Snapshot.OutgoingDamageMultiplier <= 0.0f) {
        return 0.0f;
    }

    const double Capacity = static_cast<double>(FMath::Max(0.0f, Snapshot.MaximumHealth))
        + static_cast<double>(FMath::Max(0.0f, Snapshot.MaximumShield));
    if (Capacity <= UE_SMALL_NUMBER) {
        return 0.0f;
    }

    const double CriticalChance = static_cast<double>(FMath::Clamp(Snapshot.CriticalHitChance, 0.0f, 1.0f));
    const double CriticalMultiplier = static_cast<double>(FMath::Max(1.0f, Snapshot.CriticalDamageMultiplier));
    const double ExpectedCriticalScale = 1.0 + CriticalChance * (CriticalMultiplier - 1.0);
    const double SustainedOffense = static_cast<double>(Snapshot.ExpectedDamagePerHit)
        * static_cast<double>(Snapshot.AttacksPerSecond)
        * ExpectedCriticalScale
        * static_cast<double>(Snapshot.OutgoingDamageMultiplier);

    const double ArmorPassThrough = 1.0 - static_cast<double>(FMath::Clamp(Snapshot.ArmorMitigationFraction, 0.0f, 0.95f));
    const double DodgePassThrough = 1.0 - static_cast<double>(FMath::Clamp(Snapshot.DodgeChance, 0.0f, 0.95f));
    const double EffectiveSurvivability = Capacity / FMath::Max(ArmorPassThrough * DodgePassThrough, 0.01);
    const double Pressure = SustainedOffense * EffectiveSurvivability;
    if (!FMath::IsFinite(Pressure) || Pressure <= 0.0) {
        return 0.0f;
    }
    return Pressure;
}

bool FMythicCombatPresentationProjectionRules::IsSpatiallyEligible(
    const float DistanceSquared, const float ViewDot, const bool bHasLineOfSight,
    const FMythicCombatPresentationProjectionPolicy &Policy) {
    return Policy.IsValid() && bHasLineOfSight
        && FMath::IsFinite(DistanceSquared) && DistanceSquared >= 0.0f
        && DistanceSquared <= FMath::Square(Policy.MaximumFocusRangeCentimeters)
        && FMath::IsFinite(ViewDot) && ViewDot >= Policy.MinimumFocusViewDot;
}

double FMythicCombatPresentationProjectionRules::GetRequestThrottleDelaySeconds(
    const double NowSeconds, const double LastAcceptedSeconds,
    const float MinimumIntervalSeconds) {
    if (!FMath::IsFinite(NowSeconds) || !FMath::IsFinite(LastAcceptedSeconds)
        || !FMath::IsFinite(MinimumIntervalSeconds)
        || MinimumIntervalSeconds < 0.02f || MinimumIntervalSeconds > 0.50f) {
        return TNumericLimits<double>::Max();
    }
    if (LastAcceptedSeconds == -DBL_MAX) {
        return 0.0;
    }
    return FMath::Max(0.0, static_cast<double>(MinimumIntervalSeconds)
        - FMath::Max(0.0, NowSeconds - LastAcceptedSeconds));
}

uint32 FMythicCombatPresentationProjectionRules::AdvanceNonzeroRevision(
    const uint32 CurrentRevision) {
    const uint32 Next = CurrentRevision + 1u;
    return Next == 0u ? 1u : Next;
}
