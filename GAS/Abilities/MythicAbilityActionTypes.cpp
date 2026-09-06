#include "MythicAbilityActionTypes.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/Executions/MythicDamageCalculation.h"
#include "GAS/Executions/MythicDamageApplication.h"
#include "GAS/Effects/MythicStatusRegistry.h"
#include "GAS/Effects/MythicStatusEffectDefinition.h"
#include "Settings/MythicCombatSettings.h"
#include "Settings/MythicDeveloperSettings.h"

bool FMythicAbilityActionProfile::Validate(TArray<FText>& Errors) const {
    const int32 Before=Errors.Num();
    auto Fail=[&](const TCHAR* Message){ Errors.Add(FText::Format(NSLOCTEXT("Mythic","InvalidAbilityAction","Action {0}: {1}"),FText::FromName(ActionId),FText::FromString(Message))); };
    if (ActionId.IsNone() || Facets.IsEmpty()) Fail(TEXT("requires a local ID and truthful action facets"));
    if (Work==EMythicAbilityActionWork::Utility) {
        for (const auto* Name : {TEXT("Ability.Action.Effect.Damage"), TEXT("Ability.Action.Effect.ApplyStatus")}) {
            const auto OffensiveFacet=FGameplayTag::RequestGameplayTag(Name,false);
            if (OffensiveFacet.IsValid() && Facets.HasTag(OffensiveFacet)) Fail(TEXT("utility facets cannot advertise damage or status application"));
        }
        if (MinimumDamage!=0 || MaximumDamage!=0 || Damage.DamageApplicationEffect || Damage.DamageCalculationEffect
            || BuildupEffect || BuildupAmount!=0 || StatusApplication!=EMythicAbilityStatusApplication::None
            || StatusType.IsValid() || StatusChance!=0 || StatusDuration!=0 || bScaleWithPrimaryStats)
            Fail(TEXT("utility cannot declare damage, primary-stat scaling, status application or offensive effects"));
        const auto Self=FGameplayTag::RequestGameplayTag(TEXT("Ability.Action.Target.Self"),false);
        if (!Self.IsValid() || !Facets.HasTagExact(Self) || MaximumTargets!=1 || Shares.Num()!=1 || Shares[0]!=1.0f)
            Fail(TEXT("utility requires one whole operation and one self recipient"));
        if (Parameters.Num()>16 || SourceAttributes.Num()>16) Fail(TEXT("utility declarations exceed the bounded capacity"));
        TSet<FName> Names;
        for (const auto& Parameter:Parameters) {
            if (Parameter.Name.IsNone() || Names.Contains(Parameter.Name) || !FMath::IsFinite(Parameter.Value)
                || !FMath::IsFinite(Parameter.Minimum) || !FMath::IsFinite(Parameter.Maximum)
                || Parameter.Minimum>Parameter.Value || Parameter.Value>Parameter.Maximum)
                Fail(TEXT("utility parameter must have a unique name and finite bounded value"));
            Names.Add(Parameter.Name);
        }
        TSet<FGameplayAttribute> Attributes;
        for (const auto& Attribute:SourceAttributes) {
            if (Attributes.Contains(Attribute) || !FMythicAbilityActionSource::IsSupportedStat(Attribute))
                Fail(TEXT("utility source attributes must be unique and supported by the action-stat policy"));
            Attributes.Add(Attribute);
        }
        return Errors.Num()==Before;
    }
    if (Work!=EMythicAbilityActionWork::Offensive || !Parameters.IsEmpty() || !SourceAttributes.IsEmpty())
        Fail(TEXT("utility declarations require the Utility work policy"));
    if (!FMath::IsFinite(MinimumDamage) || !FMath::IsFinite(MaximumDamage) || MinimumDamage<0 || MinimumDamage>MaximumDamage || MaximumDamage>1000000) Fail(TEXT("invalid authored damage range"));
    if (!Damage.DamageApplicationEffect || !Damage.DamageCalculationEffect) Fail(TEXT("action requires calculation and application effects, including zero-damage status work"));
    for (auto Effect:{Damage.DamageApplicationEffect,Damage.DamageCalculationEffect,BuildupEffect}) if (Effect && Effect.GetDefaultObject()->DurationPolicy!=EGameplayEffectDurationType::Instant) Fail(TEXT("action work effects must be instant"));
    if (MaximumTargets<1 || MaximumTargets>32 || Shares.IsEmpty() || Shares.Num()>8) Fail(TEXT("work limits exceed bounded action capacity"));
    double Total=0;
    for (float Share:Shares) { if (!FMath::IsFinite(Share) || Share<=0 || Share>1) Fail(TEXT("invalid share")); Total+=Share; }
    if (!FMath::IsFinite(Total) || Total>1.00001) Fail(TEXT("shares cannot exceed the whole action budget"));
    if (!FMath::IsFinite(StatusChance) || StatusChance<0 || StatusChance>1 || !FMath::IsFinite(StatusDuration) || StatusDuration<0 || StatusDuration>120) Fail(TEXT("invalid status chance or duration"));
    if (StatusApplication!=EMythicAbilityStatusApplication::None && !StatusType.IsValid()) Fail(TEXT("status work requires a canonical status ID"));
    if (StatusApplication==EMythicAbilityStatusApplication::Buildup && (!BuildupEffect || !FMath::IsFinite(BuildupAmount) || BuildupAmount<=0 || BuildupAmount>100000)) Fail(TEXT("buildup requires an effect and a finite positive budget"));
    if (StatusApplication==EMythicAbilityStatusApplication::Direct && Shares.Num()!=1) Fail(TEXT("direct status admission cannot be multiplied by projectile shares; use partitioned buildup"));
    return Errors.Num()==Before;
}


