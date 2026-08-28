#include "GAS/Abilities/MythicWeaponAttackAbility.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS/Abilities/MythicAnimNotify_SphereOverlap.h"
#include "GAS/Effects/MythicWeaponDamageEffects.h"
#include "GAS/MythicTags_GAS.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Itemization/InventoryProviderInterface.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/Fragments/Actionable/AttackFragment.h"
#include "Itemization/Inventory/Fragments/Passive/DurabilityFragment.h"
#include "Itemization/MythicTags_Inventory.h"
#include "Misc/DataValidation.h"
#include "Mythic.h"
#include "Resources/MythicResourceISM.h"
#include "World/Harvesting/MythicHarvestWorldSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MythicWeaponAttackAbility)

namespace {
struct FWeaponAttackMontageInstanceKey {
    TWeakObjectPtr<const USkeletalMeshComponent> MeshComponent;
    int32 MontageInstanceId = INDEX_NONE;

    friend bool operator==(const FWeaponAttackMontageInstanceKey &Left,
                           const FWeaponAttackMontageInstanceKey &Right) {
        return Left.MeshComponent == Right.MeshComponent
            && Left.MontageInstanceId == Right.MontageInstanceId;
    }

    friend uint32 GetTypeHash(const FWeaponAttackMontageInstanceKey &Key) {
        return HashCombine(GetTypeHash(Key.MeshComponent),
                           GetTypeHash(Key.MontageInstanceId));
    }
};

TMap<FWeaponAttackMontageInstanceKey,
     TWeakObjectPtr<UMythicWeaponAttackAbility>> &
WeaponAttackMontageRegistry() {
    static TMap<FWeaponAttackMontageInstanceKey,
                TWeakObjectPtr<UMythicWeaponAttackAbility>> Registry;
    return Registry;
}

void SetConfigurationError(FText *OutError, const TCHAR *Message) {
    if (OutError) {
        *OutError = FText::FromString(Message);
    }
}

IInventoryProviderInterface *ResolveInventoryProvider(
    const FGameplayAbilityActorInfo *ActorInfo) {
    if (!ActorInfo) {
        return nullptr;
    }
    AActor *Candidates[] = {ActorInfo->OwnerActor.Get(),
                            ActorInfo->AvatarActor.Get()};
    for (AActor *Candidate : Candidates) {
        if (IInventoryProviderInterface *Provider =
                Cast<IInventoryProviderInterface>(Candidate)) {
            return Provider;
        }
        if (const APawn *Pawn = Cast<APawn>(Candidate)) {
            if (IInventoryProviderInterface *Provider =
                    Cast<IInventoryProviderInterface>(Pawn->GetController())) {
                return Provider;
            }
        }
        if (const APlayerState *PlayerState = Cast<APlayerState>(Candidate)) {
            if (IInventoryProviderInterface *Provider =
                    Cast<IInventoryProviderInterface>(
                        PlayerState->GetPlayerController())) {
                return Provider;
            }
        }
    }
    return nullptr;
}

// The swing is authorized by gear-slot membership alone: the exact physical item must still occupy a gear slot of
// an inventory this ability's owner actually controls, so a client cannot attack with an item it has not equipped.
bool IsSourceItemEquipped(const FGameplayAbilityActorInfo *ActorInfo,
                          const UMythicItemInstance *SourceItem) {
    UMythicInventoryComponent *Inventory = IsValid(SourceItem)
        ? SourceItem->GetInventoryComponent()
        : nullptr;
    if (!Inventory) {
        return false;
    }
    const TArray<FMythicInventorySlotEntry> &Slots = Inventory->GetAllSlots();
    const int32 SlotIndex = SourceItem->GetSlot();
    if (!Slots.IsValidIndex(SlotIndex) || !Slots[SlotIndex].IsGearSlot()
        || Slots[SlotIndex].SlottedItemInstance.Get() != SourceItem) {
        return false;
    }
    const IInventoryProviderInterface *Provider =
        ResolveInventoryProvider(ActorInfo);
    return Provider
        && Provider->GetAllInventoryComponents().Contains(Inventory);
}

bool HasLivingAbilitySystem(const AActor *HitActor) {
    const UAbilitySystemComponent *AbilitySystem = IsValid(HitActor)
        ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor)
        : nullptr;
    return IsValid(AbilitySystem)
        && !AbilitySystem->HasMatchingGameplayTag(GAS_STATE_DEAD);
}

struct FCanonicalAttackHitIdentity {
    const UObject *TargetObject = nullptr;
    int32 InstanceIndex = INDEX_NONE;
    bool bPerInstance = false;

    bool IsValid() const { return TargetObject != nullptr; }

    friend bool operator==(const FCanonicalAttackHitIdentity &Left,
                           const FCanonicalAttackHitIdentity &Right) {
        return Left.TargetObject == Right.TargetObject
            && Left.InstanceIndex == Right.InstanceIndex
            && Left.bPerInstance == Right.bPerInstance;
    }

    friend uint32 GetTypeHash(const FCanonicalAttackHitIdentity &Identity) {
        return HashCombine(
            GetTypeHash(Identity.TargetObject),
            HashCombine(GetTypeHash(Identity.InstanceIndex),
                        GetTypeHash(
                            static_cast<uint8>(Identity.bPerInstance))));
    }
};

