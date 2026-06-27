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
#include "World/LivingWorld/Roles/RoleTypes.h"        // UMythicRoleDatabase + FMythicRoleDefinition (faction-requirement gate)
#include "World/LivingWorld/Roles/ArchetypeTypes.h"    // UMythicArchetypeCatalog + weighted archetype draw (WHO spawns)
#include "World/LivingWorld/Territory/MythicBiome.h"    // EMythicBiome (cell biome → archetype weight)
#include "World/LivingWorld/MythicTags_LivingWorld.h" // NPC.Role.* native role tags
#include "AI/NPCs/MythicNPCCharacter.h" // despawn tears down any embodied actor before freeing its entity
#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h" // R18-M7: global spawn-serial allocator (collision-free identity)
#include "World/EnvironmentController/MythicEnvironmentSubsystem.h"  // single-source game clock → DayFactor for the archetype draw
#include "World/EnvironmentController/MythicEnvironmentController.h" // GetTimespan() — read the hour without building an FDateTime
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

namespace {
    // Map a derived role tag to the spawn-point purpose it should occupy. Guard/Soldier → Guard posts; the hostile-camp
    // bandit → Enemy points; everything else → Civilian. Pure + file-local (the spawner is the only consumer).
    EMythicSpawnPointPurpose PurposeForRole(const FGameplayTag &RoleTag) {
        if (RoleTag == TAG_NPC_ROLE_GUARD || RoleTag == TAG_NPC_ROLE_SOLDIER) {
            return EMythicSpawnPointPurpose::Guard;
        }
        if (RoleTag == TAG_NPC_ROLE_BANDIT) {
            return EMythicSpawnPointPurpose::Enemy;
        }
        return EMythicSpawnPointPurpose::Civilian;
    }

