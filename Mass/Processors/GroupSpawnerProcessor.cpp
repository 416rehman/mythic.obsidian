
#include "Mass/Processors/GroupSpawnerProcessor.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "MassCommandBuffer.h"
#include "MassCommands.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Mass/Tags/MythicMassTags.h"
#include "Mass/Processors/PopulationSpawnerProcessor.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "World/LivingWorld/Groups/GroupTypes.h"
#include "World/LivingWorld/NPCGeneration/NPCGenerator.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h"
#include "World/LivingWorld/Social/SocialGraph.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

namespace {
    constexpr uint32 GroupChanceSalt = 0x47727043u;
    constexpr uint32 GroupPickSalt = 0x47727050u;
    constexpr uint32 GroupIdSalt = 0x47727000u;
}

UMythicGroupSpawnerProcessor::UMythicGroupSpawnerProcessor() {
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
    bRequiresGameThreadExecution = true;
    bAutoRegisterWithProcessingPhases = true;

    QueryBasedPruning = EMassQueryBasedPruning::Never;

    ExecutionOrder.ExecuteAfter.Add(TEXT("UMythicPopulationSpawnerProcessor"));

    ExistingNPCQuery.RegisterWithProcessor(*this);
    ExistingGroupQuery.RegisterWithProcessor(*this);
}

void UMythicGroupSpawnerProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    ExistingNPCQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    ExistingNPCQuery.AddRequirement<FMythicSignificanceFragment>(EMassFragmentAccess::ReadOnly);
    ExistingNPCQuery.AddTagRequirement<FMythicNPCTag>(EMassFragmentPresence::All);
    ExistingNPCQuery.AddTagRequirement<FMythicEncounterEntityTag>(EMassFragmentPresence::None);

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

    TimeSinceLastTick += Context.GetDeltaTimeSeconds();
    if (TimeSinceLastTick < Settings->GroupSpawnIntervalSeconds) {
        return;
    }
    TimeSinceLastTick = 0.0f;

    if (Settings->MaxGroupSpawnsPerTick <= 0 || Settings->MaxGroupMemberSpawnsPerTick <= 0) {
        return;
    }

    UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid();
    UMythicFactionDatabase *FactionDB = LWS->GetFactionDatabase();
    if (!Grid || !FactionDB) {
        return;
    }

    UMythicPersistentNPCRegistry *PersistentRegistry = LWS->GetPersistentNPCRegistry();
    if (!PersistentRegistry) {
        return;
    }

    TArray<FMythicGroupTemplate> TemplatesArr;
    if (const UMythicGroupTemplateDatabase *DB = Settings->GroupTemplateDatabase.LoadSynchronous()) {
        TemplatesArr = DB->Templates;
    } else {
        MythicGroupDefaults::BuildDefaultTemplates(TemplatesArr);
    }
    if (TemplatesArr.Num() == 0) {
        return;
    }

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

    const float SpawnRadiusSq = FMath::Square(Settings->GroupSpawnRadius);
    const int32 SpawnRadiusCells = FMath::CeilToInt(Settings->GroupSpawnRadius);

    int32 GroupBudget = Settings->MaxGroupSpawnsPerTick;
    int32 MemberBudget = Settings->MaxGroupMemberSpawnsPerTick;
    const int32 PerCellCap = FMath::Max(0, Settings->MaxEntitiesPerCell);

    struct FMythicGroupMemberSpawnData {
        FMythicIdentityFragment Identity;
        FMythicScheduleFragment Schedule;
        FMythicSignificanceFragment Significance;
        FGameplayTag ActivityTag;
        uint32 GroupId = 0;
        bool bIsLeader = false;
    };

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
                    DY = SpawnRadiusCells + 1;
                    break;
                }

                const FMythicCellCoord CandidateCell(PlayerCell.X + DX, PlayerCell.Y + DY);
                if (!Grid->IsValidCoord(CandidateCell)) {
                    continue;
                }

                if (ConsideredCells.Contains(CandidateCell)) {
                    continue;
                }
                ConsideredCells.Add(CandidateCell);

                FMythicSettlementData Settlement;
                if (!LWS->CopySettlementAtCell(CandidateCell, Settlement) || !Settlement.GoverningFaction.IsValid()) {
                    continue;
                }
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

                const uint32 ChanceSeed = FMythicNPCGenerator::GenerateNameHash(
                    Settlement.GoverningFaction.Index, CandidateCell, static_cast<int32>(GroupChanceSalt));
                const float ChanceRoll = static_cast<float>(ChanceSeed & 0xFFFFFFu) / 16777216.0f;
                if (ChanceRoll >= Settings->GroupSpawnChancePerCell) {
                    continue;
                }

                const EMythicSettlementEconomy EffEconomy =
                    UMythicPopulationSpawnerProcessor::ResolveEconomy(Settlement.Economy, FactionData.BaseProduction);

                const uint32 PickSeed = FMythicNPCGenerator::GenerateNameHash(
                    Settlement.GoverningFaction.Index, CandidateCell, static_cast<int32>(GroupPickSalt));
                int32 TemplateIndex = INDEX_NONE;
                if (!PickTemplateIndex(TemplatesArr, EffEconomy, FactionData, PickSeed, TemplateIndex)) {
                    continue;
                }
                const FMythicGroupTemplate &Template = TemplatesArr[TemplateIndex];

                const int32 CurrentCount = CellEntityCounts.FindRef(CandidateCell);
                const int32 CellHeadroom = FMath::Max(0, PerCellCap - CurrentCount);
                int32 RemainingGroupSlots = FMath::Min(MaxGroupMembers, FMath::Min(CellHeadroom, MemberBudget));
                if (RemainingGroupSlots <= 0) {
                    continue;
                }

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
                        M.bIsLeader = bSpecIsLeader && (c == 0);
                        ResolvedMembers.Add(M);
                        --RemainingGroupSlots;
                    }
                };

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
                    AppendSpec(Template.Members[LeaderSpecIdx],true);
                }
                for (int32 s = 0; s < Template.Members.Num(); ++s) {
                    if (s == LeaderSpecIdx) {
                        continue;
                    }
                    AppendSpec(Template.Members[s],false);
                }

                if (ResolvedMembers.Num() < 2) {
                    continue;
                }

                bool bHasLeader = false;
                for (const FMythicResolvedMember &M : ResolvedMembers) {
                    bHasLeader |= M.bIsLeader;
                }
                if (!bHasLeader) {
                    ResolvedMembers[0].bIsLeader = true;
                }

                const int32 LeaderSerial = PersistentRegistry->AllocateSpawnSerial();
                const uint32 GroupId = HashCombine(GetTypeHash(CandidateCell),
                                                   HashCombine(GroupIdSalt, static_cast<uint32>(LeaderSerial)));

                const int32 SpanStart = SpawnData.Num();
                for (int32 m = 0; m < ResolvedMembers.Num(); ++m) {
                    const FMythicResolvedMember &RM = ResolvedMembers[m];

                    FMythicGroupMemberSpawnData D;
                    D.Identity.Faction = Settlement.GoverningFaction;
                    D.Identity.Cell = CandidateCell;

                    const int32 SpawnSerial = (m == 0) ? LeaderSerial : PersistentRegistry->AllocateSpawnSerial();
                    D.Identity.NameHash = FMythicNPCGenerator::GenerateNameHash(
                        Settlement.GoverningFaction.Index, CandidateCell, SpawnSerial);
                    D.Identity.VisualArchetype = FMythicNPCGenerator::GenerateVisualArchetype(D.Identity.NameHash, 8);
                    D.Identity.DemographicFlags = FMythicNPCGenerator::GenerateDemographicFlags(
                        D.Identity.NameHash, FactionData.Population > 50);
                    D.Identity.RoleTag = RM.RoleTag;

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

    const double WorldTime = World->GetTimeSeconds();
    Context.Defer().PushCommand<FMassDeferredCreateCommand>(
        [SpawnData = MoveTemp(SpawnData), GroupSpans = MoveTemp(GroupSpans), LWS, WorldTime](FMassEntityManager &Manager) {
            TRACE_CPUPROFILER_EVENT_SCOPE(MythicGroupSpawner_DeferredCreate);

            const UScriptStruct *Composition[] = {
                FMythicIdentityFragment::StaticStruct(),
                FMythicScheduleFragment::StaticStruct(),
                FMythicSignificanceFragment::StaticStruct(),
                FMythicGroupFragment::StaticStruct(),
                FMythicNPCTag::StaticStruct(),
                FMythicGroupMemberTag::StaticStruct()
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
                                if (!bAIsLeader && bBIsLeader) {
                                    SocialGraph->AddOrStrengthenEdge(EA, EB, Span.Relation, Span.EdgeStrength,
                                                                     WorldTime, SpawnData[IdxB].Identity.Faction);
                                }
                            } else {
                                SocialGraph->AddOrStrengthenEdge(EA, EB, Span.Relation, Span.EdgeStrength,
                                                                 WorldTime, SpawnData[IdxB].Identity.Faction);
                            }
                        }
                    }
                }
            }
        });
}


