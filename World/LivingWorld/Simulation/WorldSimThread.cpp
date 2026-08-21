
#include "World/LivingWorld/Simulation/WorldSimThread.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Settlements/SettlementRegistry.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Morality/MoralSignature.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "World/LivingWorld/Simulation/SchemeEngine.h"
#include "HAL/PlatformProcess.h"

FMythicWorldSimThread::FMythicWorldSimThread() {}

FMythicWorldSimThread::~FMythicWorldSimThread() {
    StopThread();
}

void FMythicWorldSimThread::Setup(
    UMythicCausalFabric *InFabric,
    UMythicFactionDatabase *InFactionDB,
    UMythicTerritoryGrid *InTerritoryGrid,
    UMythicSettlementRegistry *InSettlementRegistry,
    const UMythicLivingWorldSettings *InSettings,
    float InTickIntervalSeconds,
    FCriticalSection *InSimulationLock,
    UMythicSchemeEngine *InSchemeEngine,
    TArray<FMythicWorldEvent> *InPendingEvents,
    FCriticalSection *InPendingEventsMutex
    ) {
    Fabric = InFabric;
    FactionDB = InFactionDB;
    TerritoryGrid = InTerritoryGrid;
    SettlementRegistry = InSettlementRegistry;
    Settings = InSettings;
    TickIntervalSeconds = InTickIntervalSeconds;
    SimulationLock = InSimulationLock;
    SchemeEngine = InSchemeEngine;
    PendingEvents = InPendingEvents;
    PendingEventsMutex = InPendingEventsMutex;

    MaxFactions = FactionDB ? FactionDB->GetMaxFactions() : 0;
    if (MaxFactions > 0) {
        TradeVolume.SetNumZeroed(MaxFactions * MaxFactions);
    }
}

void FMythicWorldSimThread::StartThread() {
    if (Thread) {
        UE_LOG(LogMythWorldSim, Warning, TEXT("World sim thread already running."));
        return;
    }

    bRunning.store(true, std::memory_order_relaxed);
    Thread = FRunnableThread::Create(
        this,
        TEXT("MythicWorldSimThread"),
        0,
        TPri_BelowNormal,
        FPlatformAffinity::GetPoolThreadMask()
        );

    UE_LOG(LogMythWorldSim, Log, TEXT("World sim thread started. Tick interval: %.2fs"), TickIntervalSeconds);
}

void FMythicWorldSimThread::StopThread() {
    if (!Thread) {
        return;
    }

    bRunning.store(false, std::memory_order_relaxed);
    Thread->WaitForCompletion();
    delete Thread;
    Thread = nullptr;

    UE_LOG(LogMythWorldSim, Log, TEXT("World sim thread stopped after %llu ticks."), TickCount);
}

bool FMythicWorldSimThread::Init() {
    UE_LOG(LogMythWorldSim, Log, TEXT("World sim thread initializing..."));
    return true;
}

uint32 FMythicWorldSimThread::Run() {
    while (bRunning.load(std::memory_order_relaxed)) {
        const double StartTime = FPlatformTime::Seconds();

        SimTick();

        const double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;

        if (TickCount % 100 == 0) {
            UE_LOG(LogMythWorldSim, Verbose, TEXT("Sim tick %llu completed in %.2fms"), TickCount, ElapsedMs);
        }

        ++TickCount;

        const double SleepTime = static_cast<double>(TickIntervalSeconds) - (FPlatformTime::Seconds() - StartTime);
        if (SleepTime > 0.0) {
            FPlatformProcess::SleepNoStats(static_cast<float>(SleepTime));
        }
    }

    return 0;
}

void FMythicWorldSimThread::Stop() {
    bRunning.store(false, std::memory_order_relaxed);
}

void FMythicWorldSimThread::Exit() {
    UE_LOG(LogMythWorldSim, Log, TEXT("World sim thread exiting."));
}

void FMythicWorldSimThread::SimTick() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicWorldSim_SimTick);

    FScopeLock Lock(SimulationLock);

    if (!Settings || !FactionDB) {
        return;
    }

    TickEconomy();
    TickPopulation();
    TickDiplomacy();
    TickTerritoryPropagation();
    TickIdeologyMetabolism();
    TickFactionEvolution();
    TickSchemeEngine();
    TickCrystallization();
    TickHistoryAppend();

    if (SettlementRegistry) {
        const double CurrentSimTime = FPlatformTime::Seconds();
        SettlementRegistry->TickShopSuccession(CurrentSimTime, Settings->ShopSuccessionDelaySeconds);

        SettlementRegistry->TickConquest(TerritoryGrid, FactionDB, Fabric, Settings->SettlementConquestThreshold);
    }

    if (PendingEvents && PendingEventsMutex && Fabric) {
        TArray<FMythicWorldEvent> EventsToProcess;
        {
            FScopeLock EventLock(PendingEventsMutex);
            EventsToProcess = *PendingEvents;
            PendingEvents->Reset();
        }
        for (const FMythicWorldEvent &Event : EventsToProcess) {
            Fabric->AppendEvent(Event);
        }
    }

    CommitAllSnapshots();

    OnWorldSimCommitted.Broadcast();
}

void FMythicWorldSimThread::CommitAllSnapshots() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicWorldSim_CommitSnapshots);

    if (Fabric) {
        Fabric->CommitWrites();
    }
    if (FactionDB) {
        FactionDB->CommitWrites();
    }
    if (TerritoryGrid) {
        TerritoryGrid->CommitWrites();
    }
}


