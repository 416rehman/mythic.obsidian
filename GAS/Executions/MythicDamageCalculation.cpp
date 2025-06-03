// 


#include "MythicDamageCalculation.h"

#include "Mythic.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"

struct FMythicDamageCalcStatics {
    /** Source Attributes */
    FGameplayEffectAttributeCaptureDefinition Power;
    FGameplayEffectAttributeCaptureDefinition DamagePerHit;
    FGameplayEffectAttributeCaptureDefinition CriticalHitChance;
    FGameplayEffectAttributeCaptureDefinition ApplyBurnOnHitChance;
    FGameplayEffectAttributeCaptureDefinition ApplyPoisonOnHitChance;
    FGameplayEffectAttributeCaptureDefinition ApplySlowOnHitChance;
    FGameplayEffectAttributeCaptureDefinition ApplyFreezeOnHitChance;
    FGameplayEffectAttributeCaptureDefinition ApplyStunOnHitChance;
    FGameplayEffectAttributeCaptureDefinition ApplyWeakenOnHitChance;
    FGameplayEffectAttributeCaptureDefinition ApplyTerrifyOnHitChance;

    FMythicDamageCalcStatics() {
        Power = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetPowerAttribute(), EGameplayEffectAttributeCaptureSource::Source,
                                                          false);
        DamagePerHit = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetDamagePerHitAttribute(),
                                                                    EGameplayEffectAttributeCaptureSource::Source, false);
        CriticalHitChance = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetCriticalHitChanceAttribute(),
                                                                      EGameplayEffectAttributeCaptureSource::Source, false);
        ApplyBurnOnHitChance = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetApplyBurnOnHitChanceAttribute(),
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
    RelevantAttributesToCapture.Add(MythicDamageCalcStatics().Power);
    RelevantAttributesToCapture.Add(MythicDamageCalcStatics().DamagePerHit);
    RelevantAttributesToCapture.Add(MythicDamageCalcStatics().CriticalHitChance);
    RelevantAttributesToCapture.Add(MythicDamageCalcStatics().ApplyBurnOnHitChance);
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
    UE_LOG(Mythic, Warning, TEXT("Calculating damage"));

    auto Spec = ExecutionParams.GetOwningSpecForPreExecuteMod();
    auto ContextHandle = Spec->GetContext().Get();
    auto MythicContext = static_cast<FMythicGameplayEffectContext *>(ContextHandle);

    const FGameplayTagContainer *SourceTags = Spec->CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer *TargetTags = Spec->CapturedTargetTags.GetAggregatedTags();

    FAggregatorEvaluateParameters EvaluateParameters;
    EvaluateParameters.SourceTags = SourceTags;
    EvaluateParameters.TargetTags = TargetTags;

    // Calculated Base Damage = Power * (Random(MinDamagePerHit, MaxDamagePerHit))
    float Power = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageCalcStatics().Power, EvaluateParameters, Power);
    if (Power <= 0.0f) {
        UE_LOG(Mythic, Error, TEXT("UMythicDamageCalculation::Execute_Implementation: Power is <= 0.0f - Overriding to 1.0f"));
        Power = 1.0f;
    }
    float DamagePerHit = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageCalcStatics().DamagePerHit, EvaluateParameters, DamagePerHit);
    float CriticalHitChance = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageCalcStatics().CriticalHitChance, EvaluateParameters, CriticalHitChance);
    float ApplyBurnOnHitChance = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageCalcStatics().ApplyBurnOnHitChance, EvaluateParameters, ApplyBurnOnHitChance);
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

    // Apply Critical damage
    MythicContext->SetCriticalHit(FMath::FRand() <= CriticalHitChance);
    // Apply bleed
    MythicContext->SetBleed(MythicContext->IsBleed() || FMath::FRand() <= ApplyPoisonOnHitChance);
    // Apply Burn
    MythicContext->SetBurn(MythicContext->IsBurn() || FMath::FRand() <= ApplyBurnOnHitChance);
    // Apply poison
    MythicContext->SetPoison(MythicContext->IsPoison() || FMath::FRand() <= ApplyPoisonOnHitChance);
    // Apply slow
    MythicContext->SetSlow(MythicContext->IsSlow() || FMath::FRand() <= ApplySlowOnHitChance);
    // Apply freeze
    MythicContext->SetFreeze(MythicContext->IsFreeze() || FMath::FRand() <= ApplyFreezeOnHitChance);
    // Apply stun
    MythicContext->SetStun(MythicContext->IsStun() || FMath::FRand() <= ApplyStunOnHitChance);
    // Apply weaken
    MythicContext->SetWeaken(MythicContext->IsWeaken() || FMath::FRand() <= ApplyWeakenOnHitChance);
    // Apply terrify
    MythicContext->SetTerrify(MythicContext->IsTerrify() || FMath::FRand() <= ApplyTerrifyOnHitChance);
}
