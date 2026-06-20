// Mythic Living World — Scheme Engine Implementation
// Background-thread faction scheme generation and progression.

#include "World/LivingWorld/Simulation/SchemeEngine.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"

DEFINE_LOG_CATEGORY(LogMythScheme);

// ─────────────────────────────────────────────────────────────
// Initialization
// ─────────────────────────────────────────────────────────────

void UMythicSchemeEngine::Initialize(
    UMythicFactionDatabase *InFactionDB,
    UMythicCausalFabric *InFabric,
    UMythicTerritoryGrid *InTerritoryGrid,
    const UMythicLivingWorldSettings *InSettings) {
    FactionDB = InFactionDB;
    Fabric = InFabric;
    TerritoryGrid = InTerritoryGrid;
    Settings = InSettings;

    // Load configuration from settings
    if (Settings) {
        GenerationTickInterval = Settings->SchemeGenerationTickInterval;
        MaxSchemesPerFaction = Settings->MaxSchemesPerFaction;
        MaxTotalSchemes = Settings->MaxTotalSchemes;
        SchemeBaseProbability = Settings->SchemeBaseProbability;
    }

    ActiveSchemes.Reserve(MaxTotalSchemes);

    UE_LOG(LogMythScheme, Log, TEXT("SchemeEngine initialized: MaxTotal=%d, MaxPerFaction=%d, GenInterval=%d"),
           MaxTotalSchemes, MaxSchemesPerFaction, GenerationTickInterval);
}

// ─────────────────────────────────────────────────────────────
// Background Thread Tick
// ─────────────────────────────────────────────────────────────

void UMythicSchemeEngine::TickSchemes(float SimDeltaTime, uint32 SimTickIndex) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicSchemeEngine_Tick);

    if (!FactionDB || !Fabric) {
        return;
    }

    // Generate new schemes periodically (not every tick)
    if (SimTickIndex % GenerationTickInterval == 0) {
        GenerateSchemes(SimDeltaTime, SimTickIndex);
    }

    // Progress all active schemes
    {
        FScopeLock Lock(&SchemeLock);

        for (int32 i = ActiveSchemes.Num() - 1; i >= 0; --i) {
            FMythicScheme &Scheme = ActiveSchemes[i];

            if (!Scheme.IsActive()) {
                // Clean up completed/failed schemes
                ActiveSchemes.RemoveAtSwap(i);
                continue;
            }

            ProgressScheme(Scheme, SimDeltaTime);
        }
    }
}

// ─────────────────────────────────────────────────────────────
// Scheme Generation
// ─────────────────────────────────────────────────────────────