FCanonicalAttackHitIdentity ResolveDestructibleTargetIdentity(
    const FHitResult &Hit) {
    const FMythicDestructibleTargetIdentity Identity =
        FMythicDestructibleTargetIdentity::Resolve(Hit);
    return {Identity.TargetObject, Identity.InstanceIndex,
            Identity.bPerInstance};
}

FCanonicalAttackHitIdentity MakeCanonicalAttackHitIdentity(
    const FHitResult &Hit,
    const EMythicAttackSourceDomain /*SourceDomain*/) {
    const AActor *HitActor = Hit.GetActor();
    if (!IsValid(HitActor)) {
        return {};
    }

    // Harvestable ISM contacts are per-instance for every source domain. A sword is allowed to contact the node so
    // the harvest service can return Requires Axe, but two trees owned by one PCG actor must never collapse to one.
    if (Cast<UMythicResourceISM>(Hit.GetComponent())) {
        return ResolveDestructibleTargetIdentity(Hit);
    }
    // Living ASC targets keep one actor identity; destructible component instances must stay distinct; anything else
    // falls back to actor identity so an ordinary hit is never normalized away.
    if (HasLivingAbilitySystem(HitActor)) {
        return {HitActor, INDEX_NONE, false};
    }
    const FCanonicalAttackHitIdentity DestructibleIdentity =
        ResolveDestructibleTargetIdentity(Hit);
    if (DestructibleIdentity.IsValid()) {
        return DestructibleIdentity;
    }
    return {HitActor, INDEX_NONE, false};
}
}

UMythicWeaponAttackAbility::UMythicWeaponAttackAbility(
    const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {
    ApplyCanonicalExecutionPolicy();
}

void UMythicWeaponAttackAbility::ApplyCanonicalExecutionPolicy() {
    // The client predicts the same montage section/rate, but only the server may terminate its copy. In particular,
    // input release or a Blueprint cancel cannot shorten the authoritative attack cycle and start another swing.
    ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnlyTermination;
    bServerRespectsRemoteAbilityCancellation = false;
    bRetriggerInstancedAbility = false;
    bReplicateInputDirectly = false;
    ActivationPolicy = EMythicAbilityActivationPolicy::OnInputTriggered;
    // The complete authored section is the attack cycle used by the stat sheet/DPS projection. Keep it blocking so
    // an unrelated exclusive ability cannot erase post-hit recovery and create an unreported effective attack rate.
    // A future dodge/combo cancel system must open an explicit, data-authored window instead of inheriting a global
    // replace-any-time loophole.
    ActivationGroup = EMythicAbilityActivationGroup::Exclusive_Blocking;
    AbilityTriggers.Reset();
    CostGameplayEffectClass = nullptr;
    CooldownGameplayEffectClass = nullptr;
    AdditionalCosts.Reset();
    MaxCharges = 1;
    RechargeSeconds = 0.0f;
    CooldownCategoryTag = FGameplayTag();
}

void UMythicWeaponAttackAbility::PostLoad() {
    Super::PostLoad();
    // Blueprint class-default serialization can otherwise restore unsafe inherited GAS policy overrides after the
    // native constructor has run. Packaged content therefore receives the same sealed policy as fresh instances.
    ApplyCanonicalExecutionPolicy();
}

bool UMythicWeaponAttackAbility::IsCanonicalWeaponAttackConfiguration(
    FText *OutError) const {
    if (GetReplicationPolicy() != EGameplayAbilityReplicationPolicy::ReplicateNo
        || GetInstancingPolicy() != EGameplayAbilityInstancingPolicy::InstancedPerActor
        || GetNetExecutionPolicy() != EGameplayAbilityNetExecutionPolicy::LocalPredicted
        || GetNetSecurityPolicy() != EGameplayAbilityNetSecurityPolicy::ServerOnlyTermination
        || bServerRespectsRemoteAbilityCancellation || bRetriggerInstancedAbility
        || bReplicateInputDirectly
        || GetActivationPolicy() != EMythicAbilityActivationPolicy::OnInputTriggered
        || GetActivationGroup() != EMythicAbilityActivationGroup::Exclusive_Blocking
        || !AbilityTriggers.IsEmpty()) {
        SetConfigurationError(
            OutError,
            TEXT("Inherited GAS execution policy does not match the sealed weapon-attack policy"));
        return false;
    }
    if (CostGameplayEffectClass || CooldownGameplayEffectClass
        || !AdditionalCosts.IsEmpty() || MaxCharges != 1
        || RechargeSeconds != 0.0f || CooldownCategoryTag.IsValid()) {
        SetConfigurationError(
            OutError,
            TEXT("Inherited cost, cooldown, and charge fields must be empty for a cadence-owned weapon attack"));
        return false;
    }
    FName ForbiddenFunction;
    if (HasForbiddenBlueprintHookOverride(GetClass(),
                                          &ForbiddenFunction)) {
        if (OutError) {
            *OutError = FText::Format(
                NSLOCTEXT(
                    "MythicWeaponAttackAbility", "ForbiddenLifecycleHook",
                    "Blueprint hook override '{0}' bypasses the native weapon-attack contract"),
                FText::FromName(ForbiddenFunction));
        }
        return false;
    }
    return true;
}

bool UMythicWeaponAttackAbility::HasForbiddenBlueprintHookOverride(
    const UClass *AbilityClass, FName *OutFunctionName) {
    if (OutFunctionName) {
        *OutFunctionName = NAME_None;
    }
    if (!AbilityClass
        || !AbilityClass->IsChildOf(UMythicWeaponAttackAbility::StaticClass())) {
        return false;
    }

    struct FForbiddenBlueprintHook {
        FName FunctionName;
        const UClass *DeclaringClass;
    };
    static const FForbiddenBlueprintHook ForbiddenHooks[] = {
        {GET_FUNCTION_NAME_CHECKED(UGameplayAbility,
                                   K2_ShouldAbilityRespondToEvent),
         UGameplayAbility::StaticClass()},
        {GET_FUNCTION_NAME_CHECKED(UGameplayAbility,
                                   K2_CanActivateAbility),
         UGameplayAbility::StaticClass()},
        {GET_FUNCTION_NAME_CHECKED(UGameplayAbility, K2_ActivateAbility),
         UGameplayAbility::StaticClass()},
        {GET_FUNCTION_NAME_CHECKED(UGameplayAbility,
                                   K2_ActivateAbilityFromEvent),
         UGameplayAbility::StaticClass()},
        {GET_FUNCTION_NAME_CHECKED(UGameplayAbility, K2_CommitExecute),
         UGameplayAbility::StaticClass()},
        {GET_FUNCTION_NAME_CHECKED(UGameplayAbility, K2_OnEndAbility),
         UGameplayAbility::StaticClass()},
        {GET_FUNCTION_NAME_CHECKED(UGameplayAbility, BP_EditSpecValues),
         UGameplayAbility::StaticClass()},
        {GET_FUNCTION_NAME_CHECKED(UMythicGameplayAbility, K2_OnAbilityAdded),
         UMythicGameplayAbility::StaticClass()},
        {GET_FUNCTION_NAME_CHECKED(UMythicGameplayAbility, K2_OnAbilityRemoved),
         UMythicGameplayAbility::StaticClass()},
        {GET_FUNCTION_NAME_CHECKED(UMythicGameplayAbility, K2_OnAvatarSet),
         UMythicGameplayAbility::StaticClass()}};
    for (const FForbiddenBlueprintHook &Hook : ForbiddenHooks) {
        const UFunction *Function =
            AbilityClass->FindFunctionByName(Hook.FunctionName);
        const UClass *FunctionOwner = Function
            ? Function->GetOuterUClass()
            : nullptr;
        if (FunctionOwner
            && FunctionOwner != Hook.DeclaringClass) {
            if (OutFunctionName) {
                *OutFunctionName = Hook.FunctionName;
            }
            return true;
        }
    }
    return false;
}

bool UMythicWeaponAttackAbility::CommitAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo *ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    FGameplayTagContainer *OptionalRelevantTags) {
    if (!CommitCheck(Handle, ActorInfo, ActivationInfo,
                     OptionalRelevantTags)) {
        return false;
    }

    // UGameplayAbility::CommitAbility unconditionally dispatches K2_CommitExecute after its native commit. This
    // final override retains GAS's commit notification while deliberately omitting that Blueprint event.
    CommitExecute(Handle, ActorInfo, ActivationInfo);
    ActorInfo->AbilitySystemComponent->NotifyAbilityCommit(this);
    return true;
}

