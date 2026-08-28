#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/Harvesting/MythicHarvestParticipantSnapshot.h"
#include "World/Harvesting/MythicHarvestReceiptTypes.h"
#include "World/Harvesting/MythicHarvestRewardPlanner.h"

#include "MythicHarvestRewardOutboxSubsystem.generated.h"

class AMythicPlayerState;
class UItemDefinition;
class UMythicItemFactorySubsystem;
class UMythicHarvestReceiptLedgerComponent;
class UMythicHarvestRewardEscrowComponent;
class UMythicHarvestableDefinition;
class UMythicItemInstance;
class UProficiencyDefinition;
struct FMythicDurableCharacterSaveResult;
struct FMythicHarvestItemEscrowSaveV1;
struct FMythicSavedHarvestItemEscrowRowV1;
struct FStreamableHandle;

/** Durable current-generation witness for one node; contiguous generation high-water covers all earlier completions. */
USTRUCT()
struct MYTHIC_API FMythicSavedHarvestRewardCompletionV1 {
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    FGuid WorldEpoch;

    UPROPERTY(SaveGame)
    FGuid NodeGuid;

    UPROPERTY(SaveGame)
    uint32 Generation = 0;
};

/** Persisted O(1) generation checkpoint for one node inside one world epoch. */
USTRUCT()
struct MYTHIC_API FMythicSavedHarvestGenerationHighWaterV1 {
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    FGuid WorldEpoch;

    UPROPERTY(SaveGame)
    FGuid NodeGuid;

    UPROPERTY(SaveGame)
    uint32 HighestKnownGeneration = 0;
};

/** Durable immutable item entitlement plus the quantity not yet acknowledged by an exact character save. */
USTRUCT()
struct MYTHIC_API FMythicSavedHarvestRewardGrantV1 {
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    FGuid WorldEpoch;

    UPROPERTY(SaveGame)
    FGuid NodeGuid;

    UPROPERTY(SaveGame)
    uint32 Generation = 0;

    UPROPERTY(SaveGame)
    EMythicHarvestRewardChannel Channel =
        EMythicHarvestRewardChannel::PrimaryMaterial;

    UPROPERTY(SaveGame)
    int32 RewardRowIndex = INDEX_NONE;

    /** Stable character routing identity; it is never part of the semantic receipt key or payload fingerprint. */
    UPROPERTY(SaveGame)
    FString ContributorKey;

    UPROPERTY(SaveGame)
    FPrimaryAssetId ItemDefinitionId;

    UPROPERTY(SaveGame)
    int32 OriginalQuantity = 0;

    /** Target units not represented by a successful exact character-save callback. */
    UPROPERTY(SaveGame)
    int32 RemainingQuantity = 0;

    UPROPERTY(SaveGame)
    int32 ItemLevel = 1;

    UPROPERTY(SaveGame)
    bool bHasResolvedQuality = false;

    UPROPERTY(SaveGame)
    EMythicYieldQuality ResolvedQuality = EMythicYieldQuality::Common;

    UPROPERTY(SaveGame)
    uint64 ItemSeed = 0;

    UPROPERTY(SaveGame)
    FMythicHarvestReceiptKey ReceiptKey;

    /** Conflict detector for the immutable item payload; it is never used to locate content. */
    UPROPERTY(SaveGame)
    FGuid ReceiptPayloadFingerprint;
};

/** Durable completion XP and typed quest-credit channels for one canonical contributor. */
USTRUCT()
struct MYTHIC_API FMythicSavedHarvestCompletionDeliveryV1 {
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    FGuid WorldEpoch;

    UPROPERTY(SaveGame)
    FGuid NodeGuid;

    UPROPERTY(SaveGame)
    uint32 Generation = 0;

    /** Stable character routing identity; entitlement authority remains in the typed receipt fields below. */
    UPROPERTY(SaveGame)
    FString ContributorKey;

    UPROPERTY(SaveGame)
    FPrimaryAssetId ProficiencyDefinitionId;

    UPROPERTY(SaveGame)
    FPrimaryAssetId HarvestableDefinitionId;

    /** Frozen completion XP in 1/10,000-XP quanta. */
    UPROPERTY(SaveGame)
    int64 CompletionProficiencyXPQuanta = 0;

    UPROPERTY(SaveGame)
    FGameplayTagContainer ProficiencyContextTags;

    UPROPERTY(SaveGame)
    int32 QuestCreditCount = 0;

    /** True only when the exact captured character snapshot durably contains the full XP receipt target. */
    UPROPERTY(SaveGame)
    bool bProficiencyDelivered = false;

    /** True only when the exact captured character snapshot durably contains terminal matched/no-match quest credit. */
    UPROPERTY(SaveGame)
    bool bQuestCreditDelivered = false;

