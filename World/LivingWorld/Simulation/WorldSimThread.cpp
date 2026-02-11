// Mythic Living World System — Background Simulation Thread Implementation

#include "World/LivingWorld/Simulation/WorldSimThread.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Morality/MoralSignature.h"
#include "HAL/PlatformProcess.h"

FMythicWorldSimThread::FMythicWorldSimThread() {}

FMythicWorldSimThread::~FMythicWorldSimThread() {
    StopThread();
}

void FMythicWorldSimThread::Setup(
    UMythicCausalFabric *InFabric,
    UMythicFactionDatabase *InFactionDB,
    UMythicTerritoryGrid *InTerritoryGrid,
    const UMythicLivingWorldSettings *InSettings,
    float InTickIntervalSeconds) {
    Fabric = InFabric;
    FactionDB = InFactionDB;
    TerritoryGrid = InTerritoryGrid;
    Settings = InSettings;
    TickIntervalSeconds = FMath::Max(InTickIntervalSeconds, 0.1f);

    // Init trade volume matrix
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

        // Sleep for the remainder of the tick interval
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

    // Commit all write buffers to game-thread-readable snapshots
    CommitAllSnapshots();
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

// ─────────────────────────────────────────────────────────────
// TickEconomy — Resource production, consumption, trade, military derivation
// ─────────────────────────────────────────────────────────────

void FMythicWorldSimThread::TickEconomy() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicWorldSim_Economy);

    const int32 FactionCount = FactionDB->GetRegisteredCount();
    const float RefCells = static_cast<float>(Settings->ReferenceCellCount);
    constexpr float Epsilon = 0.001f;

    // ── Per-faction: compute supply, demand, update reserves ──
    for (int32 i = 0; i < FactionCount; ++i) {
        FMythicFactionData *F = FactionDB->GetFactionMutableByIndex(i);
        if (!F || !F->bAlive || !F->bHasEconomy) {
            continue;
        }

        // ── Supply from territory production ──
        FMythicResourceStock NewSupply;
        if (F->bControlsTerritory && F->ControlledCellCount > 0) {
            const float CellRatio = static_cast<float>(F->ControlledCellCount) / FMath::Max(RefCells, 1.0f);
            NewSupply.Food = F->BaseProduction.Food * CellRatio;
            NewSupply.Materials = F->BaseProduction.Materials * CellRatio;
            NewSupply.Arms = F->BaseProduction.Arms * CellRatio;
            NewSupply.Wealth = F->BaseProduction.Wealth * CellRatio;
        }

        // ── Ideology-driven income sources ──

        // Taxation: factions with Authority ideology and civilian population
        if (F->bHasCivilianPopulation && F->Ideology.Authority > Settings->TaxAuthorityThreshold) {
            NewSupply.Wealth += static_cast<float>(F->Population) * Settings->TaxRate;
        }

        // Contract income: strong military, low population — selling martial services
        if (F->MilitaryStrength > Settings->ContractMilitaryThreshold &&
            F->Population < Settings->ContractPopulationCeiling) {
            NewSupply.Wealth += F->MilitaryStrength * Settings->TaxRate * 10.0f;
        }

        // Raid income: high Theft ideology or non-negotiating with economy
        const bool bIsRaider = F->Ideology.Theft > Settings->RaidIdeologyThreshold ||
            (!F->bCanNegotiate && F->bHasEconomy);
        if (bIsRaider) {
            // Steal a fraction of total trade volume happening around this faction
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

        // Arms production from materials (consume materials to produce arms)
        const float ArmsProducible = FMath::Min(
            Settings->ArmsProductionRate,
            F->Reserves.Materials / FMath::Max(Settings->MaterialsPerArm, Epsilon)
            );
        if (ArmsProducible > 0.0f) {
            NewSupply.Arms += ArmsProducible;
            F->Reserves.Materials -= ArmsProducible * Settings->MaterialsPerArm;
        }

        F->Supply = NewSupply;

        // ── Demand ──
        FMythicResourceStock NewDemand;
        NewDemand.Food = static_cast<float>(F->Population) * Settings->FoodPerCapita;
        NewDemand.Arms = F->MilitaryStrength * Settings->ArmsUpkeepRate;
        NewDemand.Wealth = F->MilitaryStrength * Settings->WealthUpkeepRate;
        F->Demand = NewDemand;

        // ── Update reserves (supply - demand) ──
        F->Reserves += F->Supply;
        F->Reserves -= F->Demand;
        F->Reserves.ClampAll(-Settings->MaxReserve, Settings->MaxReserve);

        // ── Prices: demand vs supply ratio ──
        for (int32 r = 0; r < ResourceTypeCount; ++r) {
            const EMythicResourceType Type = static_cast<EMythicResourceType>(r);
            const float S = FMath::Max(F->Supply.GetResource(Type), Epsilon);
            const float D = F->Demand.GetResource(Type);
            F->Prices.GetResourceMutable(Type) = D / S;
        }

        // ── Military strength derived from Arms + Wealth reserves ──
        const float ArmsReserve = FMath::Max(F->Reserves.Arms, 0.0f);
        const float WealthForMil = FMath::Max(F->Reserves.Wealth, 0.0f);
        F->MilitaryStrength = FMath::Clamp(
            (ArmsReserve * 0.6f + WealthForMil * 0.4f) / FMath::Max(Settings->MaxReserve, 1.0f),
            0.0f, 1.0f
            );
    }

    // ── Natural Trade between faction pairs ──
    // Decay trade volume each tick, then accumulate new trades
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

            // Only trade if relationship is Neutral or better
            const EMythicFactionRelation Rel = FactionDB->GetWriteRelationship(IdA, IdB);
            if (Rel == EMythicFactionRelation::Hostile || Rel == EMythicFactionRelation::Unfriendly) {
                continue;
            }

            // Relationship-based price modifier (allies get better deals)
            float RelMod = 1.0f;
            if (Rel == EMythicFactionRelation::Allied) { RelMod = 0.7f; }
            else
                if (Rel == EMythicFactionRelation::Friendly) { RelMod = 0.85f; }

            // Trade each resource: surplus flows to deficit
            for (int32 r = 0; r < ResourceTypeCount; ++r) {
                const EMythicResourceType Type = static_cast<EMythicResourceType>(r);
                const float SurplusA = A->Reserves.GetResource(Type) - Settings->TradeSurplusThreshold;
                const float DeficitB = Settings->TradeDeficitThreshold - B->Reserves.GetResource(Type);
                const float SurplusB = B->Reserves.GetResource(Type) - Settings->TradeSurplusThreshold;
                const float DeficitA = Settings->TradeDeficitThreshold - A->Reserves.GetResource(Type);

                // A has surplus, B has deficit
                if (SurplusA > 0.0f && DeficitB > 0.0f) {
                    const float TradeAmount = FMath::Min(SurplusA, DeficitB) * 0.5f;
                    A->Reserves.GetResourceMutable(Type) -= TradeAmount;
                    B->Reserves.GetResourceMutable(Type) += TradeAmount;
                    // Buyer pays wealth based on seller's price
                    const float Cost = TradeAmount * A->Prices.GetResource(Type) * RelMod;
                    A->Reserves.Wealth += Cost;
                    B->Reserves.Wealth -= Cost;
                    TradeVolume[i * MaxFactions + j] += TradeAmount;
                    TradeVolume[j * MaxFactions + i] += TradeAmount;
                }

                // B has surplus, A has deficit
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

// ─────────────────────────────────────────────────────────────
// TickPopulation — Birth, death, migration, spawning, annihilation
// ─────────────────────────────────────────────────────────────

void FMythicWorldSimThread::TickPopulation() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicWorldSim_Population);

    const int32 FactionCount = FactionDB->GetRegisteredCount();

    for (int32 i = 0; i < FactionCount; ++i) {
        FMythicFactionData *F = FactionDB->GetFactionMutableByIndex(i);
        if (!F || !F->bAlive) {
            continue;
        }

        if (F->bHasCivilianPopulation) {
            // ── Civilian growth: births - deaths - migration ──
            const int32 Capacity = F->ControlledCellCount * Settings->PopulationPerCell;
            const float FoodSufficiency = (F->Demand.Food > 0.001f)
                ? FMath::Clamp(F->Reserves.Food / (F->Demand.Food * 10.0f), 0.0f, 2.0f)
                : 1.0f;

            // Births: scale with food sufficiency and room to grow
            const int32 CapacityGap = FMath::Max(0, Capacity - F->Population);
            const int32 Births = static_cast<int32>(
                static_cast<float>(CapacityGap) * Settings->BaseBirthRate * FoodSufficiency
            );

            // Deaths: higher with starvation
            const float DeathModifier = FoodSufficiency < 0.3f ? 3.0f : 1.0f;
            const int32 Deaths = static_cast<int32>(
                static_cast<float>(F->Population) * Settings->BaseDeathRate * DeathModifier
            );

            // Migration: people leave when starving
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
            // ── Spawning (creatures, undead): population from territory ──
            F->Population += F->ControlledCellCount * Settings->SpawnRatePerCell;
        }
        else {
            // ── Recruitment-based (mercenary-like): population from wealth ──
            const float WealthAvail = FMath::Max(F->Reserves.Wealth, 0.0f);
            const int32 Recruits = static_cast<int32>(WealthAvail * Settings->RecruitmentPerWealth);
            F->Population += Recruits;

            // Natural attrition without civilian growth
            const int32 Attrition = static_cast<int32>(
                static_cast<float>(F->Population) * Settings->BaseDeathRate
            );
            F->Population = FMath::Max(0, F->Population - Attrition);
        }

        // Track if this faction has ever had population (gates annihilation)
        if (F->Population > 0) {
            F->bHasBeenPopulated = true;
        }

        // ── Annihilation check ──
        // Only annihilate factions that previously had population —
        // newly registered factions waiting for territory/population seeding are spared.
        if (F->bHasBeenPopulated && F->Population <= 0 && F->ControlledCellCount <= 0) {
            FMythicFactionId DeadId;
            DeadId.Index = static_cast<uint8>(i);
            FactionDB->AnnihilateFaction(DeadId);

            // Emit annihilation event
            if (Fabric) {
                FMythicWorldEvent AnnEvent;
                AnnEvent.PrimaryFaction = DeadId;
                AnnEvent.Significance = 1.0f;
                AnnEvent.CategoryFlags = EMythicEventCategory::Diplomacy | EMythicEventCategory::Death;
                Fabric->AppendEvent(AnnEvent);
            }

            UE_LOG(LogMythWorldSim, Log, TEXT("Faction '%s' annihilated (pop=0, cells=0)"), *F->DisplayName.ToString());
        }
    }
}

