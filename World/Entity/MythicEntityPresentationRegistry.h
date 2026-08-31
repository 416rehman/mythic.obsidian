#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/Entity/MythicEntityId.h"
#include "World/Entity/MythicEntityPresentationTypes.h"

#include "MythicEntityPresentationRegistry.generated.h"

class UMythicEntityPresentationComponent;

DECLARE_MULTICAST_DELEGATE_TwoParams(
    FMythicPresentationInstanceRegistered,
    const FMythicEntityPresentationInstance &,
    UMythicEntityPresentationComponent *);

DECLARE_MULTICAST_DELEGATE_TwoParams(
    FMythicPresentationInstanceUnregistered,
    const FMythicEntityPresentationInstance &,
    UMythicEntityPresentationComponent *);

/**
 * Per-world push registry for active entity-presentation components.
 *
 * Public resolution is keyed only by the opaque handle-generation pair. Canonical entity mappings exist only in
 * non-reflected authority memory and are never available to clients or Blueprint. This subsystem is game-thread only.
 */
UCLASS()
class MYTHIC_API UMythicEntityPresentationRegistry : public UWorldSubsystem {
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;

    /**
     * Allocates a fresh opaque nonce and globally advancing embodiment generation for a canonical authority entity.
     * Client worlds, invalid IDs, and off-game-thread calls return an invalid instance. Reallocating the same logical
     * entity releases its previous presentation instance before returning the new one.
     */
    FMythicEntityPresentationInstance AllocateAuthorityInstance(
        const FMythicEntityId &EntityId);

    /**
     * Registers a fully bound component under its public handle-generation pair on authority or client.
     * Returns false for invalid input or a live collision; registering the same component and instance is idempotent.
     */
    bool RegisterPresentationComponent(
        const FMythicEntityPresentationInstance &Instance,
        UMythicEntityPresentationComponent *Component);

    /**
     * Removes a public registration only when both its instance and expected component match.
     * Authority canonical resolution intentionally remains available until ReleaseAuthorityInstance is called.
     */
    void UnregisterPresentationComponent(
        const FMythicEntityPresentationInstance &Instance,
        UMythicEntityPresentationComponent *ExpectedComponent);

    /**
     * Releases authority-only canonical mappings for an embodiment and removes any remaining public registration.
     * Client and off-game-thread calls are ignored; this should run after viewer grants are revoked during deactivation.
     */
    void ReleaseAuthorityInstance(
        const FMythicEntityPresentationInstance &Instance);

    /** Resolves an exact public handle-generation pair to its live component, or null when stale or unregistered. */
    UMythicEntityPresentationComponent *ResolvePresentationComponent(
        const FMythicEntityPresentationInstance &Instance) const;

    /**
     * Copies the currently live registered components for bounded attention evaluation without an actor/world scan.
     * Invalid weak entries are omitted; callers can compare GetRegistryRevision before rebuilding cached candidates.
     */
    void GetRegisteredComponents(
        TArray<UMythicEntityPresentationComponent *> &OutComponents) const;

    /**
     * Resolves a public instance to its private canonical entity only on authority.
     * Returns false and clears OutEntityId on clients, invalid input, released instances, or stale generations.
     */
    bool ResolveAuthorityEntity(
        const FMythicEntityPresentationInstance &Instance,
        FMythicEntityId &OutEntityId) const;

    /**
     * Finds the current presentation instance for a private canonical entity only on authority.
     * Returns false and clears OutInstance when the entity has no allocated embodiment.
     */
    bool FindAuthorityInstance(
        const FMythicEntityId &EntityId,
        FMythicEntityPresentationInstance &OutInstance) const;

    /**
     * Invalidates every current presentation mapping during an authority load/reconcile barrier.
     * All subjects must bind again and receive fresh handles after this call; client calls are ignored.
     */
    void ResetAuthorityPresentationEpoch();

    /**
     * Invalidates authority mappings for one canonical domain while preserving unrelated domains. Components should
     * be deactivated first so their replicated public snapshots are cleared before this final pending-map sweep.
     */
    void ResetAuthorityDomain(EMythicEntityDomain Domain);

    /** Removes dead weak component registrations and releases their authority instance mappings when applicable. */
    void PruneStaleRegistrations();

    /** Returns the number of currently registered live-or-pending-prune public presentation instances. */
    int32 GetRegisteredPresentationCount() const {
        return RegisteredComponents.Num();
    }

    /** Returns the monotonic local change revision for registration, removal, prune, and epoch-reset events. */
    uint64 GetRegistryRevision() const { return RegistryRevision; }

    /** Native push event emitted after a new public instance becomes resolvable in this world. */
    FMythicPresentationInstanceRegistered OnPresentationRegistered;

    /** Native push event emitted immediately before a public instance stops resolving in this world. */
    FMythicPresentationInstanceUnregistered OnPresentationUnregistered;

private:
    bool IsAuthorityContext() const;
    uint32 AllocateEmbodimentGeneration();
    FMythicPresentationHandle AllocatePresentationHandle();
    void RemovePublicRegistration(
        const FMythicEntityPresentationInstance &Instance,
        UMythicEntityPresentationComponent *ExpectedComponent,
        bool bRequireExpectedComponent);

    TMap<FMythicEntityPresentationInstance,
         TWeakObjectPtr<UMythicEntityPresentationComponent>>
        RegisteredComponents;

    // Private authority maps have no reflection and therefore cannot replicate or leak into Blueprint.
    TMap<FMythicEntityPresentationInstance, FMythicEntityId>
        AuthorityEntityByInstance;

    TMap<FMythicEntityId, FMythicEntityPresentationInstance>
        AuthorityInstanceByEntity;

    // Handles remain reserved for this subsystem lifetime so delayed work can never match a recycled nonce.
    TSet<FMythicPresentationHandle> IssuedHandles;

    uint32 NextEmbodimentGeneration = 1;
    uint32 PresentationEpochRevision = 1;
    uint64 RegistryRevision = 0;
};
