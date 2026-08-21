

#include "MythicGameplayAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemLog.h"
#include "Mythic.h"
#include "MythicAbilityCost.h"
#include "MythicDamageContainer.h"
#include "GAS/MythicAbilitySourceInterface.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "GAS/MythicTags_GAS.h"
#include "Physics/PhysicalMaterialWithTags.h"
#include "Player/MythicPlayerController.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "Itemization/MythicTags_Inventory.h"
#include "Itemization/Inventory/Fragments/Passive/DurabilityFragment.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GameModes/GameState/MythicGameState.h"
#include "GameplayEffect.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "MythicChargeState.h"
#include "TimerManager.h"
#include "GAS/Feedback/MythicTags_FeedbackCues.h"
#include "Player/MythicPlayerState.h"
#include "Player/Proficiency/ProficiencyComponent.h"
#include "Settings/MythicDeveloperSettings.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameStateBase.h"
#include "GenericTeamAgentInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MythicGameplayAbility)

#define ENSURE_ABILITY_IS_INSTANTIATED_OR_RETURN(FunctionName, ReturnValue)																				\
{																																						\
    if (!ensure(IsInstantiated()))																														\
    {																																					\
        ABILITY_LOG(Error, TEXT("%s: " #FunctionName " cannot be called on a non-instanced ability. Check the instancing policy."), *GetPathName());	\
        return ReturnValue;																																\
    }																																					\
}
class UMythicTargetType;

namespace {
    FString ReadApplierKeyForStatus(const UAbilitySystemComponent *TargetASC, const FGameplayTag &StatusTag) {
        if (!TargetASC || !StatusTag.IsValid()) {
            return FString();
        }
        const FGameplayEffectQuery Query =
            FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(StatusTag));
        for (const FActiveGameplayEffectHandle &Handle : TargetASC->GetActiveEffects(Query)) {
            if (const FActiveGameplayEffect *Active = TargetASC->GetActiveGameplayEffect(Handle)) {
                if (const FMythicGameplayEffectContext *Ctx =
                        FMythicGameplayEffectContext::ExtractEffectContext(Active->Spec.GetContext())) {
                    if (!Ctx->GetApplierPlayerKey().IsEmpty()) {
                        return Ctx->GetApplierPlayerKey();
                    }
                }
            }
        }
        return FString();
    }
}

UMythicGameplayAbility::UMythicGameplayAbility(const FObjectInitializer &ObjectInitializer) {
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;

    ActivationPolicy = EMythicAbilityActivationPolicy::OnInputTriggered;
    ActivationGroup = EMythicAbilityActivationGroup::Independent;
}

void UMythicGameplayAbility::TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilitySpec &Spec) const {
    if (ActorInfo && !Spec.IsActive() && (ActivationPolicy == EMythicAbilityActivationPolicy::OnSpawn)) {
        UAbilitySystemComponent *ASC = ActorInfo->AbilitySystemComponent.Get();
        const AActor *AvatarActor = ActorInfo->AvatarActor.Get();

        if (ASC && AvatarActor && !AvatarActor->GetTearOff() && (AvatarActor->GetLifeSpan() <= 0.0f)) {
            const bool bIsLocalExecution = (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::LocalPredicted) || (NetExecutionPolicy ==
                EGameplayAbilityNetExecutionPolicy::LocalOnly);
            const bool bIsServerExecution = (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerOnly) || (NetExecutionPolicy ==
                EGameplayAbilityNetExecutionPolicy::ServerInitiated);

            const bool bClientShouldActivate = ActorInfo->IsLocallyControlled() && bIsLocalExecution;
            const bool bServerShouldActivate = ActorInfo->IsNetAuthority() && bIsServerExecution;

            if (bClientShouldActivate || bServerShouldActivate) {
                ASC->TryActivateAbility(Spec.Handle);
            }
        }
    }
}

void UMythicGameplayAbility::OnAvatarSet(const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilitySpec &Spec) {
    Super::OnAvatarSet(ActorInfo, Spec);

    TryActivateAbilityOnSpawn(ActorInfo, Spec);

    K2_OnAvatarSet();
}

