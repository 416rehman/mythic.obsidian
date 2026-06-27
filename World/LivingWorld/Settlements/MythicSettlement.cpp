// Mythic Living World — Settlement System Implementation
// Spline rasterization, registration with subsystem, zone detection.

#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Spawn/MythicPlacement.h" // FindValidSpawn — offline navmesh-valid spawn-point generation
#include "Settings/MythicDeveloperSettings.h"
#include "Components/SplineComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/GameInstance.h"
#include "UObject/ConstructorHelpers.h"

AMythicSettlement::AMythicSettlement() {
    PrimaryActorTick.bCanEverTick = false;

    BoundarySpline = CreateDefaultSubobject<USplineComponent>(TEXT("BoundarySpline"));
    BoundarySpline->SetClosedLoop(true);
    RootComponent = BoundarySpline;

    // Editor-only spawn-cell footprint preview. ISM renders for free in the editor viewport once it has a mesh +
    // instances — no canvas/PDI/HUD per-frame draw (the reliability mandate). Hidden in the shipped game.
    SpawnCellsISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("SpawnCellsISM"));
    SpawnCellsISM->SetupAttachment(RootComponent);
    SpawnCellsISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SpawnCellsISM->SetCastShadow(false);
    SpawnCellsISM->SetHiddenInGame(true); // editor viewport only — never visible in PIE/shipping
    SpawnCellsISM->bSelectable = false;
#if WITH_EDITOR
    SpawnCellsISM->SetIsVisualizationComponent(true); // excluded from cooked/runtime rendering & selection
#endif

    // Same assets the runtime Living World debug actor uses (known-good: they compile + render in this project).
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube"));
    if (CubeAsset.Succeeded()) {
        PreviewCellMesh = CubeAsset.Object;
        SpawnCellsISM->SetStaticMesh(PreviewCellMesh);
    }
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MatAsset(TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
    if (MatAsset.Succeeded()) {
        PreviewCellMaterial = MatAsset.Object;
    }
}

void AMythicSettlement::OnConstruction(const FTransform &Transform) {
    Super::OnConstruction(Transform);
    // Runs at edit-time on placement / move / spline edit (and once at spawn). Rebuild the spawn-cell footprint.
    RebuildSpawnCellPreview();
}

#if WITH_EDITOR
void AMythicSettlement::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) {
    Super::PostEditChangeProperty(PropertyChangedEvent);
    // Refresh when the toggle (or any other detail-panel property) changes — OnConstruction alone misses pure
    // property tweaks that don't re-run construction.
    RebuildSpawnCellPreview();
}
#endif