    // Deterministically pick a generated spawn point in Cell matching Desired (else an Any-purpose point, else null).
    // Among the matching points, index = NameHash % count, so the choice is stable for a given NPC identity (no RNG,
    // no wall-clock) and spreads NPCs of the same purpose across the available anchors. Returns nullptr when the cell
    // has no generated points of a usable purpose → caller uses the cell-center placement path (coexistence).
    const FMythicSpawnPoint *PickPointForCell(const FMythicSettlementData &Settlement, const FMythicCellCoord &Cell,
                                              EMythicSpawnPointPurpose Desired, uint32 NameHash) {
        if (Settlement.SpawnPoints.Num() == 0) {
            return nullptr;
        }

        // Two-pass: first try exact-purpose matches in this cell, then Any-purpose points in this cell as a fallback.
        // (Two cheap passes keep the function allocation-free; spawn-point counts per cell are tiny.)
        for (int32 Pass = 0; Pass < 2; ++Pass) {
            const EMythicSpawnPointPurpose Want = (Pass == 0) ? Desired : EMythicSpawnPointPurpose::Any;
            int32 MatchCount = 0;
            for (const FMythicSpawnPoint &P : Settlement.SpawnPoints) {
                if (P.Cell == Cell && P.Purpose == Want) {
                    ++MatchCount;
                }
            }
            if (MatchCount == 0) {
                continue;
            }
            const int32 Target = static_cast<int32>(NameHash % static_cast<uint32>(MatchCount));
            int32 Seen = 0;
            for (const FMythicSpawnPoint &P : Settlement.SpawnPoints) {
                if (P.Cell == Cell && P.Purpose == Want) {
                    if (Seen == Target) {
                        return &P;
                    }
                    ++Seen;
                }
            }
        }
        return nullptr;
    }
} // namespace

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
    // Inter-settlement travelers (caravans/patrols) carry FMythicNPCTag too, but their lifecycle is owned by the
    // UMythicTravelerRouteProcessor (arrival despawn). EXCLUDE them from this query's far-from-player despawn so a
    // traveler isn't culled mid-journey on the open road, AND from the per-cell density count so a passing caravan
    // doesn't crowd out the ambient settlement population. (Step 5 — the ONLY traveler edit to this file.)
    ExistingNPCQuery.AddTagRequirement<FMythicTravelerTag>(EMassFragmentPresence::None);
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

    // Resolve the role database ONCE per Execute (NEVER inside the per-NPC fill loop) — the role-DB faction-requirement
    // gate (ApplyFactionGate) needs it to demote roles a faction can't field. Null is fine: the gate then no-ops and the
    // drawn role passes through. TSoftObjectPtr::LoadSynchronous here is the same once-per-Execute hoist the
    // ActorSpawnProcessor uses for EmbodiedNPCClass — bRequiresGameThreadExecution guarantees we're on the game thread.
    const UMythicRoleDatabase *RoleDB = Settings->RoleDatabase.LoadSynchronous();

    // Resolve the archetype catalog ONCE per Execute (same hoist discipline as RoleDB — never load inside the fill
    // loop). Authored catalog wins; else the built-in code-default set so role-from-context runs unauthored. The
    // CodeDefaults view keeps the static backing array alive; an authored catalog's Archetypes array is owned by the
    // hard-referenced UObject (not GC'd mid-frame), so the view stays valid for this Execute either way.
    TConstArrayView<FMythicArchetypeRow> ArchetypeCatalog;
    if (const UMythicArchetypeCatalog *Catalog = Settings->ArchetypeCatalog.LoadSynchronous()) {
        ArchetypeCatalog = Catalog->Archetypes;
    } else {
        ArchetypeCatalog = MythicArchetypeDefaults::GetCodeDefaultArchetypes();
    }

    // Compute the time-of-day factor ONCE per Execute (never per NPC; never inside DeriveArchetype, which stays a pure
    // function of its inputs). DayFactor = 0.5*(1 + cos(2π*(GameHour-12)/24)) → 1 at midday (hour 12), 0 at midnight.
    // Game hour resolution mirrors the ScheduleTransitionProcessor's single-source-of-truth: prefer the environment
    // clock (so spawns and the visible sky agree), else fall back to elapsed game time over the configured day length.
    float GameHour;
    const UMythicEnvironmentSubsystem *Env = GI->GetSubsystem<UMythicEnvironmentSubsystem>();
    if (Env && Env->GetEnvironmentController() != nullptr) {
        const FTimespan Timespan = Env->GetEnvironmentController()->GetTimespan();
        GameHour = Timespan.GetHours() + Timespan.GetMinutes() / 60.0f; // [0,24), env-clock-driven
    } else {
        const float DayLengthSeconds = FMath::Max(1.0f, Settings->DayLengthSeconds);
        const float DayProgress = FMath::Fmod(static_cast<float>(World->GetTimeSeconds()), DayLengthSeconds) / DayLengthSeconds;
        GameHour = DayProgress * 24.0f; // [0,24)
    }
    const float DayFactor = 0.5f * (1.0f + FMath::Cos(2.0f * UE_PI * (GameHour - 12.0f) / 24.0f));

    // Normalizer for faction wealth → [0,1]. Reserves.Wealth can be negative (a struggling faction), so clamp.
    const float MaxReserve = FMath::Max(1.0f, Settings->MaxReserve);

    // Resolve the hostile-camp role ONCE per Execute (never per NPC): the settings override if a valid tag is authored,
    // else the native TAG_NPC_ROLE_BANDIT fallback so hostile camps work unauthored. Hoisted out of the fill loop.
    const FGameplayTag HostileRoleTag =
        Settings->BanditRoleTag.IsValid() ? Settings->BanditRoleTag : TAG_NPC_ROLE_BANDIT;

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

    // Cells already CONSIDERED this tick — dedup across overlapping player spawn radii (see the lock note below).
    TSet<FMythicCellCoord> ConsideredCells;
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

                // Dedup BEFORE the (SimulationLock-guarded) settlement copy below. When players cluster — common in
                // co-op — their spawn radii overlap heavily, so the same cell is reached once per nearby player.
                // Considering each cell only once per tick eliminates redundant CopySettlementAtCell lock acquisitions
                // that contend with the sim thread's conquest writes. The set holds every cell CONSIDERED this tick
                // (settlement or not), so even non-settlement cells aren't re-copied per overlapping player.
                if (ConsideredCells.Contains(CandidateCell)) {
                    continue;
                }
                ConsideredCells.Add(CandidateCell);

                // Only populate cells that belong to a settlement. Snapshot it under SimulationLock (copy-out) — NEVER
                // hold a live Settlements-map pointer on this game thread: the sim thread writes GoverningFaction during
                // a conquest TransferSettlement under that lock, so an unlocked read here tears (NPCs stamped to the
                // wrong faction) or dangles across a RegisterSettlement rehash.
                FMythicSettlementData Settlement;
                if (!LWS->CopySettlementAtCell(CandidateCell, Settlement) || !Settlement.GoverningFaction.IsValid()) {
                    continue;
                }

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

                    // ─── Role-from-context (sim-driven weighted archetype draw) ───
                    // One archetype is weighted-picked from the catalog against the LIVE sim snapshot: faction wealth +
                    // military strength, the settlement's resolved economy, the cell's biome, and time-of-day. The
                    // RequiredFactionTags pre-filter in the row removes archetypes this faction can't field; the
                    // settlement-population path is NOT a wilderness context, so settlement-only and group-anchor
                    // (alone-disallowed) rows remain eligible here. The role-DB faction-requirement gate (resolved once
                    // via the hoisted RoleDB) still applies authoritatively on top of the draw — a peasant faction that
                    // somehow draws an authored-restricted role is demoted to Civilian. Deterministic from NameHash, so
                    // the role decodes stably from the entity's persisted seed (decode-from-seed identity model) and
                    // reaches dialogue/shop/personality consumers the instant it embodies.
                    FMythicArchetypeContext ArchCtx;
                    ArchCtx.WealthNorm = FMath::Clamp(FactionData.Reserves.Wealth / MaxReserve, 0.0f, 1.0f);
                    ArchCtx.Military = FMath::Clamp(FactionData.MilitaryStrength, 0.0f, 1.0f);
                    ArchCtx.Economy = ResolveEconomy(Settlement.Economy, FactionData.BaseProduction);
                    ArchCtx.Biome = Grid->GetBiomeAtCell(CandidateCell);
                    ArchCtx.DayFactor = DayFactor;
                    ArchCtx.FactionTag = FactionData.FactionTag;
                    ArchCtx.bWildernessContext = false; // settlement cell

                    const FMythicArchetypeRow *ChosenRow = nullptr;
                    const FGameplayTag DerivedRole =
                        DeriveArchetype(ArchetypeCatalog, ArchCtx, SpawnData.Identity.NameHash, ChosenRow);
                    // Hostile camp: every occupant is an enemy (bandit/raider), so the peaceful archetype draw is
                    // overridden with the resolved hostile role. The faction gate is NOT applied to it — hostility is a
                    // property of the PLACE (server-authoritative bIsHostileCamp), not the governing faction's role
                    // roster. A peaceful settlement keeps the drawn-then-faction-gated role unchanged.
                    SpawnData.Identity.RoleTag = Settlement.bIsHostileCamp
                        ? HostileRoleTag
                        : ApplyFactionGate(RoleDB, DerivedRole, FactionData.FactionTag);

                    // ─── Settlement spawn-point fast-path anchor ───
                    // If the governing settlement generated navmesh-valid points for this cell, pick the one matching the
                    // NPC's role-derived purpose (deterministic from NameHash) and stamp it as the embodiment override so
                    // the ActorSpawnProcessor skips the project+scatter pipeline (it re-tests occupancy only). No matching
                    // point → leave bHasSpawnOverride false → the existing cell-center placement path is used verbatim.
                    if (const FMythicSpawnPoint *Point = PickPointForCell(
                            Settlement, CandidateCell, PurposeForRole(SpawnData.Identity.RoleTag),
                            SpawnData.Identity.NameHash)) {
                        SpawnData.Identity.SpawnOverridePos = Point->WorldLocation;
                        SpawnData.Identity.bHasSpawnOverride = true;
                    }

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
    int32 FactionCapacity) {
    // Cap by system limit
    const int32 EffectiveMax = FMath::Min(SettlementMaxDensity, SystemMaxPerCell);

    // Scale by faction health (population / capacity)
    if (FactionCapacity <= 0) {
        return 0;
    }

    const float FillRatio = FMath::Clamp(static_cast<float>(FactionPopulation) / static_cast<float>(FactionCapacity), 0.0f, 1.0f);
    return FMath::CeilToInt(EffectiveMax * FillRatio);
}

