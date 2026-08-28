

#include "MythicDamageApplication.h"

#include "Mythic.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "GAS/Effects/MythicStatusRegistry.h"
#include "GAS/MythicTags_GAS.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/Feedback/MythicTags_FeedbackCues.h"
#include "AI/MythicTags_AI.h"
#include "Itemization/MythicTags_Inventory.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/Executions/MythicCombatRoll.h"
#include "GAS/MythicStatContribution.h"
#include "GameModes/GameState/MythicGameState.h"
#include "Curves/RealCurve.h"
#include "Engine/World.h"
#include "Player/MythicPlayerController.h"
#include "GameFramework/Pawn.h"
#include "AbilitySystemComponent.h"
#include "Settings/MythicDeveloperSettings.h"
#include "Settings/MythicCombatSettings.h"
#include "GAS/MythicStatDiminishing.h"
#include "GAS/Executions/MythicDamageCompose.h"
#include "GAS/MythicWeatherCombatRules.h"
#include "World/EnvironmentController/MythicEnvironmentSubsystem.h"

struct FMythicGameplayEffectContext;

bool UMythicDamageApplication::ShouldNegateFriendlyFire(bool bSourceIsPlayer, bool bTargetIsPlayer, bool bSameActor, bool bFriendlyFireEnabled) {
    return bSourceIsPlayer && bTargetIsPlayer && !bSameActor && !bFriendlyFireEnabled;
}

float UMythicDamageApplication::ApplyChipFloor(float Damage, float MinChipDamage) {
    if (Damage <= 0.0f) {
        return 0.0f;
    }
    return FMath::Max(MinChipDamage, Damage);
}

float UMythicDamageApplication::ComputeBuildupPerProc(float BasePerProc, float SourceMultiplier) {
    return FMath::Max(0.0f, BasePerProc) * FMath::Max(0.0f, SourceMultiplier);
}

float UMythicDamageApplication::ApplySkillDamageBonus(float Damage, bool bIsSkillHit, float BonusSkillDamage) {
    return FMath::Max(0.0f, bIsSkillHit ? Damage * (1.0f + BonusSkillDamage) : Damage);
}

void UMythicDamageApplication::MarkDamageExecutionAborted(
    FGameplayEffectCustomExecutionOutput &OutExecutionOutput) {
    // The native application GE owns Damage.Hit. An execution that rejects the hit must opt out of the GE's
    // automatic cue dispatch, otherwise invulnerability, dodge, invalid data, and friendly fire still look landed.
    OutExecutionOutput.MarkGameplayCuesHandledManually();
}

bool UMythicDamageApplication::HandleResolvedDamageCuePolicy(
    const float ResolvedDamage,
    FGameplayEffectCustomExecutionOutput &OutExecutionOutput) {
    if (FMath::IsFinite(ResolvedDamage) && ResolvedDamage > 0.0f) {
        return false;
    }

    MarkDamageExecutionAborted(OutExecutionOutput);
    return true;
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
    FGameplayEffectAttributeCaptureDefinition StatusBuildupMultiplier;

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
    FGameplayEffectAttributeCaptureDefinition WeakenResistance;
    FGameplayEffectAttributeCaptureDefinition TerrifyResistance;
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
        StatusBuildupMultiplier = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetStatusBuildupMultiplierAttribute(),
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
        WeakenResistance = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Defense::GetWeakenResistanceAttribute(),
                                                                     EGameplayEffectAttributeCaptureSource::Target, false);
        TerrifyResistance = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Defense::GetTerrifyResistanceAttribute(),
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
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().StatusBuildupMultiplier);
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
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().WeakenResistance);
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().TerrifyResistance);
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().IncomingDamageMultiplier);

    if (const UMythicCombatSettings *CombatSettings = GetDefault<UMythicCombatSettings>()) {
        auto AddComposeCaptures = [this](const TArray<FGameplayAttribute> &Attrs,
                                         TArray<FGameplayEffectAttributeCaptureDefinition> &OutDefs) {
            for (const FGameplayAttribute &Attr : Attrs) {
                if (Attr.IsValid()) {
                    FGameplayEffectAttributeCaptureDefinition Def(Attr, EGameplayEffectAttributeCaptureSource::Source, false);
                    OutDefs.Add(Def);
                    RelevantAttributesToCapture.Add(Def);
                }
            }
        };
        AddComposeCaptures(CombatSettings->DamageCompose.IncreasedBucketAttributes, IncreasedComposeCaptures);
        AddComposeCaptures(CombatSettings->DamageCompose.MoreBucketAttributes, MoreComposeCaptures);
    }
}

