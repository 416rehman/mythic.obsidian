
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
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

UMythicTravelerSpawnerProcessor::UMythicTravelerSpawnerProcessor() {
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
    bRequiresGameThreadExecution = true;
    bAutoRegisterWithProcessingPhases = true;

    QueryBasedPruning = EMassQueryBasedPruning::Never;

    ExecutionOrder.ExecuteAfter.Add(TEXT("UMythicPopulationSpawnerProcessor"));

    ActiveTravelerQuery.RegisterWithProcessor(*this);
}

void UMythicTravelerSpawnerProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    ActiveTravelerQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    ActiveTravelerQuery.AddTagRequirement<FMythicTravelerTag>(EMassFragmentPresence::All);
}

FMythicCellCoord UMythicTravelerSpawnerProcessor::StepToward(FMythicCellCoord From, FMythicCellCoord To) {
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

    UMythicPersistentNPCRegistry *PersistentRegistry = LWS->GetPersistentNPCRegistry();
    if (!PersistentRegistry) {
        return;
    }

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

    TArray<int32> AllSettlementIds;
    LWS->CopyAllSettlementIds(AllSettlementIds);
    if (AllSettlementIds.Num() < 2) {
        return;
    }

    struct FMythicTravelerSettlement {
        int32 Id = INDEX_NONE;
        FMythicCellCoord Center;
        FMythicFactionId Faction;
    };
    TArray<FMythicTravelerSettlement> Settlements;
    Settlements.Reserve(AllSettlementIds.Num());
    TArray<int32> NearPlayerOriginIdx;

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

    const double WorldTime = World->GetTimeSeconds();
    const float Interval = FMath::Max(Settings->TravelerSpawnIntervalSeconds, UE_KINDA_SMALL_NUMBER);
    const uint32 WindowIndex = static_cast<uint32>(FMath::FloorToInt(static_cast<float>(WorldTime) / Interval));

    struct FMythicTravelerSpawnData {
        FMythicIdentityFragment Identity;
        FMythicScheduleFragment Schedule;
        FMythicSignificanceFragment Significance;
        FMythicTravelerFragment Traveler;
    };
    TArray<FMythicTravelerSpawnData> SpawnDataArray;
    SpawnDataArray.Reserve(SpawnSlots);

    const float CaravanRatio = FMath::Clamp(Settings->CaravanPatrolRatio, 0.0f, 1.0f);

    const FGameplayTag CaravanRole = Settings->CaravanRoleTag.IsValid() ? Settings->CaravanRoleTag : TAG_NPC_ROLE_MERCHANT;
    const FGameplayTag PatrolRole = Settings->PatrolRoleTag.IsValid() ? Settings->PatrolRoleTag : TAG_NPC_ROLE_GUARD;

    for (int32 Slot = 0; Slot < SpawnSlots; ++Slot) {
        const uint32 OriginSeed = HashCombine(HashCombine(WindowIndex, 0x4F726967u),
                                              static_cast<uint32>(Slot));
        const int32 OriginIdx = NearPlayerOriginIdx[OriginSeed % static_cast<uint32>(NearPlayerOriginIdx.Num())];
        const FMythicTravelerSettlement &Origin = Settlements[OriginIdx];

        const uint32 KindSeed = HashCombine(HashCombine(OriginSeed, static_cast<uint32>(Origin.Id)), 0x4B696E64u);
        const float KindRoll = static_cast<float>(KindSeed & 0xFFFFFFu) / 16777216.0f;
        const bool bCaravan = (KindRoll < CaravanRatio);

        int32 BestDestIdx = INDEX_NONE;
        int32 PreferredCount = 0;
        auto IsEligible = [&](const FMythicTravelerSettlement &Dest) -> bool {
            if (Dest.Id == Origin.Id) {
                return false;
            }
            const int32 Manhattan = FMath::Abs(Dest.Center.X - Origin.Center.X) + FMath::Abs(Dest.Center.Y - Origin.Center.Y);
            if (Manhattan > Settings->MaxRouteCellLength) {
                return false;
            }
            if (bCaravan) {
                const EMythicFactionRelation Rel = FactionDB->GetRelationship(Origin.Faction, Dest.Faction);
                return (Rel != EMythicFactionRelation::Hostile);
            }
            return (Dest.Faction == Origin.Faction);
        };

        for (int32 d = 0; d < Settlements.Num(); ++d) {
            if (IsEligible(Settlements[d])) {
                ++PreferredCount;
            }
        }
        if (PreferredCount == 0) {
            continue;
        }

        const uint32 DestSeed = HashCombine(HashCombine(OriginSeed, static_cast<uint32>(Origin.Id)), 0x44657374u);
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

        FMythicTravelerSpawnData SpawnData;
        SpawnData.Identity.Faction = Origin.Faction;
        SpawnData.Identity.Cell = Origin.Center;

        const int32 SpawnSerial = PersistentRegistry->AllocateNameSeedSerial();
        SpawnData.Identity.NameSeed = FMythicNPCGenerator::GenerateNameHash(
            Origin.Faction.Index, Origin.Center, SpawnSerial);
        SpawnData.Identity.EntityId = PersistentRegistry->AllocateEntityIdentity(
            SpawnData.Identity.NameSeed,
            EMythicEntityIdentityProvenance::RouteTraveler);
        if (!SpawnData.Identity.EntityId.IsValid()) {
            continue;
        }
        SpawnData.Identity.VisualArchetype = FMythicNPCGenerator::GenerateVisualArchetype(SpawnData.Identity.NameSeed, 8);
        SpawnData.Identity.DemographicFlags = FMythicNPCGenerator::GenerateDemographicFlags(SpawnData.Identity.NameSeed, true);
        SpawnData.Identity.RoleTag = bCaravan ? CaravanRole : PatrolRole;

        const FMythicCellCoord FirstStep = StepToward(Origin.Center, Dest.Center);
        SpawnData.Schedule.Phase = EMythicSchedulePhase::Work;
        SpawnData.Schedule.HomeCell = Origin.Center;
        SpawnData.Schedule.WorkCell = FirstStep;

        SpawnData.Significance.Tier = EMythicSignificanceTier::Tier0_Ambient;

        const int32 Manhattan = FMath::Abs(Dest.Center.X - Origin.Center.X) + FMath::Abs(Dest.Center.Y - Origin.Center.Y);
        SpawnData.Traveler.OriginCell = Origin.Center;
        SpawnData.Traveler.DestinationCell = Dest.Center;
        SpawnData.Traveler.DestinationSettlementId = Dest.Id;
        SpawnData.Traveler.Kind = bCaravan ? 0 : 1;
        SpawnData.Traveler.TimeSinceStepSeconds = 0.0f;
        SpawnData.Traveler.StepsRemaining = static_cast<uint16>(FMath::Clamp(Manhattan + 8, 0, 0xFFFF));

        SpawnDataArray.Add(MoveTemp(SpawnData));
    }

    if (SpawnDataArray.Num() > 0) {
        Context.Defer().PushCommand<FMassDeferredCreateCommand>([SpawnDataArray](FMassEntityManager &Manager) {
            TRACE_CPUPROFILER_EVENT_SCOPE(MythicTravelerSpawner_DeferredSpawn);

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
