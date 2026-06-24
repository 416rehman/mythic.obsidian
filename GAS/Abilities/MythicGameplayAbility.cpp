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
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "Itemization/MythicTags_Inventory.h"
#include "Itemization/Inventory/Fragments/Passive/DurabilityFragment.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h" // CooldownReduction (ApplyCooldown scaling)
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GameModes/GameState/MythicGameState.h"                 // MaxCooldownReduction cap
#include "GameplayEffect.h"                                       // FGameplayEffectSpec Get/SetDuration
#include "Engine/World.h"                                         // World->GetGameState<AMythicGameState>()

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
    if (!OwningASC) {
        // No owning ASC (stale/null actor info — e.g. this BlueprintCallable called before avatar set, or after
        // actor-info teardown). Return the default spec; the sole consumer ApplyDamageContainerSpec already
        // IsValid-guards both sub-specs, so this is graceful (no damage) rather than a server crash on the damage path.
        UE_LOG(Myth, Error, TEXT("UMythicGameplayAbility::MakeDamageContainerSpec: no owning ASC (stale/null actor info)"));
        return ReturnSpec;
    }
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

    // Thread the designer-assigned status->GE mapping so per-target debuff application can read it later.
    ReturnSpec.StatusEffects = Container.StatusEffects;

    // Inject the WIELDING weapon's class tag (Itemization.Type.Equipment.Weapon.Sword/Axe/…) into the damage specs'
    // source tags so MythicDamageApplication can apply the matching BonusXxxDamage. The weapon's ItemType tag lives on
    // the item INSTANCE, not the source ASC, so it never reaches CapturedSourceTags on its own — AppendDynamicAssetTags
    // is the engine-documented path that injects into the captured source spec tags AND survives re-capture. Injected
    // into BOTH damage specs so it lands regardless of which one owns the execution calc (harmless on the other). The
    // wielding weapon is the granted ability's SourceObject (same resolution as the durability check).
    if (UObject *Src = GetCurrentSourceObject()) {
        if (UItemFragment *SrcFragment = Cast<UItemFragment>(Src)) {
            if (UMythicItemInstance *WeaponInst = SrcFragment->GetOwningItemInstance()) {
                FGameplayTagContainer TypeProbe;
                WeaponInst->GetTypeProbe(TypeProbe);
                // Only the weapon-class subtree — never the full probe (would pollute source tags with rarity/affix tags).
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
    auto activations = SourceASC->HandleGameplayEvent(EventTag, &EventData);
    UE_LOG(Myth, Warning, TEXT("%s event sent to source %d abilities"), *EventTag.ToString(), activations);
}

TArray<FActiveGameplayEffectHandle> UMythicGameplayAbility::ApplyDamageContainerSpec(const FMythicDamageContainerSpec &ContainerSpec) {
    TArray<FActiveGameplayEffectHandle> AllEffects;

    // This prevents double damage in locally predicted abilities
    if (!CurrentActorInfo->IsNetAuthority()) {
        return AllEffects;
    }

    // 1. CREATE DAMAGE CONTEXT - This stage only sets the context like isCritical, isBlocked, etc. No damage is calculated yet.
    if (!ContainerSpec.DamageCalculationEffectSpec.IsValid()) {
        UE_LOG(Myth, Error, TEXT("UMythicGameplayAbility::ApplyDamageContainerSpec: ContainerSpec.DamageCalculationEffectSpec is null"));
    }
    auto CalculationEffectHandle = K2_ApplyGameplayEffectSpecToOwner(ContainerSpec.DamageCalculationEffectSpec);
    AllEffects.Push(CalculationEffectHandle);

    // 2. SEND DAMAGE PRE EVENT
    if (ContainerSpec.TargetsHandle.Num() > 0) {
        SendEvent(ContainerSpec.TargetsHandle, ContainerSpec.EffectContextHandle, GAS_EVENT_DMG_PRE);

        // 3. APPLY DAMAGE per-target. C1 fix: each target gets its OWN duplicated context so the per-target
        // dodge + resistance-gate writes in UMythicDamageApplication don't leak across AoE targets (the damage
        // math is already per-target; only the context-flag writes were colliding). After the damage lands,
        // apply any designer-mapped debuff GE for each status flag that survived this target's resistance gate.
        if (!ContainerSpec.DamageApplicationEffectSpec.IsValid()) {
            UE_LOG(Myth, Error, TEXT("UMythicGameplayAbility::ApplyDamageContainerSpec: ContainerSpec.DamageApplicationEffectSpec is null"));
        }
        else {
            const float DebuffLevel = ContainerSpec.DamageApplicationEffectSpec.Data->GetLevel();
            for (int32 i = 0; i < ContainerSpec.TargetsHandle.Data.Num(); ++i) {
                if (!ContainerSpec.TargetsHandle.Data[i].IsValid()) {
                    continue;
                }

                // Per-target isolated context (deep copy of the shared source-intent context).
                FGameplayEffectContextHandle PerTargetContext = ContainerSpec.EffectContextHandle.Duplicate();

                // Per-target application spec: copy the finalized spec and reseat its context.
                FGameplayEffectSpecHandle PerTargetSpec(new FGameplayEffectSpec(*ContainerSpec.DamageApplicationEffectSpec.Data));
                PerTargetSpec.Data->SetContext(PerTargetContext);

                // A handle wrapping just this one target.
                FGameplayAbilityTargetDataHandle SingleTarget;
                SingleTarget.Data.Add(ContainerSpec.TargetsHandle.Data[i]);

                AllEffects.Append(K2_ApplyGameplayEffectSpecToTarget(PerTargetSpec, SingleTarget));

                // Apply per-target debuffs for the flags that survived resistance (skips null-mapped statuses).
                AllEffects.Append(ApplyMappedStatusEffects(PerTargetContext, ContainerSpec.StatusEffects, SingleTarget, DebuffLevel));
            }
        }
    }

    // 4. Handle destructibles
    if (ContainerSpec.DestructibleTargetsHandle.Num() > 0) {
        // 4.1. SEND DAMAGE PRE EVENT - No damage is calculated as part of this
        SendEvent(ContainerSpec.DestructibleTargetsHandle, ContainerSpec.EffectContextHandle, GAS_EVENT_DMG_DESTRUCTIBLE);
    }

    return AllEffects;
}

TArray<FActiveGameplayEffectHandle> UMythicGameplayAbility::ApplyMappedStatusEffects(const FGameplayEffectContextHandle &PerTargetContext,
                                                                                     const FMythicStatusEffectMapping &Mapping,
                                                                                     const FGameplayAbilityTargetDataHandle &SingleTarget, float Level) {
    TArray<FActiveGameplayEffectHandle> Applied;

    const FMythicGameplayEffectContext *Ctx = static_cast<const FMythicGameplayEffectContext *>(PerTargetContext.Get());
    if (!Ctx) {
        return Applied;
    }
    UAbilitySystemComponent *ASC = GetAbilitySystemComponentFromActorInfo();
    if (!ASC) {
        return Applied;
    }

    // Apply a status's mapped debuff GE only if it was rolled (survived resistance) AND the designer assigned
    // an effect for it. A null mapping means "no debuff authored yet" - skipped, never fabricated.
    auto ApplyOne = [&](bool bFlagSet, const TSubclassOf<UGameplayEffect> &GE) {
        if (!bFlagSet || !GE) {
            return;
        }
        FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(GE, Level, PerTargetContext);
        if (Spec.IsValid()) {
            Applied.Append(K2_ApplyGameplayEffectSpecToTarget(Spec, SingleTarget));
        }
    };

    ApplyOne(Ctx->IsBleed(), Mapping.BleedEffect);
    ApplyOne(Ctx->IsBurn(), Mapping.BurnEffect);
    ApplyOne(Ctx->IsPoison(), Mapping.PoisonEffect);
    ApplyOne(Ctx->IsStun(), Mapping.StunEffect);
    ApplyOne(Ctx->IsSlow(), Mapping.SlowEffect);
    ApplyOne(Ctx->IsFreeze(), Mapping.FreezeEffect);
    ApplyOne(Ctx->IsWeaken(), Mapping.WeakenEffect);
    ApplyOne(Ctx->IsTerrify(), Mapping.TerrifyEffect);

    return Applied;
}


TArray<FActiveGameplayEffectHandle> UMythicGameplayAbility::ApplyDamageContainer(const FMythicDamageContainer &Container, const TArray<FHitResult> &HitResults,
                                                                                 const TArray<AActor *> &TargetActors,
                                                                                 int32 OverrideGameplayLevel) {
    TArray<FActiveGameplayEffectHandle> AllEffects;

    // Only apply damage on server (authority)
    if (!CurrentActorInfo->IsNetAuthority()) {
        return AllEffects;
    }

    if (TargetActors.IsEmpty() || HitResults.IsEmpty()) {
        UE_LOG(Myth, Warning, TEXT("No Targets"));
    }

    // Equipment durability: if this attack was granted by an item fragment whose item carries a durability
    // fragment, a BROKEN weapon deals no damage (early-out), and a working one loses 1 durability per landed
    // hit. The fragment is the granted ability's SourceObject (see UActionableItemFragment::GrantItemAbility).
    if (!TargetActors.IsEmpty() && !HitResults.IsEmpty()) {
        if (UObject *Src = GetCurrentSourceObject()) {
            if (UItemFragment *SourceFragment = Cast<UItemFragment>(Src)) {
                if (UMythicItemInstance *Inst = SourceFragment->GetOwningItemInstance()) {
                    if (const UDurabilityFragment *Durability = Inst->GetFragment<UDurabilityFragment>()) {
                        if (Durability->IsBroken()) {
                            // Broken weapon: swing connects but deals no damage until repaired.
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

namespace {
    // Cooldown duration multiplier = 1 - clamp(CooldownReduction, 0, MaxCooldownReduction). Returns 1.0 (no change)
    // when the owner has no Utility set or no reduction, so non-CDR owners (incl. most NPCs) get stock cooldowns.
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
        // Designer-tunable safety cap (default 0.8) keeps a sliver of cooldown so stacked CDR can't reach zero.
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

    // No reduction (the common case — no CDR gear/buffs): defer entirely to the stock engine cooldown application so
    // those owners are provably unaffected by this override.
    if (Mult >= 1.0f) {
        Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);
        return;
    }

    UGameplayEffect *CooldownGE = GetCooldownGameplayEffect();
    if (!CooldownGE) {
        return; // ability has no cooldown — nothing to reduce
    }
    // Mirror UGameplayAbility::ApplyGameplayEffectToOwner (incl. the prediction-key gate so the client's predicted
    // cooldown matches the server — CooldownReduction replicates, so both sides scale identically), but inject the
    // scaled, locked duration into the spec before applying.
    if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo)) {
        return;
    }
    FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, CooldownGE->GetClass(), GetAbilityLevel(Handle, ActorInfo));
    if (!SpecHandle.IsValid()) {
        return;
    }
    const float BaseDuration = SpecHandle.Data->GetDuration();
    if (BaseDuration > 0.0f) { // only scale a finite, positive cooldown (skip Instant/Infinite durations)
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

float UMythicGameplayAbility::GetClampedAttackSpeedPlayRate() const {
    float AttackSpeedScale = 1.0f;
    if (UAbilitySystemComponent *ASC = GetAbilitySystemComponentFromActorInfo()) {
        if (const UMythicAttributeSet_Offense *Offense = ASC->GetSet<UMythicAttributeSet_Offense>()) {
            AttackSpeedScale = Offense->GetAttackSpeed();
        }
    }

    // clamp play rate within safe limits to prevent animnotify skips
    return FMath::Clamp(AttackSpeedScale, 0.8f, 1.4f);
}