EMythicSettlementEconomy UMythicPopulationSpawnerProcessor::ResolveEconomy(
    EMythicSettlementEconomy Authored,
    const FMythicResourceStock &FactionBaseProduction) {
    // An explicitly authored economy always wins — designers override the derived identity per-settlement.
    if (Authored != EMythicSettlementEconomy::Generic) {
        return Authored;
    }

    // Otherwise derive from the faction's economic identity: the single dominant base-production resource picks the
    // economy. All-zero production (e.g. bandits who steal rather than produce) stays Generic.
    const float Food = FactionBaseProduction.Food;
    const float Materials = FactionBaseProduction.Materials;
    const float Arms = FactionBaseProduction.Arms;
    const float Wealth = FactionBaseProduction.Wealth;

    const float MaxVal = FMath::Max(FMath::Max(Food, Materials), FMath::Max(Arms, Wealth));
    if (MaxVal <= UE_KINDA_SMALL_NUMBER) {
        return EMythicSettlementEconomy::Generic;
    }

    // Tie-break order is fixed (Arms > Food > Materials > Wealth) so the derivation is deterministic.
    if (Arms >= MaxVal) {
        return EMythicSettlementEconomy::Military;
    }
    if (Food >= MaxVal) {
        return EMythicSettlementEconomy::Farming;
    }
    if (Materials >= MaxVal) {
        return EMythicSettlementEconomy::Mining;
    }
    return EMythicSettlementEconomy::Trade; // Wealth dominant
}

