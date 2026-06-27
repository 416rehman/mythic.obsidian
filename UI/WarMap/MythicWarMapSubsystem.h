// Mythic Living World — War-Map Texture Subsystem (client-side)
// Builds the strategic war-map UTexture2D from the REPLICATED living-world proxies and exposes a map-data API (texture,
// legend, markers, player marker, cell->UV). One ULocalPlayerSubsystem per local player (the texture is per-player GPU
// state). NOTHING here is replicated: the subsystem reads the already-replicated territory/faction/settlement/encounter
// proxies + the client-loaded settings assets, and rebuilds the texture EVENT-DRIVEN off the subsystem's
// OnLivingWorldProxiesChanged delegate (never per-frame).
//
// Territory proxies are DELTA-only (the server sends a cell once when its dominant faction flips), so the subsystem
// keeps a persistent DominantByCell accumulator that fills in over time as deltas arrive — a mid-game-joined client
// paints unclaimed cells transparent until their deltas land.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "World/LivingWorld/Factions/FactionDatabase.h" // FMythicFactionData (cached InitialFactions copy)
#include "MythicWarMapTypes.h"
#include "MythicWarMapSubsystem.generated.h"

class UTexture2D;
class UMythicLivingWorldSubsystem;

/** Fired (client-side) after the war-map texture + map data are rebuilt. The screen binds this to refresh. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMythicOnWarMapChanged);

/**
 * Per-local-player client subsystem that owns the war-map texture and map-data API. Lazily initializes its grid
 * dimensions / origin / cell size from the territory settings the first time it needs them (client-loaded, not
 * replicated). Binds to the living-world subsystem's proxy-change delegate so the texture is rebuilt exactly when the
 * replicated territory/faction state changes.
 */
UCLASS()
class MYTHIC_API UMythicWarMapSubsystem : public ULocalPlayerSubsystem {
    GENERATED_BODY()

public:
    // ─── ULocalPlayerSubsystem ────────────────────────────
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ─── Change delegate ──────────────────────────────────

    /** Fired after the texture + map data are rebuilt (RefreshNow / a proxy-change). UI binds this. */
    UPROPERTY(BlueprintAssignable, Category = "War Map")
    FMythicOnWarMapChanged OnWarMapChanged;

    // ─── Map data API ─────────────────────────────────────

    /** The war-map texture (one texel per cell, BGRA8). Lazily created + filled on first request / first refresh.
     *  Returns null only if the grid dimensions can't be resolved (no settings). */
    UFUNCTION(BlueprintCallable, Category = "War Map")
    UTexture2D* GetWarMapTexture();

    /** Grid width in cells (texture width). 0 until dimensions are resolved. */
    UFUNCTION(BlueprintPure, Category = "War Map")
    int32 GetGridWidth() const { return GridW; }

    /** Grid height in cells (texture height). 0 until dimensions are resolved. */
    UFUNCTION(BlueprintPure, Category = "War Map")
    int32 GetGridHeight() const { return GridH; }

    /** Fill Out with one legend entry per faction that currently controls >=1 cell (color + name + cell count). */
    UFUNCTION(BlueprintCallable, Category = "War Map")
    void GetLegendEntries(TArray<FMythicWarMapLegendEntry>& Out) const;

    /** Fill Out with settlement + encounter markers (positions in UMG-normalized [0,1]^2). */
    UFUNCTION(BlueprintCallable, Category = "War Map")
    void GetMarkers(TArray<FMythicWarMapMarker>& Out) const;

    /** The local player's pawn position as a Player-kind marker (normalized). bIsCapital=false. Label empty. If the
     *  pawn / grid can't be resolved the marker stays at (0,0) with Kind=Player — callers can treat (0,0) as "unknown"
     *  but it is always a well-defined value (no crash). */
    UFUNCTION(BlueprintPure, Category = "War Map")
    FMythicWarMapMarker GetPlayerMarker() const;

    /** Rebuild the texture + accumulators from the current replicated proxies, then broadcast OnWarMapChanged. Bound
     *  to the living-world subsystem's OnLivingWorldProxiesChanged. Safe to call manually (e.g. on screen open). */
    UFUNCTION(BlueprintCallable, Category = "War Map")
    void RefreshNow();

    /** Cell coord -> UMG-normalized [0,1]^2 (Y flipped). Thin wrapper over MythicWarMap::CellToNormalized for widget
     *  marker placement / minimap pins. */
    UFUNCTION(BlueprintPure, Category = "War Map")
    FVector2D CellToUV(FMythicCellCoord Cell) const;

    /** Editor/BP-tunable rendering style (colors, border darken, player tint). Converted to the POD style internally. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "War Map")
    FMythicWarMapStyleBP Style;

    // ─── Debugger read accessors (server has no war-map; this is the client view) ───

    /** Number of cells currently marked claimed in the accumulator (for the gameplay debugger). */
    int32 GetAccumulatedClaimedCellCount() const;

    /** Marker/legend counts from the last refresh (for the gameplay debugger). */
    int32 GetLastSettlementMarkerCount() const { return LastSettlementMarkerCount; }
    int32 GetLastEncounterMarkerCount() const { return LastEncounterMarkerCount; }
    int32 GetLastLegendEntryCount() const { return LastLegendEntryCount; }

private:
    /** Resolve the local UMythicLivingWorldSubsystem (the replicated-proxy cache). Null if unavailable. */
    UMythicLivingWorldSubsystem* GetLivingWorld() const;

    /** Lazily load grid dims/origin/cell size + the InitialFactions list from the client-side settings assets.
     *  Returns true once GridW/GridH are valid. Safe to call repeatedly (no-op once resolved). */
    bool EnsureGridResolved();

    /** Faction color for a faction index, via the cached InitialFactions list (override-aware). */
    FColor ResolveColor(uint8 FactionIndex) const;

    /** Bound to the living-world subsystem's proxy-change delegate — rebuilds on any proxy change. */
    UFUNCTION()
    void HandleProxiesChanged();

    /** Create the transient texture (BGRA8, nearest, never-stream) sized to the grid. No-op if already created. */
    void EnsureTexture();

    /** Re-upload PixelBuffer into the texture's mip 0. */
    void UploadTexture();

    // ─── Owned GPU + CPU state ───────────────────────────

    /** The war-map texture (transient, client-only). */
    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> WarMapTexture;

    /** CPU source-of-truth pixel buffer (W*H, BGRA via FColor). */
    TArray<FColor> PixelBuffer;

    /** Accumulated dominant faction index per cell (W*H, init 0xFF). Proxies are delta-only so this PERSISTS across
     *  refreshes and fills in as deltas arrive. */
    TArray<uint8> DominantByCell;

    /** Client-side copy of the faction definitions (color + name), loaded once from the faction settings asset. */
    TArray<FMythicFactionData> CachedInitialFactions;

    int32 GridW = 0;
    int32 GridH = 0;
    FVector2D WorldOrigin = FVector2D::ZeroVector;
    float CellWorldSize = 0.0f;

    /** Whether we've bound to the living-world proxy-change delegate yet. */
    bool bBoundToProxies = false;

    /** Last-refresh diagnostics (debugger). */
    int32 LastSettlementMarkerCount = 0;
    int32 LastEncounterMarkerCount = 0;
    int32 LastLegendEntryCount = 0;
};