void FMythicWorldSimThread::TickEconomy() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicWorldSim_Economy);

    const int32 FactionCount = FactionDB->GetRegisteredCount();
    const float RefCells = static_cast<float>(Settings->ReferenceCellCount);
    constexpr float Epsilon = 0.001f;

    for (int32 i = 0; i < FactionCount; ++i) {
        FMythicFactionData *F = FactionDB->GetFactionMutableByIndex(i);
        if (!F || !F->bAlive || !F->bHasEconomy) {
            continue;
        }

        FMythicResourceStock NewSupply;
        if (F->bControlsTerritory && F->ControlledCellCount > 0) {
            const float CellRatio = static_cast<float>(F->ControlledCellCount) / FMath::Max(RefCells, 1.0f);
            NewSupply.Food = F->BaseProduction.Food * CellRatio;
            NewSupply.Materials = F->BaseProduction.Materials * CellRatio;
            NewSupply.Arms = F->BaseProduction.Arms * CellRatio;
            NewSupply.Wealth = F->BaseProduction.Wealth * CellRatio;
        }


        if (F->bHasCivilianPopulation && F->Ideology.Authority > Settings->TaxAuthorityThreshold) {
            NewSupply.Wealth += static_cast<float>(F->Population) * Settings->TaxRate;
        }

        if (F->MilitaryStrength > Settings->ContractMilitaryThreshold &&
            F->Population < Settings->ContractPopulationCeiling) {
            NewSupply.Wealth += F->MilitaryStrength * Settings->TaxRate * 10.0f;
        }

        const bool bIsRaider = F->Ideology.Theft > Settings->RaidIdeologyThreshold ||
            (!F->bCanNegotiate && F->bHasEconomy);
        if (bIsRaider) {
            float NearbyTradeVolume = 0.0f;
            for (int32 j = 0; j < FactionCount; ++j) {
                if (j == i) {
                    continue;
                }
                NearbyTradeVolume += TradeVolume[i * MaxFactions + j];
            }
            const float RaidGain = NearbyTradeVolume * Settings->RaidFraction;
            NewSupply.Wealth += RaidGain * 0.7f;
            NewSupply.Materials += RaidGain * 0.3f;
        }

        const float ArmsProducible = FMath::Min(
            Settings->ArmsProductionRate,
            F->Reserves.Materials / FMath::Max(Settings->MaterialsPerArm, Epsilon)
            );
        if (ArmsProducible > 0.0f) {
            NewSupply.Arms += ArmsProducible;
            F->Reserves.Materials -= ArmsProducible * Settings->MaterialsPerArm;
        }

        F->Supply = NewSupply;

        FMythicResourceStock NewDemand;
        NewDemand.Food = static_cast<float>(F->Population) * Settings->FoodPerCapita;
        NewDemand.Arms = F->MilitaryStrength * Settings->ArmsUpkeepRate;
        NewDemand.Wealth = F->MilitaryStrength * Settings->WealthUpkeepRate;
        F->Demand = NewDemand;

        F->Reserves += F->Supply;
        F->Reserves -= F->Demand;
        F->Reserves.ClampAll(-Settings->MaxReserve, Settings->MaxReserve);

        for (int32 r = 0; r < ResourceTypeCount; ++r) {
            const EMythicResourceType Type = static_cast<EMythicResourceType>(r);
            const float S = FMath::Max(F->Supply.GetResource(Type), Epsilon);
            const float D = F->Demand.GetResource(Type);
            F->Prices.GetResourceMutable(Type) = D / S;
        }

        const float ArmsReserve = FMath::Max(F->Reserves.Arms, 0.0f);
        const float WealthForMil = FMath::Max(F->Reserves.Wealth, 0.0f);
        F->MilitaryStrength = FMath::Clamp(
            (ArmsReserve * 0.6f + WealthForMil * 0.4f) / FMath::Max(Settings->MaxReserve, 1.0f),
            0.0f, 1.0f
            );
    }

    for (int32 k = 0; k < TradeVolume.Num(); ++k) {
        TradeVolume[k] *= 0.95f;
    }

    for (int32 i = 0; i < FactionCount; ++i) {
        FMythicFactionData *A = FactionDB->GetFactionMutableByIndex(i);
        if (!A || !A->bAlive || !A->bParticipatesInTrade) {
            continue;
        }

        FMythicFactionId IdA;
        IdA.Index = static_cast<uint8>(i);

        for (int32 j = i + 1; j < FactionCount; ++j) {
            FMythicFactionData *B = FactionDB->GetFactionMutableByIndex(j);
            if (!B || !B->bAlive || !B->bParticipatesInTrade) {
                continue;
            }

            FMythicFactionId IdB;
            IdB.Index = static_cast<uint8>(j);

            const EMythicFactionRelation Rel = FactionDB->GetWriteRelationship(IdA, IdB);
            if (Rel == EMythicFactionRelation::Hostile || Rel == EMythicFactionRelation::Unfriendly) {
                continue;
            }

            float RelMod = 1.0f;
            if (Rel == EMythicFactionRelation::Allied) { RelMod = 0.7f; }
            else
                if (Rel == EMythicFactionRelation::Friendly) { RelMod = 0.85f; }

            for (int32 r = 0; r < ResourceTypeCount; ++r) {
                const EMythicResourceType Type = static_cast<EMythicResourceType>(r);
                const float SurplusA = A->Reserves.GetResource(Type) - Settings->TradeSurplusThreshold;
                const float DeficitB = Settings->TradeDeficitThreshold - B->Reserves.GetResource(Type);
                const float SurplusB = B->Reserves.GetResource(Type) - Settings->TradeSurplusThreshold;
                const float DeficitA = Settings->TradeDeficitThreshold - A->Reserves.GetResource(Type);

                if (SurplusA > 0.0f && DeficitB > 0.0f) {
                    const float TradeAmount = FMath::Min(SurplusA, DeficitB) * 0.5f;
                    A->Reserves.GetResourceMutable(Type) -= TradeAmount;
                    B->Reserves.GetResourceMutable(Type) += TradeAmount;
                    const float Cost = TradeAmount * A->Prices.GetResource(Type) * RelMod;
                    A->Reserves.Wealth += Cost;
                    B->Reserves.Wealth -= Cost;
                    TradeVolume[i * MaxFactions + j] += TradeAmount;
                    TradeVolume[j * MaxFactions + i] += TradeAmount;
                }

                if (SurplusB > 0.0f && DeficitA > 0.0f) {
                    const float TradeAmount = FMath::Min(SurplusB, DeficitA) * 0.5f;
                    B->Reserves.GetResourceMutable(Type) -= TradeAmount;
                    A->Reserves.GetResourceMutable(Type) += TradeAmount;
                    const float Cost = TradeAmount * B->Prices.GetResource(Type) * RelMod;
                    B->Reserves.Wealth += Cost;
                    A->Reserves.Wealth -= Cost;
                    TradeVolume[j * MaxFactions + i] += TradeAmount;
                    TradeVolume[i * MaxFactions + j] += TradeAmount;
                }
            }
        }
    }
}