void UMythicSchemeEngine::GenerateSchemes(float SimDeltaTime, uint32 SimTickIndex) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicSchemeEngine_Generate);

    FScopeLock Lock(&SchemeLock);

    if (ActiveSchemes.Num() >= MaxTotalSchemes) {
        return; // Global cap reached
    }

    const int32 FactionCount = FactionDB->GetRegisteredCount();

    for (int32 FIdx = 0; FIdx < FactionCount; ++FIdx) {
        // Count existing schemes for this faction
        FMythicFactionId FactionId;
        FactionId.Index = static_cast<uint8>(FIdx);

        int32 FactionSchemeCount = 0;
        for (const FMythicScheme &S : ActiveSchemes) {
            if (S.OriginFaction == FactionId) {
                ++FactionSchemeCount;
            }
        }

        if (FactionSchemeCount >= MaxSchemesPerFaction) {
            continue; // Per-faction cap
        }

        // Get faction data to evaluate eligibility (background thread uses write buffer)
        FMythicFactionData *FactionDataPtr = FactionDB->GetFactionMutableByIndex(FIdx);
        if (!FactionDataPtr) {
            continue;
        }
        const FMythicFactionData &FactionData = *FactionDataPtr;

        // Skip dead factions
        if (!FactionData.bAlive) {
            continue;
        }

        // Determine eligible scheme types
        TArray<EMythicSchemeType> EligibleTypes;
        GetEligibleSchemeTypes(FIdx, EligibleTypes);

        if (EligibleTypes.Num() == 0) {
            continue;
        }

        // Probabilistic generation — not every eligible faction generates a scheme
        // Base probability scales with faction population and military strength
        const float MilitaryBoost = FactionData.MilitaryStrength * SchemeBaseProbability;

        if (FMath::FRand() > SchemeBaseProbability + MilitaryBoost) {
            continue;
        }

        // Pick a random eligible scheme type
        const int32 TypeIndex = FMath::RandRange(0, EligibleTypes.Num() - 1);
        const EMythicSchemeType SchemeType = EligibleTypes[TypeIndex];

        // Find a valid target faction (hostile or unfriendly)
        FMythicFactionId TargetFaction;
        for (int32 TIdx = 0; TIdx < FactionCount; ++TIdx) {
            if (TIdx == FIdx) {
                continue;
            }

            // Skip dead candidates — symmetric with the origin's bAlive gate above. AnnihilateFaction zeroes a
            // faction's stats and sets bAlive=false but intentionally leaves its relationship rows intact (indices
            // are never recycled), so a living faction's stale Hostile/Unfriendly relation toward an annihilated one
            // would otherwise make the corpse a scheme target — wasting the per-faction/global scheme budget and
            // emitting nonsensical diplomacy/chronicle events against a faction with Population=0, bAlive=false.
            const FMythicFactionData *CandidateData = FactionDB->GetFactionMutableByIndex(TIdx);
            if (!CandidateData || !CandidateData->bAlive) {
                continue;
            }

            FMythicFactionId Candidate;
            Candidate.Index = static_cast<uint8>(TIdx);

            const EMythicFactionRelation Relation = FactionDB->GetWriteRelationship(FactionId, Candidate);
            if (Relation == EMythicFactionRelation::Hostile ||
                Relation == EMythicFactionRelation::Unfriendly) {
                TargetFaction = Candidate;
                break;
            }
        }

        if (!TargetFaction.IsValid()) {
            continue; // No valid targets
        }

        // Create the scheme
        FMythicScheme NewScheme;
        NewScheme.SchemeId = NextSchemeId++;
        NewScheme.OriginFaction = FactionId;
        NewScheme.TargetFaction = TargetFaction;
        NewScheme.Type = SchemeType;
        NewScheme.State = EMythicSchemeState::InProgress;
        NewScheme.Progress = 0.0f;
        NewScheme.ProgressRate = CalculateProgressRate(NewScheme, FIdx);
        NewScheme.DetectionRisk = CalculateDetectionRisk(NewScheme, TargetFaction.Index);
        NewScheme.StartGameTime = 0.0; // Will be set by sim thread's world time

        // TerritoryReclaim flips a REAL contested cell at execution — pick one the TARGET actually controls now. Without
        // this, TargetCell defaults to (0,0) and a reclaim would overwrite whatever faction holds the grid corner. (The
        // ApplySchemeEffects guard re-checks ownership at execution, so a target that loses the cell meanwhile is safe.)
        if (SchemeType == EMythicSchemeType::TerritoryReclaim && TerritoryGrid) {
            TArray<FMythicCellCoord> TargetCells;
            TerritoryGrid->GetFactionCells(TargetFaction, 1, TargetCells);
            if (TargetCells.Num() > 0) {
                NewScheme.TargetCell = TargetCells[0];
            }
        }

        ActiveSchemes.Add(NewScheme);

        UE_LOG(LogMythScheme, Log,
               TEXT("Scheme %d generated: Faction %d → %d, Type=%d, Rate=%.3f, Risk=%.3f"),
               NewScheme.SchemeId,
               FIdx, TargetFaction.Index,
               static_cast<int32>(SchemeType),
               NewScheme.ProgressRate,
               NewScheme.DetectionRisk);

        // Respect global cap
        if (ActiveSchemes.Num() >= MaxTotalSchemes) {
            break;
        }
    }
}

