#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/EngineTypes.h"
#include "MythicHarvestSettings.generated.h"

class UInputAction;
class UInputMappingContext;

/** Cross-cutting authority, replication, and feedback policy shared by every harvestable definition. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Mythic Harvesting"))
class MYTHIC_API UMythicHarvestSettings : public UDeveloperSettings {
    GENERATED_BODY()

public:
    virtual FName GetCategoryName() const override { return FName("Game"); }

    /**
     * Server-owned maximum distance from authority avatar to authoritative impact point; clients may read it for
     * prediction, nonfinite/nonpositive values fail validation, and units are Unreal centimeters.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Authority", meta = (ClampMin = "1.0", Units = "cm"))
    float AuthoritativeRangeCentimeters = 350.0f;

    /**
     * Server-owned blocking trace channel used for harvest line of sight; clients may mirror it for focus only,
     * invalid enum values fail validation, and failed traces reject without mutation.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Authority")
    TEnumAsByte<ECollisionChannel> LineOfSightTraceChannel = ECC_Visibility;

    /**
     * Server-owned line-of-sight complexity policy; clients may mirror it for focus only, trace failure rejects
     * without mutation, and the boolean has no units.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Authority")
    bool bTraceComplexLineOfSight = false;

    /**
     * Server-owned tolerance around the exact attack cadence contract; clients cannot extend it, negative/nonfinite
     * values fail validation, and units are seconds.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Authority", meta = (ClampMin = "0.0", Units = "s"))
    float CadenceToleranceSeconds = 0.075f;

    /**
     * Server-owned duration refreshed by accepted same-party work; clients may display claim state, invalid values
     * fail validation, expiry itself has no reward side effects, and units are seconds.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Claims", meta = (ClampMin = "0.001", Units = "s"))
    float SoftClaimDurationSeconds = 8.0f;

    /**
     * Server-owned minimum share of total applied work required for completion participation; clients may display it,
     * out-of-range/nonfinite values fail validation, and units are a normalized fraction in [0,1].
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Claims", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MinimumContributionFraction = 0.01f;

    /**
     * Server-owned lower clamp for the live GAS work multiplier; clients may display projected work, invalid values
     * fail validation, and the value is a positive unitless multiplier so an accepted hit always applies work.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Work", meta = (ClampMin = "0.0001"))
    float MinimumWorkMultiplier = 0.05f;

    /**
     * Server-owned upper clamp for the live GAS work multiplier; clients may display projected work, values below the
     * minimum or nonfinite values fail validation, and the value is a unitless multiplier.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Work", meta = (ClampMin = "0.0001"))
    float MaximumWorkMultiplier = 100.0f;

    /**
     * Settings-owned typed Interact action installed by the local focus component only while one exact resource
     * instance is focused; Blueprint may display its glyph, null/load failure disables contextual input without
     * mutation, and no hardcoded key or asset path is used.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Focus|Input")
    TSoftObjectPtr<UInputAction> ContextInteractAction;

    /**
     * Settings-owned typed mapping context installed locally at higher priority only during exact resource focus;
     * Blueprint may inspect it, null/load failure disables the contextual mapping without gameplay mutation, and no
     * hardcoded key or asset path is used.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Focus|Input")
    TSoftObjectPtr<UInputMappingContext> ContextMappingContext;

    /**
     * Client-owned priority used while installing ContextMappingContext for exact resource focus; it has local input
     * side effects only, nonpositive values fail validation, and units are Enhanced Input mapping-priority steps.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Focus|Input", meta = (ClampMin = "1"))
    int32 ContextMappingPriority = 100;

    /**
     * Client-owned interval between cheap local focus scans; the server never trusts this cadence, nonpositive or
     * nonfinite values fail validation, and units are seconds.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Focus", meta = (ClampMin = "0.01", Units = "s"))
    float FocusScanIntervalSeconds = 0.1f;

    /**
     * Client-owned radius of the local focus sweep; it predicts presentation only, negative/nonfinite values fail
     * validation, and units are Unreal centimeters.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Focus", meta = (ClampMin = "0.0", Units = "cm"))
    float FocusRadiusCentimeters = 24.0f;

    /**
     * Client-owned maximum range for local resource focus; authority independently enforces its range, values above
     * AuthoritativeRangeCentimeters or nonpositive/nonfinite values fail validation, and units are Unreal centimeters.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Focus", meta = (ClampMin = "1.0", Units = "cm"))
    float FocusRangeCentimeters = 350.0f;

    /**
     * Server-owned side length used to assign changed nodes to spatial replication cells; clients receive the
     * resulting cells, invalid values fail validation, and units are Unreal centimeters.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Replication", meta = (ClampMin = "100.0", Units = "cm"))
    float ReplicationGridSizeCentimeters = 20000.0f;

    /**
     * Server-owned relevancy radius assigned to harvest replication cells; clients cannot widen it, invalid values
     * fail validation, and units are Unreal centimeters.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Replication", meta = (ClampMin = "100.0", Units = "cm"))
    float ReplicationCullDistanceCentimeters = 50000.0f;

    /**
     * Server-owned horizontal view/streaming margin beyond a grid cell's farthest corner; clients cannot alter it,
     * negative/nonfinite values fail validation, and units are Unreal centimeters.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Replication", meta = (ClampMin = "0.0", Units = "cm"))
    float ReplicationRelevancyMarginCentimeters = 5000.0f;

    /**
     * Deployment-owned maximum touched lifecycle rows admitted by one restore; Blueprint may inspect capacity only,
     * values above the native serialization ceiling fail validation, and units are node rows.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Replication|Restore", meta = (ClampMin = "1", ClampMax = "262144"))
    int32 RestoreMaximumTouchedNodes = 131072;

    /**
     * Deployment-owned maximum persistent contributors on one partial node; Blueprint may inspect capacity only,
     * values outside the native safety ceiling fail validation, and units are contributor rows per node.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Replication|Restore", meta = (ClampMin = "1", ClampMax = "128"))
    int32 RestoreMaximumContributorsPerNode = 64;

    /**
     * Deployment-owned maximum persistent contributor rows across one restore; Blueprint may inspect pressure only,
     * values outside the native safety ceiling fail validation, and units are contributor rows.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Replication|Restore", meta = (ClampMin = "1", ClampMax = "1048576"))
    int32 RestoreMaximumTotalContributors = 262144;

    /**
     * Deployment-owned maximum spatial replication proxies reserved by one restore; Blueprint may inspect pressure
     * only, values outside the native safety ceiling fail validation, and units are distinct XY grid cells.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Replication|Restore", meta = (ClampMin = "1", ClampMax = "65536"))
    int32 RestoreMaximumReplicationCells = 4096;

    /**
     * Deployment-owned signed cell-coordinate envelope accepted from saves; Blueprint may inspect it for diagnostics,
     * values outside the native safety ceiling fail validation, and units are grid-cell coordinates from world origin.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Replication|Restore", meta = (ClampMin = "1", ClampMax = "1000000"))
    int32 RestoreMaximumCellCoordinateMagnitude = 100000;

    /**
     * Server-owned minimum interval for coalescing equivalent owner feedback; clients only present accepted messages,
     * negative/nonfinite values fail validation, and units are seconds.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Feedback", meta = (ClampMin = "0.0", Units = "s"))
    float FeedbackRateLimitSeconds = 0.25f;

    /**
     * Server-owned interval between bounded retries of frozen undelivered harvest grants; clients may inspect it for
     * diagnostics only, nonpositive/nonfinite values fail validation, and units are seconds.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Rewards", meta = (ClampMin = "0.05", Units = "s"))
    float RewardOutboxRetryIntervalSeconds = 2.0f;

    /**
     * Server-owned maximum immutable grant rows attempted per retry interval; clients cannot alter delivery, values
     * outside [1,256] fail validation, and units are grant attempts per retry tick.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Rewards", meta = (ClampMin = "1", ClampMax = "256"))
    int32 RewardOutboxGrantBudget = 8;

    /**
     * Server-owned per-character ceiling for deterministic item entitlements waiting on inventory space; Blueprint
     * may inspect mailbox pressure only, values outside [1,4096] fail validation, and units are escrow rows.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Rewards|Persistence", meta = (ClampMin = "1", ClampMax = "4096"))
    int32 RewardItemEscrowMaximumRows = 2048;

    /**
     * Server-owned soft threshold that requests a durable world/character checkpoint for safe receipt compaction;
     * Blueprint may inspect operational pressure only, values outside [1,65536] fail validation, and units are rows.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Rewards|Persistence", meta = (ClampMin = "1", ClampMax = "65536"))
    int32 RewardReceiptCompactionThreshold = 2048;

    /**
     * Server-owned hard ceiling for one character's uncompacted harvest receipts; Blueprint may inspect the limit but
     * cannot evict rows, values outside [1,65536] or below the threshold fail validation, and units are receipt rows.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Rewards|Persistence", meta = (ClampMin = "1", ClampMax = "65536"))
    int32 RewardReceiptMaximumRows = 8192;

    /**
     * Server-owned coalescing window before a receipt-mutating character snapshot is issued; Blueprint may inspect
     * expected persistence latency only, negative/nonfinite values fail validation, and units are seconds.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Rewards|Persistence", meta = (ClampMin = "0.0", ClampMax = "2.0", Units = "s"))
    float RewardReceiptSaveDebounceSeconds = 0.1f;

    /**
     * Server-owned radius in which a living player can defer visible node restoration; clients cannot affect the
     * decision, negative/nonfinite values fail validation, and units are Unreal centimeters.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Respawn", meta = (ClampMin = "0.0", Units = "cm"))
    float RespawnVisibilityRadiusCentimeters = 1500.0f;

    /**
     * Server-owned delay before rechecking a visibility-deferred deadline; clients cannot schedule it, nonpositive or
     * nonfinite values fail validation, and units are seconds.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Respawn", meta = (ClampMin = "0.01", Units = "s"))
    float RespawnVisibilityRecheckSeconds = 1.0f;

    /**
     * Native-only canonical sanitizer for the live GAS work multiplier. Returns false and zeroes OutMultiplier when
     * the raw value or designer-owned bounds are nonfinite/invalid; otherwise returns the clamped unitless value.
     */
    bool TryClampHarvestWorkMultiplier(double RawMultiplier,
                                       double &OutMultiplier) const;

    bool AppendValidationErrors(TArray<FText> &OutErrors) const;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext &Context) const override;
#endif
};
