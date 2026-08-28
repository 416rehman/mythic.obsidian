#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "World/Harvesting/MythicHarvestTypes.h"

#include "MythicHarvestReceiptTypes.generated.h"

/** Native semantic channel recorded by the player-owned harvest receipt ledger. */
UENUM()
enum class EMythicHarvestReceiptChannel : uint8 {
    PrimaryMaterial = 0,
    BonusLoot = 1,
    CompletionProficiencyXP = 2,
    CompletionQuestCredit = 3,
    AppliedWorkProficiencyXP = 4,
    /** Saturating durability cost charged to one exact persistent tool instance. */
    DurabilityCost = 5,
};

/** Terminal result of consuming one typed harvest quest-credit entitlement. */
UENUM()
enum class EMythicHarvestQuestReceiptDisposition : uint8 {
    None = 0,
    Matched = 1,
    NoMatch = 2,
};

/**
 * Player-local semantic identity for one harvest entitlement.
 *
 * The owning character is deliberately not part of this key. World/node identity, lifecycle generation, a typed
 * channel, and numeric/GUID ordinals are sufficient; strings, names, object paths, transforms, and asset locators are
 * forbidden. Work XP uses a deterministic contributor-scoped series GUID. Its cumulative target is the durable
 * logical interval [0, accepted contributor work), so replaying an older world snapshot cannot mint a new receipt.
 */
USTRUCT()
struct MYTHIC_API FMythicHarvestReceiptKey {
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    FGuid WorldEpoch;

    UPROPERTY(SaveGame)
    FMythicHarvestNodeId NodeId;

    UPROPERTY(SaveGame)
    uint32 Generation = 0;

    UPROPERTY(SaveGame)
    EMythicHarvestReceiptChannel Channel =
        EMythicHarvestReceiptChannel::PrimaryMaterial;

    UPROPERTY(SaveGame)
    uint32 EntryOrdinal = 0;

    /** Deterministic contributor/tool series identity for cumulative channels; invalid for one-shot completion rows. */
    UPROPERTY(SaveGame)
    FGuid SeriesGuid;

    static FMythicHarvestReceiptKey MakeCompletion(
        const FGuid &WorldEpoch,
        const FMythicHarvestNodeId &NodeId,
        uint32 Generation,
        EMythicHarvestReceiptChannel Channel,
        uint32 EntryOrdinal = 0);

    static FMythicHarvestReceiptKey MakeAppliedWork(
        const FGuid &WorldEpoch,
        const FMythicHarvestNodeId &NodeId,
        uint32 Generation,
        const FString &ContributorKey);

    static FMythicHarvestReceiptKey MakeDurabilityCost(
        const FGuid &WorldEpoch,
        const FMythicHarvestNodeId &NodeId,
        uint32 Generation,
        const FString &ContributorKey,
        const FGuid &ToolItemInstanceGuid);

    bool IsValid() const;

    bool operator==(const FMythicHarvestReceiptKey &Other) const {
        return WorldEpoch == Other.WorldEpoch && NodeId == Other.NodeId
            && Generation == Other.Generation && Channel == Other.Channel
            && EntryOrdinal == Other.EntryOrdinal
            && SeriesGuid == Other.SeriesGuid;
    }

    friend uint32 GetTypeHash(const FMythicHarvestReceiptKey &Key) {
        uint32 Hash = HashCombineFast(GetTypeHash(Key.WorldEpoch),
                                      GetTypeHash(Key.NodeId));
        Hash = HashCombineFast(Hash, GetTypeHash(Key.Generation));
        Hash = HashCombineFast(Hash,
                               GetTypeHash(static_cast<uint8>(Key.Channel)));
        Hash = HashCombineFast(Hash, GetTypeHash(Key.EntryOrdinal));
        return HashCombineFast(Hash, GetTypeHash(Key.SeriesGuid));
    }
};

/** Fixed-point quantity used for proficiency-XP receipt targets. */
struct MYTHIC_API FMythicHarvestReceiptQuantity {
    static constexpr int64 QuantaPerUnit = 10000;

    static bool TryFromUnits(double Units, int64 &OutQuanta);
    static double ToUnits(int64 Quanta);

    /** Calculates the rounded cumulative XP target for a deterministic applied-work series without integer overflow. */
    static bool TryCalculateCumulativeAppliedWorkXP(
        int64 CumulativeAppliedWorkQuanta,
        int64 ProficiencyXPPerWorkUnitQuanta,
        int64 &OutCumulativeXPQuanta);
};

/**
 * Immutable per-generation/contributor applied-work progression contract.
 *
 * The first accepted hit freezes this typed payload from the harvestable definition. Every later hit and partial
 * world save carries it forward, so live balance edits cannot change an in-progress receipt series.
 */
USTRUCT()
struct MYTHIC_API FMythicHarvestWorkRewardContract {
    GENERATED_BODY()

    /** Distinguishes no accepted hit from an intentionally frozen-disabled zero-XP contract. */
    UPROPERTY(SaveGame)
    bool bInitialized = false;

    UPROPERTY(SaveGame)
    FPrimaryAssetId ProficiencyDefinitionId;

