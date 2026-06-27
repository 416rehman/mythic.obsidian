// Mythic Living World — War-Map Texture Subsystem (implementation)
// Client-only. Reads replicated proxies + client-loaded settings; builds a per-cell BGRA8 texture event-driven.

#include "MythicWarMapSubsystem.h"

#include "Engine/Texture2D.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "TextureResource.h"

#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/LivingWorldReplication.h"
#include "World/LivingWorld/Factions/FactionColor.h"

// ─────────────────────────────────────────────────────────────
// Lifecycle
// ─────────────────────────────────────────────────────────────

void UMythicWarMapSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
    Super::Initialize(Collection);
    // Grid + texture are resolved lazily on first use (the living-world subsystem / replicator may not exist yet at
    // local-player-subsystem init). Bind to the proxy-change delegate the first time we can reach the subsystem.
}

void UMythicWarMapSubsystem::Deinitialize() {
    if (bBoundToProxies) {
        if (UMythicLivingWorldSubsystem* LWS = GetLivingWorld()) {
            LWS->OnLivingWorldProxiesChanged.RemoveAll(this);
        }
        bBoundToProxies = false;
    }
    WarMapTexture = nullptr;
    PixelBuffer.Empty();
    DominantByCell.Empty();
    CachedInitialFactions.Empty();
    Super::Deinitialize();
}

// ─────────────────────────────────────────────────────────────
// Subsystem / grid resolution
// ─────────────────────────────────────────────────────────────

UMythicLivingWorldSubsystem* UMythicWarMapSubsystem::GetLivingWorld() const {
    if (const ULocalPlayer* LP = GetLocalPlayer()) {
        if (const UGameInstance* GI = LP->GetGameInstance()) {
            return GI->GetSubsystem<UMythicLivingWorldSubsystem>();
        }
    }
    return nullptr;
}

bool UMythicWarMapSubsystem::EnsureGridResolved() {
    if (GridW > 0 && GridH > 0) {
        return true;
    }

    UMythicLivingWorldSubsystem* LWS = GetLivingWorld();
    if (!LWS) {
        return false;
    }

    // Bind to proxy changes the first time we can reach the subsystem (idempotent).
    if (!bBoundToProxies) {
        LWS->OnLivingWorldProxiesChanged.AddDynamic(this, &UMythicWarMapSubsystem::HandleProxiesChanged);
        bBoundToProxies = true;
    }

    const UMythicLivingWorldSettings* LWSettings = LWS->GetSettings();
    if (!LWSettings) {
        return false;
    }

    // Territory dims/origin/cell-size (client-loaded, not replicated). TSoftObjectPtr -> sync load on the client.
    const UMythicTerritoryGridSettings* TerritorySettings = LWSettings->TerritorySettings.LoadSynchronous();
    if (!TerritorySettings) {
        return false;
    }
    GridW = TerritorySettings->GridWidth;
    GridH = TerritorySettings->GridHeight;
    WorldOrigin = TerritorySettings->WorldOrigin;
    CellWorldSize = TerritorySettings->CellWorldSize;

    if (GridW <= 0 || GridH <= 0) {
        GridW = 0;
        GridH = 0;
        return false;
    }

    // Faction definitions (color + name). Optional — a missing list just means deterministic colors + empty names.
    CachedInitialFactions.Reset();
    if (const UMythicFactionDatabaseSettings* FactionSettings = LWSettings->FactionSettings.LoadSynchronous()) {
        CachedInitialFactions = FactionSettings->InitialFactions;
    }

    // Size the CPU buffers. DominantByCell starts fully unclaimed (0xFF); it accumulates as delta proxies arrive.
    const int32 Count = GridW * GridH;
    DominantByCell.Init(0xFF, Count);
    PixelBuffer.Init(Style.UnclaimedColor, Count);

    return true;
}

FColor UMythicWarMapSubsystem::ResolveColor(uint8 FactionIndex) const {
    return MythicWarMap::ResolveFactionColorForId(FactionIndex, CachedInitialFactions);
}

// ─────────────────────────────────────────────────────────────
// Texture create + upload
// ─────────────────────────────────────────────────────────────

void UMythicWarMapSubsystem::EnsureTexture() {
    if (WarMapTexture || GridW <= 0 || GridH <= 0) {
        return;
    }

    WarMapTexture = UTexture2D::CreateTransient(GridW, GridH, PF_B8G8R8A8);
    if (!WarMapTexture) {
        return;
    }
    WarMapTexture->SRGB = false;                       // raw color data, no gamma
    WarMapTexture->Filter = TextureFilter::TF_Nearest; // crisp per-cell blocks
    WarMapTexture->NeverStream = true;
    // No AddToRoot needed — WarMapTexture is a UPROPERTY(Transient) on this subsystem, so it is GC-reachable.
    WarMapTexture->UpdateResource();
}

