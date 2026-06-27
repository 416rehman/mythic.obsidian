// Mythic Living World — Actor Spawn Processor
// MASS processor that EMBODIES promoted (Tier 2 / cognitive) entities as real AMythicNPCCharacter actors.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "Mass/EntityHandle.h"
#include "ActorSpawnProcessor.generated.h"

class AMythicNPCCharacter;
class AMythicCreatureCharacter;
class UMythicLivingWorldSettings;

/**
 * Server-only MASS processor that turns promoted entities into real, AI-controllable actors — the bridge that
 * makes the living-world simulation embodied. The SignificanceProcessor adds FMythicActorSpawnRequestTag when
 * an entity reaches Tier 2 (cognitive). This consumer, for each tagged entity:
 *   1. spawns an AMythicNPCCharacter at the entity's cell location,
 *   2. binds the actor to its source entity via InitializeFromMassEntity (faction / personality / brain),
 *   3. removes the request tag (so it isn't re-spawned) and marks the entity embodied (FMythicCognitiveTag).
 *
 * The spawned NPC is faction-aware once embodied and becomes combat-capable on its own InitializeASC path
 * (AMythicNPCCharacter::CombatInit grants the attack ability + applies the NPC class's DefaultGameplayEffects
 * stat baseline — embodied NPCs carry no NPCData.Proficiencies, so that class baseline is their stat source).
 * Dehydration (despawn on de-significance, below) recycles the actor when the entity drops below Tier 2.
 * AI possession is wired via AMythicNPCCharacter's default AIControllerClass (the concrete AMythicNPCAIController)
 * + AutoPossessAI, so an embodied NPC perceives + moves on spawn. The embodied VISUAL class is resolved from
 * UMythicLivingWorldSettings::EmbodiedNPCClass (a mesh-bearing NPC Blueprint) when set, else SpawnActorClass, else
 * the bare C++ AMythicNPCCharacter. See Docs/BACKLOG.md.
 */
UCLASS()
class MYTHIC_API UMythicActorSpawnProcessor : public UMassProcessor {
    GENERATED_BODY()

public:
    UMythicActorSpawnProcessor();

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

    // Fallback embodied-NPC class, used when UMythicLivingWorldSettings::EmbodiedNPCClass is unset. Point either at a
    // mesh-bearing NPC Blueprint; possession is inherited from AMythicNPCCharacter's default controller, so no
    // Blueprint wiring is required for the NPC to perceive + move.
    UPROPERTY(EditAnywhere, Category = "Living World|Spawn")
    TSubclassOf<AMythicNPCCharacter> SpawnActorClass;

    // Fallback embodied-CREATURE class, used when UMythicLivingWorldSettings::EmbodiedCreatureClass is unset AND a
    // creature is being embodied. Defaults to the bare C++ AMythicCreatureCharacter (AI-possessed, mesh-less). Point at
    // a mesh-bearing creature Blueprint to give wildlife a body.
    UPROPERTY(EditAnywhere, Category = "Living World|Spawn")
    TSubclassOf<AMythicCreatureCharacter> SpawnCreatureClass;

private:
    // Resolved embodied-NPC class (Settings->EmbodiedNPCClass if set, else SpawnActorClass, else the C++ class),
    // cached on first use so the soft class isn't re-resolved — and the one-time BP load isn't repeated — per spawn.
    UPROPERTY(Transient)
    TSubclassOf<AMythicNPCCharacter> ResolvedSpawnClass;

    // Resolved embodied-CREATURE class (Settings->EmbodiedCreatureClass if set, else SpawnCreatureClass, else the C++
    // AMythicCreatureCharacter), cached on first use. May resolve to null ONLY if EmbodiedCreatureClass is explicitly
    // set to a soft class that fails to load — in that case the creature spawn branch consumes the request tag and the
    // entity stays MASS-only (no actor). bCreatureClassResolved latches "we tried and got nothing" so the resolve
    // isn't retried every tick.
    UPROPERTY(Transient)
    TSubclassOf<AMythicCreatureCharacter> ResolvedCreatureClass;