void FMythicWorldSimThread::TickPopulation() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicWorldSim_Population);

    const int32 FactionCount = FactionDB->GetRegisteredCount();

    for (int32 i = 0; i < FactionCount; ++i) {
        FMythicFactionData *F = FactionDB->GetFactionMutableByIndex(i);
        if (!F || !F->bAlive) {
            continue;
        }

        if (F->bHasCivilianPopulation) {
            const int32 Capacity = F->ControlledCellCount * Settings->PopulationPerCell;
            const float FoodSufficiency = (F->Demand.Food > 0.001f)
                ? FMath::Clamp(F->Reserves.Food / (F->Demand.Food * 10.0f), 0.0f, 2.0f)
                : 1.0f;

            const int32 CapacityGap = FMath::Max(0, Capacity - F->Population);
            const int32 Births = static_cast<int32>(
                static_cast<float>(CapacityGap) * Settings->BaseBirthRate * FoodSufficiency
            );

            const float DeathModifier = FoodSufficiency < 0.3f ? 3.0f : 1.0f;
            const int32 Deaths = static_cast<int32>(
                static_cast<float>(F->Population) * Settings->BaseDeathRate * DeathModifier
            );

            int32 Emigrants = 0;
            if (FoodSufficiency < Settings->MigrationFoodThreshold) {
                Emigrants = static_cast<int32>(
                    static_cast<float>(F->Population) * Settings->MigrationRate *
                    (1.0f - FoodSufficiency / Settings->MigrationFoodThreshold)
                );
            }

            F->Population = FMath::Max(0, F->Population + Births - Deaths - Emigrants);
        }
        else if (!F->bHasEconomy) {
            F->Population = ComputeCappedSpawnPopulation(F->Population, F->ControlledCellCount, Settings->SpawnRatePerCell,
                                                         Settings->PopulationPerCell);
        }
        else {
            const float WealthAvail = FMath::Max(F->Reserves.Wealth, 0.0f);
            const int32 Recruits = static_cast<int32>(WealthAvail * Settings->RecruitmentPerWealth);
            F->Population += Recruits;

            const int32 Attrition = static_cast<int32>(
                static_cast<float>(F->Population) * Settings->BaseDeathRate
            );
            F->Population = FMath::Max(0, F->Population - Attrition);
        }

        if (F->Population > 0) {
            F->bHasBeenPopulated = true;
        }

        if (F->bHasBeenPopulated && F->Population <= 0 && F->ControlledCellCount <= 0) {
            FMythicFactionId DeadId;
            DeadId.Index = static_cast<uint8>(i);

            const float EscapeCapacity = F->Reserves.Wealth + F->Reserves.Arms;
            const int32 Survivors = FMath::Min(50, static_cast<int32>(EscapeCapacity * 0.5f));

            if (Survivors >= Settings->ResistancePopulationThreshold && F->Status == EMythicFactionStatus::Active) {
                FMythicFactionId ResistanceId = FactionDB->CreateFactionFromConquest(DeadId, Survivors);
                if (ResistanceId.IsValid()) {
                    UE_LOG(LogMythWorldSim, Log, TEXT("Faction '%s' destroyed, but %d survivors formed a Resistance! (New ID: %d)"),
                           *F->DisplayName.ToString(), Survivors, ResistanceId.Index);

                    if (Fabric) {
                        FMythicWorldEvent ResEvent;
                        ResEvent.EventTag = TAG_LIVINGWORLD_EVENT_FACTION_RESISTANCE;
                        ResEvent.PrimaryFaction = ResistanceId;
                        ResEvent.SecondaryFaction = DeadId;
                        ResEvent.Significance = 0.9f;
                        ResEvent.CategoryFlags = EMythicEventCategory::Diplomacy | EMythicEventCategory::Combat;
                        Fabric->AppendEvent(ResEvent);
                    }
                }
            }

            FactionDB->AnnihilateFaction(DeadId);

            if (Fabric) {
                FMythicWorldEvent AnnEvent;
                AnnEvent.EventTag = TAG_LIVINGWORLD_EVENT_FACTION_ANNIHILATION;
                AnnEvent.PrimaryFaction = DeadId;
                AnnEvent.Significance = 1.0f;
                AnnEvent.CategoryFlags = EMythicEventCategory::Diplomacy | EMythicEventCategory::Death;
                Fabric->AppendEvent(AnnEvent);
            }

            UE_LOG(LogMythWorldSim, Log, TEXT("Faction '%s' annihilated (pop=0, cells=0)"), *F->DisplayName.ToString());
        }

        if (Settings && FactionDB && F->Status == EMythicFactionStatus::Resistance &&
            F->ControlledCellCount >= Settings->RestorationCellThreshold) {
            FMythicFactionId RestoredId;
            RestoredId.Index = static_cast<uint8>(i);
            FactionDB->RestoreResistanceToFaction(RestoredId);

            if (Fabric) {
                FMythicWorldEvent RestEvent;
                RestEvent.EventTag = TAG_LIVINGWORLD_EVENT_FACTION_RESTORATION;
                RestEvent.PrimaryFaction = RestoredId;
                RestEvent.Significance = 0.85f;
                RestEvent.CategoryFlags = EMythicEventCategory::Diplomacy;
                Fabric->AppendEvent(RestEvent);
            }
        }
    }
}


