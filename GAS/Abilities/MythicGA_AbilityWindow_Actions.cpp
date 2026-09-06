#include "MythicGA_AbilityWindow.h"
#include "AbilitySystemGlobals.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "GAS/MythicStatContribution.h"
#include "GAS/MythicTags_GAS.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/Effects/MythicStatusRegistry.h"
#include "GAS/Effects/MythicStatusEffectDefinition.h"
#include "GAS/Executions/MythicCombatRoll.h"
#include "GAS/Executions/MythicDamageApplication.h"
#include "GAS/Executions/MythicDamageCalculation.h"
#include "Progression/Skills/MythicSkillDefinition.h"
#include "Settings/MythicCombatSettings.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Itemization/Affixes/MythicAffixApplicationComponent.h"
#include "Misc/ScopeExit.h"

float UMythicGA_AbilityWindow::GetAbilityParameter(FName Name,float Fallback) const {
    if (const float* Value=Parameters.Find(Name)) return *Value;
    return FMath::IsFinite(Fallback) ? Fallback : 0;
}

bool UMythicGA_AbilityWindow::CaptureActionSource() {
    if (ActionProfiles.IsEmpty()) return true;
    if (bCleaned || !CurrentActorInfo || !CurrentActorInfo->IsNetAuthority() || !WindowAvatar.IsValid()
        || WindowAvatar.Get()!=CurrentActorInfo->AvatarActor.Get() || !CheckTargetIncarnation(WindowAvatar.Get())) return false;
    auto* ASC=CurrentActorInfo->AbilitySystemComponent.Get();
    auto Source=MakeShared<FMythicAbilityActionSource>();
    ASC->GetOwnedGameplayTags(Source->StateTags);
    Source->AbilityFacets=Classification;
    bool bHasOffensiveWork=false;
    for (const auto& Profile:ActionProfiles) {
        TArray<FText> Errors;
        if (!Profile.Value.Validate(Errors)) return false;
        bHasOffensiveWork|=Profile.Value.Work==EMythicAbilityActionWork::Offensive;
    }
    if (bHasOffensiveWork) {
    GetDefault<UMythicDamageCalculation>()->CaptureActionAttributes(ASC,*Source);
    GetDefault<UMythicDamageApplication>()->CaptureActionAttributes(ASC,*Source);
    const auto* Settings=GetDefault<UMythicCombatSettings>();
    for (const auto& Contribution:Settings->StatContributions.Contributions) {
        const auto& Attribute=Contribution.SourceStat;
        if (!Attribute.IsValid() || Source->Stats.Contains(Attribute)) continue;
        const float Value=ASC->HasAttributeSetForAttribute(Attribute) ? ASC->GetNumericAttribute(Attribute) : 0;
        Source->Stats.Add(Attribute,FMath::IsFinite(Value) ? Value : 0);
    }
    auto CaptureStatusStat=[&](const FGameplayAttribute& Attribute) {
        if (!Attribute.IsValid() || Source->Stats.Contains(Attribute) || !ASC->HasAttributeSetForAttribute(Attribute)) return;
        const float Value=ASC->GetNumericAttribute(Attribute); Source->Stats.Add(Attribute,FMath::IsFinite(Value) ? Value : 0);
    };
    CaptureStatusStat(UMythicAttributeSet_Offense::GetStatusThresholdReductionAttribute());
    if (const auto* GI=GetWorld()->GetGameInstance()) if (const auto* Registry=GI->GetSubsystem<UMythicStatusRegistry>()) {
        for (const auto* Status:Registry->GetAllStatuses()) if (Status) {
            CaptureStatusStat(Status->BonusDamageAttribute); CaptureStatusStat(Status->DurationMultiplierAttribute); CaptureStatusStat(Status->ControlMagnitudeAttribute);
        }
    }
    Source->ControlDamageMultiplier=UMythicStatusRegistry::GetControlReductionMultiplier(ASC,GAS_DEBUFF_WEAKENED,Settings->WeakenedDamagePenalty);
    }
    for (const auto& Profile:ActionProfiles) for (const auto& Attribute:Profile.Value.SourceAttributes) {
        if (!ASC->HasAttributeSetForAttribute(Attribute)) return false;
        const float Value=ASC->GetNumericAttribute(Attribute);
        if (!FMath::IsFinite(Value)) return false;
        Source->Stats.FindOrAdd(Attribute)=Value;
    }
    if (!Source->Stats.IsEmpty()) Source->RetainEffectDefinitions(ASC);
    for (const auto& Stat:Source->Stats) {
        if (Source->Captures.Contains(Stat.Key) || !ASC->HasAttributeSetForAttribute(Stat.Key)) continue;
        FGameplayEffectAttributeCaptureSpec Capture(FGameplayEffectAttributeCaptureDefinition(Stat.Key,EGameplayEffectAttributeCaptureSource::Source,true));
        ASC->CaptureAttributeForGameplayEffect(Capture); Source->Captures.Add(Stat.Key,Capture);
    }
    FMythicActionAffixSnapshot Equipment;
    const auto* Affixes=ASC->GetOwner() ? ASC->GetOwner()->FindComponentByClass<UMythicAffixApplicationComponent>() : nullptr;
    if (Affixes && !Affixes->CaptureActionAffixState(Equipment)) return false;
    for (const auto& Profile:ActionProfiles) {
        TMap<FGameplayAttribute,float> Bases;
        if (!UMythicAffixApplicationComponent::ResolveActionEquipmentBases(Equipment,Profile.Value.Facets,Classification,Source->StateTags,Bases)) { ActionSources.Reset(); return false; }
        auto PerAction=MakeShared<FMythicAbilityActionSource>(*Source); PerAction->EquipmentBases=Bases;
        FGameplayTagContainer EvaluationTags=Source->StateTags; EvaluationTags.AppendTags(Profile.Value.Facets); EvaluationTags.AddTag(GAS_ABILITY_TYPE_SKILL);
        for (auto& Stat:PerAction->Stats) {
            float Value=0;
            if (PerAction->EvaluateStat(Stat.Key,EvaluationTags,nullptr,Value)) Stat.Value=Value;
            else if (ASC->HasAttributeSetForAttribute(Stat.Key)) { ActionSources.Reset(); return false; }
        }
        ActionSources.Add(Profile.Key,PerAction);
    }
    ActionSource=Source;
    return true;
}

