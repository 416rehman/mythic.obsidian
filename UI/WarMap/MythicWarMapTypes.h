// Mythic Living World — War-Map pure types + helpers
// The strategic war-map renders the replicated TERRITORY grid into a UTexture2D (one texel per cell), tinted by each
// cell's controlling-faction color, with conquered borders darkened. This header holds the ENGINE-FREE, deterministic
// core of that pipeline: the POD cell/style structs, the BP-facing legend/marker USTRUCTs, and the MythicWarMap pure
// namespace (cell->color, border edge-detect, world<->normalized transforms, faction-color resolve). Everything here is
// pure + static (TFunctionRef seams for the neighbor/color lookups) so it unit-tests with NO render or replication link.
//
// Single source of truth for the war-map: the texture subsystem (UMythicWarMapSubsystem) and the screen
// (UMythicWarMapScreen) both consume THESE types. Slices that previously duplicated FMythicWarMapLegendEntry / border
// helpers / cell->color are merged here.

#pragma once

#include "CoreMinimal.h"
#include "World/LivingWorld/LivingWorldTypes.h"     // FMythicCellCoord, FMythicFactionId
#include "MythicWarMapTypes.generated.h"

struct FMythicFactionData;

// ─────────────────────────────────────────────────────────────
// Marker kind — what a war-map overlay marker represents
// ─────────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class EMythicWarMapMarkerKind : uint8 {
    Settlement = 0 UMETA(DisplayName = "Settlement"),
    Capital UMETA(DisplayName = "Capital"),
    Player UMETA(DisplayName = "Player"),
    Encounter UMETA(DisplayName = "Encounter"),
    COUNT UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────
// POD cell + style — the texture build inputs (NOT USTRUCT — pure C++)
// ─────────────────────────────────────────────────────────────

/** One war-map cell sampled from the client's accumulated territory state. FactionIndex 0xFF == unclaimed. */
struct FMythicWarMapCell {
    uint8 FactionIndex = 0xFF; // 0xFF == unclaimed (FMythicFactionId::InvalidIndex)
    bool bPlayerOwned = false;
};

/** Designer-tunable rendering style for the war-map texture. Plain C++ (game-thread build input), mirrored as a
 *  BlueprintReadWrite USTRUCT on the subsystem via FMythicWarMapStyleBP below for editor exposure. */
struct FMythicWarMapStyle {
    FColor UnclaimedColor = FColor(0, 0, 0, 0); // fully transparent — unclaimed land reads as empty on the UI
    FColor PlayerOwnedColor = FColor::White;    // blend target for player-held cells
    float PlayerTint = 0.5f;                     // [0,1] lerp weight toward PlayerOwnedColor for player cells
    float BorderDarken = 0.45f;                  // RGB multiplier on a border texel
    bool bDrawBorders = true;
};

// ─────────────────────────────────────────────────────────────
// BP-facing style — editor-exposed mirror of FMythicWarMapStyle
// ─────────────────────────────────────────────────────────────

/** Blueprint/editor-exposed war-map style. The subsystem holds this and converts to the POD FMythicWarMapStyle for the
 *  pure render helpers. Kept a separate type so the pure core stays USTRUCT/UHT-free. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicWarMapStyleBP {
    GENERATED_BODY()

    /** Color for cells no faction controls (default fully transparent so empty land doesn't paint the UI). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "War Map")
    FColor UnclaimedColor = FColor(0, 0, 0, 0);

    /** Blend target color for cells the local player owns. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "War Map")
    FColor PlayerOwnedColor = FColor::White;

    /** How strongly a player-owned cell is tinted toward PlayerOwnedColor [0,1]. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "War Map", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float PlayerTint = 0.5f;

    /** RGB multiplier applied to border texels (lower = darker outline). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "War Map", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BorderDarken = 0.45f;

    /** Whether to darken the outer edge of each faction's territory. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "War Map")
    bool bDrawBorders = true;

    /** Convert to the engine-free POD style the pure helpers consume. */
    FMythicWarMapStyle ToPOD() const {
        FMythicWarMapStyle S;
        S.UnclaimedColor = UnclaimedColor;
        S.PlayerOwnedColor = PlayerOwnedColor;
        S.PlayerTint = PlayerTint;
        S.BorderDarken = BorderDarken;
        S.bDrawBorders = bDrawBorders;
        return S;
    }
};

// ─────────────────────────────────────────────────────────────
// Legend + marker — BP-facing read DTOs the screen renders
// ─────────────────────────────────────────────────────────────