// ─────────────────────────────────────────────────────────────
// TickDiplomacy — Relationship evaluation from ideology + trade + events
// ─────────────────────────────────────────────────────────────

void FMythicWorldSimThread::TickDiplomacy() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicWorldSim_Diplomacy);

    const int32 FactionCount = FactionDB->GetRegisteredCount();

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

            // ── Ideology distance (negative = opposed) ──
            float IdeologyDist = 0.0f;
            for (int32 Axis = 0; Axis < MoralAxisCount; ++Axis) {
                const EMythicMoralAxis AxisEnum = static_cast<EMythicMoralAxis>(Axis);
                const float Delta = A->Ideology.GetAxis(AxisEnum) - B->Ideology.GetAxis(AxisEnum);
                IdeologyDist += Delta * Delta;
            }
            IdeologyDist = FMath::Sqrt(IdeologyDist);

            // ── Economic dependency (symmetric) ──
            const float TradeAB = TradeVolume[i * MaxFactions + j];
            const float TradeBA = TradeVolume[j * MaxFactions + i];
            const float EconDep = (TradeAB + TradeBA) * 0.5f;

            // ── Recent event score ──
            float EventScore = 0.0f;
            if (Fabric) {
                TArray<const FMythicWorldEvent *> RecentEvents;
                const double CurrentTime = FPlatformTime::Seconds();
                Fabric->QueryEventsByCategory(
                    EMythicEventCategory::Combat | EMythicEventCategory::Trade | EMythicEventCategory::Diplomacy,
                    CurrentTime - 60.0, CurrentTime,
                    Settings->DiplomacyEventScanCap,
                    RecentEvents
                    );

                for (const FMythicWorldEvent *Event : RecentEvents) {
                    if (!Event) {
                        continue;
                    }
                    const bool bInvolvesBoth =
                        (Event->PrimaryFaction == IdA && Event->SecondaryFaction == IdB) ||
                        (Event->PrimaryFaction == IdB && Event->SecondaryFaction == IdA);
                    if (!bInvolvesBoth) {
                        continue;
                    }

                    // Combat events hurt relationship, trade/diplomacy help
                    if (Event->CategoryFlags & EMythicEventCategory::Combat) {
                        EventScore -= Event->Significance;
                    }
                    else {
                        EventScore += Event->Significance * 0.5f;
                    }
                }
            }

            // ── Composite score ──
            const float Score = -(IdeologyDist * Settings->IdeologyWeight)
                + (EconDep * Settings->DependencyWeight)
                + (EventScore * Settings->EventWeight);

            // ── Map to relationship with hysteresis ──
            const EMythicFactionRelation CurrentRel = FactionDB->GetWriteRelationship(IdA, IdB);
            EMythicFactionRelation NewRel = EMythicFactionRelation::Neutral;

            const float Hyst = Settings->HysteresisMargin;
            if (Score > Settings->AllyThreshold + (CurrentRel == EMythicFactionRelation::Allied ? 0.0f : Hyst)) {
                NewRel = EMythicFactionRelation::Allied;
            }
            else if (Score > Settings->FriendlyThreshold + (CurrentRel == EMythicFactionRelation::Friendly ? 0.0f : Hyst)) {
                NewRel = EMythicFactionRelation::Friendly;
            }
            else if (Score < Settings->HostileThreshold - (CurrentRel == EMythicFactionRelation::Hostile ? 0.0f : Hyst)) {
                NewRel = EMythicFactionRelation::Hostile;
            }
            else if (Score < Settings->UnfriendlyThreshold - (CurrentRel == EMythicFactionRelation::Unfriendly ? 0.0f : Hyst)) {
                NewRel = EMythicFactionRelation::Unfriendly;
            }

            if (NewRel != CurrentRel) {
                FactionDB->SetRelationship(IdA, IdB, NewRel);

                // Emit diplomacy event for significant shifts
                if (Fabric) {
                    FMythicWorldEvent DiploEvent;
                    DiploEvent.PrimaryFaction = IdA;
                    DiploEvent.SecondaryFaction = IdB;
                    DiploEvent.Significance = 0.5f;
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
    }
}

