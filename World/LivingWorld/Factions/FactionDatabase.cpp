// Mythic Living World System — Faction Database Implementation

#include "World/LivingWorld/Factions/FactionDatabase.h"

void UMythicFactionDatabase::Initialize(const UMythicFactionDatabaseSettings *Settings) {
    check(Settings);


    MaxFactions = Settings->MaxFactions;
    WriteFactions.SetNum(MaxFactions);
    ReadFactions.SetNum(MaxFactions);

    const int32 RelationCount = MaxFactions * MaxFactions;
    WriteRelationships.SetNum(RelationCount);
    ReadRelationships.SetNum(RelationCount);

    // Default all relationships to Neutral
    for (int32 i = 0; i < RelationCount; ++i) {
        WriteRelationships[i] = EMythicFactionRelation::Neutral;
        ReadRelationships[i] = EMythicFactionRelation::Neutral;
    }

    // Load initial factions
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

    // Initial commit so game thread has valid data
    CommitWrites();

    UE_LOG(LogMythFaction, Log, TEXT("Faction Database initialized: %d factions loaded, max capacity %d"),
           RegisteredCount, MaxFactions);
}

void UMythicFactionDatabase::BeginDestroy() {
    Super::BeginDestroy();

    // Explicitly empty arrays to ensure FText/struct destruction happens 
    // before the UObject memory is reclaimed
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

    // Symmetric relationship
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

    // Default new faction relationships to Neutral
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

void UMythicFactionDatabase::AnnihilateFaction(FMythicFactionId Id) {
    FMythicFactionData *Faction = GetFactionMutable(Id);
    if (!Faction) {
        return;
    }

    Faction->bAlive = false;
    Faction->Population = 0;
    Faction->MilitaryStrength = 0.0f;
    Faction->ControlledCellCount = 0;
    Faction->LeaderEntityId = 0;
    Faction->Supply = FMythicResourceStock();
    Faction->Demand = FMythicResourceStock();
    Faction->Reserves = FMythicResourceStock();
    Faction->Prices = FMythicResourceStock();

    UE_LOG(LogMythFaction, Log, TEXT("Faction '%s' (index %d) annihilated"), *Faction->DisplayName.ToString(), Id.Index);
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

// ─── Commit Snapshot ──────────────────────────────────
void UMythicFactionDatabase::CommitWrites() {
    FScopeLock Lock(&SnapshotLock);

    // SAFETY: Use TArray assignment, NOT Memcpy. 
    // FMythicFactionData contains FText which requires copy construction to manage ref-counts.
    // Memcpy bypasses this, leading to double-free crashes.
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