/** One row of the war-map faction legend: a faction, its display name + color, and how many cells it controls. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicWarMapLegendEntry {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "War Map")
    FMythicFactionId FactionId;

    UPROPERTY(BlueprintReadOnly, Category = "War Map")
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "War Map")
    FColor Color = FColor::White;

    UPROPERTY(BlueprintReadOnly, Category = "War Map")
    int32 ControlledCellCount = 0;
};

/** A point overlay on the war-map (settlement / capital / player / encounter), positioned in UMG-normalized [0,1]^2. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicWarMapMarker {
    GENERATED_BODY()

    /** Position in UMG normalized space: (0,0) = top-left, (1,1) = bottom-right (Y already flipped from grid space). */
    UPROPERTY(BlueprintReadOnly, Category = "War Map")
    FVector2D NormalizedPos = FVector2D::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "War Map")
    FColor Color = FColor::White;

    UPROPERTY(BlueprintReadOnly, Category = "War Map")
    FText Label;

    UPROPERTY(BlueprintReadOnly, Category = "War Map")
    EMythicWarMapMarkerKind Kind = EMythicWarMapMarkerKind::Settlement;

    UPROPERTY(BlueprintReadOnly, Category = "War Map")
    bool bIsCapital = false;
};

// ─────────────────────────────────────────────────────────────
// Pure render namespace — engine-free, deterministic, unit-testable
// ─────────────────────────────────────────────────────────────

namespace MythicWarMap {
    /** Flatten a cell coord to a texel index. Matches UMythicTerritoryGrid::CoordToIndex (Y*W+X) so the texture's
     *  row/column layout is identical to the grid's. Caller guarantees 0<=X<W, 0<=Y<H. */
    MYTHIC_API int32 CoordToTexelIndex(int32 X, int32 Y, int32 W);

    /**
     * Resolve a single cell's base color.
     *   unclaimed (0xFF)       -> Style.UnclaimedColor (transparent by default)
     *   player-owned           -> ColorForIndex(idx) blended toward Style.PlayerOwnedColor by Style.PlayerTint
     *   else                   -> ColorForIndex(idx)
     * ColorForIndex maps a faction index to its display color (faction-color resolver, injected as a seam for tests).
     */
    MYTHIC_API FColor CellToColor(const FMythicWarMapCell& Cell, TFunctionRef<FColor(uint8)> ColorForIndex,
                                  const FMythicWarMapStyle& Style);

    /**
     * Border test: a cell is a border iff it is CLAIMED and at least one in-bounds 4-neighbor has a DIFFERENT faction
     * index. Unclaimed cells are never borders. Out-of-bounds neighbors are skipped (do not count as "different").
     * FactionAt(X,Y) returns the faction index at a cell (0xFF for unclaimed); only called for in-bounds neighbors.
     */
    MYTHIC_API bool IsBorderCell(int32 X, int32 Y, int32 W, int32 H, TFunctionRef<uint8(int32, int32)> FactionAt);

    /** Darken a base color for a border texel: RGB *= Style.BorderDarken, alpha forced opaque (255) so the outline
     *  is always visible even when the faction fill is partly transparent. */
    MYTHIC_API FColor ApplyBorder(FColor Base, const FMythicWarMapStyle& Style);

    /**
     * World XY -> UMG-normalized [0,1]^2. Maps a world position into cell-fraction space then flips Y (grid Y grows
     * "up"/north, UMG Y grows down), clamping to [0,1]. WorldOrigin is the grid's bottom-left corner; CellWorldSize is
     * cm per cell. Degenerate inputs (CellWorldSize<=0, GridW/H<=0) clamp to (0,0).
     */
    MYTHIC_API FVector2D WorldToNormalized(const FVector2D& WorldXY, const FVector2D& WorldOrigin, float CellWorldSize,
                                           int32 GridW, int32 GridH);

    /** Cell center -> UMG-normalized [0,1]^2 (Y flipped). { (X+0.5)/W, 1 - (Y+0.5)/H }. Degenerate grid -> (0,0). */
    MYTHIC_API FVector2D CellToNormalized(int32 CellX, int32 CellY, int32 GridW, int32 GridH);

    /**
     * Resolve the display color for a faction index from the client-loaded InitialFactions list: when the index is a
     * valid row, use MythicFactionColor::GetFactionColor (honors the per-faction override); otherwise fall back to the
     * deterministic-from-id color. Single resolver shared by the texture build + the legend so a faction's color is
     * identical in both. Defined in the .cpp (needs the FactionColor + FMythicFactionData headers).
     */
    MYTHIC_API FColor ResolveFactionColorForId(uint8 FactionIndex, TConstArrayView<FMythicFactionData> InitialFactions);
} // namespace MythicWarMap