bool UMythicWeaponAttackAbility::CommitAbilityCooldown(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo *ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const bool /*bForceCooldown*/,
    FGameplayTagContainer *OptionalRelevantTags) {
    return CommitCheck(Handle, ActorInfo, ActivationInfo,
                       OptionalRelevantTags);
}

bool UMythicWeaponAttackAbility::CommitAbilityCost(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo *ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    FGameplayTagContainer *OptionalRelevantTags) {
    return CommitCheck(Handle, ActorInfo, ActivationInfo,
                       OptionalRelevantTags);
}

bool UMythicWeaponAttackAbility::CommitCheck(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo *ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    FGameplayTagContainer *OptionalRelevantTags) {
    return UGameplayAbility::CommitCheck(
        Handle, ActorInfo, ActivationInfo, OptionalRelevantTags);
}

void UMythicWeaponAttackAbility::CommitExecute(
    const FGameplayAbilitySpecHandle /*Handle*/,
    const FGameplayAbilityActorInfo * /*ActorInfo*/,
    const FGameplayAbilityActivationInfo /*ActivationInfo*/) {
}

UGameplayEffect *
UMythicWeaponAttackAbility::GetCooldownGameplayEffect() const {
    return nullptr;
}

UGameplayEffect *UMythicWeaponAttackAbility::GetCostGameplayEffect() const {
    return nullptr;
}

bool UMythicWeaponAttackAbility::CheckCooldown(
    const FGameplayAbilitySpecHandle /*Handle*/,
    const FGameplayAbilityActorInfo * /*ActorInfo*/,
    FGameplayTagContainer * /*OptionalRelevantTags*/) const {
    return true;
}

bool UMythicWeaponAttackAbility::CheckCost(
    const FGameplayAbilitySpecHandle /*Handle*/,
    const FGameplayAbilityActorInfo * /*ActorInfo*/,
    FGameplayTagContainer * /*OptionalRelevantTags*/) const {
    return true;
}

void UMythicWeaponAttackAbility::ApplyCooldown(
    const FGameplayAbilitySpecHandle /*Handle*/,
    const FGameplayAbilityActorInfo * /*ActorInfo*/,
    const FGameplayAbilityActivationInfo /*ActivationInfo*/) const {
}