    UPROPERTY(SaveGame)
    FMythicHarvestReceiptKey ProficiencyReceiptKey;

    UPROPERTY(SaveGame)
    FGuid ProficiencyReceiptPayloadFingerprint;

    UPROPERTY(SaveGame)
    FMythicHarvestReceiptKey QuestReceiptKey;

    UPROPERTY(SaveGame)
    FGuid QuestReceiptPayloadFingerprint;
};

/** Durable cumulative per-work proficiency delivery for one contributor and node lifecycle. */
USTRUCT()
struct MYTHIC_API FMythicSavedHarvestWorkDeliveryV1 {
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    FGuid WorldEpoch;

    UPROPERTY(SaveGame)
    FGuid NodeGuid;

    UPROPERTY(SaveGame)
    uint32 Generation = 0;

    /** Stable character routing identity; it is not semantic receipt authority. */
    UPROPERTY(SaveGame)
    FString ContributorKey;

    UPROPERTY(SaveGame)
    FMythicHarvestWorkRewardContract WorkRewardContract;

    /** Cumulative accepted contributor work in 1/10,000-work-unit quanta. */
    UPROPERTY(SaveGame)
    int64 CumulativeAppliedWorkQuanta = 0;

    /** Cumulative XP target for this contributor/node lifecycle series. */
    UPROPERTY(SaveGame)
    int64 ProficiencyXPQuanta = 0;

    UPROPERTY(SaveGame)
    FMythicHarvestReceiptKey ReceiptKey;

    UPROPERTY(SaveGame)
    FGuid ReceiptPayloadFingerprint;
};

/** World-owned cumulative durability-cost series for one contributor's exact physical tool. */
USTRUCT()
struct MYTHIC_API FMythicSavedHarvestDurabilityCostV1 {
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    FGuid WorldEpoch;

    UPROPERTY(SaveGame)
    FGuid NodeGuid;

    UPROPERTY(SaveGame)
    uint32 Generation = 0;

    UPROPERTY(SaveGame)
    FString ContributorKey;

    UPROPERTY(SaveGame)
    FGuid ToolItemInstanceGuid;

    /** Cumulative semantic wear target for this tool/node lifecycle series. */
    UPROPERTY(SaveGame)
    int64 CumulativeWearTarget = 0;

    /** Prefix proven present in an exact successful character snapshot. */
    UPROPERTY(SaveGame)
    int64 DurablyAppliedWearTarget = 0;

    UPROPERTY(SaveGame)
    FMythicHarvestReceiptKey ReceiptKey;

    UPROPERTY(SaveGame)
    FGuid ReceiptPayloadFingerprint;
};

/** Persistent cross-file fence preventing a newer world from accepting an older/replaced character receipt ledger. */
USTRUCT()
struct MYTHIC_API FMythicSavedHarvestContributorLedgerFenceV1 {
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    FString ContributorKey;

    UPROPERTY(SaveGame)
    FGuid LedgerEpoch;

    UPROPERTY(SaveGame)
    uint64 MinimumLedgerRevision = 0;
};

/** Complete replace-on-restore snapshot for the typed deterministic reward outbox. */
USTRUCT()
struct MYTHIC_API FMythicHarvestRewardOutboxSaveV1 {
    GENERATED_BODY()

    static constexpr uint32 CurrentSchemaVersion = 4;
    static constexpr int32 AbsoluteMaximumKnownCompletions = 1048576;
    static constexpr int32 AbsoluteMaximumPendingDeliveries = 131072;
    static constexpr int32 AbsoluteMaximumPendingDeliveriesPerContributor =
        4096;
    static constexpr int32 AbsoluteMaximumDurabilityCostSeries = 131072;
    static constexpr int32 AbsoluteMaximumDurabilityCostSeriesPerContributor =
        4096;
    static constexpr int32 AbsoluteMaximumContributorLedgerFences = 65536;

    UPROPERTY(SaveGame)
    uint32 SchemaVersion = CurrentSchemaVersion;

    UPROPERTY(SaveGame)
    FGuid WorldEpoch;

    /** Monotonic sequence assigned when this exact world-outbox snapshot was captured. */
    UPROPERTY(SaveGame)
    uint64 SnapshotSequence = 0;

    /** Next completion/item/work queue to receive a remainder retry attempt. */
    UPROPERTY(SaveGame)
    uint8 RetryQueueCursor = 0;

    UPROPERTY(SaveGame)
    int32 CompletionRetryRowCursor = 0;

    UPROPERTY(SaveGame)
    int32 ItemRetryRowCursor = 0;

    UPROPERTY(SaveGame)
    int32 WorkRetryRowCursor = 0;

    UPROPERTY(SaveGame)
    int32 DurabilityRetryRowCursor = 0;

