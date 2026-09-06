

#include "MythicDamageCalculation.h"
#include "AbilitySystemComponent.h"

#include "Mythic.h"
#include "GAS/MythicTags_GAS.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "Engine/World.h"
#include "GAS/Executions/MythicCombatRoll.h"
#include "Settings/MythicCombatSettings.h"
#include "GameModes/GameState/MythicGameState.h"

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

bool UMythicDamageCalculation::IsSourceAttribute(const FGameplayAttribute& Attribute) const {
    return RelevantAttributesToCapture.ContainsByPredicate([&](const auto& Capture) { return Capture.AttributeSource==EGameplayEffectAttributeCaptureSource::Source && Capture.AttributeToCapture==Attribute; });
}

void UMythicDamageCalculation::CaptureActionAttributes(const UAbilitySystemComponent* ASC,FMythicAbilityActionSource& Out) const {
    if (!ASC) return;
    Out.RetainEffectDefinitions(ASC);
    for (const auto& Capture:RelevantAttributesToCapture) {
        if (Capture.AttributeSource!=EGameplayEffectAttributeCaptureSource::Source || Out.Stats.Contains(Capture.AttributeToCapture)) continue;
        const float Value=ASC->HasAttributeSetForAttribute(Capture.AttributeToCapture) ? ASC->GetNumericAttribute(Capture.AttributeToCapture) : 0;
        Out.Stats.Add(Capture.AttributeToCapture,FMath::IsFinite(Value) ? Value : 0);
        if (ASC->HasAttributeSetForAttribute(Capture.AttributeToCapture)) {
            FGameplayEffectAttributeCaptureSpec Sealed(FGameplayEffectAttributeCaptureDefinition(Capture.AttributeToCapture,EGameplayEffectAttributeCaptureSource::Source,true));
            const_cast<UAbilitySystemComponent*>(ASC)->CaptureAttributeForGameplayEffect(Sealed);
            Out.Captures.Add(Capture.AttributeToCapture,Sealed);
        }
    }
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

    if (MythicContext->ActionSnapshot) {
        ResolveActionIntents(*MythicContext,nullptr); return;
    }
    const FGameplayTagContainer *SourceTags = Spec->CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer *TargetTags = Spec->CapturedTargetTags.GetAggregatedTags();

    FAggregatorEvaluateParameters EvaluateParameters;
    EvaluateParameters.SourceTags = SourceTags;
    EvaluateParameters.TargetTags = TargetTags;
    // Authoritative source attributes are sealed once at commit; defenses still evaluate on the recipient.
    const auto CaptureMagnitude=[&](const FGameplayEffectAttributeCaptureDefinition& Capture,const FAggregatorEvaluateParameters& Evaluation,float& Out) {
        if (MythicContext->ActionSnapshot && MythicContext->ActionSnapshot->Source
            && Capture.AttributeSource==EGameplayEffectAttributeCaptureSource::Source) {
            const auto& Source=*MythicContext->ActionSnapshot->Source;
            const auto& Tags=Evaluation.SourceTags ? *Evaluation.SourceTags : Source.StateTags;
            return Source.EvaluateStat(Capture.AttributeToCapture,Tags,Evaluation.TargetTags,Out);
        }
        return ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Capture,Evaluation,Out);
    };

    float CriticalHitChance = 0.0f;
    CaptureMagnitude(MythicDamageCalcStatics().CriticalHitChance, EvaluateParameters, CriticalHitChance);
    float ApplyBurnOnHitChance = 0.0f;
    CaptureMagnitude(MythicDamageCalcStatics().ApplyBurnOnHitChance, EvaluateParameters, ApplyBurnOnHitChance);
    float ApplyBleedOnHitChance = 0.0f;
    CaptureMagnitude(MythicDamageCalcStatics().ApplyBleedOnHitChance, EvaluateParameters, ApplyBleedOnHitChance);
    float ApplyPoisonOnHitChance = 0.0f;
    CaptureMagnitude(MythicDamageCalcStatics().ApplyPoisonOnHitChance, EvaluateParameters, ApplyPoisonOnHitChance);
    float ApplySlowOnHitChance = 0.0f;
    CaptureMagnitude(MythicDamageCalcStatics().ApplySlowOnHitChance, EvaluateParameters, ApplySlowOnHitChance);
    float ApplyFreezeOnHitChance = 0.0f;
    CaptureMagnitude(MythicDamageCalcStatics().ApplyFreezeOnHitChance, EvaluateParameters, ApplyFreezeOnHitChance);
    float ApplyStunOnHitChance = 0.0f;
    CaptureMagnitude(MythicDamageCalcStatics().ApplyStunOnHitChance, EvaluateParameters, ApplyStunOnHitChance);
    float ApplyWeakenOnHitChance = 0.0f;
    CaptureMagnitude(MythicDamageCalcStatics().ApplyWeakenOnHitChance, EvaluateParameters, ApplyWeakenOnHitChance);
    float ApplyTerrifyOnHitChance = 0.0f;
    CaptureMagnitude(MythicDamageCalcStatics().ApplyTerrifyOnHitChance, EvaluateParameters, ApplyTerrifyOnHitChance);

    /**
     * On-hit chances stack from gear, so they are bent toward certainty rather than clamped at it. Past the soft
     * cap each further point buys less than the last and the curve never reaches 1, which keeps stacking worth
     * something without ever removing the roll.
     */
    const float SoftCap = GetDefault<UMythicCombatSettings>()->ProbabilitySoftCap;
    const auto ProcRoll = [SoftCap](float Chance) {
        return MythicCombat::RollSucceeds(MythicCombat::DiminishProbability(Chance, SoftCap), FMath::FRand());
    };

    MythicContext->SetCalculatedCriticalHit(ProcRoll(CriticalHitChance));
    MythicContext->SetCalculatedBleed(MythicContext->IsBleed() || ProcRoll(ApplyBleedOnHitChance));
    MythicContext->SetCalculatedBurn(MythicContext->IsBurn() || ProcRoll(ApplyBurnOnHitChance));
    MythicContext->SetCalculatedPoison(MythicContext->IsPoison() || ProcRoll(ApplyPoisonOnHitChance));
    MythicContext->SetCalculatedSlow(MythicContext->IsSlow() || ProcRoll(ApplySlowOnHitChance));
    MythicContext->SetCalculatedFreeze(MythicContext->IsFreeze() || ProcRoll(ApplyFreezeOnHitChance));
    MythicContext->SetCalculatedStun(MythicContext->IsStun() || ProcRoll(ApplyStunOnHitChance));
    MythicContext->SetCalculatedWeaken(MythicContext->IsWeaken() || ProcRoll(ApplyWeakenOnHitChance));
    MythicContext->SetCalculatedTerrify(MythicContext->IsTerrify() || ProcRoll(ApplyTerrifyOnHitChance));
}


