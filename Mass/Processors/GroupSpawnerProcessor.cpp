// Mythic Living World — Group Spawner Processor Implementation

#include "Mass/Processors/GroupSpawnerProcessor.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "MassCommandBuffer.h"
#include "MassCommands.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Mass/Tags/MythicMassTags.h"
#include "Mass/Processors/PopulationSpawnerProcessor.h" // UMythicPopulationSpawnerProcessor::ResolveEconomy
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "World/LivingWorld/Groups/GroupTypes.h"
#include "World/LivingWorld/NPCGeneration/NPCGenerator.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h" // NPC.Role.* fallbacks
#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h"
#include "World/LivingWorld/Social/SocialGraph.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

namespace {
    // Salt the per-cell group CHANCE roll so it is independent of every per-NPC roll but deterministic per (faction,cell).
    constexpr uint32 GroupChanceSalt = 0x47727043u; // 'GrpC'
    // Salt for the template PICK roll (independent of the chance roll).
    constexpr uint32 GroupPickSalt = 0x47727050u;   // 'GrpP'
    // Salt mixed into GroupId so it doesn't collide with any NameHash.
    constexpr uint32 GroupIdSalt = 0x47727000u;     // 'Grp\0'
} // namespace

UMythicGroupSpawnerProcessor::UMythicGroupSpawnerProcessor() {
    // PrePhysics, server/standalone only, game thread — mirrors the population + patrol spawners exactly.
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
    bRequiresGameThreadExecution = true;
    bAutoRegisterWithProcessingPhases = true;

    // This processor creates entities — it must run even when zero group/NPC archetypes exist yet, so don't let MASS
    // prune it at startup (same chicken-and-egg reason as the population spawner).
    QueryBasedPruning = EMassQueryBasedPruning::Never;

    // Run AFTER the ambient population spawner so settlement cells are populated first and the per-cell density count we
    // read already reflects this tick's ambient spawns where the radii overlap.
    ExecutionOrder.ExecuteAfter.Add(TEXT("UMythicPopulationSpawnerProcessor"));

    ExistingNPCQuery.RegisterWithProcessor(*this);
    ExistingGroupQuery.RegisterWithProcessor(*this);
}

void UMythicGroupSpawnerProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    // SAME SHAPE as UMythicPopulationSpawnerProcessor::ExistingNPCQuery so the per-cell density count is consistent with
    // the ambient population (shared MaxEntitiesPerCell cap). Group members carry FMythicNPCTag, so they self-count here
    // on subsequent ticks; encounter-owned entities are excluded (the EncounterDirector owns those).
    ExistingNPCQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    ExistingNPCQuery.AddRequirement<FMythicSignificanceFragment>(EMassFragmentAccess::ReadOnly);
    ExistingNPCQuery.AddTagRequirement<FMythicNPCTag>(EMassFragmentPresence::All);
    ExistingNPCQuery.AddTagRequirement<FMythicEncounterEntityTag>(EMassFragmentPresence::None);

    // Distinct-active-group count (MaxActiveGroups cap) — only entities that carry the group fragment.
    ExistingGroupQuery.AddRequirement<FMythicGroupFragment>(EMassFragmentAccess::ReadOnly);
    ExistingGroupQuery.AddTagRequirement<FMythicGroupMemberTag>(EMassFragmentPresence::All);
}

void UMythicGroupSpawnerProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicGroupSpawner_Execute);

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
    if (TimeSinceLastTick < Settings->GroupSpawnIntervalSeconds) {
        return;
    }
    TimeSinceLastTick = 0.0f;

    // Per-tick budgets — nothing to do if either is fully spent.
    if (Settings->MaxGroupSpawnsPerTick <= 0 || Settings->MaxGroupMemberSpawnsPerTick <= 0) {
        return;
    }

    UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid();
    UMythicFactionDatabase *FactionDB = LWS->GetFactionDatabase();
    if (!Grid || !FactionDB) {
        return;
    }

    // Global collision-free identity source (same allocator the ambient + patrol spawners use). Without it we can't
    // guarantee unique NameHash identities — don't spawn aliasable entities.
    UMythicPersistentNPCRegistry *PersistentRegistry = LWS->GetPersistentNPCRegistry();
    if (!PersistentRegistry) {
        return;
    }

    // Resolve the group template set ONCE per Execute (never inside the per-cell loop — same hoist discipline as the
    // population spawner's catalog). Authored DB wins; else the built-in code defaults so the system runs unauthored.
    // The CodeDefaults are copied into a stable local array whose lifetime spans this Execute (and is captured by-copy
    // into the deferred-create lambda below); an authored DB's Templates array is owned by the hard-referenced UObject.
    // TemplatesArr is the working array (a stable TArray owned by this Execute): either a copy of the authored DB's
    // Templates or the code defaults. A copy of the (tiny) authored array is cheap and keeps a single by-value source
    // for both the static PickTemplateIndex helper (takes a const TArray&) and the deferred-create lambda capture.
    TArray<FMythicGroupTemplate> TemplatesArr;
    if (const UMythicGroupTemplateDatabase *DB = Settings->GroupTemplateDatabase.LoadSynchronous()) {
        TemplatesArr = DB->Templates;
    } else {
        MythicGroupDefaults::BuildDefaultTemplates(TemplatesArr);
    }
    if (TemplatesArr.Num() == 0) {
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

    // ─── Step 2: Count existing per-cell NPC density + distinct active groups ───
    TMap<FMythicCellCoord, int32> CellEntityCounts;
    ExistingNPCQuery.ForEachEntityChunk(Context, [&CellEntityCounts](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();
        for (int32 i = 0; i < NumEntities; ++i) {
            CellEntityCounts.FindOrAdd(IdentityView[i].Cell)++;
        }
    });

    TSet<uint32> ActiveGroupIds;
    ExistingGroupQuery.ForEachEntityChunk(Context, [&ActiveGroupIds](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto GroupView = ChunkContext.GetFragmentView<FMythicGroupFragment>();
        for (int32 i = 0; i < NumEntities; ++i) {
            if (GroupView[i].GroupId != 0) {
                ActiveGroupIds.Add(GroupView[i].GroupId);
            }
        }
    });
    int32 ActiveGroupCount = ActiveGroupIds.Num();

    // ─── Step 3: Per settlement cell — chance-gated weighted template draw ───
    const float SpawnRadiusSq = FMath::Square(Settings->GroupSpawnRadius);
    const int32 SpawnRadiusCells = FMath::CeilToInt(Settings->GroupSpawnRadius);

    int32 GroupBudget = Settings->MaxGroupSpawnsPerTick;
    int32 MemberBudget = Settings->MaxGroupMemberSpawnsPerTick;
    const int32 PerCellCap = FMath::Max(0, Settings->MaxEntitiesPerCell);

    // One member's pre-resolved spawn data (mirrors the patrol spawner's FMythicTerritorySpawnData, plus the group
    // fragment members ride).
    struct FMythicGroupMemberSpawnData {
        FMythicIdentityFragment Identity;
        FMythicScheduleFragment Schedule;
        FMythicSignificanceFragment Significance;
        FGameplayTag ActivityTag;
        uint32 GroupId = 0;
        bool bIsLeader = false;
    };

    // All members queued this tick, plus per-group [start,count) spans so the deferred lambda can wire intra-group edges
    // without re-deriving membership.
    TArray<FMythicGroupMemberSpawnData> SpawnData;
    struct FMythicGroupSpan {
        int32 Start = 0;
        int32 Count = 0;
        EMythicSocialRelation Relation = EMythicSocialRelation::Friend;
        float EdgeStrength = 0.6f;
    };
    TArray<FMythicGroupSpan> GroupSpans;

    TSet<FMythicCellCoord> ConsideredCells;
    for (const FMythicCellCoord &PlayerCell : PlayerCells) {
        for (int32 DY = -SpawnRadiusCells; DY <= SpawnRadiusCells && GroupBudget > 0 && MemberBudget > 0; ++DY) {
            for (int32 DX = -SpawnRadiusCells; DX <= SpawnRadiusCells && GroupBudget > 0 && MemberBudget > 0; ++DX) {
                if ((DX * DX + DY * DY) > SpawnRadiusSq) {
                    continue;
                }
                if (ActiveGroupCount >= Settings->MaxActiveGroups) {
                    // World-wide active-group cap reached — stop considering cells entirely this tick.
                    DY = SpawnRadiusCells + 1;
                    break;
                }

                const FMythicCellCoord CandidateCell(PlayerCell.X + DX, PlayerCell.Y + DY);
                if (!Grid->IsValidCoord(CandidateCell)) {
                    continue;
                }

                // Dedup across overlapping player radii BEFORE the SimulationLock-guarded settlement copy (same
                // contention-avoidance reason as the population/patrol spawners).
                if (ConsideredCells.Contains(CandidateCell)) {
                    continue;
                }
                ConsideredCells.Add(CandidateCell);

                // Groups spawn in SETTLEMENT cells only (opposite of the patrol spawner). Snapshot under SimulationLock.
                FMythicSettlementData Settlement;
                if (!LWS->CopySettlementAtCell(CandidateCell, Settlement) || !Settlement.GoverningFaction.IsValid()) {
                    continue;
                }
                // Hostile camps host enemies, not social groups — skip them (their occupants are the population spawner's
                // bandit branch).
                if (Settlement.bIsHostileCamp) {
                    continue;
                }

                FMythicFactionData FactionData;
                if (!FactionDB->GetFaction(Settlement.GoverningFaction, FactionData)) {
                    continue;
                }
                if (!FactionData.bAlive || FactionData.Status != EMythicFactionStatus::Active) {
                    continue;
                }

                // Deterministic per-(faction,cell) chance roll: stable for this cell's lifetime under a fixed snapshot.
                const uint32 ChanceSeed = FMythicNPCGenerator::GenerateNameHash(
                    Settlement.GoverningFaction.Index, CandidateCell, static_cast<int32>(GroupChanceSalt));
                const float ChanceRoll = static_cast<float>(ChanceSeed & 0xFFFFFFu) / 16777216.0f;
                if (ChanceRoll >= Settings->GroupSpawnChancePerCell) {
                    continue;
                }

                // Resolve the cell's effective economy (authored wins; else derived from faction base production) — the
                // same resolver the population spawner uses, so the economy gate matches the ambient role mix.
                const EMythicSettlementEconomy EffEconomy =
                    UMythicPopulationSpawnerProcessor::ResolveEconomy(Settlement.Economy, FactionData.BaseProduction);

                // Weighted-pick one eligible template deterministically.
                const uint32 PickSeed = FMythicNPCGenerator::GenerateNameHash(
                    Settlement.GoverningFaction.Index, CandidateCell, static_cast<int32>(GroupPickSalt));
                int32 TemplateIndex = INDEX_NONE;
                if (!PickTemplateIndex(TemplatesArr, EffEconomy, FactionData, PickSeed, TemplateIndex)) {
                    continue;
                }
                const FMythicGroupTemplate &Template = TemplatesArr[TemplateIndex];

                // ─── Roll member counts (clamped to MaxGroupMembers + per-cell + per-tick headroom) ───
                const int32 CurrentCount = CellEntityCounts.FindRef(CandidateCell);
                const int32 CellHeadroom = FMath::Max(0, PerCellCap - CurrentCount);
                int32 RemainingGroupSlots = FMath::Min(MaxGroupMembers, FMath::Min(CellHeadroom, MemberBudget));
                if (RemainingGroupSlots <= 0) {
                    continue;
                }

                // Gather (role, isLeader) for each member up to the headroom, leader-first so a clipped group keeps its
                // anchor. We resolve the leader spec first, then the rest.
                struct FMythicResolvedMember {
                    FGameplayTag RoleTag;
                    bool bIsLeader = false;
                };
                TArray<FMythicResolvedMember> ResolvedMembers;

                auto AppendSpec = [&](const FMythicGroupMemberSpec &Spec, bool bSpecIsLeader) {
                    if (RemainingGroupSlots <= 0) {
                        return;
                    }
                    const uint32 CountSeed = FMythicNPCGenerator::GenerateNameHash(
                        Settlement.GoverningFaction.Index, CandidateCell, TemplateIndex * 31 + ResolvedMembers.Num());
                    int32 Count = RollMemberCount(Spec, CountSeed);
                    Count = FMath::Min(Count, RemainingGroupSlots);
                    const FGameplayTag Role = Spec.RoleTag.IsValid() ? Spec.RoleTag : TAG_NPC_ROLE_CIVILIAN;
                    for (int32 c = 0; c < Count; ++c) {
                        FMythicResolvedMember M;
                        M.RoleTag = Role;
                        // Only the first member of the leader spec is the actual leader.
                        M.bIsLeader = bSpecIsLeader && (c == 0);
                        ResolvedMembers.Add(M);
                        --RemainingGroupSlots;
                    }
                };

                // Leader spec first (an explicit bIsLeader spec, else the first spec).
                int32 LeaderSpecIdx = INDEX_NONE;
                for (int32 s = 0; s < Template.Members.Num(); ++s) {
                    if (Template.Members[s].bIsLeader) {
                        LeaderSpecIdx = s;
                        break;
                    }
                }
                if (LeaderSpecIdx == INDEX_NONE && Template.Members.Num() > 0) {
                    LeaderSpecIdx = 0;
                }
                if (LeaderSpecIdx != INDEX_NONE) {
                    AppendSpec(Template.Members[LeaderSpecIdx], /*bSpecIsLeader=*/true);
                }
                for (int32 s = 0; s < Template.Members.Num(); ++s) {
                    if (s == LeaderSpecIdx) {
                        continue;
                    }
                    AppendSpec(Template.Members[s], /*bSpecIsLeader=*/false);
                }

                // A group needs at least 2 members to be a "group" (and to have any social edge). A clipped 1-member
                // group is just an ambient NPC the population spawner already covers — skip it so we don't burn the
                // group budget on a degenerate cluster.
                if (ResolvedMembers.Num() < 2) {
                    continue;
                }

                // Ensure exactly one leader survives clipping (the leader spec might have produced 0 if headroom was 0
                // when it ran — but we ran it first, so member 0 is the leader; if somehow none flagged, flag member 0).
                bool bHasLeader = false;
                for (const FMythicResolvedMember &M : ResolvedMembers) {
                    bHasLeader |= M.bIsLeader;
                }
                if (!bHasLeader) {
                    ResolvedMembers[0].bIsLeader = true;
                }

                // ─── Allocate a deterministic-per-spawn GroupId + build each member's spawn data ───
                const int32 LeaderSerial = PersistentRegistry->AllocateSpawnSerial();
                const uint32 GroupId = HashCombine(GetTypeHash(CandidateCell),
                                                   HashCombine(GroupIdSalt, static_cast<uint32>(LeaderSerial)));

                const int32 SpanStart = SpawnData.Num();
                for (int32 m = 0; m < ResolvedMembers.Num(); ++m) {
                    const FMythicResolvedMember &RM = ResolvedMembers[m];

                    FMythicGroupMemberSpawnData D;
                    D.Identity.Faction = Settlement.GoverningFaction;
                    D.Identity.Cell = CandidateCell;

                    // Globally-unique, never-reused serial → collision-free NameHash (same model as the other spawners).
                    // The leader reuses the serial already allocated for the GroupId; the rest allocate fresh.
                    const int32 SpawnSerial = (m == 0) ? LeaderSerial : PersistentRegistry->AllocateSpawnSerial();
                    D.Identity.NameHash = FMythicNPCGenerator::GenerateNameHash(
                        Settlement.GoverningFaction.Index, CandidateCell, SpawnSerial);
                    D.Identity.VisualArchetype = FMythicNPCGenerator::GenerateVisualArchetype(D.Identity.NameHash, 8);
                    D.Identity.DemographicFlags = FMythicNPCGenerator::GenerateDemographicFlags(
                        D.Identity.NameHash, FactionData.Population > 50);
                    D.Identity.RoleTag = RM.RoleTag;

                    // Members share Identity.Cell → existing scatter cohesion clusters them; the schedule anchors them to
                    // the cell (the AIController drives local movement once embodied). No new placement call.
                    D.Schedule.Phase = EMythicSchedulePhase::Social;
                    D.Schedule.HomeCell = CandidateCell;
                    D.Schedule.WorkCell = CandidateCell;

                    D.Significance.Tier = EMythicSignificanceTier::Tier0_Ambient;

                    D.ActivityTag = Template.GroupTag;
                    D.GroupId = GroupId;
                    D.bIsLeader = RM.bIsLeader;

                    SpawnData.Add(MoveTemp(D));
                }

                const int32 SpawnedThisGroup = SpawnData.Num() - SpanStart;

                FMythicGroupSpan Span;
                Span.Start = SpanStart;
                Span.Count = SpawnedThisGroup;
                Span.Relation = Template.IntraRelation;
                Span.EdgeStrength = FMath::Clamp(Template.IntraEdgeStrength, 0.0f, 1.0f);
                GroupSpans.Add(Span);

                // Charge budgets + per-cell bookkeeping so a second group in the same cell this tick can't overflow it.
                CellEntityCounts.FindOrAdd(CandidateCell) += SpawnedThisGroup;
                MemberBudget -= SpawnedThisGroup;
                --GroupBudget;
                ++ActiveGroupCount;
            }
        }
    }

    if (SpawnData.IsEmpty()) {
        return;
    }

    // ─── Step 4: Deferred batch-create + social wiring (handles exist only inside the lambda) ───
    const double WorldTime = World->GetTimeSeconds();
    Context.Defer().PushCommand<FMassDeferredCreateCommand>(
        [SpawnData = MoveTemp(SpawnData), GroupSpans = MoveTemp(GroupSpans), LWS, WorldTime](FMassEntityManager &Manager) {
            TRACE_CPUPROFILER_EVENT_SCOPE(MythicGroupSpawner_DeferredCreate);

            const UScriptStruct *Composition[] = {
                FMythicIdentityFragment::StaticStruct(),
                FMythicScheduleFragment::StaticStruct(),
                FMythicSignificanceFragment::StaticStruct(),
                FMythicGroupFragment::StaticStruct(),
                FMythicNPCTag::StaticStruct(),       // embody as the humanoid EmbodiedNPCClass + shared per-cell cap
                FMythicGroupMemberTag::StaticStruct() // KIND marker (debugger tally / archetype filtering)
            };
            FMassArchetypeHandle Archetype = Manager.CreateArchetype(MakeArrayView(Composition));

            TArray<FMassEntityHandle> Spawned;
            Manager.BatchCreateEntities(Archetype, SpawnData.Num(), Spawned);
            for (int32 i = 0; i < SpawnData.Num(); ++i) {
                const FMythicGroupMemberSpawnData &D = SpawnData[i];
                const FMassEntityHandle E = Spawned[i];
                Manager.GetFragmentDataChecked<FMythicIdentityFragment>(E) = D.Identity;
                Manager.GetFragmentDataChecked<FMythicScheduleFragment>(E) = D.Schedule;
                Manager.GetFragmentDataChecked<FMythicSignificanceFragment>(E) = D.Significance;

                FMythicGroupFragment &GroupFrag = Manager.GetFragmentDataChecked<FMythicGroupFragment>(E);
                GroupFrag.GroupId = D.GroupId;
                GroupFrag.ActivityTag = D.ActivityTag;
                GroupFrag.bIsLeader = D.bIsLeader ? 1 : 0;
            }

            // Wire intra-group social edges. Leader-anchored relations (Subordinate) orient toward the leader; symmetric
            // relations (Friend/Associate) get both directions. <= MaxGroupMembers members ⇒ <= MaxGroupMembers-1
            // outgoing edges per member, under the social graph's MaxEdgesPerEntity. The graph is self-locked
            // (write-locked here); this runs on the game thread during the command flush, and LWS (a GameInstance
            // subsystem) outlives the frame.
            UMythicSocialGraph *SocialGraph = LWS ? LWS->GetSocialGraph() : nullptr;
            if (SocialGraph) {
                for (const FMythicGroupSpan &Span : GroupSpans) {
                    for (int32 a = 0; a < Span.Count; ++a) {
                        const int32 IdxA = Span.Start + a;
                        if (!Spawned.IsValidIndex(IdxA)) {
                            continue;
                        }
                        const FMassEntityHandle EA = Spawned[IdxA];
                        const bool bAIsLeader = SpawnData[IdxA].bIsLeader;
                        for (int32 b = 0; b < Span.Count; ++b) {
                            if (a == b) {
                                continue;
                            }
                            const int32 IdxB = Span.Start + b;
                            if (!Spawned.IsValidIndex(IdxB)) {
                                continue;
                            }
                            const FMassEntityHandle EB = Spawned[IdxB];
                            const bool bBIsLeader = SpawnData[IdxB].bIsLeader;

                            if (Span.Relation == EMythicSocialRelation::Subordinate) {
                                // Authority relation: only NON-leaders point AT the leader (subordinate → superior). A
                                // member that isn't the source's superior gets no Subordinate edge.
                                if (!bAIsLeader && bBIsLeader) {
                                    SocialGraph->AddOrStrengthenEdge(EA, EB, Span.Relation, Span.EdgeStrength,
                                                                     WorldTime, SpawnData[IdxB].Identity.Faction);
                                }
                            } else {
                                // Symmetric peer relation (Friend/Associate/etc.): every ordered pair gets an edge.
                                SocialGraph->AddOrStrengthenEdge(EA, EB, Span.Relation, Span.EdgeStrength,
                                                                 WorldTime, SpawnData[IdxB].Identity.Faction);
                            }
                        }
                    }
                }
            }
        });
}

