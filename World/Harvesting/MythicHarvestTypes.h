#pragma once

#include "CoreMinimal.h"
#include "Engine/HitResult.h"
#include "GameplayAbilitySpecHandle.h"
#include "InstanceDataTypes.h"
#include "MythicHarvestTypes.generated.h"

class AController;
class APawn;
class UAttackFragment;
class UMythicHarvestableDefinition;
class UMythicHarvestToolTypeDefinition;
class UMythicResourceISM;
class UMythicWeaponAttackAbility;

/** Result category produced by the authoritative harvesting transaction. */
UENUM(BlueprintType)
enum class EMythicHarvestOutcome : uint8 {
    Rejected,
    Accepted,
    Completed,
};

/** Exhaustive, presentation-safe reason for an authoritative harvest rejection. */
UENUM(BlueprintType)
enum class EMythicHarvestRejectReason : uint8 {
    None,
    WorldNotReady,
    NoTool,
    WrongTool,
    ToolTierTooLow,
    ToolBroken,
    NodeDepleted,
    ClaimedByOther,
    OutOfRange,
    NoLineOfSight,
    InvalidInstance,
    InvalidSource,
    GenerationMismatch,
    CadenceRejected,
};

/** Authoritative lifecycle state for one stable harvest node generation. */
UENUM(BlueprintType)
enum class EMythicHarvestNodeState : uint8 {
    Available,
    Depleted,
    Regrowing,
};

/**
 * Bounded, presentation-only harvest response delivered to the owning client after authority has accepted or
 * rejected one exact contact. It contains direct definition references and versioned numeric state, never an asset
 * path, gameplay tag, localized-name lookup key, or client-authoritative mutation input.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicHarvestClientFeedback {
    GENERATED_BODY()

    /**
     * Authority owns this committed/rejected result category; Blueprint may select presentation without gameplay
     * side effects, malformed delivery defaults to Rejected, and the enum is unitless.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Harvest|Feedback")
    EMythicHarvestOutcome Outcome = EMythicHarvestOutcome::Rejected;

    /**
     * Authority owns this exhaustive rejection classification; Blueprint may render it without mutation, None is
     * valid only for accepted/completed feedback, and the enum is unitless.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Harvest|Feedback")
    EMythicHarvestRejectReason RejectReason = EMythicHarvestRejectReason::WorldNotReady;

    /**
     * Authority resolves this exact resource definition from the registered node; Blueprint may inspect it without
     * side effects, null denotes malformed/unknown contact and must fail presentation closed, and it has no units.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Harvest|Feedback")
    TObjectPtr<UMythicHarvestableDefinition> HarvestableDefinition = nullptr;

    /**
     * Authority copies this exact required family from the node definition; Blueprint may render its authored
     * presentation only, null is invalid for the launch exact-tool policy and fails closed, and it has no units.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Harvest|Feedback")
    TObjectPtr<UMythicHarvestToolTypeDefinition> RequiredToolType = nullptr;

    /**
     * Authority copies this minimum from the node definition; Blueprint may display it without mutation, negative
     * or unpaired values are invalid content, and units are discrete integer tool tiers.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Harvest|Feedback")
    int32 RequiredToolTier = 0;

    /**
     * Authority resolves this contact/fallback anchor; Blueprint may position feedback without gameplay side
     * effects, malformed nonfinite contacts fall back to the node/origin, and units are network-quantized centimeters.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Harvest|Feedback")
    FVector_NetQuantize Location = FVector::ZeroVector;

    /**
     * Authority encodes the post-result remaining-work fraction; Blueprint may draw progress without mutating work,
     * values outside [0,65535] are malformed and fail closed, and units are uint16 normalized code points.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Harvest|Feedback")
    int32 QuantizedRemainingWork = 0;

    /**
     * Authority owns this node lifecycle generation; Blueprint may suppress older presentation without side effects,
     * negative values are invalid after transport conversion, and units are monotonic generation steps.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Harvest|Feedback")
    int64 Generation = 0;

    /**
     * Authority owns this revision within Generation; Blueprint may suppress reordered presentation without
     * mutation, negative values are invalid after transport conversion, and units are monotonic revision steps.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Harvest|Feedback")
    int64 Revision = 0;

    /**
     * Authority-owned monotonic transaction sequence copied into owner feedback; Blueprint may use it only to discard
     * reordered presentation, reading is side-effect free, zero means no committed sequence, and the value is unitless.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Harvest|Feedback")
    int64 ServerSequence = 0;
};

/**
 * Opaque durable identity for one harvest node. Only identity providers may construct it from an FGuid; runtime
 * instance indices, transforms, object paths, tags, names, and strings are deliberately unsupported.
 */
