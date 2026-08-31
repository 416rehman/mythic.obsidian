#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "World/Entity/MythicEntityPresentationTypes.h"
#include "World/Entity/MythicEntityViewerKnowledgeTypes.h"

#include "MythicEntityViewerKnowledgeComponent.generated.h"

class UMythicEntityPresentationRegistry;
class UMythicEntityViewerKnowledgeComponent;

/** One short-lived, owner-only recognition binding for an exact public embodiment. */
USTRUCT()
struct MYTHIC_API FMythicReplicatedEntityRecognition : public FFastArraySerializerItem {
    GENERATED_BODY()

    // Public instance stays handle+generation exact so pooled actors and delayed packets cannot inherit recognition.
    UPROPERTY()
    FMythicEntityPresentationInstance Subject;

    // Canonical identity is owner-only and deliberately inaccessible to Blueprint or public subject replication.
    UPROPERTY()
    FMythicEntityId EntityId;

    // Safe learned projection copied from the durable authority dossier for allocation-free client lookup.
    UPROPERTY()
    FMythicEntityKnowledgeView Knowledge;

    // Native-only dossier revision used to reject stale refresh work without exposing canonical identity.
    UPROPERTY()
    uint32 KnowledgeRevision = 0;

    // Absolute synchronized server time; expired bindings fail queries even before the removal delta arrives.
    UPROPERTY()
    double ExpiryServerTimeSeconds = 0.0;

    // Authority-only LRU order. It is not reflected, serialized, saved, or sent to the owning client.
    uint64 AuthorityTouchSerial = 0;

    bool IsExpired(const double ServerTimeSeconds) const {
        return ExpiryServerTimeSeconds <= 0.0
               || ServerTimeSeconds >= ExpiryServerTimeSeconds;
    }
};

/** Owner-only delta container for bounded recognition bindings. */
USTRUCT()
struct MYTHIC_API FMythicReplicatedEntityRecognitionArray : public FFastArraySerializer {
    GENERATED_BODY()

private:
    UPROPERTY()
    TArray<FMythicReplicatedEntityRecognition> Items;

    TWeakObjectPtr<UMythicEntityViewerKnowledgeComponent> Owner;

public:
    bool NetDeltaSerialize(FNetDeltaSerializeInfo &DeltaParms) {
        return FFastArraySerializer::FastArrayDeltaSerialize<
            FMythicReplicatedEntityRecognition,
            FMythicReplicatedEntityRecognitionArray>(Items, DeltaParms, *this);
    }

    void PreReplicatedRemove(const TArrayView<int32> &RemovedIndices, int32 FinalSize);
    void PostReplicatedAdd(const TArrayView<int32> &AddedIndices, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32> &ChangedIndices, int32 FinalSize);
    void SetOwner(UMythicEntityViewerKnowledgeComponent *InOwner) { Owner = InOwner; }

    friend class UMythicEntityViewerKnowledgeComponent;
};

template <>
struct TStructOpsTypeTraits<FMythicReplicatedEntityRecognitionArray>
    : TStructOpsTypeTraitsBase2<FMythicReplicatedEntityRecognitionArray> {
    enum { WithNetDeltaSerializer = true };
};

/** Native invalidation edge for allocation-stable local presentation directors. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMythicEntityViewerKnowledgeRevision, uint32);

/** Blueprint invalidation edge fired after owner-only knowledge is safe to query again. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMythicEntityViewerKnowledgeChanged,
                                            int32, LocalRevision);

/**
 * Player-owned recognition and learned-knowledge boundary for contextual entity presentation.
 *
 * Authority keeps durable dossiers keyed by FMythicEntityId. Only a bounded, expiring handle+generation binding and
 * its learned DTO replicate to the owning connection. Public actors, other players, and Blueprint never receive the
 * canonical identity or raw LivingWorld truth.
 */
