
#include "World/LivingWorld/Territory/TerritoryGrid.h"

void UMythicTerritoryGrid::Initialize(const UMythicTerritoryGridSettings *Settings) {
    check(Settings);

    Width = Settings->GridWidth;
    Height = Settings->GridHeight;
    CellWorldSize = Settings->CellWorldSize;
    WorldOrigin = Settings->WorldOrigin;
    InfluenceBleedRate = Settings->InfluenceBleedRate;
    MinControlThreshold = Settings->MinControlThreshold;

    BiomeWorldSeed = Settings->BiomeWorldSeed;
    BiomeNoiseFrequency = Settings->BiomeNoiseFrequency;
    BiomeThresholds = Settings->BiomeThresholds;

    const int32 TotalCells = Width * Height;
    WriteBuffer.SetNum(TotalCells);
    ReadBuffer.SetNum(TotalCells);
    DirtyCells.Init(false, TotalCells);
    ReadDirtyCells.Init(false, TotalCells);

    WriteFactionCells.SetNum(256);
    ReadFactionCells.SetNum(256);

    UE_LOG(LogMythTerritory, Log, TEXT("Territory Grid initialized: %dx%d (%d cells), cell size %.0fcm"),
           Width, Height, TotalCells, CellWorldSize);
}

void UMythicTerritoryGrid::SetCellInfluence(const FMythicCellCoord &Coord, FMythicFactionId Faction, float Influence) {
    if (!IsValidCoord(Coord)) {
        return;
    }

    const int32 Index = CoordToIndex(Coord);
    FMythicTerritoryCell &Cell = WriteBuffer[Index];
    Cell.DominantFaction = Faction;
    Cell.Influence = FMath::Clamp(Influence, 0.0f, 1.0f);
    DirtyCells[Index] = true;
}

void UMythicTerritoryGrid::SetCellPlayerOwned(const FMythicCellCoord &Coord, bool bOwned, uint8 PlayerIndex) {
    if (!IsValidCoord(Coord)) {
        return;
    }

    const int32 Index = CoordToIndex(Coord);
    FMythicTerritoryCell &Cell = WriteBuffer[Index];
    Cell.bPlayerOwned = bOwned;
    Cell.OwningPlayerIndex = PlayerIndex;
    DirtyCells[Index] = true;
}