void UMythicWeaponAttackAbility::ApplyCost(
    const FGameplayAbilitySpecHandle /*Handle*/,
    const FGameplayAbilityActorInfo * /*ActorInfo*/,
    const FGameplayAbilityActivationInfo /*ActivationInfo*/) const {
}

bool UMythicWeaponAttackAbility::ShouldAbilityRespondToEvent(
    const FGameplayAbilityActorInfo * /*ActorInfo*/,
    const FGameplayEventData * /*Payload*/) const {
    return false;
}

void UMythicWeaponAttackAbility::OnGiveAbility(
    const FGameplayAbilityActorInfo *ActorInfo,
    const FGameplayAbilitySpec &Spec) {
    // The engine implementation establishes CurrentActorInfo and routes an existing avatar through this class's
    // final OnAvatarSet. Calling the Mythic override would add Blueprint grant behavior and an OnSpawn policy that
    // are deliberately outside the weapon-attack contract.
    UGameplayAbility::OnGiveAbility(ActorInfo, Spec);
}

void UMythicWeaponAttackAbility::OnRemoveAbility(
    const FGameplayAbilityActorInfo *ActorInfo,
    const FGameplayAbilitySpec &Spec) {
    // Charges and shared cooldown categories are sealed off, so the Mythic override has no native cleanup to retain.
    UGameplayAbility::OnRemoveAbility(ActorInfo, Spec);
}

void UMythicWeaponAttackAbility::OnAvatarSet(
    const FGameplayAbilityActorInfo *ActorInfo,
    const FGameplayAbilitySpec &Spec) {
    UGameplayAbility::OnAvatarSet(ActorInfo, Spec);
}

const UAttackFragment *UMythicWeaponAttackAbility::ResolveAttackFragment(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo *ActorInfo) const {
    const UAbilitySystemComponent *ASC =
        ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    const FGameplayAbilitySpec *Spec = ASC ? ASC->FindAbilitySpecFromHandle(Handle) : nullptr;
    return Cast<UAttackFragment>(Spec ? Spec->SourceObject.Get() : nullptr);
}

EMythicAttackSourceDomain UMythicWeaponAttackAbility::ResolveAttackSourceDomain(
    const FGameplayTagContainer &ItemTypeProbe) {
    const bool bWeapon =
        ItemTypeProbe.HasTag(ITEMIZATION_TYPE_EQUIPMENT_WEAPON);
    const bool bTool = ItemTypeProbe.HasTag(ITEMIZATION_TYPE_EQUIPMENT_TOOL);
    // A tool never attacks, and an item claiming both domains is contradictory content.
    return bWeapon && !bTool ? EMythicAttackSourceDomain::Weapon
                             : EMythicAttackSourceDomain::Invalid;
}

EMythicAttackSourceDomain UMythicWeaponAttackAbility::ResolveAttackSourceDomain(
    const UAttackFragment *AttackFragment) {
    const UMythicItemInstance *ItemInstance = AttackFragment
        ? AttackFragment->GetOwningItemInstance()
        : nullptr;
    if (!IsValid(ItemInstance)) {
        return EMythicAttackSourceDomain::Invalid;
    }
    FGameplayTagContainer ItemTypeProbe;
    ItemInstance->GetTypeProbe(ItemTypeProbe);
    return ResolveAttackSourceDomain(ItemTypeProbe);
}

bool UMythicWeaponAttackAbility::IsTargetAllowedForSourceDomain(
    const EMythicAttackSourceDomain SourceDomain,
    const bool bHasLivingAbilitySystem,
    const bool bIsDestructible,
    const bool bIsHarvestableResource) {
    if (SourceDomain != EMythicAttackSourceDomain::Weapon) {
        return false;
    }
    // The weapon is the only attacker, so it reaches every target class: living targets, plain destructibles, and
    // harvestable nodes, where the harvest service decides whether the required tool is slotted.
    return bIsHarvestableResource || bIsDestructible || bHasLivingAbilitySystem;
}

void UMythicWeaponAttackAbility::FilterTargetHitsForSourceDomain(
    TArray<FHitResult> &InOutHits,
    const EMythicAttackSourceDomain SourceDomain,
    const AActor *AvatarActor) {
    InOutHits.RemoveAll(
        [SourceDomain, AvatarActor](const FHitResult &Hit) {
            const AActor *HitActor = Hit.GetActor();
            if (!IsValid(HitActor) || HitActor == AvatarActor) {
                return true;
            }
            return !IsTargetAllowedForSourceDomain(
                SourceDomain, HasLivingAbilitySystem(HitActor),
                ResolveDestructibleTargetIdentity(Hit).IsValid(),
                Cast<UMythicResourceISM>(Hit.GetComponent()) != nullptr);
        });
}

