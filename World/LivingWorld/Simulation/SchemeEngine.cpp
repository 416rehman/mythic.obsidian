
#include "World/LivingWorld/Simulation/SchemeEngine.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "GAS/Executions/MythicCombatRoll.h"

DEFINE_LOG_CATEGORY(LogMythScheme);


void UMythicSchemeEngine::Initialize(
    UMythicFactionDatabase *InFactionDB,
    UMythicCausalFabric *InFabric,
    UMythicTerritoryGrid *InTerritoryGrid,
    const UMythicLivingWorldSettings *InSettings) {
    FactionDB = InFactionDB;
    Fabric = InFabric;
    TerritoryGrid = InTerritoryGrid;
    Settings = InSettings;

    if (Settings) {
        GenerationTickInterval = Settings->SchemeGenerationTickInterval;
        MaxSchemesPerFaction = Settings->MaxSchemesPerFaction;
        MaxTotalSchemes = Settings->MaxTotalSchemes;
        SchemeBaseProbability = Settings->SchemeBaseProbability;
    }

    if (GenerationTickInterval <= 0) {
        UE_LOG(LogMythScheme, Warning,
               TEXT("SchemeGenerationTickInterval misconfigured (%d <= 0); clamping to 1 to avoid a modulo-by-zero on the sim thread."),
               GenerationTickInterval);
        GenerationTickInterval = 1;
    }

    ActiveSchemes.Reserve(MaxTotalSchemes);

    UE_LOG(LogMythScheme, Log, TEXT("SchemeEngine initialized: MaxTotal=%d, MaxPerFaction=%d, GenInterval=%d"),
           MaxTotalSchemes, MaxSchemesPerFaction, GenerationTickInterval);
}


void UMythicSchemeEngine::TickSchemes(float SimDeltaTime, uint32 SimTickIndex) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicSchemeEngine_Tick);

    if (!FactionDB || !Fabric) {
        return;
    }

    if (SimTickIndex % GenerationTickInterval == 0) {
        GenerateSchemes(SimDeltaTime, SimTickIndex);
    }

    {
        FScopeLock Lock(&SchemeLock);

        for (int32 i = ActiveSchemes.Num() - 1; i >= 0; --i) {
            FMythicScheme &Scheme = ActiveSchemes[i];

            if (!Scheme.IsActive()) {
                ActiveSchemes.RemoveAtSwap(i);
                continue;
            }

            ProgressScheme(Scheme, SimDeltaTime);
        }
    }
}