FMythicDamageContainerSpec UMythicGameplayAbility::MakeDamageContainerSpec(const FMythicDamageContainer &Container, int32 OverrideGameplayLevel) {
    FMythicDamageContainerSpec ReturnSpec;

    if (OverrideGameplayLevel == INDEX_NONE) {
        OverrideGameplayLevel = OverrideGameplayLevel = this->GetAbilityLevel();
    }
    auto OwningASC = GetAbilitySystemComponentFromActorInfo();
    if (!OwningASC) {
        UE_LOG(Myth, Error, TEXT("UMythicGameplayAbility::MakeDamageContainerSpec: no owning ASC (stale/null actor info)"));
        return ReturnSpec;
    }
    ReturnSpec.EffectContextHandle = OwningASC->MakeEffectContext();

    if (Container.DamageCalculationEffect) {
        FGameplayEffectSpecHandle DamageCalculationSpecHandle = OwningASC->MakeOutgoingSpec(Container.DamageCalculationEffect, OverrideGameplayLevel,
                                                                                            ReturnSpec.EffectContextHandle);
        ReturnSpec.DamageCalculationEffectSpec = DamageCalculationSpecHandle;
    }
    else {
        UE_LOG(Myth, Error, TEXT("UMythicGameplayAbility::MakeDamageContainerSpec: Container.DamageCalculationEffect is null"));
    }

    if (Container.DamageApplicationEffect) {
        FGameplayEffectSpecHandle DamageApplicationSpecHandle = OwningASC->MakeOutgoingSpec(Container.DamageApplicationEffect, OverrideGameplayLevel,
                                                                                            ReturnSpec.EffectContextHandle);
        ReturnSpec.DamageApplicationEffectSpec = DamageApplicationSpecHandle;
    }
    else {
        UE_LOG(Myth, Error, TEXT("UMythicGameplayAbility::MakeDamageContainerSpec: Container.DamageApplicationEffect is null"));
    }

    ReturnSpec.StatusEffects = Container.StatusEffects;

    if (UObject *Src = GetCurrentSourceObject()) {
        if (UItemFragment *SrcFragment = Cast<UItemFragment>(Src)) {
            if (UMythicItemInstance *WeaponInst = SrcFragment->GetOwningItemInstance()) {
                FGameplayTagContainer TypeProbe;
                WeaponInst->GetTypeProbe(TypeProbe);
                const FGameplayTagContainer WeaponClassTags = TypeProbe.Filter(FGameplayTagContainer(ITEMIZATION_TYPE_EQUIPMENT_WEAPON));
                if (!WeaponClassTags.IsEmpty()) {
                    if (ReturnSpec.DamageApplicationEffectSpec.IsValid() && ReturnSpec.DamageApplicationEffectSpec.Data.IsValid()) {
                        ReturnSpec.DamageApplicationEffectSpec.Data->AppendDynamicAssetTags(WeaponClassTags);
                    }
                    if (ReturnSpec.DamageCalculationEffectSpec.IsValid() && ReturnSpec.DamageCalculationEffectSpec.Data.IsValid()) {
                        ReturnSpec.DamageCalculationEffectSpec.Data->AppendDynamicAssetTags(WeaponClassTags);
                    }
                }
            }
        }
    }

    return ReturnSpec;
}

void UMythicGameplayAbility::SendEvent(FGameplayAbilityTargetDataHandle TargetData, FGameplayEffectContextHandle EffectContextHandle, FGameplayTag EventTag) {
    if (!EventTag.IsValid()) {
        UE_LOG(Myth, Warning, TEXT("UMythicGameplayAbility::SendEvent: EventTag is not valid"));
        return;
    }

    auto SourceASC = GetAbilitySystemComponentFromActorInfo();
    FGameplayEventData EventData;
    EventData.Instigator = SourceASC->GetAvatarActor();
    EventData.TargetData = TargetData;
    EventData.ContextHandle = EffectContextHandle;
    EventData.InstigatorTags.AppendTags(BuildAbilityContextTags());

    for (int32 i = 0; i < TargetData.Num(); ++i) {
        const TArray<TWeakObjectPtr<AActor>> Actors = TargetData.Get(i) ? TargetData.Get(i)->GetActors() : TArray<TWeakObjectPtr<AActor>>();
        for (const TWeakObjectPtr<AActor> &Weak : Actors) {
            if (AActor *Hit = Weak.Get()) {
                EventData.Target = Hit;
                break;
            }
        }
        if (EventData.Target) {
            break;
        }
    }
    auto activations = SourceASC->HandleGameplayEvent(EventTag, &EventData);
    UE_LOG(Myth, Warning, TEXT("%s event sent to source %d abilities"), *EventTag.ToString(), activations);
}