// ─────────────────────────────────────────────────────────────
// TickIdeologyMetabolism — Faction ideology drifts toward lived experience
// ─────────────────────────────────────────────────────────────

void FMythicWorldSimThread::TickIdeologyMetabolism() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicWorldSim_Ideology);

    if (!Fabric) {
        return;
    }

    const int32 FactionCount = FactionDB->GetRegisteredCount();
    const double CurrentTime = FPlatformTime::Seconds();

    for (int32 i = 0; i < FactionCount; ++i) {
        FMythicFactionData *F = FactionDB->GetFactionMutableByIndex(i);
        if (!F || !F->bAlive || !F->bCanNegotiate) {
            continue;
        }

        FMythicFactionId FId;
        FId.Index = static_cast<uint8>(i);

        // Query recent significant events involving this faction
        TArray<const FMythicWorldEvent *> RecentEvents;
        Fabric->QueryEventsByCategory(
            0xFFFF, // All categories
            CurrentTime - 30.0, CurrentTime,
            Settings->IdeologyEventScanCap,
            RecentEvents
            );

        // Accumulate the moral vector of events involving this faction
        float AccumulatedVector[MoralAxisCount] = {};
        int32 RelevantEventCount = 0;

        for (const FMythicWorldEvent *Event : RecentEvents) {
            if (!Event) {
                continue;
            }
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

        // Drift ideology toward the accumulated event vector
        if (RelevantEventCount > 0) {
            const float InvCount = 1.0f / static_cast<float>(RelevantEventCount);
            for (int32 Axis = 0; Axis < MoralAxisCount; ++Axis) {
                const EMythicMoralAxis AxisEnum = static_cast<EMythicMoralAxis>(Axis);
                const float EventMean = AccumulatedVector[Axis] * InvCount;
                const float CurrentValue = F->Ideology.GetAxis(AxisEnum);
                const float DriftedValue = CurrentValue + (EventMean - CurrentValue) * Settings->IdeologyDriftRate;
                F->Ideology.GetAxisMutable(AxisEnum) = FMath::Clamp(DriftedValue, -1.0f, 1.0f);
            }
        }

        // Ideology bleed: non-territorial factions influence co-located territorial ones
        if (!F->bControlsTerritory && F->ControlledCellCount > 0) {
            for (int32 j = 0; j < FactionCount; ++j) {
                if (j == i) {
                    continue;
                }
                FMythicFactionData *Other = FactionDB->GetFactionMutableByIndex(j);
                if (!Other || !Other->bAlive || !Other->bControlsTerritory) {
                    continue;
                }

                // Simple co-location check: both factions control some cells
                if (Other->ControlledCellCount <= 0) {
                    continue;
                }

                // Bleed ideology from non-territorial to territorial
                for (int32 Axis = 0; Axis < MoralAxisCount; ++Axis) {
                    const EMythicMoralAxis AxisEnum = static_cast<EMythicMoralAxis>(Axis);
                    const float SourceValue = F->Ideology.GetAxis(AxisEnum);
                    const float TargetValue = Other->Ideology.GetAxis(AxisEnum);
                    const float BledValue = TargetValue + (SourceValue - TargetValue) * Settings->IdeologyBleedRate;
                    Other->Ideology.GetAxisMutable(AxisEnum) = FMath::Clamp(BledValue, -1.0f, 1.0f);
                }
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────
// TickFactionEvolution — Flag transitions, schisms, absorption
// ─────────────────────────────────────────────────────────────

void FMythicWorldSimThread::TickFactionEvolution() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicWorldSim_FactionEvolution);

    const int32 FactionCount = FactionDB->GetRegisteredCount();

    // Track recently annihilated factions for absorption
    TArray<int32> AnnihilatedIndices;

    for (int32 i = 0; i < FactionCount; ++i) {
        FMythicFactionData *F = FactionDB->GetFactionMutableByIndex(i);
        if (!F || !F->bAlive) {
            // Collect annihilated factions for absorption pass
            if (F && !F->bAlive && F->Population > 0) {
                AnnihilatedIndices.Add(i);
            }
            continue;
        }

        FMythicFactionId FId;
        FId.Index = static_cast<uint8>(i);

        // ── Territorial evolution: gain territory control ──
        // Non-territorial faction grows enough territory + population → becomes governing
        if (!F->bControlsTerritory &&
            F->ControlledCellCount >= Settings->TerritorialCellThreshold &&
            F->Population >= Settings->MinTerritorialPopulation &&
            F->bCanNegotiate) {

            F->bControlsTerritory = true;
            F->bHasCivilianPopulation = true;
            F->bParticipatesInTrade = true;

            if (Fabric) {
                FMythicWorldEvent EvolutionEvent;
                EvolutionEvent.PrimaryFaction = FId;
                EvolutionEvent.Significance = 0.8f;
                EvolutionEvent.CategoryFlags = EMythicEventCategory::Diplomacy;
                Fabric->AppendEvent(EvolutionEvent);
            }

            UE_LOG(LogMythWorldSim, Log, TEXT("Faction '%s' evolved: gained territorial control (cells=%d, pop=%d)"),
                   *F->DisplayName.ToString(), F->ControlledCellCount, F->Population);
        }

        // ── Territorial devolution: lose territory control ──
        if (F->bControlsTerritory &&
            F->ControlledCellCount <= Settings->DevolveCellThreshold &&
            F->MilitaryStrength < 0.1f) {

            F->bControlsTerritory = false;
            // Losing territory doesn't instantly kill economy — it shifts to
            // raiding/recruitment based on ideology

            if (Fabric) {
                FMythicWorldEvent DevolutionEvent;
                DevolutionEvent.PrimaryFaction = FId;
                DevolutionEvent.Significance = 0.8f;
                DevolutionEvent.CategoryFlags = EMythicEventCategory::Diplomacy;
                Fabric->AppendEvent(DevolutionEvent);
            }

            UE_LOG(LogMythWorldSim, Log, TEXT("Faction '%s' devolved: lost territorial control (cells=%d, military=%.2f)"),
                   *F->DisplayName.ToString(), F->ControlledCellCount, F->MilitaryStrength);
        }

        // ── Schism detection ──
        // Large territorial factions with ideological divergence can split
        if (F->bControlsTerritory && F->ControlledCellCount >= Settings->MinSchismSize) {
            // Simplified schism check: if population is large enough and ideology
            // has diverged from baseline, chance of schism increases.
            // A full implementation uses geographic cluster ideology from territory grid events.
            // For now: check if contradiction is high within the faction's recent events,
            // indicating internal ideological tension.

            float InternalDivergence = 0.0f;
            if (Fabric) {
                TArray<const FMythicWorldEvent *> FactionEvents;
                const double CurrentTime = FPlatformTime::Seconds();
                Fabric->QueryEventsByCategory(
                    0xFFFF, CurrentTime - 60.0, CurrentTime,
                    Settings->IdeologyEventScanCap,
                    FactionEvents
                    );

                // Measure variance of moral vectors in events involving this faction
                float MeanVector[MoralAxisCount] = {};
                int32 EventCount = 0;
                for (const FMythicWorldEvent *Event : FactionEvents) {
                    if (!Event) {
                        continue;
                    }
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

                    // Compute distance between event mean and faction ideology
                    for (int32 Axis = 0; Axis < MoralAxisCount; ++Axis) {
                        const EMythicMoralAxis AxisEnum = static_cast<EMythicMoralAxis>(Axis);
                        const float Delta = MeanVector[Axis] - F->Ideology.GetAxis(AxisEnum);
                        InternalDivergence += Delta * Delta;
                    }
                    InternalDivergence = FMath::Sqrt(InternalDivergence);
                }
            }

            // Schism occurs if internal divergence is high enough
            if (InternalDivergence > Settings->SchismIdeologyThreshold &&
                F->Population >= Settings->MinSchismPopulation * 2) {

                // ── Procedural faction generation ──
                FMythicFactionData NewFaction;

                // Display name: derived from parent
                const int32 NewIndex = FactionDB->GetRegisteredCount();
                NewFaction.DisplayName = FText::FromString(
                    FString::Printf(TEXT("%s Separatists"), *F->DisplayName.ToString())
                    );

                // Gameplay tag
                NewFaction.FactionTag = FGameplayTag::RequestGameplayTag(
                    FName(*FString::Printf(TEXT("Faction.Generated.%d"), NewIndex)),
                    false
                    );

                // Ideology: parent's ideology with random mutation per axis
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

                // Transfer half population
                const int32 SplitPop = F->Population / 2;
                NewFaction.Population = SplitPop;
                F->Population -= SplitPop;

                // Transfer half territory
                const int32 SplitCells = F->ControlledCellCount / 2;
                NewFaction.ControlledCellCount = SplitCells;
                F->ControlledCellCount -= SplitCells;

                // Inherit behavior flags
                NewFaction.bControlsTerritory = F->bControlsTerritory;
                NewFaction.bHasEconomy = F->bHasEconomy;
                NewFaction.bHasCivilianPopulation = F->bHasCivilianPopulation;
                NewFaction.bParticipatesInTrade = F->bParticipatesInTrade;
                NewFaction.bCanNegotiate = F->bCanNegotiate;

                // BaseProduction scaled by territory ratio
                const float TerritoryRatio = (F->ControlledCellCount + SplitCells > 0)
                    ? static_cast<float>(SplitCells) / static_cast<float>(F->ControlledCellCount + SplitCells)
                    : 0.5f;
                NewFaction.BaseProduction = F->BaseProduction;
                NewFaction.BaseProduction *= TerritoryRatio;

                // Split reserves proportionally
                const float PopRatio = static_cast<float>(SplitPop) / static_cast<float>(FMath::Max(F->Population + SplitPop, 1));
                NewFaction.Reserves = F->Reserves;
                NewFaction.Reserves *= PopRatio;
                F->Reserves *= (1.0f - PopRatio);

                // Thresholds from parent
                NewFaction.DisapproveThreshold = F->DisapproveThreshold;
                NewFaction.CondemnThreshold = F->CondemnThreshold;
                NewFaction.HostileThreshold = F->HostileThreshold;

                // Register the new faction
                FMythicFactionId NewId = FactionDB->RegisterFaction(NewFaction);

                if (NewId.IsValid()) {
                    // New faction starts Hostile to parent
                    FactionDB->SetRelationship(FId, NewId, EMythicFactionRelation::Hostile);

                    // Inherit parent's other relationships, decayed by one tier
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
    }

    // ── Absorption: annihilated factions' refugees go to nearest ally ──
    for (int32 DeadIdx : AnnihilatedIndices) {
        FMythicFactionData *Dead = FactionDB->GetFactionMutableByIndex(DeadIdx);
        if (!Dead || Dead->Population <= 0) {
            continue;
        }

        FMythicFactionId DeadId;
        DeadId.Index = static_cast<uint8>(DeadIdx);

        // Find the closest Allied or Friendly alive faction
        int32 BestAbsorber = -1;
        EMythicFactionRelation BestRel = EMythicFactionRelation::Hostile;

        for (int32 j = 0; j < FactionCount; ++j) {
            FMythicFactionData *Candidate = FactionDB->GetFactionMutableByIndex(j);
            if (!Candidate || !Candidate->bAlive || !Candidate->bHasCivilianPopulation) {
                continue;
            }

            FMythicFactionId CandId;
            CandId.Index = static_cast<uint8>(j);
            const EMythicFactionRelation Rel = FactionDB->GetWriteRelationship(DeadId, CandId);

            if (Rel == EMythicFactionRelation::Allied ||
                (Rel == EMythicFactionRelation::Friendly && BestRel != EMythicFactionRelation::Allied)) {
                BestAbsorber = j;
                BestRel = Rel;
            }
        }

        if (BestAbsorber >= 0) {
            FMythicFactionData *Absorber = FactionDB->GetFactionMutableByIndex(BestAbsorber);
            const int32 Refugees = static_cast<int32>(
                static_cast<float>(Dead->Population) * Settings->AbsorptionFraction
            );
            Absorber->Population += Refugees;
            Dead->Population = 0;

            if (Fabric) {
                FMythicFactionId AbsorberId;
                AbsorberId.Index = static_cast<uint8>(BestAbsorber);
                FMythicWorldEvent AbsorbEvent;
                AbsorbEvent.PrimaryFaction = AbsorberId;
                AbsorbEvent.SecondaryFaction = DeadId;
                AbsorbEvent.Significance = 0.6f;
                AbsorbEvent.CategoryFlags = EMythicEventCategory::Diplomacy;
                Fabric->AppendEvent(AbsorbEvent);
            }

            UE_LOG(LogMythWorldSim, Log, TEXT("Faction '%s' absorbed %d refugees from '%s'"),
                   *Absorber->DisplayName.ToString(), Refugees, *Dead->DisplayName.ToString());
        }
    }
}

void FMythicWorldSimThread::TickSchemeEngine() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicWorldSim_Schemes);
    // Phase 5: Progress ~5-10 active schemes. Each scheme is a state machine
    // advancing through phases (plan → prepare → execute → resolve).
}

void FMythicWorldSimThread::TickCrystallization() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicWorldSim_Crystallization);
    // Phase 6: Batch of 50 persistent NPCs per tick, round-robin.
    // Age check → memory detail → trait tag replacement.
    // Cultural promotion on counter threshold.
}

void FMythicWorldSimThread::TickHistoryAppend() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicWorldSim_HistoryAppend);
    // Phase 6: Write significant events to the causal fabric.
    // Background-generated events from sim (diplomatic shifts, faction creation, etc.)
}
