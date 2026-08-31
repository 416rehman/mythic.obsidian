#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Interaction/ContextActions/MythicContextActionProvider.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "World/Entity/MythicEntityPresentationTypes.h"
#include "MythicEntityActionGrantComponent.generated.h"

class UMythicEntityActionGrantComponent;
class UMythicEntityPresentationComponent;
class UMythicEntityPresentationRegistry;
class AActor;

/** Replicated viewer-safe action state; Hidden remains an authority-only provider result and has no wire value. */
UENUM(BlueprintType)
enum class EMythicContextActionGrantState : uint8 {
    Available UMETA(DisplayName = "Available"),
    UnavailableWithReason UMETA(DisplayName = "Unavailable with Reason")
};

/** One owner-only, embodiment-scoped action grant projected from authority domain rules. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicReplicatedContextActionGrant : public FFastArraySerializerItem {
    GENERATED_BODY()

    /** Opaque public subject instance; handle or generation changes make this grant stale and unusable. */
    UPROPERTY(BlueprintReadOnly, Category = "Context Action Grant")
    FMythicEntityPresentationInstance Subject;

    /** Canonical Context.Action.* identity resolved to a local Primary Data Asset by the owning client. */
    UPROPERTY(BlueprintReadOnly, Category = "Context Action Grant", meta = (Categories = "Context.Action"))
    FGameplayTag ActionTag;

    /** Available or safely explainable unavailable state; Hidden is never representable in this replicated item. */
    UPROPERTY(BlueprintReadOnly, Category = "Context Action Grant")
    EMythicContextActionGrantState State = EMythicContextActionGrantState::UnavailableWithReason;

    /** Optional safe Context.Action.Reason.* explanation; invalid uses the action definition's generic fallback text. */
    UPROPERTY(BlueprintReadOnly, Category = "Context Action Grant",
              meta = (Categories = "Context.Action.Reason"))
    FGameplayTag UnavailableReasonTag;

    /**
     * Nonzero authority-minted lease nonce echoed by execution coordination. It is intentionally opaque to Blueprint;
     * the provider's private source revision remains only in the authority ledger and never crosses the wire.
     */
    UPROPERTY()
    uint32 OfferRevision = 0;

    /** Absolute synchronized server-world time in seconds when this lease expires; zero or less means no automatic expiry. */
    UPROPERTY(BlueprintReadOnly, Category = "Context Action Grant", meta = (Units = "s"))
    double ExpiryServerTimeSeconds = 0.0;

    bool Matches(const FMythicEntityPresentationInstance &InSubject, const FGameplayTag InActionTag) const {
        return Subject == InSubject && ActionTag == InActionTag;
    }

    bool IsExpired(const double ServerTimeSeconds) const {
        return ExpiryServerTimeSeconds > 0.0 && ServerTimeSeconds >= ExpiryServerTimeSeconds;
    }
};

/** Owner-only delta container; replication bookkeeping is transient and never enters save or public subject state. */
USTRUCT()
struct MYTHIC_API FMythicReplicatedContextActionGrantArray : public FFastArraySerializer {
    GENERATED_BODY()

private:
    UPROPERTY()
    TArray<FMythicReplicatedContextActionGrant> Items;

    TWeakObjectPtr<UMythicEntityActionGrantComponent> Owner;

public:
    bool NetDeltaSerialize(FNetDeltaSerializeInfo &DeltaParms) {
        return FFastArraySerializer::FastArrayDeltaSerialize<FMythicReplicatedContextActionGrant,
                                                              FMythicReplicatedContextActionGrantArray>(
            Items, DeltaParms, *this);
    }

    void PreReplicatedRemove(const TArrayView<int32> &RemovedIndices, int32 FinalSize);
    void PostReplicatedAdd(const TArrayView<int32> &AddedIndices, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32> &ChangedIndices, int32 FinalSize);
    void SetOwner(UMythicEntityActionGrantComponent *InOwner) { Owner = InOwner; }

    const TArray<FMythicReplicatedContextActionGrant> &GetItems() const { return Items; }

    friend class UMythicEntityActionGrantComponent;
};

template <>
struct TStructOpsTypeTraits<FMythicReplicatedContextActionGrantArray>
    : TStructOpsTypeTraitsBase2<FMythicReplicatedContextActionGrantArray> {
    enum { WithNetDeltaSerializer = true };
};

/**
 * One authority-only provider binding gathered for an exact subject. This native staging row is never reflected,
 * replicated, persisted, or exposed to UI; the grant component converts it into an opaque nonce-backed lease.
 */
struct MYTHIC_API FMythicAuthorityContextActionOffer {
    TWeakObjectPtr<UObject> Provider;
    FMythicContextActionOffer Offer;

    FMythicAuthorityContextActionOffer() = default;

    FMythicAuthorityContextActionOffer(UObject *InProvider,
                                       const FMythicContextActionOffer &InOffer)
        : Provider(InProvider), Offer(InOffer) {}
};

/**
 * Authority-only immutable snapshot of every definition field used by grant selection or execution validation.
 * This native value is the single signature implementation shared by the ledger and execution callback boundary;
 * it is never reflected, replicated, serialized, or used as an asset identity.
 */
