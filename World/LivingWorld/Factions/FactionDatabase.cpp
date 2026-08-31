
#include "World/LivingWorld/Factions/FactionDatabase.h"

namespace {
bool SerializeFactionEntityId(FArchive &Ar, FMythicEntityId &EntityId) {
    uint8 Domain = static_cast<uint8>(EntityId.GetDomain());
    FGuid Guid = EntityId.GetAuthorityGuid();
    Ar << Domain;
    Ar << Guid.A;
    Ar << Guid.B;
    Ar << Guid.C;
    Ar << Guid.D;
    if (Ar.IsLoading()) {
        const bool bEmpty = Domain == static_cast<uint8>(
                                EMythicEntityDomain::Invalid)
                            && !Guid.IsValid();
        const bool bLivingWorldIdentity =
            Domain == static_cast<uint8>(EMythicEntityDomain::LivingWorld)
            && Guid.IsValid();
        if (!bEmpty && !bLivingWorldIdentity) {
            EntityId.Reset();
            return false;
        }
        EntityId = bLivingWorldIdentity
            ? FMythicEntityId::FromAuthorityGuid(
                  EMythicEntityDomain::LivingWorld, Guid)
            : FMythicEntityId();
    }
    return !EntityId.IsValid()
           || EntityId.GetDomain() == EMythicEntityDomain::LivingWorld;
}
}

void UMythicFactionDatabase::Initialize(const UMythicFactionDatabaseSettings *Settings) {
    check(Settings);


    MaxFactions = Settings->MaxFactions;
    RelationStandingBaselines = Settings->RelationStandingBaselines;
    WriteFactions.SetNum(MaxFactions);
    ReadFactions.SetNum(MaxFactions);

    const int32 RelationCount = MaxFactions * MaxFactions;
    WriteRelationships.SetNum(RelationCount);
    ReadRelationships.SetNum(RelationCount);

    for (int32 i = 0; i < RelationCount; ++i) {
        WriteRelationships[i] = EMythicFactionRelation::Neutral;
        ReadRelationships[i] = EMythicFactionRelation::Neutral;
    }

    RegisteredCount = 0;
    for (int32 i = 0; i < Settings->InitialFactions.Num(); ++i) {
        if (RegisteredCount >= MaxFactions) {
            UE_LOG(LogMythFaction, Warning, TEXT("Max faction capacity reached during initialization. Remaining factions skipped."));
            break;
        }

        const FMythicFactionData &InitialFaction = Settings->InitialFactions[i];

        WriteFactions[RegisteredCount] = InitialFaction;
        ++RegisteredCount;
    }

    CommitWrites();

    UE_LOG(LogMythFaction, Log, TEXT("Faction Database initialized: %d factions loaded, max capacity %d"),
           RegisteredCount.load(), MaxFactions);
}

void UMythicFactionDatabase::BeginDestroy() {
    Super::BeginDestroy();

    WriteFactions.Empty();
    ReadFactions.Empty();
    WriteRelationships.Empty();
    ReadRelationships.Empty();
}

bool UMythicFactionDatabase::GetFaction(FMythicFactionId Id, FMythicFactionData &OutData) const {
    if (!Id.IsValid() || Id.Index >= RegisteredCount) {
        return false;
    }

    FScopeLock Lock(&SnapshotLock);
    OutData = ReadFactions[Id.Index];
    return true;
}

bool UMythicFactionDatabase::GetFactionMoralProfile(FMythicFactionId Id, FMythicFactionMoralProfile &Out) const {
    if (!Id.IsValid() || Id.Index >= RegisteredCount) {
        return false;
    }
    FScopeLock Lock(&SnapshotLock);
    const FMythicFactionData &Data = ReadFactions[Id.Index];
    Out.Ideology = Data.Ideology;
    Out.DisapproveThreshold = Data.DisapproveThreshold;
    Out.CondemnThreshold = Data.CondemnThreshold;
    Out.HostileThreshold = Data.HostileThreshold;
    return true;
}

