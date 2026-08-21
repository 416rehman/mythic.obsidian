
#pragma once

#include "CoreMinimal.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "MythicWarMapTypes.generated.h"

struct FMythicFactionData;


UENUM(BlueprintType)
enum class EMythicWarMapMarkerKind : uint8 {
    Settlement = 0 UMETA(DisplayName = "Settlement"),
    Capital UMETA(DisplayName = "Capital"),
    Player UMETA(DisplayName = "Player"),
    Encounter UMETA(DisplayName = "Encounter"),
    Objective UMETA(DisplayName = "Objective"),
    Waypoint UMETA(DisplayName = "Waypoint"),
    COUNT UMETA(Hidden)
};


struct FMythicWarMapCell {
    uint8 FactionIndex = 0xFF;
    bool bPlayerOwned = false;
};

struct FMythicWarMapStyle {
    FColor UnclaimedColor = FColor(0, 0, 0, 0);
    FColor PlayerOwnedColor = FColor::White;
    float PlayerTint = 0.5f;
    float BorderDarken = 0.45f;
    bool bDrawBorders = true;
};


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

    /** Pin colour for a discovered landmark. Deliberately faction-neutral — a landmark belongs to whoever found it. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "War Map")
    FColor WaypointColor = FColor(222, 184, 105);

    /** Pin colour for an active quest objective. Reads as "go here" rather than as territory. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "War Map")
    FColor ObjectiveColor = FColor(120, 200, 235);

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

    /**
     * What this marker points at, when that is a thing you can act on. For a Waypoint it is the POI id the fast-travel
     * RPC takes; other kinds leave it INDEX_NONE. Without it a marker is decoration — the map can draw a discovered
     * landmark but the player cannot travel to it.
     */
    UPROPERTY(BlueprintReadOnly, Category = "War Map")
    int32 SourceId = INDEX_NONE;
};


namespace MythicWarMap {
    MYTHIC_API int32 CoordToTexelIndex(int32 X, int32 Y, int32 W);

    MYTHIC_API FColor CellToColor(const FMythicWarMapCell& Cell, TFunctionRef<FColor(uint8)> ColorForIndex,
                                  const FMythicWarMapStyle& Style);

    MYTHIC_API bool IsBorderCell(int32 X, int32 Y, int32 W, int32 H, TFunctionRef<uint8(int32, int32)> FactionAt);

    MYTHIC_API FColor ApplyBorder(FColor Base, const FMythicWarMapStyle& Style);

    MYTHIC_API FVector2D WorldToNormalized(const FVector2D& WorldXY, const FVector2D& WorldOrigin, float CellWorldSize,
                                           int32 GridW, int32 GridH);

    MYTHIC_API FVector2D CellToNormalized(int32 CellX, int32 CellY, int32 GridW, int32 GridH);

    MYTHIC_API FVector2D NormalizedToWorld(const FVector2D& Normalized, const FVector2D& WorldOrigin,
                                           float CellWorldSize, int32 GridW, int32 GridH);

    MYTHIC_API FColor ResolveFactionColorForId(uint8 FactionIndex, TConstArrayView<FMythicFactionData> InitialFactions);
}