bool UMythicGroupSpawnerProcessor::TemplateEligible(const FMythicGroupTemplate &Template,
                                                    const FMythicFactionData &Faction,
                                                    EMythicSettlementEconomy EffEconomy) {
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
    if (Template.AllowedEconomies.Num() > 0 && !Template.AllowedEconomies.Contains(EffEconomy)) {
        return false;
    }
    return true;
}

bool UMythicGroupSpawnerProcessor::PickTemplateIndex(const TArray<FMythicGroupTemplate> &Templates,
                                                     EMythicSettlementEconomy EffEconomy,
                                                     const FMythicFactionData &Faction, uint32 Seed,
                                                     int32 &OutIndex) {
    float Total = 0.0f;
    for (const FMythicGroupTemplate &T : Templates) {
        if (TemplateEligible(T, Faction, EffEconomy)) {
            Total += FMath::Max(0.0f, T.RelativeWeight);
        }
    }
    if (Total <= UE_KINDA_SMALL_NUMBER) {
        return false;
    }

    const float Roll = (static_cast<float>(Seed & 0xFFFFFFu) / 16777216.0f) * Total;

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
    const uint32 Mixed = HashCombine(Seed, GetTypeHash(Spec.RoleTag));
    const int32 Range = (Max - Min) + 1;
    return Min + static_cast<int32>(Mixed % static_cast<uint32>(Range));
}
