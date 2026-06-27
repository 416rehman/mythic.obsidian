// Mythic Living World — War-Map pure helpers (implementation)
// Engine-free, deterministic, allocation-free. Unit-tested by MythicWarMapTests.cpp.

#include "MythicWarMapTypes.h"
#include "World/LivingWorld/Factions/FactionColor.h"
#include "World/LivingWorld/Factions/FactionDatabase.h" // FMythicFactionData (full type for GetFactionColor)

namespace MythicWarMap {

int32 CoordToTexelIndex(int32 X, int32 Y, int32 W) {
    return Y * W + X;
}

FColor CellToColor(const FMythicWarMapCell& Cell, TFunctionRef<FColor(uint8)> ColorForIndex,
                   const FMythicWarMapStyle& Style) {
    // Unclaimed land paints nothing (transparent by default).
    if (Cell.FactionIndex == 0xFF) {
        return Style.UnclaimedColor;
    }

    const FColor FactionColor = ColorForIndex(Cell.FactionIndex);

    if (Cell.bPlayerOwned) {
        // Blend the faction color toward the player-owned highlight by PlayerTint. Lerp per-channel in integer space;
        // FColor is BGRA bytes — channel identity doesn't matter for a symmetric lerp, so do it field-wise.
        const float T = FMath::Clamp(Style.PlayerTint, 0.0f, 1.0f);
        auto LerpByte = [T](uint8 A, uint8 B) -> uint8 {
            return static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(A + (static_cast<int32>(B) - A) * T), 0, 255));
        };
        FColor Out;
        Out.R = LerpByte(FactionColor.R, Style.PlayerOwnedColor.R);
        Out.G = LerpByte(FactionColor.G, Style.PlayerOwnedColor.G);
        Out.B = LerpByte(FactionColor.B, Style.PlayerOwnedColor.B);
        Out.A = LerpByte(FactionColor.A, Style.PlayerOwnedColor.A);
        return Out;
    }

    return FactionColor;
}

bool IsBorderCell(int32 X, int32 Y, int32 W, int32 H, TFunctionRef<uint8(int32, int32)> FactionAt) {
    const uint8 Self = FactionAt(X, Y);
    if (Self == 0xFF) {
        return false; // unclaimed cells are never borders
    }

    // 4-neighbourhood. Skip out-of-bounds neighbors (a grid edge is not a faction border).
    static const int32 DX[4] = {1, -1, 0, 0};
    static const int32 DY[4] = {0, 0, 1, -1};
    for (int32 i = 0; i < 4; ++i) {
        const int32 NX = X + DX[i];
        const int32 NY = Y + DY[i];
        if (NX < 0 || NX >= W || NY < 0 || NY >= H) {
            continue;
        }
        if (FactionAt(NX, NY) != Self) {
            return true; // a differing in-bounds neighbor (including unclaimed) makes this an edge
        }
    }
    return false;
}

FColor ApplyBorder(FColor Base, const FMythicWarMapStyle& Style) {
    const float D = FMath::Clamp(Style.BorderDarken, 0.0f, 1.0f);
    FColor Out;
    Out.R = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(Base.R * D), 0, 255));
    Out.G = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(Base.G * D), 0, 255));
    Out.B = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(Base.B * D), 0, 255));
    Out.A = 255; // border outline is always fully opaque so it reads against transparent fills
    return Out;
}

FVector2D WorldToNormalized(const FVector2D& WorldXY, const FVector2D& WorldOrigin, float CellWorldSize,
                            int32 GridW, int32 GridH) {
    if (CellWorldSize <= 0.0f || GridW <= 0 || GridH <= 0) {
        return FVector2D::ZeroVector;
    }
    // Cell-fraction coordinates (continuous): how many cells from the origin corner.
    const float CellFX = (WorldXY.X - WorldOrigin.X) / CellWorldSize;
    const float CellFY = (WorldXY.Y - WorldOrigin.Y) / CellWorldSize;

    const float NX = FMath::Clamp(CellFX / static_cast<float>(GridW), 0.0f, 1.0f);
    const float NYUp = FMath::Clamp(CellFY / static_cast<float>(GridH), 0.0f, 1.0f);
    // Flip Y for UMG (top-left origin). Re-clamp guards against -0.0 / FP edge.
    const float NY = FMath::Clamp(1.0f - NYUp, 0.0f, 1.0f);
    return FVector2D(NX, NY);
}

FVector2D CellToNormalized(int32 CellX, int32 CellY, int32 GridW, int32 GridH) {
    if (GridW <= 0 || GridH <= 0) {
        return FVector2D::ZeroVector;
    }
    const float NX = (static_cast<float>(CellX) + 0.5f) / static_cast<float>(GridW);
    const float NY = 1.0f - (static_cast<float>(CellY) + 0.5f) / static_cast<float>(GridH);
    return FVector2D(FMath::Clamp(NX, 0.0f, 1.0f), FMath::Clamp(NY, 0.0f, 1.0f));
}

FColor ResolveFactionColorForId(uint8 FactionIndex, TConstArrayView<FMythicFactionData> InitialFactions) {
    if (FactionIndex != 0xFF && static_cast<int32>(FactionIndex) < InitialFactions.Num()) {
        return MythicFactionColor::GetFactionColor(InitialFactions[FactionIndex], FactionIndex);
    }
    return MythicFactionColor::DeterministicColorForId(FactionIndex);
}

} // namespace MythicWarMap
