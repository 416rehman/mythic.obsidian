// Mythic Living World — Actor Spawn Processor
// MASS processor that EMBODIES promoted (Tier 2 / cognitive) entities as real AMythicNPCCharacter actors.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "ActorSpawnProcessor.generated.h"

class AMythicNPCCharacter;

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
 * Still deferred: AI possession (point SpawnActorClass at a Blueprint that wires AIControllerClass +
 * AutoPossessAI — the C++ AMythicAIController is Abstract), so the NPC is attack-capable but won't autonomously
 * engage until possessed. See Docs/BACKLOG.md.
 */
UCLASS()
class MYTHIC_API UMythicActorSpawnProcessor : public UMassProcessor {
    GENERATED_BODY()

public:
    UMythicActorSpawnProcessor();

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

    // The actor class to spawn. Point this at the NPC Blueprint that wires AIControllerClass + AutoPossessAI so
    // the spawned NPC is AI-possessed (the C++ AMythicAIController is Abstract; possession is Blueprint-driven).
    UPROPERTY(EditAnywhere, Category = "Living World|Spawn")
    TSubclassOf<AMythicNPCCharacter> SpawnActorClass;

private:
    FMassEntityQuery SpawnRequestQuery;

    // Embodied entities (FMythicCognitiveTag) that requested their actor be despawned (de-promotion).
    FMassEntityQuery DespawnRequestQuery;
};
