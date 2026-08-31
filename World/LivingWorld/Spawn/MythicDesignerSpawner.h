
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/Entity/MythicEntityId.h"
#include "World/LivingWorld/Spawn/DesignerSpawnerTypes.h"
#include "MythicDesignerSpawner.generated.h"

class AMythicNPCCharacter;
class UMythicLivingWorldSubsystem;
class UMythicDesignerSpawnerRegistry;
class UMythicEntityIdentityDefinition;

UCLASS(Blueprintable)
class MYTHIC_API AMythicDesignerSpawner : public AActor {
    GENERATED_BODY()

public:
    AMythicDesignerSpawner();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


    /** Stable key for this spawner's counters and conditions; it is never used as the spawned NPC's identity. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Designer Spawner")
    FName DesignerId;

    /** The NPC class to spawn. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Designer Spawner")
    TSubclassOf<AMythicNPCCharacter> NPCClass;

    /**
     * Explicit public cover/role identity for spawned characters. It is independent of private NPC type and spawn
     * gates; empty intentionally presents a generic stranger.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Designer Spawner|Presentation")
    TSoftObjectPtr<UMythicEntityIdentityDefinition> PublicIdentityDefinition;

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

    /** Clears old live NPC handles before an in-place LivingWorld restore replaces canonical identity state. */
    void BeginLivingWorldRestore();

    /** Reloads persisted counters and resumes evaluation after the LivingWorld restore transaction completes. */
    void CompleteLivingWorldRestore();

private:
    void TickEvaluate();

    void ReapLiveNPCs();

    UMythicLivingWorldSubsystem *GetLWS() const;

    bool GatherInputs(FMythicDesignerConditionInputs &OutInputs) const;

    float GetCurrentGameHour() const;

    bool AnyPlayerSatisfiesPlayerGate() const;

    void SpawnNPC();

    UFUNCTION()
    void OnDesignerNPCDeath(AActor *DeadActor);

    void StopEvaluation();


    TArray<TWeakObjectPtr<AMythicNPCCharacter>> LiveNPCs;

    TMap<TWeakObjectPtr<AMythicNPCCharacter>, FMythicEntityId> LiveEntityIds;

    FTimerHandle EvalTimerHandle;

    int32 CachedSpawnsEver = 0;
    bool bCachedPermaDead = false;
    double CachedLastDeathTime = 0.0;
};