FGuid UMythicGA_AbilityWindow::BeginAbilityAction(FName ActionId) {
    if (!IsWindowAuthority() || !ActionSource || ActionGroups.Num()>=64 || !bHasReactionLineage || (bInsideReaction && CurrentGeneration>=4)) return {};
    const auto* Profile=ActionProfiles.Find(ActionId);
    const auto* PerActionSource=ActionSources.Find(ActionId);
    TArray<FText> Errors;
    // Multi-member actions are admitted when their shared secondary-proc receipt path is available.
    if (!Profile || !PerActionSource || !Profile->Validate(Errors) || Profile->Shares.Num()!=1) return {};
    auto* ASC=CurrentActorInfo->AbilitySystemComponent.Get();
    const FGuid Serial=ActivationId;
    const FGuid GroupId=FGuid::NewGuid();
    auto Group=MakeShared<FActionGroup>();
    Group->Activation=Serial; Group->Profile=*Profile;
    Group->ClaimedTargets.SetNum(Profile->Shares.Num());
    auto Snapshot=MakeShared<FMythicAbilityActionSnapshot>();
    Snapshot->Source=*PerActionSource; Snapshot->ActionId=ActionId; Snapshot->Facets=Profile->Facets;
    Group->Context=MakeEffectContext(CurrentSpecHandle,CurrentActorInfo);
    auto* Context=FMythicGameplayEffectContext::ExtractEffectContext(Group->Context);
    if (!Context) return {};
    Context->ActionSnapshot=Snapshot; Context->ActionId=ActionId; Context->ActionGroupId=GroupId;
    Context->ActionFacets=Snapshot->Facets; Context->DamageOrigin=EMythicDamageOrigin::Skill;
    Context->SourceActivationId=Serial; Context->ReactionGeneration=bInsideReaction ? CurrentGeneration+1 : 0;
    if (Profile->Work==EMythicAbilityActionWork::Utility) {
        ActionGroups.Add(GroupId,Group); return GroupId;
    }
    Group->bStatusIntent=MythicCombat::RollSucceeds(Profile->StatusChance,FMath::FRand());
    Snapshot->RolledDamage=FMath::FRandRange(Profile->MinimumDamage,Profile->MaximumDamage);
    Snapshot->bScaleWithPrimaryStats=Profile->bScaleWithPrimaryStats;
    for (float& Roll:Snapshot->IntentRolls) Roll=FMath::FRand();
    if (!FMath::IsFinite(Snapshot->RolledDamage) || Snapshot->RolledDamage<0) return {};
    FGameplayTagContainer WorkTags=Snapshot->Facets; WorkTags.AddTag(GAS_ABILITY_TYPE_SKILL);
    auto MakeWork=[&](TSubclassOf<UGameplayEffect> Effect) {
        auto Spec=ASC->MakeOutgoingSpec(Effect,GetAbilityLevel(),Group->Context);
        if (Spec.IsValid()) { Spec.Data->AppendDynamicAssetTags(WorkTags); Spec.Data->CapturedSourceTags.GetSpecTags().AppendTags(WorkTags); }
        return Spec;
    };
    Group->Application=MakeWork(Profile->Damage.DamageApplicationEffect);
    const auto Calculation=MakeWork(Profile->Damage.DamageCalculationEffect);
    if (!Group->Application.IsValid() || !Calculation.IsValid()) return {};
    // Roll crit and equipment status intent once for the group, then copy their result into each occurrence.
    ASC->ApplyGameplayEffectSpecToSelf(*Calculation.Data.Get());
    if (Serial!=ActivationId || !IsWindowAuthority()) return {};
    ActionGroups.Add(GroupId,Group);
    return GroupId;
}

