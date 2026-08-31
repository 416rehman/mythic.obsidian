#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "GAS/Combat/MythicCombatThreatAssessment.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "World/Entity/MythicEntityPresentationTypes.h"

#include "MythicEntityCombatPresentationComponent.generated.h"

class UMythicEntityCombatPresentationComponent;
class UMythicEntityPresentationComponent;
class UMythicEntityPresentationRegistry;

/**
 * Viewer-safe categorical combat rank carried by the owner-private presentation channel.
 *
 * Unknown is the mandatory fail-closed value when authority has not permitted rank presentation. WorldBoss is
 * reserved for an explicit encounter-authority source and is never inferred from an NPC's ordinary AI tier.
 */
UENUM(BlueprintType)
enum class EMythicPresentedCombatRank : uint8 {
    /** Rank is unavailable, withheld, or not yet safe to present to this viewer. */
    Unknown,

    /** Ordinary combatant. */
    Standard,

    /** Superior combatant above the ordinary tier. */
    Superior,

    /** Elite combatant. */
    Elite,

    /** Champion combatant below boss rank. */
    Champion,

    /** Boss combatant. */
    Boss,

    /** World-scale encounter boss supplied only by an explicit encounter-authority source. */
    WorldBoss,
};

/**
 * One ephemeral, owner-private combat read for an exact public entity embodiment.
 *
 * Raw levels, attributes, canonical rank sources, pressure, and canonical entity identity are deliberately absent.
 * The authority may include only a sanitized categorical rank and exact level after separately permitting each value.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicReplicatedEntityCombatPresentation : public FFastArraySerializerItem {
    GENERATED_BODY()

    /** Opaque public subject instance; a changed handle or generation makes this combat read stale. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Combat Presentation")
    FMythicEntityPresentationInstance Subject;

    /** Combat-owned categorical warning for this viewer; raw pressure values never cross this boundary. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Combat Presentation")
    EMythicThreatBand ThreatBand = EMythicThreatBand::Unknown;

    /** True only when authority policy permits this viewer to know that the subject is combat-capable. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Combat Presentation")
    bool bCombatCapable = false;

    /** Authority-sanitized player-facing rank; Unknown when rank presentation is not separately permitted. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Combat Presentation")
    EMythicPresentedCombatRank PresentedCombatRank = EMythicPresentedCombatRank::Unknown;

    /** Whether ExactCombatLevel contains a separately permissioned exact value. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Combat Presentation")
    bool bHasExactCombatLevel = false;

    /** Permissioned exact combat level; zero when bHasExactCombatLevel is false. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Combat Presentation", meta = (ClampMin = "0"))
    int32 ExactCombatLevel = 0;

    /** Opaque nonzero authority source revision used to reject out-of-order projection writes. */
    UPROPERTY()
    uint32 SourceRevision = 0;

    /** Absolute synchronized server-world deadline for this lease; zero means authority must explicitly revoke it. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Combat Presentation", meta = (Units = "s"))
    double ExpiryServerTimeSeconds = 0.0;

    bool IsExpired(double ServerTimeSeconds) const { return ExpiryServerTimeSeconds > 0.0 && ServerTimeSeconds >= ExpiryServerTimeSeconds; }

    bool HasSamePayload(const FMythicReplicatedEntityCombatPresentation &Other) const {
        return Subject == Other.Subject && ThreatBand == Other.ThreatBand && bCombatCapable == Other.bCombatCapable &&
            PresentedCombatRank == Other.PresentedCombatRank && bHasExactCombatLevel == Other.bHasExactCombatLevel &&
            ExactCombatLevel == Other.ExactCombatLevel && SourceRevision == Other.SourceRevision &&
            ExpiryServerTimeSeconds == Other.ExpiryServerTimeSeconds;
    }
};

/** Owner-only FastArray container; its replication bookkeeping is transient and never enters character saves. */
USTRUCT()
struct MYTHIC_API FMythicReplicatedEntityCombatPresentationArray : public FFastArraySerializer {
    GENERATED_BODY()

private:
    UPROPERTY()
    TArray<FMythicReplicatedEntityCombatPresentation> Items;

