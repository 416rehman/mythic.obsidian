// Mythic Living World — Population Spawner Processor Implementation

#include "Mass/Processors/PopulationSpawnerProcessor.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "MassCommandBuffer.h"
#include "MassCommands.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Mass/Tags/MythicMassTags.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "AI/Party/PartySubsystem.h" // companion far-despawn exemption
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Settlements/SettlementRegistry.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "World/LivingWorld/NPCGeneration/NPCGenerator.h"
#include "AI/NPCs/MythicNPCCharacter.h" // despawn tears down any embodied actor before freeing its entity
#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h" // R18-M7: global spawn-serial allocator (collision-free identity)
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

UMythicPopulationSpawnerProcessor::UMythicPopulationSpawnerProcessor() {
    // Run during PrePhysics on server/standalone only — clients receive replicated state eventually
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
    bRequiresGameThreadExecution = true;
    bAutoRegisterWithProcessingPhases = true;

    // This processor spawns the initial NPC entities — it must run even when zero NPC
    // archetypes exist. Without this, MASS prunes it at startup (chicken-and-egg deadlock).
    QueryBasedPruning = EMassQueryBasedPruning::Never;

    // Register queries in constructor so CallInitialize can bind the entity manager before ConfigureQueries
    ExistingNPCQuery.RegisterWithProcessor(*this);
}

void UMythicPopulationSpawnerProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    // Query existing NPC entities by Identity + Significance fragments + NPC tag
    ExistingNPCQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    ExistingNPCQuery.AddRequirement<FMythicSignificanceFragment>(EMassFragmentAccess::ReadOnly);
    ExistingNPCQuery.AddTagRequirement<FMythicNPCTag>(EMassFragmentPresence::All);
    // Encounter-owned entities are managed solely by the EncounterDirector — exclude them from BOTH this query's uses
    // (the per-cell density count + the far-from-player despawn) so the population system never counts or destroys them.
    ExistingNPCQuery.AddTagRequirement<FMythicEncounterEntityTag>(EMassFragmentPresence::None);
}

void UMythicPopulationSpawnerProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicPopulationSpawner_Execute);

    // Get the living world subsystem
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

    const UMythicLivingWorldSettings *Settings = LWS->GetSettings();
    if (!Settings) {
        return;
    }

    // Party companions are EXEMPT from the far-from-player despawn below (Step 4) — a companion follows its player out
    // of its spawn zone, and its Identity.Cell is frozen at spawn, so it would otherwise be culled the moment the
    // player travels > PopulationDespawnDistance cells (leaking an unrefillable party slot). Game-thread-safe query
    // (bRequiresGameThreadExecution), mirroring the SignificanceProcessor exemption.
    const UMythicPartySubsystem *PartySubsystem = World->GetSubsystem<UMythicPartySubsystem>();

    // Throttle: only run at configured interval
    TimeSinceLastTick += Context.GetDeltaTimeSeconds();
    if (TimeSinceLastTick < Settings->PopulationSpawnIntervalSeconds) {
        return;
    }
    TimeSinceLastTick = 0.0f;

    UE_LOG(LogMythLivingWorld, Verbose, TEXT("PopulationSpawner: tick fired (interval %.1fs)"),
           Settings->PopulationSpawnIntervalSeconds);

    UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid();
    UMythicSettlementRegistry *Registry = LWS->GetSettlementRegistry();
    UMythicFactionDatabase *FactionDB = LWS->GetFactionDatabase();

    if (!Grid || !Registry || !FactionDB) {
        return;
    }

    // R18-M7: the persistent NPC registry owns the global spawn-serial allocator that guarantees collision-free
    // NameHash identity. Without it we cannot guarantee unique identities — do not spawn aliasable NPCs.
    UMythicPersistentNPCRegistry *PersistentRegistry = LWS->GetPersistentNPCRegistry();
    if (!PersistentRegistry) {
        return;
    }

    // ─── Step 1: Gather player positions ───────────────

    TArray<FMythicCellCoord> PlayerCells;
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It) {
        if (const APlayerController *PC = It->Get()) {
            if (const APawn *Pawn = PC->GetPawn()) {
                PlayerCells.Add(Grid->WorldToCell(Pawn->GetActorLocation()));
            }
        }
    }

    if (PlayerCells.IsEmpty()) {
        UE_LOG(LogMythLivingWorld, Verbose, TEXT("PopulationSpawner: no player pawns found, skipping."));
        return;
    }

    UE_LOG(LogMythLivingWorld, Verbose, TEXT("PopulationSpawner: %d player(s) detected."), PlayerCells.Num());

    // ─── Step 2: Count existing entities per cell ──────

    TMap<FMythicCellCoord, int32> CellEntityCounts;
    ExistingNPCQuery.ForEachEntityChunk(Context, [&CellEntityCounts](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();

        for (int32 i = 0; i < NumEntities; ++i) {
            CellEntityCounts.FindOrAdd(IdentityView[i].Cell)++;
        }
    });

    // ─── Step 3: Determine cells needing spawns ────────

    const float SpawnRadiusSq = FMath::Square(Settings->PopulationSpawnRadius);
    const int32 SpawnRadiusCells = FMath::CeilToInt(Settings->PopulationSpawnRadius);
    int32 SpawnBudget = Settings->MaxSpawnsPerTick;

    struct FMythicNPCPopulationSpawnData {
        FMythicIdentityFragment Identity;
        FMythicScheduleFragment Schedule;
        FMythicSignificanceFragment Significance;
    };
    TArray<FMythicNPCPopulationSpawnData> SpawnDataArray;
    SpawnDataArray.Reserve(SpawnBudget);

    // Collect unique cells within spawn radius of any player
    TSet<FMythicCellCoord> CellsToPopulate;
    for (const FMythicCellCoord &PlayerCell : PlayerCells) {
        for (int32 DY = -SpawnRadiusCells; DY <= SpawnRadiusCells && SpawnBudget > 0; ++DY) {
            for (int32 DX = -SpawnRadiusCells; DX <= SpawnRadiusCells && SpawnBudget > 0; ++DX) {
                // Circular check
                if ((DX * DX + DY * DY) > SpawnRadiusSq) {
                    continue;
                }

                const FMythicCellCoord CandidateCell(PlayerCell.X + DX, PlayerCell.Y + DY);
                if (!Grid->IsValidCoord(CandidateCell)) {
                    continue;
                }

                // Only populate cells that belong to a settlement. Snapshot it under SimulationLock (copy-out) — NEVER
                // hold a live Settlements-map pointer on this game thread: the sim thread writes GoverningFaction during
                // a conquest TransferSettlement under that lock, so an unlocked read here tears (NPCs stamped to the
                // wrong faction) or dangles across a RegisterSettlement rehash.
                FMythicSettlementData Settlement;
                if (!LWS->CopySettlementAtCell(CandidateCell, Settlement) || !Settlement.GoverningFaction.IsValid()) {
                    continue;
                }

                if (CellsToPopulate.Contains(CandidateCell)) {
                    continue;
                }
                CellsToPopulate.Add(CandidateCell);

                FMythicFactionData FactionData;
                if (!FactionDB->GetFaction(Settlement.GoverningFaction, FactionData)) {
                    continue;
                }

                // Capacity = ControlledCellCount × PopulationPerCell (same formula as WorldSimThread)
                const int32 FactionCapacity = FactionData.ControlledCellCount * Settings->PopulationPerCell;

                const int32 TargetCount = ComputeTargetDensity(
                    Settlement.MaxPopulationDensity,
                    Settings->MaxEntitiesPerCell,
                    FactionData.Population,
                    FactionCapacity
                    );

                const int32 CurrentCount = CellEntityCounts.FindRef(CandidateCell);
                const int32 Deficit = TargetCount - CurrentCount;

                if (Deficit <= 0) {
                    continue;
                }

                // Spawn entities to fill the deficit (budget-capped)
                const int32 ToSpawn = FMath::Min(Deficit, SpawnBudget);

                for (int32 SpawnIdx = 0; SpawnIdx < ToSpawn; ++SpawnIdx) {
                    FMythicNPCPopulationSpawnData SpawnData;
                    SpawnData.Identity.Faction = Settlement.GoverningFaction;
                    SpawnData.Identity.Cell = CandidateCell;

                    // ─── Full NPC Generation Pipeline ───
                    // Identity is GLOBALLY UNIQUE (R18-M7), not reproducible-from-cell: feed a PERSISTED, never-reused
                    // monotonic serial — NOT the wave-local SpawnIdx, which restarted at 0 each refill wave so two waves
                    // in the same faction+cell aliased one NameHash, poisoning the perma-death registry (a collision
                    // permanently zombie-locks a living NPC + misattributes death/chronicle events). Mirrors
                    // AMythicEncounterDirector's NextEncounterId. The serial still feeds the same Wang-hash mix, so the
                    // hash stays a varied, decodable value (name/demographics/archetype render unchanged).
                    const int32 SpawnSerial = PersistentRegistry->AllocateSpawnSerial();

                    SpawnData.Identity.NameHash = FMythicNPCGenerator::GenerateNameHash(
                        Settlement.GoverningFaction.Index, CandidateCell, SpawnSerial);

                    SpawnData.Identity.VisualArchetype = FMythicNPCGenerator::GenerateVisualArchetype(
                        SpawnData.Identity.NameHash, 8);

                    SpawnData.Identity.DemographicFlags = FMythicNPCGenerator::GenerateDemographicFlags(
                        SpawnData.Identity.NameHash, FactionData.Population > 50);

                    // ─── Schedule — work cell offset by hash for spatial variety ───
                    SpawnData.Schedule.Phase = EMythicSchedulePhase::Idle;
                    SpawnData.Schedule.HomeCell = CandidateCell;
                    SpawnData.Schedule.WorkCell = FMythicCellCoord(
                        CandidateCell.X + static_cast<int32>((SpawnData.Identity.NameHash >> 4) % 3) - 1,
                        CandidateCell.Y + static_cast<int32>((SpawnData.Identity.NameHash >> 8) % 3) - 1);

                    SpawnData.Significance.Tier = EMythicSignificanceTier::Tier0_Ambient;

                    // Note: Personality (VentWeights) is NOT generated here for Tier 0.
                    // Tier 0 entities have no PersonalityFragment — it's added on promotion
                    // to Tier 1 by the SignificanceProcessor. At that point, the personality
                    // is generated from the NameHash + faction ideology via NPCGenerator.
                    // This ensures:
                    //   1. Zero CPU for Tier 0 personality (no wasted generation)
                    //   2. Ideology-dirty respawns automatically get new ideology
                    //   3. Deterministic: same hash + faction = same personality

                    SpawnDataArray.Add(SpawnData);
                }

                SpawnBudget -= ToSpawn;

                UE_LOG(LogMythLivingWorld, Verbose, TEXT("PopulationSpawner: cell (%d,%d) — target=%d, current=%d, spawned=%d (faction=%d, pop=%d, cap=%d)"),
                       CandidateCell.X, CandidateCell.Y, TargetCount, CurrentCount, ToSpawn,
                       Settlement.GoverningFaction.Index, FactionData.Population, FactionCapacity);
            }
        }
    }

    if (SpawnDataArray.Num() > 0) {
        Context.Defer().PushCommand<FMassDeferredCreateCommand>([SpawnDataArray](FMassEntityManager &Manager) {
            TRACE_CPUPROFILER_EVENT_SCOPE(MythicPopulationSpawner_DeferredSpawn);

            const UScriptStruct *Composition[] = {
                FMythicIdentityFragment::StaticStruct(),
                FMythicScheduleFragment::StaticStruct(),
                FMythicSignificanceFragment::StaticStruct(),
                FMythicNPCTag::StaticStruct()
            };
            FMassArchetypeHandle Archetype = Manager.CreateArchetype(MakeArrayView(Composition));

            TArray<FMassEntityHandle> SpawnedEntities;
            Manager.BatchCreateEntities(Archetype, SpawnDataArray.Num(), SpawnedEntities);

            for (int32 i = 0; i < SpawnDataArray.Num(); ++i) {
                const FMythicNPCPopulationSpawnData &Data = SpawnDataArray[i];
                FMassEntityHandle Entity = SpawnedEntities[i];

                Manager.GetFragmentDataChecked<FMythicIdentityFragment>(Entity) = Data.Identity;
                Manager.GetFragmentDataChecked<FMythicScheduleFragment>(Entity) = Data.Schedule;
                Manager.GetFragmentDataChecked<FMythicSignificanceFragment>(Entity) = Data.Significance;
            }
        });
    }

    // ─── Step 4: Despawn entities too far from all players ──

    const float DespawnRadiusSq = FMath::Square(Settings->PopulationDespawnDistance);
    int32 DespawnBudget = Settings->MaxDespawnsPerTick;

    ExistingNPCQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext &ChunkContext) {
        if (DespawnBudget <= 0) {
            return;
        }

        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();

        for (int32 i = 0; i < NumEntities && DespawnBudget > 0; ++i) {
            const FMythicCellCoord &EntityCell = IdentityView[i].Cell;

            // Check if any player is within despawn distance
            bool bNearPlayer = false;
            for (const FMythicCellCoord &PlayerCell : PlayerCells) {
                const float DistSq = static_cast<float>(
                    FMath::Square(EntityCell.X - PlayerCell.X) + FMath::Square(EntityCell.Y - PlayerCell.Y)
                );
                if (DistSq <= DespawnRadiusSq) {
                    bNearPlayer = true;
                    break;
                }
            }

            if (!bNearPlayer) {
                const FMassEntityHandle DespawnEntity = ChunkContext.GetEntity(i);
                // Never far-despawn a party companion (it follows its player; its entity cell is frozen at spawn).
                if (PartySubsystem && PartySubsystem->IsCompanionEntity(DespawnEntity)) {
                    continue;
                }
                // Tear down any embodied cognitive actor FIRST. A Tier2-promoted NPC that drifts into the despawn band
                // before the significance pass demotes it still has an AMythicNPCCharacter in EmbodiedActors; a bare
                // DestroyEntity would orphan that actor (dangling CognitiveBrain SourceEntity + live think timer) and
                // leak the EmbodiedActors[Entity] key — the same hole the EncounterDirector cleanup fixed on its path.
                // Safe to call directly: this processor is bRequiresGameThreadExecution (ctor), so we are on the game
                // thread; LWS is captured from Execute.
                if (AMythicNPCCharacter *Actor = LWS->FindEmbodiedActor(DespawnEntity)) {
                    Actor->Destroy();
                }
                LWS->UnregisterEmbodiedActor(DespawnEntity); // idempotent — also clears a stale key if no actor
                Context.Defer().DestroyEntity(DespawnEntity);
                --DespawnBudget;
            }
        }
    });
}

int32 UMythicPopulationSpawnerProcessor::ComputeTargetDensity(
    int32 SettlementMaxDensity,
    int32 SystemMaxPerCell,
    int32 FactionPopulation,
    int32 FactionCapacity) const {
    // Cap by system limit
    const int32 EffectiveMax = FMath::Min(SettlementMaxDensity, SystemMaxPerCell);

    // Scale by faction health (population / capacity)
    if (FactionCapacity <= 0) {
        return 0;
    }

    const float FillRatio = FMath::Clamp(static_cast<float>(FactionPopulation) / static_cast<float>(FactionCapacity), 0.0f, 1.0f);
    return FMath::CeilToInt(EffectiveMax * FillRatio);
}
