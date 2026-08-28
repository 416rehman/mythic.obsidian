#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/MythicGameplayAbility.h"
#include "GameplayTagContainer.h"
#include "World/Harvesting/MythicHarvestTypes.h"
#include "MythicWeaponAttackAbility.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAnimMontage;
class UAttackFragment;
class UGameplayEffect;
class UMythicAnimNotify_SphereOverlap;
class USkeletalMeshComponent;

/** Native target domain resolved from the exact item that granted an attack ability. Only an equipped weapon
 *  attacks: a harvesting tool authorizes work by occupying its slot and never swings. */
enum class EMythicAttackSourceDomain : uint8 {
    Invalid,
    Weapon
};

/**
 * Canonical execution path for an item-granted weapon attack.
 *
 * The native class owns source validation, commit, prediction-stable variant selection, attack-speed cadence,
 * the single impact event, damage dispatch, and every terminal montage path. Blueprint children are presentation
 * and target-selection assets only; they cannot replace the activation graph or author a second damage cadence.
 */
UCLASS(
    Abstract, Blueprintable, HideCategories = (Advanced, Triggers),
    meta = (KismetHideOverrides =
                "K2_ShouldAbilityRespondToEvent,K2_CanActivateAbility,K2_ActivateAbility,K2_ActivateAbilityFromEvent,K2_CommitExecute,K2_OnEndAbility,BP_EditSpecValues,K2_OnAbilityAdded,K2_OnAbilityRemoved,K2_OnAvatarSet"))
class MYTHIC_API UMythicWeaponAttackAbility : public UMythicGameplayAbility {
    GENERATED_BODY()

public:
    UMythicWeaponAttackAbility(const FObjectInitializer &ObjectInitializer = FObjectInitializer::Get());

    /** Returns true only when this class has the sealed network, activation-group, and cadence policy. */
    bool IsCanonicalWeaponAttackConfiguration(FText *OutError = nullptr) const;

    /** Returns true when a Blueprint class serializes any hook outside the target-filter presentation seam. */
    static bool HasForbiddenBlueprintHookOverride(
        const UClass *AbilityClass, FName *OutFunctionName = nullptr);

    /** Claims the first impact occurrence and rejects every later occurrence from the same selected section. */
    static bool TryConsumeHitEvent(bool &bInOutConsumed);

    /**
     * Rejects foreign same-tag traffic unless it carries both an authorized temporal sample and this activation's
     * transient montage token; the first accepted sample spends the section's single damage budget.
     */
    static bool TryConsumeExpectedHitEvent(
        TConstArrayView<const UMythicAnimNotify_SphereOverlap *> AuthorizedSamples,
        const UObject *ExpectedActivationToken,
        const FGameplayEventData &HitEvent, bool &bInOutConsumed);

    /** Resolves exactly one weapon/tool domain from an item's current canonical type probe; ambiguous probes fail. */
    static EMythicAttackSourceDomain ResolveAttackSourceDomain(
        const FGameplayTagContainer &ItemTypeProbe);

    /** Resolves the attack domain from the exact live item instance that owns the granting fragment. */
    static EMythicAttackSourceDomain ResolveAttackSourceDomain(
        const UAttackFragment *AttackFragment);

    /** Pure source/target-domain policy seam shared by runtime filtering and focused automation coverage. */
    static bool IsTargetAllowedForSourceDomain(
        EMythicAttackSourceDomain SourceDomain,
        bool bHasLivingAbilitySystem,
        bool bIsDestructible,
        bool bIsHarvestableResource = false);

    /** Removes invalid, self, dead, or cross-domain hits without changing canonical hit geometry. */
    static void FilterTargetHitsForSourceDomain(
        TArray<FHitResult> &InOutHits,
        EMythicAttackSourceDomain SourceDomain,
        const AActor *AvatarActor);

    /**
     * Removes invalid, self, and duplicate hits while preserving deterministic source order. Living combatants and
     * actor-backed destructibles use actor identity; component-backed destructibles use the exact component and
     * FHitResult instance index.
     */
    static void NormalizeUniqueTargetHits(
        TArray<FHitResult> &InOutHits,
        const AActor *AvatarActor,
        EMythicAttackSourceDomain SourceDomain = EMythicAttackSourceDomain::Invalid);

    /**
     * Intersects a Blueprint-filtered target list with the native query results, preserving only original hit data
     * and preventing a filter from expanding radius, cap, or target policy.
     */
    static void IntersectWithCanonicalTargetHits(
        const TArray<FHitResult> &CanonicalHits,
        const TArray<FHitResult> &RequestedHits,
        TArray<FHitResult> &OutHits,
        const AActor *AvatarActor,
        EMythicAttackSourceDomain SourceDomain = EMythicAttackSourceDomain::Invalid);

    /** Resolves the active ability token registered for an exact skeletal-mesh montage instance. */
    static UObject *ResolveMontageActivationToken(
        const USkeletalMeshComponent *MeshComponent, int32 MontageInstanceId);

    /** Returns the exact source domain captured before this activation committed. */
    EMythicAttackSourceDomain GetActiveSourceDomain() const {
        return ActiveSourceDomain;
    }