void UMythicSchemeEngine::GenerateSchemes(float SimDeltaTime, uint32 SimTickIndex) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicSchemeEngine_Generate);

    FScopeLock Lock(&SchemeLock);

    if (ActiveSchemes.Num() >= MaxTotalSchemes) {
        return;
    }

    const int32 FactionCount = FactionDB->GetRegisteredCount();

    for (int32 FIdx = 0; FIdx < FactionCount; ++FIdx) {
        FMythicFactionId FactionId;
        FactionId.Index = static_cast<uint8>(FIdx);

        int32 FactionSchemeCount = 0;
        for (const FMythicScheme &S : ActiveSchemes) {
            if (S.OriginFaction == FactionId) {
                ++FactionSchemeCount;
            }
        }

        if (FactionSchemeCount >= MaxSchemesPerFaction) {
            continue;
        }

        FMythicFactionData *FactionDataPtr = FactionDB->GetFactionMutableByIndex(FIdx);
        if (!FactionDataPtr) {
            continue;
        }
        const FMythicFactionData &FactionData = *FactionDataPtr;

        if (!FactionData.bAlive) {
            continue;
        }

        TArray<EMythicSchemeType> EligibleTypes;
        GetEligibleSchemeTypes(FIdx, EligibleTypes);

        if (EligibleTypes.Num() == 0) {
            continue;
        }

        const float MilitaryBoost = FactionData.MilitaryStrength * SchemeBaseProbability;

        if (!MythicCombat::RollSucceeds(SchemeBaseProbability + MilitaryBoost, FMath::FRand())) {
            continue;
        }

        const int32 TypeIndex = FMath::RandRange(0, EligibleTypes.Num() - 1);
        const EMythicSchemeType SchemeType = EligibleTypes[TypeIndex];

        FMythicFactionId TargetFaction;
        for (int32 TIdx = 0; TIdx < FactionCount; ++TIdx) {
            if (TIdx == FIdx) {
                continue;
            }

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
            continue;
        }

        FMythicScheme NewScheme;
        NewScheme.SchemeId = NextSchemeId++;
        NewScheme.OriginFaction = FactionId;
        NewScheme.TargetFaction = TargetFaction;
        NewScheme.Type = SchemeType;
        NewScheme.State = EMythicSchemeState::InProgress;
        NewScheme.Progress = 0.0f;
        NewScheme.ProgressRate = CalculateProgressRate(NewScheme, FIdx);
        NewScheme.DetectionRisk = CalculateDetectionRisk(NewScheme, TargetFaction.Index);
        NewScheme.StartGameTime = 0.0;

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

    if (Data.MilitaryStrength > 0.2f) {
        OutEligibleTypes.Add(EMythicSchemeType::Assassination);
    }

    if (Data.bHasEconomy) {
        OutEligibleTypes.Add(EMythicSchemeType::TradeDisruption);
    }

    if (Data.bControlsTerritory && Data.MilitaryStrength > 0.4f) {
        OutEligibleTypes.Add(EMythicSchemeType::TerritoryReclaim);
    }

    if (Data.Population > 20) {
        OutEligibleTypes.Add(EMythicSchemeType::SpyInfiltration);
    }

    if (Data.MilitaryStrength > 0.6f) {
        OutEligibleTypes.Add(EMythicSchemeType::MilitaryRaid);
    }

    if (Data.Population > 50) {
        OutEligibleTypes.Add(EMythicSchemeType::DiplomaticPressure);
    }

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

    float BaseRate = 0.02f;

    switch (Scheme.Type) {
    case EMythicSchemeType::Assassination:
        BaseRate = 0.015f;
        break;
    case EMythicSchemeType::TradeDisruption:
        BaseRate = 0.03f;
        break;
    case EMythicSchemeType::TerritoryReclaim:
        BaseRate = 0.01f;
        break;
    case EMythicSchemeType::SpyInfiltration:
        BaseRate = 0.02f;
        break;
    case EMythicSchemeType::MilitaryRaid:
        BaseRate = 0.025f;
        break;
    case EMythicSchemeType::DiplomaticPressure:
        BaseRate = 0.04f;
        break;
    default:
        break;
    }

    const float MilitaryMod = 0.5f + Data.MilitaryStrength * 0.5f;
    const float PopulationMod = FMath::Min(static_cast<float>(Data.Population) / 200.0f, 1.0f);

    return BaseRate * MilitaryMod * PopulationMod;
}

float UMythicSchemeEngine::CalculateDetectionRisk(const FMythicScheme &Scheme, int32 TargetFactionIndex) const {
    FMythicFactionData *TargetDataPtr = FactionDB->GetFactionMutableByIndex(TargetFactionIndex);
    if (!TargetDataPtr) {
        return 0.05f;
    }
    const FMythicFactionData &TargetData = *TargetDataPtr;

    float BaseRisk = 0.03f;

    switch (Scheme.Type) {
    case EMythicSchemeType::Assassination:
        BaseRisk = 0.05f;
        break;
    case EMythicSchemeType::TradeDisruption:
        BaseRisk = 0.02f;
        break;
    case EMythicSchemeType::TerritoryReclaim:
        BaseRisk = 0.08f;
        break;
    case EMythicSchemeType::SpyInfiltration:
        BaseRisk = 0.04f;
        break;
    case EMythicSchemeType::MilitaryRaid:
        BaseRisk = 0.10f;
        break;
    case EMythicSchemeType::DiplomaticPressure:
        BaseRisk = 0.01f;
        break;
    default:
        break;
    }

    const float PopulationMod = FMath::Min(static_cast<float>(TargetData.Population) / 200.0f, 1.5f);

    return FMath::Clamp(BaseRisk * PopulationMod, 0.005f, 0.2f);
}


