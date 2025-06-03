// 


#include "MythicDamageApplication.h"

#include "Mythic.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "GAS/MythicTags_GAS.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"

struct FMythicGameplayEffectContext;

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
    FGameplayEffectAttributeCaptureDefinition IncreasedDamageToEnemiesUnderStatusEffects;
    FGameplayEffectAttributeCaptureDefinition BonusDamageToSuperiorEnemies;

    FDamageApplicationStatics() {
        Power = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetPowerAttribute(), EGameplayEffectAttributeCaptureSource::Source,
                                                          false);
        DamagePerHit = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetDamagePerHitAttribute(),
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
        IncreasedDamageToEnemiesUnderStatusEffects = FGameplayEffectAttributeCaptureDefinition(
            UMythicAttributeSet_Offense::GetIncreasedDamageToEnemiesUnderStatusEffectsAttribute(), EGameplayEffectAttributeCaptureSource::Source, false);
        BonusDamageToSuperiorEnemies = FGameplayEffectAttributeCaptureDefinition(UMythicAttributeSet_Offense::GetBonusDamageToSuperiorEnemiesAttribute(),
                                                                                 EGameplayEffectAttributeCaptureSource::Source, false);
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
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().IncreasedDamageToEnemiesUnderStatusEffects);
    RelevantAttributesToCapture.Add(MythicDamageApplicationStatics().BonusDamageToSuperiorEnemies);
}

void UMythicDamageApplication::Execute_Implementation(const FGameplayEffectCustomExecutionParameters &ExecutionParams,
                                                      FGameplayEffectCustomExecutionOutput &OutExecutionOutput) const {
    Super::Execute_Implementation(ExecutionParams, OutExecutionOutput);
    UE_LOG(Mythic, Warning, TEXT("DamageApplication:: Applying damage"));

    FGameplayEffectSpec *Spec = ExecutionParams.GetOwningSpecForPreExecuteMod();
    FGameplayEffectContext *Context = Spec->GetContext().Get();
    FMythicGameplayEffectContext *MythicContext = static_cast<FMythicGameplayEffectContext *>(Context);

    // Send event with tag BeforeDamageDealt
    auto SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
    auto TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();

    const FGameplayTagContainer *SourceTags = Spec->CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer *TargetTags = Spec->CapturedTargetTags.GetAggregatedTags();

    FAggregatorEvaluateParameters EvaluateParameters;
    EvaluateParameters.SourceTags = SourceTags;
    EvaluateParameters.TargetTags = TargetTags;

    float Power = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageApplicationStatics().Power, EvaluateParameters, Power);
    float DmgPerHit = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageApplicationStatics().DamagePerHit, EvaluateParameters, DmgPerHit);
    float CriticalHitDamage = 0.5f;
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
    float IncreasedDamageToEnemiesUnderStatusEffects = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageApplicationStatics().IncreasedDamageToEnemiesUnderStatusEffects, EvaluateParameters,
                                                               IncreasedDamageToEnemiesUnderStatusEffects);
    float BonusDamageToSuperiorEnemies = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MythicDamageApplicationStatics().BonusDamageToSuperiorEnemies, EvaluateParameters,
                                                               BonusDamageToSuperiorEnemies);

    auto FinalDamage = FMath::Max(1.0f, Power) * FMath::RandRange(DmgPerHit, DmgPerHit * 1.5f);
    UE_LOG(Mythic, Log, TEXT("DamageApplication:: Damage %f = DmgPerHit (%f - %f) * Power (%f)"), FinalDamage, DmgPerHit, DmgPerHit * 1.5f, Power);
    if (MythicContext->IsCriticalHit()) {
        FinalDamage += FinalDamage * CriticalHitDamage;
        UE_LOG(Mythic, Warning, TEXT("DamageApplication:: Critical hit! Damage increased by %f Percent"), CriticalHitDamage/100.0f);
    }

    UE_LOG(Mythic, Warning, TEXT("DamageApplication:: Final damage: %f"), FinalDamage);

    // TODO: Apply status effects to the target
    // TODO: Modify damage based on IncreasedDamageToEnemiesUnderStatusEffects, BonusDamageToSuperiorEnemies, WeaponType, etc.

    // Output the final damage
    OutExecutionOutput.
        AddOutputModifier(FGameplayModifierEvaluatedData(UMythicAttributeSet_Life::GetHealthAttribute(), EGameplayModOp::Additive, -FinalDamage));

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
    // UE_LOG(Mythic, Warning, TEXT("Hit event sent to source %d abilities"), activations);
}