void UMythicDamageCalculation::ResolveActionIntents(FMythicGameplayEffectContext& Context,const FGameplayTagContainer* TargetTags) {
    if (!Context.ActionSnapshot || !Context.ActionSnapshot->Source) return;
    const auto& Snapshot=*Context.ActionSnapshot; const auto& Source=*Snapshot.Source;
    FGameplayTagContainer Tags=Source.StateTags; Tags.AppendTags(Snapshot.Facets); Tags.AddTag(GAS_ABILITY_TYPE_SKILL);
    const auto& Defs=MythicDamageCalcStatics();
    const FGameplayEffectAttributeCaptureDefinition Captures[]={Defs.CriticalHitChance,Defs.ApplyBurnOnHitChance,Defs.ApplyBleedOnHitChance,
        Defs.ApplyPoisonOnHitChance,Defs.ApplySlowOnHitChance,Defs.ApplyFreezeOnHitChance,Defs.ApplyStunOnHitChance,Defs.ApplyWeakenOnHitChance,Defs.ApplyTerrifyOnHitChance};
    bool Intent[9]={};
    for (int32 Index=0;Index<9;++Index) {
        float Chance=0; Source.EvaluateStat(Captures[Index].AttributeToCapture,Tags,TargetTags,Chance);
        Chance=MythicCombat::DiminishProbability(Chance,GetDefault<UMythicCombatSettings>()->ProbabilitySoftCap);
        Intent[Index]=MythicCombat::RollSucceeds(Chance,Snapshot.IntentRolls[Index]);
    }
    if (!Context.HasIntentOverride(0)) Context.SetCalculatedCriticalHit(Intent[0]);
    if (!Context.HasIntentOverride(1)) Context.SetCalculatedBurn(Intent[1]);
    if (!Context.HasIntentOverride(2)) Context.SetCalculatedBleed(Intent[2]);
    if (!Context.HasIntentOverride(3)) Context.SetCalculatedPoison(Intent[3]);
    if (!Context.HasIntentOverride(4)) Context.SetCalculatedSlow(Intent[4]);
    if (!Context.HasIntentOverride(5)) Context.SetCalculatedFreeze(Intent[5]);
    if (!Context.HasIntentOverride(6)) Context.SetCalculatedStun(Intent[6]);
    if (!Context.HasIntentOverride(7)) Context.SetCalculatedWeaken(Intent[7]);
    if (!Context.HasIntentOverride(8)) Context.SetCalculatedTerrify(Intent[8]);
}