void UMythicSchemeEngine::ProgressScheme(FMythicScheme &Scheme, float SimDeltaTime) {
    Scheme.Progress += Scheme.ProgressRate * SimDeltaTime;

    if (MythicCombat::RollSucceeds(Scheme.DetectionRisk * SimDeltaTime, FMath::FRand())) {
        OnSchemeDiscovered(Scheme);
        return;
    }

    if (Scheme.Progress >= 1.0f) {
        ExecuteScheme(Scheme);
    }
}

static EMythicFactionRelation WorsenRelation(EMythicFactionRelation R) {
    const uint8 Cur = static_cast<uint8>(R);
    const uint8 Worst = static_cast<uint8>(EMythicFactionRelation::Hostile);
    return static_cast<EMythicFactionRelation>(Cur < Worst ? Cur + 1 : Worst);
}

void UMythicSchemeEngine::ApplySchemeEffects(const FMythicScheme &Scheme) {
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
        return;
    }

    switch (Scheme.Type) {
    case EMythicSchemeType::TradeDisruption: {
        const float F = Settings->SchemeTradeDisruptionFraction;
        Target->Reserves.Wealth -= FMath::Max(0.0f, Target->Reserves.Wealth) * F;
        Target->Reserves.Materials -= FMath::Max(0.0f, Target->Reserves.Materials) * F;
        break;
    }
    case EMythicSchemeType::MilitaryRaid: {
        Target->Reserves.Arms = FMath::Max(0.0f, Target->Reserves.Arms - Settings->SchemeRaidArmsLoss);
        Target->Population = FMath::Max(0, Target->Population -
                                        FMath::RoundToInt(Target->Population * Settings->SchemeRaidPopulationLossFraction));
        break;
    }
    case EMythicSchemeType::DiplomaticPressure: {
        FactionDB->SetRelationship(Scheme.OriginFaction, Scheme.TargetFaction,
                                   WorsenRelation(FactionDB->GetWriteRelationship(Scheme.OriginFaction, Scheme.TargetFaction)));
        break;
    }
    case EMythicSchemeType::Assassination: {
        Target->LeaderEntityId = 0;
        Target->LeaderSignificanceScore = 0.0f;
        Target->Population = FMath::Max(0, Target->Population - Settings->SchemeAssassinationPopulationLoss);
        break;
    }
    case EMythicSchemeType::TerritoryReclaim: {
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

    ApplySchemeEffects(Scheme);

    if (Fabric) {
        FMythicWorldEvent Event;
        Event.PrimaryFaction = Scheme.OriginFaction;
        Event.SecondaryFaction = Scheme.TargetFaction;
        Event.Cell = Scheme.TargetCell;
        Event.Significance = 0.8f;
        Event.CategoryFlags = EMythicEventCategory::Scheme;
        Event.EventTag = TAG_LIVINGWORLD_EVENT_SCHEME_COMPLETED;

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

    if (FactionDB && Scheme.OriginFaction.IsValid() && Scheme.TargetFaction.IsValid()) {
        FactionDB->SetRelationship(Scheme.OriginFaction, Scheme.TargetFaction,
                                   WorsenRelation(FactionDB->GetWriteRelationship(Scheme.OriginFaction, Scheme.TargetFaction)));
    }

    if (Fabric) {
        FMythicWorldEvent Event;
        Event.PrimaryFaction = Scheme.TargetFaction;
        Event.SecondaryFaction = Scheme.OriginFaction;
        Event.Cell = Scheme.TargetCell;
        Event.Significance = 0.6f;
        Event.CategoryFlags = EMythicEventCategory::Scheme | EMythicEventCategory::Diplomacy;
        Event.EventTag = TAG_LIVINGWORLD_EVENT_SCHEME_DISCOVERED;

        Fabric->AppendEvent(Event);
    }

    UE_LOG(LogMythScheme, Log,
           TEXT("Scheme %d discovered! Faction %d was plotting against %d, Type=%d"),
           Scheme.SchemeId,
           Scheme.OriginFaction.Index,
           Scheme.TargetFaction.Index,
           static_cast<int32>(Scheme.Type));
}


TArray<FMythicScheme> UMythicSchemeEngine::GetActiveSchemes() const {
    FScopeLock Lock(&SchemeLock);
    return ActiveSchemes;
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