void UMythicDamageApplication::Execute_Implementation(const FGameplayEffectCustomExecutionParameters &ExecutionParams,
                                                      FGameplayEffectCustomExecutionOutput &OutExecutionOutput) const {
    Super::Execute_Implementation(ExecutionParams, OutExecutionOutput);
    UE_LOG(Myth, Verbose, TEXT("DamageApplication:: Applying damage"));

    FGameplayEffectSpec *Spec = ExecutionParams.GetOwningSpecForPreExecuteMod();
    if (!Spec) {
        MarkDamageExecutionAborted(OutExecutionOutput);
        UE_LOG(Myth, Error, TEXT("DamageApplication:: missing owning effect spec - aborting damage execution"));
        return;
    }

    FMythicGameplayEffectContext *MythicContext =
        FMythicGameplayEffectContext::ExtractEffectContext(Spec->GetContext());
    if (!MythicContext) {
        MarkDamageExecutionAborted(OutExecutionOutput);
        UE_LOG(Myth, Error, TEXT("DamageApplication:: non-Mythic/empty effect context - aborting damage execution"));
        return;
    }

    auto SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
    auto TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
    if (!SourceASC || !TargetASC) {
        MarkDamageExecutionAborted(OutExecutionOutput);
        UE_LOG(Myth, Error, TEXT("DamageApplication:: missing source or target ASC - aborting damage execution"));
        return;
    }

    {
        const AActor *SourceActor = SourceASC ? SourceASC->GetAvatarActor() : nullptr;
        const AActor *TargetActor = TargetASC ? TargetASC->GetAvatarActor() : nullptr;
        const APawn *SourcePawn = Cast<APawn>(SourceActor);
        const APawn *TargetPawn = Cast<APawn>(TargetActor);
        const bool bSourceIsPlayer = SourcePawn && SourcePawn->IsPlayerControlled();
        const bool bTargetIsPlayer = TargetPawn && TargetPawn->IsPlayerControlled();
        const bool bFriendlyFire = GetDefault<UMythicDeveloperSettings>()->bFriendlyFireEnabled;
        if (ShouldNegateFriendlyFire(bSourceIsPlayer, bTargetIsPlayer, SourceActor == TargetActor, bFriendlyFire)) {
            MarkDamageExecutionAborted(OutExecutionOutput);
            UE_LOG(Myth, Log, TEXT("DamageApplication:: friendly fire OFF — player→player hit negated"));
            return;
        }
    }

    const FGameplayTagContainer *SourceTags = Spec->CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer *TargetTags = Spec->CapturedTargetTags.GetAggregatedTags();

    FAggregatorEvaluateParameters EvaluateParameters;
    EvaluateParameters.SourceTags = SourceTags;
    EvaluateParameters.TargetTags = TargetTags;

    const UWorld *World = TargetASC ? TargetASC->GetWorld() : nullptr;
    const AMythicGameState *GS = World ? World->GetGameState<AMythicGameState>() : nullptr;

    float Power = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageApplicationStatics().Power, EvaluateParameters, Power);
    float DmgPerHit = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageApplicationStatics().DamagePerHit, EvaluateParameters, DmgPerHit);
    float CriticalHitDamage = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageApplicationStatics().CriticalHitDamage, EvaluateParameters, CriticalHitDamage);
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

    /**
     * Power contributes an additive, diminished FRACTION of the weapon roll - not a bare multiplier.
     *
     * It used to be FMath::Max(1.0f, Power) * Roll. Weapon damage rises with item level and Power rises with
     * character level, so multiplying them made damage grow quadratically: twice the level with twice the weapon
     * dealt four times the damage. No tuning pass fixes a quadratic. The Max(1.0f, ...) also flattened every
     * Power value from 0 to 1 into the same output, so early investment did nothing and any debuff below 1 was
     * silently inert.
     *
     * The fraction comes from the authored mapping, so the coefficient is not a literal here, and it passes the
     * same diminishing curve as every other stacked stat.
     */
    float MinimumWeaponDamage = 0.0f;
    float MaximumWeaponDamage = 0.0f;
    float AverageWeaponDamage = 0.0f;
    if (!MythicCombat::ResolveWeaponDamageRange(
            DmgPerHit, MinimumWeaponDamage, MaximumWeaponDamage, AverageWeaponDamage)) {
        MarkDamageExecutionAborted(OutExecutionOutput);
        UE_LOG(Myth, Error, TEXT("DamageApplication:: invalid DamagePerHit or weapon damage range configuration"));
        return;
    }
    const float WeaponRoll = FMath::FRandRange(MinimumWeaponDamage, MaximumWeaponDamage);
    float FinalDamage = WeaponRoll;
    if (const UMythicCombatSettings *CombatSettings = GetDefault<UMythicCombatSettings>()) {
        FinalDamage = FMythicStatContributionRules::ApplyToBase(
            CombatSettings->StatContributions.Contributions,
            UMythicAttributeSet_Offense::GetDamagePerHitAttribute(), WeaponRoll,
            [Power](const FGameplayAttribute &Attr) -> float {
                // Only Power is captured by this execution, so any other source reads as absent rather than
                // as zero-with-authority: a row feeding weapon damage off some stat we cannot see contributes
                // nothing here instead of quietly cancelling itself.
                return Attr == UMythicAttributeSet_Offense::GetPowerAttribute() ? Power : 0.0f;
            });
    }
    const float PowerFraction = WeaponRoll > KINDA_SMALL_NUMBER ? (FinalDamage / WeaponRoll) - 1.0f : 0.0f;
    UE_LOG(Myth, Verbose,
           TEXT("DamageApplication:: Damage %f = weapon range (%f - %f, expected %f) * (1 + Power %f -> +%.1f%%)"),
           FinalDamage, MinimumWeaponDamage, MaximumWeaponDamage, AverageWeaponDamage,
           Power, PowerFraction * 100.0f);
    // Every damage-affecting fraction below is a stacked 0.0-based bonus, so each rides its authored diminishing
    // curve before it multiplies. A stat with no curve passes through unchanged, so this is a no-op until one is
    // authored - the curves in UMythicCombatSettings::StatDiminishing are the brake, not this call.
    const UMythicCombatSettings *CurveSettings = GetDefault<UMythicCombatSettings>();
    auto CurveBonus = [CurveSettings](const FGameplayAttribute &Attribute, float RawBonus) {
        return CurveSettings ? FMythicStatDiminishingRules::ApplyToBonus(CurveSettings->StatDiminishing, Attribute, RawBonus)
                             : 1.0f + FMath::Max(0.0f, RawBonus);
    };

    if (MythicContext->IsCriticalHit()) {
        FinalDamage = FMath::Max(0.0f, FinalDamage * CurveBonus(UMythicAttributeSet_Offense::GetCriticalHitDamageAttribute(), CriticalHitDamage));
        UE_LOG(Myth, Verbose, TEXT("DamageApplication:: Critical hit! Damage increased by %f Percent"), CriticalHitDamage * 100.0f);
    }

    FinalDamage = FMath::Max(0.0f, FinalDamage * OutgoingDamageMultiplier * IncomingDamageMultiplier);

    if (GS) {
        float StatusMult = 1.0f;
        if (SourceTags) {
            if (SourceTags->HasTag(GAS_BUFF_RAGE)) { StatusMult *= (1.0f + GS->RageDamageBonus); }
            if (SourceTags->HasTag(GAS_DEBUFF_WEAKENED)) { StatusMult *= UMythicStatusRegistry::GetControlReductionMultiplier(SourceASC, GAS_DEBUFF_WEAKENED, GS->WeakenedDamagePenalty); }
        }
        if (TargetTags) {
            if (TargetTags->HasTag(GAS_DEBUFF_TERRIFIED)) { StatusMult *= UMythicStatusRegistry::GetControlBonusMultiplier(TargetASC, GAS_DEBUFF_TERRIFIED, GS->TerrifiedDamageBonus); }
            if (TargetTags->HasTag(GAS_BUFF_FORTIFY)) { StatusMult *= FMath::Max(0.0f, 1.0f - GS->FortifyDamageReduction); }
        }
        FinalDamage = FMath::Max(0.0f, FinalDamage * StatusMult);
    }

    static const FGameplayTag DebuffParent = GAS_DEBUFF;

    if (TargetTags && IncreasedDamageToEnemiesUnderStatusEffects != 0.0f && DebuffParent.IsValid() && TargetTags->HasTag(DebuffParent)) {
        FinalDamage = FMath::Max(0.0f, FinalDamage * CurveBonus(UMythicAttributeSet_Offense::GetIncreasedDamageToEnemiesUnderStatusEffectsAttribute(),
                                                                IncreasedDamageToEnemiesUnderStatusEffects));
    }

    if (SourceTags && DebuffParent.IsValid() && SourceTags->HasTag(DebuffParent)) {
        float DecreasedDamageFromEnemiesUnderStatusEffects = 0.0f;
        ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageApplicationStatics().DecreasedDamageFromEnemiesUnderStatusEffects,
                                                                   EvaluateParameters, DecreasedDamageFromEnemiesUnderStatusEffects);
        if (DecreasedDamageFromEnemiesUnderStatusEffects > 0.0f) {
            FinalDamage = FMath::Max(0.0f, FinalDamage * FMath::Max(0.0f, 1.0f - DecreasedDamageFromEnemiesUnderStatusEffects));
        }
    }

    if (SourceTags) {
        using Off = UMythicAttributeSet_Offense;
        float WeaponMult = 1.0f;
        if (SourceTags->HasTag(ITEMIZATION_TYPE_EQUIPMENT_WEAPON_SWORD)) { WeaponMult = CurveBonus(Off::GetBonusSwordDamageAttribute(), BonusSwordDamage); }
        else if (SourceTags->HasTag(ITEMIZATION_TYPE_EQUIPMENT_WEAPON_AXE)) { WeaponMult = CurveBonus(Off::GetBonusAxeDamageAttribute(), BonusAxeDamage); }
        else if (SourceTags->HasTag(ITEMIZATION_TYPE_EQUIPMENT_WEAPON_DAGGERS)) { WeaponMult = CurveBonus(Off::GetBonusDaggerDamageAttribute(), BonusDaggerDamage); }
        else if (SourceTags->HasTag(ITEMIZATION_TYPE_EQUIPMENT_WEAPON_SICKLE)) { WeaponMult = CurveBonus(Off::GetBonusSickleDamageAttribute(), BonusSickleDamage); }
        else if (SourceTags->HasTag(ITEMIZATION_TYPE_EQUIPMENT_WEAPON_SPEAR)) { WeaponMult = CurveBonus(Off::GetBonusSpearDamageAttribute(), BonusSpearDamage); }
        else
            if (SourceTags->HasTag(ITEMIZATION_TYPE_EQUIPMENT_WEAPON_HAMMER)) { WeaponMult = CurveBonus(Off::GetBonusHammerDamageAttribute(), BonusHammerDamage); }
        if (WeaponMult != 1.0f) {
            FinalDamage = FMath::Max(0.0f, FinalDamage * WeaponMult);
        }
    }

    if (SourceTags && BonusSkillDamage != 0.0f) {
        const bool bIsSkillHit = SourceTags->HasTag(GAS_ABILITY_TYPE_SKILL);
        // Curve the raw bonus, then hand the bent fraction to the (1+x) skill helper so stacked skill damage bends
        // like every other stacked stat while ApplySkillDamageBonus keeps its own tested contract.
        const float CurvedSkillBonus = CurveBonus(UMythicAttributeSet_Offense::GetBonusSkillDamageAttribute(), BonusSkillDamage) - 1.0f;
        FinalDamage = ApplySkillDamageBonus(FinalDamage, bIsSkillHit, CurvedSkillBonus);
    }

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
        FinalDamage = FMath::Max(0.0f, FinalDamage * CurveBonus(UMythicAttributeSet_Offense::GetBonusDamageToSuperiorEnemiesAttribute(),
                                                                BonusDamageToSuperiorEnemies));
    }

    if (const UMythicCombatSettings *CombatSettings = GetDefault<UMythicCombatSettings>()) {
        const FMythicDamageComposeConfig &ComposeCfg = CombatSettings->DamageCompose;
        if (ComposeCfg.IsConfigured()) {
            float SumIncreased = 0.0f;
            for (const FGameplayEffectAttributeCaptureDefinition &Def : IncreasedComposeCaptures) {
                float Value = 0.0f;
                ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Def, EvaluateParameters, Value);
                SumIncreased += Value;
            }
            TArray<float, TInlineAllocator<8>> MoreMultipliers;
            for (const FGameplayEffectAttributeCaptureDefinition &Def : MoreComposeCaptures) {
                float Value = 0.0f;
                ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Def, EvaluateParameters, Value);
                MoreMultipliers.Add(Value);
            }
            FinalDamage = FMath::Max(0.0f, FMythicDamageComposer::ComposeDamage(FinalDamage, SumIncreased, MoreMultipliers, ComposeCfg.MoreStackCap));
            UE_LOG(Myth, Log, TEXT("DamageApplication:: compose ΣIncreased=%.3f MoreCount=%d -> %f"), SumIncreased, MoreMultipliers.Num(), FinalDamage);
        }
    }

    FGameplayTag ResolvedWeatherTag;
    FGameplayTag WeatherBonusStatusTag;
    if (const UMythicCombatSettings *CombatSettings = GetDefault<UMythicCombatSettings>()) {
        const FMythicWeatherCombatConfig &WeatherCfg = CombatSettings->WeatherCombat;
        if (WeatherCfg.IsConfigured() && World) {
            if (const UGameInstance *GI = World->GetGameInstance()) {
                if (const UMythicEnvironmentSubsystem *Env = GI->GetSubsystem<UMythicEnvironmentSubsystem>()) {
                    if (Env->GetEnvironmentController() != nullptr) {
                        ResolvedWeatherTag = Env->GetWeather();
                    }
                }
            }
            if (ResolvedWeatherTag.IsValid()) {
                FGameplayTagContainer HitDamageTags;
                if (SourceTags) {
                    HitDamageTags.AppendTags(*SourceTags);
                }
                if (MythicContext->IsBurn()) { HitDamageTags.AddTag(GAS_DEBUFF_BURNING); }
                if (MythicContext->IsBleed()) { HitDamageTags.AddTag(GAS_DEBUFF_BLEEDING); }
                if (MythicContext->IsPoison()) { HitDamageTags.AddTag(GAS_DEBUFF_POISONED); }
                if (MythicContext->IsFreeze()) { HitDamageTags.AddTag(GAS_DEBUFF_FROZEN); }
                if (MythicContext->IsStun()) { HitDamageTags.AddTag(GAS_DEBUFF_STUNNED); }
                if (MythicContext->IsSlow()) { HitDamageTags.AddTag(GAS_DEBUFF_SLOWED); }

                const float WeatherMult =
                    FMythicWeatherCombatRules::ResolveWeatherMultiplier(WeatherCfg.Mods, ResolvedWeatherTag, HitDamageTags);
                if (WeatherMult != 1.0f) {
                    FinalDamage = FMath::Max(0.0f, FinalDamage * WeatherMult);
                    UE_LOG(Myth, Log, TEXT("DamageApplication:: weather %s x%.2f -> %f"),
                           *ResolvedWeatherTag.ToString(), WeatherMult, FinalDamage);
                }
                WeatherBonusStatusTag =
                    FMythicWeatherCombatRules::ResolveWeatherStatusBonus(WeatherCfg.Mods, ResolvedWeatherTag, HitDamageTags);
            }
        }
    }

    UE_LOG(Myth, Verbose, TEXT("DamageApplication:: Pre-mitigation damage: %f"), FinalDamage);

    // Incoming/outgoing immunity and other authored multipliers are allowed to resolve the damage component to
    // zero while status intent continues through its own resistance/buildup path below. Suppress only the native
    // application GE's automatic Damage.Hit cue; otherwise an intentional zero-damage proc still looks like a
    // landed weapon hit. Non-finite combat math fails the entire execution closed.
    const bool bDamageNullified =
        HandleResolvedDamageCuePolicy(FinalDamage, OutExecutionOutput);
    if (!FMath::IsFinite(FinalDamage)) {
        UE_LOG(Myth, Error,
               TEXT("DamageApplication:: non-finite composed damage - aborting damage and status execution"));
        return;
    }

    auto &Statics = MythicDamageApplicationStatics();

    if (TargetTags && TargetTags->HasTag(GAS_BUFF_INVINCIBLE)) {
        MarkDamageExecutionAborted(OutExecutionOutput);
        UE_LOG(Myth, Log, TEXT("DamageApplication:: Target INVINCIBLE - hit negated"));
        return;
    }

    float DodgeChance = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.DodgeChance, EvaluateParameters, DodgeChance);
    DodgeChance = MythicCombat::ClampProbability(DodgeChance, GS ? GS->MaxDodgeChance : 0.75f);
    if (MythicCombat::RollSucceeds(DodgeChance, FMath::FRand())) {
        MythicContext->SetDodged(true);
        MarkDamageExecutionAborted(OutExecutionOutput);
        UE_LOG(Myth, Log, TEXT("DamageApplication:: Attack DODGED (chance %.2f)"), DodgeChance);
        if (const APawn *VictimPawn = TargetASC ? Cast<APawn>(TargetASC->GetAvatarActor()) : nullptr) {
            if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(VictimPawn->GetController())) {
                PC->ClientShowDodge();
            }
        }
        return;
    }

    // Plays here, not where the crit is rolled: everything above can still negate the hit, and a crit bang on a
    // dodged or nullified swing reads as a hit that landed.
    if (!bDamageNullified && MythicContext->IsCriticalHit()) {
        if (UMythicAbilitySystemComponent *SourceMythicASC = Cast<UMythicAbilitySystemComponent>(SourceASC)) {
            FGameplayCueParameters CueParams;
            if (const FHitResult *Hit = MythicContext->GetHitResult()) {
                CueParams.Location = Hit->ImpactPoint;
            }
            else if (const AActor *TargetActor = TargetASC ? TargetASC->GetAvatarActor() : nullptr) {
                CueParams.Location = TargetActor->GetActorLocation();
            }
            if (AActor *SourceActor = SourceASC->GetAvatarActor()) {
                CueParams.Instigator = SourceActor;
            }
            SourceMythicASC->ExecuteGameplayCueMulticast(TAG_GameplayCue_Combat_Crit, CueParams);
        }
    }

    bool bAnyStatusResisted = false;
    auto GateStatus = [&](const FGameplayEffectAttributeCaptureDefinition &ResistDef, bool bSourceIntent) -> bool {
        if (!bSourceIntent) {
            return false;
        }
        float Resist = 0.0f;
        ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(ResistDef, EvaluateParameters, Resist);
        const float SurviveChance = FMath::Clamp(1.0f - Resist, 0.0f, 1.0f);
        const bool bSurvived = MythicCombat::RollSucceeds(SurviveChance, FMath::FRand());
        if (!bSurvived) {
            bAnyStatusResisted = true;
        }
        return bSurvived;
    };
    MythicContext->SetBleed(GateStatus(Statics.BleedResistance, MythicContext->IsBleed()));
    MythicContext->SetBurn(GateStatus(Statics.BurnResistance, MythicContext->IsBurn()));
    MythicContext->SetPoison(GateStatus(Statics.PoisonResistance, MythicContext->IsPoison()));
    MythicContext->SetSlow(GateStatus(Statics.SlowResistance, MythicContext->IsSlow()));
    MythicContext->SetFreeze(GateStatus(Statics.FreezeResistance, MythicContext->IsFreeze()));
    MythicContext->SetStun(GateStatus(Statics.StunResistance, MythicContext->IsStun()));
    MythicContext->SetWeaken(GateStatus(Statics.WeakenResistance, MythicContext->IsWeaken()));
    MythicContext->SetTerrify(GateStatus(Statics.TerrifyResistance, MythicContext->IsTerrify()));

    if (bAnyStatusResisted) {
        if (UMythicAbilitySystemComponent *TargetMythicASC = Cast<UMythicAbilitySystemComponent>(TargetASC)) {
            FGameplayCueParameters CueParams;
            if (const AActor *TargetActor = TargetASC->GetAvatarActor()) {
                CueParams.Location = TargetActor->GetActorLocation();
            }
            TargetMythicASC->ExecuteGameplayCueMulticast(TAG_GameplayCue_Combat_Resisted, CueParams);
        }
    }


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
    if (GS) {
        FinalDamage = ApplyChipFloor(FinalDamage, GS->MinChipDamage);
    }

    float Shield = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.Shield, EvaluateParameters, Shield);
    Shield = FMath::Max(0.0f, Shield);
    const float ToShield = FMath::Min(Shield, FinalDamage);
    const float ToHealth = FinalDamage - ToShield;

    UE_LOG(Myth, Verbose, TEXT("DamageApplication:: ArmorMit=%.2f Armor=%.1f -> ToShield=%.1f ToHealth=%.1f"),
           MitigationFraction, Armor, ToShield, ToHealth);

    if (ToShield > 0.0f) {
        OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
            UMythicAttributeSet_Defense::GetShieldAttribute(), EGameplayModOp::Additive, -ToShield));
    }
    // Emitted even when the shield ate all of it. The life set's Damage branch owns the whole on-hit chain,
    // so skipping the modifier means a shielded target never aggros and no on-hit effect fires.
    if (FinalDamage > 0.0f) {
        MythicContext->SetShieldAbsorbed(ToShield);
        OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
            UMythicAttributeSet_Life::GetDamageAttribute(), EGameplayModOp::Additive, ToHealth));
    }

    float BuildupMultiplier = 1.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.StatusBuildupMultiplier, EvaluateParameters, BuildupMultiplier);
    const float StatusBuildupPerProc = ComputeBuildupPerProc(GS ? GS->StatusBuildupPerProc : 25.0f, BuildupMultiplier);
    auto AddBuildup = [&](bool bProcSurvived, const FGameplayAttribute &BuildupAttr) {
        if (bProcSurvived) {
            OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
                BuildupAttr, EGameplayModOp::Additive, StatusBuildupPerProc));
        }
    };
    AddBuildup(MythicContext->IsBurn(), UMythicAttributeSet_Defense::GetBurnBuildupAttribute());
    AddBuildup(MythicContext->IsBleed(), UMythicAttributeSet_Defense::GetBleedBuildupAttribute());
    AddBuildup(MythicContext->IsPoison(), UMythicAttributeSet_Defense::GetPoisonBuildupAttribute());
    AddBuildup(MythicContext->IsSlow(), UMythicAttributeSet_Defense::GetSlowBuildupAttribute());
    AddBuildup(MythicContext->IsFreeze(), UMythicAttributeSet_Defense::GetFreezeBuildupAttribute());
    AddBuildup(MythicContext->IsStun(), UMythicAttributeSet_Defense::GetStunBuildupAttribute());
    AddBuildup(MythicContext->IsWeaken(), UMythicAttributeSet_Defense::GetWeakenBuildupAttribute());
    AddBuildup(MythicContext->IsTerrify(), UMythicAttributeSet_Defense::GetTerrifyBuildupAttribute());

    if (WeatherBonusStatusTag.IsValid()) {
        const FGameplayAttribute WeatherBuildupAttr =
            WeatherBonusStatusTag == GAS_DEBUFF_BURNING    ? UMythicAttributeSet_Defense::GetBurnBuildupAttribute()
            : WeatherBonusStatusTag == GAS_DEBUFF_BLEEDING ? UMythicAttributeSet_Defense::GetBleedBuildupAttribute()
            : WeatherBonusStatusTag == GAS_DEBUFF_POISONED ? UMythicAttributeSet_Defense::GetPoisonBuildupAttribute()
            : WeatherBonusStatusTag == GAS_DEBUFF_SLOWED   ? UMythicAttributeSet_Defense::GetSlowBuildupAttribute()
            : WeatherBonusStatusTag == GAS_DEBUFF_FROZEN   ? UMythicAttributeSet_Defense::GetFreezeBuildupAttribute()
            : WeatherBonusStatusTag == GAS_DEBUFF_STUNNED  ? UMythicAttributeSet_Defense::GetStunBuildupAttribute()
            : WeatherBonusStatusTag == GAS_DEBUFF_WEAKENED ? UMythicAttributeSet_Defense::GetWeakenBuildupAttribute()
            : WeatherBonusStatusTag == GAS_DEBUFF_TERRIFIED ? UMythicAttributeSet_Defense::GetTerrifyBuildupAttribute()
                                                           : FGameplayAttribute();
        if (WeatherBuildupAttr.IsValid()) {
            OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
                WeatherBuildupAttr, EGameplayModOp::Additive, StatusBuildupPerProc));
            UE_LOG(Myth, Log, TEXT("DamageApplication:: weather bonus buildup %s (+%.0f)"),
                   *WeatherBonusStatusTag.ToString(), StatusBuildupPerProc);
        }
    }
}
