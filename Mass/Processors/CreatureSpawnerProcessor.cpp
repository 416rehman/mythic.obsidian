// Mythic Living World — Creature Spawner Processor Implementation

#include "Mass/Processors/CreatureSpawnerProcessor.h"
#include "MassExecutionContext.h"
#include "MassCommandBuffer.h"
#include "MassCommands.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Mass/Tags/MythicMassTags.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "World/LivingWorld/Creatures/CreatureSpeciesTypes.h"
#include "AI/NPCs/MythicNPCCharacter.h" // far-despawn tears down any embodied creature actor (IS-A NPC character)
#include "Engine/DataTable.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

UMythicCreatureSpawnerProcessor::UMythicCreatureSpawnerProcessor() {
    // Server/standalone, game thread (creates entities via the command buffer + reads sim-lock-guarded settlement
    // copies). Mirrors UMythicPopulationSpawnerProcessor's threading contract exactly.
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
    bRequiresGameThreadExecution = true;
    bAutoRegisterWithProcessingPhases = true;

    // The creature archetype may not exist at startup — keep this processor alive so it can create the first creatures.
    QueryBasedPruning = EMassQueryBasedPruning::Never;

    // Wilderness creatures fill the cells the patrol spawner leaves behind (true unowned land). Order after it so the
    // ownership predicates evaluate against a settled view, mirroring the spec's mutually-exclusive split.
    ExecutionOrder.ExecuteAfter.Add(TEXT("UMythicTerritoryPatrolSpawnerProcessor"));

    ExistingCreatureQuery.RegisterWithProcessor(*this);
}

void UMythicCreatureSpawnerProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    // Existing creatures, by Identity + Creature fragment + creature tag. Drives BOTH the per-cell density count and
    // the far-from-player despawn. Creatures are NOT counted by (or despawned by) the NPC population spawner because
    // they carry FMythicCreatureTag, not FMythicNPCTag — so this is the sole lifecycle authority for wildlife counts.
    ExistingCreatureQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    ExistingCreatureQuery.AddRequirement<FMythicCreatureFragment>(EMassFragmentAccess::ReadOnly);
    ExistingCreatureQuery.AddTagRequirement<FMythicCreatureTag>(EMassFragmentPresence::All);
}

uint16 UMythicCreatureSpawnerProcessor::AllocatePackId() {
    const uint16 Id = NextPackId++;
    if (NextPackId == 0) {
        NextPackId = 1; // 0 is reserved for "solitary"; skip it on wrap.
    }
    return Id;
}

int32 UMythicCreatureSpawnerProcessor::ComputeCreatureTargetDensity(int32 MaxCreaturesPerBiomeCell, float DensityScale, int32 SystemCap) {
    if (MaxCreaturesPerBiomeCell <= 0 || DensityScale <= 0.0f) {
        return 0;
    }
    const int32 Scaled = FMath::RoundToInt(static_cast<float>(MaxCreaturesPerBiomeCell) * DensityScale);
    return FMath::Clamp(Scaled, 0, FMath::Max(0, SystemCap));
}

void UMythicCreatureSpawnerProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicCreatureSpawner_Execute);

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

    // Throttle: only run at the configured interval.
    TimeSinceLastTick += Context.GetDeltaTimeSeconds();
    if (TimeSinceLastTick < Settings->CreatureSpawnIntervalSeconds) {
        return;
    }
    TimeSinceLastTick = 0.0f;

    UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid();
    if (!Grid) {
        return;
    }

    // ─── Resolve species rows ONCE per Execute, bucketed by biome (no per-cell loads/allocs) ───
    // Authored table wins; else the built-in code-default set so wildlife runs with zero authored data. We copy the
    // rows we need into a per-biome bucket array so the per-cell weighted pick is a flat index walk.
    TConstArrayView<FMythicCreatureSpeciesRow> CodeDefaults; // keeps the static backing array alive for the views below
    TArray<const FMythicCreatureSpeciesRow *> SpeciesByBiome[MythicBiomeCount];
    {
        // Authored DataTable path: GetAllRows returns pointers into the table's row storage, valid for this Execute
        // (the table is a hard-referenced UObject, not GC'd mid-frame). Code-default path: a function-local static
        // array, valid for the program lifetime.
        if (const UDataTable *Table = Settings->CreatureSpeciesTable.LoadSynchronous()) {
            TArray<FMythicCreatureSpeciesRow *> Rows;
            Table->GetAllRows<FMythicCreatureSpeciesRow>(TEXT("CreatureSpawner"), Rows);
            for (const FMythicCreatureSpeciesRow *Row : Rows) {
                if (!Row) {
                    continue;
                }
                const int32 BiomeIdx = static_cast<int32>(Row->Biome);
                if (BiomeIdx >= 0 && BiomeIdx < MythicBiomeCount) {
                    SpeciesByBiome[BiomeIdx].Add(Row);
                }
            }
        } else {
            CodeDefaults = MythicCreatureDefaults::GetCodeDefaultSpecies();
            for (const FMythicCreatureSpeciesRow &Row : CodeDefaults) {
                const int32 BiomeIdx = static_cast<int32>(Row.Biome);
                if (BiomeIdx >= 0 && BiomeIdx < MythicBiomeCount) {
                    SpeciesByBiome[BiomeIdx].Add(&Row);
                }
            }
        }
    }

    // ─── Gather player cells ───
    TArray<FMythicCellCoord> PlayerCells;
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It) {
        if (const APlayerController *PC = It->Get()) {
            if (const APawn *Pawn = PC->GetPawn()) {
                PlayerCells.Add(Grid->WorldToCell(Pawn->GetActorLocation()));
            }
        }
    }
    if (PlayerCells.IsEmpty()) {
        return;
    }

    // ─── Count existing creatures per cell ───
    TMap<FMythicCellCoord, int32> CellCreatureCounts;
    ExistingCreatureQuery.ForEachEntityChunk(Context, [&CellCreatureCounts](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();
        for (int32 i = 0; i < NumEntities; ++i) {
            CellCreatureCounts.FindOrAdd(IdentityView[i].Cell)++;
        }
    });

    // ─── Determine cells needing creatures ───
    const float SpawnRadius = Settings->CreatureSpawnRadius;
    const float SpawnRadiusSq = FMath::Square(SpawnRadius);
    const int32 SpawnRadiusCells = FMath::CeilToInt(SpawnRadius);
    int32 SpawnBudget = Settings->MaxCreatureSpawnsPerTick;

    // Per-creature spawn payload (mirrors PopulationSpawner's local struct discipline).
    struct FMythicCreatureSpawnData {
        FMythicIdentityFragment Identity;
        FMythicCreatureFragment Creature;
        FMythicSignificanceFragment Significance;
    };
    TArray<FMythicCreatureSpawnData> SpawnDataArray;
    SpawnDataArray.Reserve(SpawnBudget);

    TSet<FMythicCellCoord> ConsideredCells;
    for (const FMythicCellCoord &PlayerCell : PlayerCells) {
        for (int32 DY = -SpawnRadiusCells; DY <= SpawnRadiusCells && SpawnBudget > 0; ++DY) {
            for (int32 DX = -SpawnRadiusCells; DX <= SpawnRadiusCells && SpawnBudget > 0; ++DX) {
                if ((DX * DX + DY * DY) > SpawnRadiusSq) {
                    continue;
                }

                const FMythicCellCoord Cell(PlayerCell.X + DX, PlayerCell.Y + DY);
                if (!Grid->IsValidCoord(Cell)) {
                    continue;
                }

                // Dedup BEFORE the SimulationLock-guarded settlement copy (overlapping co-op spawn radii reach the same
                // cell once per nearby player). Mirrors the population spawner's dedup-before-lock pattern.
                if (ConsideredCells.Contains(Cell)) {
                    continue;
                }
                ConsideredCells.Add(Cell);

                // ── Ownership predicates: wilderness ONLY ──
                // Settlement cell -> population spawner owns it. Snapshot under SimulationLock (copy-out) — never hold a
                // live Settlements pointer on the game thread (the sim thread mutates it under that lock).
                FMythicSettlementData Settlement;
                if (LWS->CopySettlementAtCell(Cell, Settlement)) {
                    continue;
                }
                // Faction-controlled non-settlement cell -> territory-patrol spawner owns it. Lock-free committed read.
                const FMythicTerritoryCell TC = Grid->GetCell(Cell);
                if (TC.DominantFaction.IsValid()) {
                    continue;
                }

                // ── Biome -> eligible species ── (pure, lock-free; consumes Step 1's biome layer, NOT a self-derived one)
                const EMythicBiome Biome = Grid->GetBiomeAtCell(Cell);
                const int32 BiomeIdx = static_cast<int32>(Biome);
                if (BiomeIdx < 0 || BiomeIdx >= MythicBiomeCount) {
                    continue;
                }
                const TArray<const FMythicCreatureSpeciesRow *> &Eligible = SpeciesByBiome[BiomeIdx];
                if (Eligible.IsEmpty()) {
                    continue; // No species defined for this biome (e.g. a sparse authored table) — leave it empty.
                }

                // ── Deficit ──
                const int32 TargetCount = ComputeCreatureTargetDensity(
                    Settings->MaxCreaturesPerBiomeCell, Settings->CreatureSpawnDensityScale, Settings->MaxEntitiesPerCell);
                const int32 CurrentCount = CellCreatureCounts.FindRef(Cell);
                const int32 Deficit = TargetCount - CurrentCount;
                if (Deficit <= 0) {
                    continue;
                }

                // ── Deterministic weighted species pick for THIS cell ──
                // Seed from the cell coord (+ a salt) so the same wilderness cell always rolls the same species across
                // runs/saves — a coherent ecology rather than per-tick noise. FRandomStream gives a reproducible draw.
                const uint32 CellSeed = HashCombine(GetTypeHash(Cell), 0x53706563u /* 'Spec' */);
                FRandomStream Stream(static_cast<int32>(CellSeed));

                float TotalWeight = 0.0f;
                for (const FMythicCreatureSpeciesRow *Row : Eligible) {
                    TotalWeight += FMath::Max(0.0f, Row->SpawnWeight);
                }
                const FMythicCreatureSpeciesRow *Picked = Eligible[0];
                if (TotalWeight > UE_KINDA_SMALL_NUMBER) {
                    float Roll = Stream.FRandRange(0.0f, TotalWeight);
                    for (const FMythicCreatureSpeciesRow *Row : Eligible) {
                        Roll -= FMath::Max(0.0f, Row->SpawnWeight);
                        if (Roll <= 0.0f) {
                            Picked = Row;
                            break;
                        }
                    }
                }

                // ── Pack sizing: one pack id shared across the members spawned for this cell this tick ──
                const bool bPack = Picked->bIsPackAnimal;
                uint16 PackId = 0;
                int32 DesiredMembers = 1;
                if (bPack) {
                    const int32 MinPack = FMath::Max<int32>(1, Picked->MinPackSize);
                    const int32 MaxPack = FMath::Max<int32>(MinPack, Picked->MaxPackSize);
                    DesiredMembers = Stream.RandRange(MinPack, MaxPack);
                    PackId = AllocatePackId();
                }

                // Spawn min(deficit, desired pack members, budget).
                const int32 ToSpawn = FMath::Min3(Deficit, DesiredMembers, SpawnBudget);
                for (int32 SpawnIdx = 0; SpawnIdx < ToSpawn; ++SpawnIdx) {
                    FMythicCreatureSpawnData Data;

                    // Identity: faction left INVALID (wildlife has no faction — the leader-candidate path guards on
                    // IsValid()). Cell = this wilderness cell. NameHash unique per creature: fold the cell, species,
                    // and a monotonic per-Execute counter through the same Wang mix the NPC identity uses.
                    Data.Identity.Cell = Cell;
                    uint32 Hash = HashCombine(GetTypeHash(Cell), static_cast<uint32>(Picked->SpeciesId));
                    Hash = HashCombine(Hash, ++SpawnCounter);
                    // Wang finalizer (verbatim from FMythicNPCGenerator::HashStep) for good avalanche.
                    Hash = (Hash ^ 61u) ^ (Hash >> 16u);
                    Hash *= 9u;
                    Hash = Hash ^ (Hash >> 4u);
                    Hash *= 0x27d4eb2du;
                    Hash = Hash ^ (Hash >> 15u);
                    Data.Identity.NameHash = Hash;
                    Data.Identity.VisualArchetype = static_cast<uint8>(Hash % 8);

                    // Creature ecology data straight from the species row; den = spawn cell.
                    Data.Creature.SpeciesId = Picked->SpeciesId;
                    Data.Creature.PackId = PackId;
                    Data.Creature.BaseAggression = FMath::Clamp(Picked->BaseAggression, 0.0f, 1.0f);
                    Data.Creature.CurrentAggression = Data.Creature.BaseAggression;
                    Data.Creature.DenCell = Cell;
                    Data.Creature.TerritorialRadius = Picked->DefaultTerritorialRadius;

                    Data.Significance.Tier = EMythicSignificanceTier::Tier0_Ambient;

                    SpawnDataArray.Add(MoveTemp(Data));
                }
                SpawnBudget -= ToSpawn;
            }
        }
    }

    // ─── Deferred batch-create the creature entities ───
    if (SpawnDataArray.Num() > 0) {
        // Copy-capture (NOT move-capture): FMassDeferredCreateCommand stores a TFunction, which requires a COPYABLE
        // callable — a move-only init-capture would make the lambda move-only and fail to bind. Mirrors the proven
        // PopulationSpawnerProcessor capture exactly.
        Context.Defer().PushCommand<FMassDeferredCreateCommand>([SpawnDataArray](FMassEntityManager &Manager) {
            TRACE_CPUPROFILER_EVENT_SCOPE(MythicCreatureSpawner_DeferredSpawn);

            // Composition: Identity + Creature + Significance + creature tag. NO Schedule, NO NPCTag. Significance is
            // present so the entity-type-agnostic significance gate force-embodies it near players; the creature tag
            // routes that embodiment to AMythicCreatureCharacter via the ActorSpawnProcessor's creature query branch.
            const UScriptStruct *Composition[] = {
                FMythicIdentityFragment::StaticStruct(),
                FMythicCreatureFragment::StaticStruct(),
                FMythicSignificanceFragment::StaticStruct(),
                FMythicCreatureTag::StaticStruct()
            };
            FMassArchetypeHandle Archetype = Manager.CreateArchetype(MakeArrayView(Composition));

            TArray<FMassEntityHandle> SpawnedEntities;
            Manager.BatchCreateEntities(Archetype, SpawnDataArray.Num(), SpawnedEntities);

            for (int32 i = 0; i < SpawnDataArray.Num(); ++i) {
                const FMythicCreatureSpawnData &Data = SpawnDataArray[i];
                const FMassEntityHandle Entity = SpawnedEntities[i];
                Manager.GetFragmentDataChecked<FMythicIdentityFragment>(Entity) = Data.Identity;
                Manager.GetFragmentDataChecked<FMythicCreatureFragment>(Entity) = Data.Creature;
                Manager.GetFragmentDataChecked<FMythicSignificanceFragment>(Entity) = Data.Significance;
            }
        });
    }

    // ─── Despawn creatures too far from all players ───
    // Creatures carry FMythicCreatureTag (not FMythicNPCTag), so the NPC population spawner never despawns them — this
    // is their dedicated far-despawn pass. Tear down any embodied creature actor FIRST (it registered into the shared
    // EmbodiedActors map), exactly like the population spawner does for humanoids, then destroy the entity.
    const float DespawnRadiusSq = FMath::Square(Settings->CreatureDespawnDistance);
    int32 DespawnBudget = Settings->MaxCreatureSpawnsPerTick; // reuse the spawn budget as the despawn cap (symmetric)

    ExistingCreatureQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext &ChunkContext) {
        if (DespawnBudget <= 0) {
            return;
        }
        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();

        for (int32 i = 0; i < NumEntities && DespawnBudget > 0; ++i) {
            const FMythicCellCoord &EntityCell = IdentityView[i].Cell;

            bool bNearPlayer = false;
            for (const FMythicCellCoord &PlayerCell : PlayerCells) {
                const float DistSq = static_cast<float>(
                    FMath::Square(EntityCell.X - PlayerCell.X) + FMath::Square(EntityCell.Y - PlayerCell.Y));
                if (DistSq <= DespawnRadiusSq) {
                    bNearPlayer = true;
                    break;
                }
            }

            if (!bNearPlayer) {
                const FMassEntityHandle DespawnEntity = ChunkContext.GetEntity(i);
                // Safe to call directly: bRequiresGameThreadExecution guarantees the game thread; LWS is from Execute.
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
