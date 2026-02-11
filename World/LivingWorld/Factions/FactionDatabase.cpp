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
    for (const FMythicFactionData &InitialFaction : Settings->InitialFactions) {
        if (RegisteredCount >= MaxFactions) {
            UE_LOG(LogMythFaction, Warning, TEXT("Max faction capacity reached during initialization. Remaining factions skipped."));
            break;
        }

        WriteFactions[RegisteredCount] = InitialFaction;
        ++RegisteredCount;
    }

    // Initial commit so game thread has valid data
    CommitWrites();

    UE_LOG(LogMythFaction, Log, TEXT("Faction Database initialized: %d factions loaded, max capacity %d"),
           RegisteredCount, MaxFactions);
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
    Faction->EconomicStrength = 0.0f;
    Faction->MilitaryStrength = 0.0f;
    Faction->ControlledCellCount = 0;
    Faction->LeaderEntityId = 0;

    UE_LOG(LogMythFaction, Log, TEXT("Faction '%s' (index %d) annihilated"), *Faction->DisplayName.ToString(), Id.Index);
}

void UMythicFactionDatabase::CommitWrites() {
    FMemory::Memcpy(ReadFactions.GetData(), WriteFactions.GetData(), MaxFactions * sizeof(FMythicFactionData));
    FMemory::Memcpy(ReadRelationships.GetData(), WriteRelationships.GetData(), WriteRelationships.Num() * sizeof(EMythicFactionRelation));
}

const FMythicFactionData *UMythicFactionDatabase::GetFaction(FMythicFactionId Id) const {
    if (!Id.IsValid() || Id.Index >= RegisteredCount) {
        return nullptr;
    }
    return &ReadFactions[Id.Index];
}

const FMythicFactionData *UMythicFactionDatabase::FindFactionByTag(const FGameplayTag &Tag, FMythicFactionId *OutId) const {
    for (int32 i = 0; i < RegisteredCount; ++i) {
        if (ReadFactions[i].FactionTag == Tag) {
            if (OutId) {
                OutId->Index = static_cast<uint8>(i);
            }
            return &ReadFactions[i];
        }
    }
    return nullptr;
}

EMythicFactionRelation UMythicFactionDatabase::GetRelationship(FMythicFactionId A, FMythicFactionId B) const {
    if (!A.IsValid() || !B.IsValid() || A.Index >= RegisteredCount || B.Index >= RegisteredCount) {
        return EMythicFactionRelation::Neutral;
    }

    if (A == B) {
        return EMythicFactionRelation::Allied;
    }

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
    for (int32 i = 0; i < RegisteredCount; ++i) {
        if (ReadFactions[i].bAlive) {
            FMythicFactionId Id;
            Id.Index = static_cast<uint8>(i);
            Callback(Id, ReadFactions[i]);
        }
    }
}
