
#include "Mass/Processors/PopulationSpawnerProcessor.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "MassCommandBuffer.h"
#include "MassCommands.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Mass/Tags/MythicMassTags.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "AI/Party/PartySubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Settlements/SettlementRegistry.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "World/LivingWorld/NPCGeneration/NPCGenerator.h"
#include "World/LivingWorld/Roles/RoleTypes.h"
#include "World/LivingWorld/Roles/ArchetypeTypes.h"
#include "World/LivingWorld/Territory/MythicBiome.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h"
#include "World/EnvironmentController/MythicEnvironmentSubsystem.h"
#include "World/EnvironmentController/MythicEnvironmentController.h"
#include "GameFramework/PlayerController.h"
#include "World/GameDirector/MythicPacingDirectorSubsystem.h"
#include "Engine/World.h"

namespace {
    EMythicSpawnPointPurpose PurposeForRole(const FGameplayTag &RoleTag) {
        if (RoleTag == TAG_NPC_ROLE_GUARD || RoleTag == TAG_NPC_ROLE_SOLDIER) {
            return EMythicSpawnPointPurpose::Guard;
        }
        if (RoleTag == TAG_NPC_ROLE_BANDIT) {
            return EMythicSpawnPointPurpose::Enemy;
        }
        return EMythicSpawnPointPurpose::Civilian;
    }

    const FMythicSpawnPoint *PickPointForCell(const FMythicSettlementData &Settlement, const FMythicCellCoord &Cell,
                                              EMythicSpawnPointPurpose Desired, uint32 NameHash) {
        if (Settlement.SpawnPoints.Num() == 0) {
            return nullptr;
        }

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
}

UMythicPopulationSpawnerProcessor::UMythicPopulationSpawnerProcessor() {
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
    bRequiresGameThreadExecution = true;
    bAutoRegisterWithProcessingPhases = true;

    QueryBasedPruning = EMassQueryBasedPruning::Never;

    ExistingNPCQuery.RegisterWithProcessor(*this);
}

void UMythicPopulationSpawnerProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    ExistingNPCQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    ExistingNPCQuery.AddRequirement<FMythicSignificanceFragment>(EMassFragmentAccess::ReadOnly);
    ExistingNPCQuery.AddTagRequirement<FMythicNPCTag>(EMassFragmentPresence::All);
    ExistingNPCQuery.AddTagRequirement<FMythicEncounterEntityTag>(EMassFragmentPresence::None);
    ExistingNPCQuery.AddTagRequirement<FMythicTravelerTag>(EMassFragmentPresence::None);
}

void UMythicPopulationSpawnerProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicPopulationSpawner_Execute);

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

    const UMythicPartySubsystem *PartySubsystem = World->GetSubsystem<UMythicPartySubsystem>();

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

    UMythicPersistentNPCRegistry *PersistentRegistry = LWS->GetPersistentNPCRegistry();
    if (!PersistentRegistry) {
        return;
    }

    const UMythicRoleDatabase *RoleDB = Settings->RoleDatabase.LoadSynchronous();

    TConstArrayView<FMythicArchetypeRow> ArchetypeCatalog;
    if (const UMythicArchetypeCatalog *Catalog = Settings->ArchetypeCatalog.LoadSynchronous()) {
        ArchetypeCatalog = Catalog->Archetypes;
    } else {
        ArchetypeCatalog = MythicArchetypeDefaults::GetCodeDefaultArchetypes();
    }

    float GameHour;
    const UMythicEnvironmentSubsystem *Env = GI->GetSubsystem<UMythicEnvironmentSubsystem>();
    if (Env && Env->GetEnvironmentController() != nullptr) {
        const FTimespan Timespan = Env->GetEnvironmentController()->GetTimespan();
        GameHour = Timespan.GetHours() + Timespan.GetMinutes() / 60.0f;
    } else {
        const float DayLengthSeconds = FMath::Max(1.0f, Settings->DayLengthSeconds);
        const float DayProgress = FMath::Fmod(static_cast<float>(World->GetTimeSeconds()), DayLengthSeconds) / DayLengthSeconds;
        GameHour = DayProgress * 24.0f;
    }
    const float DayFactor = 0.5f * (1.0f + FMath::Cos(2.0f * UE_PI * (GameHour - 12.0f) / 24.0f));

    const float MaxReserve = FMath::Max(1.0f, Settings->MaxReserve);

    const FGameplayTag HostileRoleTag =
        Settings->BanditRoleTag.IsValid() ? Settings->BanditRoleTag : TAG_NPC_ROLE_BANDIT;


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


    TMap<FMythicCellCoord, int32> CellEntityCounts;
    ExistingNPCQuery.ForEachEntityChunk(Context, [&CellEntityCounts](FMassExecutionContext &ChunkContext) {
        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();

        for (int32 i = 0; i < NumEntities; ++i) {
            CellEntityCounts.FindOrAdd(IdentityView[i].Cell)++;
        }
    });


    const float SpawnRadiusSq = FMath::Square(Settings->PopulationSpawnRadius);
    const int32 SpawnRadiusCells = FMath::CeilToInt(Settings->PopulationSpawnRadius);
    int32 SpawnBudget = Settings->MaxSpawnsPerTick;

    if (const UMythicPacingDirectorSubsystem *Pacing = World->GetSubsystem<UMythicPacingDirectorSubsystem>()) {
        const float Intensity = Pacing->GetSpawnIntensityMultiplier();
        if (Intensity > 0.0f && !FMath::IsNearlyEqual(Intensity, 1.0f)) {
            SpawnBudget = FMath::Max(1, FMath::RoundToInt(static_cast<float>(SpawnBudget) * Intensity));
        }
    }

    struct FMythicNPCPopulationSpawnData {
        FMythicIdentityFragment Identity;
        FMythicScheduleFragment Schedule;
        FMythicSignificanceFragment Significance;
    };
    TArray<FMythicNPCPopulationSpawnData> SpawnDataArray;
    SpawnDataArray.Reserve(SpawnBudget);

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

                if (ConsideredCells.Contains(CandidateCell)) {
                    continue;
                }
                ConsideredCells.Add(CandidateCell);

                FMythicSettlementData Settlement;
                if (!LWS->CopySettlementAtCell(CandidateCell, Settlement) || !Settlement.GoverningFaction.IsValid()) {
                    continue;
                }

                FMythicFactionData FactionData;
                if (!FactionDB->GetFaction(Settlement.GoverningFaction, FactionData)) {
                    continue;
                }

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

                const int32 ToSpawn = FMath::Min(Deficit, SpawnBudget);

                for (int32 SpawnIdx = 0; SpawnIdx < ToSpawn; ++SpawnIdx) {
                    FMythicNPCPopulationSpawnData SpawnData;
                    SpawnData.Identity.Faction = Settlement.GoverningFaction;
                    SpawnData.Identity.Cell = CandidateCell;

                    const int32 SpawnSerial = PersistentRegistry->AllocateSpawnSerial();

                    SpawnData.Identity.NameHash = FMythicNPCGenerator::GenerateNameHash(
                        Settlement.GoverningFaction.Index, CandidateCell, SpawnSerial);

                    SpawnData.Identity.VisualArchetype = FMythicNPCGenerator::GenerateVisualArchetype(
                        SpawnData.Identity.NameHash, 8);

                    SpawnData.Identity.DemographicFlags = FMythicNPCGenerator::GenerateDemographicFlags(
                        SpawnData.Identity.NameHash, FactionData.Population > 50);

                    FMythicArchetypeContext ArchCtx;
                    ArchCtx.WealthNorm = FMath::Clamp(FactionData.Reserves.Wealth / MaxReserve, 0.0f, 1.0f);
                    ArchCtx.Military = FMath::Clamp(FactionData.MilitaryStrength, 0.0f, 1.0f);
                    ArchCtx.Economy = ResolveEconomy(Settlement.Economy, FactionData.BaseProduction);
                    ArchCtx.Biome = Grid->GetBiomeAtCell(CandidateCell);
                    ArchCtx.DayFactor = DayFactor;
                    ArchCtx.FactionTag = FactionData.FactionTag;
                    ArchCtx.bWildernessContext = false;

                    const FMythicArchetypeRow *ChosenRow = nullptr;
                    const FGameplayTag DerivedRole =
                        DeriveArchetype(ArchetypeCatalog, ArchCtx, SpawnData.Identity.NameHash, ChosenRow);
                    SpawnData.Identity.RoleTag = Settlement.bIsHostileCamp
                        ? HostileRoleTag
                        : ApplyFactionGate(RoleDB, DerivedRole, FactionData.FactionTag);

                    if (const FMythicSpawnPoint *Point = PickPointForCell(
                            Settlement, CandidateCell, PurposeForRole(SpawnData.Identity.RoleTag),
                            SpawnData.Identity.NameHash)) {
                        SpawnData.Identity.SpawnOverridePos = Point->WorldLocation;
                        SpawnData.Identity.bHasSpawnOverride = true;
                    }

                    SpawnData.Schedule.Phase = EMythicSchedulePhase::Idle;
                    SpawnData.Schedule.HomeCell = CandidateCell;
                    SpawnData.Schedule.WorkCell = FMythicCellCoord(
                        CandidateCell.X + static_cast<int32>((SpawnData.Identity.NameHash >> 4) % 3) - 1,
                        CandidateCell.Y + static_cast<int32>((SpawnData.Identity.NameHash >> 8) % 3) - 1);

                    SpawnData.Significance.Tier = EMythicSignificanceTier::Tier0_Ambient;


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
                if (PartySubsystem && PartySubsystem->IsCompanionEntity(DespawnEntity)) {
                    continue;
                }
                if (AMythicNPCCharacter *Actor = LWS->FindEmbodiedActor(DespawnEntity)) {
                    Actor->Destroy();
                }
                LWS->UnregisterEmbodiedActor(DespawnEntity);
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
    const int32 EffectiveMax = FMath::Min(SettlementMaxDensity, SystemMaxPerCell);

    if (FactionCapacity <= 0) {
        return 0;
    }

    const float FillRatio = FMath::Clamp(static_cast<float>(FactionPopulation) / static_cast<float>(FactionCapacity), 0.0f, 1.0f);
    return FMath::CeilToInt(EffectiveMax * FillRatio);
}