TArray<FActiveGameplayEffectHandle> UMythicGameplayAbility::ApplyDamageContainerSpec(const FMythicDamageContainerSpec &ContainerSpec) {
    TArray<FActiveGameplayEffectHandle> AllEffects;

    if (!CurrentActorInfo->IsNetAuthority()) {
        return AllEffects;
    }

    if (!ContainerSpec.DamageCalculationEffectSpec.IsValid()) {
        UE_LOG(Myth, Error, TEXT("UMythicGameplayAbility::ApplyDamageContainerSpec: ContainerSpec.DamageCalculationEffectSpec is null"));
    }
    auto CalculationEffectHandle = K2_ApplyGameplayEffectSpecToOwner(ContainerSpec.DamageCalculationEffectSpec);
    AllEffects.Push(CalculationEffectHandle);

    if (ContainerSpec.TargetsHandle.Num() > 0) {
        SendEvent(ContainerSpec.TargetsHandle, ContainerSpec.EffectContextHandle, GAS_EVENT_DMG_PRE);

        if (!ContainerSpec.DamageApplicationEffectSpec.IsValid()) {
            UE_LOG(Myth, Error, TEXT("UMythicGameplayAbility::ApplyDamageContainerSpec: ContainerSpec.DamageApplicationEffectSpec is null"));
        }
        else {
            const float DebuffLevel = ContainerSpec.DamageApplicationEffectSpec.Data->GetLevel();
            for (int32 i = 0; i < ContainerSpec.TargetsHandle.Data.Num(); ++i) {
                if (!ContainerSpec.TargetsHandle.Data[i].IsValid()) {
                    continue;
                }

                FGameplayEffectContextHandle PerTargetContext = ContainerSpec.EffectContextHandle.Duplicate();

                FGameplayEffectSpecHandle PerTargetSpec(new FGameplayEffectSpec(*ContainerSpec.DamageApplicationEffectSpec.Data));
                PerTargetSpec.Data->SetContext(PerTargetContext);

                FGameplayAbilityTargetDataHandle SingleTarget;
                SingleTarget.Data.Add(ContainerSpec.TargetsHandle.Data[i]);

                AllEffects.Append(K2_ApplyGameplayEffectSpecToTarget(PerTargetSpec, SingleTarget));
            }
        }
    }

    if (ContainerSpec.DestructibleTargetsHandle.Num() > 0) {
        SendEvent(ContainerSpec.DestructibleTargetsHandle, ContainerSpec.EffectContextHandle, GAS_EVENT_DMG_DESTRUCTIBLE);
    }

    return AllEffects;
}

TArray<FActiveGameplayEffectHandle> UMythicGameplayAbility::ApplyDamageContainer(const FMythicDamageContainer &Container, const TArray<FHitResult> &HitResults,
                                                                                 const TArray<AActor *> &TargetActors,
                                                                                 int32 OverrideGameplayLevel) {
    TArray<FActiveGameplayEffectHandle> AllEffects;

    if (!CurrentActorInfo->IsNetAuthority()) {
        return AllEffects;
    }

    if (TargetActors.IsEmpty() || HitResults.IsEmpty()) {
        UE_LOG(Myth, Warning, TEXT("No Targets"));
    }

    if (!TargetActors.IsEmpty() && !HitResults.IsEmpty()) {
        if (UObject *Src = GetCurrentSourceObject()) {
            if (UItemFragment *SourceFragment = Cast<UItemFragment>(Src)) {
                if (UMythicItemInstance *Inst = SourceFragment->GetOwningItemInstance()) {
                    if (const UDurabilityFragment *Durability = Inst->GetFragment<UDurabilityFragment>()) {
                        if (Durability->IsBroken()) {
                            return AllEffects;
                        }
                        const_cast<UDurabilityFragment *>(Durability)->ServerApplyWear(1);
                    }
                }
            }
        }
    }

    FMythicDamageContainerSpec Spec = MakeDamageContainerSpec(Container, OverrideGameplayLevel);
    AddTargetsToDamageContainerSpec(Spec, HitResults, TargetActors);
    return ApplyDamageContainerSpec(Spec);
}

