

#include "MythicDamageCalculation.h"

#include "Mythic.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/Executions/MythicCombatRoll.h"

struct FMythicDamageCalcStatics {
    FGameplayEffectAttributeCaptureDefinition CriticalHitChance;
    FGameplayEffectAttributeCaptureDefinition ApplyBurnOnHitChance;
    FGameplayEffectAttributeCaptureDefinition ApplyBleedOnHitChance;
    FGameplayEffectAttributeCaptureDefinition ApplyPoisonOnHitChance;
    FGameplayEffectAttributeCaptureDefinition ApplySlowOnHitChance;
    FGameplayEffectAttributeCaptureDefinition ApplyFreezeOnHitChance;
    FGameplayEffectAttributeCaptureDefinition ApplyStunOnHitChance;
    FGameplayEffectAttributeCaptureDefinition ApplyWeakenOnHitChance;
    FGameplayEffectAttributeCaptureDefinition ApplyTerrifyOnHitChance;

    FMythicDamageCalcStatics() {
        CriticalHitChance = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetCriticalHitChanceAttribute(),
                                                                      EGameplayEffectAttributeCaptureSource::Source, false);
        ApplyBurnOnHitChance = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetApplyBurnOnHitChanceAttribute(),
                                                                         EGameplayEffectAttributeCaptureSource::Source, false);
        ApplyBleedOnHitChance = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetApplyBleedOnHitChanceAttribute(),
                                                                          EGameplayEffectAttributeCaptureSource::Source, false);
        ApplyPoisonOnHitChance = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetApplyPoisonOnHitChanceAttribute(),
                                                                           EGameplayEffectAttributeCaptureSource::Source, false);
        ApplySlowOnHitChance = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetApplySlowOnHitChanceAttribute(),
                                                                         EGameplayEffectAttributeCaptureSource::Source, false);
        ApplyFreezeOnHitChance = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetApplyFreezeOnHitChanceAttribute(),
                                                                           EGameplayEffectAttributeCaptureSource::Source, false);
        ApplyStunOnHitChance = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetApplyStunOnHitChanceAttribute(),
                                                                         EGameplayEffectAttributeCaptureSource::Source, false);
        ApplyWeakenOnHitChance = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetApplyWeakenOnHitChanceAttribute(),
                                                                           EGameplayEffectAttributeCaptureSource::Source, false);
        ApplyTerrifyOnHitChance = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetApplyTerrifyOnHitChanceAttribute(),
                                                                            EGameplayEffectAttributeCaptureSource::Source, false);
    }
};

static FMythicDamageCalcStatics &MythicDamageCalcStatics() {
    static FMythicDamageCalcStatics Statics;
    return Statics;
}

UMythicDamageCalculation::UMythicDamageCalculation() {
    RelevantAttributesToCapture.Add(MythicDamageCalcStatics().CriticalHitChance);
    RelevantAttributesToCapture.Add(MythicDamageCalcStatics().ApplyBurnOnHitChance);
    RelevantAttributesToCapture.Add(MythicDamageCalcStatics().ApplyBleedOnHitChance);
    RelevantAttributesToCapture.Add(MythicDamageCalcStatics().ApplyPoisonOnHitChance);
    RelevantAttributesToCapture.Add(MythicDamageCalcStatics().ApplySlowOnHitChance);
    RelevantAttributesToCapture.Add(MythicDamageCalcStatics().ApplyFreezeOnHitChance);
    RelevantAttributesToCapture.Add(MythicDamageCalcStatics().ApplyStunOnHitChance);
    RelevantAttributesToCapture.Add(MythicDamageCalcStatics().ApplyWeakenOnHitChance);
    RelevantAttributesToCapture.Add(MythicDamageCalcStatics().ApplyTerrifyOnHitChance);
}