USTRUCT()
struct MYTHIC_API FMythicHarvestNodeId {
    GENERATED_BODY()

public:
    FMythicHarvestNodeId() = default;
    explicit FMythicHarvestNodeId(const FGuid &InValue) : Value(InValue) {}

    bool IsValid() const { return Value.IsValid(); }
    const FGuid &GetGuid() const { return Value; }

    bool operator==(const FMythicHarvestNodeId &Other) const { return Value == Other.Value; }
    bool operator!=(const FMythicHarvestNodeId &Other) const { return !(*this == Other); }

    friend uint32 GetTypeHash(const FMythicHarvestNodeId &NodeId) { return GetTypeHash(NodeId.Value); }

private:
    UPROPERTY()
    FGuid Value;
};

/**
 * Non-negative deterministic work amount. Authoring remains in continuous float work units, while authoritative
 * accumulation uses exactly 10,000 quanta per work unit so fractional hits never drift through repeated addition.
 */
USTRUCT()
struct MYTHIC_API FMythicHarvestWork {
    GENERATED_BODY()

public:
    static constexpr int64 QuantaPerWorkUnit = 10000;

    static bool TryFromWorkUnits(double WorkUnits, FMythicHarvestWork &OutWork);
    static FMythicHarvestWork FromQuanta(int64 InQuanta);

    double ToWorkUnits() const;
    int64 GetQuanta() const { return Quanta; }
    bool IsZero() const { return Quanta == 0; }

    FMythicHarvestWork SubtractClamped(FMythicHarvestWork Amount) const;
    static FMythicHarvestWork Min(FMythicHarvestWork A, FMythicHarvestWork B);

    bool operator==(const FMythicHarvestWork &Other) const { return Quanta == Other.Quanta; }
    bool operator!=(const FMythicHarvestWork &Other) const { return !(*this == Other); }
    bool operator<(const FMythicHarvestWork &Other) const { return Quanta < Other.Quanta; }
    bool operator<=(const FMythicHarvestWork &Other) const { return Quanta <= Other.Quanta; }

private:
    UPROPERTY()
    int64 Quanta = 0;
};

/** Pure deterministic contribution math shared by authority commits and automation; it owns no gameplay state. */
struct MYTHIC_API FMythicHarvestContributionMath {
    /**
     * Returns one contributor's proportional share of a finite non-negative pool from exact applied-work quanta.
     * Invalid, zero, or over-total inputs fail closed to zero; the result uses the same units as TotalPool.
     */
    static double CalculateProportionalShare(int64 ContributorQuanta,
                                             int64 TotalEligibleQuanta,
                                             double TotalPool);
};

/** Pure streaming-retention policy for authority runtime rows; it owns no provider or lifecycle state. */
struct MYTHIC_API FMythicHarvestStreamingPolicy {
    /**
     * Returns false for every untouched Available node, whose state and next generation are completely implicit.
     * Partially worked nodes remain resident across World Partition provider lifetimes so streaming cannot reset work,
     * contribution ownership, or per-work progression. Durable reward high-water prevents generation aliasing.
     */
    static bool ShouldRetainDetachedNode(EMythicHarvestNodeState State,
                                         uint32 Generation,
                                         bool bHasPartialWork);
};