EMythicFactionRelation FMythicWorldSimThread::MapDiplomacyScoreToRelation(
    float Score, EMythicFactionRelation Current,
    float AllyThreshold, float FriendlyThreshold, float UnfriendlyThreshold, float HostileThreshold, float Hyst) {
    if (Score > AllyThreshold + (Current == EMythicFactionRelation::Allied ? 0.0f : Hyst)) {
        return EMythicFactionRelation::Allied;
    }
    if (Score > FriendlyThreshold + (Current == EMythicFactionRelation::Friendly ? 0.0f : Hyst)) {
        return EMythicFactionRelation::Friendly;
    }
    if (Score < HostileThreshold - (Current == EMythicFactionRelation::Hostile ? 0.0f : Hyst)) {
        return EMythicFactionRelation::Hostile;
    }
    if (Score < UnfriendlyThreshold - (Current == EMythicFactionRelation::Unfriendly ? 0.0f : Hyst)) {
        return EMythicFactionRelation::Unfriendly;
    }
    return EMythicFactionRelation::Neutral;
}

float FMythicWorldSimThread::DiplomacyShiftSignificance(EMythicFactionRelation NewRelation) {
    switch (NewRelation) {
    case EMythicFactionRelation::Allied:     return 0.9f;
    case EMythicFactionRelation::Hostile:    return 0.9f;
    case EMythicFactionRelation::Friendly:   return 0.6f;
    case EMythicFactionRelation::Unfriendly: return 0.6f;
    case EMythicFactionRelation::Neutral:    return 0.4f;
    default:                                 return 0.5f;
    }
}

float FMythicWorldSimThread::DriftTowardClamped(float Current, float Target, float Rate) {
    return FMath::Clamp(Current + (Target - Current) * Rate, -1.0f, 1.0f);
}

bool FMythicWorldSimThread::ShouldFactionSchism(float InternalDivergence, float IdeologyThreshold, bool bGeographicallyFragmented,
                                                int32 Population, int32 MinSchismPopulation) {
    const bool bDestabilized = InternalDivergence > IdeologyThreshold || bGeographicallyFragmented;
    return bDestabilized && Population >= MinSchismPopulation * 2;
}

int32 FMythicWorldSimThread::ComputeCappedSpawnPopulation(int32 CurrentPop, int32 CellCount, int32 SpawnRatePerCell, int32 PopulationPerCell) {
    const int32 Growth = CellCount * SpawnRatePerCell;
    const int32 Capacity = CellCount * PopulationPerCell;
    return FMath::Min(CurrentPop + Growth, FMath::Max(CurrentPop, Capacity));
}

void FMythicWorldSimThread::TickDiplomacy() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicWorldSim_Diplomacy);

    const int32 FactionCount = FactionDB->GetRegisteredCount();

    TArray<FMythicWorldEvent> RecentEvents;
    if (Fabric) {
        const double CurrentTime = FPlatformTime::Seconds();
        Fabric->QueryEventsByCategory(
            EMythicEventCategory::Combat | EMythicEventCategory::Trade | EMythicEventCategory::Diplomacy,
            CurrentTime - 60.0, CurrentTime,
            Settings->DiplomacyEventScanCap,
            RecentEvents
            );
    }

    for (int32 i = 0; i < FactionCount; ++i) {
        FMythicFactionData *A = FactionDB->GetFactionMutableByIndex(i);
        if (!A || !A->bAlive || !A->bCanNegotiate) {
            continue;
        }

        FMythicFactionId IdA;
        IdA.Index = static_cast<uint8>(i);

        for (int32 j = i + 1; j < FactionCount; ++j) {
            FMythicFactionData *B = FactionDB->GetFactionMutableByIndex(j);
            if (!B || !B->bAlive || !B->bCanNegotiate) {
                continue;
            }

            FMythicFactionId IdB;
            IdB.Index = static_cast<uint8>(j);

            float IdeologyDist = 0.0f;
            for (int32 Axis = 0; Axis < MoralAxisCount; ++Axis) {
                const EMythicMoralAxis AxisEnum = static_cast<EMythicMoralAxis>(Axis);
                const float Delta = A->Ideology.GetAxis(AxisEnum) - B->Ideology.GetAxis(AxisEnum);
                IdeologyDist += Delta * Delta;
            }
            IdeologyDist = FMath::Sqrt(IdeologyDist);

            const float TradeAB = TradeVolume[i * MaxFactions + j];
            const float TradeBA = TradeVolume[j * MaxFactions + i];
            const float EconDep = (TradeAB + TradeBA) * 0.5f;

            float EventScore = 0.0f;
            for (const FMythicWorldEvent &EventRef : RecentEvents) {
                const FMythicWorldEvent *Event = &EventRef;
                const bool bInvolvesBoth =
                    (Event->PrimaryFaction == IdA && Event->SecondaryFaction == IdB) ||
                    (Event->PrimaryFaction == IdB && Event->SecondaryFaction == IdA);
                if (!bInvolvesBoth) {
                    continue;
                }

                if (Event->CategoryFlags & EMythicEventCategory::Combat) {
                    EventScore -= Event->Significance;
                }
                else {
                    EventScore += Event->Significance * 0.5f;
                }
            }

            const float Score = -(IdeologyDist * Settings->IdeologyWeight)
                + (EconDep * Settings->DependencyWeight)
                + (EventScore * Settings->EventWeight);

            const EMythicFactionRelation CurrentRel = FactionDB->GetWriteRelationship(IdA, IdB);
            const EMythicFactionRelation NewRel = MapDiplomacyScoreToRelation(
                Score, CurrentRel,
                Settings->AllyThreshold, Settings->FriendlyThreshold,
                Settings->UnfriendlyThreshold, Settings->HostileThreshold,
                Settings->HysteresisMargin);

            if (NewRel != CurrentRel) {
                FactionDB->SetRelationship(IdA, IdB, NewRel);

                if (Fabric) {
                    FMythicWorldEvent DiploEvent;
                    DiploEvent.EventTag = TAG_LIVINGWORLD_EVENT_DIPLOMACY_SHIFT;
                    DiploEvent.PrimaryFaction = IdA;
                    DiploEvent.SecondaryFaction = IdB;
                    DiploEvent.Significance = DiplomacyShiftSignificance(NewRel);
                    DiploEvent.CategoryFlags = EMythicEventCategory::Diplomacy;
                    Fabric->AppendEvent(DiploEvent);
                }
            }
        }
    }
}