FMythicFactionData *UMythicFactionDatabase::GetFactionMutable(FMythicFactionId Id) {
    if (!Id.IsValid() || Id.Index >= RegisteredCount) {
        return nullptr;
    }
    return &WriteFactions[Id.Index];
}

void UMythicFactionDatabase::SetRelationship(FMythicFactionId A, FMythicFactionId B, EMythicFactionRelation Relation) {
    if (!A.IsValid() || !B.IsValid() || A.Index >= RegisteredCount || B.Index >= RegisteredCount) {
        return;
    }

    WriteRelationships[RelationIndex(A, B)] = Relation;
    WriteRelationships[RelationIndex(B, A)] = Relation;
}

FMythicFactionId UMythicFactionDatabase::RegisterFaction(const FMythicFactionData &Data) {
    if (RegisteredCount >= MaxFactions) {
        UE_LOG(LogMythFaction, Warning, TEXT("Cannot register faction '%s' — max capacity %d reached"),
               *Data.DisplayName.ToString(), MaxFactions);
        return FMythicFactionId();
    }

    const int32 NewIndex = RegisteredCount;
    WriteFactions[NewIndex] = Data;
    WriteFactions[NewIndex].bHasBeenPopulated = (Data.Population > 0);
    ++RegisteredCount;

    FMythicFactionId NewId;
    NewId.Index = static_cast<uint8>(NewIndex);

    for (int32 i = 0; i < RegisteredCount - 1; ++i) {
        FMythicFactionId OtherId;
        OtherId.Index = static_cast<uint8>(i);
        SetRelationship(NewId, OtherId, EMythicFactionRelation::Neutral);
    }

    UE_LOG(LogMythFaction, Log, TEXT("Registered new faction '%s' at index %d"), *Data.DisplayName.ToString(), NewIndex);
    return NewId;
}

FMythicFactionId UMythicFactionDatabase::CreateFactionFromConquest(FMythicFactionId OriginalFaction, int32 SurvivorCount) {
    if (!OriginalFaction.IsValid() || OriginalFaction.Index >= RegisteredCount) {
        return FMythicFactionId();
    }

    if (RegisteredCount >= MaxFactions) {
        return FMythicFactionId();
    }

    const FMythicFactionData &Original = WriteFactions[OriginalFaction.Index];

    FMythicFactionData Resistance;
    Resistance.DisplayName = FText::Format(NSLOCTEXT("LivingWorld", "ResistanceFmt", "{0} Resistance"), Original.DisplayName);

    FString NewTagStr = Original.FactionTag.ToString() + TEXT(".Resistance");
    Resistance.FactionTag = FGameplayTag::RequestGameplayTag(FName(*NewTagStr), false);

    Resistance.bAlive = true;
    Resistance.Status = EMythicFactionStatus::Resistance;
    Resistance.Population = SurvivorCount;
    Resistance.Ideology = Original.Ideology;

    Resistance.ControlledCellCount = 0;
    Resistance.MilitaryStrength = 0.1f;

    return RegisterFaction(Resistance);
}

void UMythicFactionDatabase::AnnihilateFaction(FMythicFactionId Id) {
    FMythicFactionData *Faction = GetFactionMutable(Id);
    if (!Faction) {
        return;
    }

    Faction->bAlive = false;
    Faction->Status = EMythicFactionStatus::Annihilated;
    Faction->LastAlivePopulation = Faction->Population;
    Faction->Population = 0;
    Faction->MilitaryStrength = 0.0f;
    Faction->ControlledCellCount = 0;
    Faction->LeaderEntityId.Reset();
    Faction->Supply = FMythicResourceStock();
    Faction->Demand = FMythicResourceStock();
    Faction->Reserves = FMythicResourceStock();
    Faction->Prices = FMythicResourceStock();

    UE_LOG(LogMythFaction, Log, TEXT("Faction '%s' (index %d) annihilated"), *Faction->DisplayName.ToString(), Id.Index);
}

