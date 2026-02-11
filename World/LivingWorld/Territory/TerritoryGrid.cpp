// Mythic Living World System — Territory Grid Implementation

#include "World/LivingWorld/Territory/TerritoryGrid.h"

void UMythicTerritoryGrid::Initialize(const UMythicTerritoryGridSettings *Settings) {
    check(Settings);

    Width = Settings->GridWidth;
    Height = Settings->GridHeight;
    CellWorldSize = Settings->CellWorldSize;
    WorldOrigin = Settings->WorldOrigin;
    InfluenceBleedRate = Settings->InfluenceBleedRate;
    MinControlThreshold = Settings->MinControlThreshold;

    const int32 TotalCells = Width * Height;
    WriteBuffer.SetNum(TotalCells);
    ReadBuffer.SetNum(TotalCells);
    DirtyCells.Init(false, TotalCells);

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

    // Simple neighbor-averaging influence bleed
    // Each cell's influence bleeds to 4-neighbors at the configured rate
    // Iterates all cells — ~16K cells at 128x128, each with 4 neighbor checks
    // Cost: ~1ms worst case (only runs on background thread every 5-30s)

    // Work on a temp buffer to avoid read-during-write issues
    TArray<float> NewInfluence;
    NewInfluence.SetNumUninitialized(Width * Height);

    for (int32 Y = 0; Y < Height; ++Y) {
        for (int32 X = 0; X < Width; ++X) {
            const int32 Index = Y * Width + X;
            const FMythicTerritoryCell &Cell = WriteBuffer[Index];

            float BleedIn = 0.0f;
            int32 NeighborCount = 0;

            // 4-direction neighbors
            const int32 Neighbors[4][2] = {{X - 1, Y}, {X + 1, Y}, {X, Y - 1}, {X, Y + 1}};
            for (const auto &N : Neighbors) {
                if (N[0] >= 0 && N[0] < Width && N[1] >= 0 && N[1] < Height) {
                    const int32 NIdx = N[1] * Width + N[0];
                    const FMythicTerritoryCell &Neighbor = WriteBuffer[NIdx];

                    // Same faction neighbors reinforce influence
                    if (Neighbor.DominantFaction == Cell.DominantFaction) {
                        BleedIn += Neighbor.Influence * InfluenceBleedRate;
                    }
                    ++NeighborCount;
                }
            }

            // New influence = current influence + average bleed from same-faction neighbors, capped at 1.0
            if (NeighborCount > 0) {
                NewInfluence[Index] = FMath::Min(Cell.Influence + BleedIn / static_cast<float>(NeighborCount), 1.0f);
            }
            else {
                NewInfluence[Index] = Cell.Influence;
            }
        }
    }

    // Apply new influence values
    for (int32 i = 0; i < Width * Height; ++i) {
        if (WriteBuffer[i].Influence != NewInfluence[i]) {
            WriteBuffer[i].Influence = NewInfluence[i];
            DirtyCells[i] = true;
        }

        // Drop faction control if influence falls below threshold
        if (WriteBuffer[i].Influence < MinControlThreshold && WriteBuffer[i].DominantFaction.IsValid()) {
            WriteBuffer[i].DominantFaction = FMythicFactionId();
            WriteBuffer[i].Influence = 0.0f;
            DirtyCells[i] = true;
        }
    }
}

void UMythicTerritoryGrid::CommitWrites() {
    FMemory::Memcpy(ReadBuffer.GetData(), WriteBuffer.GetData(), WriteBuffer.Num() * sizeof(FMythicTerritoryCell));
    // DirtyCells preserved until GetChangedCells is called
}

FMythicTerritoryCell UMythicTerritoryGrid::GetCell(const FMythicCellCoord &Coord) const {
    if (!IsValidCoord(Coord)) {
        return FMythicTerritoryCell();
    }
    return ReadBuffer[CoordToIndex(Coord)];
}

FMythicFactionId UMythicTerritoryGrid::GetDominantFaction(const FMythicCellCoord &Coord) const {
    if (!IsValidCoord(Coord)) {
        return FMythicFactionId();
    }
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

void UMythicTerritoryGrid::GetFactionCells(FMythicFactionId Faction, int32 MaxResults, TArray<FMythicCellCoord> &OutCells) const {
    OutCells.Reset();
    for (int32 i = 0; i < ReadBuffer.Num() && OutCells.Num() < MaxResults; ++i) {
        if (ReadBuffer[i].DominantFaction == Faction) {
            OutCells.Add(FMythicCellCoord(
                i % Width,
                i / Width
                ));
        }
    }
}

void UMythicTerritoryGrid::GetChangedCells(TArray<FMythicCellCoord> &OutChangedCells) const {
    OutChangedCells.Reset();
    for (TConstSetBitIterator<> It(DirtyCells); It; ++It) {
        const int32 Index = It.GetIndex();
        OutChangedCells.Add(FMythicCellCoord(
            Index % Width,
            Index / Width
            ));
    }
}