void FMythicWorldSimThread::TickTerritoryPropagation() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicWorldSim_Territory);

    if (TerritoryGrid) {
        TerritoryGrid->PropagateInfluence();

        if (FactionDB) {
            TArray<int32> CellCounts;
            TerritoryGrid->GetWriteCellCounts(CellCounts);
            const int32 FactionCount = FactionDB->GetRegisteredCount();
            for (int32 i = 0; i < FactionCount && i < CellCounts.Num(); ++i) {
                if (FMythicFactionData *F = FactionDB->GetFactionMutableByIndex(i)) {
                    F->ControlledCellCount = CellCounts[i];
                }
            }
        }
    }
}


void FMythicWorldSimThread::TickIdeologyMetabolism() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicWorldSim_Ideology);

    if (!Fabric) {
        return;
    }

    const int32 FactionCount = FactionDB->GetRegisteredCount();
    const double CurrentTime = FPlatformTime::Seconds();

    TArray<FMythicWorldEvent> RecentEvents;

    for (int32 i = 0; i < FactionCount; ++i) {
        FMythicFactionData *F = FactionDB->GetFactionMutableByIndex(i);
        if (!F || !F->bAlive || !F->bCanNegotiate) {
            continue;
        }

        FMythicFactionId FId;
        FId.Index = static_cast<uint8>(i);

        Fabric->QueryEventsByCategory(
            0xFFFF,
            CurrentTime - 30.0, CurrentTime,
            Settings->IdeologyEventScanCap,
            RecentEvents
            );

        float AccumulatedVector[MoralAxisCount] = {};
        int32 RelevantEventCount = 0;

        for (const FMythicWorldEvent &EventRef : RecentEvents) {
            const FMythicWorldEvent *Event = &EventRef;
            if (Event->Significance < Settings->DriftMinSignificance) {
                continue;
            }
            if (Event->PrimaryFaction != FId && Event->SecondaryFaction != FId) {
                continue;
            }

            for (int32 Axis = 0; Axis < MoralAxisCount; ++Axis) {
                AccumulatedVector[Axis] += Event->MoralVector.AxisValues[Axis] * Event->Significance;
            }
            ++RelevantEventCount;
        }

        if (RelevantEventCount > 0) {
            const float InvCount = 1.0f / static_cast<float>(RelevantEventCount);
            for (int32 Axis = 0; Axis < MoralAxisCount; ++Axis) {
                const EMythicMoralAxis AxisEnum = static_cast<EMythicMoralAxis>(Axis);
                const float EventMean = AccumulatedVector[Axis] * InvCount;
                const float CurrentValue = F->Ideology.GetAxis(AxisEnum);
                F->Ideology.GetAxisMutable(AxisEnum) = DriftTowardClamped(CurrentValue, EventMean, Settings->IdeologyDriftRate);
            }

            F->bIdeologyDirty = true;
        }

        if (!F->bControlsTerritory && F->ControlledCellCount > 0) {
            for (int32 j = 0; j < FactionCount; ++j) {
                if (j == i) {
                    continue;
                }
                FMythicFactionData *Other = FactionDB->GetFactionMutableByIndex(j);
                if (!Other || !Other->bAlive || !Other->bControlsTerritory) {
                    continue;
                }

                const float TradeBetween = TradeVolume[i * MaxFactions + j] + TradeVolume[j * MaxFactions + i];
                if (TradeBetween <= 0.0f) {
                    continue;
                }

                for (int32 Axis = 0; Axis < MoralAxisCount; ++Axis) {
                    const EMythicMoralAxis AxisEnum = static_cast<EMythicMoralAxis>(Axis);
                    const float SourceValue = F->Ideology.GetAxis(AxisEnum);
                    const float TargetValue = Other->Ideology.GetAxis(AxisEnum);
                    Other->Ideology.GetAxisMutable(AxisEnum) = DriftTowardClamped(TargetValue, SourceValue, Settings->IdeologyBleedRate);
                }
            }
        }
    }
}


