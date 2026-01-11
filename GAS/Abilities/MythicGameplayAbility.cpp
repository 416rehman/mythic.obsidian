// 


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

UMythicGameplayAbility::UMythicGameplayAbility(const FObjectInitializer &ObjectInitializer) {
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;

    ActivationPolicy = EMythicAbilityActivationPolicy::OnInputTriggered;
    ActivationGroup = EMythicAbilityActivationGroup::Independent;
}

void UMythicGameplayAbility::TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilitySpec &Spec) const {
    // Try to activate if activation policy is on spawn.
    if (ActorInfo && !Spec.IsActive() && (ActivationPolicy == EMythicAbilityActivationPolicy::OnSpawn)) {
        UAbilitySystemComponent *ASC = ActorInfo->AbilitySystemComponent.Get();
        const AActor *AvatarActor = ActorInfo->AvatarActor.Get();

        // If avatar actor is torn off or about to die, don't try to activate until we get the new one.
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

    // Try to activate if activation policy is on spawn.
    TryActivateAbilityOnSpawn(ActorInfo, Spec);

    K2_OnAvatarSet();
}

FMythicDamageContainerSpec UMythicGameplayAbility::MakeDamageContainerSpec(const FMythicDamageContainer &Container, int32 OverrideGameplayLevel) {
    // First figure out our actor info
    FMythicDamageContainerSpec ReturnSpec;

    // If we don't have an override level, use the default on the ability itself
    if (OverrideGameplayLevel == INDEX_NONE) {
        OverrideGameplayLevel = OverrideGameplayLevel = this->GetAbilityLevel(); //OwningASC->GetDefaultAbilityLevel();
    }
    auto OwningASC = GetAbilitySystemComponentFromActorInfo();
    ReturnSpec.EffectContextHandle = OwningASC->MakeEffectContext();

    // Add the damage calculation effect
    if (Container.DamageCalculationEffect) {
        FGameplayEffectSpecHandle DamageCalculationSpecHandle = OwningASC->MakeOutgoingSpec(Container.DamageCalculationEffect, OverrideGameplayLevel,
                                                                                            ReturnSpec.EffectContextHandle);
        ReturnSpec.DamageCalculationEffectSpec = DamageCalculationSpecHandle;
    }
    else {
        UE_LOG(Myth, Error, TEXT("UMythicGameplayAbility::MakeDamageContainerSpec: Container.DamageCalculationEffect is null"));
    }

    // Add the damage application effect
    if (Container.DamageApplicationEffect) {
        FGameplayEffectSpecHandle DamageApplicationSpecHandle = OwningASC->MakeOutgoingSpec(Container.DamageApplicationEffect, OverrideGameplayLevel,
                                                                                            ReturnSpec.EffectContextHandle);
        ReturnSpec.DamageApplicationEffectSpec = DamageApplicationSpecHandle;
    }
    else {
        UE_LOG(Myth, Error, TEXT("UMythicGameplayAbility::MakeDamageContainerSpec: Container.DamageApplicationEffect is null"));
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
    auto activations = SourceASC->HandleGameplayEvent(EventTag, &EventData);
    UE_LOG(Myth, Warning, TEXT("%s event sent to source %d abilities"), *EventTag.ToString(), activations);
}

TArray<FActiveGameplayEffectHandle> UMythicGameplayAbility::ApplyDamageContainerSpec(const FMythicDamageContainerSpec &ContainerSpec) {
    TArray<FActiveGameplayEffectHandle> AllEffects;

    // 1. CREATE DAMAGE CONTEXT - This stage only sets the context like isCritical, isBlocked, etc. No damage is calculated yet.
    if (!ContainerSpec.DamageCalculationEffectSpec.IsValid()) {
        UE_LOG(Myth, Error, TEXT("UMythicGameplayAbility::ApplyDamageContainerSpec: ContainerSpec.DamageCalculationEffectSpec is null"));
    }
    auto CalculationEffectHandle = K2_ApplyGameplayEffectSpecToOwner(ContainerSpec.DamageCalculationEffectSpec);
    AllEffects.Push(CalculationEffectHandle);

    // Scoped prediction window for the damage application
    UAbilitySystemComponent *const AbilitySystemComponent = CurrentActorInfo->AbilitySystemComponent.Get();
    FScopedPredictionWindow NewScopedWindow(AbilitySystemComponent, true);

    // 2. SEND DAMAGE PRE EVENT
    if (ContainerSpec.TargetsHandle.Num() > 0) {
        SendEvent(ContainerSpec.TargetsHandle, ContainerSpec.EffectContextHandle, GAS_EVENT_DMG_PRE);

        // 3. APPLY DAMAGE - This stage calculates the damage and applies it to the targets
        if (!ContainerSpec.DamageApplicationEffectSpec.IsValid()) {
            UE_LOG(Myth, Error, TEXT("UMythicGameplayAbility::ApplyDamageContainerSpec: ContainerSpec.DamageApplicationEffectSpec is null"));
        }
        auto ApplicationEffectHandle = K2_ApplyGameplayEffectSpecToTarget(ContainerSpec.DamageApplicationEffectSpec, ContainerSpec.TargetsHandle);
        AllEffects.Append(ApplicationEffectHandle);
    }

    // 4. Handle destructibles
    if (ContainerSpec.DestructibleTargetsHandle.Num() > 0) {
        // 4.1. SEND DAMAGE PRE EVENT - No damage is calculated as part of this
        SendEvent(ContainerSpec.DestructibleTargetsHandle, ContainerSpec.EffectContextHandle, GAS_EVENT_DMG_DESTRUCTIBLE);
    }

    return AllEffects;
}

TArray<FActiveGameplayEffectHandle> UMythicGameplayAbility::ApplyDamageContainer(const FMythicDamageContainer &Container, const TArray<FHitResult> &HitResults,
                                                                                 const TArray<AActor *> &TargetActors,
                                                                                 int32 OverrideGameplayLevel) {
    // If no HitResults or TargetActors, gtfo
    if (TargetActors.IsEmpty() || HitResults.IsEmpty()) {
        UE_LOG(Myth, Warning, TEXT("No Targets"));
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
        // This ability can't change groups if it's blocked (unless it is the one doing the blocking).
        return false;
    }

    if ((NewGroup == EMythicAbilityActivationGroup::Exclusive_Replaceable) && !CanBeCanceled()) {
        // This ability can't become replaceable if it can't be canceled.
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

    return true;
}

void UMythicGameplayAbility::SetCanBeCanceled(bool bCanBeCanceled) {
    // The ability can not block canceling if it's replaceable.
    if (!bCanBeCanceled && (ActivationGroup == EMythicAbilityActivationGroup::Exclusive_Replaceable)) {
        UE_LOG(Myth, Error, TEXT("SetCanBeCanceled: Ability [%s] can not block canceling because its activation group is replaceable."), *GetName());
        return;
    }

    Super::SetCanBeCanceled(bCanBeCanceled);
}

void UMythicGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilitySpec &Spec) {
    Super::OnGiveAbility(ActorInfo, Spec);

    K2_OnAbilityAdded();

    TryActivateAbilityOnSpawn(ActorInfo, Spec);
}

void UMythicGameplayAbility::OnRemoveAbility(const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilitySpec &Spec) {
    K2_OnAbilityRemoved();

    Super::OnRemoveAbility(ActorInfo, Spec);
}

bool UMythicGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                                       FGameplayTagContainer *OptionalRelevantTags) const {
    if (!Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags) || !ActorInfo) {
        return false;
    }

    // Verify we can afford any additional costs
    for (const TObjectPtr<UMythicAbilityCost> &AdditionalCost : AdditionalCosts) {
        if (AdditionalCost != nullptr) {
            if (!AdditionalCost->CheckCost(this, Handle, ActorInfo, /*inout*/ OptionalRelevantTags)) {
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

    // Used to determine if the ability actually hit a target (as some costs are only spent on successful attempts)
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

    // Pay any additional costs
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

FGameplayEffectContextHandle UMythicGameplayAbility::MakeEffectContext(const FGameplayAbilitySpecHandle Handle,
                                                                       const FGameplayAbilityActorInfo *ActorInfo) const {
    FGameplayEffectContextHandle ContextHandle = Super::MakeEffectContext(Handle, ActorInfo);

    FMythicGameplayEffectContext *EffectContext = FMythicGameplayEffectContext::ExtractEffectContext(ContextHandle);
    check(EffectContext);

    check(ActorInfo);

    AActor *EffectCauser = nullptr;
    const IMythicAbilitySourceInterface *AbilitySource = nullptr;
    float SourceLevel = 0.0f;
    GetAbilitySource(Handle, ActorInfo, /*out*/ SourceLevel, /*out*/ AbilitySource, /*out*/ EffectCauser);

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

    // If we were added by something that's an ability info source, use it
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
    // Specialized version to handle death exclusion and AbilityTags expansion via ASC

    bool bBlocked = false;
    bool bMissing = false;

    UAbilitySystemGlobals &AbilitySystemGlobals = UAbilitySystemGlobals::Get();
    const FGameplayTag &BlockedTag = AbilitySystemGlobals.ActivateFailTagsBlockedTag;
    const FGameplayTag &MissingTag = AbilitySystemGlobals.ActivateFailTagsMissingTag;

    // Check if any of this ability's tags are currently blocked
    if (AbilitySystemComponent.AreAbilityTagsBlocked(GetAssetTags())) {
        bBlocked = true;
    }

    const auto MythicASC = Cast<UMythicAbilitySystemComponent>(&AbilitySystemComponent);
    static FGameplayTagContainer AllRequiredTags;
    static FGameplayTagContainer AllBlockedTags;

    AllRequiredTags = ActivationRequiredTags;
    AllBlockedTags = ActivationBlockedTags;

    // Expand our ability tags to add additional required/blocked tags
    if (MythicASC) {
        MythicASC->GetAdditionalActivationTagRequirements(GetAssetTags(), AllRequiredTags, AllBlockedTags);
    }

    // Check to see the required/blocked tags for this ability
    if (AllBlockedTags.Num() || AllRequiredTags.Num()) {
        static FGameplayTagContainer AbilitySystemComponentTags;

        AbilitySystemComponentTags.Reset();
        AbilitySystemComponent.GetOwnedGameplayTags(AbilitySystemComponentTags);

        if (AbilitySystemComponentTags.HasAny(AllBlockedTags)) {
            if (OptionalRelevantTags && AbilitySystemComponentTags.HasTag(GAS_STATE_DEAD)) {
                // If player is dead and was rejected due to blocking tags, give that feedback
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