UCLASS(BlueprintType, ClassGroup = (Mythic))
class MYTHIC_API UMythicEntityViewerKnowledgeComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicEntityViewerKnowledgeComponent();

    /** Fired locally after recognition or learned presentation data changes; query functions are then coherent. */
    UPROPERTY(BlueprintAssignable, Category = "Mythic|Entity Knowledge")
    FMythicEntityViewerKnowledgeChanged OnViewerKnowledgeChanged;

    /**
     * Copies the learned, viewer-safe knowledge for one exact active embodiment; stale, expired, or unknown subjects
     * return false and reset OutKnowledge. This performs no RPC and never returns a canonical entity identifier.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Entity Knowledge")
    bool GetKnowledgeForSubject(FMythicEntityPresentationInstance Subject,
                                FMythicEntityKnowledgeView &OutKnowledge) const;

    /** Returns true only while authority has granted recognition for this exact handle+generation embodiment. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Entity Knowledge")
    bool IsSubjectRecognized(FMythicEntityPresentationInstance Subject) const;

    /** Returns the number of nonexpired owner-only recognition bindings currently available to this local player. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Entity Knowledge")
    int32 GetActiveRecognitionCount() const;

    /** Returns the local UI invalidation counter in 1..2147483647; it wraps to 1 when required. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Entity Knowledge")
    int32 GetLocalKnowledgeRevision() const { return static_cast<int32>(LocalRevision); }

    /**
     * Native-only resolution of an entitled recognition binding. It never exposes canonical identity to Blueprint,
     * and authority additionally verifies the registry still maps this exact embodiment to the same entity.
     */
    bool ResolveRecognizedEntity(const FMythicEntityPresentationInstance &Subject,
                                 FMythicEntityId &OutEntityId) const;

    /**
     * Replaces one durable learned dossier on authority after fail-closed sanitization. Recognition remains a separate
     * explicit grant; raw simulation snapshots must never be passed into this API.
     */
    bool AuthorityReplaceLearnedDossier(const FMythicEntityId &EntityId,
                                        const FMythicEntityKnowledgeView &LearnedKnowledge);

    /**
     * Additively merges newly earned player knowledge on authority. Unknown scalar fields do not erase prior learning,
     * and semantic fact containers are unioned, categorized, deduplicated, and bounded.
     */
    bool AuthorityMergeLearnedKnowledge(const FMythicEntityId &EntityId,
                                        const FMythicEntityKnowledgeView &LearnedDelta);

    /**
     * Grants or refreshes recognition for an exact live embodiment on authority. The registry mapping must match the
     * typed entity ID and an existing durable dossier; lease seconds are clamped to a bounded positive duration.
     */
    bool AuthorityGrantRecognition(FMythicEntityPresentationInstance Subject,
                                    const FMythicEntityId &EntityId,
                                    float LeaseSeconds = 0.0f);

    /** Learns a safe delta and grants the matching embodiment in one authority transaction. */
    bool AuthorityLearnAndGrantRecognition(FMythicEntityPresentationInstance Subject,
                                            const FMythicEntityId &EntityId,
                                            const FMythicEntityKnowledgeView &LearnedDelta,
                                            float LeaseSeconds = 0.0f);

    /** Revokes recognition for one exact embodiment on authority; stale or missing subjects are harmless no-ops. */
    bool AuthorityRevokeRecognition(FMythicEntityPresentationInstance Subject);

    /** Revokes all current embodiment bindings for one typed entity while preserving its durable learned dossier. */
    int32 AuthorityRevokeRecognitionForEntity(const FMythicEntityId &EntityId);

    /** Clears every ephemeral recognition binding on authority, such as during a travel or persistence load barrier. */
    int32 AuthorityClearRecognitionBindings();

    /** Removes expired authority bindings using synchronized server-world seconds; invalid times are rejected. */
    int32 AuthorityPruneExpiredRecognition(double ServerTimeSeconds);

    /** Restores a bounded, typed-ID dossier snapshot on authority without importing legacy hash or string identities. */
    bool AuthorityRestoreLearnedDossiers(const TArray<FMythicEntityLearnedDossier> &SavedDossiers);

    /** Returns the native save snapshot; canonical IDs remain opaque and this function is never Blueprint-exposed. */
    const TArray<FMythicEntityLearnedDossier> &GetLearnedDossiersForSave() const {
        return LearnedDossiers;
    }

    /** Returns the native number of durable typed-ID dossiers retained by authority. */
    int32 GetLearnedDossierCount() const { return LearnedDossiers.Num(); }

    /** Returns true when this player's durable knowledge owns the exact private canonical identity. */
    bool HasLearnedDossierForEntity(const FMythicEntityId &EntityId) const {
        return FindDossierIndex(EntityId) != INDEX_NONE;
    }

    FOnMythicEntityViewerKnowledgeRevision &OnNativeKnowledgeRevision() {
        return NativeRevisionDelegate;
    }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