void FMythicWorldSimThread::TickFactionEvolution() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicWorldSim_FactionEvolution);

    const int32 FactionCount = FactionDB->GetRegisteredCount();

    TArray<int32> AnnihilatedIndices;

    TArray<FMythicWorldEvent> FactionEvents;

    for (int32 i = 0; i < FactionCount; ++i) {
        FMythicFactionData *F = FactionDB->GetFactionMutableByIndex(i);
        if (!F || !F->bAlive) {
            if (F && !F->bAlive && F->LastAlivePopulation > 0) {
                AnnihilatedIndices.Add(i);
            }
            continue;
        }

        FMythicFactionId FId;
        FId.Index = static_cast<uint8>(i);

        if (!F->bControlsTerritory &&
            F->ControlledCellCount >= Settings->TerritorialCellThreshold &&
            F->Population >= Settings->MinTerritorialPopulation &&
            F->bCanNegotiate) {
            F->bControlsTerritory = true;
            F->bHasCivilianPopulation = true;
            F->bParticipatesInTrade = true;

            if (Fabric) {
                FMythicWorldEvent EvolutionEvent;
                EvolutionEvent.EventTag = TAG_LIVINGWORLD_EVENT_FACTION_EVOLUTION;
                EvolutionEvent.PrimaryFaction = FId;
                EvolutionEvent.Significance = 0.8f;
                EvolutionEvent.CategoryFlags = EMythicEventCategory::Diplomacy;
                Fabric->AppendEvent(EvolutionEvent);
            }

            UE_LOG(LogMythWorldSim, Log, TEXT("Faction '%s' evolved: gained territorial control (cells=%d, pop=%d)"),
                   *F->DisplayName.ToString(), F->ControlledCellCount, F->Population);
        }

        if (F->bControlsTerritory &&
            F->ControlledCellCount <= Settings->DevolveCellThreshold &&
            F->MilitaryStrength < 0.1f) {
            F->bControlsTerritory = false;

            if (Fabric) {
                FMythicWorldEvent DevolutionEvent;
                DevolutionEvent.EventTag = TAG_LIVINGWORLD_EVENT_FACTION_DEVOLUTION;
                DevolutionEvent.PrimaryFaction = FId;
                DevolutionEvent.Significance = 0.8f;
                DevolutionEvent.CategoryFlags = EMythicEventCategory::Diplomacy;
                Fabric->AppendEvent(DevolutionEvent);
            }

            UE_LOG(LogMythWorldSim, Log, TEXT("Faction '%s' devolved: lost territorial control (cells=%d, military=%.2f)"),
                   *F->DisplayName.ToString(), F->ControlledCellCount, F->MilitaryStrength);
        }


        int32 SecondLargestCluster = 0;
        if (TerritoryGrid) {
            TArray<FMythicCellCoord> FactionCells;
            TerritoryGrid->GetFactionCells(FId, 10000, FactionCells);

            if (FactionCells.Num() > 0) {
                int32 MaxCluster = 0;
                TSet<int32> VisitedIdx;
                for (const FMythicCellCoord &Cell : FactionCells) {
                    int32 CIdx = Cell.Y * TerritoryGrid->GetWidth() + Cell.X;
                    if (VisitedIdx.Contains(CIdx)) {
                        continue;
                    }

                    int32 ClusterSize = 0;
                    TArray<FMythicCellCoord> Queue;
                    Queue.Add(Cell);
                    VisitedIdx.Add(CIdx);

                    while (Queue.Num() > 0) {
                        FMythicCellCoord Curr = Queue.Pop(EAllowShrinking::No);
                        ClusterSize++;

                        FMythicCellCoord Neighbors[4] = {
                            FMythicCellCoord(Curr.X + 1, Curr.Y), FMythicCellCoord(Curr.X - 1, Curr.Y),
                            FMythicCellCoord(Curr.X, Curr.Y + 1), FMythicCellCoord(Curr.X, Curr.Y - 1)
                        };
                        for (const FMythicCellCoord &N : Neighbors) {
                            if (TerritoryGrid->IsValidCoord(N) && TerritoryGrid->GetDominantFaction(N) == FId) {
                                int32 NIdx = N.Y * TerritoryGrid->GetWidth() + N.X;
                                if (!VisitedIdx.Contains(NIdx)) {
                                    VisitedIdx.Add(NIdx);
                                    Queue.Add(N);
                                }
                            }
                        }
                    }

                    if (ClusterSize > MaxCluster) {
                        SecondLargestCluster = MaxCluster;
                        MaxCluster = ClusterSize;
                    }
                    else if (ClusterSize > SecondLargestCluster) {
                        SecondLargestCluster = ClusterSize;
                    }
                }
            }
        }

        const bool bGeographicSchism = (SecondLargestCluster >= Settings->MinSchismSize / 2);


        float InternalDivergence = 0.0f;
        if (Fabric) {
            const double CurrentTime = FPlatformTime::Seconds();
            Fabric->QueryEventsByCategory(
                0xFFFF, CurrentTime - 60.0, CurrentTime,
                Settings->IdeologyEventScanCap,
                FactionEvents
                );

            float MeanVector[MoralAxisCount] = {};
            int32 EventCount = 0;
            for (const FMythicWorldEvent &EventRef : FactionEvents) {
                const FMythicWorldEvent *Event = &EventRef;
                if (Event->PrimaryFaction != FId) {
                    continue;
                }
                for (int32 Axis = 0; Axis < MoralAxisCount; ++Axis) {
                    MeanVector[Axis] += Event->MoralVector.AxisValues[Axis];
                }
                ++EventCount;
            }

            if (EventCount > 1) {
                const float InvCount = 1.0f / static_cast<float>(EventCount);
                for (int32 Axis = 0; Axis < MoralAxisCount; ++Axis) {
                    MeanVector[Axis] *= InvCount;
                }

                for (int32 Axis = 0; Axis < MoralAxisCount; ++Axis) {
                    const EMythicMoralAxis AxisEnum = static_cast<EMythicMoralAxis>(Axis);
                    const float Delta = MeanVector[Axis] - F->Ideology.GetAxis(AxisEnum);
                    InternalDivergence += Delta * Delta;
                }
                InternalDivergence = FMath::Sqrt(InternalDivergence);
            }
        }

        if (ShouldFactionSchism(InternalDivergence, Settings->SchismIdeologyThreshold, bGeographicSchism,
                                F->Population, Settings->MinSchismPopulation)) {
            FMythicFactionData NewFaction;

            const int32 NewIndex = FactionDB->GetRegisteredCount();
            NewFaction.DisplayName = FText::FromString(
                FString::Printf(TEXT("%s Separatists"), *F->DisplayName.ToString())
                );

            NewFaction.FactionTag = FGameplayTag();

            NewFaction.Ideology = F->Ideology;
            for (int32 Axis = 0; Axis < MoralAxisCount; ++Axis) {
                const EMythicMoralAxis AxisEnum = static_cast<EMythicMoralAxis>(Axis);
                const float Mutation = FMath::FRandRange(
                    -Settings->SchismIdeologyMutation,
                    Settings->SchismIdeologyMutation
                    );
                NewFaction.Ideology.GetAxisMutable(AxisEnum) = FMath::Clamp(
                    F->Ideology.GetAxis(AxisEnum) + Mutation, -1.0f, 1.0f
                    );
            }

            const int32 SplitPop = F->Population / 2;
            NewFaction.Population = SplitPop;
            F->Population -= SplitPop;

            const int32 SplitCells = F->ControlledCellCount / 2;
            NewFaction.ControlledCellCount = SplitCells;
            F->ControlledCellCount -= SplitCells;

            NewFaction.bControlsTerritory = F->bControlsTerritory;
            NewFaction.bHasEconomy = F->bHasEconomy;
            NewFaction.bHasCivilianPopulation = F->bHasCivilianPopulation;
            NewFaction.bParticipatesInTrade = F->bParticipatesInTrade;
            NewFaction.bCanNegotiate = F->bCanNegotiate;

            const float TerritoryRatio = (F->ControlledCellCount + SplitCells > 0)
                ? static_cast<float>(SplitCells) / static_cast<float>(F->ControlledCellCount + SplitCells)
                : 0.5f;
            NewFaction.BaseProduction = F->BaseProduction;
            NewFaction.BaseProduction *= TerritoryRatio;

            const float PopRatio = static_cast<float>(SplitPop) / static_cast<float>(FMath::Max(F->Population + SplitPop, 1));
            NewFaction.Reserves = F->Reserves;
            NewFaction.Reserves *= PopRatio;
            F->Reserves *= (1.0f - PopRatio);

            NewFaction.DisapproveThreshold = F->DisapproveThreshold;
            NewFaction.CondemnThreshold = F->CondemnThreshold;
            NewFaction.HostileThreshold = F->HostileThreshold;

            FMythicFactionId NewId = FactionDB->RegisterFaction(NewFaction);

            if (NewId.IsValid()) {
                FactionDB->SetRelationship(FId, NewId, EMythicFactionRelation::Hostile);

                for (int32 k = 0; k < FactionCount; ++k) {
                    if (k == i) {
                        continue;
                    }
                    FMythicFactionId OtherId;
                    OtherId.Index = static_cast<uint8>(k);
                    const EMythicFactionRelation ParentRel = FactionDB->GetWriteRelationship(FId, OtherId);

                    EMythicFactionRelation InheritedRel = EMythicFactionRelation::Neutral;
                    switch (ParentRel) {
                    case EMythicFactionRelation::Allied:
                        InheritedRel = EMythicFactionRelation::Friendly;
                        break;
                    case EMythicFactionRelation::Friendly:
                        InheritedRel = EMythicFactionRelation::Neutral;
                        break;
                    case EMythicFactionRelation::Neutral:
                        InheritedRel = EMythicFactionRelation::Neutral;
                        break;
                    case EMythicFactionRelation::Unfriendly:
                        InheritedRel = EMythicFactionRelation::Unfriendly;
                        break;
                    case EMythicFactionRelation::Hostile:
                        InheritedRel = EMythicFactionRelation::Hostile;
                        break;
                    }
                    FactionDB->SetRelationship(NewId, OtherId, InheritedRel);
                }

                if (Fabric) {
                    FMythicWorldEvent SchismEvent;
                    SchismEvent.EventTag = TAG_LIVINGWORLD_EVENT_FACTION_SCHISM;
                    SchismEvent.PrimaryFaction = FId;
                    SchismEvent.SecondaryFaction = NewId;
                    SchismEvent.Significance = 1.0f;
                    SchismEvent.CategoryFlags = EMythicEventCategory::Diplomacy;
                    Fabric->AppendEvent(SchismEvent);
                }

                UE_LOG(LogMythWorldSim, Log, TEXT("Faction '%s' schism → new faction '%s' (pop=%d, cells=%d)"),
                       *F->DisplayName.ToString(), *NewFaction.DisplayName.ToString(), SplitPop, SplitCells);
            }
        }
    }

    for (int32 DeadIdx : AnnihilatedIndices) {
        FMythicFactionData *Dead = FactionDB->GetFactionMutableByIndex(DeadIdx);
        if (!Dead || Dead->LastAlivePopulation <= 0) {
            continue;
        }

        FMythicFactionId DeadId;
        DeadId.Index = static_cast<uint8>(DeadIdx);

        int32 BestAbsorber = -1;
        int32 BestTier = 0;
        int32 BestPop = -1;

        for (int32 j = 0; j < FactionCount; ++j) {
            FMythicFactionData *Candidate = FactionDB->GetFactionMutableByIndex(j);
            if (!Candidate || !Candidate->bAlive || !Candidate->bHasCivilianPopulation) {
                continue;
            }

            FMythicFactionId CandId;
            CandId.Index = static_cast<uint8>(j);
            const EMythicFactionRelation Rel = FactionDB->GetWriteRelationship(DeadId, CandId);
            const int32 Tier = (Rel == EMythicFactionRelation::Allied)
                ? 2
                : (Rel == EMythicFactionRelation::Friendly)
                ? 1
                : 0;
            if (Tier == 0) {
                continue;
            }

            if (Tier > BestTier || (Tier == BestTier && Candidate->Population > BestPop)) {
                BestAbsorber = j;
                BestTier = Tier;
                BestPop = Candidate->Population;
            }
        }

        if (BestAbsorber >= 0) {
            FMythicFactionData *Absorber = FactionDB->GetFactionMutableByIndex(BestAbsorber);
            const int32 Refugees = static_cast<int32>(
                static_cast<float>(Dead->LastAlivePopulation) * Settings->AbsorptionFraction
            );
            Absorber->Population += Refugees;

            if (Fabric) {
                FMythicFactionId AbsorberId;
                AbsorberId.Index = static_cast<uint8>(BestAbsorber);
                FMythicWorldEvent AbsorbEvent;
                AbsorbEvent.EventTag = TAG_LIVINGWORLD_EVENT_FACTION_ABSORPTION;
                AbsorbEvent.PrimaryFaction = AbsorberId;
                AbsorbEvent.SecondaryFaction = DeadId;
                AbsorbEvent.Significance = 0.6f;
                AbsorbEvent.CategoryFlags = EMythicEventCategory::Diplomacy;
                Fabric->AppendEvent(AbsorbEvent);
            }

            UE_LOG(LogMythWorldSim, Log, TEXT("Faction '%s' absorbed %d refugees from '%s'"),
                   *Absorber->DisplayName.ToString(), Refugees, *Dead->DisplayName.ToString());
        }

        Dead->LastAlivePopulation = 0;
    }
}

