#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "World/Entity/MythicEntityId.h"
#include "World/Entity/MythicEntityPresentationState.h"
#include "World/Entity/MythicEntityPresentationTypes.h"
#include "MythicEntityPresentationComponent.generated.h"

class UAbilitySystemComponent;
class USceneComponent;
struct FActiveGameplayEffect;
struct FGameplayEffectSpec;
struct FOnAttributeChangeData;

/** Delegate tokens owned for one active GameplayEffect that can contribute to the bounded public status projection. */
struct FMythicProjectedStatusEffectDelegateHandles {
    FDelegateHandle StackChanged;
    FDelegateHandle TimeChanged;
    FDelegateHandle InhibitionChanged;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FMythicEntityPresentationRevision, uint64);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMythicEntityPresentationLifecycle,
                                    UMythicEntityPresentationComponent &,
                                    const FMythicEntityPresentationInstance &);

/**
 * Replicated, domain-facing presentation adapter for one embodied entity.
 *
 * It owns safe public identity, stateful observable facts, and a bounded status projection. It never decides widget
 * tier, relationship, quest state, danger, copy, color, or layout, and it can operate without an Ability System.
 */
UCLASS(ClassGroup = (Mythic), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicEntityPresentationComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicEntityPresentationComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    /** Returns the complete safe public identity snapshot; inactive means the actor is pooled/unbound and unusable. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Entity Presentation")
    const FMythicPublicIdentitySnapshot &GetPublicIdentitySnapshot() const { return PublicIdentity; }

    /** Returns the opaque public instance for focus/actions, or an invalid value while pooled or not fully active. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Entity Presentation")
    FMythicEntityPresentationInstance GetPresentationInstance() const {
        return PublicIdentity.IsActive() ? PublicIdentity.Instance : FMythicEntityPresentationInstance();
    }

    /** Returns a copy of current public executed facts after dropping rows from any stale embodiment generation. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Entity Presentation")
    TArray<FMythicObservableFactItem> GetObservableFacts() const;

    /** Allocation-free native fact view; consumers must still reject rows not stamped for the current instance. */
    TConstArrayView<FMythicObservableFactItem> GetObservableFactsView() const {
        return ObservableFacts.GetItems();
    }

    /** Returns a copy of bounded public status rows after dropping rows from any stale embodiment generation. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Entity Presentation")
    TArray<FMythicPublicStatusPresentationItem> GetPublicStatuses() const;

    /**
     * Returns the authority-authored normalized vitality for the current exact embodiment. Invalid means the subject
     * has no public health source; it never exposes exact current or maximum health.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Entity Presentation")
    const FMythicPublicVitalitySnapshot &GetPublicVitalitySnapshot() const {
        return PublicVitality;
    }

    /** Allocation-free native status view; consumers must still reject rows not stamped for the current instance. */
    TConstArrayView<FMythicPublicStatusPresentationItem> GetPublicStatusesView() const {
        return PublicStatuses.GetItems();
    }

    /** Quantizes a finite health fraction into the public eight-bit transport representation. */
    static uint8 QuantizePublicHealthFraction(float HealthFraction);

    /** Expands the public eight-bit transport value into a normalized presentation fraction. */
    static float DequantizePublicHealthFraction(uint8 QuantizedFraction);

    /**
     * Builds the only replication-safe identity shape from a domain candidate. It accepts only native visible kinds
     * and the typed identity-definition asset type; instance/active state are always authority-owned and cleared.
     */
    static bool BuildSanitizedPublicIdentity(
        const FMythicPublicIdentitySnapshot &Candidate,
        FMythicPublicIdentitySnapshot &OutSanitizedIdentity);

    /**
     * Returns the current world-space plate anchor. It uses the authored direct component plus offset, falling back to
     * the owner's root/location when the component is null or destroyed.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Entity Presentation")
    FVector GetPresentationAnchorWorldLocation() const;

    /**
     * Selects a direct scene component and local-centimeter offset as the plate anchor. Authority and local cosmetic
     * setup may call this; null safely falls back to the owner root and no string/component-name lookup is performed.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Entity Presentation")
    void SetPresentationAnchor(USceneComponent *InAnchor, FVector InLocalOffsetCentimeters);

    /**
     * Prepares an authority embodiment while the actor is hidden/unregistered. SafeIdentity must contain only public
     * tags/asset IDs; its instance/active fields are ignored. Returns false for client, invalid ID, or allocation error.
     */
    bool AuthorityPrepareEmbodiment(const FMythicEntityId &EntityId,
                                    const FMythicPublicIdentitySnapshot &SafeIdentity);

    /**
     * Publishes a completely prepared authority embodiment into the world registry. Returns false until preparation
     * succeeded; the actor must remain hidden/collision-disabled until this returns true.
     */
    bool AuthorityActivateEmbodiment();

    /**
     * Revokes and clears the current authority embodiment before pool mapping removal/hide. Client calls are ignored;
     * public facts/statuses and all GAS delegates are cleared, and the old handle can never resolve again.
     */
    void AuthorityDeactivateEmbodiment();

    /**
     * Writes one allowlisted, stateful public fact on authority. One slot owns one value; invalid/private slots or
     * values fail closed. RelatedSubject may be invalid and is always a public presentation handle, never an entity ID.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Entity Presentation")
    bool SetObservableFact(FGameplayTag FactSlotTag, FGameplayTag ValueTag,
                           FMythicPresentationHandle RelatedSubject);

    /** Clears one allowlisted public fact slot on authority; client or missing-slot calls are safe no-ops. */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Entity Presentation")
    void ClearObservableFact(FGameplayTag FactSlotTag);

    /**
     * Binds the canonical authority GAS status adapter and rebuilds public rows. Null unbinds safely. This never makes
     * private attributes, effect sources, or hidden status definitions public.
     */
    void AuthorityBindAbilitySystem(UAbilitySystemComponent *InAbilitySystem);

    /** Defers GAS-derived public status and vitality publication until the matching authority batch ends. */
    void AuthorityBeginAbilitySystemProjectionBatch();

    /** Publishes at most one final GAS-derived status/vitality snapshot after a nested authority batch completes. */
    void AuthorityEndAbilitySystemProjectionBatch();

    /** Authority-only private identity for persistence/domain systems; never expose or copy it into public replication. */
    const FMythicEntityId &GetAuthorityEntityId() const { return AuthorityEntityId; }

    /** True when the supplied public instance is exactly the component's complete active current embodiment. */
    bool RepresentsInstance(const FMythicEntityPresentationInstance &Instance) const {
        return PublicIdentity.IsActive() && PublicIdentity.Instance == Instance;
    }

    /** Replication callback used by the fact fast array after a complete client delta batch. */
    void HandleReplicatedFactsReceived();

    /** Replication callback used by the status fast array after a complete client delta batch. */
    void HandleReplicatedStatusesReceived();

    /** Emits after this component becomes publicly resolvable on the local world. */
    FMythicEntityPresentationLifecycle OnPresentationActivated;

    /** Emits before this component stops resolving on the local world. */
    FMythicEntityPresentationLifecycle OnPresentationDeactivated;

    /** Emits after safe identity, fact, status, or anchor state changes; payload is a monotonic local revision. */
    FMythicEntityPresentationRevision OnPresentationRevision;

private:
    friend struct FMythicEntityPresentationComponentTestAccess;

    UFUNCTION()
    void OnRep_PublicIdentity(FMythicPublicIdentitySnapshot PreviousIdentity);

    UFUNCTION()
    void OnRep_PublicVitality();

    void RegisterCurrentInstance();
    void UnregisterInstance(const FMythicEntityPresentationInstance &Instance);
    void PublishLocalRevision();
    void ClearPresentationState(bool bMarkReplicationDirty);
    void UnbindAbilitySystem();
    bool BindProjectedStatusEffectDelegates(
        UAbilitySystemComponent &AbilitySystem,
        FActiveGameplayEffectHandle EffectHandle,
        const FGameplayEffectSpec &EffectSpec);
    static bool GameplayEffectGrantsProjectedStatus(
        const FGameplayEffectSpec &EffectSpec,
        const FGameplayTagContainer &ProjectedStateTags);
    void BindExistingProjectedStatusEffects(
        UAbilitySystemComponent &AbilitySystem);
    void HandleActiveGameplayEffectAdded(
        UAbilitySystemComponent *TargetAbilitySystem,
        const FGameplayEffectSpec &EffectSpec,
        FActiveGameplayEffectHandle EffectHandle);
    void HandleActiveGameplayEffectRemoved(
        const FActiveGameplayEffect &RemovedEffect);
    void HandleStatusEffectStackChanged(
        FActiveGameplayEffectHandle EffectHandle, int32 NewCount,
        int32 PreviousCount);
    void HandleStatusEffectTimeChanged(
        FActiveGameplayEffectHandle EffectHandle, float NewStartTime,
        float NewDuration);
    void HandleStatusEffectInhibitionChanged(
        FActiveGameplayEffectHandle EffectHandle, bool bIsInhibited);
    void HandleStatusTagChanged(FGameplayTag StateTag, int32 NewCount);
    void HandleVitalityAttributeChanged(const FOnAttributeChangeData &ChangeData);
    void RequestAuthorityStatusRefresh();
    void FlushQueuedAuthorityStatusRefresh();
    void AuthorityRefreshPublicStatuses();
    void AuthorityRefreshPublicVitality();
    bool IsFactAllowed(FGameplayTag FactSlotTag, FGameplayTag ValueTag) const;
    bool IsCurrentFact(const FMythicObservableFactItem &Item) const;
    bool IsCurrentStatus(const FMythicPublicStatusPresentationItem &Item) const;
    void WakeOwnerForReplication();
    void FinishOwnerReplicationBatch();
    static uint32 AdvanceNonzeroRevision(uint32 &Counter);

    /** Complete safe public identity; it never contains the canonical authority/save entity ID or name seed. */
    UPROPERTY(ReplicatedUsing = OnRep_PublicIdentity)
    FMythicPublicIdentitySnapshot PublicIdentity;

    /** Late-join-safe public executed facts, each stamped with the current presentation handle/generation. */
    UPROPERTY(Replicated)
    FMythicObservableFactArray ObservableFacts;

    /** Late-join-safe bounded status rows derived authority-side from canonical status definitions and GAS. */
    UPROPERTY(Replicated)
    FMythicPublicStatusPresentationArray PublicStatuses;

    /** Late-join-safe normalized public health for this embodiment; exact GAS values never cross this boundary. */
    UPROPERTY(ReplicatedUsing = OnRep_PublicVitality)
    FMythicPublicVitalitySnapshot PublicVitality;

    /** Direct local anchor reference; actor/components already replicate through their owning embodiment. */
    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> PresentationAnchor;

    /** Local-centimeter offset transformed by PresentationAnchor, or added to the owner location on fallback. */
    UPROPERTY(Transient)
    FVector PresentationAnchorOffsetCentimeters = FVector(0.0, 0.0, 20.0);

    // Private authority-only state: no reflection means it cannot accidentally replicate to clients or Blueprint.
    FMythicEntityId AuthorityEntityId;
    TWeakObjectPtr<UAbilitySystemComponent> BoundAbilitySystem;
    FGameplayTagContainer ProjectedStatusStateTags;
    TMap<FGameplayTag, FDelegateHandle> StatusTagDelegateHandles;
    TMap<FActiveGameplayEffectHandle,
         FMythicProjectedStatusEffectDelegateHandles>
        StatusEffectDelegateHandles;
    FDelegateHandle ActiveGameplayEffectAddedDelegateHandle;
    FDelegateHandle ActiveGameplayEffectRemovedDelegateHandle;
    FDelegateHandle HealthAttributeDelegateHandle;
    FDelegateHandle MaximumHealthAttributeDelegateHandle;
    uint32 FactRevisionCounter = 0;
    uint32 StatusRevisionCounter = 0;
    uint32 VitalityRevisionCounter = 0;
    uint64 LocalRevision = 0;
    bool bRegisteredCurrentInstance = false;
    bool bStatusRefreshQueued = false;
    int32 AbilitySystemProjectionBatchDepth = 0;
    bool bStatusProjectionDirty = false;
    bool bVitalityProjectionDirty = false;
};