bool UMythicWeaponAttackAbility::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo *ActorInfo,
    const FGameplayTagContainer *SourceTags,
    const FGameplayTagContainer *TargetTags,
    FGameplayTagContainer *OptionalRelevantTags) const {
    FText ConfigurationError;
    if (!IsCanonicalWeaponAttackConfiguration(&ConfigurationError)) {
        UE_LOG(Myth, Error, TEXT("Weapon attack %s cannot activate: %s"),
               *GetPathName(), *ConfigurationError.ToString());
        return false;
    }

    // Validate the class before entering UGameplayAbility::CanActivateAbility: that engine path invokes a serialized
    // K2_CanActivateAbility implementation. Valid weapon classes have no such override, so the remaining native GAS,
    // tag, input-inhibition, and Mythic activation-group checks are retained without a hidden availability seam.
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags,
                                   OptionalRelevantTags)) {
        return false;
    }

    const UAttackFragment *AttackFragment = ResolveAttackFragment(Handle, ActorInfo);
    const EMythicAttackSourceDomain SourceDomain =
        ResolveAttackSourceDomain(AttackFragment);
    if (SourceDomain == EMythicAttackSourceDomain::Invalid) {
        SetConfigurationError(
            &ConfigurationError,
            TEXT("source item must match exactly one weapon or tool type domain"));
    }
    if (!AttackFragment
        || AttackFragment->AttackConfig.TriggerAbility.Get() != GetClass()
        || SourceDomain == EMythicAttackSourceDomain::Invalid
        || !AttackFragment->ResolveRuntimeAttackContract(&ConfigurationError)) {
        UE_LOG(Myth, Error,
               TEXT("Weapon attack %s requires an exact, live AttackFragment source: %s"),
               *GetPathName(),
               ConfigurationError.IsEmpty() ? TEXT("source ability/configuration mismatch")
                                            : *ConfigurationError.ToString());
        return false;
    }
    UMythicItemInstance *SourceItem = AttackFragment->GetOwningItemInstance();
    const UDurabilityFragment *Durability = SourceItem
        ? SourceItem->GetFragment<UDurabilityFragment>()
        : nullptr;
    if (!IsSourceItemEquipped(ActorInfo, SourceItem)
        || (Durability && Durability->IsBroken())) {
        UE_LOG(Myth, Verbose,
               TEXT("Weapon attack %s rejected an unequipped or broken physical source"),
               *GetPathName());
        return false;
    }
    return true;
}

void UMythicWeaponAttackAbility::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo *ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData *TriggerEventData) {
    // Deliberately do not call UGameplayAbility::ActivateAbility: its only behavior here would be dispatching the
    // Blueprint ActivateAbility graph, which would let a child replace this canonical sequence.
    bHitEventConsumed = false;
    bEndingNativeLifecycle = false;
    HitEventTask = nullptr;
    MontageTask = nullptr;
    AuthorizedHitSamples.Reset();
    ActiveSourceDomain = EMythicAttackSourceDomain::Invalid;
    ActiveHarvestCycleToken = FMythicHarvestAttackCycleToken();
    UnregisterActiveMontageInstance();

    FText ConfigurationError;
    const UAttackFragment *AttackFragment = ResolveAttackFragment(Handle, ActorInfo);
    UAnimMontage *AttackMontage =
        AttackFragment ? AttackFragment->AttackConfig.AttackMontage : nullptr;
    const EMythicAttackSourceDomain SourceDomain =
        ResolveAttackSourceDomain(AttackFragment);
    if (SourceDomain == EMythicAttackSourceDomain::Invalid) {
        SetConfigurationError(
            &ConfigurationError,
            TEXT("source item must match exactly one weapon or tool type domain"));
    }
    if (!AttackFragment
        || AttackFragment->AttackConfig.TriggerAbility.Get() != GetClass()
        || SourceDomain == EMythicAttackSourceDomain::Invalid
        || !IsCanonicalWeaponAttackConfiguration(&ConfigurationError)
        || !AttackFragment->ResolveRuntimeAttackContract(&ConfigurationError)) {
        UE_LOG(Myth, Error, TEXT("Weapon attack %s rejected invalid live source: %s"),
               *GetPathName(),
               ConfigurationError.IsEmpty() ? TEXT("source ability/configuration mismatch")
                                            : *ConfigurationError.ToString());
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    ActiveSourceDomain = SourceDomain;

    const FName SelectedSection = SelectAttackMontageSection(AttackFragment);
    const float PlayRate = GetClampedAttackSpeedPlayRate();
    TArray<const UMythicAnimNotify_SphereOverlap *> ResolvedHitSamples;
    const bool bResolvedSamples = AttackFragment->GetRuntimeAuthorizedHitSamples(
        SelectedSection, ResolvedHitSamples, &ConfigurationError);
    for (const UMythicAnimNotify_SphereOverlap *Sample : ResolvedHitSamples) {
        AuthorizedHitSamples.Add(Sample);
    }
    const bool bHasCanonicalMontageRateScale =
        UAttackFragment::HasCanonicalMontageRateScale(AttackMontage);
    if (!bHasCanonicalMontageRateScale && ConfigurationError.IsEmpty()) {
        SetConfigurationError(
            &ConfigurationError,
            TEXT("live attack montage Rate Scale must be finite and exactly 1.0; GAS AttackSpeed exclusively owns cadence"));
    }
    if (SelectedSection.IsNone() || !FMath::IsFinite(PlayRate)
        || PlayRate <= KINDA_SMALL_NUMBER || !bHasCanonicalMontageRateScale
        || !bResolvedSamples
        || AuthorizedHitSamples.IsEmpty()) {
        UE_LOG(Myth, Error,
               TEXT("Weapon attack %s could not resolve a valid section, play rate, or hit samples: %s"),
               *GetPathName(), *ConfigurationError.ToString());
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo)) {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (HasAuthority(&ActivationInfo)) {
        UWorld *World = GetWorld();
        UMythicHarvestWorldSubsystem *HarvestSubsystem = World
            ? World->GetSubsystem<UMythicHarvestWorldSubsystem>() : nullptr;
        if (!HarvestSubsystem
            || !HarvestSubsystem->BeginAttackCycle(
                *this, *AttackFragment, Handle, ActiveHarvestCycleToken)) {
            UE_LOG(Myth, Warning,
                   TEXT("Weapon attack %s committed without harvest-cycle provenance; combat remains available but resource contacts will fail closed."),
                   *GetPathName());
        }
    }

    HitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
        this, GAS_EVENT_HITBOX, nullptr, false, true);
    MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this, TEXT("CanonicalWeaponAttack"), AttackMontage, PlayRate,
        SelectedSection, true);
    if (!HitEventTask || !MontageTask) {
        UE_LOG(Myth, Error, TEXT("Weapon attack %s could not create canonical tasks"),
               *GetPathName());
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    HitEventTask->EventReceived.AddDynamic(
        this, &UMythicWeaponAttackAbility::HandleHitEvent);
    MontageTask->OnCompleted.AddDynamic(
        this, &UMythicWeaponAttackAbility::HandleMontageCompleted);
    MontageTask->OnInterrupted.AddDynamic(
        this, &UMythicWeaponAttackAbility::HandleMontageInterrupted);
    MontageTask->OnCancelled.AddDynamic(
        this, &UMythicWeaponAttackAbility::HandleMontageCancelled);

    // Keep the task armed until EndAbility: unrelated GAS.Event.Hitbox traffic must not consume the engine task
    // before an authorized temporal sample from this exact montage instance arrives. Sample authorization,
    // activation identity, and bHitEventConsumed jointly own one-shot semantics.
    HitEventTask->ReadyForActivation();
    if (IsActive()) {
        MontageTask->ReadyForActivation();
        if (IsActive() && !RegisterActiveMontageInstance(AttackMontage)) {
            UE_LOG(Myth, Error,
                   TEXT("Weapon attack %s could not register its montage-instance identity"),
                   *GetPathName());
            EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        }
    }
}