void FMythicWorldSimThread::TickSchemeEngine() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicWorldSim_Schemes);

    if (SchemeEngine) {
        SchemeEngine->TickSchemes(1.0f, static_cast<uint32>(TickCount));
    }
}

void FMythicWorldSimThread::TickCrystallization() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicWorldSim_Crystallization);


    if (!FactionDB || !Settings || !Fabric) {
        return;
    }

    const int32 FactionCount = FactionDB->GetRegisteredCount();

    TArray<FMythicWorldEvent> RecentEvents;

    for (int32 i = 0; i < FactionCount; ++i) {
        FMythicFactionData *F = FactionDB->GetFactionMutableByIndex(i);
        if (!F || !F->bAlive) {
            continue;
        }

        FMythicFactionId FId;
        FId.Index = static_cast<uint8>(i);


        Fabric->QueryEventsByFaction(FId, RecentEvents, 64);

        if (RecentEvents.Num() >= 8) {
            int32 CombatCount = 0;
            int32 EconomyCount = 0;
            int32 DiplomacyCount = 0;

            for (const FMythicWorldEvent &Event : RecentEvents) {
                if ((Event.CategoryFlags & EMythicEventCategory::Combat) != 0) {
                    ++CombatCount;
                }
                if ((Event.CategoryFlags & EMythicEventCategory::Trade) != 0) {
                    ++EconomyCount;
                }
                if ((Event.CategoryFlags & EMythicEventCategory::Diplomacy) != 0) {
                    ++DiplomacyCount;
                }
            }

            const float TotalEvents = static_cast<float>(RecentEvents.Num());
            constexpr float CulturalThreshold = 0.5f;
            constexpr float CulturalDriftRate = 0.02f;

            if (static_cast<float>(CombatCount) / TotalEvents > CulturalThreshold) {
                const float CurrentViolence = F->Ideology.GetAxis(EMythicMoralAxis::Violence);
                F->Ideology.GetAxisMutable(EMythicMoralAxis::Violence) =
                    FMath::Clamp(CurrentViolence + CulturalDriftRate, -1.0f, 1.0f);
                F->bIdeologyDirty = true;

                UE_LOG(LogMythWorldSim, Verbose, TEXT("Crystallization: Faction '%s' warrior culture drift (combat=%d/%d)"),
                       *F->DisplayName.ToString(), CombatCount, RecentEvents.Num());
            }

            if (static_cast<float>(EconomyCount) / TotalEvents > CulturalThreshold) {
                const float CurrentTheft = F->Ideology.GetAxis(EMythicMoralAxis::Theft);
                F->Ideology.GetAxisMutable(EMythicMoralAxis::Theft) =
                    FMath::Clamp(CurrentTheft - CulturalDriftRate, -1.0f, 1.0f);
                F->bIdeologyDirty = true;

                UE_LOG(LogMythWorldSim, Verbose, TEXT("Crystallization: Faction '%s' merchant culture drift (economy=%d/%d)"),
                       *F->DisplayName.ToString(), EconomyCount, RecentEvents.Num());
            }

            if (static_cast<float>(DiplomacyCount) / TotalEvents > CulturalThreshold) {
                const float CurrentAuthority = F->Ideology.GetAxis(EMythicMoralAxis::Authority);
                F->Ideology.GetAxisMutable(EMythicMoralAxis::Authority) =
                    FMath::Clamp(CurrentAuthority + CulturalDriftRate, -1.0f, 1.0f);
                F->bIdeologyDirty = true;
            }
        }


        if (F->LeaderEntityId != 0 && F->LeaderSignificanceScore <= 0.0f) {
            UE_LOG(LogMythWorldSim, Log, TEXT("Crystallization: Faction '%s' leader vacancy (prev leader %d deceased/irrelevant)"),
                   *F->DisplayName.ToString(), F->LeaderEntityId);
            F->LeaderEntityId = 0;
            F->LeaderSignificanceScore = 0.0f;
        }
    }
}

