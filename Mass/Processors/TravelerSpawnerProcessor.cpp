// Mythic Living World — Traveler Spawner Processor Implementation

#include "Mass/Processors/TravelerSpawnerProcessor.h"
#include "MassExecutionContext.h"
#include "MassCommandBuffer.h"
#include "MassCommands.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Mass/Fragments/MythicTravelerFragment.h"
#include "Mass/Tags/MythicMassTags.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "World/LivingWorld/NPCGeneration/NPCGenerator.h"
#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h" // NPC.Role.Merchant / NPC.Role.Guard fallbacks (unauthored-asset default)
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

UMythicTravelerSpawnerProcessor::UMythicTravelerSpawnerProcessor() {
    // PrePhysics, server/standalone only, game thread — mirrors the other Living-World spawners exactly (creates
    // entities via the command buffer + reads SimulationLock-guarded settlement copies).
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
    bRequiresGameThreadExecution = true;
    bAutoRegisterWithProcessingPhases = true;

    // Creates the first traveler archetype — must survive startup pruning (same chicken-and-egg reason as the other
    // spawners: the archetype may not exist yet).
    QueryBasedPruning = EMassQueryBasedPruning::Never;

    // Stable ordering relative to the rest of the population pipeline (deterministic processor order). Travelers are
    // an independent zone (the open road between towns), so this is for ordering hygiene, not a data dependency.
    ExecutionOrder.ExecuteAfter.Add(TEXT("UMythicPopulationSpawnerProcessor"));

    ActiveTravelerQuery.RegisterWithProcessor(*this);
}

void UMythicTravelerSpawnerProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    // Count active travelers for the MaxActiveTravelers cap. Identity is required only so the query has a fragment
    // requirement (a tag-only count is fine too, but every traveler carries Identity — keep it explicit + cheap).
    ActiveTravelerQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    ActiveTravelerQuery.AddTagRequirement<FMythicTravelerTag>(EMassFragmentPresence::All);
}

FMythicCellCoord UMythicTravelerSpawnerProcessor::StepToward(FMythicCellCoord From, FMythicCellCoord To) {
    // Greedy king-move (8-dir): advance each axis by its sign toward the target. Diagonal = one step. Deterministic.
    const int32 DX = To.X - From.X;
    const int32 DY = To.Y - From.Y;
    const int32 StepX = (DX > 0) ? 1 : ((DX < 0) ? -1 : 0);
    const int32 StepY = (DY > 0) ? 1 : ((DY < 0) ? -1 : 0);
    return FMythicCellCoord(From.X + StepX, From.Y + StepY);
}

void UMythicTravelerSpawnerProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicTravelerSpawner_Execute);

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
    if (TimeSinceLastTick < Settings->TravelerSpawnIntervalSeconds) {
        return;
    }
    TimeSinceLastTick = 0.0f;

    UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid();
    UMythicFactionDatabase *FactionDB = LWS->GetFactionDatabase();
    if (!Grid || !FactionDB) {
        return;
    }

    // Global collision-free identity source (same allocator the ambient + patrol + encounter spawners use).
    UMythicPersistentNPCRegistry *PersistentRegistry = LWS->GetPersistentNPCRegistry();
    if (!PersistentRegistry) {
        return;
    }

    // ─── Cap gate: bail BEFORE any settlement work if we're already at the active-traveler ceiling ───
    int32 ActiveTravelers = 0;
    ActiveTravelerQuery.ForEachEntityChunk(Context, [&ActiveTravelers](FMassExecutionContext &ChunkContext) {
        ActiveTravelers += ChunkContext.GetNumEntities();
    });
    int32 SpawnSlots = Settings->MaxActiveTravelers - ActiveTravelers;
    if (SpawnSlots <= 0) {
        return;
    }
    SpawnSlots = FMath::Min(SpawnSlots, Settings->MaxTravelersPerTick);
    if (SpawnSlots <= 0) {
        return;
    }

    // ─── Step 1: Gather player cells ───
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

    // ─── Step 2: Build the near-player settlement set (origins) + the full settlement set (destinations) ───
    // SimulationLock-safe enumeration + per-id copy-out (NEVER hold a live Settlements pointer on the game thread).
    TArray<int32> AllSettlementIds;
    LWS->CopyAllSettlementIds(AllSettlementIds);
    if (AllSettlementIds.Num() < 2) {
        return; // need at least two settlements to have a route between them
    }

    // A compact snapshot of the fields we need per settlement (id, center, governing faction). Copied once so the
    // pairing loop is a flat array walk with no further locks.
    struct FMythicTravelerSettlement {
        int32 Id = INDEX_NONE;
        FMythicCellCoord Center;
        FMythicFactionId Faction;
    };
    TArray<FMythicTravelerSettlement> Settlements; // ALL settlements (candidate destinations)
    Settlements.Reserve(AllSettlementIds.Num());
    TArray<int32> NearPlayerOriginIdx; // indices INTO Settlements whose center is near a player (candidate origins)

    const float OriginRadiusSq = FMath::Square(Settings->TravelerSpawnPlayerRadiusCells);
    for (int32 SettlementId : AllSettlementIds) {
        FMythicSettlementData Data;
        if (!LWS->CopySettlementById(SettlementId, Data)) {
            continue;
        }
        if (!Data.GoverningFaction.IsValid()) {
            continue;
        }
        FMythicTravelerSettlement S;
        S.Id = SettlementId;
        S.Center = Data.CenterCell;
        S.Faction = Data.GoverningFaction;
        const int32 Index = Settlements.Add(S);

        // Near a player? (Origins must be visible so the player sees the caravan depart.)
        for (const FMythicCellCoord &PC : PlayerCells) {
            const float DistSq = static_cast<float>(
                FMath::Square(S.Center.X - PC.X) + FMath::Square(S.Center.Y - PC.Y));
            if (DistSq <= OriginRadiusSq) {
                NearPlayerOriginIdx.Add(Index);
                break;
            }
        }
    }

    if (NearPlayerOriginIdx.IsEmpty() || Settlements.Num() < 2) {
        return;
    }

    // ─── Step 3: Deterministic time window for the pairing seed ───
    // floor(WorldTime / interval) is stable within one spawn window so the same origin/dest pairing is reproducible
    // across re-evaluations within that window (and across save/reload at the same world time). Lock-free game-thread
    // clock (the schedule processor uses the same source for its fallback hour).
    const double WorldTime = World->GetTimeSeconds();
    const float Interval = FMath::Max(Settings->TravelerSpawnIntervalSeconds, UE_KINDA_SMALL_NUMBER);
    const uint32 WindowIndex = static_cast<uint32>(FMath::FloorToInt(static_cast<float>(WorldTime) / Interval));

    // ─── Step 4: For each spawn slot, pick an origin (near player) + a kind-appropriate destination ───
    struct FMythicTravelerSpawnData {
        FMythicIdentityFragment Identity;
        FMythicScheduleFragment Schedule;
        FMythicSignificanceFragment Significance;
        FMythicTravelerFragment Traveler;
    };
    TArray<FMythicTravelerSpawnData> SpawnDataArray;
    SpawnDataArray.Reserve(SpawnSlots);

    const float CaravanRatio = FMath::Clamp(Settings->CaravanPatrolRatio, 0.0f, 1.0f);

    // Role tags: the authored settings tags win; fall back to the native NPC.Role.Merchant / NPC.Role.Guard tags when a
    // designer left them unset, so travelers get a MEANINGFUL role (and the role-aware personality generator fires on
    // embodiment) even with a freshly-created, unauthored settings asset. The native leaf names preserve the
    // Merchant/Guard substrings GeneratePersonality keys off.
    const FGameplayTag CaravanRole = Settings->CaravanRoleTag.IsValid() ? Settings->CaravanRoleTag : TAG_NPC_ROLE_MERCHANT;
    const FGameplayTag PatrolRole = Settings->PatrolRoleTag.IsValid() ? Settings->PatrolRoleTag : TAG_NPC_ROLE_GUARD;

    for (int32 Slot = 0; Slot < SpawnSlots; ++Slot) {
        // Deterministic origin pick for this slot/window. Salt with the slot index so multiple slots in one window
        // don't all collapse onto the same origin.
        const uint32 OriginSeed = HashCombine(HashCombine(WindowIndex, 0x4F726967u /* 'Orig' */),
                                              static_cast<uint32>(Slot));
        const int32 OriginIdx = NearPlayerOriginIdx[OriginSeed % static_cast<uint32>(NearPlayerOriginIdx.Num())];
        const FMythicTravelerSettlement &Origin = Settlements[OriginIdx];

        // Kind roll (caravan vs patrol), deterministic per slot/window/origin.
        const uint32 KindSeed = HashCombine(HashCombine(OriginSeed, static_cast<uint32>(Origin.Id)), 0x4B696E64u /* 'Kind' */);
        const float KindRoll = static_cast<float>(KindSeed & 0xFFFFFFu) / 16777216.0f;
        const bool bCaravan = (KindRoll < CaravanRatio);

        // ─── Destination selection ───
        // Build the eligible-destination list for this kind, then deterministically pick one. Caravans head to any
        // OTHER reachable settlement, preferring a Friendly/Neutral-or-better relationship (trade partners); patrols
        // escort a route to a SAME-governing-faction settlement (a faction patrolling between its own towns).
        int32 BestDestIdx = INDEX_NONE;
        int32 PreferredCount = 0;
        // First pass: count preferred candidates so the deterministic pick indexes only among them when any exist.
        auto IsEligible = [&](const FMythicTravelerSettlement &Dest) -> bool {
            if (Dest.Id == Origin.Id) {
                return false;
            }
            // Within route cap (manhattan on the grid). MaxRouteCellLength bounds both the route length and the
            // dead-reckoning cost when the traveler is off-screen.
            const int32 Manhattan = FMath::Abs(Dest.Center.X - Origin.Center.X) + FMath::Abs(Dest.Center.Y - Origin.Center.Y);
            if (Manhattan > Settings->MaxRouteCellLength) {
                return false;
            }
            if (bCaravan) {
                // Caravan: prefer friendly/neutral; reject only outright Hostile (a merchant won't drive into an
                // enemy capital). Same-faction is fine (internal trade).
                const EMythicFactionRelation Rel = FactionDB->GetRelationship(Origin.Faction, Dest.Faction);
                return (Rel != EMythicFactionRelation::Hostile);
            }
            // Patrol: ONLY same governing faction (escorting the flag between own towns).
            return (Dest.Faction == Origin.Faction);
        };

        for (int32 d = 0; d < Settlements.Num(); ++d) {
            if (IsEligible(Settlements[d])) {
                ++PreferredCount;
            }
        }
        if (PreferredCount == 0) {
            continue; // no valid destination for this kind from this origin — skip this slot
        }

        // Deterministic pick among the eligible set: walk the Nth eligible where N = seeded index.
        const uint32 DestSeed = HashCombine(HashCombine(OriginSeed, static_cast<uint32>(Origin.Id)), 0x44657374u /* 'Dest' */);
        int32 Target = static_cast<int32>(DestSeed % static_cast<uint32>(PreferredCount));
        for (int32 d = 0; d < Settlements.Num(); ++d) {
            if (IsEligible(Settlements[d])) {
                if (Target == 0) {
                    BestDestIdx = d;
                    break;
                }
                --Target;
            }
        }
        if (BestDestIdx == INDEX_NONE) {
            continue;
        }
        const FMythicTravelerSettlement &Dest = Settlements[BestDestIdx];

        // ─── Build the spawn payload ───
        FMythicTravelerSpawnData SpawnData;
        SpawnData.Identity.Faction = Origin.Faction; // the traveler flies the origin faction's colors
        SpawnData.Identity.Cell = Origin.Center;

        // Globally-unique, never-reused serial → collision-free NameHash (same identity model as every other spawner).
        const int32 SpawnSerial = PersistentRegistry->AllocateSpawnSerial();
        SpawnData.Identity.NameHash = FMythicNPCGenerator::GenerateNameHash(
            Origin.Faction.Index, Origin.Center, SpawnSerial);
        SpawnData.Identity.VisualArchetype = FMythicNPCGenerator::GenerateVisualArchetype(SpawnData.Identity.NameHash, 8);
        SpawnData.Identity.DemographicFlags = FMythicNPCGenerator::GenerateDemographicFlags(SpawnData.Identity.NameHash, true);
        SpawnData.Identity.RoleTag = bCaravan ? CaravanRole : PatrolRole;

        // Schedule: Phase = Work + WorkCell = the FIRST step toward the destination, so the embodied cognitive brain's
        // FollowSchedule desire (0.7, Work-phase-gated) immediately routes the AIController to CellToWorld(WorkCell).
        // HomeCell = origin (a fallback anchor for the Rest/idle path if the traveler is ever knocked off-route).
        const FMythicCellCoord FirstStep = StepToward(Origin.Center, Dest.Center);
        SpawnData.Schedule.Phase = EMythicSchedulePhase::Work;
        SpawnData.Schedule.HomeCell = Origin.Center;
        SpawnData.Schedule.WorkCell = FirstStep;

        SpawnData.Significance.Tier = EMythicSignificanceTier::Tier0_Ambient;

        // Route state. StepsRemaining = manhattan + slack, a HARD cap so a traveler always terminates.
        const int32 Manhattan = FMath::Abs(Dest.Center.X - Origin.Center.X) + FMath::Abs(Dest.Center.Y - Origin.Center.Y);
        SpawnData.Traveler.OriginCell = Origin.Center;
        SpawnData.Traveler.DestinationCell = Dest.Center;
        SpawnData.Traveler.DestinationSettlementId = Dest.Id;
        SpawnData.Traveler.Kind = bCaravan ? 0 : 1;
        SpawnData.Traveler.TimeSinceStepSeconds = 0.0f;
        SpawnData.Traveler.StepsRemaining = static_cast<uint16>(FMath::Clamp(Manhattan + 8, 0, 0xFFFF));

        SpawnDataArray.Add(MoveTemp(SpawnData));
    }

    // ─── Step 5: Deferred batch-create the traveler entities (single archetype) ───
    if (SpawnDataArray.Num() > 0) {
        // Copy-capture (FMassDeferredCreateCommand stores a COPYABLE TFunction) — mirrors the proven population/creature
        // spawner idiom. The array is bounded by MaxTravelersPerTick, so the copy is cheap.
        Context.Defer().PushCommand<FMassDeferredCreateCommand>([SpawnDataArray](FMassEntityManager &Manager) {
            TRACE_CPUPROFILER_EVENT_SCOPE(MythicTravelerSpawner_DeferredSpawn);

            // Composition: Identity + Schedule + Significance + Traveler + NPCTag + TravelerTag. NPCTag → embodies as
            // the humanoid EmbodiedNPCClass (existing ActorSpawnProcessor path, no change). TravelerTag → despawn
            // exemption (population spawner) + route-query marker.
            const UScriptStruct *Composition[] = {
                FMythicIdentityFragment::StaticStruct(),
                FMythicScheduleFragment::StaticStruct(),
                FMythicSignificanceFragment::StaticStruct(),
                FMythicTravelerFragment::StaticStruct(),
                FMythicNPCTag::StaticStruct(),
                FMythicTravelerTag::StaticStruct()
            };
            FMassArchetypeHandle Archetype = Manager.CreateArchetype(MakeArrayView(Composition));

            TArray<FMassEntityHandle> SpawnedEntities;
            Manager.BatchCreateEntities(Archetype, SpawnDataArray.Num(), SpawnedEntities);

            for (int32 i = 0; i < SpawnDataArray.Num(); ++i) {
                const FMythicTravelerSpawnData &Data = SpawnDataArray[i];
                const FMassEntityHandle Entity = SpawnedEntities[i];
                Manager.GetFragmentDataChecked<FMythicIdentityFragment>(Entity) = Data.Identity;
                Manager.GetFragmentDataChecked<FMythicScheduleFragment>(Entity) = Data.Schedule;
                Manager.GetFragmentDataChecked<FMythicSignificanceFragment>(Entity) = Data.Significance;
                Manager.GetFragmentDataChecked<FMythicTravelerFragment>(Entity) = Data.Traveler;
            }
        });

        UE_LOG(LogMythLivingWorld, Verbose, TEXT("TravelerSpawner: spawned %d traveler(s) (active was %d / cap %d)."),
               SpawnDataArray.Num(), ActiveTravelers, Settings->MaxActiveTravelers);
    }
}