void UMythicGA_AbilityWindow::EndAbilityAction(FGuid ActionHandle) {
    if (IsWindowAuthority()) ActionGroups.Remove(ActionHandle);
}

FMythicAbilityDamageResult UMythicGA_AbilityWindow::ApplyAbilityAction(AActor* Target,FGuid ActionHandle,int32 ShareIndex) {
    FMythicAbilityDamageResult Empty;
    const auto ValidTarget=[&](){ return IsValidAbilityTarget(Target,false,true) && !Target->GetActorLocation().ContainsNaN()
        && FVector::DistSquared(Origin,Target->GetActorLocation())<=FMath::Square(6000.0f); };
    if (!IsWindowAuthority() || !ValidTarget()) return Empty;
    const auto* Found=ActionGroups.Find(ActionHandle);
    const TSharedPtr<FActionGroup> Group=Found ? *Found : nullptr;
    if (!Group || Group->Activation!=ActivationId || Group->Profile.Work!=EMythicAbilityActionWork::Offensive || !Group->Profile.Shares.IsValidIndex(ShareIndex)) return Empty;
    auto& Claims=Group->ClaimedTargets[ShareIndex];
    if (Claims.Contains(Target) || (Group->Profile.bOneTargetPerShare && !Claims.IsEmpty())
        || (!Group->AllTargets.Contains(Target) && Group->AllTargets.Num()>=Group->Profile.MaximumTargets)) return Empty;
    auto* TargetASC=UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
    auto* SourceASC=CurrentActorInfo->AbilitySystemComponent.Get();
    if (!TargetASC || !SourceASC) return Empty;
    // Reserve before callbacks, so recursive delivery cannot spend the same target/share twice.
    Claims.Add(Target); Group->AllTargets.Add(Target);
    FGameplayEffectContextHandle Occurrence=Group->Context.Duplicate();
    auto* Context=FMythicGameplayEffectContext::ExtractEffectContext(Occurrence);
    Context->ActionOccurrenceId=FGuid::NewGuid(); Context->ActionShare=Group->Profile.Shares[ShareIndex];
    Context->ResolvedDamage={}; Context->ResolvedHealing={}; Context->OwnedShieldAbsorption.Reset();
    Context->SetShieldAbsorbed(0); Context->SetDodged(false);
    const FGuid Serial=ActivationId; const int32 PreviousGeneration=CurrentGeneration;
    CurrentGeneration=Context->ReactionGeneration;
    ON_SCOPE_EXIT { if (Serial==ActivationId) CurrentGeneration=PreviousGeneration; };
    FHitResult Hit(Target,nullptr,Target->GetActorLocation(),FVector::UpVector);
    Context->AddHitResult(Hit,true);
    FGameplayEventData Pre;
    Pre.EventTag=GAS_EVENT_DMG_PRE; Pre.Instigator=WindowAvatar.Get(); Pre.Target=Target; Pre.ContextHandle=Occurrence;
    Pre.InstigatorTags=Context->ActionFacets; Pre.InstigatorTags.AddTag(GAS_ABILITY_TYPE_SKILL);
    auto* TargetData=new FGameplayAbilityTargetData_SingleTargetHit(Hit); Pre.TargetData.Add(TargetData);
    SourceASC->HandleGameplayEvent(GAS_EVENT_DMG_PRE,&Pre);
    if (Group->Activation!=ActivationId || !IsWindowAuthority() || !ActionGroups.Contains(ActionHandle) || !ValidTarget()) return Empty;
    FGameplayEffectSpec Spec(*Group->Application.Data.Get()); Spec.SetContext(Occurrence);
    SourceASC->ApplyGameplayEffectSpecToTarget(Spec,TargetASC);
    const FMythicAbilityDamageResult Result=Context->ResolvedDamage;
    if (!Result.bAccepted || Result.bDodged || !Group->bStatusIntent || Group->Profile.StatusApplication==EMythicAbilityStatusApplication::None
        || Group->Activation!=ActivationId || !IsWindowAuthority() || !ActionGroups.Contains(ActionHandle) || !ValidTarget()) return Result;
    const auto* GI=GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    const auto* Registry=GI ? GI->GetSubsystem<UMythicStatusRegistry>() : nullptr;
    const auto* Status=Registry ? Registry->FindStatus(Group->Profile.StatusType) : nullptr;
    if (!Status) return Result;
    const float Resistance=Status->ResistanceAttribute.IsValid() && TargetASC->HasAttributeSetForAttribute(Status->ResistanceAttribute)
        ? TargetASC->GetNumericAttribute(Status->ResistanceAttribute) : 0;
    const float Survive=1-MythicCombat::ClampProbability(Resistance,GetDefault<UMythicCombatSettings>()->MaxStatusResistance);
    if (!MythicCombat::RollSucceeds(Survive,FMath::FRand())) return Result;
    if (Group->Profile.StatusApplication==EMythicAbilityStatusApplication::Direct) {
        UMythicStatusRegistry::ApplyStatusEffect(TargetASC,Status,WindowAvatar.Get(),WindowAvatar.Get(),Group->Profile.StatusDuration,Occurrence);
    } else {
        auto Buildup=SourceASC->MakeOutgoingSpec(Group->Profile.BuildupEffect,GetAbilityLevel(),Occurrence.Duplicate());
        if (Buildup.IsValid()) {
            float Multiplier=1; UMythicStatusRegistry::TryReadApplierStat(WindowAvatar.Get(),UMythicAttributeSet_Offense::GetStatusBuildupMultiplierAttribute(),Multiplier,Occurrence);
            const float Amount=UMythicDamageApplication::ComputeBuildupPerProc(Group->Profile.BuildupAmount,Multiplier)*Context->ActionShare;
            Buildup.Data->SetSetByCallerMagnitude(GAS_SETBYCALLER_GENERIC,Amount);
            Buildup.Data->AppendDynamicAssetTags(Context->ActionFacets);
            SourceASC->ApplyGameplayEffectSpecToTarget(*Buildup.Data.Get(),TargetASC);
        }
    }
    return Result;
}