void UMythicGameplayAbility::AddTargetsToDamageContainerSpec(FMythicDamageContainerSpec &ContainerSpec, const TArray<FHitResult> &HitResults,
                                                             const TArray<AActor *> &TargetActors) {
    ContainerSpec.AddTargets(HitResults, TargetActors);
}

bool UMythicGameplayAbility::CanChangeActivationGroup(EMythicAbilityActivationGroup NewGroup) const {
    if (!IsInstantiated() || !IsActive()) {
        return false;
    }

    if (ActivationGroup == NewGroup) {
        return true;
    }

    auto ASC = GetMythicAbilitySystemComponentFromActorInfo();
    check(ASC);

    if ((ActivationGroup != EMythicAbilityActivationGroup::Exclusive_Blocking) && ASC->IsActivationGroupBlocked(NewGroup)) {
        return false;
    }

    if ((NewGroup == EMythicAbilityActivationGroup::Exclusive_Replaceable) && !CanBeCanceled()) {
        return false;
    }

    return true;
}

bool UMythicGameplayAbility::ChangeActivationGroup(EMythicAbilityActivationGroup NewGroup) {
    ENSURE_ABILITY_IS_INSTANTIATED_OR_RETURN(ChangeActivationGroup, false);

    if (!CanChangeActivationGroup(NewGroup)) {
        return false;
    }

    if (ActivationGroup != NewGroup) {
        auto MythicASC = GetMythicAbilitySystemComponentFromActorInfo();
        check(MythicASC);

        MythicASC->RemoveAbilityFromActivationGroup(ActivationGroup, this);
        MythicASC->AddAbilityToActivationGroup(NewGroup, this);

        ActivationGroup = NewGroup;
    }

    return true;
}

bool UMythicGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                                                const FGameplayTagContainer *SourceTags, const FGameplayTagContainer *TargetTags,
                                                FGameplayTagContainer *OptionalRelevantTags) const {
    if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid()) {
        return false;
    }

    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)) {
        return false;
    }

    auto MythicASC = CastChecked<UMythicAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());
    if (MythicASC->IsActivationGroupBlocked(ActivationGroup)) {
        return false;
    }

    if (AreChargesEnabled() && CurrentCharges <= 0) {
        if (OptionalRelevantTags) {
            OptionalRelevantTags->AddTag(UAbilitySystemGlobals::Get().ActivateFailCooldownTag);
        }
        return false;
    }

    return true;
}

void UMythicGameplayAbility::SetCanBeCanceled(bool bCanBeCanceled) {
    if (!bCanBeCanceled && (ActivationGroup == EMythicAbilityActivationGroup::Exclusive_Replaceable)) {
        UE_LOG(Myth, Error, TEXT("SetCanBeCanceled: Ability [%s] can not block canceling because its activation group is replaceable."), *GetName());
        return;
    }

    Super::SetCanBeCanceled(bCanBeCanceled);
}

void UMythicGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilitySpec &Spec) {
    Super::OnGiveAbility(ActorInfo, Spec);

    InitializeCharges();

    K2_OnAbilityAdded();

    TryActivateAbilityOnSpawn(ActorInfo, Spec);
}

void UMythicGameplayAbility::OnRemoveAbility(const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilitySpec &Spec) {
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(ChargeRechargeTimer);
    }
    if (AreChargesEnabled()) {
        SetCategoryBlock(false);
    }

    K2_OnAbilityRemoved();

    Super::OnRemoveAbility(ActorInfo, Spec);
}


void UMythicGameplayAbility::InitializeCharges() {
    CurrentCharges = FMath::Max(1, MaxCharges);
}

void UMythicGameplayAbility::ConsumeChargeOnActivation() {
    if (!AreChargesEnabled()) {
        return;
    }
    if (CurrentCharges > 0) {
        --CurrentCharges;
    }
    if (CurrentCharges <= 0) {
        SetCategoryBlock(true);
    }
    EnsureRechargeTimer();
}

void UMythicGameplayAbility::EnsureRechargeTimer() {
    if (!AreChargesEnabled() || CurrentCharges >= MaxCharges || RechargeSeconds <= 0.0f) {
        return;
    }
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    if (World->GetTimerManager().IsTimerActive(ChargeRechargeTimer)) {
        return;
    }
    World->GetTimerManager().SetTimer(ChargeRechargeTimer, this, &UMythicGameplayAbility::HandleChargeRecharge, RechargeSeconds, false);
}

