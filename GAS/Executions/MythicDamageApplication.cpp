// 


#include "MythicDamageApplication.h"

#include "Mythic.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "GAS/MythicTags_GAS.h"
#include "AI/MythicTags_AI.h"
#include "Itemization/MythicTags_Inventory.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/Executions/MythicCombatRoll.h"
#include "GameModes/GameState/MythicGameState.h"
#include "Curves/RealCurve.h"
#include "Engine/World.h"
#include "Player/MythicPlayerController.h" // ClientShowDodge callout (dodge negates before the damage cue)
#include "GameFramework/Pawn.h"
#include "Settings/MythicDeveloperSettings.h" // friendly-fire policy (bFriendlyFireEnabled)

struct FMythicGameplayEffectContext;

bool UMythicDamageApplication::ShouldNegateFriendlyFire(bool bSourceIsPlayer, bool bTargetIsPlayer, bool bSameActor, bool bFriendlyFireEnabled) {
    return bSourceIsPlayer && bTargetIsPlayer && !bSameActor && !bFriendlyFireEnabled;
}

struct FDamageApplicationStatics {
    FGameplayEffectAttributeCaptureDefinition Power;
    FGameplayEffectAttributeCaptureDefinition DamagePerHit;
    FGameplayEffectAttributeCaptureDefinition CriticalHitDamage;
    FGameplayEffectAttributeCaptureDefinition BonusSkillDamage;
    FGameplayEffectAttributeCaptureDefinition BonusSwordDamage;
    FGameplayEffectAttributeCaptureDefinition BonusAxeDamage;
    FGameplayEffectAttributeCaptureDefinition BonusDaggerDamage;
    FGameplayEffectAttributeCaptureDefinition BonusSickleDamage;
    FGameplayEffectAttributeCaptureDefinition BonusSpearDamage;
    FGameplayEffectAttributeCaptureDefinition BonusHammerDamage;
    FGameplayEffectAttributeCaptureDefinition IncreasedDamageToEnemiesUnderStatusEffects;
    FGameplayEffectAttributeCaptureDefinition BonusDamageToSuperiorEnemies;
    FGameplayEffectAttributeCaptureDefinition OutgoingDamageMultiplier;

    // target defensive captures
    FGameplayEffectAttributeCaptureDefinition Armor;
    FGameplayEffectAttributeCaptureDefinition Shield;
    FGameplayEffectAttributeCaptureDefinition DecreasedDamageFromEnemiesUnderStatusEffects;
    FGameplayEffectAttributeCaptureDefinition DodgeChance;
    FGameplayEffectAttributeCaptureDefinition BurnResistance;
    FGameplayEffectAttributeCaptureDefinition BleedResistance;
    FGameplayEffectAttributeCaptureDefinition PoisonResistance;
    FGameplayEffectAttributeCaptureDefinition SlowResistance;
    FGameplayEffectAttributeCaptureDefinition FreezeResistance;
    FGameplayEffectAttributeCaptureDefinition StunResistance;
    FGameplayEffectAttributeCaptureDefinition IncomingDamageMultiplier;