struct MYTHIC_API FMythicAuthorityContextActionDefinitionSignature {
    static FMythicAuthorityContextActionDefinitionSignature Capture(
        const UMythicContextActionDefinition &Definition);

    bool Matches(const UMythicContextActionDefinition &Definition) const;
    bool operator==(
        const FMythicAuthorityContextActionDefinitionSignature &Other) const;

private:
    FGameplayTag ActionTag;
    int32 PresentationPriority = 0;
    float HoldDurationSeconds = 0.0f;
    float MaximumFocusAngleDegrees = 0.0f;
    float MaximumRangeCentimeters = 0.0f;
    EMythicContextActionPresentationSemantic PresentationSemantic =
        EMythicContextActionPresentationSemantic::Other;
    EMythicContextActionWorldPresentationPolicy WorldPresentationPolicy =
        EMythicContextActionWorldPresentationPolicy::FocusOnly;
    EMythicContextActionFocusPolicy FocusPolicy =
        EMythicContextActionFocusPolicy::NotRequired;
    EMythicContextActionRangePolicy RangePolicy =
        EMythicContextActionRangePolicy::NotRequired;
    EMythicContextActionLineOfSightPolicy LineOfSightPolicy =
        EMythicContextActionLineOfSightPolicy::NotRequired;
    bool bExplainWhenUnavailable = false;
};

/** Native revision edge used by C++ nameplate directors on clients and by listen-host authority. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMythicEntityActionGrantRevision, uint32);

/** Blueprint revision edge fired after the local owner-only action grant view is safe to query. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMythicEntityActionGrantsChanged, int32, LocalRevision);

/**
 * Owner-only transport for viewer-relative entity actions.
 *
 * Add this as a replicated default subobject of the owning PlayerState. Authority writers project quest, dialogue,
 * service, revive, and other private decisions here; subjects never receive a global private-action flag.
 */