    /** Exactly one latest-generation witness per touched node; earlier contiguous generations are covered by high-water. */
    UPROPERTY(SaveGame)
    TArray<FMythicSavedHarvestRewardCompletionV1> KnownCompletions;

    UPROPERTY(SaveGame)
    TArray<FMythicSavedHarvestGenerationHighWaterV1> GenerationHighWatermarks;

    UPROPERTY(SaveGame)
    TArray<FMythicSavedHarvestRewardGrantV1> PendingGrants;

    UPROPERTY(SaveGame)
    TArray<FMythicSavedHarvestCompletionDeliveryV1> PendingCompletionDeliveries;

    UPROPERTY(SaveGame)
    TArray<FMythicSavedHarvestWorkDeliveryV1> PendingWorkDeliveries;

    /** Active-generation high-water and unacknowledged completed-generation durability costs. */
    UPROPERTY(SaveGame)
    TArray<FMythicSavedHarvestDurabilityCostV1> DurabilityCosts;

    /** One monotonic character-ledger lineage/revision floor per contributor observed durably by this world. */
    UPROPERTY(SaveGame)
    TArray<FMythicSavedHarvestContributorLedgerFenceV1>
        ContributorLedgerFences;

    /** Normalizes all set-like persisted arrays before hashing or durable capture. */
    void SortCanonical();
};

enum class EMythicHarvestRewardPrepareStatus : uint8 {
    Prepared,
    AlreadyKnown,
    InvalidWorld,
    InvalidWorldEpoch,
    InvalidContributor,
    PlanningFailed,
    CapacityExceeded,
    SequenceExhausted,
};

enum class EMythicHarvestCompletionAdmission : uint8 {
    Committed,
    AlreadyKnown,
    Invalid,
    CapacityExceeded,
};

struct MYTHIC_API FMythicHarvestRewardPrepareResult {
    EMythicHarvestRewardPrepareStatus Status =
        EMythicHarvestRewardPrepareStatus::InvalidWorld;
    EMythicHarvestRewardPlanStatus PlanStatus =
        EMythicHarvestRewardPlanStatus::InvalidCompletion;
    FName DiagnosticCode;
    int32 PlannedGrantCount = 0;

    bool WasPrepared() const {
        return Status == EMythicHarvestRewardPrepareStatus::Prepared;
    }
};

struct MYTHIC_API FMythicHarvestRewardRetryResult {
    int32 AttemptedGrantCount = 0;
    int32 CompletedGrantCount = 0;
    int64 DeliveredQuantity = 0;
    int32 AttemptedCompletionDeliveryCount = 0;
    int32 CompletedCompletionDeliveryCount = 0;
    int32 AttemptedWorkDeliveryCount = 0;
    int32 CompletedWorkDeliveryCount = 0;
    int32 AttemptedDurabilityCostCount = 0;
    int32 CompletedDurabilityCostCount = 0;
    int32 AttemptedEscrowDeliveryCount = 0;
    int32 CompletedEscrowDeliveryCount = 0;
    int32 CharacterSaveRequestCount = 0;
    int32 PendingGrantCount = 0;
    int32 PendingCompletionDeliveryCount = 0;
    int32 PendingAppliedWorkDeliveryCount = 0;
    int32 PendingEscrowDeliveryCount = 0;
};

/** Pure fair-share retry allocation result for all world and character reward queues. */
struct MYTHIC_API FMythicHarvestRewardRetryBudgets {
    int32 CompletionBudget = 0;
    int32 ItemBudget = 0;
    int32 WorkBudget = 0;
    int32 DurabilityBudget = 0;
    int32 EscrowBudget = 0;
    uint8 NextQueueCursor = 0;
};

USTRUCT()
struct FMythicPendingHarvestRewardDelivery {
    GENERATED_BODY()

    UPROPERTY()
    FMythicHarvestPlannedRewardGrant Grant;

    UPROPERTY()
    int32 RemainingQuantity = 0;
};

/** Runtime completion delivery with transient direct definitions and online-controller fast path. */
USTRUCT()
struct FMythicPendingHarvestCompletionDelivery {
    GENERATED_BODY()

    UPROPERTY()
    FMythicHarvestRewardCompletionKey CompletionKey;

    UPROPERTY()
    FString ContributorKey;

    UPROPERTY()
    FPrimaryAssetId ProficiencyDefinitionId;

    UPROPERTY(Transient)
    TObjectPtr<UProficiencyDefinition> ProficiencyDefinition = nullptr;

    UPROPERTY()
    FPrimaryAssetId HarvestableDefinitionId;

    UPROPERTY(Transient)
    TObjectPtr<UMythicHarvestableDefinition> HarvestableDefinition = nullptr;

    UPROPERTY()
    int64 CompletionProficiencyXPQuanta = 0;

