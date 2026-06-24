// 


#include "MythicDamageApplication.h"

#include "Mythic.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "GAS/MythicTags_GAS.h"
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

    // ---- TARGET (victim) defensive captures. This execution runs once per target with the victim's ASC. ----
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

    auto FinalDamage = FMath::Max(1.0f, Power) * FMath::RandRange(DmgPerHit, DmgPerHit * 1.5f);
    UE_LOG(Myth, Log, TEXT("DamageApplication:: Damage %f = DmgPerHit (%f - %f) * Power (%f)"), FinalDamage, DmgPerHit, DmgPerHit * 1.5f, Power);
    if (MythicContext->IsCriticalHit()) {
        FinalDamage += FinalDamage * CriticalHitDamage;
        UE_LOG(Myth, Warning, TEXT("DamageApplication:: Critical hit! Damage increased by %f Percent"), CriticalHitDamage * 100.0f);
    }

    // Never negative regardless of a (mis)configured DmgPerHit - make the invariant explicit, not emergent.
    FinalDamage = FMath::Max(0.0f, FinalDamage);

    // ===== Status-tag damage modifiers (pre-mitigation) =====
    // Source RAGE deals more, WEAKENED deals less; target TERRIFIED takes more, FORTIFY takes less. These tags came
    // from the GE pipeline (declared with exactly these damage semantics in MythicTags_GAS.h) but were never read
    // here. Multipliers are designer-tunable on the GameState (mirrors MinChipDamage / the armor curve).
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

    // ===== Status-effect-conditional damage modifiers (one offensive, one defensive — both key off any GAS.Debuff.*,
    // which HasTag matches via its children). Shared parent-tag handle. Not gated on the GameState. =====
    static const FGameplayTag DebuffParent = FGameplayTag::RequestGameplayTag(FName("GAS.Debuff"), /*ErrorIfNotFound=*/false);

    // Source's "IncreasedDamageToEnemiesUnderStatusEffects" (source Offense attr): amplify the hit when the TARGET
    // already carries any status effect.
    if (TargetTags && IncreasedDamageToEnemiesUnderStatusEffects != 0.0f && DebuffParent.IsValid() && TargetTags->HasTag(DebuffParent)) {
        FinalDamage = FMath::Max(0.0f, FinalDamage * (1.0f + IncreasedDamageToEnemiesUnderStatusEffects));
    }

    // Target's "DecreasedDamageFromEnemiesUnderStatusEffects" (defensive MIRROR, target Defense attr): a target with
    // this stat takes LESS damage from an ATTACKER who is under any status effect (any GAS.Debuff.* on the SOURCE).
    // Captured inline (target capture, like Armor) only when the source is actually debuffed. Clamped so a value >= 1
    // floors damage at 0 rather than going negative.
    if (SourceTags && DebuffParent.IsValid() && SourceTags->HasTag(DebuffParent)) {
        float DecreasedDamageFromEnemiesUnderStatusEffects = 0.0f;
        ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageApplicationStatics().DecreasedDamageFromEnemiesUnderStatusEffects,
                                                                   EvaluateParameters, DecreasedDamageFromEnemiesUnderStatusEffects);
        // > 0 only: a NEGATIVE value must be a no-op, never an AMPLIFIER (1 - (-x) > 1). PreAttributeChange also clamps
        // the attr to [0,1]; this guard is belt-and-suspenders against a SetNumericAttributeBase path that bypasses it.
        if (DecreasedDamageFromEnemiesUnderStatusEffects > 0.0f) {
            FinalDamage = FMath::Max(0.0f, FinalDamage * FMath::Max(0.0f, 1.0f - DecreasedDamageFromEnemiesUnderStatusEffects));
        }
    }

    // Weapon-class damage bonus: the WIELDING weapon's class tag (Itemization.Type.Equipment.Weapon.*) was injected
    // into the source tags by MakeDamageContainerSpec. Apply the matching captured BonusXxxDamage (source Offense
    // attrs, captured above but previously DISCARDED — no weapon ever changed its own damage; this also revives
    // AffixesFragment rolls of these attrs). Keyed STRICTLY off the weapon-class tags MythicTags_Inventory already
    // defines + every weapon ItemDefinition already sets — no invented scheme. (Superior-enemy / Elite bonus stays
    // deferred: no enemy-tier tag scheme exists to key it off.)
    if (SourceTags) {
        // A weapon has ONE class — if/else-if applies a SINGLE class bonus (a multi-class instance from a runtime
        // transform/conversion would otherwise stack multiplicatively). All six first-class weapon tags are covered;
        // Hammer was added in the iter-71 review (it was a defined, selectable class with no consumer = silently dead).
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

    UE_LOG(Myth, Log, TEXT("DamageApplication:: Pre-mitigation damage: %f"), FinalDamage);

    // ===== Defensive mitigation (per-target; captures resolve against THIS victim's ASC) =====
    auto &Statics = MythicDamageApplicationStatics();

    // (0) Invincible - the victim cannot be damaged at all. Negate the hit entirely (mirrors the Dodge early-return:
    //     no damage, no shield drain, no status). Honors GAS.Buff.Invincible, declared "cannot be damaged" but never
    //     enforced until now — any designer GE granting the tag now works.
    if (TargetTags && TargetTags->HasTag(GAS_BUFF_INVINCIBLE)) {
        UE_LOG(Myth, Log, TEXT("DamageApplication:: Target INVINCIBLE - hit negated"));
        return; // no damage, no shield drain, no status
    }

    // (1) Dodge - per-target roll against the victim's DodgeChance: negates the hit entirely.
    float DodgeChance = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.DodgeChance, EvaluateParameters, DodgeChance);
    if (MythicCombat::RollSucceeds(DodgeChance, FMath::FRand())) { // shared boundary rule (0% never dodges, 100% always)
        MythicContext->SetDodged(true);
        UE_LOG(Myth, Log, TEXT("DamageApplication:: Attack DODGED (chance %.2f)"), DodgeChance);
        // Player-facing callout: the dodge negates the hit BEFORE the damage cue runs, so push a "DODGE" float to the
        // dodging player (server-authoritative execution -> the victim's owning client; mirrors ClientShowShieldAbsorbed).
        if (const APawn *VictimPawn = TargetASC ? Cast<APawn>(TargetASC->GetAvatarActor()) : nullptr) {
            if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(VictimPawn->GetController())) {
                PC->ClientShowDodge();
            }
        }
        return; // no damage, no shield drain, no status
    }

    // (2) Resistance-gate the source-intended status flags against the victim's resistances. The surviving
    //     flags live on THIS per-target context for the status applier to consume (status stage).
    auto GateStatus = [&](const FGameplayEffectAttributeCaptureDefinition &ResistDef, bool bSourceIntent) -> bool {
        if (!bSourceIntent) {
            return false;
        }
        float Resist = 0.0f;
        ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(ResistDef, EvaluateParameters, Resist);
        // Two-sided: a fully-resisted status (SurviveChance <= 0) must NEVER survive; an unresisted one (>= 1) ALWAYS
        // survives. Same boundary rule as the proc/dodge rolls — shared via MythicCombat::RollSucceeds.
        const float SurviveChance = FMath::Clamp(1.0f - Resist, 0.0f, 1.0f);
        return MythicCombat::RollSucceeds(SurviveChance, FMath::FRand());
    };
    MythicContext->SetBleed(GateStatus(Statics.BleedResistance, MythicContext->IsBleed()));
    MythicContext->SetBurn(GateStatus(Statics.BurnResistance, MythicContext->IsBurn()));
    MythicContext->SetPoison(GateStatus(Statics.PoisonResistance, MythicContext->IsPoison()));
    MythicContext->SetSlow(GateStatus(Statics.SlowResistance, MythicContext->IsSlow()));
    MythicContext->SetFreeze(GateStatus(Statics.FreezeResistance, MythicContext->IsFreeze()));
    MythicContext->SetStun(GateStatus(Statics.StunResistance, MythicContext->IsStun()));
    // Weaken / Terrify: unresisted in v1.

    // (3) Armor mitigation - reduction fraction from a GameState curve, floored so high Armor never makes a
    //     non-zero hit unkillable.
    float Armor = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.Armor, EvaluateParameters, Armor);
    Armor = FMath::Max(0.0f, Armor);
    float MitigationFraction = 0.0f;
    if (GS) {
        if (const FRealCurve *Curve = GS->ArmorMitigationCurveRowHandle.GetCurve(TEXT("DamageApplication.Armor"))) {
            MitigationFraction = FMath::Clamp(Curve->Eval(Armor), 0.0f, 1.0f);
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