void UMythicGameplayAbility::HandleChargeRecharge() {
    const bool bWasEmpty = (CurrentCharges <= 0);
    CurrentCharges = FMythicChargeState::ComputeChargesAfterElapsed(CurrentCharges, MaxCharges, RechargeSeconds, RechargeSeconds);
    ChargeRechargeTimer.Invalidate();

    if (bWasEmpty && CurrentCharges > 0) {
        SetCategoryBlock(false);
    }
    if (CurrentCharges < MaxCharges) {
        EnsureRechargeTimer();
    }
}

void UMythicGameplayAbility::SetCategoryBlock(bool bBlocked) {
    if (!CooldownCategoryTag.IsValid()) {
        return;
    }
    UAbilitySystemComponent *ASC = GetAbilitySystemComponentFromActorInfo();
    if (!ASC || !CurrentActorInfo || !CurrentActorInfo->IsNetAuthority()) {
        return;
    }
    if (bBlocked) {
        ASC->AddLooseGameplayTag(CooldownCategoryTag);
    }
    else {
        ASC->RemoveLooseGameplayTag(CooldownCategoryTag);
    }
}

bool UMythicGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                                       FGameplayTagContainer *OptionalRelevantTags) const {
    if (!Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags) || !ActorInfo) {
        return false;
    }

    for (const TObjectPtr<UMythicAbilityCost> &AdditionalCost : AdditionalCosts) {
        if (AdditionalCost != nullptr) {
            if (!AdditionalCost->CheckCost(this, Handle, ActorInfo, OptionalRelevantTags)) {
                return false;
            }
        }
    }

    return true;
}

void UMythicGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                                       const FGameplayAbilityActivationInfo ActivationInfo) const {
    Super::ApplyCost(Handle, ActorInfo, ActivationInfo);

    check(ActorInfo);

    auto DetermineIfAbilityHitTarget = [&]() {
        if (ActorInfo->IsNetAuthority()) {
            if (auto ASC = Cast<UMythicAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get())) {
                FGameplayAbilityTargetDataHandle TargetData;
                ASC->GetAbilityTargetData(Handle, ActivationInfo, TargetData);
                for (int32 TargetDataIdx = 0; TargetDataIdx < TargetData.Data.Num(); ++TargetDataIdx) {
                    if (UAbilitySystemBlueprintLibrary::TargetDataHasHitResult(TargetData, TargetDataIdx)) {
                        return true;
                    }
                }
            }
        }

        return false;
    };

    if (ActorInfo->IsNetAuthority() && AreChargesEnabled()) {
        const_cast<UMythicGameplayAbility *>(this)->ConsumeChargeOnActivation();
    }

    bool bAbilityHitTarget = false;
    bool bHasDeterminedIfAbilityHitTarget = false;
    for (const TObjectPtr<UMythicAbilityCost> &AdditionalCost : AdditionalCosts) {
        if (AdditionalCost != nullptr) {
            if (AdditionalCost->ShouldOnlyApplyCostOnHit()) {
                if (!bHasDeterminedIfAbilityHitTarget) {
                    bAbilityHitTarget = DetermineIfAbilityHitTarget();
                    bHasDeterminedIfAbilityHitTarget = true;
                }

                if (!bAbilityHitTarget) {
                    continue;
                }
            }

            AdditionalCost->ApplyCost(this, Handle, ActorInfo, ActivationInfo);
        }
    }
}

namespace {
    float ComputeCooldownMultiplier(const FGameplayAbilityActorInfo *ActorInfo) {
        if (!ActorInfo) {
            return 1.0f;
        }
        const UAbilitySystemComponent *ASC = ActorInfo->AbilitySystemComponent.Get();
        if (!ASC) {
            return 1.0f;
        }
        const UMythicAttributeSet_Utility *Util = ASC->GetSet<UMythicAttributeSet_Utility>();
        if (!Util) {
            return 1.0f;
        }
        float MaxCDR = 0.8f;
        if (const UWorld *World = ASC->GetWorld()) {
            if (const AMythicGameState *GS = World->GetGameState<AMythicGameState>()) {
                MaxCDR = FMath::Clamp(GS->MaxCooldownReduction, 0.0f, 1.0f);
            }
        }
        const float CDR = FMath::Clamp(Util->GetCooldownReduction(), 0.0f, MaxCDR);
        return 1.0f - CDR;
    }
}

void UMythicGameplayAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                                           const FGameplayAbilityActivationInfo ActivationInfo) const {
    const float Mult = ComputeCooldownMultiplier(ActorInfo);

    if (Mult >= 1.0f) {
        Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);
        return;
    }

    UGameplayEffect *CooldownGE = GetCooldownGameplayEffect();
    if (!CooldownGE) {
        return;
    }
    if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo)) {
        return;
    }
    FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, CooldownGE->GetClass(), GetAbilityLevel(Handle, ActorInfo));
    if (!SpecHandle.IsValid()) {
        return;
    }
    const float BaseDuration = SpecHandle.Data->GetDuration();
    if (BaseDuration > 0.0f) {
        SpecHandle.Data->SetDuration(BaseDuration * Mult, true);
    }
    ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
}

FGameplayEffectContextHandle UMythicGameplayAbility::MakeEffectContext(const FGameplayAbilitySpecHandle Handle,
                                                                       const FGameplayAbilityActorInfo *ActorInfo) const {
    FGameplayEffectContextHandle ContextHandle = Super::MakeEffectContext(Handle, ActorInfo);

    FMythicGameplayEffectContext *EffectContext = FMythicGameplayEffectContext::ExtractEffectContext(ContextHandle);
    check(EffectContext);

    check(ActorInfo);

    AActor *EffectCauser = nullptr;
    const IMythicAbilitySourceInterface *AbilitySource = nullptr;
    float SourceLevel = 0.0f;
    GetAbilitySource(Handle, ActorInfo, SourceLevel, AbilitySource, EffectCauser);

    UObject *SourceObject = GetSourceObject(Handle, ActorInfo);

    AActor *Instigator = ActorInfo ? ActorInfo->OwnerActor.Get() : nullptr;

    EffectContext->SetAbilitySource(AbilitySource, SourceLevel);
    EffectContext->AddInstigator(Instigator, EffectCauser);
    EffectContext->AddSourceObject(SourceObject);

    return ContextHandle;
}

void UMythicGameplayAbility::GetAbilitySource(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo, float &OutSourceLevel,
                                              const IMythicAbilitySourceInterface *&OutAbilitySource, AActor *&OutEffectCauser) const {
    OutSourceLevel = 0.0f;
    OutAbilitySource = nullptr;
    OutEffectCauser = nullptr;

    OutEffectCauser = ActorInfo->AvatarActor.Get();

    UObject *SourceObject = GetSourceObject(Handle, ActorInfo);

    OutAbilitySource = Cast<IMythicAbilitySourceInterface>(SourceObject);
}

void UMythicGameplayAbility::ApplyAbilityTagsToGameplayEffectSpec(FGameplayEffectSpec &Spec, FGameplayAbilitySpec *AbilitySpec) const {
    Super::ApplyAbilityTagsToGameplayEffectSpec(Spec, AbilitySpec);

    if (const FHitResult *HitResult = Spec.GetContext().GetHitResult()) {
        if (const auto PhysMatWithTags = Cast<const UPhysicalMaterialWithTags>(HitResult->PhysMaterial.Get())) {
            Spec.CapturedTargetTags.GetSpecTags().AppendTags(PhysMatWithTags->Tags);
        }
    }
}

