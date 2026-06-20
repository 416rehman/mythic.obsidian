// Mythic Living World — Actor Spawn Processor Implementation

#include "Mass/Processors/ActorSpawnProcessor.h"
#include "MassExecutionContext.h"
#include "MassCommandBuffer.h"
#include "MassCommands.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Mass/Tags/MythicMassTags.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "Engine/World.h"

UMythicActorSpawnProcessor::UMythicActorSpawnProcessor() {
    // Server/standalone only, game thread (spawns actors). Mirrors UMythicPopulationSpawnerProcessor.
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
    bRequiresGameThreadExecution = true;
    bAutoRegisterWithProcessingPhases = true;

    // The spawn-request archetype may not exist at startup; don't let MASS prune this processor.
    QueryBasedPruning = EMassQueryBasedPruning::Never;

    SpawnActorClass = AMythicNPCCharacter::StaticClass();
    SpawnRequestQuery.RegisterWithProcessor(*this);
    DespawnRequestQuery.RegisterWithProcessor(*this);
}

void UMythicActorSpawnProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    SpawnRequestQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    SpawnRequestQuery.AddTagRequirement<FMythicActorSpawnRequestTag>(EMassFragmentPresence::All);

    // Embodied entities that asked to despawn (de-promoted below Tier 2).
    DespawnRequestQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    DespawnRequestQuery.AddTagRequirement<FMythicCognitiveTag>(EMassFragmentPresence::All);
    DespawnRequestQuery.AddTagRequirement<FMythicActorDespawnRequestTag>(EMassFragmentPresence::All);
}

void UMythicActorSpawnProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicActorSpawn_Execute);

    UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    UGameInstance *GI = World->GetGameInstance();
    if (!GI) {
        return;
    }
    UMythicLivingWorldSubsystem *LWS = GI->GetSubsystem<UMythicLivingWorldSubsystem>();
    if (!LWS || !LWS->IsSystemActive()) {
        return;
    }
    UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid();
    if (!Grid) {
        return;
    }
    UMythicPersistentNPCRegistry *Registry = LWS->GetPersistentNPCRegistry();

    // Collect requests first. Spawning actors while iterating the entity chunks is unsafe, so gather (entity,
    // identity) pairs in the query, then spawn after the iteration completes.
    TArray<TPair<FMassEntityHandle, FMythicIdentityFragment>> Requests;
    SpawnRequestQuery.ForEachEntityChunk(Context, [&Requests](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();
        for (int32 i = 0; i < NumEntities; ++i) {
            Requests.Emplace(ChunkContext.GetEntity(i), IdentityView[i]);
        }
    });

    for (const TPair<FMassEntityHandle, FMythicIdentityFragment> &Req : Requests) {
        const FMassEntityHandle Entity = Req.Key;
        const FMythicIdentityFragment &Identity = Req.Value;

        // Always consume the request so the entity isn't re-visited next frame.
        Context.Defer().RemoveTag<FMythicActorSpawnRequestTag>(Entity);

        // Never embody a perma-dead NPC (the direct Tier0->Tier2 promotion path doesn't check this).
        if (Registry && Registry->IsPermaDead(Identity.NameHash)) {
            continue;
        }

        const FVector SpawnLoc = Grid->CellToWorld(Identity.Cell);
        FActorSpawnParameters SpawnInfo;
        SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        TSubclassOf<AMythicNPCCharacter> ClassToSpawn = SpawnActorClass;
        if (!ClassToSpawn) {
            ClassToSpawn = AMythicNPCCharacter::StaticClass();
        }
        AMythicNPCCharacter *NPC = World->SpawnActor<AMythicNPCCharacter>(ClassToSpawn, SpawnLoc, FRotator::ZeroRotator, SpawnInfo);
        if (!NPC) {
            continue;
        }

        // Bind the actor to its source entity (faction / personality / cognitive brain). First-ever caller of
        // InitializeFromMassEntity.
        NPC->InitializeFromMassEntity(Entity);

        // Record the entity->actor reverse link so dehydration can find + despawn this actor on de-promotion.
        LWS->RegisterEmbodiedActor(Entity, NPC);

        // Mark the entity embodied so it isn't re-spawned while an actor exists.
        Context.Defer().AddTag<FMythicCognitiveTag>(Entity);
    }

    // ─── Dehydration: despawn actors whose entity dropped below Tier 2 ───
    // Gather first (don't mutate during chunk iteration), then act.
    TArray<FMassEntityHandle> Despawns;
    DespawnRequestQuery.ForEachEntityChunk(Context, [&Despawns](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        for (int32 i = 0; i < NumEntities; ++i) {
            Despawns.Add(ChunkContext.GetEntity(i));
        }
    });

    for (const FMassEntityHandle Entity : Despawns) {
        // Always consume both tags so the entity isn't re-visited and becomes re-embodiable later.
        Context.Defer().RemoveTag<FMythicActorDespawnRequestTag>(Entity);
        Context.Defer().RemoveTag<FMythicCognitiveTag>(Entity);

        if (AMythicNPCCharacter *Actor = LWS->FindEmbodiedActor(Entity)) {
            Actor->Destroy();
        }
        LWS->UnregisterEmbodiedActor(Entity);
    }
}
