// Mythic Living World — Territory Patrol Spawner Processor Implementation

#include "Mass/Processors/TerritoryPatrolSpawnerProcessor.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "MassCommandBuffer.h"
#include "MassCommands.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Mass/Tags/MythicMassTags.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Territory/MythicBiome.h"      // EMythicBiome (frontier-density modifier)
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "World/LivingWorld/NPCGeneration/NPCGenerator.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"     // TAG_NPC_ROLE_SOLDIER / TAG_NPC_ROLE_TRAVELER fallbacks
#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

UMythicTerritoryPatrolSpawnerProcessor::UMythicTerritoryPatrolSpawnerProcessor() {
    // PrePhysics, server/standalone only, game thread — mirrors UMythicPopulationSpawnerProcessor exactly.
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
    bRequiresGameThreadExecution = true;
    bAutoRegisterWithProcessingPhases = true;

    // This processor creates entities — it must run even when zero NPC/soldier archetypes exist yet, so don't let MASS
    // prune it at startup (same chicken-and-egg reason as the population spawner).
    QueryBasedPruning = EMassQueryBasedPruning::Never;

    // Run AFTER the ambient population spawner so settlement cells are populated first and, where spawn radii overlap,
    // the per-cell density count we read already reflects this tick's ambient spawns.
    ExecutionOrder.ExecuteAfter.Add(TEXT("UMythicPopulationSpawnerProcessor"));

    ExistingNPCQuery.RegisterWithProcessor(*this);
}

void UMythicTerritoryPatrolSpawnerProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    // SAME SHAPE as UMythicPopulationSpawnerProcessor::ExistingNPCQuery so the per-cell density count is consistent with
    // the ambient population (shared MaxEntitiesPerCell cap). Soldiers/travelers we spawn carry FMythicNPCTag, so they
    // are counted here on subsequent ticks; encounter-owned entities are excluded (the EncounterDirector owns those).
    ExistingNPCQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    ExistingNPCQuery.AddRequirement<FMythicSignificanceFragment>(EMassFragmentAccess::ReadOnly);
    ExistingNPCQuery.AddTagRequirement<FMythicNPCTag>(EMassFragmentPresence::All);
    ExistingNPCQuery.AddTagRequirement<FMythicEncounterEntityTag>(EMassFragmentPresence::None);
}

void UMythicTerritoryPatrolSpawnerProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicTerritoryPatrolSpawner_Execute);

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
    if (TimeSinceLastTick < Settings->TerritoryPatrolSpawnIntervalSeconds) {
        return;
    }
    TimeSinceLastTick = 0.0f;

    UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid();
    UMythicFactionDatabase *FactionDB = LWS->GetFactionDatabase();
    if (!Grid || !FactionDB) {
        return;
    }

    // Global collision-free identity source (same allocator the ambient + encounter spawners use). Without it we can't
    // guarantee unique NameHash identities — don't spawn aliasable entities.
    UMythicPersistentNPCRegistry *PersistentRegistry = LWS->GetPersistentNPCRegistry();
    if (!PersistentRegistry) {
        return;
    }

    // ─── Step 1: Gather player cells ───────────────────────
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

    // ─── Step 2: Count existing ambient NPCs per cell (shared per-cell cap) ───
    TMap<FMythicCellCoord, int32> CellEntityCounts;
    ExistingNPCQuery.ForEachEntityChunk(Context, [&CellEntityCounts](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();
        for (int32 i = 0; i < NumEntities; ++i) {
            CellEntityCounts.FindOrAdd(IdentityView[i].Cell)++;
        }
    });

    // ─── Step 3: Determine faction-controlled cells needing soldiers/travelers ───
    const float SpawnRadius = Settings->TerritoryPatrolSpawnRadius;
    const float SpawnRadiusSq = FMath::Square(SpawnRadius);
    const int32 SpawnRadiusCells = FMath::CeilToInt(SpawnRadius);
    int32 SpawnBudget = Settings->MaxPatrolSpawnsPerTick;

    // Soldier and traveler spawn-data are gathered separately because they create DIFFERENT archetypes (kind tag), so we
    // build one deferred-create command per kind after the cell pass.
    struct FMythicTerritorySpawnData {
        FMythicIdentityFragment Identity;
        FMythicScheduleFragment Schedule;
        FMythicSignificanceFragment Significance;
    };
    TArray<FMythicTerritorySpawnData> SoldierSpawnData;
    TArray<FMythicTerritorySpawnData> TravelerSpawnData;
    SoldierSpawnData.Reserve(SpawnBudget);

    // Salt the traveler chance roll so it is independent of the role/name rolls but still deterministic per cell.
    constexpr uint32 TravelerChanceSalt = 0x54726176u; // 'Trav'

    TSet<FMythicCellCoord> ConsideredCells;
    for (const FMythicCellCoord &PlayerCell : PlayerCells) {
        for (int32 DY = -SpawnRadiusCells; DY <= SpawnRadiusCells && SpawnBudget > 0; ++DY) {
            for (int32 DX = -SpawnRadiusCells; DX <= SpawnRadiusCells && SpawnBudget > 0; ++DX) {
                if ((DX * DX + DY * DY) > SpawnRadiusSq) {
                    continue;
                }

                const FMythicCellCoord CandidateCell(PlayerCell.X + DX, PlayerCell.Y + DY);
                if (!Grid->IsValidCoord(CandidateCell)) {
                    continue;
                }

                // Dedup across overlapping player radii BEFORE the (SimulationLock-guarded) settlement copy below — same
                // contention-avoidance reason as the population spawner. Holds every cell considered this tick.
                if (ConsideredCells.Contains(CandidateCell)) {
                    continue;
                }
                ConsideredCells.Add(CandidateCell);

                // Territory cell is read lock-free from the committed game-thread snapshot (GetCell copies by value).
                const FMythicTerritoryCell TC = Grid->GetCell(CandidateCell);

                // TRUE wilderness (no dominant faction) belongs to the creature spawner; player-owned property is its
                // owner's concern. Either way, no faction soldiers here.
                if (!TC.DominantFaction.IsValid() || TC.bPlayerOwned) {
                    continue;
                }

                // Settlement cells are owned by the ambient population spawner — never double-populate them. Snapshot
                // under SimulationLock (copy-out); the bare presence of a settlement (regardless of governing faction)
                // hands the cell to that system.
                FMythicSettlementData SettlementScratch;
                if (LWS->CopySettlementAtCell(CandidateCell, SettlementScratch)) {
                    continue;
                }

                // Dead / non-Active factions field no soldiers (a conquered or annihilated faction's banner is gone even
                // if its influence hasn't fully bled away yet).
                FMythicFactionData FactionData;
                if (!FactionDB->GetFaction(TC.DominantFaction, FactionData)) {
                    continue;
                }
                if (!FactionData.bAlive || FactionData.Status != EMythicFactionStatus::Active) {
                    continue;
                }

                // ─── Soldiers: count scaled by military strength × cell influence × biome ───
                // Ownership (strength × influence) sets the base garrison; biome then flavors it — a faction holds its
                // fertile heartland and defensible mountain passes in force but only thinly garrisons arid frontier.
                // GetBiomeAtCell is pure + lock-free (reads only Initialize-time-cached immutables), so it is safe in this
                // per-cell loop. The modifier is applied to the float target THEN re-clamped to the shared per-cell cap so
                // the biome can only THIN a garrison, never push it past MaxEntitiesPerCell.
                const EMythicBiome Biome = Grid->GetBiomeAtCell(CandidateCell);
                const float BiomeMod = BiomeGarrisonModifier(Biome);

                const int32 BaseSoldierTarget = ComputeTerritoryPatrolDensity(
                    FactionData.MilitaryStrength, TC.Influence,
                    Settings->MaxSoldiersPerControlledCell, Settings->MaxEntitiesPerCell);

                int32 SoldierTarget = FMath::CeilToInt(static_cast<float>(BaseSoldierTarget) * BiomeMod);
                SoldierTarget = FMath::Clamp(SoldierTarget, 0, FMath::Min(Settings->MaxSoldiersPerControlledCell, Settings->MaxEntitiesPerCell));

                // ─── Contested-border (border-war) garrison boost ───
                // A cell that touches a cell held by a faction this one is AT WAR with (EMythicFactionRelation::Hostile) is
                // a frontline — it gets a deterministic soldier-count boost so wars VISIBLY thicken the garrison along the
                // front. 4-neighbor probe (matches the influence-propagation adjacency): GetCell is a lock-free value-copy
                // (OOB => default cell with an invalid DominantFaction), GetRelationship takes the FactionDB SnapshotLock —
                // the SAME lock kind already taken by GetFaction above, so no new lock is introduced. No RNG: identical
                // snapshot => identical result. Cost is bounded (<=4 GetCell + <=4 GetRelationship) and only paid once per
                // deduped considered cell.
                bool bContestedBorder = false;
                {
                    static const FMythicCellCoord NeighborOffsets[4] = {
                        FMythicCellCoord(1, 0), FMythicCellCoord(-1, 0),
                        FMythicCellCoord(0, 1), FMythicCellCoord(0, -1)};
                    for (const FMythicCellCoord &Offset : NeighborOffsets) {
                        const FMythicCellCoord NeighborCell(CandidateCell.X + Offset.X, CandidateCell.Y + Offset.Y);
                        const FMythicTerritoryCell NeighborTC = Grid->GetCell(NeighborCell);

                        // Skip true wilderness (no dominant faction; OOB neighbors land here) and same-faction interior
                        // borders — only a DIFFERENT, valid dominant faction can be an enemy.
                        if (!NeighborTC.DominantFaction.IsValid() ||
                            NeighborTC.DominantFaction == TC.DominantFaction) {
                            continue;
                        }

                        // Hostile == at war (FactionDatabase: Allied for self, Neutral for invalid/OOB, Hostile is the
                        // top-of-scale at-war stance). A single hostile neighbor makes this a frontline cell.
                        if (FactionDB->GetRelationship(TC.DominantFaction, NeighborTC.DominantFaction) ==
                            EMythicFactionRelation::Hostile) {
                            bContestedBorder = true;
                            break;
                        }
                    }
                }

                SoldierTarget = ApplyContestedBorderBoost(
                    SoldierTarget, bContestedBorder, Settings->ContestedBorderSoldierMultiplier,
                    Settings->MaxSoldiersPerControlledCell, Settings->MaxEntitiesPerCell);

                const int32 CurrentCount = CellEntityCounts.FindRef(CandidateCell);
                const int32 SoldierDeficit = SoldierTarget - CurrentCount;
                const int32 SoldiersToSpawn = (SoldierDeficit > 0) ? FMath::Min(SoldierDeficit, SpawnBudget) : 0;

                for (int32 SpawnIdx = 0; SpawnIdx < SoldiersToSpawn; ++SpawnIdx) {
                    FMythicTerritorySpawnData Data;
                    Data.Identity.Faction = TC.DominantFaction;
                    Data.Identity.Cell = CandidateCell;

                    // Globally-unique, never-reused serial → collision-free NameHash (same model as the ambient spawner).
                    const int32 SpawnSerial = PersistentRegistry->AllocateSpawnSerial();
                    Data.Identity.NameHash = FMythicNPCGenerator::GenerateNameHash(
                        TC.DominantFaction.Index, CandidateCell, SpawnSerial);
                    Data.Identity.VisualArchetype = FMythicNPCGenerator::GenerateVisualArchetype(Data.Identity.NameHash, 8);
                    Data.Identity.DemographicFlags = FMythicNPCGenerator::GenerateDemographicFlags(
                        Data.Identity.NameHash, FactionData.Population > 50);

                    // KIND is derived from sim state: this is the cell's conquest-accurate dominant faction, and the role
                    // is the designer-authored SoldierRoleTag (the personality generator reads the leaf name on promotion).
                    // SoldierRoleTag defaults to an EMPTY tag, so fall back to the native NPC.Role.Soldier tag when unauthored
                    // — an invalid RoleTag would silently no-op every downstream role consumer (personality/dialogue/debugger),
                    // violating the "systems RUN without authoring" invariant. The leaf name 'Soldier' is what
                    // NPCGenerator::GeneratePersonality's RoleName.Contains() reads on Tier1 promotion.
                    Data.Identity.RoleTag = Settings->SoldierRoleTag.IsValid() ? Settings->SoldierRoleTag : TAG_NPC_ROLE_SOLDIER;

                    // Soldiers idle/patrol at their post (the schedule processor + AIController drive local movement once
                    // embodied). HomeCell/WorkCell anchor them to the territory cell they guard.
                    Data.Schedule.Phase = EMythicSchedulePhase::Idle;
                    Data.Schedule.HomeCell = CandidateCell;
                    Data.Schedule.WorkCell = CandidateCell;

                    Data.Significance.Tier = EMythicSignificanceTier::Tier0_Ambient;

                    SoldierSpawnData.Add(MoveTemp(Data));
                }

                int32 CellSpawned = SoldiersToSpawn;

                // ─── Optional roaming traveler in this faction-controlled cell ───
                // Deterministic per-cell chance (stable within a cell's lifetime — same cell yields the same roll until
                // the world seed / faction changes). Only spawned if there's still per-cell headroom and tick budget.
                if (CellSpawned < SpawnBudget) {
                    const uint32 TravelerSeed = FMythicNPCGenerator::GenerateNameHash(
                        TC.DominantFaction.Index, CandidateCell, static_cast<int32>(TravelerChanceSalt));
                    const float TravelerRoll = static_cast<float>(TravelerSeed & 0xFFFFFFu) / 16777216.0f;

                    const int32 PostTraveler = CurrentCount + CellSpawned;
                    if (TravelerRoll < Settings->TravelerSpawnChancePerCell &&
                        PostTraveler < Settings->MaxEntitiesPerCell) {
                        FMythicTerritorySpawnData Data;
                        Data.Identity.Faction = TC.DominantFaction;
                        Data.Identity.Cell = CandidateCell;

                        const int32 SpawnSerial = PersistentRegistry->AllocateSpawnSerial();
                        Data.Identity.NameHash = FMythicNPCGenerator::GenerateNameHash(
                            TC.DominantFaction.Index, CandidateCell, SpawnSerial);
                        Data.Identity.VisualArchetype = FMythicNPCGenerator::GenerateVisualArchetype(Data.Identity.NameHash, 8);
                        Data.Identity.DemographicFlags = FMythicNPCGenerator::GenerateDemographicFlags(
                            Data.Identity.NameHash, FactionData.Population > 50);
                        // TravelerRoleTag defaults EMPTY — fall back to the native NPC.Role.Traveler tag when unauthored
                        // (same rationale as the soldier fallback above; mirrors TravelerSpawnerProcessor.cpp:197-198).
                        Data.Identity.RoleTag = Settings->TravelerRoleTag.IsValid() ? Settings->TravelerRoleTag : TAG_NPC_ROLE_TRAVELER;

                        // Travelers move — Travel phase routes the schedule/AIController toward their work cell.
                        Data.Schedule.Phase = EMythicSchedulePhase::Travel;
                        Data.Schedule.HomeCell = CandidateCell;
                        Data.Schedule.WorkCell = CandidateCell;

                        Data.Significance.Tier = EMythicSignificanceTier::Tier0_Ambient;

                        TravelerSpawnData.Add(MoveTemp(Data));
                        ++CellSpawned;
                    }
                }

                SpawnBudget -= CellSpawned;
            }
        }
    }

    // ─── Step 4: Deferred batch-create (one command per kind/archetype) ───
    if (SoldierSpawnData.Num() > 0) {
        // Copy-capture (matches UMythicPopulationSpawnerProcessor's proven deferred-create idiom). The array is bounded
        // by MaxPatrolSpawnsPerTick, so the copy is cheap, and FMassDeferredCreateCommand stores a copy-constructible
        // TFunction either way.
        Context.Defer().PushCommand<FMassDeferredCreateCommand>(
            [SoldierSpawnData](FMassEntityManager &Manager) {
                TRACE_CPUPROFILER_EVENT_SCOPE(MythicTerritoryPatrolSpawner_DeferredSoldiers);

                const UScriptStruct *Composition[] = {
                    FMythicIdentityFragment::StaticStruct(),
                    FMythicScheduleFragment::StaticStruct(),
                    FMythicSignificanceFragment::StaticStruct(),
                    FMythicNPCTag::StaticStruct(),   // embody as the humanoid EmbodiedNPCClass + counted by population spawner
                    FMythicSoldierTag::StaticStruct() // KIND marker (debugger tally / archetype filtering)
                };
                FMassArchetypeHandle Archetype = Manager.CreateArchetype(MakeArrayView(Composition));

                TArray<FMassEntityHandle> Spawned;
                Manager.BatchCreateEntities(Archetype, SoldierSpawnData.Num(), Spawned);
                for (int32 i = 0; i < SoldierSpawnData.Num(); ++i) {
                    const FMythicTerritorySpawnData &D = SoldierSpawnData[i];
                    const FMassEntityHandle E = Spawned[i];
                    Manager.GetFragmentDataChecked<FMythicIdentityFragment>(E) = D.Identity;
                    Manager.GetFragmentDataChecked<FMythicScheduleFragment>(E) = D.Schedule;
                    Manager.GetFragmentDataChecked<FMythicSignificanceFragment>(E) = D.Significance;
                }
            });
    }

    if (TravelerSpawnData.Num() > 0) {
        Context.Defer().PushCommand<FMassDeferredCreateCommand>(
            [TravelerSpawnData](FMassEntityManager &Manager) {
                TRACE_CPUPROFILER_EVENT_SCOPE(MythicTerritoryPatrolSpawner_DeferredTravelers);

                const UScriptStruct *Composition[] = {
                    FMythicIdentityFragment::StaticStruct(),
                    FMythicScheduleFragment::StaticStruct(),
                    FMythicSignificanceFragment::StaticStruct(),
                    FMythicNPCTag::StaticStruct(),     // embody as the humanoid EmbodiedNPCClass
                    FMythicTravelerTag::StaticStruct() // despawn-exemption + route/kind marker
                };
                FMassArchetypeHandle Archetype = Manager.CreateArchetype(MakeArrayView(Composition));

                TArray<FMassEntityHandle> Spawned;
                Manager.BatchCreateEntities(Archetype, TravelerSpawnData.Num(), Spawned);
                for (int32 i = 0; i < TravelerSpawnData.Num(); ++i) {
                    const FMythicTerritorySpawnData &D = TravelerSpawnData[i];
                    const FMassEntityHandle E = Spawned[i];
                    Manager.GetFragmentDataChecked<FMythicIdentityFragment>(E) = D.Identity;
                    Manager.GetFragmentDataChecked<FMythicScheduleFragment>(E) = D.Schedule;
                    Manager.GetFragmentDataChecked<FMythicSignificanceFragment>(E) = D.Significance;
                }
            });
    }
}