void UMythicTerritoryGrid::PropagateInfluence() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicTerritoryGrid_PropagateInfluence);


    struct FCellUpdate {
        FMythicFactionId Faction;
        float Influence;
    };

    const int32 TotalCells = Width * Height;
    TArray<FCellUpdate> Updates;
    Updates.SetNumUninitialized(TotalCells);

    for (int32 Y = 0; Y < Height; ++Y) {
        for (int32 X = 0; X < Width; ++X) {
            const int32 Index = Y * Width + X;
            const FMythicTerritoryCell &Cell = WriteBuffer[Index];

            struct FFactionBleed {
                FMythicFactionId Faction;
                float TotalInfluence;
            };
            FFactionBleed FactionBleeds[4];
            int32 BleedCount = 0;

            const int32 Neighbors[4][2] = {{X - 1, Y}, {X + 1, Y}, {X, Y - 1}, {X, Y + 1}};

            for (const auto &N : Neighbors) {
                if (N[0] < 0 || N[0] >= Width || N[1] < 0 || N[1] >= Height) {
                    continue;
                }

                const int32 NIdx = N[1] * Width + N[0];
                const FMythicTerritoryCell &Neighbor = WriteBuffer[NIdx];

                if (!Neighbor.DominantFaction.IsValid() || Neighbor.Influence <= 0.0f) {
                    continue;
                }

                bool bFound = false;
                for (int32 b = 0; b < BleedCount; ++b) {
                    if (FactionBleeds[b].Faction == Neighbor.DominantFaction) {
                        FactionBleeds[b].TotalInfluence += Neighbor.Influence * InfluenceBleedRate;
                        bFound = true;
                        break;
                    }
                }
                if (!bFound && BleedCount < 4) {
                    FactionBleeds[BleedCount].Faction = Neighbor.DominantFaction;
                    FactionBleeds[BleedCount].TotalInfluence = Neighbor.Influence * InfluenceBleedRate;
                    ++BleedCount;
                }
            }


            if (!Cell.DominantFaction.IsValid()) {
                float BestInfluence = 0.0f;
                FMythicFactionId BestFaction;
                for (int32 b = 0; b < BleedCount; ++b) {
                    if (FactionBleeds[b].TotalInfluence > BestInfluence) {
                        BestInfluence = FactionBleeds[b].TotalInfluence;
                        BestFaction = FactionBleeds[b].Faction;
                    }
                }

                Updates[Index].Faction = BestFaction;
                Updates[Index].Influence = BestInfluence;
            }
            else {
                float Reinforcement = 0.0f;
                float Erosion = 0.0f;

                for (int32 b = 0; b < BleedCount; ++b) {
                    if (FactionBleeds[b].Faction == Cell.DominantFaction) {
                        Reinforcement += FactionBleeds[b].TotalInfluence;
                    }
                    else {
                        Erosion += FactionBleeds[b].TotalInfluence;
                    }
                }

                float NewInfluence = Cell.Influence + Reinforcement - Erosion;
                Updates[Index].Influence = NewInfluence;
                Updates[Index].Faction = Cell.DominantFaction;

                if (NewInfluence <= 0.0f) {
                    float BestAttacker = 0.0f;
                    FMythicFactionId BestFaction;
                    for (int32 b = 0; b < BleedCount; ++b) {
                        if (FactionBleeds[b].Faction != Cell.DominantFaction) {
                            if (FactionBleeds[b].TotalInfluence > BestAttacker) {
                                BestAttacker = FactionBleeds[b].TotalInfluence;
                                BestFaction = FactionBleeds[b].Faction;
                            }
                        }
                    }
                    Updates[Index].Faction = BestFaction;
                    Updates[Index].Influence = BestAttacker;
                }
            }
        }
    }

    for (int32 i = 0; i < TotalCells; ++i) {
        const float ClampedInfluence = FMath::Clamp(Updates[i].Influence, 0.0f, 1.0f);
        const bool bAboveThreshold = ClampedInfluence >= MinControlThreshold && Updates[i].Faction.IsValid();

        const FMythicFactionId NewFaction = bAboveThreshold ? Updates[i].Faction : FMythicFactionId();
        const float NewInfluence = bAboveThreshold ? ClampedInfluence : 0.0f;

        if (WriteBuffer[i].DominantFaction != NewFaction || WriteBuffer[i].Influence != NewInfluence) {
            WriteBuffer[i].DominantFaction = NewFaction;
            WriteBuffer[i].Influence = NewInfluence;
            DirtyCells[i] = true;
        }
    }
}

void UMythicTerritoryGrid::GetWriteCellCounts(TArray<int32> &OutCountsByFactionIndex) const {
    OutCountsByFactionIndex.Reset();
    OutCountsByFactionIndex.AddZeroed(256);

    const int32 TotalCells = Width * Height;
    for (int32 i = 0; i < TotalCells; ++i) {
        const FMythicFactionId Faction = WriteBuffer[i].DominantFaction;
        if (Faction.IsValid()) {
            ++OutCountsByFactionIndex[Faction.Index];
        }
    }
}

void UMythicTerritoryGrid::CommitWrites() {
    for (int32 i = 0; i < 256; ++i) {
        WriteFactionCells[i].Reset();
    }

    const int32 TotalCells = Width * Height;
    for (int32 i = 0; i < TotalCells; ++i) {
        const FMythicFactionId Faction = WriteBuffer[i].DominantFaction;
        if (Faction.IsValid()) {
            WriteFactionCells[Faction.Index].Add(FMythicCellCoord(i % Width, i / Width));
        }
    }

    FScopeLock Lock(&SnapshotLock);
    FMemory::Memcpy(ReadBuffer.GetData(), WriteBuffer.GetData(), WriteBuffer.Num() * sizeof(FMythicTerritoryCell));
    ReadFactionCells = WriteFactionCells;

    for (TConstSetBitIterator<> It(DirtyCells); It; ++It) {
        ReadDirtyCells[It.GetIndex()] = true;
    }
    DirtyCells.Init(false, Width * Height);
}

FMythicTerritoryCell UMythicTerritoryGrid::GetCell(const FMythicCellCoord &Coord) const {
    if (!IsValidCoord(Coord)) {
        return FMythicTerritoryCell();
    }
    FScopeLock Lock(&SnapshotLock);
    return ReadBuffer[CoordToIndex(Coord)];
}

FMythicFactionId UMythicTerritoryGrid::GetDominantFaction(const FMythicCellCoord &Coord) const {
    if (!IsValidCoord(Coord)) {
        return FMythicFactionId();
    }
    FScopeLock Lock(&SnapshotLock);
    return ReadBuffer[CoordToIndex(Coord)].DominantFaction;
}