    UPROPERTY()
    FGameplayTagContainer ProficiencyContextTags;

    UPROPERTY()
    int32 QuestCreditCount = 0;

    UPROPERTY()
    bool bProficiencyDelivered = false;

    UPROPERTY()
    bool bQuestCreditDelivered = false;

    UPROPERTY()
    FMythicHarvestReceiptKey ProficiencyReceiptKey;

    UPROPERTY()
    FGuid ProficiencyReceiptPayloadFingerprint;

    UPROPERTY()
    FMythicHarvestReceiptKey QuestReceiptKey;

    UPROPERTY()
    FGuid QuestReceiptPayloadFingerprint;

    UPROPERTY(Transient)
    TWeakObjectPtr<AMythicPlayerController> CurrentController;

    bool IsComplete() const {
        return bProficiencyDelivered && bQuestCreditDelivered;
    }
};

/** Runtime admitted per-work XP delivery. */
USTRUCT()
struct FMythicPendingHarvestWorkDelivery {
    GENERATED_BODY()

    UPROPERTY()
    FMythicHarvestRewardCompletionKey NodeGenerationKey;

    UPROPERTY()
    FString ContributorKey;

    UPROPERTY()
    FMythicHarvestWorkRewardContract WorkRewardContract;

    UPROPERTY(Transient)
    TObjectPtr<UProficiencyDefinition> ProficiencyDefinition = nullptr;

    UPROPERTY()
    int64 CumulativeAppliedWorkQuanta = 0;

    UPROPERTY()
    int64 ProficiencyXPQuanta = 0;

    UPROPERTY()
    FMythicHarvestReceiptKey ReceiptKey;

    UPROPERTY()
    FGuid ReceiptPayloadFingerprint;

    UPROPERTY(Transient)
    TWeakObjectPtr<AMythicPlayerController> CurrentController;
};

/** Runtime cumulative durability charge; the world owns target authority and the character receipt owns application. */
USTRUCT()
struct FMythicPendingHarvestDurabilityCost {
    GENERATED_BODY()

    UPROPERTY()
    FMythicHarvestRewardCompletionKey NodeGenerationKey;

    UPROPERTY()
    FString ContributorKey;

    UPROPERTY()
    FGuid ToolItemInstanceGuid;

    UPROPERTY()
    int64 CumulativeWearTarget = 0;

    UPROPERTY()
    int64 DurablyAppliedWearTarget = 0;

    UPROPERTY()
    FMythicHarvestReceiptKey ReceiptKey;

    UPROPERTY()
    FGuid ReceiptPayloadFingerprint;

    UPROPERTY(Transient)
    TWeakObjectPtr<UMythicItemInstance> CurrentTool;

    UPROPERTY(Transient)
    TWeakObjectPtr<AMythicPlayerController> CurrentController;

    bool HasPendingApplication() const {
        return DurablyAppliedWearTarget < CumulativeWearTarget;
    }
};

/** Side-effect-free completion plan admitted only after the owning node transaction commits. */
struct MYTHIC_API FMythicPreparedHarvestCompletion {
    FMythicHarvestRewardCompletionKey CompletionKey;
    TArray<FMythicHarvestPlannedRewardGrant> Grants;
    TArray<FMythicPendingHarvestCompletionDelivery> CompletionDeliveries;
    uint64 FirstObservableWorldSnapshotSequence = 0;

    bool IsValid() const;
};

/** Side-effect-free per-work XP plan admitted only after the exact accepted-work transaction commits. */
struct MYTHIC_API FMythicPreparedHarvestWorkDelivery {
    FMythicPendingHarvestWorkDelivery Delivery;
    uint64 FirstObservableWorldSnapshotSequence = 0;
    bool bHasDelivery = false;

    bool IsValid() const;
};

/** Side-effect-free durability-cost extension admitted only after the owning node mutation commits. */
struct MYTHIC_API FMythicPreparedHarvestDurabilityCost {
    FMythicPendingHarvestDurabilityCost Cost;
    int64 PreviousCumulativeWearTarget = 0;
    bool bHasCost = false;

    bool IsValid() const;
};

/** Exact save request latch used to reject callbacks crossing a world-outbox restore domain. */
struct MYTHIC_API FMythicHarvestCharacterSaveRequestIdentity {
    FGuid RequestToken;
    FGuid OperationId;
    uint64 RestoreDomainEpoch = 0;

    bool IsValid() const {
        return RequestToken.IsValid() && OperationId.IsValid()
            && RestoreDomainEpoch > 0;
    }
};