    // Latches that the one-time creature-class resolve has run (so a deliberately-null EmbodiedCreatureClass doesn't
    // re-trigger a LoadSynchronous attempt every Execute).
    bool bCreatureClassResolved = false;

    // Humanoid spawn requests: Identity + FMythicActorSpawnRequestTag, EXCLUDING creatures (FMythicCreatureTag None) so
    // a creature never hits the humanoid branch and double-embodies.
    FMassEntityQuery SpawnRequestQuery;

    // Creature spawn requests: Identity + FMythicActorSpawnRequestTag + FMythicCreatureTag (All). Routes creatures to
    // the AMythicCreatureCharacter branch.
    FMassEntityQuery CreatureSpawnRequestQuery;

    // Embodied entities (FMythicCognitiveTag) that requested their actor be despawned (de-promotion). Type-agnostic:
    // creatures and humanoids both register into the same AMythicNPCCharacter* map, so ONE despawn query handles both.
    FMassEntityQuery DespawnRequestQuery;

    // ─── Placement-defer cooldown (embodiment-service-LOCK-v1 §4: no infinite spawn-retry loop) ───
    // When MythicPlacement::FindValidSpawn fails for an entity (no navmesh-valid, reachable, non-overlapping,
    // non-water spot within SpawnRetryBudget candidates), we RE-ADD the spawn-request tag AND stamp this map with
    // Now + SpawnDeferCooldownSeconds. The spawn loops SKIP any entity whose DeferUntil > Now, so a failing cell is
    // retried at most once per cooldown window — never every frame (no hitch, no busy-loop). Game-thread only (this
    // processor is bRequiresGameThreadExecution=true), so no lock. NOT a UPROPERTY (FMassEntityHandle isn't a
    // UPROPERTY-compatible key). Pruned of entities no longer requesting a spawn each Execute (bounded growth).
    TMap<FMassEntityHandle, double> SpawnDeferUntil;

    /**
     * Shared spawn-validity + per-tick budget helper for ONE gathered request. Builds the FMythicPlacementParams from
     * the frozen settings contract (capsule dims from ResolvedClass's CDO), charges ONE validation against
     * ValidationsThisTick, and calls MythicPlacement::FindValidSpawn. Returns true + fills OutTransform on success
     * (clearing any stale defer stamp). Returns false when no valid candidate was found within SpawnRetryBudget — and
     * stamps SpawnDeferUntil[Entity] = Now + SpawnDeferCooldownSeconds so the caller's loop skips this entity until the
     * cooldown elapses (the caller KEEPS the spawn-request tag on a false return). Caller must check the per-tick budget
     * BEFORE calling. Centralizes the params build + budget charge + defer stamp so the humanoid and creature loops stay
     * identical.
     */
    bool TryFindSpawnTransform(
        UWorld *World,
        const UMythicLivingWorldSettings *Settings,
        const FVector &CellCenterXY,
        UClass *ResolvedClass,
        bool bWaterCapable,
        double Now,
        int32 &ValidationsThisTick,
        const FMassEntityHandle &Entity,
        FTransform &OutTransform);

    /**
     * no-vanish-in-front despawn gate (embodiment-service-LOCK-v1 §1.3 rule 5: never DESPAWN an in-view actor).
     * Returns true if Actor is within ViewGateMinSpawnDistance of, AND inside the (margin-widened) view cone of, ANY
     * local player camera — in which case the despawn must be skipped this tick and re-deferred. Self-contained (reads
     * the same ViewGateMinSpawnDistance / ViewConeMarginDeg the SignificanceProcessor spawn-gate reads — the locked
     * single source of truth for the distance) so this consumer does not hard-depend on the SignificanceProcessor view
     * helper. Game-thread only (PlayerCameraManager reads). No-op (returns false) when bViewGateEmbodiment is false.
     */
    static bool IsActorInCloseView(UWorld *World, const AMythicNPCCharacter *Actor, const UMythicLivingWorldSettings *Settings);
};