    /** Frozen XP rate in 1/10,000-XP quanta per work unit. */
    UPROPERTY(SaveGame)
    int64 ProficiencyXPPerWorkUnitQuanta = 0;

    UPROPERTY(SaveGame)
    FGameplayTagContainer ContextTags;

    bool IsUnset() const;
    bool IsValid() const;
    bool IsEnabled() const {
        return bInitialized && ProficiencyXPPerWorkUnitQuanta > 0;
    }

    bool operator==(const FMythicHarvestWorkRewardContract &Other) const {
        return bInitialized == Other.bInitialized
            && ProficiencyDefinitionId == Other.ProficiencyDefinitionId
            && ProficiencyXPPerWorkUnitQuanta
                == Other.ProficiencyXPPerWorkUnitQuanta
            && ContextTags == Other.ContextTags;
    }
};

/** One immutable entitlement contract and its cumulative player-side application receipt. */
USTRUCT()
struct MYTHIC_API FMythicSavedHarvestReceiptRowV1 {
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    FMythicHarvestReceiptKey Key;

    /** Canonical conflict detector for the frozen payload; it is never an asset locator. */
    UPROPERTY(SaveGame)
    FGuid PayloadFingerprint;

    /** Immutable cumulative target, in item/quest units or 1/10,000 XP according to Key.Channel. */
    UPROPERTY(SaveGame)
    int64 TargetQuantity = 0;

    /** Cumulative quantity already applied to this character in the same units as TargetQuantity. */
    UPROPERTY(SaveGame)
    int64 AppliedQuantity = 0;

    /** Latest issued world-outbox snapshot sequence when this receipt was first admitted. */
    UPROPERTY(SaveGame)
    uint64 FirstObservedWorldSnapshotSequence = 0;

    /** Monotonic ledger revision at which AppliedQuantity last changed. */
    UPROPERTY(SaveGame)
    uint64 MutationRevision = 0;

    /** Quest channels persist both matched and no-match consumption so future quests cannot consume a replay. */
    UPROPERTY(SaveGame)
    EMythicHarvestQuestReceiptDisposition QuestDisposition =
        EMythicHarvestQuestReceiptDisposition::None;

    bool IsComplete() const {
        return TargetQuantity > 0 && AppliedQuantity == TargetQuantity;
    }
};

/** Minimum durable world-outbox snapshot accepted after compacting receipts in one world epoch. */
USTRUCT()
struct MYTHIC_API FMythicSavedHarvestReceiptWorldWatermarkV1 {
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    FGuid WorldEpoch;

    UPROPERTY(SaveGame)
    uint64 MinimumAcceptedSnapshotSequence = 0;
};

/** Complete replace-on-restore character-owned receipt snapshot. */
USTRUCT()
struct MYTHIC_API FMythicHarvestReceiptLedgerSaveV1 {
    GENERATED_BODY()

    static constexpr uint32 CurrentSchemaVersion = 3;
    static constexpr int32 AbsoluteMaximumRows = 65536;
    static constexpr int32 AbsoluteMaximumWorldWatermarks = 64;

    UPROPERTY(SaveGame)
    uint32 SchemaVersion = CurrentSchemaVersion;

    /** Stable lineage of this character receipt ledger; world fences reject snapshots from an older/replaced lineage. */
    UPROPERTY(SaveGame)
    FGuid LedgerEpoch;

    UPROPERTY(SaveGame)
    uint64 LedgerRevision = 0;

    UPROPERTY(SaveGame)
    TArray<FMythicSavedHarvestReceiptRowV1> Rows;

    UPROPERTY(SaveGame)
    TArray<FMythicSavedHarvestReceiptWorldWatermarkV1> WorldWatermarks;

    static bool Validate(const FMythicHarvestReceiptLedgerSaveV1 &Snapshot,
                         FName &OutDiagnosticCode,
                         int32 ConfiguredMaximumRows = AbsoluteMaximumRows);

    void SortCanonical();

    const FMythicSavedHarvestReceiptRowV1 *FindRow(
        const FMythicHarvestReceiptKey &Key) const;
};

/** Pure canonical payload fingerprinting; returned GUIDs validate equality and never resolve content. */
struct MYTHIC_API FMythicHarvestReceiptFingerprint {
    static FGuid Build(
        const FMythicHarvestReceiptKey &Key,
        const FPrimaryAssetId &TypedAssetId,
        int64 TargetQuantity,
        uint64 FrozenSeed,
        uint32 FrozenAuxiliaryA,
        uint32 FrozenAuxiliaryB,
        const FGameplayTagContainer &FrozenContextTags =
            FGameplayTagContainer());

    /** Builds the immutable contract for a cumulative work-XP series; the changing cumulative target is excluded. */
    static FGuid BuildAppliedWorkSeries(
        const FMythicHarvestReceiptKey &Key,
        const FPrimaryAssetId &ProficiencyDefinitionId,
        int64 ProficiencyXPPerWorkUnitQuanta,
        const FGameplayTagContainer &FrozenContextTags =
            FGameplayTagContainer());

    /** Builds the immutable contract fingerprint for one tool-specific cumulative durability-cost series. */
    static FGuid BuildDurabilityCostSeries(
        const FMythicHarvestReceiptKey &Key,
        const FGuid &ToolItemInstanceGuid);
};