void UMythicWarMapSubsystem::UploadTexture() {
    if (!WarMapTexture || PixelBuffer.Num() != GridW * GridH || GridW <= 0 || GridH <= 0) {
        return;
    }

    FTexturePlatformData* PlatformData = WarMapTexture->GetPlatformData();
    if (!PlatformData || PlatformData->Mips.Num() == 0) {
        return;
    }

    FTexture2DMipMap& Mip = PlatformData->Mips[0];
    void* Dest = Mip.BulkData.Lock(LOCK_READ_WRITE);
    if (Dest) {
        const int32 NumBytes = PixelBuffer.Num() * sizeof(FColor);
        FMemory::Memcpy(Dest, PixelBuffer.GetData(), NumBytes);
    }
    Mip.BulkData.Unlock();
    WarMapTexture->UpdateResource();
}

UTexture2D* UMythicWarMapSubsystem::GetWarMapTexture() {
    if (!EnsureGridResolved()) {
        return nullptr;
    }
    EnsureTexture();
    if (PixelBuffer.Num() != GridW * GridH) {
        // First access before any refresh — build once so the texture isn't blank garbage.
        RefreshNow();
    }
    return WarMapTexture;
}

// ─────────────────────────────────────────────────────────────
// Refresh — accumulate deltas, rebuild pixels, upload, broadcast
// ─────────────────────────────────────────────────────────────

void UMythicWarMapSubsystem::HandleProxiesChanged() {
    RefreshNow();
}

void UMythicWarMapSubsystem::RefreshNow() {
    if (!EnsureGridResolved()) {
        return;
    }
    EnsureTexture();

    const int32 Count = GridW * GridH;
    if (DominantByCell.Num() != Count) {
        DominantByCell.Init(0xFF, Count);
    }
    if (PixelBuffer.Num() != Count) {
        PixelBuffer.Init(Style.UnclaimedColor, Count);
    }

    UMythicLivingWorldSubsystem* LWS = GetLivingWorld();
    if (!LWS) {
        return;
    }

    // 1) Accumulate the replicated territory deltas into DominantByCell. Proxies are delta-only — the server sends a
    //    cell once when its dominant faction flips, so we keep the persistent accumulator and overwrite touched cells.
    for (const FMythicTerritoryProxyItem& Item : LWS->GetAllTerritoryProxies()) {
        const int32 X = Item.Cell.X;
        const int32 Y = Item.Cell.Y;
        if (X < 0 || X >= GridW || Y < 0 || Y >= GridH) {
            continue;
        }
        DominantByCell[MythicWarMap::CoordToTexelIndex(X, Y, GridW)] = Item.ControllingFaction.Index;
    }

    const FMythicWarMapStyle PODStyle = Style.ToPOD();

    // Neighbor lookup seam for the border pass (reads the accumulator directly).
    auto FactionAt = [this](int32 X, int32 Y) -> uint8 {
        return DominantByCell[MythicWarMap::CoordToTexelIndex(X, Y, GridW)];
    };
    auto ColorFor = [this](uint8 Idx) -> FColor {
        return ResolveColor(Idx);
    };

    // 2) Base fill, then border darken on claimed border texels.
    for (int32 Y = 0; Y < GridH; ++Y) {
        for (int32 X = 0; X < GridW; ++X) {
            const int32 Idx = MythicWarMap::CoordToTexelIndex(X, Y, GridW);

            FMythicWarMapCell Cell;
            Cell.FactionIndex = DominantByCell[Idx];
            Cell.bPlayerOwned = false; // bPlayerOwned isn't carried on the territory proxy yet (half-wired upstream)

            FColor Px = MythicWarMap::CellToColor(Cell, ColorFor, PODStyle);

            if (PODStyle.bDrawBorders && Cell.FactionIndex != 0xFF &&
                MythicWarMap::IsBorderCell(X, Y, GridW, GridH, FactionAt)) {
                Px = MythicWarMap::ApplyBorder(Px, PODStyle);
            }

            PixelBuffer[Idx] = Px;
        }
    }

    UploadTexture();

    // Cache marker/legend counts for the debugger (cheap recompute).
    {
        TArray<FMythicWarMapMarker> Markers;
        GetMarkers(Markers);
        LastSettlementMarkerCount = 0;
        LastEncounterMarkerCount = 0;
        for (const FMythicWarMapMarker& M : Markers) {
            if (M.Kind == EMythicWarMapMarkerKind::Encounter) {
                ++LastEncounterMarkerCount;
            } else {
                ++LastSettlementMarkerCount; // Settlement + Capital
            }
        }
        TArray<FMythicWarMapLegendEntry> Legend;
        GetLegendEntries(Legend);
        LastLegendEntryCount = Legend.Num();
    }

    OnWarMapChanged.Broadcast();
}

