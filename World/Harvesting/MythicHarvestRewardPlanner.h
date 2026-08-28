#pragma once

#include "CoreMinimal.h"
#include "World/Harvesting/MythicHarvestableDefinition.h"
#include "World/Harvesting/MythicHarvestTypes.h"
#include "World/Harvesting/MythicHarvestReceiptTypes.h"

#include "MythicHarvestRewardPlanner.generated.h"

class AMythicPlayerController;
class UItemDefinition;

/** Native reward channel encoded into deterministic roll and grant identities. */
UENUM()
enum class EMythicHarvestRewardChannel : uint8 {
    PrimaryMaterial = 0,
    BonusLoot = 1,
};

/** Stable identity of one node-generation completion. */
USTRUCT()
struct MYTHIC_API FMythicHarvestRewardCompletionKey {
    GENERATED_BODY()

    UPROPERTY()
    FGuid WorldEpoch;

    UPROPERTY()
    FMythicHarvestNodeId NodeId;

    UPROPERTY()
    uint32 Generation = 0;

    bool IsValid() const {
        return WorldEpoch.IsValid() && NodeId.IsValid() && Generation > 0;
    }

    bool operator==(const FMythicHarvestRewardCompletionKey &Other) const {
        return WorldEpoch == Other.WorldEpoch && NodeId == Other.NodeId
            && Generation == Other.Generation;
    }

    friend uint32 GetTypeHash(const FMythicHarvestRewardCompletionKey &Key) {
        uint32 Hash = HashCombineFast(GetTypeHash(Key.WorldEpoch),
                                      GetTypeHash(Key.NodeId));
        return HashCombineFast(Hash, GetTypeHash(Key.Generation));
    }
};

/** One immutable item grant produced before any delivery attempt. */
USTRUCT()
struct MYTHIC_API FMythicHarvestPlannedRewardGrant {
    GENERATED_BODY()

    UPROPERTY()
    FMythicHarvestRewardCompletionKey CompletionKey;

    UPROPERTY()
    EMythicHarvestRewardChannel Channel = EMythicHarvestRewardChannel::PrimaryMaterial;

    UPROPERTY()
    int32 RewardRowIndex = INDEX_NONE;

    UPROPERTY()
    FString ContributorKey;

    UPROPERTY()
    TObjectPtr<UItemDefinition> ItemDefinition = nullptr;

    UPROPERTY()
    FPrimaryAssetId ItemDefinitionId;

    UPROPERTY()
    int32 Quantity = 0;

    UPROPERTY()
    int32 ItemLevel = 1;

    /** True when ResolvedQuality must override the item definition template during transactional construction. */
    UPROPERTY()
    bool bHasResolvedQuality = false;

    /** Immutable quality outcome frozen by the authority planner; never a deferred policy or mutable data lookup. */
    UPROPERTY()
    EMythicYieldQuality ResolvedQuality = EMythicYieldQuality::Common;

    UPROPERTY()
    uint64 ItemSeed = 0;

    /** Typed player-receipt identity frozen by the outbox prepare step before this grant may be admitted. */
    UPROPERTY()
    FMythicHarvestReceiptKey ReceiptKey;

    /** Immutable payload conflict detector; it never resolves an item or any other content. */
    UPROPERTY()
    FGuid ReceiptPayloadFingerprint;

    UPROPERTY(Transient)
    TWeakObjectPtr<AMythicPlayerController> InitialController;

    bool IsValid() const;
};

/** Distilled stable participant consumed by the pure deterministic planner. */
struct MYTHIC_API FMythicHarvestRewardParticipant {
    FString ContributorKey;
    int64 ContributionQuanta = 0;
    int32 ItemLevel = 1;
    /** ItemQuantityFind quantized to QuantityMultiplierScale; consumed only by the primary-material channel. */
    int32 QuantityMultiplierQuanta = 1000000;
    /** Exact level of the Harvestable Definition's direct proficiency at authoritative plan time. */
    int32 ProficiencyLevel = 0;
    TWeakObjectPtr<AMythicPlayerController> InitialController;
};

enum class EMythicHarvestRewardPlanStatus : uint8 {
    Success,
    InvalidCompletion,
    InvalidDefinition,
    InvalidContributor,
    DuplicateContributor,
    ArithmeticOverflow,
};

struct MYTHIC_API FMythicHarvestRewardPlanResult {
    EMythicHarvestRewardPlanStatus Status =
        EMythicHarvestRewardPlanStatus::InvalidCompletion;
    FName DiagnosticCode;
    TArray<FMythicHarvestPlannedRewardGrant> Grants;

    bool IsSuccess() const { return Status == EMythicHarvestRewardPlanStatus::Success; }
};

/** Pure, versioned reward planning and exact contribution splitting. */
struct MYTHIC_API FMythicHarvestRewardPlanner {
    static constexpr uint32 CanonicalVersion = 2;
    static constexpr int32 QuantityMultiplierScale = 1000000;
    /** Arithmetic safety ceiling, not a balance clamp; out-of-contract live stats fail planning instead of truncating. */
    static constexpr double MaximumQuantityMultiplier = 100.0;

    static FMythicHarvestRewardPlanResult PlanCompletion(
        const UMythicHarvestableDefinition &Definition,
        const FGuid &WorldEpoch,
        const FMythicHarvestNodeId &NodeId,
        uint32 Generation,
        const FMythicYieldQualityRules &QualityRules,
        TConstArrayView<FMythicHarvestRewardParticipant> Participants);

    /** Quantizes one finite non-negative live ItemQuantityFind value without silently clamping invalid balance data. */
    static bool TryQuantizeQuantityMultiplier(
        double Multiplier, int32 &OutMultiplierQuanta);

    /** Splits Quantity exactly; output follows canonical contributor-key order, not input order. */
    static bool SplitQuantityLargestRemainder(
        int32 Quantity,
        TConstArrayView<FMythicHarvestRewardParticipant> Participants,
        TArray<int32> &OutQuantities,
        TArray<int32> *OutCanonicalSourceIndices = nullptr);

    /**
     * Applies each participant's fixed ItemQuantityFind entitlement and Hamilton-allocates whole material units.
     * The supplied rounding seed freezes the final fractional unit; output follows canonical contributor-key order.
     */
    static bool SplitPrimaryMaterialQuantity(
        int32 BaseQuantity,
        uint64 RoundingSeed,
        TConstArrayView<FMythicHarvestRewardParticipant> Participants,
        TArray<int32> &OutQuantities,
        TArray<int32> *OutCanonicalSourceIndices = nullptr);

    /** Returns the nonzero factory seed frozen into one contributor grant. */
    static uint64 DeriveItemSeed(
        const FGuid &WorldEpoch,
        const FMythicHarvestNodeId &NodeId,
        uint32 Generation,
        EMythicHarvestRewardChannel Channel,
        int32 RewardRowIndex,
        const FString &ContributorKey);
};