int32 UMythicTerritoryPatrolSpawnerProcessor::ComputeTerritoryPatrolDensity(
    float MilitaryStrength, float Influence, int32 MaxSoldiersPerControlledCell, int32 MaxEntitiesPerCell) {
    if (MaxSoldiersPerControlledCell <= 0) {
        return 0;
    }

    // Soldier presence scales with BOTH how militarized the faction is AND how firmly it holds this cell. A heartland
    // cell (Influence ≈ 1) of a strong army fields the full ceiling; a weakly-held frontier cell of a peaceful faction
    // fields ~none. Product of the two clamped factors, rounded up so any nonzero strength yields at least one soldier.
    const float StrengthFactor = FMath::Clamp(MilitaryStrength, 0.0f, 1.0f);
    const float InfluenceFactor = FMath::Clamp(Influence, 0.0f, 1.0f);
    const float Scaled = static_cast<float>(MaxSoldiersPerControlledCell) * StrengthFactor * InfluenceFactor;

    int32 Target = FMath::CeilToInt(Scaled);

    // Never exceed the designer ceiling or the global per-cell entity cap (shared safety valve).
    Target = FMath::Min(Target, MaxSoldiersPerControlledCell);
    Target = FMath::Min(Target, MaxEntitiesPerCell);
    return FMath::Max(Target, 0);
}