bool FMythicAbilityActionSource::IsSupportedStat(const FGameplayAttribute& Attribute) {
    if (!Attribute.IsValid()) return false;
    // Direct weapon projection is deliberately absent from authored ability actions.
    for (const auto& Weapon:{UMythicAttributeSet_Offense::GetDamagePerHitAttribute(),UMythicAttributeSet_Offense::GetBonusSwordDamageAttribute(),
        UMythicAttributeSet_Offense::GetBonusAxeDamageAttribute(),UMythicAttributeSet_Offense::GetBonusDaggerDamageAttribute(),
        UMythicAttributeSet_Offense::GetBonusSickleDamageAttribute(),UMythicAttributeSet_Offense::GetBonusSpearDamageAttribute(),UMythicAttributeSet_Offense::GetBonusHammerDamageAttribute()})
        if (Attribute==Weapon) return false;
    if (GetDefault<UMythicDamageApplication>()->IsSourceAttribute(Attribute) || GetDefault<UMythicDamageCalculation>()->IsSourceAttribute(Attribute)
        || Attribute==UMythicAttributeSet_Offense::GetStatusThresholdReductionAttribute()) return true;
    for (const auto& Row:GetDefault<UMythicCombatSettings>()->StatContributions.Contributions) if (Row.SourceStat==Attribute && Row.TargetAttribute==UMythicAttributeSet_Offense::GetDamagePerHitAttribute()) return true;
    // Explicit canonical authoring dependency. Validation must not depend on a prior GameInstance cache hit.
    // Registry compilation may resolve this once; action execution never loads content.
    if (const auto* Library=GetDefault<UMythicDeveloperSettings>()->StatusEffectLibrary.LoadSynchronous())
        for (const UMythicStatusEffectDefinition* Status:Library->Statuses) if (Status && (Status->BonusDamageAttribute==Attribute || Status->DurationMultiplierAttribute==Attribute || Status->ControlMagnitudeAttribute==Attribute)) return true;
    return false;
}

void FMythicAbilityActionSource::RetainEffectDefinitions(const UAbilitySystemComponent* ASC) {
    if (!ASC || EffectDefinitions) return;
    auto Definitions=MakeShared<TArray<TStrongObjectPtr<const UGameplayEffect>>>();
    TSet<const UGameplayEffect*> Seen;
    for (const auto Handle:ASC->GetActiveEffects(FGameplayEffectQuery())) {
        const auto* Active=ASC->GetActiveGameplayEffect(Handle);
        const auto* Definition=Active ? Active->Spec.Def.Get() : nullptr;
        if (Definition && !Seen.Contains(Definition)) { Seen.Add(Definition); Definitions->Emplace(Definition); }
    }
    EffectDefinitions=Definitions;
}

bool FMythicAbilityActionSource::CaptureStat(UAbilitySystemComponent* ASC,const FGameplayAttribute& Attribute,
    const FGameplayTagContainer& SourceTags,const TMap<FGameplayAttribute,float>& ReplacementBases,float& Out) {
    Out=0;
    if (!ASC || !Attribute.IsValid() || !ASC->HasAttributeSetForAttribute(Attribute)) return false;
    FGameplayEffectAttributeCaptureSpec Capture(FGameplayEffectAttributeCaptureDefinition(Attribute,EGameplayEffectAttributeCaptureSource::Source,true));
    ASC->CaptureAttributeForGameplayEffect(Capture);
    FAggregatorEvaluateParameters Evaluation; Evaluation.SourceTags=&SourceTags;
    const float* Base=ReplacementBases.Find(Attribute);
    const bool Valid=Base ? Capture.AttemptCalculateAttributeMagnitudeWithBase(Evaluation,*Base,Out) : Capture.AttemptCalculateAttributeMagnitude(Evaluation,Out);
    return Valid && FMath::IsFinite(Out);
}


bool FMythicAbilityActionSource::EvaluateStat(const FGameplayAttribute& Attribute,const FGameplayTagContainer& SourceTags,
    const FGameplayTagContainer* TargetTags,float& Out) const {
    if (const auto* Capture=Captures.Find(Attribute)) {
        FAggregatorEvaluateParameters Evaluation; Evaluation.SourceTags=&SourceTags; Evaluation.TargetTags=TargetTags;
        const float* Base=EquipmentBases.Find(Attribute);
        const bool Valid=Base ? Capture->AttemptCalculateAttributeMagnitudeWithBase(Evaluation,*Base,Out) : Capture->AttemptCalculateAttributeMagnitude(Evaluation,Out);
        return Valid && FMath::IsFinite(Out);
    }
    if (const float* Value=Stats.Find(Attribute)) { Out=*Value; return FMath::IsFinite(Out); }
    return false;
}