void AMythicSettlement::RebuildSpawnCellPreview() {
    // Guard every step: an editor-spawned actor in a fresh map may have no ISM, no spline, or unassigned settings.
    // Any failure path just clears the instances and returns — never crashes the editor.
    if (!SpawnCellsISM) {
        return;
    }
    SpawnCellsISM->ClearInstances();

    if (!bShowSpawnCellsInEditor || !BoundarySpline || !PreviewCellMesh) {
        return;
    }

    // ── Load the territory grid settings at edit-time (no PIE, no subsystem, no UMythicTerritoryGrid object) ──
    // Mirrors the runtime chain (UMythicLivingWorldSubsystem reads exactly these): DeveloperSettings CDO ->
    // LivingWorldSettings (soft) -> TerritorySettings (soft). LoadSynchronous handles a not-yet-loaded soft ref.
    // Fall back to the data-asset defaults (5000 / origin 0 / 128×128) so a missing asset still draws something.
    float CellWorldSize = 5000.0f;
    FVector2D WorldOrigin = FVector2D::ZeroVector;
    int32 GridWidth = 128;
    int32 GridHeight = 128;

    if (const UMythicDeveloperSettings *DevSettings = GetDefault<UMythicDeveloperSettings>()) {
        if (const UMythicLivingWorldSettings *LWSettings = DevSettings->LivingWorldSettings.LoadSynchronous()) {
            if (const UMythicTerritoryGridSettings *GridSettings = LWSettings->TerritorySettings.LoadSynchronous()) {
                CellWorldSize = GridSettings->CellWorldSize;
                WorldOrigin = GridSettings->WorldOrigin;
                GridWidth = GridSettings->GridWidth;
                GridHeight = GridSettings->GridHeight;
            }
        }
    }

    // ── Rasterize this settlement's spline to cells (SAME static helper the runtime spawn path uses) ──
    // NOTE ON GRANULARITY: PopulationSpawnerProcessor spawns every NPC at the deterministic CELL CENTER
    // (Grid->CellToWorld(cell)) with NO within-cell random/hashed jitter, so the per-cell footprint IS the exact
    // spawn picture — there is nothing finer to draw. We therefore visualize one cube per covered cell (the cell
    // footprint), which is both correct and sufficient. (The engine's post-spawn collision AdjustIfPossible nudge can
    // move an individual body off-center at runtime, but the spawn TARGET each cube marks is exact.)
    TArray<FMythicCellCoord> Cells;
    RasterizeSplineCells(BoundarySpline, CellWorldSize, WorldOrigin, GridWidth, GridHeight, Cells);
    if (Cells.Num() == 0) {
        return; // degenerate spline / out of grid bounds — nothing to draw
    }

    // ── One translucent cube instance per cell, at the cell's world CENTER, sized to the cell ──
    // Cube native size is 100cm; XY scale = CellWorldSize/100 fills the cell; thin Z reads as a floor tile.
    const float XYScale = CellWorldSize / 100.0f;
    const FVector InstanceScale(XYScale, XYScale, 0.05f);
    const float LiftZ = 50.0f; // lift slightly above terrain so the tile isn't z-fighting the ground

    for (const FMythicCellCoord &Cell : Cells) {
        // Cell CENTER in world space (identical to UMythicTerritoryGrid::CellToWorld) — the exact NPC spawn target.
        const float CenterX = static_cast<float>(WorldOrigin.X) + (static_cast<float>(Cell.X) + 0.5f) * CellWorldSize;
        const float CenterY = static_cast<float>(WorldOrigin.Y) + (static_cast<float>(Cell.Y) + 0.5f) * CellWorldSize;
        const FVector WorldCenter(CenterX, CenterY, LiftZ);
        // AddInstance's second arg = bWorldSpace (the ISM converts to component-relative internally) — same call the
        // runtime debug actor uses, so the instance lands on the cell regardless of the actor's transform.
        SpawnCellsISM->AddInstance(FTransform(FRotator::ZeroRotator, WorldCenter, InstanceScale), /*bWorldSpace*/ true);
    }

    // ── Tint by the governing faction tag for parity with the runtime debug actor (fixed translucent blue if none) ──
    if (PreviewCellMaterial) {
        FLinearColor Tint(0.1f, 0.6f, 1.0f, 0.35f); // default: translucent blue
        if (InitialFactionTag.IsValid()) {
            // Deterministic hue from the tag name hash so different factions read differently in-editor.
            const uint32 Hash = GetTypeHash(InitialFactionTag.GetTagName());
            const float Hue = static_cast<float>(Hash % 360);
            Tint = FLinearColor::MakeFromHSV8(static_cast<uint8>(Hue / 360.0f * 255.0f), 200, 255);
            Tint.A = 0.35f;
        }
        UMaterialInstanceDynamic *MID = UMaterialInstanceDynamic::Create(PreviewCellMaterial, this);
        if (MID) {
            MID->SetVectorParameterValue(TEXT("Color"), Tint);
            MID->SetVectorParameterValue(TEXT("BaseColor"), Tint); // redundant-safety, mirrors the debug actor
            SpawnCellsISM->SetMaterial(0, MID);
        }
    }
}