void UMythicSchemeEngine::GetEligibleSchemeTypes(int32 FactionIndex, TArray<EMythicSchemeType> &OutEligibleTypes) const {
    OutEligibleTypes.Reset();

    FMythicFactionData *DataPtr = FactionDB->GetFactionMutableByIndex(FactionIndex);
    if (!DataPtr) {
        return;
    }
    const FMythicFactionData &Data = *DataPtr;

    // Assassination — requires some military strength
    if (Data.MilitaryStrength > 0.2f) {
        OutEligibleTypes.Add(EMythicSchemeType::Assassination);
    }

    // Trade Disruption — requires economy
    if (Data.bHasEconomy) {
        OutEligibleTypes.Add(EMythicSchemeType::TradeDisruption);
    }

    // Territory Reclaim — requires territory control and cells lost
    if (Data.bControlsTerritory && Data.MilitaryStrength > 0.4f) {
        OutEligibleTypes.Add(EMythicSchemeType::TerritoryReclaim);
    }

    // Spy Infiltration — any faction with population can spy
    if (Data.Population > 20) {
        OutEligibleTypes.Add(EMythicSchemeType::SpyInfiltration);
    }

    // Military Raid — requires strong military
    if (Data.MilitaryStrength > 0.6f) {
        OutEligibleTypes.Add(EMythicSchemeType::MilitaryRaid);
    }

    // Diplomatic Pressure — any established faction
    if (Data.Population > 50) {
        OutEligibleTypes.Add(EMythicSchemeType::DiplomaticPressure);
    }

    // Companion Recruitment — faction must be hostile toward player and have infiltration capability
    // Requires significant population (proxy for having spies/agents) and military strength
    if (Data.MilitaryStrength > 0.3f && Data.Population > 100 && Data.bCanNegotiate) {
        OutEligibleTypes.Add(EMythicSchemeType::CompanionRecruitment);
    }
}

float UMythicSchemeEngine::CalculateProgressRate(const FMythicScheme &Scheme, int32 FactionIndex) const {
    FMythicFactionData *DataPtr = FactionDB->GetFactionMutableByIndex(FactionIndex);
    if (!DataPtr) {
        return 0.01f;
    }
    const FMythicFactionData &Data = *DataPtr;

    // Base rate scaled by scheme type difficulty
    float BaseRate = 0.02f;

    switch (Scheme.Type) {
    case EMythicSchemeType::Assassination:
        BaseRate = 0.015f; // Slow — complex operation
        break;
    case EMythicSchemeType::TradeDisruption:
        BaseRate = 0.03f; // Medium — economic sabotage
        break;
    case EMythicSchemeType::TerritoryReclaim:
        BaseRate = 0.01f; // Very slow — military operation
        break;
    case EMythicSchemeType::SpyInfiltration:
        BaseRate = 0.02f; // Medium
        break;
    case EMythicSchemeType::MilitaryRaid:
        BaseRate = 0.025f; // Medium-fast — direct action
        break;
    case EMythicSchemeType::DiplomaticPressure:
        BaseRate = 0.04f; // Fast — diplomatic channels
        break;
    default:
        break;
    }

    // Scale by faction capability
    const float MilitaryMod = 0.5f + Data.MilitaryStrength * 0.5f; // [0.5, 1.0]
    const float PopulationMod = FMath::Min(static_cast<float>(Data.Population) / 200.0f, 1.0f); // Normalize to 200

    return BaseRate * MilitaryMod * PopulationMod;
}

float UMythicSchemeEngine::CalculateDetectionRisk(const FMythicScheme &Scheme, int32 TargetFactionIndex) const {
    FMythicFactionData *TargetDataPtr = FactionDB->GetFactionMutableByIndex(TargetFactionIndex);
    if (!TargetDataPtr) {
        return 0.05f;
    }
    const FMythicFactionData &TargetData = *TargetDataPtr;

    // Base detection risk per scheme type
    float BaseRisk = 0.03f;

    switch (Scheme.Type) {
    case EMythicSchemeType::Assassination:
        BaseRisk = 0.05f; // High risk — one slip and you're caught
        break;
    case EMythicSchemeType::TradeDisruption:
        BaseRisk = 0.02f; // Low risk — hard to trace
        break;
    case EMythicSchemeType::TerritoryReclaim:
        BaseRisk = 0.08f; // Very high — military movements are visible
        break;
    case EMythicSchemeType::SpyInfiltration:
        BaseRisk = 0.04f; // Medium — counter-intelligence
        break;
    case EMythicSchemeType::MilitaryRaid:
        BaseRisk = 0.10f; // Near-certain — armies are detected
        break;
    case EMythicSchemeType::DiplomaticPressure:
        BaseRisk = 0.01f; // Low — plausibly deniable
        break;
    default:
        break;
    }

    // Scale by target faction's capability — larger factions detect more
    const float PopulationMod = FMath::Min(static_cast<float>(TargetData.Population) / 200.0f, 1.5f);

    return FMath::Clamp(BaseRisk * PopulationMod, 0.005f, 0.2f);
}