void UMythicDamageCalculation::Execute_Implementation(const FGameplayEffectCustomExecutionParameters &ExecutionParams,
                                                      FGameplayEffectCustomExecutionOutput &OutExecutionOutput) const {
    Super::Execute_Implementation(ExecutionParams, OutExecutionOutput);
    UE_LOG(Myth, Verbose, TEXT("Calculating damage"));

    auto Spec = ExecutionParams.GetOwningSpecForPreExecuteMod();
    FMythicGameplayEffectContext *MythicContext = FMythicGameplayEffectContext::ExtractEffectContext(Spec->GetContext());
    if (!MythicContext) {
        UE_LOG(Myth, Error, TEXT("DamageCalculation:: non-Mythic/empty effect context - aborting"));
        return;
    }

    const FGameplayTagContainer *SourceTags = Spec->CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer *TargetTags = Spec->CapturedTargetTags.GetAggregatedTags();

    FAggregatorEvaluateParameters EvaluateParameters;
    EvaluateParameters.SourceTags = SourceTags;
    EvaluateParameters.TargetTags = TargetTags;

    float CriticalHitChance = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageCalcStatics().CriticalHitChance, EvaluateParameters, CriticalHitChance);
    float ApplyBurnOnHitChance = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageCalcStatics().ApplyBurnOnHitChance, EvaluateParameters, ApplyBurnOnHitChance);
    float ApplyBleedOnHitChance = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageCalcStatics().ApplyBleedOnHitChance, EvaluateParameters, ApplyBleedOnHitChance);
    float ApplyPoisonOnHitChance = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageCalcStatics().ApplyPoisonOnHitChance, EvaluateParameters, ApplyPoisonOnHitChance);
    float ApplySlowOnHitChance = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageCalcStatics().ApplySlowOnHitChance, EvaluateParameters, ApplySlowOnHitChance);
    float ApplyFreezeOnHitChance = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageCalcStatics().ApplyFreezeOnHitChance, EvaluateParameters, ApplyFreezeOnHitChance);
    float ApplyStunOnHitChance = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageCalcStatics().ApplyStunOnHitChance, EvaluateParameters, ApplyStunOnHitChance);
    float ApplyWeakenOnHitChance = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageCalcStatics().ApplyWeakenOnHitChance, EvaluateParameters, ApplyWeakenOnHitChance);
    float ApplyTerrifyOnHitChance = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageCalcStatics().ApplyTerrifyOnHitChance, EvaluateParameters, ApplyTerrifyOnHitChance);

    // Chance past certainty is spent on buildup rather than discarded. Tracked from whichever ailment procced
    // with the most to spare, so a build stacking one element keeps being paid for stacking it.
    float StatusOverflow = 0.0f;
    const auto ProcRoll = [&StatusOverflow](float Chance) {
        const bool bProcced = MythicCombat::RollSucceeds(MythicCombat::ClampProbability(Chance), FMath::FRand());
        if (bProcced) {
            StatusOverflow = FMath::Max(StatusOverflow, MythicCombat::ProbabilityOverflow(Chance));
        }
        return bProcced;
    };

    // Crit is not an ailment, so its overflow does not feed ailment buildup.
    MythicContext->SetCriticalHit(MythicCombat::RollSucceeds(MythicCombat::ClampProbability(CriticalHitChance), FMath::FRand()));
    MythicContext->SetBleed(MythicContext->IsBleed() || ProcRoll(ApplyBleedOnHitChance));
    MythicContext->SetBurn(MythicContext->IsBurn() || ProcRoll(ApplyBurnOnHitChance));
    MythicContext->SetPoison(MythicContext->IsPoison() || ProcRoll(ApplyPoisonOnHitChance));
    MythicContext->SetSlow(MythicContext->IsSlow() || ProcRoll(ApplySlowOnHitChance));
    MythicContext->SetFreeze(MythicContext->IsFreeze() || ProcRoll(ApplyFreezeOnHitChance));
    MythicContext->SetStun(MythicContext->IsStun() || ProcRoll(ApplyStunOnHitChance));
    MythicContext->SetWeaken(MythicContext->IsWeaken() || ProcRoll(ApplyWeakenOnHitChance));
    MythicContext->SetTerrify(MythicContext->IsTerrify() || ProcRoll(ApplyTerrifyOnHitChance));

    MythicContext->SetStatusOverflow(StatusOverflow);
}