bool UMythicGameplayAbility::DoesAbilitySatisfyTagRequirements(const UAbilitySystemComponent &AbilitySystemComponent, const FGameplayTagContainer *SourceTags,
                                                               const FGameplayTagContainer *TargetTags, FGameplayTagContainer *OptionalRelevantTags) const {
    bool bBlocked = false;
    bool bMissing = false;

    UAbilitySystemGlobals &AbilitySystemGlobals = UAbilitySystemGlobals::Get();
    const FGameplayTag &BlockedTag = AbilitySystemGlobals.ActivateFailTagsBlockedTag;
    const FGameplayTag &MissingTag = AbilitySystemGlobals.ActivateFailTagsMissingTag;

    if (AbilitySystemComponent.AreAbilityTagsBlocked(GetAssetTags())) {
        bBlocked = true;
    }

    const auto MythicASC = Cast<UMythicAbilitySystemComponent>(&AbilitySystemComponent);
    static FGameplayTagContainer AllRequiredTags;
    static FGameplayTagContainer AllBlockedTags;

    AllRequiredTags = ActivationRequiredTags;
    AllBlockedTags = ActivationBlockedTags;

    if (MythicASC) {
        MythicASC->GetAdditionalActivationTagRequirements(GetAssetTags(), AllRequiredTags, AllBlockedTags);
    }

    if (AllBlockedTags.Num() || AllRequiredTags.Num()) {
        static FGameplayTagContainer AbilitySystemComponentTags;

        AbilitySystemComponentTags.Reset();
        AbilitySystemComponent.GetOwnedGameplayTags(AbilitySystemComponentTags);

        if (AbilitySystemComponentTags.HasAny(AllBlockedTags)) {
            if (OptionalRelevantTags && AbilitySystemComponentTags.HasTag(GAS_STATE_DEAD)) {
                OptionalRelevantTags->AddTag(NOTIFY_ABILITY_ACTIVATION_FAILED_ISDEAD);
            }

            bBlocked = true;
        }

        if (!AbilitySystemComponentTags.HasAll(AllRequiredTags)) {
            bMissing = true;
        }
    }

    if (SourceTags != nullptr) {
        if (SourceBlockedTags.Num() || SourceRequiredTags.Num()) {
            if (SourceTags->HasAny(SourceBlockedTags)) {
                bBlocked = true;
            }

            if (!SourceTags->HasAll(SourceRequiredTags)) {
                bMissing = true;
            }
        }
    }

    if (TargetTags != nullptr) {
        if (TargetBlockedTags.Num() || TargetRequiredTags.Num()) {
            if (TargetTags->HasAny(TargetBlockedTags)) {
                bBlocked = true;
            }

            if (!TargetTags->HasAll(TargetRequiredTags)) {
                bMissing = true;
            }
        }
    }

    if (bBlocked) {
        if (OptionalRelevantTags && BlockedTag.IsValid()) {
            OptionalRelevantTags->AddTag(BlockedTag);
        }
        UE_LOG(Myth, Verbose, TEXT("Ability %s blocked by tags."), *GetName());
        return false;
    }
    if (bMissing) {
        if (OptionalRelevantTags && MissingTag.IsValid()) {
            OptionalRelevantTags->AddTag(MissingTag);
        }
        UE_LOG(Myth, Verbose, TEXT("Ability %s missing required tags."), *GetName());
        return false;
    }

    return true;
}


UMythicAbilitySystemComponent *UMythicGameplayAbility::GetMythicAbilitySystemComponentFromActorInfo() const {
    return CurrentActorInfo ? Cast<UMythicAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get()) : nullptr;
}

AController *UMythicGameplayAbility::GetControllerFromActorInfo() const {
    return CurrentActorInfo ? Cast<AController>(CurrentActorInfo->PlayerController.Get()) : nullptr;
}

AMythicPlayerController *UMythicGameplayAbility::GetMythicPlayerControllerFromActorInfo() const {
    return CurrentActorInfo ? Cast<AMythicPlayerController>(CurrentActorInfo->PlayerController.Get()) : nullptr;
}

float UMythicGameplayAbility::GetClampedAttackSpeedPlayRate() const {
    float AttackSpeedScale = 1.0f;
    if (UAbilitySystemComponent *ASC = GetAbilitySystemComponentFromActorInfo()) {
        if (const UMythicAttributeSet_Offense *Offense = ASC->GetSet<UMythicAttributeSet_Offense>()) {
            AttackSpeedScale = Offense->GetAttackSpeed();
        }
    }

    return FMath::Clamp(AttackSpeedScale, 0.8f, 1.4f);
}

FGameplayTagContainer UMythicGameplayAbility::BuildAbilityContextTags() const {
    FGameplayTagContainer Out = GetAssetTags();

    if (UObject *Src = GetCurrentSourceObject()) {
        if (UItemFragment *SrcFragment = Cast<UItemFragment>(Src)) {
            if (UMythicItemInstance *WeaponInst = SrcFragment->GetOwningItemInstance()) {
                FGameplayTagContainer TypeProbe;
                WeaponInst->GetTypeProbe(TypeProbe);
                Out.AppendTags(TypeProbe.Filter(FGameplayTagContainer(ITEMIZATION_TYPE_EQUIPMENT_WEAPON)));
            }
        }
    }

    return Out;
}