    TWeakObjectPtr<UMythicEntityCombatPresentationComponent> Owner;

public:
    bool NetDeltaSerialize(FNetDeltaSerializeInfo &DeltaParms) {
        return FFastArraySerializer::FastArrayDeltaSerialize<FMythicReplicatedEntityCombatPresentation, FMythicReplicatedEntityCombatPresentationArray>(
            Items, DeltaParms, *this);
    }

    void PreReplicatedRemove(const TArrayView<int32> &RemovedIndices, int32 FinalSize);
    void PostReplicatedAdd(const TArrayView<int32> &AddedIndices, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32> &ChangedIndices, int32 FinalSize);
    void SetOwner(UMythicEntityCombatPresentationComponent *InOwner) { Owner = InOwner; }

    friend class UMythicEntityCombatPresentationComponent;
};

template <>
struct TStructOpsTypeTraits<FMythicReplicatedEntityCombatPresentationArray> : TStructOpsTypeTraitsBase2<FMythicReplicatedEntityCombatPresentationArray> {
    enum { WithNetDeltaSerializer = true };
};

/**
 * Authority-only input for one viewer-relative combat projection.
 *
 * This native, non-reflected request is the only place raw assessment inputs, canonical presented rank, and exact level
 * coexist. Callers must set each disclosure permission only after authoritative knowledge and game-mode policy allow it.
 */
struct MYTHIC_API FMythicEntityCombatPresentationAuthorityRequest {
    /** Exact current public embodiment receiving this viewer-relative projection. */
    FMythicEntityPresentationInstance Subject;

    /** Private transient combat inputs; embedded rank fields are overwritten from canonical PresentedCombatRank. */
    FMythicCombatThreatAssessmentInputs AssessmentInputs;

    /** Canonical combat-owned player-facing rank before viewer-specific disclosure sanitization. */
    EMythicPresentedCombatRank PresentedCombatRank = EMythicPresentedCombatRank::Unknown;

    /** Explicit authority decision that this viewer may receive PresentedCombatRank and its combat-warning floor. */
    bool bRankPresentationPermitted = false;

    /** Explicit authority decision that this viewer may receive SubjectCombatLevel. */
    bool bExactCombatLevelPermitted = false;

    /** Raw subject combat level consumed only when its separate permission is true. */
    int32 SubjectCombatLevel = 0;

    /** Nonzero monotonic revision owned by the authority source for this exact subject. */
    uint32 SourceRevision = 0;

    /** Absolute synchronized server-world lease deadline; zero requires explicit revoke. */
    double ExpiryServerTimeSeconds = 0.0;
};

/** Native invalidation edge emitted after the local owner-private combat view is safe to query. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMythicEntityCombatPresentationRevision, uint32);

/**
 * Ephemeral owner-only transport for viewer-relative entity combat presentation.
 *
 * Install as a replicated PlayerState default subobject. Authority combat coordination classifies private inputs through
 * FMythicCombatThreatAssessment and writes only categorical/permissioned output. The owning local player's nameplate
 * director queries this component; subjects never replicate viewer-relative danger or exact levels to every observer.
 */