FMythicCellCoord UMythicTerritoryGrid::WorldToCell(const FVector &WorldPosition) const {
    const float LocalX = static_cast<float>(WorldPosition.X) - static_cast<float>(WorldOrigin.X);
    const float LocalY = static_cast<float>(WorldPosition.Y) - static_cast<float>(WorldOrigin.Y);

    const int32 CellX = FMath::Clamp(FMath::FloorToInt(LocalX / CellWorldSize), 0, Width - 1);
    const int32 CellY = FMath::Clamp(FMath::FloorToInt(LocalY / CellWorldSize), 0, Height - 1);

    return FMythicCellCoord(CellX, CellY);
}

FVector UMythicTerritoryGrid::CellToWorld(const FMythicCellCoord &Coord) const {
    const float WorldX = static_cast<float>(WorldOrigin.X) + (static_cast<float>(Coord.X) + 0.5f) * CellWorldSize;
    const float WorldY = static_cast<float>(WorldOrigin.Y) + (static_cast<float>(Coord.Y) + 0.5f) * CellWorldSize;

    return FVector(WorldX, WorldY, 0.0f);
}

bool UMythicTerritoryGrid::IsValidCoord(const FMythicCellCoord &Coord) const {
    return Coord.X >= 0 && Coord.X < Width && Coord.Y >= 0 && Coord.Y < Height;
}


uint32 UMythicTerritoryGrid::BiomeHash2D(int32 X, int32 Y, uint32 Seed) {
    uint32 H = Seed;
    H = HashCombine(H, ::GetTypeHash(X));
    H = HashCombine(H, ::GetTypeHash(Y));
    H = (H ^ 61u) ^ (H >> 16u);
    H *= 9u;
    H = H ^ (H >> 4u);
    H *= 0x27d4eb2du;
    H = H ^ (H >> 15u);
    return H;
}

float UMythicTerritoryGrid::BiomeValueNoise(int32 X, int32 Y, uint32 Seed, float Frequency) {
    const float SampleX = static_cast<float>(X) * Frequency;
    const float SampleY = static_cast<float>(Y) * Frequency;

    const int32 X0 = FMath::FloorToInt(SampleX);
    const int32 Y0 = FMath::FloorToInt(SampleY);
    const int32 X1 = X0 + 1;
    const int32 Y1 = Y0 + 1;

    const float Fx = SampleX - static_cast<float>(X0);
    const float Fy = SampleY - static_cast<float>(Y0);

    const float Wx = Fx * Fx * (3.0f - 2.0f * Fx);
    const float Wy = Fy * Fy * (3.0f - 2.0f * Fy);

    constexpr float InvU32 = 1.0f / 4294967295.0f;
    const float V00 = static_cast<float>(BiomeHash2D(X0, Y0, Seed)) * InvU32;
    const float V10 = static_cast<float>(BiomeHash2D(X1, Y0, Seed)) * InvU32;
    const float V01 = static_cast<float>(BiomeHash2D(X0, Y1, Seed)) * InvU32;
    const float V11 = static_cast<float>(BiomeHash2D(X1, Y1, Seed)) * InvU32;

    const float Top = FMath::Lerp(V00, V10, Wx);
    const float Bottom = FMath::Lerp(V01, V11, Wx);
    return FMath::Clamp(FMath::Lerp(Top, Bottom, Wy), 0.0f, 1.0f);
}

EMythicBiome UMythicTerritoryGrid::ComputeBiome(float Elevation, float Moisture, const FMythicBiomeThresholds &T) {
    if (Elevation > T.MountainElevation) {
        return EMythicBiome::Mountain;
    }
    if (Moisture < T.WastelandMoisture && Elevation > T.DesertElevation) {
        return EMythicBiome::Desert;
    }
    if (Moisture > T.WetlandMoisture) {
        return EMythicBiome::Wetland;
    }
    if (Moisture > T.ForestMoisture) {
        return EMythicBiome::Forest;
    }
    if (Moisture < T.WastelandMoisture) {
        return EMythicBiome::Wasteland;
    }
    return EMythicBiome::Plains;
}