void FMythicWorldSimThread::TickHistoryAppend() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicWorldSim_HistoryAppend);


    if (!Fabric || !FactionDB || !Settings) {
        return;
    }

    const int32 FactionCount = FactionDB->GetRegisteredCount();

    for (int32 i = 0; i < FactionCount; ++i) {
        FMythicFactionData *F = FactionDB->GetFactionMutableByIndex(i);
        if (!F || !F->bAlive) {
            continue;
        }

        FMythicFactionId FId;
        FId.Index = static_cast<uint8>(i);

        const bool bFamineNow = F->Reserves.GetResource(EMythicResourceType::Food) < -50.0f;
        if (bFamineNow && !F->bFamineActive) {
            F->bFamineActive = true;
            FMythicWorldEvent FamineEvent;
            FamineEvent.EventTag = TAG_LIVINGWORLD_EVENT_FACTION_FAMINE;
            FamineEvent.PrimaryFaction = FId;
            FamineEvent.Significance = 0.4f;
            FamineEvent.CategoryFlags = EMythicEventCategory::Diplomacy;
            Fabric->AppendEvent(FamineEvent);
        }
        else if (!bFamineNow) {
            F->bFamineActive = false;
        }

        const bool bWeakNow = (F->MilitaryStrength < 0.1f && F->Population > 10);
        if (bWeakNow && !F->bWeaknessActive) {
            F->bWeaknessActive = true;
            FMythicWorldEvent WeaknessEvent;
            WeaknessEvent.EventTag = TAG_LIVINGWORLD_EVENT_FACTION_WEAKNESS;
            WeaknessEvent.PrimaryFaction = FId;
            WeaknessEvent.Significance = 0.3f;
            WeaknessEvent.CategoryFlags = EMythicEventCategory::Diplomacy | EMythicEventCategory::Combat;
            Fabric->AppendEvent(WeaknessEvent);
        }
        else if (!bWeakNow) {
            F->bWeaknessActive = false;
        }
    }
}