/** Pure callback ordering policy shared by production and deterministic crash-race automation. */
struct MYTHIC_API FMythicHarvestCharacterSaveCallbackPolicy {
    static bool MatchesCurrentRequest(
        const FMythicHarvestCharacterSaveRequestIdentity &Latched,
        const FGuid &CallbackRequestToken,
        const FGuid &CallbackOperationId,
        uint64 CallbackRestoreDomainEpoch,
        uint64 ActiveRestoreDomainEpoch);

    static bool RequiresCorrectiveSave(
        bool bPhysicalSaveSucceeded,
        uint64 CallbackRestoreDomainEpoch,
        uint64 ActiveRestoreDomainEpoch,
        bool bHasCurrentDomainRequest);
};

/** Native key for the transient O(1) generation high-water index. */
struct MYTHIC_API FMythicHarvestEpochNodeKey {
    FGuid WorldEpoch;
    FMythicHarvestNodeId NodeId;

    bool IsValid() const { return WorldEpoch.IsValid() && NodeId.IsValid(); }
    bool operator==(const FMythicHarvestEpochNodeKey &Other) const {
        return WorldEpoch == Other.WorldEpoch && NodeId == Other.NodeId;
    }
    friend uint32 GetTypeHash(const FMythicHarvestEpochNodeKey &Key) {
        return HashCombineFast(GetTypeHash(Key.WorldEpoch),
                               GetTypeHash(Key.NodeId));
    }
};

/**
 * Server-only durable delivery service. Receipt application is cumulative and idempotent; an outbox row is
 * acknowledged only from the immutable receipt snapshot attached to a successful character-save callback.
 */