float UMythicTerritoryPatrolSpawnerProcessor::BiomeGarrisonModifier(EMythicBiome Biome) {
    // Frontier garrison density by biome. Hardcoded (not designer-data) on purpose: this is a small, stable flavoring of
    // an already designer-tuned base (MaxSoldiersPerControlledCell × strength × influence), not a content axis. Plains and
    // Mountain — the fertile heartland and the defensible passes — hold full garrisons; arid Wasteland/Desert frontier is
    // only thinly held; Forest/Wetland sit between (contested, harder to patrol).
    switch (Biome) {
    case EMythicBiome::Plains:
        return 1.0f;
    case EMythicBiome::Mountain:
        return 1.0f;
    case EMythicBiome::Forest:
        return 0.75f;
    case EMythicBiome::Wetland:
        return 0.75f;
    case EMythicBiome::Wasteland:
        return 0.5f;
    case EMythicBiome::Desert:
        return 0.5f;
    default:
        return 1.0f; // well-defined fallback (incl. COUNT sentinel / out-of-bounds Plains default)
    }
}

int32 UMythicTerritoryPatrolSpawnerProcessor::ApplyContestedBorderBoost(
    int32 BaseSoldierTarget, bool bContested, float ContestedMultiplier,
    int32 MaxSoldiersPerControlledCell, int32 MaxEntitiesPerCell) {
    // A quiet (non-frontline) cell keeps its base garrison verbatim.
    if (!bContested) {
        return BaseSoldierTarget;
    }

    // The multiplier is floored at 1.0 so a misconfigured value can never SHRINK a frontline garrison below its peacetime
    // strength. CeilToInt so a war always adds at least one soldier to any cell that already had any (0 stays 0 — a cell
    // a faction can't even garrison in peacetime gets no phantom war garrison).
    const float Mult = FMath::Max(ContestedMultiplier, 1.0f);
    const int32 Boosted = FMath::CeilToInt(static_cast<float>(BaseSoldierTarget) * Mult);

    // Re-clamp to the SAME per-cell ceilings the base target honors — the boost only thickens UP TO the cap, never past
    // it, so the shared MaxEntitiesPerCell safety valve still holds on a frontline.
    const int32 Ceiling = FMath::Min(MaxSoldiersPerControlledCell, MaxEntitiesPerCell);
    return FMath::Max(0, FMath::Min(Boosted, Ceiling));
}