// ─────────────────────────────────────────────────────────────
// Scheme Progression
// ─────────────────────────────────────────────────────────────

void UMythicSchemeEngine::ProgressScheme(FMythicScheme &Scheme, float SimDeltaTime) {
    // Progress toward completion
    Scheme.Progress += Scheme.ProgressRate * SimDeltaTime;

    // Detection roll
    if (FMath::FRand() < Scheme.DetectionRisk * SimDeltaTime) {
        OnSchemeDiscovered(Scheme);
        return;
    }

    // Check completion
    if (Scheme.Progress >= 1.0f) {
        ExecuteScheme(Scheme);
    }
}

// Move a faction relationship one step toward Hostile (Allied→Friendly→Neutral→Unfriendly→Hostile), clamped.
static EMythicFactionRelation WorsenRelation(EMythicFactionRelation R) {
    const uint8 Cur = static_cast<uint8>(R);
    const uint8 Worst = static_cast<uint8>(EMythicFactionRelation::Hostile);
    return static_cast<EMythicFactionRelation>(Cur < Worst ? Cur + 1 : Worst);
}

void UMythicSchemeEngine::ApplySchemeEffects(const FMythicScheme &Scheme) {
    // Runs on the sim thread under the already-held SimulationLock (same context as TickEconomy), so direct FactionDB
    // write-buffer mutation is safe and needs no extra lock. All magnitudes come from designer-tunable settings.
    if (!FactionDB || !Settings) {
        return;
    }
    FMythicFactionData *Target = Scheme.TargetFaction.IsValid()
        ? FactionDB->GetFactionMutableByIndex(Scheme.TargetFaction.Index)
        : nullptr;
    FMythicFactionData *Origin = Scheme.OriginFaction.IsValid()
        ? FactionDB->GetFactionMutableByIndex(Scheme.OriginFaction.Index)
        : nullptr;
    if (!Target) {
        return; // every implemented effect mutates the target faction
    }

    switch (Scheme.Type) {
    case EMythicSchemeType::TradeDisruption: {
        // Drain a fraction of the target's POSITIVE liquid reserves (inverse of raid income). Only positive stocks can
        // be looted — disrupting a faction already in deficit must not paradoxically reduce its deficit.
        const float F = Settings->SchemeTradeDisruptionFraction;
        Target->Reserves.Wealth -= FMath::Max(0.0f, Target->Reserves.Wealth) * F;
        Target->Reserves.Materials -= FMath::Max(0.0f, Target->Reserves.Materials) * F;
        break;
    }
    case EMythicSchemeType::MilitaryRaid: {
        // Burn arms (MilitaryStrength re-derives down next economy tick) + kill a fraction of the target's population.
        Target->Reserves.Arms = FMath::Max(0.0f, Target->Reserves.Arms - Settings->SchemeRaidArmsLoss);
        Target->Population = FMath::Max(0, Target->Population -
                                        FMath::RoundToInt(Target->Population * Settings->SchemeRaidPopulationLossFraction));
        break;
    }
    case EMythicSchemeType::DiplomaticPressure: {
        // A successful pressure campaign hardens the target against the schemer — one step toward Hostile (symmetric).
        FactionDB->SetRelationship(Scheme.OriginFaction, Scheme.TargetFaction,
                                   WorsenRelation(FactionDB->GetWriteRelationship(Scheme.OriginFaction, Scheme.TargetFaction)));
        break;
    }
    case EMythicSchemeType::Assassination: {
        // Behead the target faction: vacate its leader slot (sim-thread-safe — identical to the vacancy clear the sim
        // already performs in TickCrystallization) + a small population loss. The MASS leader-entity kill + the
        // succession spawn are GAME-THREAD contracts and are deferred (logged follow-up), NOT faked here.
        Target->LeaderEntityId = 0;
        Target->LeaderSignificanceScore = 0.0f;
        Target->Population = FMath::Max(0, Target->Population - Settings->SchemeAssassinationPopulationLoss);
        break;
    }
    case EMythicSchemeType::TerritoryReclaim: {
        // Flip the contested cell to the origin + move the cell tally — ONLY if the TARGET still actually dominates
        // that cell. The cell is chosen at generation from the target's territory, but it may default to (0,0) or have
        // changed hands since; without this guard a reclaim could overwrite an UNRELATED faction's cell and desync the
        // tallies (review MEDIUM). A full settlement handoff needs the SettlementRegistry (not held here) — deferred.
        if (Origin && TerritoryGrid &&
            TerritoryGrid->GetDominantFaction(Scheme.TargetCell).Index == Scheme.TargetFaction.Index) {
            TerritoryGrid->SetCellInfluence(Scheme.TargetCell, Scheme.OriginFaction, 1.0f);
            const int32 N = Settings->SchemeTerritoryReclaimCells;
            Target->ControlledCellCount = FMath::Max(0, Target->ControlledCellCount - N);
            Origin->ControlledCellCount += N;
        }
        break;
    }
    case EMythicSchemeType::SpyInfiltration:
    case EMythicSchemeType::CompanionRecruitment:
        // Intentionally effect-less: a spy-NPC placement and a player-party defection are GAME-THREAD / MASS /
        // PartySubsystem contracts the sim thread cannot reach. Chronicle-event-only BY DESIGN (deferred, logged) —
        // they apply no mechanical effect and do not pretend to.
        break;
    default:
        break;
    }

    UE_LOG(LogMythScheme, Verbose, TEXT("ApplySchemeEffects: Type=%d Origin=%d Target=%d"),
           static_cast<int32>(Scheme.Type), Scheme.OriginFaction.Index, Scheme.TargetFaction.Index);
}