UCLASS(BlueprintType, ClassGroup = (Mythic))
class MYTHIC_API UMythicEntityActionGrantComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicEntityActionGrantComponent();

    /** Native hard ceiling for every owner-only grant row held by one PlayerState component. */
    static constexpr int32 MaximumReplicatedGrants = 128;

    /** Native hard ceiling shared by projection policy and transport for one exact subject embodiment. */
    static constexpr int32 MaximumReplicatedGrantsPerSubject = 16;

    /** Fired locally when grants change; the owning client and listen-host authority receive the same monotonic revision edge. */
    UPROPERTY(BlueprintAssignable, Category = "Mythic|Context Actions")
    FMythicEntityActionGrantsChanged OnActionGrantsChanged;

    /**
     * Returns the nonexpired owner-only grants for one exact public embodiment; invalid subjects return an empty array.
     * This performs no RPC and uses synchronized server-world seconds when a world is available.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Context Actions")
    TArray<FMythicReplicatedContextActionGrant> GetActionGrantsForSubject(
        FMythicEntityPresentationInstance Subject) const;

    /** Allocation-stable C++ query used by directors that retain OutGrants capacity between 10 Hz projection passes. */
    void GatherCurrentActionGrantsForSubject(const FMythicEntityPresentationInstance &Subject,
                                             TArray<FMythicReplicatedContextActionGrant> &OutGrants) const;

    /**
     * Finds one nonexpired owner-only grant for an exact subject and Context.Action.* tag; invalid or stale keys return false.
     * This performs no RPC, and OutGrant is reset when no current grant exists.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Context Actions")
    bool FindActionGrant(FMythicEntityPresentationInstance Subject, FGameplayTag ActionTag,
                         FMythicReplicatedContextActionGrant &OutGrant) const;

    /** Returns the local UI invalidation counter in 1..2147483647; it wraps to 1 and resets when the component is recreated. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Context Actions")
    int32 GetLocalGrantRevision() const { return static_cast<int32>(LocalRevision); }

    /**
     * Atomically replaces leases for one exact embodiment from authority-only provider-bound offers. Every accepted
     * action receives a nonzero opaque nonce backed by the exact provider, definition, source revision, and definition
     * signature. Duplicate same-tag rows fail closed regardless of provider order. Hidden or malformed rows are omitted.
     */
    bool AuthorityReplaceBoundContextActionOffers(
        FMythicEntityPresentationInstance Subject, AActor *SubjectActor,
        const TArray<FMythicAuthorityContextActionOffer> &Offers,
        int32 MaximumOffers, double ExpiryServerTimeSeconds = 0.0);

    /**
     * Resolves one exact current nonce to its authority-only issuing provider and definition without consuming it.
     * Stale subject instances, changed definitions, dead providers, expired rows, and unbound transport data fail closed.
     */
    bool AuthorityResolveActionGrantBinding(
        FMythicEntityPresentationInstance Subject, FGameplayTag ActionTag,
        uint32 GrantNonce, UObject *&OutProvider,
        UMythicContextActionDefinition *&OutDefinition,
        uint32 &OutProviderSourceRevision);

    /**
     * Atomically removes one exact replicated lease and its authority ledger entry, returning the retained issuer data.
     * A second consume of the same subject/tag/nonce always fails, including provider re-entry during domain execution.
     */
    bool AuthorityConsumeActionGrantBinding(
        FMythicEntityPresentationInstance Subject, FGameplayTag ActionTag,
        uint32 GrantNonce, UObject *&OutProvider,
        UMythicContextActionDefinition *&OutDefinition,
        uint32 &OutProviderSourceRevision);

    /** Invalidates one exact owner-only subject/action grant on authority; clients or missing keys return false without mutation. */
    bool AuthorityRevokeActionGrant(FMythicEntityPresentationInstance Subject, FGameplayTag ActionTag);

    /** Invalidates every owner-only action grant for one exact embodiment on authority; invalid subjects and clients are no-ops. */
    int32 AuthorityRevokeSubjectGrants(FMythicEntityPresentationInstance Subject);

    /** Clears every owner-only action grant on authority, such as during travel/load barriers; clients are no-ops. */
    int32 AuthorityRevokeAllActionGrants();

    /** Removes expired leases on authority using absolute synchronized server-world seconds; invalid/nonfinite time is rejected. */
    int32 AuthorityPruneExpiredActionGrants(double ServerTimeSeconds);

    FOnMythicEntityActionGrantRevision &OnNativeGrantRevision() { return NativeRevisionDelegate; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

private:
    /** Delta-compressed grants sent only to the owning connection; canonical entity IDs and Hidden results are forbidden. */
    UPROPERTY(Replicated)
    FMythicReplicatedContextActionGrantArray ReplicatedGrants;

    uint32 LocalRevision = 0;
    bool bReplicatedRevisionQueued = false;
    FOnMythicEntityActionGrantRevision NativeRevisionDelegate;
    FTimerHandle ExpiryTimerHandle;

    struct FAuthorityGrantLedgerKey {
        FMythicEntityPresentationInstance Subject;
        FGameplayTag ActionTag;
        uint32 GrantNonce = 0;

        bool operator==(const FAuthorityGrantLedgerKey &Other) const {
            return Subject == Other.Subject && ActionTag == Other.ActionTag
                   && GrantNonce == Other.GrantNonce;
        }

        friend uint32 GetTypeHash(const FAuthorityGrantLedgerKey &Key) {
            return HashCombineFast(
                HashCombineFast(::GetTypeHash(Key.Subject),
                                GetTypeHash(Key.ActionTag.GetTagName())),
                ::GetTypeHash(Key.GrantNonce));
        }
    };

    struct FAuthorityGrantLedgerEntry {
        TWeakObjectPtr<AActor> SubjectActor;
        TWeakObjectPtr<UObject> Provider;
        TWeakObjectPtr<UMythicContextActionDefinition> Definition;
        FMythicAuthorityContextActionDefinitionSignature DefinitionSignature;
        uint32 ProviderSourceRevision = 0;
        double ExpiryServerTimeSeconds = 0.0;
    };

    TMap<FAuthorityGrantLedgerKey, FAuthorityGrantLedgerEntry>
        AuthorityGrantLedger;
    uint64 NextAuthorityGrantNonce = 1;

    TWeakObjectPtr<UMythicEntityPresentationRegistry>
        BoundPresentationRegistry;
    FDelegateHandle PresentationUnregisteredHandle;

    bool IsAuthority() const;
    double GetSynchronizedServerTimeSeconds() const;
    uint32 AllocateAuthorityGrantNonce();
    static bool DoesProviderBelongToSubject(UObject *Provider,
                                            AActor *SubjectActor);
    bool ResolveAuthorityGrantBindingInternal(
        const FMythicEntityPresentationInstance &Subject,
        FGameplayTag ActionTag, uint32 GrantNonce, bool bConsume,
        UObject *&OutProvider,
        UMythicContextActionDefinition *&OutDefinition,
        uint32 &OutProviderSourceRevision);
    bool SetGrantInternal(const FMythicReplicatedContextActionGrant &NewGrant);
    int32 RemoveGrantsByPredicate(TFunctionRef<bool(const FMythicReplicatedContextActionGrant &)> Predicate);
    int32 RemoveAuthorityLedgerEntriesByPredicate(
        TFunctionRef<bool(const FAuthorityGrantLedgerKey &,
                          const FAuthorityGrantLedgerEntry &)> Predicate);
    void PublishRevision();
    void QueueReplicatedRevision();
    void PublishQueuedReplicatedRevision();
    void ScheduleAuthorityExpiryTimer();
    void HandleAuthorityExpiryTimer();
    void EnsurePresentationRegistryBinding();
    void RemovePresentationRegistryBinding();
    void HandlePresentationUnregistered(
        const FMythicEntityPresentationInstance &Subject,
        UMythicEntityPresentationComponent *Presentation);
    static bool IsSafeActionTag(FGameplayTag Tag);
    static FGameplayTag SanitizeReasonTag(FGameplayTag Tag);

    friend struct FMythicReplicatedContextActionGrantArray;
};