void UMythicFactionDatabase::RestoreResistanceToFaction(FMythicFactionId Id) {
    FMythicFactionData *Faction = GetFactionMutable(Id);
    if (!Faction || Faction->Status != EMythicFactionStatus::Resistance) {
        return;
    }

    Faction->Status = EMythicFactionStatus::Active;

    UE_LOG(LogMythFaction, Log, TEXT("Resistance '%s' (index %d) restored to a full faction (cells=%d, pop=%d)"),
           *Faction->DisplayName.ToString(), Id.Index, Faction->ControlledCellCount, Faction->Population);
}

FMythicFactionData *UMythicFactionDatabase::GetFactionMutableByIndex(int32 Index) {
    if (Index < 0 || Index >= RegisteredCount) {
        return nullptr;
    }
    return &WriteFactions[Index];
}

EMythicFactionRelation UMythicFactionDatabase::GetWriteRelationship(FMythicFactionId A, FMythicFactionId B) const {
    if (!A.IsValid() || !B.IsValid() || A.Index >= RegisteredCount || B.Index >= RegisteredCount) {
        return EMythicFactionRelation::Neutral;
    }
    if (A == B) {
        return EMythicFactionRelation::Allied;
    }
    return WriteRelationships[RelationIndex(A, B)];
}

void UMythicFactionDatabase::ForEachAliveFactionMutable(TFunctionRef<void(FMythicFactionId, FMythicFactionData &)> Callback) {
    for (int32 i = 0; i < RegisteredCount; ++i) {
        if (WriteFactions[i].bAlive) {
            FMythicFactionId Id;
            Id.Index = static_cast<uint8>(i);
            Callback(Id, WriteFactions[i]);
        }
    }
}

void UMythicFactionDatabase::CommitWrites() {
    FScopeLock Lock(&SnapshotLock);

    ReadFactions = WriteFactions;
    ReadRelationships = WriteRelationships;
}


bool UMythicFactionDatabase::FindFactionByTag(const FGameplayTag &Tag, FMythicFactionData &OutData, FMythicFactionId *OutId) const {
    FScopeLock Lock(&SnapshotLock);

    for (int32 i = 0; i < RegisteredCount; ++i) {
        if (ReadFactions[i].FactionTag == Tag) {
            OutData = ReadFactions[i];
            if (OutId) {
                OutId->Index = static_cast<uint8>(i);
            }
            return true;
        }
    }
    return false;
}

FMythicFactionId UMythicFactionDatabase::FindFactionId(const FGameplayTag &Tag) const {
    FScopeLock Lock(&SnapshotLock);

    for (int32 i = 0; i < RegisteredCount; ++i) {
        if (ReadFactions[i].FactionTag == Tag) {
            FMythicFactionId Id;
            Id.Index = static_cast<uint8>(i);
            return Id;
        }
    }
    return FMythicFactionId();
}

float UMythicFactionDatabase::GetRelationStandingBaseline(const EMythicFactionRelation Relation) const {
    if (const float *Baseline = RelationStandingBaselines.Find(Relation)) {
        return *Baseline;
    }
    return 0.0f;
}

ETeamAttitude::Type UMythicFactionDatabase::BandStanding(const float Stance, const float HostileThreshold, const float FriendlyThreshold) {
    if (Stance <= HostileThreshold) {
        return ETeamAttitude::Hostile;
    }
    if (Stance >= FriendlyThreshold) {
        return ETeamAttitude::Friendly;
    }
    return ETeamAttitude::Neutral;
}

EMythicFactionRelation UMythicFactionDatabase::GetRelationship(FMythicFactionId A, FMythicFactionId B) const {
    if (!A.IsValid() || !B.IsValid() || A.Index >= RegisteredCount || B.Index >= RegisteredCount) {
        return EMythicFactionRelation::Neutral;
    }

    if (A == B) {
        return EMythicFactionRelation::Allied;
    }

    FScopeLock Lock(&SnapshotLock);
    return ReadRelationships[RelationIndex(A, B)];
}