bool UMythicWeaponAttackAbility::TryConsumeHitEvent(bool &bInOutConsumed) {
    if (bInOutConsumed) {
        return false;
    }
    bInOutConsumed = true;
    return true;
}

bool UMythicWeaponAttackAbility::TryConsumeExpectedHitEvent(
    const TConstArrayView<const UMythicAnimNotify_SphereOverlap *> AuthorizedSamples,
    const UObject *ExpectedActivationToken,
    const FGameplayEventData &HitEvent, bool &bInOutConsumed) {
    const UMythicAnimNotify_SphereOverlap *EventSample =
        Cast<UMythicAnimNotify_SphereOverlap>(HitEvent.OptionalObject.Get());
    return ExpectedActivationToken && EventSample
        && EventSample->GetClass()
            == UMythicAnimNotify_SphereOverlap::StaticClass()
        && AuthorizedSamples.Contains(EventSample)
        && HitEvent.EventTag == GAS_EVENT_HITBOX
        && HitEvent.OptionalObject2.Get() == ExpectedActivationToken
        && TryConsumeHitEvent(bInOutConsumed);
}

UObject *UMythicWeaponAttackAbility::ResolveMontageActivationToken(
    const USkeletalMeshComponent *MeshComponent, const int32 MontageInstanceId) {
    if (!MeshComponent || MontageInstanceId == INDEX_NONE) {
        return nullptr;
    }
    FWeaponAttackMontageInstanceKey Key{MeshComponent, MontageInstanceId};
    TWeakObjectPtr<UMythicWeaponAttackAbility> *Entry =
        WeaponAttackMontageRegistry().Find(Key);
    UMythicWeaponAttackAbility *Ability = Entry ? Entry->Get() : nullptr;
    if (!Ability || !Ability->IsActive()) {
        WeaponAttackMontageRegistry().Remove(Key);
        return nullptr;
    }
    return Ability;
}

bool UMythicWeaponAttackAbility::RegisterActiveMontageInstance(
    const UAnimMontage *AttackMontage) {
    UnregisterActiveMontageInstance();
    USkeletalMeshComponent *MeshComponent = CurrentActorInfo
        ? CurrentActorInfo->SkeletalMeshComponent.Get()
        : nullptr;
    UAnimInstance *AnimInstance = MeshComponent
        ? MeshComponent->GetAnimInstance()
        : nullptr;
    FAnimMontageInstance *MontageInstance = AnimInstance
        ? AnimInstance->GetActiveInstanceForMontage(AttackMontage)
        : nullptr;
    if (!MeshComponent || !MontageInstance) {
        return false;
    }

    RegisteredMontageMesh = MeshComponent;
    RegisteredMontageInstanceId = MontageInstance->GetInstanceID();
    FWeaponAttackMontageInstanceKey Key{MeshComponent,
                                       RegisteredMontageInstanceId};
    WeaponAttackMontageRegistry().Add(Key, this);
    return true;
}

void UMythicWeaponAttackAbility::UnregisterActiveMontageInstance() {
    if (RegisteredMontageMesh && RegisteredMontageInstanceId != INDEX_NONE) {
        const FWeaponAttackMontageInstanceKey Key{
            RegisteredMontageMesh.Get(), RegisteredMontageInstanceId};
        const TWeakObjectPtr<UMythicWeaponAttackAbility> *Entry =
            WeaponAttackMontageRegistry().Find(Key);
        if (Entry && Entry->Get() == this) {
            WeaponAttackMontageRegistry().Remove(Key);
        }
    }
    RegisteredMontageMesh = nullptr;
    RegisteredMontageInstanceId = INDEX_NONE;
}