private:
    static constexpr int32 AbsoluteMaximumRecognitionBindings = 128;
    static constexpr int32 AbsoluteMaximumLearnedDossiers = 8192;

    /** Maximum simultaneous owner-only embodiment bindings; overflow evicts the least recently refreshed binding. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Entity Knowledge",
              meta = (ClampMin = "8", ClampMax = "128"))
    int32 MaxRecognitionBindings = 64;

    /** Default positive recognition lease used when authority callers pass zero seconds. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Entity Knowledge",
              meta = (ClampMin = "1.0", ClampMax = "300.0", Units = "s"))
    float DefaultRecognitionLeaseSeconds = 45.0f;

    /** Hard upper bound for a recognition lease; long-lived knowledge remains in the durable dossier instead. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Entity Knowledge",
              meta = (ClampMin = "1.0", ClampMax = "300.0", Units = "s"))
    float MaximumRecognitionLeaseSeconds = 120.0f;

    /** Maximum persistent learned dossiers retained per character; reaching it rejects new identities without eviction. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Entity Knowledge",
              meta = (ClampMin = "64", ClampMax = "8192"))
    int32 MaxLearnedDossiers = 4096;

    /** Save-backed authority knowledge; never replicated wholesale and never keyed by name hash, object name, or string. */
    UPROPERTY(SaveGame)
    TArray<FMythicEntityLearnedDossier> LearnedDossiers;

    /** Delta-compressed recognition projections sent only to the owning connection. */
    UPROPERTY(Replicated)
    FMythicReplicatedEntityRecognitionArray RecognitionBindings;

    uint32 LocalRevision = 0;
    uint64 NextBindingTouchSerial = 1;
    bool bReplicatedRevisionQueued = false;
    FOnMythicEntityViewerKnowledgeRevision NativeRevisionDelegate;
    FTimerHandle ExpiryTimerHandle;
    FDelegateHandle PresentationUnregisteredHandle;

    bool IsAuthority() const;
    double GetSynchronizedServerTimeSeconds() const;
    int32 FindBindingIndex(const FMythicEntityPresentationInstance &Subject) const;
    int32 FindDossierIndex(const FMythicEntityId &EntityId) const;
    uint32 AdvanceKnowledgeRevision(uint32 CurrentRevision) const;
    uint64 AllocateBindingTouchSerial();
    bool ValidateAuthoritySubjectBinding(const FMythicEntityPresentationInstance &Subject,
                                         const FMythicEntityId &EntityId) const;
    bool RefreshProjectedBindings(const FMythicEntityId &EntityId,
                                  const FMythicEntityLearnedDossier &Dossier);
    int32 RemoveBindingsByPredicate(
        TFunctionRef<bool(const FMythicReplicatedEntityRecognition &)> Predicate);
    void HandlePresentationUnregistered(
        const FMythicEntityPresentationInstance &Subject,
        UMythicEntityPresentationComponent *PresentationComponent);
    void PublishRevision();
    void QueueReplicatedRevision();
    void PublishQueuedReplicatedRevision();
    void ScheduleAuthorityExpiryTimer();
    void HandleAuthorityExpiryTimer();

    friend struct FMythicReplicatedEntityRecognitionArray;
};