int32 UMythicFactionDatabase::GetActiveFactionCount() const {
    FScopeLock Lock(&SnapshotLock);
    int32 Count = 0;
    for (int32 i = 0; i < RegisteredCount; ++i) {
        if (ReadFactions[i].bAlive) {
            ++Count;
        }
    }
    return Count;
}

void UMythicFactionDatabase::ForEachAliveFaction(TFunctionRef<void(FMythicFactionId, const FMythicFactionData &)> Callback) const {
    FScopeLock Lock(&SnapshotLock);
    for (int32 i = 0; i < RegisteredCount; ++i) {
        if (ReadFactions[i].bAlive) {
            FMythicFactionId Id;
            Id.Index = static_cast<uint8>(i);
            Callback(Id, ReadFactions[i]);
        }
    }
}

void UMythicFactionDatabase::ReportLeaderCandidate(
    FMythicFactionId FactionId, const FMythicEntityId &EntityId, float Score) {
    if (!FactionId.IsValid() || FactionId.Index >= RegisteredCount
        || !EntityId.IsValid()
        || EntityId.GetDomain() != EMythicEntityDomain::LivingWorld) {
        return;
    }

    FMythicFactionData &Faction = WriteFactions[FactionId.Index];

    if (Score > Faction.LeaderSignificanceScore) {
        const FMythicEntityId PreviousLeader = Faction.LeaderEntityId;
        Faction.LeaderEntityId = EntityId;
        Faction.LeaderSignificanceScore = Score;

        if (PreviousLeader != EntityId) {
            UE_LOG(LogMythFaction, Log,
                   TEXT("Faction '%s': new leader nominated (%s, score=%.2f, prev=%s)"),
                   *Faction.DisplayName.ToString(), *EntityId.ToDebugString(), Score,
                   *PreviousLeader.ToDebugString());
        }
    }
}

bool UMythicFactionDatabase::ReferencesEntityIdentity(
    const FMythicEntityId &EntityId) const {
    if (!EntityId.IsValid()) {
        return false;
    }
    const int32 Count = FMath::Min(RegisteredCount.load(), WriteFactions.Num());
    for (int32 Index = 0; Index < Count; ++Index) {
        const FMythicFactionData &Faction = WriteFactions[Index];
        if (Faction.bAlive && Faction.LeaderEntityId == EntityId) {
            return true;
        }
    }
    return false;
}

bool UMythicFactionDatabase::HandlePermanentEntityDeath(
    const FMythicEntityId &EntityId) {
    return ClearLeaderReference(EntityId);
}

bool UMythicFactionDatabase::ClearLeaderReference(
    const FMythicEntityId &EntityId) {
    if (!EntityId.IsValid()) {
        return false;
    }
    bool bCleared = false;
    const int32 Count = FMath::Min(RegisteredCount.load(), WriteFactions.Num());
    for (int32 Index = 0; Index < Count; ++Index) {
        FMythicFactionData &Faction = WriteFactions[Index];
        if (Faction.LeaderEntityId == EntityId) {
            Faction.LeaderEntityId.Reset();
            Faction.LeaderSignificanceScore = 0.0f;
            bCleared = true;
        }
    }
    return bCleared;
}

int32 UMythicFactionDatabase::ClearUnrestorableLeaderReferences(
    const TSet<FMythicEntityId> &RestorableEntityIds) {
    int32 ClearedCount = 0;
    const int32 Count = FMath::Min(RegisteredCount.load(), WriteFactions.Num());
    for (int32 Index = 0; Index < Count; ++Index) {
        FMythicFactionData &Faction = WriteFactions[Index];
        if (!Faction.LeaderEntityId.IsValid()
            || RestorableEntityIds.Contains(Faction.LeaderEntityId)) {
            continue;
        }

        UE_LOG(LogMythFaction, Warning,
               TEXT("Faction '%s': cleared unrestorable leader %s during LivingWorld restore; succession may resume."),
               *Faction.DisplayName.ToString(),
               *Faction.LeaderEntityId.ToDebugString());
        Faction.LeaderEntityId.Reset();
        Faction.LeaderSignificanceScore = 0.0f;
        ++ClearedCount;
    }
    return ClearedCount;
}