FGameplayTag UMythicPopulationSpawnerProcessor::DeriveArchetype(
    TConstArrayView<FMythicArchetypeRow> Catalog,
    const FMythicArchetypeContext &Ctx,
    uint32 NameHash,
    const FMythicArchetypeRow *&OutChosen) {
    OutChosen = nullptr;

    // Compute each row's effective weight against the live context. We do a single pass to accumulate the total, then a
    // second cumulative walk to pick the winner — keeping the function allocation-free (no temp weight array). Both
    // passes apply the identical formula so the winner-selection is consistent with the total.
    const int32 EcoIdx = static_cast<int32>(Ctx.Economy);
    const int32 BiomeIdx = static_cast<int32>(Ctx.Biome);

    auto EffectiveWeight = [&Ctx, EcoIdx, BiomeIdx](const FMythicArchetypeRow &Row) -> float {
        // Hard gates first (cheap, and they zero the row out of this context entirely).
        if (!Row.RequiredFactionTags.IsEmpty()) {
            // A single-tag container is the cheapest way to ask "does this one faction tag satisfy the requirement set"
            // via the same HasAny API used across the codebase. An invalid faction tag never satisfies a requirement.
            const FGameplayTagContainer FactionTagContainer(Ctx.FactionTag);
            if (!Row.RequiredFactionTags.HasAny(FactionTagContainer)) {
                return 0.0f;
            }
        }
        // Wilderness/patrol context: a settlement-only OR group-only archetype can't be a lone wilderness spawn.
        if (Ctx.bWildernessContext && (Row.bRequiresSettlement || !Row.bAllowedAlone)) {
            return 0.0f;
        }

        float W = FMath::Max(0.0f, Row.BaseWeight);
        if (W <= 0.0f) {
            return 0.0f;
        }

        // Wealth axis: a row can be favored by wealth AND/OR by poverty; both lerps multiply, so a wealth-neutral row
        // (both == 1) is unaffected at any wealth level.
        W *= FMath::Lerp(1.0f, FMath::Max(0.0f, Row.WealthFavor), Ctx.WealthNorm);
        W *= FMath::Lerp(1.0f, FMath::Max(0.0f, Row.WealthDisfavor), 1.0f - Ctx.WealthNorm);

        // Military axis. A military-FAVORED row (MilitaryFavor > 1) is SCARCE at peace and abundant at full
        // militarization: scale from 1/Favor (suppressed) at Military=0 up to Favor at Military=1. A neutral row
        // (Favor==1) is unaffected; a military-averse row (Favor<1) simply lerps down. This is what gives a
        // zero-military town a near-zero soldier/guard share while a fortress fields an armed majority (the linear
        // 1→Favor lerp left armed rows at full base weight at peace).
        {
            const float MF = FMath::Max(0.0f, Row.MilitaryFavor);
            W *= (MF > 1.0f) ? FMath::Lerp(1.0f / MF, MF, Ctx.Military)
                             : FMath::Lerp(1.0f, MF, Ctx.Military);
        }

        // Economy / biome multipliers — IsValidIndex-guarded so an empty/short array means neutral.
        if (Row.EconomyWeights.IsValidIndex(EcoIdx)) {
            W *= FMath::Max(0.0f, Row.EconomyWeights[EcoIdx]);
        }
        if (Row.BiomeWeights.IsValidIndex(BiomeIdx)) {
            W *= FMath::Max(0.0f, Row.BiomeWeights[BiomeIdx]);
        }

        // Time-of-day: lerp Night→Day by DayFactor (1=midday, 0=midnight).
        W *= FMath::Lerp(FMath::Max(0.0f, Row.NightWeight), FMath::Max(0.0f, Row.DayWeight),
                         FMath::Clamp(Ctx.DayFactor, 0.0f, 1.0f));

        return FMath::Max(0.0f, W);
    };

    float Total = 0.0f;
    for (const FMythicArchetypeRow &Row : Catalog) {
        Total += EffectiveWeight(Row);
    }

    // Empty catalog or every row gated to 0 → safe civilian fallback (the doctrine guarantees the code defaults always
    // include an always-eligible civilian, but an authored catalog might not — never crash, never spawn role-less).
    if (Total <= UE_KINDA_SMALL_NUMBER) {
        return TAG_NPC_ROLE_CIVILIAN;
    }

    // Deterministic weighted pick. Salt with 'Arch' (0x41726368) so this roll is independent of the demographic/visual/
    // personality rolls, mask to 24 bits for a clean [0,1) fraction, scale into the cumulative-weight space.
    const uint32 Seed = HashCombine(NameHash, 0x41726368u);
    const float Roll = (static_cast<float>(Seed & 0xFFFFFFu) / 16777216.0f) * Total; // 2^24

    float Cumulative = 0.0f;
    const FMythicArchetypeRow *Last = nullptr;
    for (const FMythicArchetypeRow &Row : Catalog) {
        const float W = EffectiveWeight(Row);
        if (W <= 0.0f) {
            continue;
        }
        Last = &Row;
        Cumulative += W;
        if (Roll < Cumulative) {
            OutChosen = &Row;
            return Row.RoleTag.IsValid() ? Row.RoleTag : TAG_NPC_ROLE_CIVILIAN;
        }
    }

    // Floating-point guard: Roll can equal Total at the high boundary — fall to the last positive-weight row.
    if (Last) {
        OutChosen = Last;
        return Last->RoleTag.IsValid() ? Last->RoleTag : TAG_NPC_ROLE_CIVILIAN;
    }
    return TAG_NPC_ROLE_CIVILIAN;
}

FGameplayTag UMythicPopulationSpawnerProcessor::ApplyFactionGate(
    const UMythicRoleDatabase *RoleDB,
    const FGameplayTag &DerivedRole,
    const FGameplayTag &FactionTag) {
    // No DB → no gate (systems must run unauthored). Invalid role → nothing to gate.
    if (!RoleDB || !DerivedRole.IsValid()) {
        return DerivedRole;
    }

    const FMythicRoleDefinition *RoleDef = RoleDB->FindRole(DerivedRole);
    if (!RoleDef || RoleDef->RequiredFactionTags.IsEmpty()) {
        // Role not authored in the DB, or it has no faction requirement → it can be held by any faction.
        return DerivedRole;
    }

    // The role requires a specific faction profile. If the spawning faction's tag matches none of the required tags,
    // demote to a plain civilian (a peasant faction shouldn't field this role). A single-tag container is the cheapest
    // way to ask "does this one tag satisfy the requirement set" via the same HasAny API used across the codebase.
    const FGameplayTagContainer FactionTagContainer(FactionTag);
    if (!RoleDef->RequiredFactionTags.HasAny(FactionTagContainer)) {
        return TAG_NPC_ROLE_CIVILIAN;
    }

    return DerivedRole;
}