void UMythicWeaponAttackAbility::NormalizeUniqueTargetHits(
    TArray<FHitResult> &InOutHits,
    const AActor *AvatarActor,
    const EMythicAttackSourceDomain SourceDomain) {
    TSet<FCanonicalAttackHitIdentity> SeenTargets;
    InOutHits.RemoveAll(
        [AvatarActor, SourceDomain,
         &SeenTargets](const FHitResult &Hit) {
            const AActor *HitActor = Hit.GetActor();
            if (!IsValid(HitActor) || HitActor == AvatarActor) {
                return true;
            }
            const FCanonicalAttackHitIdentity Identity =
                MakeCanonicalAttackHitIdentity(Hit, SourceDomain);
            if (!Identity.IsValid() || SeenTargets.Contains(Identity)) {
                return true;
            }
            SeenTargets.Add(Identity);
            return false;
        });
}

void UMythicWeaponAttackAbility::ResolveCanonicalEventHits(
    const FGameplayEventData &HitEvent, TArray<FHitResult> &OutHits) {
    OutHits.Reset();
    for (int32 Index = 0; Index < HitEvent.TargetData.Num(); ++Index) {
        const FGameplayAbilityTargetData *TargetData = HitEvent.TargetData.Get(Index);
        if (!TargetData) {
            continue;
        }
        if (const FHitResult *HitResult = TargetData->GetHitResult()) {
            OutHits.Add(*HitResult);
            continue;
        }
        for (const TWeakObjectPtr<AActor> &WeakActor : TargetData->GetActors()) {
            if (AActor *Actor = WeakActor.Get()) {
                OutHits.Emplace(Actor, nullptr, Actor->GetActorLocation(),
                                FVector::ZeroVector);
            }
        }
    }
    if (OutHits.IsEmpty()) {
        if (AActor *PayloadTarget = const_cast<AActor *>(HitEvent.Target.Get())) {
            OutHits.Emplace(PayloadTarget, nullptr,
                            PayloadTarget->GetActorLocation(),
                            FVector::ZeroVector);
        }
    }
}

TArray<FHitResult>
UMythicWeaponAttackAbility::FilterWeaponAttackTargets_Implementation(
    const TArray<FHitResult> &CanonicalHits) const {
    return CanonicalHits;
}

void UMythicWeaponAttackAbility::IntersectWithCanonicalTargetHits(
    const TArray<FHitResult> &CanonicalHits,
    const TArray<FHitResult> &RequestedHits,
    TArray<FHitResult> &OutHits,
    const AActor *AvatarActor,
    const EMythicAttackSourceDomain SourceDomain) {
    TSet<FCanonicalAttackHitIdentity> RequestedTargets;
    for (const FHitResult &Requested : RequestedHits) {
        const FCanonicalAttackHitIdentity Identity =
            MakeCanonicalAttackHitIdentity(Requested, SourceDomain);
        if (Identity.IsValid()) {
            RequestedTargets.Add(Identity);
        }
    }

    OutHits.Reset();
    for (const FHitResult &Canonical : CanonicalHits) {
        const FCanonicalAttackHitIdentity Identity =
            MakeCanonicalAttackHitIdentity(Canonical, SourceDomain);
        if (Identity.IsValid() && RequestedTargets.Contains(Identity)) {
            OutHits.Add(Canonical);
        }
    }
    NormalizeUniqueTargetHits(OutHits, AvatarActor, SourceDomain);
}

void UMythicWeaponAttackAbility::HandleHitEvent(FGameplayEventData HitEvent) {
    if (!IsActive()
        || !TryConsumeExpectedHitEvent(AuthorizedHitSamples, this, HitEvent,
                                       bHitEventConsumed)) {
        return;
    }

    if (!HasAuthority(&CurrentActivationInfo)) {
        return;
    }

    TArray<FHitResult> CanonicalHits;
    ResolveCanonicalEventHits(HitEvent, CanonicalHits);
    FilterTargetHitsForSourceDomain(CanonicalHits, ActiveSourceDomain,
                                    GetAvatarActorFromActorInfo());
    NormalizeUniqueTargetHits(CanonicalHits, GetAvatarActorFromActorInfo(),
                              ActiveSourceDomain);
    const TArray<FHitResult> RequestedHits =
        FilterWeaponAttackTargets(CanonicalHits);
    TArray<FHitResult> Hits;
    IntersectWithCanonicalTargetHits(CanonicalHits, RequestedHits, Hits,
                                     GetAvatarActorFromActorInfo(),
                                     ActiveSourceDomain);
    if (Hits.IsEmpty()) {
        return;
    }

    TArray<FHitResult> CombatHits;
    CombatHits.Reserve(Hits.Num());
    bool bHarvestAccepted = false;
    UWorld *World = GetWorld();
    UMythicHarvestWorldSubsystem *HarvestSubsystem = World
        ? World->GetSubsystem<UMythicHarvestWorldSubsystem>() : nullptr;
    const UAttackFragment *AttackFragment = ResolveAttackFragment(
        CurrentSpecHandle, CurrentActorInfo);
    APawn *Avatar = Cast<APawn>(GetAvatarActorFromActorInfo());
    AController *Controller = Avatar ? Avatar->GetController() : nullptr;

    for (const FHitResult &Hit : Hits) {
        UMythicResourceISM *Resource =
            Cast<UMythicResourceISM>(Hit.GetComponent());
        if (!Resource) {
            CombatHits.Add(Hit);
            continue;
        }

        FPrimitiveInstanceId RuntimeInstanceId;
        FMythicHarvestNodeId NodeId;
        uint32 ExpectedGeneration = 0;
        if (!HarvestSubsystem || !AttackFragment || Hit.Item == INDEX_NONE
            || !HarvestSubsystem->ResolveHarvestTarget(
                *Resource, Hit.Item, RuntimeInstanceId, NodeId,
                ExpectedGeneration)) {
            continue;
        }

        FMythicHarvestRequest Request;
        Request.AuthorityAvatar = Avatar;
        Request.AuthorityController = Controller;
        Request.SourceAttackFragment =
            const_cast<UAttackFragment *>(AttackFragment);
        Request.ActiveAttackAbility = this;
        Request.AttackCycleToken = ActiveHarvestCycleToken;
        Request.TargetResource = Resource;
        Request.RuntimeInstanceId = RuntimeInstanceId;
        Request.ExpectedGeneration = ExpectedGeneration;
        Request.AuthoritativeHit = Hit;
        const FMythicHarvestResult HarvestResult =
            HarvestSubsystem->TryApplyHarvest(Request);
        bHarvestAccepted |= HarvestResult.WasAccepted();
    }

    if (!CombatHits.IsEmpty()) {
        // Harvest transactions own their exact source wear. A mixed contact therefore dispatches combat without a
        // second wear; when every resource contact rejected, ordinary combat retains canonical one-hit wear.
        ApplyDamageContainerNative(MythicWeaponDamage::MakeDamageContainer(),
                                   CombatHits, TArray<AActor *>(), -1,
                                   !bHarvestAccepted);
    }
}