    /** Native commit path: validates the active spec and broadcasts commit without dispatching Blueprint hooks. */
    virtual bool CommitAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo *ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        FGameplayTagContainer *OptionalRelevantTags = nullptr) override final;

    /** Weapon attacks have no secondary cooldown contract beyond their authored montage cycle. */
    virtual bool CommitAbilityCooldown(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo *ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bForceCooldown,
        FGameplayTagContainer *OptionalRelevantTags = nullptr) override final;

    /** Weapon attacks have no activation cost contract outside hit-driven durability wear. */
    virtual bool CommitAbilityCost(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo *ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        FGameplayTagContainer *OptionalRelevantTags = nullptr) override final;

    virtual bool CommitCheck(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo *ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        FGameplayTagContainer *OptionalRelevantTags = nullptr) override final;
    virtual void CommitExecute(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo *ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo) override final;
    virtual UGameplayEffect *GetCooldownGameplayEffect() const override final;
    virtual UGameplayEffect *GetCostGameplayEffect() const override final;
    virtual bool CheckCooldown(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo *ActorInfo,
        FGameplayTagContainer *OptionalRelevantTags = nullptr) const override final;
    virtual bool CheckCost(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo *ActorInfo,
        FGameplayTagContainer *OptionalRelevantTags = nullptr) const override final;
    virtual void ApplyCooldown(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo *ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo) const override final;
    virtual void ApplyCost(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo *ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo) const override final;

    /** Item attacks are input-only and never opt into gameplay-event activation routing. */
    virtual bool ShouldAbilityRespondToEvent(
        const FGameplayAbilityActorInfo *ActorInfo,
        const FGameplayEventData *Payload) const override final;

    /** Preserves native GAS grant bookkeeping without dispatching Blueprint grant or avatar hooks. */
    virtual void OnGiveAbility(
        const FGameplayAbilityActorInfo *ActorInfo,
        const FGameplayAbilitySpec &Spec) override final;

    /** Removes a weapon ability without dispatching Blueprint removal hooks. */
    virtual void OnRemoveAbility(
        const FGameplayAbilityActorInfo *ActorInfo,
        const FGameplayAbilitySpec &Spec) override final;

    /** Weapon avatar changes require no Blueprint lifecycle behavior. */
    virtual void OnAvatarSet(
        const FGameplayAbilityActorInfo *ActorInfo,
        const FGameplayAbilitySpec &Spec) override final;

    virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                    const FGameplayAbilityActorInfo *ActorInfo,
                                    const FGameplayTagContainer *SourceTags,
                                    const FGameplayTagContainer *TargetTags,
                                    FGameplayTagContainer *OptionalRelevantTags) const override final;

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                 const FGameplayAbilityActorInfo *ActorInfo,
                                 const FGameplayAbilityActivationInfo ActivationInfo,
                                 const FGameplayEventData *TriggerEventData) override final;

    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
                            const FGameplayAbilityActorInfo *ActorInfo,
                            const FGameplayAbilityActivationInfo ActivationInfo,
                            bool bReplicateEndAbility,
                            bool bWasCancelled) override final;

    virtual void PostLoad() override;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext &Context) const override;
    virtual bool CanEditChange(const FProperty *InProperty) const override;
    virtual void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override;
#endif

protected:
    /**
     * Filters native query results for weapon-specific target rules. Returned actors are intersected with the
     * original results, so Blueprint can reject targets but cannot fabricate range, cap, geometry, or authority.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Mythic|Ability|Weapon Attack",
              meta = (BlueprintProtected = "true", DisplayName = "Filter Weapon Attack Targets",
                      ToolTip = "Filter the native weapon-hit results. Added or altered targets are discarded by native validation."))
    TArray<FHitResult> FilterWeaponAttackTargets(
        const TArray<FHitResult> &CanonicalHits) const;
    virtual TArray<FHitResult> FilterWeaponAttackTargets_Implementation(
        const TArray<FHitResult> &CanonicalHits) const;

private:
    void ApplyCanonicalExecutionPolicy();
    const UAttackFragment *ResolveAttackFragment(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo *ActorInfo) const;
    static void ResolveCanonicalEventHits(const FGameplayEventData &HitEvent,
                                          TArray<FHitResult> &OutHits);
    bool RegisterActiveMontageInstance(const UAnimMontage *AttackMontage);
    void UnregisterActiveMontageInstance();
    void FinishAttack(bool bWasCancelled);

    UFUNCTION()
    void HandleHitEvent(FGameplayEventData HitEvent);

    UFUNCTION()
    void HandleMontageCompleted();

    UFUNCTION()
    void HandleMontageInterrupted();

    UFUNCTION()
    void HandleMontageCancelled();

    UPROPERTY(Transient)
    TObjectPtr<UAbilityTask_WaitGameplayEvent> HitEventTask = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask = nullptr;

    TArray<const UMythicAnimNotify_SphereOverlap *> AuthorizedHitSamples;

    UPROPERTY(Transient)
    TObjectPtr<USkeletalMeshComponent> RegisteredMontageMesh = nullptr;

    int32 RegisteredMontageInstanceId = INDEX_NONE;

    EMythicAttackSourceDomain ActiveSourceDomain =
        EMythicAttackSourceDomain::Invalid;
    FMythicHarvestAttackCycleToken ActiveHarvestCycleToken;
    bool bHitEventConsumed = false;
    bool bEndingNativeLifecycle = false;
};