void AMythicSettlement::BeginPlay() {
    Super::BeginPlay();

    // Populate runtime data from editor-configured properties
    SettlementData.DisplayName = SettlementName;
    SettlementData.MaxPopulationDensity = MaxPopulationDensity;
    SettlementData.SettlementTag = SettlementTag;
    SettlementData.bIsCapital = bIsCapital;
    SettlementData.Economy = Economy;
    SettlementData.bIsHostileCamp = bIsHostileCamp;

    // Register with the Living World Subsystem
    if (UGameInstance *GI = GetGameInstance()) {
        if (UMythicLivingWorldSubsystem *LWS = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
            // Resolve faction tag to runtime ID
            if (InitialFactionTag.IsValid()) {
                FMythicFactionId ResolvedId;
                if (const UMythicFactionDatabase *FactionDB = LWS->GetFactionDatabase()) {
                    ResolvedId = FactionDB->FindFactionId(InitialFactionTag);
                }

                if (ResolvedId.IsValid()) {
                    SettlementData.GoverningFaction = ResolvedId;
                }
                else {
                    UE_LOG(LogMythSettlement, Warning, TEXT("Settlement '%s' could not resolve faction tag '%s' — no matching faction in database."),
                           *SettlementName.ToString(), *InitialFactionTag.ToString());
                }
            }

            // Rasterize spline boundary into territory cells
            if (const UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid()) {
                RasterizeSplineToCells(Grid);

                // Generate navmesh-validated tagged spawn anchors from the just-rasterized cells, BEFORE registering so
                // the registry's by-value settlement copy carries them. Navmesh-ready at BeginPlay on the server; if it
                // isn't (or a cell is fully built-over), GenerateSpawnPoints simply produces fewer/no points and the
                // population spawner falls back to the cell-center placement path for those cells (coexistence).
                GenerateSpawnPoints(Grid);
            }

            LWS->RegisterSettlement(this);
        }
        else {
            UE_LOG(LogMythSettlement, Warning, TEXT("Settlement '%s' could not find Living World Subsystem."),
                   *SettlementName.ToString());
        }
    }
}

void AMythicSettlement::RasterizeSplineToCells(const UMythicTerritoryGrid *TerritoryGrid) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicSettlement_RasterizeSpline);

    if (!BoundarySpline || !TerritoryGrid) {
        UE_LOG(LogMythSettlement, Error, TEXT("Cannot rasterize: missing spline or territory grid."));
        return;
    }

    // Pull the grid's coordinate basis and rasterize via the SHARED static helper so this runtime result is
    // bit-identical to the edit-time spawn-cell preview (both call RasterizeSplineCells with the same four params).
    // CellToWorld((0,0)) = WorldOrigin + 0.5*CellSize, so back out WorldOrigin by subtracting half a cell.
    const float GridCellSize = TerritoryGrid->GetCellSize();
    const FVector OriginCellCenter = TerritoryGrid->CellToWorld(FMythicCellCoord(0, 0));
    const FVector2D GridWorldOrigin(OriginCellCenter.X - 0.5 * GridCellSize, OriginCellCenter.Y - 0.5 * GridCellSize);
    RasterizeSplineCells(BoundarySpline, GridCellSize, GridWorldOrigin,
                         TerritoryGrid->GetWidth(), TerritoryGrid->GetHeight(), SettlementData.RasterizedCells);

    // Set the center cell now that the rasterized cells are known — consumed by the Socialize gather-point (AIController).
    // (Was never assigned → defaulted to (0,0), sending socializing NPCs to the grid's origin corner.)
    SettlementData.CenterCell = ComputeCenterCell(SettlementData.RasterizedCells);

    UE_LOG(LogMythSettlement, Log, TEXT("Settlement '%s' rasterized to %d cells (center %d,%d)."),
           *SettlementName.ToString(), SettlementData.RasterizedCells.Num(),
           SettlementData.CenterCell.X, SettlementData.CenterCell.Y);
}

EMythicSpawnPointPurpose AMythicSettlement::DerivePurpose(const FMythicCellCoord &Cell, int32 Index, bool bHostile) {
    // A hostile camp is uniformly enemy-occupied — every anchor hosts a bandit/raider.
    if (bHostile) {
        return EMythicSpawnPointPurpose::Enemy;
    }

    // Deterministic per-(cell,index) roll → a fixed minority of Guard points, the rest Civilian. Salt mixes the cell
    // hash with 'SPNT' XOR the index so two points in the SAME cell get distinct rolls (no all-guard / all-civilian
    // cells). Masked to 24 bits / 2^24 for a clean [0,1) fraction — same idiom as DeriveArchetype.
    const uint32 Seed = HashCombine(GetTypeHash(Cell), 0x53504E54u ^ static_cast<uint32>(Index));
    const float Roll = static_cast<float>(Seed & 0xFFFFFFu) / 16777216.0f;
    constexpr float GuardShare = 0.25f;
    return (Roll < GuardShare) ? EMythicSpawnPointPurpose::Guard : EMythicSpawnPointPurpose::Civilian;
}

