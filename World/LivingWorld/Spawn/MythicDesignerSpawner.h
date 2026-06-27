// Mythic Living World — Designer Conditional / Stateful Spawner (placed actor)
//
// A designer drops AMythicDesignerSpawner in a level. On the SERVER, it periodically (EvaluationIntervalSeconds)
// evaluates an authored FMythicDesignerConditionSet (time-of-day, player-tag/proximity, faction lifecycle state,
// faction relation) and, when all gates pass and the count/cooldown/concurrency caps allow, spawns an
// AMythicNPCCharacter at the placed transform (or a navmesh-validated point via MythicPlacement).
//
// PERSISTENCE: a per-DesignerId MaxSpawnsEver counter + perma-death flag live in UMythicDesignerSpawnerRegistry (owned
// by the LWS, serialized in the save blob). The registry is AUTHORITATIVE and survives placed-actor path-name churn;
// the actor mirrors it on BeginPlay. Perma-death also routes through UMythicPersistentNPCRegistry so the dead NPC's
// stable NameHash can never be re-issued.
//
// MP: 100% server-authoritative. The spawner itself does NOT replicate (bReplicates=false) — only the spawned
// AMythicNPCCharacter replicates, via its own replication. All logic is gated on HasAuthority().
//
// BOUNDED / NO-HITCH: timer-throttled (no per-frame Tick); LiveNPCs <= MaxConcurrent; O(players) gather; the timer
// self-terminates once the spawner reaches MaxSpawnsEver or perma-death.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/LivingWorld/Spawn/DesignerSpawnerTypes.h"
#include "MythicDesignerSpawner.generated.h"

class AMythicNPCCharacter;
class UMythicLivingWorldSubsystem;
class UMythicDesignerSpawnerRegistry;

UCLASS(Blueprintable)
class MYTHIC_API AMythicDesignerSpawner : public AActor {
    GENERATED_BODY()

public:
    AMythicDesignerSpawner();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // ─── Designer-authored configuration ──────────────────

    /** Stable, designer-authored identity. Persistence key + perma-death identity seed. MUST be non-None. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Designer Spawner")
    FName DesignerId;

    /** The NPC class to spawn. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Designer Spawner")
    TSubclassOf<AMythicNPCCharacter> NPCClass;

    /** The conditions that must all be met to spawn. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Designer Spawner")
    FMythicDesignerConditionSet Conditions;

    /** Maximum NPCs this spawner will EVER spawn across the save's lifetime (persisted). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Designer Spawner", meta = (ClampMin = "0"))
    int32 MaxSpawnsEver = 1;

    /** Minimum seconds after one of this spawner's NPCs dies before another may spawn. 0 = no cooldown. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Designer Spawner", meta = (ClampMin = "0.0"))
    float RespawnCooldownSeconds = 0.0f;

    /** Maximum NPCs from this spawner that may be alive at once. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Designer Spawner", meta = (ClampMin = "1"))
    int32 MaxConcurrent = 1;

    /** How often (seconds) to re-evaluate conditions. Timer-throttled (no per-frame Tick). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Designer Spawner", meta = (ClampMin = "0.1"))
    float EvaluationIntervalSeconds = 2.0f;

    /** When true, spawn exactly at the placed actor transform; else navmesh-validate via MythicPlacement::FindValidSpawn. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Designer Spawner")
    bool bUseExactPlacedTransform = true;

    /** When true, the last spawn (at MaxSpawnsEver) becomes a perma-death identity once it dies. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Designer Spawner")
    bool bMarkPermaDeadOnDeath = true;

    // ─── Public read API (Blueprint observability + debugger) ──────────────────

    /** True if conditions currently evaluate to met (re-gathers inputs; server-only meaningful). */
    UFUNCTION(BlueprintPure, Category = "Designer Spawner")
    bool AreConditionsMet() const;

    /** Total NPCs ever spawned by this DesignerId (mirror of the registry). */
    UFUNCTION(BlueprintPure, Category = "Designer Spawner")
    int32 GetSpawnsEver() const { return CachedSpawnsEver; }

    /** True if this spawner has reached its terminal perma-death state. */
    UFUNCTION(BlueprintPure, Category = "Designer Spawner")
    bool IsPermaDead() const { return bCachedPermaDead; }

    /** Number of this spawner's NPCs currently alive. */
    UFUNCTION(BlueprintPure, Category = "Designer Spawner")
    int32 GetLiveCount() const;

private:
    /** Periodic condition evaluation (game-thread timer body). */
    void TickEvaluate();

    /** Drop dead/stale entries from LiveNPCs (and their cached name hashes). */
    void ReapLiveNPCs();

    /** Resolve the LWS subsystem (game instance). Null if unavailable. */
    UMythicLivingWorldSubsystem *GetLWS() const;

    /** Gather the live engine/sim inputs for the pure evaluator. */
    bool GatherInputs(FMythicDesignerConditionInputs &OutInputs) const;

    /** Current game hour in [0,24) from the environment clock (fallback: world time mod day length). */
    float GetCurrentGameHour() const;

    /** True if at least one player satisfies the combined player gate (required tags AND/OR proximity). */
    bool AnyPlayerSatisfiesPlayerGate() const;

    /** Perform a spawn (placement, bind death, identity hash, registry bookkeeping). */
    void SpawnNPC();

    /** SERVER: a spawned NPC died — update cooldown, record death + (terminal) perma-death. Matches FMythicOnDeath. */
    UFUNCTION()
    void OnDesignerNPCDeath(AActor *DeadActor);

    /** Stop the eval timer (terminal state reached). */
    void StopEvaluation();

    // ─── Runtime state (server-only) ──────────────────

    /** Live NPCs spawned by this spawner. */
    TArray<TWeakObjectPtr<AMythicNPCCharacter>> LiveNPCs;

    /** Stable NameHash per live NPC (for the perma-death death record on its eventual death). */
    TMap<TWeakObjectPtr<AMythicNPCCharacter>, uint32> LiveNameHashes;

    FTimerHandle EvalTimerHandle;

    /** Mirror of the registry state (also drives Blueprint getters + the debugger). */
    int32 CachedSpawnsEver = 0;
    bool bCachedPermaDead = false;
    double CachedLastDeathTime = 0.0;
};