void UMythicSchemeEngine::ExecuteScheme(FMythicScheme &Scheme) {
    Scheme.State = EMythicSchemeState::Succeeded;
    Scheme.Progress = 1.0f;

    // Apply the REAL faction-state consequences (the half this function long promised but skipped).
    ApplySchemeEffects(Scheme);

    // Write event to Causal Fabric
    if (Fabric) {
        FMythicWorldEvent Event;
        Event.PrimaryFaction = Scheme.OriginFaction;
        Event.SecondaryFaction = Scheme.TargetFaction;
        Event.Cell = Scheme.TargetCell;
        Event.Significance = 0.8f;
        Event.CategoryFlags = EMythicEventCategory::Scheme;
        Event.EventTag = TAG_LIVINGWORLD_EVENT_SCHEME_COMPLETED; // chronicle now reads "Scheme Completed", not "World Event"

        // Set event-specific significance and categories
        switch (Scheme.Type) {
        case EMythicSchemeType::Assassination:
            Event.Significance = 1.0f;
            Event.CategoryFlags |= EMythicEventCategory::Death;
            break;
        case EMythicSchemeType::TradeDisruption:
            Event.CategoryFlags |= EMythicEventCategory::Trade;
            break;
        case EMythicSchemeType::TerritoryReclaim:
            Event.Significance = 0.9f;
            Event.CategoryFlags |= EMythicEventCategory::Territory;
            break;
        case EMythicSchemeType::MilitaryRaid:
            Event.Significance = 0.9f;
            Event.CategoryFlags |= EMythicEventCategory::Combat;
            break;
        case EMythicSchemeType::SpyInfiltration:
            Event.CategoryFlags |= EMythicEventCategory::Social;
            break;
        case EMythicSchemeType::DiplomaticPressure:
            Event.CategoryFlags |= EMythicEventCategory::Diplomacy;
            break;
        default:
            break;
        }

        Fabric->AppendEvent(Event);
    }

    UE_LOG(LogMythScheme, Log,
           TEXT("Scheme %d executed: Faction %d → %d, Type=%d"),
           Scheme.SchemeId,
           Scheme.OriginFaction.Index,
           Scheme.TargetFaction.Index,
           static_cast<int32>(Scheme.Type));
}