UCLASS()
class MYTHIC_API UMythicHarvestRewardOutboxSubsystem final
    : public UWorldSubsystem {
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;

    FMythicHarvestRewardPrepareResult PrepareCompletion(
        const UMythicHarvestableDefinition &Definition,
        const FGuid &WorldEpoch,
        const FMythicHarvestNodeId &NodeId,
        uint32 Generation,
        TConstArrayView<FMythicHarvestParticipantSnapshot> Participants,
        FMythicPreparedHarvestCompletion &OutPreparedCompletion) const;

    EMythicHarvestCompletionAdmission CommitPreparedCompletion(
        FMythicPreparedHarvestCompletion &&PreparedCompletion);

    /**
     * Freezes one accepted-work XP entitlement without mutating player or outbox state. ReservedAdditionalRows lets
     * a caller reserve a completion plan from the same atomic harvest transaction so both commits are guaranteed to
     * fit before any authoritative node mutation. Participant.ContributionQuanta must be the contributor's cumulative
     * accepted work through this hit; its WorkRewardContract is authored once when unset and otherwise remains frozen.
     */
    bool PrepareAppliedWorkDelivery(
        const UMythicHarvestableDefinition &Definition,
        const FGuid &WorldEpoch,
        const FMythicHarvestNodeId &NodeId,
        uint32 Generation,
        const FMythicHarvestParticipantSnapshot &Participant,
        double AppliedWorkUnits,
        FMythicPreparedHarvestWorkDelivery &OutPreparedDelivery,
        FName &OutDiagnosticCode,
        int32 ReservedAdditionalRows = 0) const;

    /** Admits one prevalidated work entitlement after the owning work transaction commits. */
    bool CommitPreparedAppliedWorkDelivery(
        FMythicPreparedHarvestWorkDelivery &&PreparedDelivery);

    /** Freezes one exact-tool cumulative wear extension without mutating the item, receipt ledger, or outbox. */
    bool PrepareDurabilityCost(
        const FGuid &WorldEpoch,
        const FMythicHarvestNodeId &NodeId,
        uint32 Generation,
        const FString &ContributorKey,
        UMythicItemInstance &Tool,
        int32 WearAmount,
        AMythicPlayerController &Controller,
        FMythicPreparedHarvestDurabilityCost &OutPreparedCost,
        FName &OutDiagnosticCode) const;

    /** Admits one prevalidated exact-tool durability cost after the owning work transaction commits. */
    bool CommitPreparedDurabilityCost(
        FMythicPreparedHarvestDurabilityCost &&PreparedCost);

    /** True only when all older durability costs for Contributor are already represented by the live receipt ledger. */
    bool CanAccrueDurabilityCost(const FString &ContributorKey) const;

    static bool BuildCompletionDeliveries(
        const UMythicHarvestableDefinition &Definition,
        const FMythicHarvestRewardCompletionKey &CompletionKey,
        TConstArrayView<FMythicHarvestParticipantSnapshot> Participants,
        uint64 FirstObservableWorldSnapshotSequence,
        TArray<FMythicPendingHarvestCompletionDelivery> &OutDeliveries,
        FName &OutDiagnosticCode);

    /** Compatibility overload for pure automation; uses sequence one. */
    static bool BuildCompletionDeliveries(
        const UMythicHarvestableDefinition &Definition,
        const FMythicHarvestRewardCompletionKey &CompletionKey,
        TConstArrayView<FMythicHarvestParticipantSnapshot> Participants,
        TArray<FMythicPendingHarvestCompletionDelivery> &OutDeliveries,
        FName &OutDiagnosticCode) {
        return BuildCompletionDeliveries(
            Definition, CompletionKey, Participants, 1, OutDeliveries,
            OutDiagnosticCode);
    }

    static EMythicHarvestCompletionAdmission TryCommitCompletionKey(
        TSet<FMythicHarvestRewardCompletionKey> &InOutKnownCompletions,
        const FMythicHarvestRewardCompletionKey &CompletionKey);

    static int32 CalculateRemainingQuantityAfterInsertion(
        int32 RemainingQuantity, int32 InsertedQuantity);

    /** Allocates a bounded retry budget with a starvation-free cursor rotating all five remainder queues. */
    static FMythicHarvestRewardRetryBudgets CalculateRetryBudgets(
        int32 MaximumAttempts,
        bool bHasCompletionQueue,
        bool bHasItemQueue,
        bool bHasWorkQueue,
        bool bHasDurabilityQueue,
        bool bHasEscrowQueue,
        uint8 QueueCursor);

    /** Pure per-character admission backpressure used to isolate a full inventory from the shard-wide outbox cap. */
    static bool WouldExceedPerContributorPendingCapacity(
        int32 CurrentRows, int32 AdditionalRows);

    /** Advances a queue-local retry cursor after retaining or removing the attempted row. */
    static int32 AdvanceRetryRowCursor(
        int32 CurrentRow, int32 RemainingRowCount, bool bCurrentRowRemoved);

    FMythicHarvestRewardRetryResult RetryPendingDeliveries(
        int32 MaxGrantAttempts = 8);

    /** Captures a new monotonically sequenced world outbox snapshot. */
    bool BuildSaveSnapshot(const FGuid &WorldEpoch,
                           FMythicHarvestRewardOutboxSaveV1 &OutSnapshot,
                           FName &OutDiagnosticCode);

    bool RestoreSaveSnapshot(const FMythicHarvestRewardOutboxSaveV1 &Snapshot,
                             FName &OutDiagnosticCode);

    static bool ValidateSaveSnapshot(
        const FMythicHarvestRewardOutboxSaveV1 &Snapshot,
        FName &OutDiagnosticCode);

    /** Builds a deterministic SaveGame-serialization SHA-256 over the canonical outbox payload. */
    static bool BuildSaveSnapshotFingerprint(
        const FMythicHarvestRewardOutboxSaveV1 &Snapshot,
        FSHA256Signature &OutFingerprint);

    /** Returns the exact canonical payload fingerprint installed by the last successful restore. */
    bool TryGetRestoredSaveSnapshotFingerprint(
        FSHA256Signature &OutFingerprint) const;

    /** Verifies that a candidate snapshot is the exact payload installed by the last successful restore. */
    bool MatchesRestoredSaveSnapshot(
        const FMythicHarvestRewardOutboxSaveV1 &Snapshot) const;

    /** Preflights one durable character receipt snapshot against this world's persisted lineage/revision fence. */
    bool ValidateCharacterReceiptSnapshot(
        const FString &ContributorKey,
        const FMythicHarvestReceiptLedgerSaveV1 &Snapshot,
        const FMythicHarvestItemEscrowSaveV1 &EscrowSnapshot,
        FName &OutDiagnosticCode) const;

    /** Pure candidate-world overload used before a world snapshot is installed over live characters. */
    static bool ValidateCharacterReceiptSnapshotAgainstWorld(
        const FMythicHarvestRewardOutboxSaveV1 &WorldSnapshot,
        const FString &ContributorKey,
        const FMythicHarvestReceiptLedgerSaveV1 &CharacterSnapshot,
        const FMythicHarvestItemEscrowSaveV1 &EscrowSnapshot,
        FName &OutDiagnosticCode);

    /** Observes one exact successful/loaded durable character snapshot and reconciles pending delivery prefixes. */
    bool ObserveDurableCharacterReceiptSnapshot(
        const FString &ContributorKey,
        const FMythicHarvestReceiptLedgerSaveV1 &Snapshot,
        const FMythicHarvestItemEscrowSaveV1 &EscrowSnapshot,
        FName &OutDiagnosticCode);

    /** Returns the next never-issued lifecycle generation in O(1), failing closed on invalid identity/exhaustion. */
    bool TryResolveNextGeneration(const FGuid &WorldEpoch,
                                  const FMythicHarvestNodeId &NodeId,
                                  uint32 &OutGeneration) const;

    /** Returns the installed outbox high-water for one epoch/node identity; false means invalid identity or no witness. */
    bool TryGetHighestKnownGeneration(
        const FGuid &WorldEpoch,
        const FMythicHarvestNodeId &NodeId,
        uint32 &OutGeneration) const;

    bool HasKnownCompletion(const FGuid &WorldEpoch,
                            const FMythicHarvestNodeId &NodeId,
                            uint32 Generation) const;

    /** Records a successfully persisted world sequence for safe player-receipt compaction boundaries. */
    bool MarkWorldSnapshotDurable(
        const FMythicHarvestRewardOutboxSaveV1 &Snapshot,
        FName &OutDiagnosticCode);

    /** Safely compacts one online player's completed receipts against the latest durable world omission boundary. */
    bool TryCompactReceiptLedgerForPlayer(AMythicPlayerState &PlayerState,
                                          FName &OutDiagnosticCode);

    int32 GetPendingGrantCount() const { return PendingDeliveries.Num(); }
    int32 GetPendingWorkCount() const {
        return PendingDeliveries.Num() + PendingCompletionDeliveries.Num()
            + PendingWorkDeliveries.Num()
            + PendingDurabilityReceiptOrder.Num();
    }
    bool HasPendingWork() const;
    uint64 GetLastIssuedWorldSnapshotSequence() const {
        return LastIssuedWorldSnapshotSequence;
    }

private:
    AMythicPlayerController *ResolveContributorController(
        FMythicHarvestPlannedRewardGrant &Grant) const;
    AMythicPlayerController *ResolveContributorController(
        const FString &ContributorKey,
        TWeakObjectPtr<AMythicPlayerController> &InOutCurrentController) const;
    AMythicPlayerState *ResolveContributorPlayerState(
        const FString &ContributorKey) const;
    UMythicHarvestReceiptLedgerComponent *ResolveReceiptLedger(
        AMythicPlayerState *PlayerState) const;
    UMythicHarvestRewardEscrowComponent *ResolveRewardEscrow(
        AMythicPlayerState *PlayerState) const;
    UItemDefinition *ResolveItemDefinition(
        FMythicHarvestPlannedRewardGrant &Grant) const;
    UProficiencyDefinition *ResolveProficiencyDefinition(
        FMythicPendingHarvestCompletionDelivery &Delivery) const;
    UProficiencyDefinition *ResolveProficiencyDefinition(
        FMythicPendingHarvestWorkDelivery &Delivery) const;
    UMythicHarvestableDefinition *ResolveHarvestableDefinition(
        FMythicPendingHarvestCompletionDelivery &Delivery) const;
    void RequestPendingDefinitionLoads();
    void RefreshLoadedDefinitions();
    bool RequestContributorDurabilitySave(const FString &ContributorKey,
                                          AMythicPlayerState &PlayerState);
    void HandleDurableCharacterSaveResult(
        FString ExpectedContributorKey,
        FGuid ExpectedRequestToken,
        TSharedRef<FGuid> ExpectedOperationId,
        uint64 ExpectedRestoreDomainEpoch,
        const FMythicDurableCharacterSaveResult &Result);
    void ReconcileContributorFromDurableSnapshot(
        const FString &ContributorKey,
        const FMythicHarvestReceiptLedgerSaveV1 &Snapshot,
        const FMythicHarvestItemEscrowSaveV1 &EscrowSnapshot);
    bool TryDeliverOneEscrowRow(
        AMythicPlayerState &PlayerState,
        UMythicItemFactorySubsystem &Factory,
        int64 &OutDeliveredQuantity,
        bool &bOutRowCompleted);
    bool HasPendingEscrowWork() const;
    int32 GetPendingEscrowRowCount() const;
    static FMythicSavedHarvestItemEscrowRowV1 BuildEscrowRow(
        const FMythicPendingHarvestRewardDelivery &Pending,
        uint64 FirstObservedWorldSnapshotSequence);
    UMythicItemInstance *ResolveDurabilityTool(
        FMythicPendingHarvestDurabilityCost &Cost,
        AMythicPlayerController &Controller) const;
    bool IsGenerationCompleted(
        const FMythicHarvestRewardCompletionKey &Key) const;
    void RemoveDurabilityCostSeries(
        const FMythicHarvestReceiptKey &ReceiptKey);
    void RemovePendingDurabilityReceipt(
        const FString &ContributorKey,
        const FMythicHarvestReceiptKey &ReceiptKey);
    void AdjustPendingContributorRows(
        const FString &ContributorKey, int32 Delta);
    void AdjustPendingItemContributorRows(
        const FString &ContributorKey, int32 Delta);
    void AdjustDurabilityContributorSeries(
        const FString &ContributorKey, int32 Delta);
    bool HasPendingDurabilityCost() const {
        return !PendingDurabilityReceiptOrder.IsEmpty();
    }
    bool IsContributorAwaitingCurrentCharacterSave(
        const FString &ContributorKey) const;
    static bool ValidateReceiptInSnapshot(
        const FMythicHarvestReceiptLedgerSaveV1 &Snapshot,
        const FMythicHarvestReceiptKey &Key,
        const FGuid &Fingerprint,
        int64 TargetQuantity,
        int64 &OutAppliedQuantity,
        EMythicHarvestQuestReceiptDisposition *OutQuestDisposition = nullptr);
    static bool IsSameGrantIdentity(
        const FMythicPendingHarvestRewardDelivery &Left,
        const FMythicPendingHarvestRewardDelivery &Right);
    static void SortPendingDeliveries(
        TArray<FMythicPendingHarvestRewardDelivery> &Deliveries);
    static void SortPendingCompletionDeliveries(
        TArray<FMythicPendingHarvestCompletionDelivery> &Deliveries);
    static void SortPendingWorkDeliveries(
        TArray<FMythicPendingHarvestWorkDelivery> &Deliveries);
    bool WouldExceedPendingCapacity(int32 AdditionalRows) const;
    int32 GetPendingRowCountForContributor(
        const FString &ContributorKey) const;
    bool WouldExceedContributorPendingCapacity(
        const FString &ContributorKey,
        int32 AdditionalRows) const;
    bool WouldExceedDurabilityContributorCapacity(
        const FString &ContributorKey, int32 AdditionalRows) const;
    bool PreparedCompletionWouldExceedContributorCapacity(
        const FMythicPreparedHarvestCompletion &PreparedCompletion) const;
    bool PreparedCompletionWouldExceedEscrowCapacity(
        const FMythicPreparedHarvestCompletion &PreparedCompletion) const;
    bool WouldExceedContributorFenceCapacity(
        const FString &ContributorKey) const;
    void ReleaseContributorFenceReservationIfUnused(
        const FString &ContributorKey);
    uint64 GetFirstObservableSnapshotSequence() const;

    UPROPERTY(Transient)
    TArray<FMythicPendingHarvestRewardDelivery> PendingDeliveries;

    UPROPERTY(Transient)
    TArray<FMythicPendingHarvestCompletionDelivery>
        PendingCompletionDeliveries;

    UPROPERTY(Transient)
    TArray<FMythicPendingHarvestWorkDelivery> PendingWorkDeliveries;

    UPROPERTY(Transient)
    TMap<FMythicHarvestReceiptKey, FMythicPendingHarvestDurabilityCost>
        DurabilityCostsByReceipt;

    TArray<FMythicHarvestReceiptKey> PendingDurabilityReceiptOrder;
    TMultiMap<FString, FMythicHarvestReceiptKey>
        PendingDurabilityReceiptsByContributor;
    TMultiMap<FMythicHarvestRewardCompletionKey,
              FMythicHarvestReceiptKey>
        DurabilityReceiptsByNodeGeneration;

    UPROPERTY(Transient)
    TSet<FMythicHarvestRewardCompletionKey> KnownCompletions;

    TMap<FMythicHarvestEpochNodeKey, uint32>
        HighestKnownGenerationByNode;
    TMap<FString, int32> PendingRowCountByContributor;
    TMap<FString, int32> PendingItemRowCountByContributor;
    TMap<FString, int32> DurabilitySeriesCountByContributor;
    TMap<FString, FMythicSavedHarvestContributorLedgerFenceV1>
        ContributorLedgerFenceByKey;
    TSet<FString> ReservedContributorLedgerFenceKeys;
    TMap<FString, FMythicHarvestCharacterSaveRequestIdentity>
        CharacterSaveRequestsByContributor;
    TSet<TWeakObjectPtr<AMythicPlayerState>> TrackedReceiptOwners;
    TSet<FMythicHarvestReceiptKey> LastDurablePendingReceiptKeys;
    TMap<FMythicHarvestNodeId, uint32>
        LastDurableCompletedGenerationByNode;
    TSet<FPrimaryAssetId> InFlightDefinitionLoads;
    TArray<TSharedPtr<FStreamableHandle>> DefinitionLoadHandles;
    uint64 LastIssuedWorldSnapshotSequence = 0;
    uint64 LastDurableWorldSnapshotSequence = 0;
    FGuid LastDurableWorldEpoch;
    uint8 RetryQueueCursor = 0;
    int32 CompletionRetryRowCursor = 0;
    int32 ItemRetryRowCursor = 0;
    int32 WorkRetryRowCursor = 0;
    int32 DurabilityRetryRowCursor = 0;
    int32 EscrowContributorRetryCursor = 0;
    uint64 RestoreDomainEpoch = 1;
    FSHA256Signature RestoredSaveSnapshotFingerprint{};
    bool bHasRestoredSaveSnapshotFingerprint = false;
};
