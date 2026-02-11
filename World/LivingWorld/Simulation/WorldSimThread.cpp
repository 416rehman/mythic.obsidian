// Mythic Living World System — Background Simulation Thread Implementation

#include "World/LivingWorld/Simulation/WorldSimThread.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "HAL/PlatformProcess.h"

FMythicWorldSimThread::FMythicWorldSimThread() {}

FMythicWorldSimThread::~FMythicWorldSimThread() {
    StopThread();
}

void FMythicWorldSimThread::Setup(
    UMythicCausalFabric *InFabric,
    UMythicFactionDatabase *InFactionDB,
    UMythicTerritoryGrid *InTerritoryGrid,
    float InTickIntervalSeconds) {
    Fabric = InFabric;
    FactionDB = InFactionDB;
    TerritoryGrid = InTerritoryGrid;
    TickIntervalSeconds = FMath::Max(InTickIntervalSeconds, 0.1f);
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

    TickEconomy();
    TickPopulation();
    TickDiplomacy();
    TickTerritoryPropagation();
    TickIdeologyMetabolism();
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
// Simulation sub-steps — Stubs for Phase 2 implementation
// Each operates only on write buffers (background thread safe)
// ─────────────────────────────────────────────────────────────

void FMythicWorldSimThread::TickEconomy() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicWorldSim_Economy);
    // Phase 2: Per-faction resource delta. Reads territory cell counts,
    // adjusts EconomicStrength based on controlled cells and trade routes.
}

void FMythicWorldSimThread::TickPopulation() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicWorldSim_Population);
    // Phase 2: Birth/death/migration per faction.
    // Reads faction data, adjusts Population based on economic strength and territory.
}

void FMythicWorldSimThread::TickDiplomacy() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicWorldSim_Diplomacy);
    // Phase 2: Relationship threshold checks per faction pair (~190 checks for 20 factions).
    // Reads recent events from fabric involving faction pairs.
}

void FMythicWorldSimThread::TickTerritoryPropagation() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicWorldSim_Territory);

    if (TerritoryGrid) {
        TerritoryGrid->PropagateInfluence();
    }
}

void FMythicWorldSimThread::TickIdeologyMetabolism() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicWorldSim_Ideology);
    // Phase 2: One vector adjustment per faction per tick based on recent events
    // impacting that faction. Dirty-flags NPC template regeneration.
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
