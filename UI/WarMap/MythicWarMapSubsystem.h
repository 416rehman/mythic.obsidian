
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "MythicWarMapTypes.h"
#include "MythicWarMapSubsystem.generated.h"

class UTexture2D;
class UMythicLivingWorldSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMythicOnWarMapChanged);

UCLASS()
class MYTHIC_API UMythicWarMapSubsystem : public ULocalPlayerSubsystem {
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;


    /** Fired after the texture + map data are rebuilt (RefreshNow / a proxy-change). UI binds this. */
    UPROPERTY(BlueprintAssignable, Category = "War Map")
    FMythicOnWarMapChanged OnWarMapChanged;


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

    /**
     * World XY of a marker's normalized position, for anything that needs a BEARING rather than a map pixel.
     *
     * The HUD compass is the caller: markers are gathered once, normalized for the map, and inverted back here so
     * both screens draw the same set. Returns false while the grid is unresolved, so a caller can skip rather than
     * point at the world origin.
     */
    UFUNCTION(BlueprintPure, Category = "War Map")
    bool GetMarkerWorldXY(const FMythicWarMapMarker &Marker, FVector2D &OutWorldXY) const;

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


    int32 GetAccumulatedClaimedCellCount() const;

    int32 GetLastSettlementMarkerCount() const { return LastSettlementMarkerCount; }
    int32 GetLastEncounterMarkerCount() const { return LastEncounterMarkerCount; }
    int32 GetLastLegendEntryCount() const { return LastLegendEntryCount; }

private:
    UMythicLivingWorldSubsystem* GetLivingWorld() const;

    bool EnsureGridResolved();

    FColor ResolveColor(uint8 FactionIndex) const;

    UFUNCTION()
    void HandleProxiesChanged();

    void EnsureTexture();

    void UploadTexture();


    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> WarMapTexture;

    TArray<FColor> PixelBuffer;

    TArray<uint8> DominantByCell;

    TArray<FMythicFactionData> CachedInitialFactions;

    int32 GridW = 0;
    int32 GridH = 0;
    FVector2D WorldOrigin = FVector2D::ZeroVector;
    float CellWorldSize = 0.0f;

    bool bBoundToProxies = false;

    int32 LastSettlementMarkerCount = 0;
    int32 LastEncounterMarkerCount = 0;
    int32 LastLegendEntryCount = 0;
};