/** Pure server-time cadence policy for opaque attack-cycle provenance; it owns no ability or montage state. */
struct MYTHIC_API FMythicHarvestCadencePolicy {
    /** Builds a finite expiry from montage seconds, captured play rate, and designer-owned tolerance. */
    static bool TryCalculateExpiry(double IssuedServerTime,
                                   double MaximumMontageSeconds,
                                   double CapturedPlayRate,
                                   double ToleranceSeconds,
                                   double &OutExpiresServerTime);

    /** Invalid/nonfinite times fail closed as expired; equality remains valid through the configured boundary. */
    static bool IsExpired(double ServerNow, double ExpiresServerTime);
};

/**
 * One-shot native barrier between authoritative state/replication commit and reward, progression, quest, pressure,
 * or feedback dispatch. Preparation cannot open it, and a recursive/replayed dispatch attempt cannot open it twice.
 */
struct MYTHIC_API FMythicHarvestPostCommitBarrier {
    void MarkStateCommitted() { bStateCommitted = true; }

    bool TryBeginSideEffects() {
        if (!bStateCommitted || bSideEffectsStarted) {
            return false;
        }
        bSideEffectsStarted = true;
        return true;
    }

    bool IsStateCommitted() const { return bStateCommitted; }
    bool HaveSideEffectsStarted() const { return bSideEffectsStarted; }

private:
    bool bStateCommitted = false;
    bool bSideEffectsStarted = false;
};

/**
 * Server-issued identity for one committed attack activation. It is native-only: clients and Blueprint cannot mint
 * provenance, and a zero serial, invalid ability handle, or missing physical-item GUID is always invalid.
 */
struct MYTHIC_API FMythicHarvestAttackCycleToken {
    uint64 ServerSerial = 0;
    FGameplayAbilitySpecHandle AbilitySpecHandle;
    FGuid SourceItemGuid;

    bool IsValid() const { return ServerSerial != 0 && AbilitySpecHandle.IsValid() && SourceItemGuid.IsValid(); }

    bool operator==(const FMythicHarvestAttackCycleToken &Other) const {
        return ServerSerial == Other.ServerSerial && AbilitySpecHandle == Other.AbilitySpecHandle && SourceItemGuid == Other.SourceItemGuid;
    }

    friend uint32 GetTypeHash(const FMythicHarvestAttackCycleToken &Token) {
        uint32 Hash = GetTypeHash(Token.ServerSerial);
        Hash = HashCombineFast(Hash, GetTypeHash(Token.AbilitySpecHandle));
        return HashCombineFast(Hash, GetTypeHash(Token.SourceItemGuid));
    }
};

/**
 * Native-only request accepted by the harvest authority subsystem. It carries contact and exact live attack
 * provenance, but intentionally carries no caller-selected work, tool family, tier, reward, or source-item pointer.
 */
struct MYTHIC_API FMythicHarvestRequest {
    APawn *AuthorityAvatar = nullptr;
    AController *AuthorityController = nullptr;
    UAttackFragment *SourceAttackFragment = nullptr;
    UMythicWeaponAttackAbility *ActiveAttackAbility = nullptr;
    FMythicHarvestAttackCycleToken AttackCycleToken;
    UMythicResourceISM *TargetResource = nullptr;
    FPrimitiveInstanceId RuntimeInstanceId;
    uint32 ExpectedGeneration = 0;
    FHitResult AuthoritativeHit;
};

/** Immutable native result returned by one authoritative harvesting attempt. */
struct MYTHIC_API FMythicHarvestResult {
    EMythicHarvestOutcome Outcome = EMythicHarvestOutcome::Rejected;
    EMythicHarvestRejectReason RejectReason = EMythicHarvestRejectReason::WorldNotReady;
    FMythicHarvestWork AppliedWork;
    FMythicHarvestWork RemainingWork;
    FMythicHarvestWork MaxWork;
    FMythicHarvestNodeId NodeId;
    uint32 Generation = 0;
    uint32 Revision = 0;
    uint64 ServerSequence = 0;
    bool WasAccepted() const { return Outcome == EMythicHarvestOutcome::Accepted || Outcome == EMythicHarvestOutcome::Completed; }
};