void AMythicSettlement::GenerateSpawnPoints(const UMythicTerritoryGrid *Grid) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicSettlement_GenerateSpawnPoints);

    SettlementData.SpawnPoints.Reset();

    UWorld *World = GetWorld();
    if (!World || !Grid) {
        return;
    }

    // One-time generation: per covered cell, roll SpawnPointsPerCell navmesh-valid anchors near the cell center. The
    // scatter radius is half the cell so points spread within the cell footprint (and FindValidSpawn's reachable roll
    // keeps them on the same nav island as the projected anchor). Humanoid placement (water-incapable) — the per-archetype
    // water flag is consumed at the embodiment fast-path, not at generation; a town anchor is always a land anchor.
    const int32 PerCell = FMath::Max(1, SpawnPointsPerCell);
    const float CellSize = Grid->GetCellSize();

    SettlementData.SpawnPoints.Reserve(SettlementData.RasterizedCells.Num() * PerCell);

    for (const FMythicCellCoord &Cell : SettlementData.RasterizedCells) {
        const FVector CellCenter = Grid->CellToWorld(Cell);

        for (int32 Index = 0; Index < PerCell; ++Index) {
            FMythicPlacementParams Params;
            Params.CellCenterXY = CellCenter;
            Params.ScatterRadius = CellSize * 0.5f;
            // CapsuleRadius / CapsuleHalfHeight / NavExtent / RetryBudget keep their humanoid placement-defaults; the
            // anchor only needs to be reachable + non-overlapping at generation time (the embodiment fast-path re-tests
            // occupancy with the REAL resolved-class capsule before use).

            FTransform OutTM;
            if (!MythicPlacement::FindValidSpawn(World, Params, OutTM)) {
                // Couldn't place this anchor (navmesh not ready / fully built-over) — skip it; the population spawner
                // falls back to the cell-center path for any cell short of matching points.
                continue;
            }

            FMythicSpawnPoint Point;
            Point.WorldLocation = OutTM.GetLocation();
            Point.Cell = Cell;
            Point.Purpose = DerivePurpose(Cell, Index, bIsHostileCamp);
            SettlementData.SpawnPoints.Add(Point);
        }
    }

    UE_LOG(LogMythSettlement, Log, TEXT("Settlement '%s' generated %d spawn point(s) across %d cell(s)%s."),
           *SettlementName.ToString(), SettlementData.SpawnPoints.Num(), SettlementData.RasterizedCells.Num(),
           bIsHostileCamp ? TEXT(" [HOSTILE CAMP]") : TEXT(""));
}

void AMythicSettlement::RasterizeSplineCells(const USplineComponent *Spline, float CellWorldSize, FVector2D WorldOrigin,
                                             int32 GridWidth, int32 GridHeight, TArray<FMythicCellCoord> &OutCells) {
    OutCells.Reset();

    if (!Spline || CellWorldSize <= 0.0f || GridWidth <= 0 || GridHeight <= 0) {
        return;
    }

    // Sample spline into a 2D polygon for point-in-polygon testing.
    const int32 NumSplinePoints = Spline->GetNumberOfSplinePoints();
    if (NumSplinePoints < 3) {
        return; // not a polygon — no cells (matches the runtime early-out)
    }

    // Sample the spline at regular intervals to build a polygon (every 100cm along the spline for accuracy).
    const float SplineLength = Spline->GetSplineLength();
    const float SampleInterval = 100.0f;
    const int32 NumSamples = FMath::Max(NumSplinePoints, FMath::CeilToInt(SplineLength / SampleInterval));

    TArray<FVector2D> SplinePolygon;
    SplinePolygon.Reserve(NumSamples);

    for (int32 i = 0; i < NumSamples; ++i) {
        const float Distance = (static_cast<float>(i) / NumSamples) * SplineLength;
        const FVector WorldPos = Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
        SplinePolygon.Emplace(WorldPos.X, WorldPos.Y);
    }

    // Compute axis-aligned bounding box of the polygon.
    FVector2D PolygonMin(TNumericLimits<double>::Max());
    FVector2D PolygonMax(TNumericLimits<double>::Lowest());

    for (const FVector2D &Point : SplinePolygon) {
        PolygonMin.X = FMath::Min(PolygonMin.X, Point.X);
        PolygonMin.Y = FMath::Min(PolygonMin.Y, Point.Y);
        PolygonMax.X = FMath::Max(PolygonMax.X, Point.X);
        PolygonMax.Y = FMath::Max(PolygonMax.Y, Point.Y);
    }

    // Local replicas of UMythicTerritoryGrid::WorldToCell / CellToWorld (float casts + bounds clamp, identical math).
    auto WorldToCellX = [&](double X) {
        return FMath::Clamp(FMath::FloorToInt((static_cast<float>(X) - static_cast<float>(WorldOrigin.X)) / CellWorldSize), 0, GridWidth - 1);
    };
    auto WorldToCellY = [&](double Y) {
        return FMath::Clamp(FMath::FloorToInt((static_cast<float>(Y) - static_cast<float>(WorldOrigin.Y)) / CellWorldSize), 0, GridHeight - 1);
    };

    const int32 MinCellX = WorldToCellX(PolygonMin.X);
    const int32 MinCellY = WorldToCellY(PolygonMin.Y);
    const int32 MaxCellX = WorldToCellX(PolygonMax.X);
    const int32 MaxCellY = WorldToCellY(PolygonMax.Y);

    // Test each cell center within the bounding box.
    for (int32 Y = MinCellY; Y <= MaxCellY; ++Y) {
        for (int32 X = MinCellX; X <= MaxCellX; ++X) {
            if (X < 0 || X >= GridWidth || Y < 0 || Y >= GridHeight) {
                continue; // IsValidCoord
            }
            // CellToWorld (cell CENTER) — the exact point the runtime rasterizer + the NPC spawn target use.
            const float CenterX = static_cast<float>(WorldOrigin.X) + (static_cast<float>(X) + 0.5f) * CellWorldSize;
            const float CenterY = static_cast<float>(WorldOrigin.Y) + (static_cast<float>(Y) + 0.5f) * CellWorldSize;
            if (IsPointInsideSpline(FVector2D(CenterX, CenterY), SplinePolygon)) {
                OutCells.Add(FMythicCellCoord(X, Y));
            }
        }
    }
}