    FDamageApplicationStatics() {
        Power = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetPowerAttribute(), EGameplayEffectAttributeCaptureSource::Source,
                                                          false);
        DamagePerHit = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetDamagePerHitAttribute(),
                                                                 EGameplayEffectAttributeCaptureSource::Source, false);
        CriticalHitDamage = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetCriticalHitDamageAttribute(),
                                                                      EGameplayEffectAttributeCaptureSource::Source, false);
        BonusSkillDamage = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetBonusSkillDamageAttribute(),
                                                                     EGameplayEffectAttributeCaptureSource::Source, false);
        BonusSwordDamage = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetBonusSwordDamageAttribute(),
                                                                     EGameplayEffectAttributeCaptureSource::Source, false);
        BonusAxeDamage = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetBonusAxeDamageAttribute(),
                                                                   EGameplayEffectAttributeCaptureSource::Source, false);
        BonusDaggerDamage = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetBonusDaggerDamageAttribute(),
                                                                      EGameplayEffectAttributeCaptureSource::Source, false);
        BonusSickleDamage = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetBonusSickleDamageAttribute(),
                                                                      EGameplayEffectAttributeCaptureSource::Source, false);
        BonusSpearDamage = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetBonusSpearDamageAttribute(),
                                                                     EGameplayEffectAttributeCaptureSource::Source, false);
        BonusHammerDamage = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetBonusHammerDamageAttribute(),
                                                                      EGameplayEffectAttributeCaptureSource::Source, false);
        IncreasedDamageToEnemiesUnderStatusEffects = FGameplayEffectAttributeCaptureDefinition(
            UMythicAttributeSet_Offense::GetIncreasedDamageToEnemiesUnderStatusEffectsAttribute(), EGameplayEffectAttributeCaptureSource::Source, false);
        BonusDamageToSuperiorEnemies = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetBonusDamageToSuperiorEnemiesAttribute(),
                                                                                 EGameplayEffectAttributeCaptureSource::Source, false);
        OutgoingDamageMultiplier = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetOutgoingDamageMultiplierAttribute(),
                                                                             EGameplayEffectAttributeCaptureSource::Source, false);

        Armor = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Defense::GetArmorAttribute(), EGameplayEffectAttributeCaptureSource::Target,
                                                          false);
        Shield = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Defense::GetShieldAttribute(), EGameplayEffectAttributeCaptureSource::Target,
                                                           false);
        DecreasedDamageFromEnemiesUnderStatusEffects = FGameplayEffectAttributeCaptureDefinition(
            UMythicAttributeSet_Defense::GetDecreasedDamageFromEnemiesUnderStatusEffectsAttribute(), EGameplayEffectAttributeCaptureSource::Target, false);
        DodgeChance = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Defense::GetDodgeChanceAttribute(),
                                                                EGameplayEffectAttributeCaptureSource::Target, false);
        BurnResistance = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Defense::GetBurnResistanceAttribute(),
                                                                   EGameplayEffectAttributeCaptureSource::Target, false);
        BleedResistance = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Defense::GetBleedResistanceAttribute(),
                                                                    EGameplayEffectAttributeCaptureSource::Target, false);
        PoisonResistance = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Defense::GetPoisonResistanceAttribute(),
                                                                     EGameplayEffectAttributeCaptureSource::Target, false);
        SlowResistance = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Defense::GetSlowResistanceAttribute(),
                                                                   EGameplayEffectAttributeCaptureSource::Target, false);
        FreezeResistance = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Defense::GetFreezeResistanceAttribute(),
                                                                     EGameplayEffectAttributeCaptureSource::Target, false);
        StunResistance = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Defense::GetStunResistanceAttribute(),
                                                                   EGameplayEffectAttributeCaptureSource::Target, false);
        IncomingDamageMultiplier = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Defense::GetIncomingDamageMultiplierAttribute(),
                                                                             EGameplayEffectAttributeCaptureSource::Target, false);
    }
};

static FDamageApplicationStatics &MythicDamageApplicationStatics() {
    static FDamageApplicationStatics Statics;
    return Statics;
}

UMythicDamageApplication::UMythicDamageApplication() {
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().Power);
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().DamagePerHit);
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().CriticalHitDamage);
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().BonusSkillDamage);
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().BonusSwordDamage);
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().BonusAxeDamage);
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().BonusDaggerDamage);
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().BonusSickleDamage);
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().BonusSpearDamage);
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().BonusHammerDamage);
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().IncreasedDamageToEnemiesUnderStatusEffects);
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().BonusDamageToSuperiorEnemies);
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().OutgoingDamageMultiplier);
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().Armor);
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().Shield);
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().DecreasedDamageFromEnemiesUnderStatusEffects);
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().DodgeChance);
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().BurnResistance);
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().BleedResistance);
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().PoisonResistance);
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().SlowResistance);
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().FreezeResistance);
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().StunResistance);
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().IncomingDamageMultiplier);
}