void UMythicFactionDatabase::Serialize(FArchive &Ar) {
    constexpr int32 CurrentVersion = 5;
    int32 Version = CurrentVersion;
    Ar << Version;
    if (Ar.IsLoading() && Version != CurrentVersion) {
        Ar.SetError();
        return;
    }

    Ar << MaxFactions;
    int32 RegCountTmp = RegisteredCount.load();
    Ar << RegCountTmp;
    RegisteredCount.store(RegCountTmp);

    if (Ar.IsLoading()) {
        if (MaxFactions < 0 || MaxFactions > 1024 || RegCountTmp < 0 || RegCountTmp > MaxFactions) {
            Ar.SetError();
            return;
        }
        WriteFactions.SetNum(MaxFactions);
        ReadFactions.SetNum(MaxFactions);
        const int32 RelationCount = MaxFactions * MaxFactions;
        WriteRelationships.SetNum(RelationCount);
        ReadRelationships.SetNum(RelationCount);
    }

    for (int32 i = 0; i < RegisteredCount; ++i) {
        FMythicFactionData &F = WriteFactions[i];

        Ar << F.DisplayName;
        Ar << F.FactionTag;
        Ar << F.bAlive;

        uint8 StatusVal = static_cast<uint8>(F.Status);
        Ar << StatusVal;
        if (Ar.IsLoading()) {
            F.Status = static_cast<EMythicFactionStatus>(StatusVal);
        }

        Ar << F.bHasBeenPopulated;
        Ar << F.bIdeologyDirty;

        if (Version >= 2) {
            Ar << F.bControlsTerritory;
            Ar << F.bHasEconomy;
            Ar << F.bHasCivilianPopulation;
            Ar << F.bParticipatesInTrade;
            Ar << F.bCanNegotiate;
        }

        Ar << F.Population;
        Ar << F.MilitaryStrength;
        Ar << F.ControlledCellCount;
        if (!SerializeFactionEntityId(Ar, F.LeaderEntityId)) {
            Ar.SetError();
            return;
        }
        Ar << F.LeaderSignificanceScore;
        if (Ar.IsLoading() && !F.LeaderEntityId.IsValid()) {
            F.LeaderSignificanceScore = 0.0f;
        }

        Ar << F.Ideology.Violence;
        Ar << F.Ideology.Theft;
        Ar << F.Ideology.Deception;
        Ar << F.Ideology.Mercy;
        Ar << F.Ideology.Loyalty;
        Ar << F.Ideology.Sanctity;
        Ar << F.Ideology.Authority;
        Ar << F.Ideology.Arcane;

        Ar << F.Supply.Food << F.Supply.Materials << F.Supply.Arms << F.Supply.Wealth;
        Ar << F.Demand.Food << F.Demand.Materials << F.Demand.Arms << F.Demand.Wealth;
        Ar << F.Reserves.Food << F.Reserves.Materials << F.Reserves.Arms << F.Reserves.Wealth;
        Ar << F.Prices.Food << F.Prices.Materials << F.Prices.Arms << F.Prices.Wealth;

        if (Version >= 3) {
            Ar << F.BaseProduction.Food << F.BaseProduction.Materials << F.BaseProduction.Arms << F.BaseProduction.Wealth;
            Ar << F.DisapproveThreshold << F.CondemnThreshold << F.HostileThreshold;
        }

        if (Version >= 4) {
            Ar << F.bOverrideFactionColor;
            Ar << F.FactionColor;
        }
    }

    const int32 RelationCount = MaxFactions * MaxFactions;
    for (int32 i = 0; i < RelationCount; ++i) {
        uint8 RelVal = static_cast<uint8>(WriteRelationships[i]);
        Ar << RelVal;
        if (Ar.IsLoading()) {
            WriteRelationships[i] = static_cast<EMythicFactionRelation>(RelVal);
        }
    }

    if (Ar.IsLoading()) {
        CommitWrites();
    }
}
