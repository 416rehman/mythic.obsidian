#pragma once

#include "CoreMinimal.h"
#include "World/Harvesting/MythicHarvestReceiptTypes.h"
#include "World/Harvesting/MythicHarvestTypes.h"

#include "MythicHarvestSaveTypes.generated.h"

/** Frozen contributor inputs for one touched Available node. */
USTRUCT()
struct MYTHIC_API FMythicSavedHarvestContributorV1 {
    GENERATED_BODY()

    /** Canonical persistent character identity; connection/session identities are never durable entitlement keys. */
    UPROPERTY(SaveGame)
    FString ContributorKey;

    /** Exact accepted work attributed to this contributor in authoritative fixed-point quanta. */
    UPROPERTY(SaveGame)
    int64 ContributionQuanta = 0;

    /** Latest authority-captured item level used if this generation later completes while the player is offline. */
    UPROPERTY(SaveGame)
    int32 ItemLevel = 1;

    /** Latest authority-captured quantity multiplier in deterministic fixed-point quanta. */
    UPROPERTY(SaveGame)
    int32 QuantityMultiplierQuanta = 0;

    /** Latest authority-captured proficiency level used by completion reward planning. */
    UPROPERTY(SaveGame)
    int32 ProficiencyLevel = 0;

    /** First-hit-frozen typed work-XP contract; required even when progression was intentionally disabled. */
    UPROPERTY(SaveGame)
    FMythicHarvestWorkRewardContract WorkRewardContract;
};

/**
 * Version-one durable lifecycle row for one touched stable harvest node.
 *
 * Untouched Available nodes remain implicit. Partially worked Available nodes persist exact work and frozen
 * contributor inputs so save/load, crashes, and World Partition lifetimes cannot heal a node after durable work XP
 * has been granted. The captured maximum is this touched generation's immutable work contract; later definition
 * balance applies only to untouched or future generations. Soft claims and controller references never persist.
 */
USTRUCT()
struct MYTHIC_API FMythicSavedHarvestNodeV1 {
    GENERATED_BODY()

    /** Opaque stable identity emitted by cooked authored/PCG identity data; never an instance index or object path. */
    UPROPERTY(SaveGame)
    FGuid NodeGuid;

    /** Persisted world-lifetime epoch that scopes completion idempotency across available-node resets. */
    UPROPERTY(SaveGame)
    FGuid WorldEpoch;

    /** Lifecycle generation active when the snapshot was captured; zero is invalid. */
    UPROPERTY(SaveGame)
    uint32 Generation = 0;

    /** Monotonic revision within Generation when the snapshot was captured; zero is invalid. */
    UPROPERTY(SaveGame)
    uint32 Revision = 0;

    /** Cooked spatial replication bucket used even while the authority provider is streamed out. */
    UPROPERTY(SaveGame)
    FIntPoint ReplicationCellCoordinate = FIntPoint::ZeroValue;

    /** Finite cell-center height used to spawn the spatial replication proxy without loading a provider. */
    UPROPERTY(SaveGame)
    float ReplicationCellCenterZ = 0.0f;

    /** Available is legal only for strict partial work; untouched Available nodes remain implicit. */
    UPROPERTY(SaveGame)
    EMythicHarvestNodeState State = EMythicHarvestNodeState::Depleted;

    /** Respawn duration remaining at capture; exactly zero for a partially worked Available row. */
    UPROPERTY(SaveGame)
    double RemainingRespawnSeconds = 0.0;

    /** First-hit-frozen maximum work for this exact lifecycle generation. */
    UPROPERTY(SaveGame)
    int64 CapturedMaximumWorkQuanta = 0;

    /** Exact remaining authoritative work; positive and below CapturedMaximumWorkQuanta for Available rows. */
    UPROPERTY(SaveGame)
    int64 RemainingWorkQuanta = 0;

    /** Canonically sorted frozen contributors for a partial Available generation; empty for unavailable rows. */
    UPROPERTY(SaveGame)
    TArray<FMythicSavedHarvestContributorV1> Contributors;
};

/** Complete replace-on-restore snapshot for authoritative harvest-node lifecycle state. */
USTRUCT()
struct MYTHIC_API FMythicHarvestWorldSaveV1 {
    GENERATED_BODY()

    static constexpr uint32 CurrentSchemaVersion = 1;
    /** Hard allocation ceiling for touched node rows admitted from one world snapshot. */
    static constexpr int32 AbsoluteMaximumNodes = 262144;
    /** Hard allocation ceiling for contributors retained by one partially worked node. */
    static constexpr int32 AbsoluteMaximumContributorsPerNode = 128;
    /** Hard allocation ceiling across every partial-node contributor row in one snapshot. */
    static constexpr int32 AbsoluteMaximumTotalContributors = 1048576;
    /** Hard ceiling on distinct runtime replication proxies a restore may pre-create. */
    static constexpr int32 AbsoluteMaximumReplicationCells = 65536;
    /** Supported deployment envelope in signed spatial-cell coordinates. */
    static constexpr int32 AbsoluteMaximumCellCoordinateMagnitude = 1000000;
    /** Supported deployment envelope for saved replication-cell center height, in centimeters. */
    static constexpr float AbsoluteMaximumCellCenterZCentimeters = 1000000000.0f;

    /** Exact schema version; unreleased hard cutover rejects every other version instead of migrating it. */
    UPROPERTY(SaveGame)
    uint32 SchemaVersion = CurrentSchemaVersion;

    /** Persisted world-lifetime epoch shared by every row and the deterministic completion/reward outbox. */
    UPROPERTY(SaveGame)
    FGuid WorldEpoch;

    /** Canonically sorted touched-node rows; streamed-out providers remain staged by stable identity. */
    UPROPERTY(SaveGame)
    TArray<FMythicSavedHarvestNodeV1> Nodes;

    /** Pure structural validation used before any live world or reward-outbox state is replaced. */
    static bool Validate(
        const FMythicHarvestWorldSaveV1 &Snapshot,
        FName &OutDiagnosticCode,
        int32 ConfiguredMaximumNodes = AbsoluteMaximumNodes,
        int32 ConfiguredMaximumContributorsPerNode =
            AbsoluteMaximumContributorsPerNode,
        int32 ConfiguredMaximumTotalContributors =
            AbsoluteMaximumTotalContributors,
        int32 ConfiguredMaximumReplicationCells =
            AbsoluteMaximumReplicationCells,
        int32 ConfiguredMaximumCellCoordinateMagnitude =
            AbsoluteMaximumCellCoordinateMagnitude);

    /** Sorts rows by stable GUID for deterministic serialization/checksums without changing row semantics. */
    void SortCanonical();
};