void UMythicWeaponAttackAbility::HandleMontageCompleted() {
    FinishAttack(false);
}

void UMythicWeaponAttackAbility::HandleMontageInterrupted() {
    FinishAttack(true);
}

void UMythicWeaponAttackAbility::HandleMontageCancelled() {
    FinishAttack(true);
}

void UMythicWeaponAttackAbility::FinishAttack(const bool bWasCancelled) {
    if (bEndingNativeLifecycle || !IsActive()) {
        return;
    }
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true,
               bWasCancelled);
}

void UMythicWeaponAttackAbility::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo *ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const bool bReplicateEndAbility,
    const bool bWasCancelled) {
    if (bEndingNativeLifecycle || !IsEndAbilityValid(Handle, ActorInfo)) {
        return;
    }

    bEndingNativeLifecycle = true;
    if (ActiveHarvestCycleToken.IsValid()) {
        if (UWorld *World = GetWorld()) {
            if (UMythicHarvestWorldSubsystem *HarvestSubsystem =
                    World->GetSubsystem<UMythicHarvestWorldSubsystem>()) {
                HarvestSubsystem->EndAttackCycle(ActiveHarvestCycleToken, this);
            }
        }
    }
    ActiveHarvestCycleToken = FMythicHarvestAttackCycleToken();
    UnregisterActiveMontageInstance();
    HitEventTask = nullptr;
    MontageTask = nullptr;
    AuthorizedHitSamples.Reset();
    ActiveSourceDomain = EMythicAttackSourceDomain::Invalid;
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility,
                      bWasCancelled);
    bHitEventConsumed = false;
    bEndingNativeLifecycle = false;
}

#if WITH_EDITOR
EDataValidationResult UMythicWeaponAttackAbility::IsDataValid(
    FDataValidationContext &Context) const {
    EDataValidationResult Result = Super::IsDataValid(Context);
    FText Error;
    if (!IsCanonicalWeaponAttackConfiguration(&Error)) {
        Context.AddError(FText::Format(
            NSLOCTEXT("MythicWeaponAttackAbility", "InvalidConfiguration",
                      "Weapon attack configuration is invalid: {0}"),
            Error));
        Result = EDataValidationResult::Invalid;
    }
    return Result;
}

bool UMythicWeaponAttackAbility::CanEditChange(
    const FProperty *InProperty) const {
    if (InProperty) {
        static const TSet<FName> SealedPolicyProperties = {
            TEXT("ReplicationPolicy"),
            TEXT("InstancingPolicy"),
            TEXT("NetExecutionPolicy"),
            TEXT("NetSecurityPolicy"),
            TEXT("bServerRespectsRemoteAbilityCancellation"),
            TEXT("bRetriggerInstancedAbility"),
            TEXT("bReplicateInputDirectly"),
            TEXT("ActivationPolicy"),
            TEXT("ActivationGroup"),
            TEXT("AbilityTriggers"),
            TEXT("CostGameplayEffectClass"),
            TEXT("CooldownGameplayEffectClass"),
            TEXT("AdditionalCosts"),
            TEXT("MaxCharges"),
            TEXT("RechargeSeconds"),
            TEXT("CooldownCategoryTag")};
        if (SealedPolicyProperties.Contains(InProperty->GetFName())) {
            return false;
        }
    }
    return Super::CanEditChange(InProperty);
}

void UMythicWeaponAttackAbility::PostEditChangeProperty(
    FPropertyChangedEvent &PropertyChangedEvent) {
    Super::PostEditChangeProperty(PropertyChangedEvent);
    ApplyCanonicalExecutionPolicy();
}
#endif