EMythicSettlementEconomy UMythicPopulationSpawnerProcessor::ResolveEconomy(
    EMythicSettlementEconomy Authored,
    const FMythicResourceStock &FactionBaseProduction) {
    if (Authored != EMythicSettlementEconomy::Generic) {
        return Authored;
    }

    const float Food = FactionBaseProduction.Food;
    const float Materials = FactionBaseProduction.Materials;
    const float Arms = FactionBaseProduction.Arms;
    const float Wealth = FactionBaseProduction.Wealth;

    const float MaxVal = FMath::Max(FMath::Max(Food, Materials), FMath::Max(Arms, Wealth));
    if (MaxVal <= UE_KINDA_SMALL_NUMBER) {
        return EMythicSettlementEconomy::Generic;
    }

    if (Arms >= MaxVal) {
        return EMythicSettlementEconomy::Military;
    }
    if (Food >= MaxVal) {
        return EMythicSettlementEconomy::Farming;
    }
    if (Materials >= MaxVal) {
        return EMythicSettlementEconomy::Mining;
    }
    return EMythicSettlementEconomy::Trade;
}

FGameplayTag UMythicPopulationSpawnerProcessor::DeriveArchetype(
    TConstArrayView<FMythicArchetypeRow> Catalog,
    const FMythicArchetypeContext &Ctx,
    uint32 NameHash,
    const FMythicArchetypeRow *&OutChosen) {
    OutChosen = nullptr;

    const int32 EcoIdx = static_cast<int32>(Ctx.Economy);
    const int32 BiomeIdx = static_cast<int32>(Ctx.Biome);

    auto EffectiveWeight = [&Ctx, EcoIdx, BiomeIdx](const FMythicArchetypeRow &Row) -> float {
        if (!Row.RequiredFactionTags.IsEmpty()) {
            const FGameplayTagContainer FactionTagContainer(Ctx.FactionTag);
            if (!Row.RequiredFactionTags.HasAny(FactionTagContainer)) {
                return 0.0f;
            }
        }
        if (Ctx.bWildernessContext && (Row.bRequiresSettlement || !Row.bAllowedAlone)) {
            return 0.0f;
        }

        float W = FMath::Max(0.0f, Row.BaseWeight);
        if (W <= 0.0f) {
            return 0.0f;
        }

        W *= FMath::Lerp(1.0f, FMath::Max(0.0f, Row.WealthFavor), Ctx.WealthNorm);
        W *= FMath::Lerp(1.0f, FMath::Max(0.0f, Row.WealthDisfavor), 1.0f - Ctx.WealthNorm);

        {
            const float MF = FMath::Max(0.0f, Row.MilitaryFavor);
            W *= (MF > 1.0f) ? FMath::Lerp(1.0f / MF, MF, Ctx.Military)
                             : FMath::Lerp(1.0f, MF, Ctx.Military);
        }

        if (Row.EconomyWeights.IsValidIndex(EcoIdx)) {
            W *= FMath::Max(0.0f, Row.EconomyWeights[EcoIdx]);
        }
        if (Row.BiomeWeights.IsValidIndex(BiomeIdx)) {
            W *= FMath::Max(0.0f, Row.BiomeWeights[BiomeIdx]);
        }

        W *= FMath::Lerp(FMath::Max(0.0f, Row.NightWeight), FMath::Max(0.0f, Row.DayWeight),
                         FMath::Clamp(Ctx.DayFactor, 0.0f, 1.0f));

        return FMath::Max(0.0f, W);
    };

    float Total = 0.0f;
    for (const FMythicArchetypeRow &Row : Catalog) {
        Total += EffectiveWeight(Row);
    }

    if (Total <= UE_KINDA_SMALL_NUMBER) {
        return TAG_NPC_ROLE_CIVILIAN;
    }

    const uint32 Seed = HashCombine(NameHash, 0x41726368u);
    const float Roll = (static_cast<float>(Seed & 0xFFFFFFu) / 16777216.0f) * Total;

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
    if (!RoleDB || !DerivedRole.IsValid()) {
        return DerivedRole;
    }

    const FMythicRoleDefinition *RoleDef = RoleDB->FindRole(DerivedRole);
    if (!RoleDef || RoleDef->RequiredFactionTags.IsEmpty()) {
        return DerivedRole;
    }

    const FGameplayTagContainer FactionTagContainer(FactionTag);
    if (!RoleDef->RequiredFactionTags.HasAny(FactionTagContainer)) {
        return TAG_NPC_ROLE_CIVILIAN;
    }

    return DerivedRole;
}