EMythicBiome UMythicTerritoryGrid::GetBiomeAtCell(const FMythicCellCoord &Coord) const {
    if (!IsValidCoord(Coord)) {
        return EMythicBiome::Plains;
    }
    const float Elevation = BiomeValueNoise(Coord.X, Coord.Y, BiomeWorldSeed ^ 0xA5A5A5A5u, BiomeNoiseFrequency);
    const float Moisture = BiomeValueNoise(Coord.X, Coord.Y, BiomeWorldSeed ^ 0x5A5A5A5Au, BiomeNoiseFrequency);
    return ComputeBiome(Elevation, Moisture, BiomeThresholds);
}

EMythicBiome UMythicTerritoryGrid::GetBiomeAtWorld(const FVector &WorldPos) const {
    return GetBiomeAtCell(WorldToCell(WorldPos));
}


EMythicDangerTier UMythicTerritoryGrid::GetCellDangerTier(const FMythicCellCoord &Cell) const {
    if (!IsValidCoord(Cell)) {
        return EMythicDangerTier::Safe;
    }

    const FMythicCellCoord Core(Width / 2, Height / 2);
    const FMythicTerritoryCell CellData = GetCell(Cell);

    const int32 Distance = FMythicDanger::ChebyshevDistance(Cell, Core);
    const bool bSafeZone = (Cell == Core) || CellData.bPlayerOwned;
    return FMythicDanger::ComputeDangerTier(Distance, CellData.Influence, bSafeZone, FMythicDangerTierParams());
}

void UMythicTerritoryGrid::GetFactionCells(FMythicFactionId Faction, int32 MaxResults, TArray<FMythicCellCoord> &OutCells) const {
    OutCells.Reset();
    if (!Faction.IsValid()) {
        return;
    }

    FScopeLock Lock(&SnapshotLock);
    const TArray<FMythicCellCoord> &CachedCells = ReadFactionCells[Faction.Index];

    const int32 Count = FMath::Min(MaxResults, CachedCells.Num());
    if (Count > 0) {
        OutCells.Append(CachedCells.GetData(), Count);
    }
}

void UMythicTerritoryGrid::GetChangedCells(TArray<FMythicCellCoord> &OutChangedCells) const {
    OutChangedCells.Reset();
    FScopeLock Lock(&SnapshotLock);
    for (TConstSetBitIterator<> It(ReadDirtyCells); It; ++It) {
        const int32 Index = It.GetIndex();
        OutChangedCells.Add(FMythicCellCoord(
            Index % Width,
            Index / Width
            ));
    }
    ReadDirtyCells.Init(false, ReadDirtyCells.Num());
}

void UMythicTerritoryGrid::Serialize(FArchive &Ar) {
    int32 Version = 2;
    Ar << Version;

    Ar << Width;
    Ar << Height;
    Ar << CellWorldSize;
    Ar << WorldOrigin.X;
    Ar << WorldOrigin.Y;
    Ar << InfluenceBleedRate;
    Ar << MinControlThreshold;

    const int64 TotalCells64 = static_cast<int64>(Width) * static_cast<int64>(Height);

    if (Ar.IsLoading()) {
        if (Width < 0 || Height < 0 || TotalCells64 < 0 || TotalCells64 > 100000000) {
            Ar.SetError();
            return;
        }
        const int32 SafeTotal = static_cast<int32>(TotalCells64);
        WriteBuffer.SetNum(SafeTotal);
        ReadBuffer.SetNum(SafeTotal);
        DirtyCells.Init(false, SafeTotal);
        ReadDirtyCells.Init(false, SafeTotal);
    }

    const int32 TotalCells = static_cast<int32>(TotalCells64);

    for (int32 i = 0; i < TotalCells; ++i) {
        FMythicTerritoryCell &Cell = WriteBuffer[i];
        Ar << Cell.DominantFaction.Index;
        Ar << Cell.Influence;

        if (Version >= 2) {
            uint8 PlayerOwnedByte = Cell.bPlayerOwned ? 1 : 0;
            Ar << PlayerOwnedByte;
            Ar << Cell.OwningPlayerIndex;
            if (Ar.IsLoading()) {
                Cell.bPlayerOwned = PlayerOwnedByte != 0;
            }
        }
        else {
            uint8 PlayerData = (Cell.bPlayerOwned ? 1 : 0) | (Cell.OwningPlayerIndex << 1);
            Ar << PlayerData;
            if (Ar.IsLoading()) {
                Cell.bPlayerOwned = (PlayerData & 1) != 0;
                Cell.OwningPlayerIndex = PlayerData >> 1;
            }
        }
    }

    if (Ar.IsLoading()) {
        CommitWrites();
    }
}