void UMythicSchemeEngine::OnSchemeDiscovered(FMythicScheme &Scheme) {
    Scheme.State = EMythicSchemeState::Discovered;

    // Getting caught is itself a diplomatic incident: the target now trusts the schemer one step less (symmetric).
    // A discovered scheme is a real outcome, not just a chronicle line.
    if (FactionDB && Scheme.OriginFaction.IsValid() && Scheme.TargetFaction.IsValid()) {
        FactionDB->SetRelationship(Scheme.OriginFaction, Scheme.TargetFaction,
                                   WorsenRelation(FactionDB->GetWriteRelationship(Scheme.OriginFaction, Scheme.TargetFaction)));
    }

    // Write discovery event to Causal Fabric
    if (Fabric) {
        FMythicWorldEvent Event;
        Event.PrimaryFaction = Scheme.TargetFaction; // Target discovers it
        Event.SecondaryFaction = Scheme.OriginFaction; // Origin was scheming
        Event.Cell = Scheme.TargetCell;
        Event.Significance = 0.6f;
        Event.CategoryFlags = EMythicEventCategory::Scheme | EMythicEventCategory::Diplomacy;
        Event.EventTag = TAG_LIVINGWORLD_EVENT_SCHEME_DISCOVERED; // chronicle now reads "Scheme Discovered", not "World Event"

        Fabric->AppendEvent(Event);
    }

    UE_LOG(LogMythScheme, Log,
           TEXT("Scheme %d discovered! Faction %d was plotting against %d, Type=%d"),
           Scheme.SchemeId,
           Scheme.OriginFaction.Index,
           Scheme.TargetFaction.Index,
           static_cast<int32>(Scheme.Type));
}

// ─────────────────────────────────────────────────────────────
// Game Thread Queries (Lock-Protected)
// ─────────────────────────────────────────────────────────────

TArray<FMythicScheme> UMythicSchemeEngine::GetActiveSchemes() const {
    FScopeLock Lock(&SchemeLock);
    return ActiveSchemes; // Returns a copy
}

TArray<FMythicScheme> UMythicSchemeEngine::GetSchemesByFaction(FMythicFactionId Faction) const {
    FScopeLock Lock(&SchemeLock);

    TArray<FMythicScheme> Result;
    for (const FMythicScheme &S : ActiveSchemes) {
        if (S.OriginFaction == Faction) {
            Result.Add(S);
        }
    }
    return Result;
}

int32 UMythicSchemeEngine::GetActiveSchemeCount() const {
    FScopeLock Lock(&SchemeLock);
    return ActiveSchemes.Num();
}

void UMythicSchemeEngine::Serialize(FArchive &Ar) {
    int32 Version = 1;
    Ar << Version;

    Ar << NextSchemeId;

    int32 SchemeCount = ActiveSchemes.Num();
    Ar << SchemeCount;

    if (Ar.IsLoading()) {
        // Bound-check before SetNum: a desynced/corrupted stream (e.g. a differing optional-subsystem set upstream in
        // LoadLivingWorld) can yield a garbage count; an unbounded SetNum would then attempt a massive allocation
        // (OOM/crash). 1,000,000 is far above any legitimate scheme count (the real cap is MaxTotalSchemes — dozens).
        if (SchemeCount < 0 || SchemeCount > 1000000) {
            UE_LOG(LogMythScheme, Error, TEXT("SchemeEngine::Serialize: implausible SchemeCount %d — aborting load"), SchemeCount);
            Ar.SetError();
            return;
        }
        ActiveSchemes.SetNum(SchemeCount);
    }

    for (int32 i = 0; i < SchemeCount; ++i) {
        FMythicScheme &S = ActiveSchemes[i];
        Ar << S.SchemeId;
        Ar << S.OriginFaction.Index;
        Ar << S.TargetFaction.Index;

        uint8 TypeVal = static_cast<uint8>(S.Type);
        Ar << TypeVal;
        if (Ar.IsLoading()) {
            S.Type = static_cast<EMythicSchemeType>(TypeVal);
        }

        uint8 StateVal = static_cast<uint8>(S.State);
        Ar << StateVal;
        if (Ar.IsLoading()) {
            S.State = static_cast<EMythicSchemeState>(StateVal);
        }

        Ar << S.Progress;
        Ar << S.ProgressRate;
        Ar << S.DetectionRisk;
        Ar << S.StartGameTime;
        Ar << S.TargetCell.X;
        Ar << S.TargetCell.Y;
    }
}