// ─────────────────────────────────────────────────────────────
// Static helpers (pure, deterministic, unit-testable)
// ─────────────────────────────────────────────────────────────

bool UMythicGroupSpawnerProcessor::TemplateEligible(const FMythicGroupTemplate &Template,
                                                    const FMythicFactionData &Faction,
                                                    EMythicSettlementEconomy EffEconomy) {
    // A zero-weight or member-less template can never be drawn.
    if (Template.RelativeWeight <= 0.0f || Template.Members.Num() == 0) {
        return false;
    }
    if (Faction.MilitaryStrength < Template.MinFactionMilitaryStrength) {
        return false;
    }
    if (Faction.Population < Template.MinFactionPopulation) {
        return false;
    }
    if (Faction.Reserves.Wealth < Template.MinReserveWealth) {
        return false;
    }
    // Economy gate: empty list = any economy; otherwise the resolved economy must be in the list.
    if (Template.AllowedEconomies.Num() > 0 && !Template.AllowedEconomies.Contains(EffEconomy)) {
        return false;
    }
    return true;
}

bool UMythicGroupSpawnerProcessor::PickTemplateIndex(const TArray<FMythicGroupTemplate> &Templates,
                                                     EMythicSettlementEconomy EffEconomy,
                                                     const FMythicFactionData &Faction, uint32 Seed,
                                                     int32 &OutIndex) {
    // Single pass to total eligible weight, second cumulative walk to pick — allocation-free, identical predicate both
    // passes so the winner-selection is consistent with the total.
    float Total = 0.0f;
    for (const FMythicGroupTemplate &T : Templates) {
        if (TemplateEligible(T, Faction, EffEconomy)) {
            Total += FMath::Max(0.0f, T.RelativeWeight);
        }
    }
    if (Total <= UE_KINDA_SMALL_NUMBER) {
        return false;
    }

    const float Roll = (static_cast<float>(Seed & 0xFFFFFFu) / 16777216.0f) * Total; // 2^24

    float Cumulative = 0.0f;
    int32 LastEligible = INDEX_NONE;
    for (int32 i = 0; i < Templates.Num(); ++i) {
        const FMythicGroupTemplate &T = Templates[i];
        if (!TemplateEligible(T, Faction, EffEconomy)) {
            continue;
        }
        const float W = FMath::Max(0.0f, T.RelativeWeight);
        if (W <= 0.0f) {
            continue;
        }
        LastEligible = i;
        Cumulative += W;
        if (Roll < Cumulative) {
            OutIndex = i;
            return true;
        }
    }

    // Floating-point boundary guard: Roll can equal Total — fall to the last eligible template.
    if (LastEligible != INDEX_NONE) {
        OutIndex = LastEligible;
        return true;
    }
    return false;
}

int32 UMythicGroupSpawnerProcessor::RollMemberCount(const FMythicGroupMemberSpec &Spec, uint32 Seed) {
    const int32 Min = FMath::Max(1, Spec.MinCount);
    const int32 Max = FMath::Max(Min, Spec.MaxCount);
    if (Max == Min) {
        return Min;
    }
    // Salt by the spec's role so two specs in one template (same Seed base) roll independently.
    const uint32 Mixed = HashCombine(Seed, GetTypeHash(Spec.RoleTag));
    const int32 Range = (Max - Min) + 1;
    return Min + static_cast<int32>(Mixed % static_cast<uint32>(Range));
}