void UMythicDamageApplication::Execute_Implementation(const FGameplayEffectCustomExecutionParameters &ExecutionParams,
                                                      FGameplayEffectCustomExecutionOutput &OutExecutionOutput) const {
    Super::Execute_Implementation(ExecutionParams, OutExecutionOutput);
    UE_LOG(Myth, Warning, TEXT("DamageApplication:: Applying damage"));

    FGameplayEffectSpec *Spec = ExecutionParams.GetOwningSpecForPreExecuteMod();
    FMythicGameplayEffectContext *MythicContext = FMythicGameplayEffectContext::ExtractEffectContext(Spec->GetContext());
    if (!MythicContext) {
        // Non-Mythic / empty effect context (e.g. a hand-rolled spec supplying a foreign context subclass). Abort the
        // execution — mirrors the INVINCIBLE/DODGE early-returns (no output modifiers added = no damage applied) so a
        // mis-wired GE no-ops instead of dereferencing a bad pointer. ExtractEffectContext is the checked single source.
        UE_LOG(Myth, Error, TEXT("DamageApplication:: non-Mythic/empty effect context - aborting damage execution"));
        return;
    }

    // Send event with tag BeforeDamageDealt
    auto SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
    auto TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();

    // ===== Friendly fire (co-op policy, server-authoritative) =====
    // A combat hit from one player onto ANOTHER player is negated unless friendly fire is enabled (default OFF — the
    // standard co-op default). Mirrors the Invincible/Dodge early-returns (no output modifiers = no damage/shield/
    // status). PvE is provably unaffected (the gate needs BOTH source+target player-controlled), and SELF-damage
    // (fall damage, env hazards, self-DoT — source == target) is excluded, so only a distinct player→player hit is
    // gated. Policy lives on the consolidated dev settings. (Co-op behaviour is replication/2-client-UNVERIFIED.)
    {
        const AActor *SourceActor = SourceASC ? SourceASC->GetAvatarActor() : nullptr;
        const AActor *TargetActor = TargetASC ? TargetASC->GetAvatarActor() : nullptr;
        const APawn *SourcePawn = Cast<APawn>(SourceActor);
        const APawn *TargetPawn = Cast<APawn>(TargetActor);
        const bool bSourceIsPlayer = SourcePawn && SourcePawn->IsPlayerControlled();
        const bool bTargetIsPlayer = TargetPawn && TargetPawn->IsPlayerControlled();
        const bool bFriendlyFire = GetDefault<UMythicDeveloperSettings>()->bFriendlyFireEnabled;
        if (ShouldNegateFriendlyFire(bSourceIsPlayer, bTargetIsPlayer, SourceActor == TargetActor, bFriendlyFire)) {
            UE_LOG(Myth, Log, TEXT("DamageApplication:: friendly fire OFF — player→player hit negated"));
            return; // no damage, no shield drain, no status
        }
    }

    const FGameplayTagContainer *SourceTags = Spec->CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer *TargetTags = Spec->CapturedTargetTags.GetAggregatedTags();

    FAggregatorEvaluateParameters EvaluateParameters;
    EvaluateParameters.SourceTags = SourceTags;
    EvaluateParameters.TargetTags = TargetTags;

    // GameState holds the damage-tuning designer scalars (status-tag modifiers, armor curve, chip floor). Resolve
    // once here; reused for the pre-mitigation status modifiers and the armor mitigation below.
    const UWorld *World = TargetASC ? TargetASC->GetWorld() : nullptr;
    const AMythicGameState *GS = World ? World->GetGameState<AMythicGameState>() : nullptr;

    float Power = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageApplicationStatics().Power, EvaluateParameters, Power);
    float DmgPerHit = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageApplicationStatics().DamagePerHit, EvaluateParameters, DmgPerHit);
    float CriticalHitDamage = 0.0f; // unresolved capture (e.g. source has no Offense set) = no crit bonus, not a silent +50%
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageApplicationStatics().CriticalHitDamage, EvaluateParameters, CriticalHitDamage);
    // BonusSkillDamage (source Offense attr): captured in readiness but NOT yet applied — like BonusDamageToSuperiorEnemies
    // below, it has no keying scheme. "Skill damage" means damage dealt BY A SKILL (vs a basic attack), but no
    // skill-vs-basic-attack classification exists (UMythicGameplayAbility has only Activation policy/group, and no
    // GAS tag distinguishes the two), so there is no honest signal to gate this on. DEFERRED pending that design
    // decision (see backlog) — captured here so the wire-up is a one-liner once a skill tag is injected by
    // MakeDamageContainerSpec, mirroring the weapon-class tags.
    float BonusSkillDamage = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageApplicationStatics().BonusSkillDamage, EvaluateParameters, BonusSkillDamage);
    float BonusSwordDamage = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageApplicationStatics().BonusSwordDamage, EvaluateParameters, BonusSwordDamage);
    float BonusAxeDamage = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageApplicationStatics().BonusAxeDamage, EvaluateParameters, BonusAxeDamage);
    float BonusDaggerDamage = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageApplicationStatics().BonusDaggerDamage, EvaluateParameters, BonusDaggerDamage);
    float BonusSickleDamage = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageApplicationStatics().BonusSickleDamage, EvaluateParameters, BonusSickleDamage);
    float BonusSpearDamage = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageApplicationStatics().BonusSpearDamage, EvaluateParameters, BonusSpearDamage);
    float BonusHammerDamage = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageApplicationStatics().BonusHammerDamage, EvaluateParameters, BonusHammerDamage);
    float IncreasedDamageToEnemiesUnderStatusEffects = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageApplicationStatics().IncreasedDamageToEnemiesUnderStatusEffects, EvaluateParameters,
                                                               IncreasedDamageToEnemiesUnderStatusEffects);
    float BonusDamageToSuperiorEnemies = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageApplicationStatics().BonusDamageToSuperiorEnemies, EvaluateParameters,
                                                               BonusDamageToSuperiorEnemies);
    float OutgoingDamageMultiplier = 1.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageApplicationStatics().OutgoingDamageMultiplier, EvaluateParameters,
                                                               OutgoingDamageMultiplier);
    OutgoingDamageMultiplier = FMath::Max(0.0f, OutgoingDamageMultiplier);

    float IncomingDamageMultiplier = 1.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageApplicationStatics().IncomingDamageMultiplier, EvaluateParameters,
                                                               IncomingDamageMultiplier);
    IncomingDamageMultiplier = FMath::Max(0.0f, IncomingDamageMultiplier);

    auto FinalDamage = FMath::Max(1.0f, Power) * FMath::RandRange(DmgPerHit, DmgPerHit * 1.5f);
    UE_LOG(Myth, Log, TEXT("DamageApplication:: Damage %f = DmgPerHit (%f - %f) * Power (%f)"), FinalDamage, DmgPerHit, DmgPerHit * 1.5f, Power);
    if (MythicContext->IsCriticalHit()) {
        FinalDamage += FinalDamage * CriticalHitDamage;
        UE_LOG(Myth, Warning, TEXT("DamageApplication:: Critical hit! Damage increased by %f Percent"), CriticalHitDamage * 100.0f);
    }

    // prevent negative damage from bad configuration
    FinalDamage = FMath::Max(0.0f, FinalDamage * OutgoingDamageMultiplier * IncomingDamageMultiplier);

    // apply status tag damage modifiers based on gamestate config
    if (GS) {
        float StatusMult = 1.0f;
        if (SourceTags) {
            if (SourceTags->HasTag(GAS_BUFF_RAGE)) { StatusMult *= (1.0f + GS->RageDamageBonus); }
            if (SourceTags->HasTag(GAS_DEBUFF_WEAKENED)) { StatusMult *= FMath::Max(0.0f, 1.0f - GS->WeakenedDamagePenalty); }
        }
        if (TargetTags) {
            if (TargetTags->HasTag(GAS_DEBUFF_TERRIFIED)) { StatusMult *= (1.0f + GS->TerrifiedDamageBonus); }
            if (TargetTags->HasTag(GAS_BUFF_FORTIFY)) { StatusMult *= FMath::Max(0.0f, 1.0f - GS->FortifyDamageReduction); }
        }
        FinalDamage = FMath::Max(0.0f, FinalDamage * StatusMult);
    }

    // apply status effect conditional damage modifiers
    static const FGameplayTag DebuffParent = GAS_DEBUFF;

    // amplify hit if target has active status effect
    if (TargetTags && IncreasedDamageToEnemiesUnderStatusEffects != 0.0f && DebuffParent.IsValid() && TargetTags->HasTag(DebuffParent)) {
        FinalDamage = FMath::Max(0.0f, FinalDamage * (1.0f + IncreasedDamageToEnemiesUnderStatusEffects));
    }

    // reduce incoming damage if attacker has active status effect
    if (SourceTags && DebuffParent.IsValid() && SourceTags->HasTag(DebuffParent)) {
        float DecreasedDamageFromEnemiesUnderStatusEffects = 0.0f;
        ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageApplicationStatics().DecreasedDamageFromEnemiesUnderStatusEffects,
                                                                   EvaluateParameters, DecreasedDamageFromEnemiesUnderStatusEffects);
        // check if reduction is positive to avoid amplifying damage
        if (DecreasedDamageFromEnemiesUnderStatusEffects > 0.0f) {
            FinalDamage = FMath::Max(0.0f, FinalDamage * FMath::Max(0.0f, 1.0f - DecreasedDamageFromEnemiesUnderStatusEffects));
        }
    }

    // apply weapon class damage bonus based on source weapon tag
    if (SourceTags) {
        // check each weapon class to apply a single class bonus
        float WeaponMult = 1.0f;
        if (SourceTags->HasTag(ITEMIZATION_TYPE_EQUIPMENT_WEAPON_SWORD)) { WeaponMult = (1.0f + BonusSwordDamage); }
        else if (SourceTags->HasTag(ITEMIZATION_TYPE_EQUIPMENT_WEAPON_AXE)) { WeaponMult = (1.0f + BonusAxeDamage); }
        else if (SourceTags->HasTag(ITEMIZATION_TYPE_EQUIPMENT_WEAPON_DAGGERS)) { WeaponMult = (1.0f + BonusDaggerDamage); }
        else if (SourceTags->HasTag(ITEMIZATION_TYPE_EQUIPMENT_WEAPON_SICKLE)) { WeaponMult = (1.0f + BonusSickleDamage); }
        else if (SourceTags->HasTag(ITEMIZATION_TYPE_EQUIPMENT_WEAPON_SPEAR)) { WeaponMult = (1.0f + BonusSpearDamage); }
        else
            if (SourceTags->HasTag(ITEMIZATION_TYPE_EQUIPMENT_WEAPON_HAMMER)) { WeaponMult = (1.0f + BonusHammerDamage); }
        if (WeaponMult != 1.0f) {
            FinalDamage = FMath::Max(0.0f, FinalDamage * WeaponMult);
        }
    }

    // apply skill damage multiplier if incoming hit was delivered by a skill
    if (SourceTags && BonusSkillDamage != 0.0f) {
        if (SourceTags->HasTag(GAS_ABILITY_TYPE_SKILL)) {
            FinalDamage = FMath::Max(0.0f, FinalDamage * (1.0f + BonusSkillDamage));
        }
    }

    // apply superior damage bonus if target matches superior tier tag
    FGameplayTag SuperiorRoot = AI_TIER_SUPERIOR;
    bool bIsSuperior = false;
    if (TargetTags) {
        for (const FGameplayTag& Tag : *TargetTags) {
            if ((SuperiorRoot.IsValid() && Tag.MatchesTag(SuperiorRoot)) ||
                Tag.MatchesTag(AI_TIER_ELITE) ||
                Tag.MatchesTag(AI_TIER_CHAMPION) ||
                Tag.MatchesTag(AI_TIER_BOSS)) {
                bIsSuperior = true;
                break;
            }
        }
    }
    if (bIsSuperior && BonusDamageToSuperiorEnemies != 0.0f) {
        FinalDamage = FMath::Max(0.0f, FinalDamage * (1.0f + BonusDamageToSuperiorEnemies));
    }

    UE_LOG(Myth, Log, TEXT("DamageApplication:: Pre-mitigation damage: %f"), FinalDamage);

    // apply defensive mitigation checks
    auto &Statics = MythicDamageApplicationStatics();

    // check if target is invincible to negate hit entirely
    if (TargetTags && TargetTags->HasTag(GAS_BUFF_INVINCIBLE)) {
        UE_LOG(Myth, Log, TEXT("DamageApplication:: Target INVINCIBLE - hit negated"));
        return; // no damage, no shield drain, no status
    }

    // perform dodge roll against targets dodge chance attribute
    float DodgeChance = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.DodgeChance, EvaluateParameters, DodgeChance);
    if (MythicCombat::RollSucceeds(DodgeChance, FMath::FRand())) {
        MythicContext->SetDodged(true);
        UE_LOG(Myth, Log, TEXT("DamageApplication:: Attack DODGED (chance %.2f)"), DodgeChance);
        // show dodge floating text feedback on client
        if (const APawn *VictimPawn = TargetASC ? Cast<APawn>(TargetASC->GetAvatarActor()) : nullptr) {
            if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(VictimPawn->GetController())) {
                PC->ClientShowDodge();
            }
        }
        return; // no damage, no shield drain, no status
    }

    // roll for status effect application based on resistance attribute
    auto GateStatus = [&](const FGameplayEffectAttributeCaptureDefinition &ResistDef, bool bSourceIntent) -> bool {
        if (!bSourceIntent) {
            return false;
        }
        float Resist = 0.0f;
        ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(ResistDef, EvaluateParameters, Resist);
        const float SurviveChance = FMath::Clamp(1.0f - Resist, 0.0f, 1.0f);
        return MythicCombat::RollSucceeds(SurviveChance, FMath::FRand());
    };
    MythicContext->SetBleed(GateStatus(Statics.BleedResistance, MythicContext->IsBleed()));
    MythicContext->SetBurn(GateStatus(Statics.BurnResistance, MythicContext->IsBurn()));
    MythicContext->SetPoison(GateStatus(Statics.PoisonResistance, MythicContext->IsPoison()));
    MythicContext->SetSlow(GateStatus(Statics.SlowResistance, MythicContext->IsSlow()));
    MythicContext->SetFreeze(GateStatus(Statics.FreezeResistance, MythicContext->IsFreeze()));
    MythicContext->SetStun(GateStatus(Statics.StunResistance, MythicContext->IsStun()));

    // calculate armor mitigation using gamestate mitigation curve
    float Armor = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.Armor, EvaluateParameters, Armor);
    Armor = FMath::Max(0.0f, Armor);
    float MitigationFraction = 0.0f;
    if (GS) {
        if (const FRealCurve *Curve = GS->ArmorMitigationCurveRowHandle.GetCurve(TEXT("DamageApplication.Armor"))) {
            MitigationFraction = FMath::Clamp(Curve->Eval(Armor), 0.0f, 0.85f);
        }
    }
    FinalDamage *= (1.0f - MitigationFraction);
    // Every landed (non-dodged) hit chips at least MinChipDamage, so high Armor (or a 0-base hit) never fully no-ops.
    if (GS) {
        FinalDamage = FMath::Max(GS->MinChipDamage, FinalDamage);
    }

    // (4) Shield absorbs before health; only the overflow reaches the Damage meta-attribute (so a fully
    //     absorbed hit cannot trigger death).
    float Shield = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.Shield, EvaluateParameters, Shield);
    Shield = FMath::Max(0.0f, Shield);
    const float ToShield = FMath::Min(Shield, FinalDamage);
    const float ToHealth = FinalDamage - ToShield;

    UE_LOG(Myth, Log, TEXT("DamageApplication:: ArmorMit=%.2f Armor=%.1f -> ToShield=%.1f ToHealth=%.1f"),
           MitigationFraction, Armor, ToShield, ToHealth);

    if (ToShield > 0.0f) {
        OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
            UMythicAttributeSet_Defense::GetShieldAttribute(), EGameplayModOp::Additive, -ToShield));
    }
    if (ToHealth > 0.0f) {
        // PostGameplayEffectExecute in AttributeSet_Life converts Damage -> -Health and drives damage cues.
        OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
            UMythicAttributeSet_Life::GetDamageAttribute(), EGameplayModOp::Additive, ToHealth));
    }

    // // Send a "HIT" gameplay event to source
    // FGameplayTag HitTag = GAS_EVENT_DMG_DELIVERED;
    // FGameplayEventData EventData;
    // EventData.Instigator = SourceASC->GetAvatarActor();
    // EventData.Target = TargetASC->GetAvatarActor();
    // EventData.EventMagnitude = FinalDamage;
    // EventData.EventTag = HitTag;
    // EventData.ContextHandle = Spec->GetContext();
    //
    // auto activations = SourceASC->HandleGameplayEvent(HitTag, &EventData);
    // UE_LOG(Myth, Warning, TEXT("Hit event sent to source %d abilities"), activations);
}
