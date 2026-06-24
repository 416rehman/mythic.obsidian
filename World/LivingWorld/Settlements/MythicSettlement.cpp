// Mythic Living World — Settlement System Implementation
// Spline rasterization, registration with subsystem, zone detection.

#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "Components/SplineComponent.h"
#include "Engine/GameInstance.h"

AMythicSettlement::AMythicSettlement() {
    PrimaryActorTick.bCanEverTick = false;

    BoundarySpline = CreateDefaultSubobject<USplineComponent>(TEXT("BoundarySpline"));
    BoundarySpline->SetClosedLoop(true);
    RootComponent = BoundarySpline;
}

void AMythicSettlement::BeginPlay() {
    Super::BeginPlay();

    // Populate runtime data from editor-configured properties
    SettlementData.DisplayName = SettlementName;
    SettlementData.MaxPopulationDensity = MaxPopulationDensity;
    SettlementData.SettlementTag = SettlementTag;
    SettlementData.bIsCapital = bIsCapital;

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

    SettlementData.RasterizedCells.Reset();

    // Sample spline into a 2D polygon for point-in-polygon testing
    const int32 NumSplinePoints = BoundarySpline->GetNumberOfSplinePoints();
    if (NumSplinePoints < 3) {
        UE_LOG(LogMythSettlement, Warning, TEXT("Settlement '%s' spline has fewer than 3 points — cannot form a polygon."),
               *SettlementName.ToString());
        return;
    }

    // Sample the spline at regular intervals to build a polygon
    const float SplineLength = BoundarySpline->GetSplineLength();
    const float SampleInterval = 100.0f; // Sample every 100cm along the spline for accuracy
    const int32 NumSamples = FMath::Max(NumSplinePoints, FMath::CeilToInt(SplineLength / SampleInterval));

    TArray<FVector2D> SplinePolygon;
    SplinePolygon.Reserve(NumSamples);

    for (int32 i = 0; i < NumSamples; ++i) {
        const float Distance = (static_cast<float>(i) / NumSamples) * SplineLength;
        const FVector WorldPos = BoundarySpline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
        SplinePolygon.Emplace(WorldPos.X, WorldPos.Y);
    }

    // Compute axis-aligned bounding box of the polygon in cell space
    FVector2D PolygonMin(TNumericLimits<double>::Max());
    FVector2D PolygonMax(TNumericLimits<double>::Lowest());

    for (const FVector2D &Point : SplinePolygon) {
        PolygonMin.X = FMath::Min(PolygonMin.X, Point.X);
        PolygonMin.Y = FMath::Min(PolygonMin.Y, Point.Y);
        PolygonMax.X = FMath::Max(PolygonMax.X, Point.X);
        PolygonMax.Y = FMath::Max(PolygonMax.Y, Point.Y);
    }

    const FMythicCellCoord MinCell = TerritoryGrid->WorldToCell(FVector(PolygonMin.X, PolygonMin.Y, 0.0));
    const FMythicCellCoord MaxCell = TerritoryGrid->WorldToCell(FVector(PolygonMax.X, PolygonMax.Y, 0.0));

    // Test each cell center within the bounding box
    for (int32 Y = MinCell.Y; Y <= MaxCell.Y; ++Y) {
        for (int32 X = MinCell.X; X <= MaxCell.X; ++X) {
            const FMythicCellCoord CellCoord(X, Y);

            if (!TerritoryGrid->IsValidCoord(CellCoord)) {
                continue;
            }

            const FVector CellWorldPos = TerritoryGrid->CellToWorld(CellCoord);
            const FVector2D CellPoint(CellWorldPos.X, CellWorldPos.Y);

            if (IsPointInsideSpline(CellPoint, SplinePolygon)) {
                SettlementData.RasterizedCells.Add(CellCoord);
            }
        }
    }

    // Set the center cell now that the rasterized cells are known — consumed by the Socialize gather-point (AIController).
    // (Was never assigned → defaulted to (0,0), sending socializing NPCs to the grid's origin corner.)
    SettlementData.CenterCell = ComputeCenterCell(SettlementData.RasterizedCells);

    UE_LOG(LogMythSettlement, Log, TEXT("Settlement '%s' rasterized to %d cells from %d spline samples (center %d,%d)."),
           *SettlementName.ToString(), SettlementData.RasterizedCells.Num(), NumSamples,
           SettlementData.CenterCell.X, SettlementData.CenterCell.Y);
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

bool AMythicSettlement::IsPointInsideSpline(const FVector2D &TestPoint, const TArray<FVector2D> &SplinePolygon) const {
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