FMythicCellCoord AMythicSettlement::ComputeCenterCell(const TArray<FMythicCellCoord> &Cells) {
    if (Cells.Num() == 0) {
        return FMythicCellCoord(0, 0); // degenerate (MinSettlementCells should prevent it) — matches the default
    }
    // Centroid of the rasterized cells (int64 sums avoid overflow at max grid 1024×1024).
    int64 SumX = 0;
    int64 SumY = 0;
    for (const FMythicCellCoord &C : Cells) {
        SumX += C.X;
        SumY += C.Y;
    }
    const double CenX = static_cast<double>(SumX) / Cells.Num();
    const double CenY = static_cast<double>(SumY) / Cells.Num();
    // Return the ACTUAL rasterized cell nearest the centroid (so it lies inside the settlement even for concave shapes).
    FMythicCellCoord Best = Cells[0];
    double BestDistSq = TNumericLimits<double>::Max();
    for (const FMythicCellCoord &C : Cells) {
        const double dx = static_cast<double>(C.X) - CenX;
        const double dy = static_cast<double>(C.Y) - CenY;
        const double DistSq = dx * dx + dy * dy;
        if (DistSq < BestDistSq) {
            BestDistSq = DistSq;
            Best = C;
        }
    }
    return Best;
}

void AMythicSettlement::TransferToFaction(FMythicFactionId NewFaction) {
    const FMythicFactionId OldFaction = SettlementData.GoverningFaction;
    SettlementData.GoverningFaction = NewFaction;

    UE_LOG(LogMythSettlement, Log, TEXT("Settlement '%s' transferred from faction %d to faction %d."),
           *SettlementData.DisplayName.ToString(), OldFaction.Index, NewFaction.Index);
}

bool AMythicSettlement::IsPointInsideSpline(const FVector2D &TestPoint, const TArray<FVector2D> &SplinePolygon) {
    // Ray casting algorithm (even-odd rule)
    const int32 NumVertices = SplinePolygon.Num();
    if (NumVertices < 3) {
        return false;
    }

    bool bInside = false;
    int32 J = NumVertices - 1;

    for (int32 I = 0; I < NumVertices; ++I) {
        const FVector2D &Vi = SplinePolygon[I];
        const FVector2D &Vj = SplinePolygon[J];

        if (((Vi.Y > TestPoint.Y) != (Vj.Y > TestPoint.Y)) &&
            (TestPoint.X < (Vj.X - Vi.X) * (TestPoint.Y - Vi.Y) / (Vj.Y - Vi.Y) + Vi.X)) {
            bInside = !bInside;
        }

        J = I;
    }

    return bInside;
}