UCLASS(BlueprintType, ClassGroup = (Mythic))
class MYTHIC_API UMythicEntityCombatPresentationComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicEntityCombatPresentationComponent();

    /**
     * Copies the current nonexpired combat read for one exact public embodiment. This is local-only, performs no RPC,
     * and clears OutPresentation when the subject is invalid, stale, missing, or expired.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Combat Presentation")
    bool GetCombatPresentationForSubject(FMythicEntityPresentationInstance Subject, FMythicReplicatedEntityCombatPresentation &OutPresentation) const;

    /** Returns the number of nonexpired owner-private combat reads currently available to this local player. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Combat Presentation")
    int32 GetActiveCombatPresentationCount() const;

    /** Returns the monotonic local invalidation revision; zero means no local presentation mutation has occurred. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Combat Presentation")
    int32 GetLocalCombatPresentationRevision() const { return static_cast<int32>(LocalRevision); }

    /**
     * Allocation-free C++ lookup for a current exact embodiment. The returned pointer remains valid only until the next
     * native revision edge, authority mutation, or replication delta; callers must never retain it across those edges.
     */
    const FMythicReplicatedEntityCombatPresentation *FindCurrentCombatPresentation(const FMythicEntityPresentationInstance &Subject) const;

    /**
     * Classifies one authority-only request with UMythicCombatSettings thresholds and updates its owner-private lease.
     * Revision zero, stale/conflicting revisions, invalid subjects, invalid exact levels, and malformed leases fail.
     * An already-expired newer request safely revokes the matching subject.
     */
    bool AuthoritySetCombatPresentation(const FMythicEntityCombatPresentationAuthorityRequest &Request);

    /**
     * Atomically replaces the complete authority-owned projection set from a bounded, duplicate-free request view.
     * ReplacementRevision is a nonzero monotonic revision for the whole snapshot, protecting omitted subjects from
     * delayed older batches. Every request is classified before mutation; invalid or stale/conflicting data rejects all.
     */
    bool AuthorityReplaceCombatPresentations(uint32 ReplacementRevision, TArrayView<const FMythicEntityCombatPresentationAuthorityRequest> Requests);

    /** Revokes the owner-private combat read for one exact embodiment on authority; stale generations cannot alias. */
    bool AuthorityRevokeCombatPresentation(FMythicEntityPresentationInstance Subject);

    /** Clears every ephemeral combat read on authority, including during travel and presentation-epoch barriers. */
    int32 AuthorityRevokeAllCombatPresentations();

    /** Removes leases expired at the supplied synchronized server-world time; malformed or client calls are ignored. */
    int32 AuthorityPruneExpiredCombatPresentations(double ServerTimeSeconds);

    /** Returns the native local invalidation delegate used by allocation-stable C++ presentation consumers. */
    FOnMythicEntityCombatPresentationRevision &OnNativeCombatPresentationRevision() { return NativeRevisionDelegate; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

private:
    static constexpr int32 MaxReplicatedPresentations = 128;

    enum class ESetResult : uint8 {
        Rejected,
        Unchanged,
        Changed,
    };

    /** Delta-compressed ephemeral reads replicated only to the PlayerState's owning connection. */
    UPROPERTY(Transient, Replicated)
    FMythicReplicatedEntityCombatPresentationArray ReplicatedPresentations;

    uint32 LocalRevision = 0;
    uint32 AuthorityReplacementRevision = 0;
    bool bReplicatedRevisionQueued = false;
    FOnMythicEntityCombatPresentationRevision NativeRevisionDelegate;
    FTimerHandle ExpiryTimerHandle;
    TWeakObjectPtr<UMythicEntityPresentationRegistry> BoundPresentationRegistry;
    FDelegateHandle PresentationUnregisteredHandle;

    bool IsAuthority() const;
    double GetSynchronizedServerTimeSeconds() const;
    bool BuildSanitizedPresentation(const FMythicEntityCombatPresentationAuthorityRequest &Request,
                                    FMythicReplicatedEntityCombatPresentation &OutPresentation) const;
    ESetResult SetPresentationInternal(const FMythicReplicatedEntityCombatPresentation &NewPresentation);
    int32 RemovePresentationsByPredicate(TFunctionRef<bool(const FMythicReplicatedEntityCombatPresentation &)> Predicate);
    void PublishRevision();
    void QueueReplicatedRevision();
    void PublishQueuedReplicatedRevision();
    void ScheduleAuthorityExpiryTimer();
    void HandleAuthorityExpiryTimer();
    void EnsurePresentationRegistryBinding();
    void RemovePresentationRegistryBinding();
    void HandlePresentationUnregistered(
        const FMythicEntityPresentationInstance &Instance,
        UMythicEntityPresentationComponent *PresentationComponent);

    friend struct FMythicReplicatedEntityCombatPresentationArray;
};