// ─────────────────────────────────────────────────────────────
// Map data API
// ─────────────────────────────────────────────────────────────

void UMythicWarMapSubsystem::GetLegendEntries(TArray<FMythicWarMapLegendEntry>& Out) const {
    Out.Reset();
    if (GridW <= 0 || GridH <= 0 || DominantByCell.Num() != GridW * GridH) {
        return;
    }

    // Tally controlled cells per faction index from the client accumulator.
    TMap<uint8, int32> CountByIndex;
    for (const uint8 Idx : DominantByCell) {
        if (Idx != 0xFF) {
            CountByIndex.FindOrAdd(Idx)++;
        }
    }

    Out.Reserve(CountByIndex.Num());
    for (const TPair<uint8, int32>& Pair : CountByIndex) {
        FMythicWarMapLegendEntry Entry;
        Entry.FactionId.Index = Pair.Key;
        Entry.Color = ResolveColor(Pair.Key);
        Entry.ControlledCellCount = Pair.Value;
        if (static_cast<int32>(Pair.Key) < CachedInitialFactions.Num()) {
            Entry.DisplayName = CachedInitialFactions[Pair.Key].DisplayName;
        }
        Out.Add(Entry);
    }

    // Stable ordering: highest cell count first (the dominant power leads the legend).
    Out.Sort([](const FMythicWarMapLegendEntry& A, const FMythicWarMapLegendEntry& B) {
        return A.ControlledCellCount > B.ControlledCellCount;
    });
}

void UMythicWarMapSubsystem::GetMarkers(TArray<FMythicWarMapMarker>& Out) const {
    Out.Reset();
    if (GridW <= 0 || GridH <= 0) {
        return;
    }

    UMythicLivingWorldSubsystem* LWS = GetLivingWorld();
    if (!LWS) {
        return;
    }

    // Settlements (proxy carries center cell, governing faction, display name, capital flag).
    for (const FMythicSettlementProxyItem& S : LWS->GetAllSettlementProxies()) {
        FMythicWarMapMarker M;
        M.NormalizedPos = MythicWarMap::CellToNormalized(S.CenterCell.X, S.CenterCell.Y, GridW, GridH);
        M.Color = ResolveColor(S.GoverningFaction.Index);
        M.Label = S.DisplayName;
        M.bIsCapital = S.bIsCapital;
        M.Kind = S.bIsCapital ? EMythicWarMapMarkerKind::Capital : EMythicWarMapMarkerKind::Settlement;
        Out.Add(M);
    }

    // Active encounters.
    for (const FMythicEncounterProxyItem& E : LWS->GetAllEncounterProxies()) {
        FMythicWarMapMarker M;
        M.NormalizedPos = MythicWarMap::CellToNormalized(E.Cell.X, E.Cell.Y, GridW, GridH);
        M.Color = ResolveColor(E.OriginFaction.Index);
        M.Kind = EMythicWarMapMarkerKind::Encounter;
        Out.Add(M);
    }
}

FMythicWarMapMarker UMythicWarMapSubsystem::GetPlayerMarker() const {
    FMythicWarMapMarker M;
    M.Kind = EMythicWarMapMarkerKind::Player;
    M.Color = Style.PlayerOwnedColor;

    if (GridW <= 0 || GridH <= 0 || CellWorldSize <= 0.0f) {
        return M;
    }

    if (const ULocalPlayer* LP = GetLocalPlayer()) {
        if (const APlayerController* PC = LP->GetPlayerController(LP->GetWorld())) {
            if (const APawn* Pawn = PC->GetPawn()) {
                const FVector Loc = Pawn->GetActorLocation();
                M.NormalizedPos = MythicWarMap::WorldToNormalized(FVector2D(Loc.X, Loc.Y), WorldOrigin,
                                                                  CellWorldSize, GridW, GridH);
            }
        }
    }
    return M;
}

FVector2D UMythicWarMapSubsystem::CellToUV(FMythicCellCoord Cell) const {
    return MythicWarMap::CellToNormalized(Cell.X, Cell.Y, GridW, GridH);
}

int32 UMythicWarMapSubsystem::GetAccumulatedClaimedCellCount() const {
    int32 N = 0;
    for (const uint8 Idx : DominantByCell) {
        if (Idx != 0xFF) {
            ++N;
        }
    }
    return N;
}
