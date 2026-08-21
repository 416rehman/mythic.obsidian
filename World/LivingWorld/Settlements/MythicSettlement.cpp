
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Spawn/MythicPlacement.h"
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

    SpawnCellsISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("SpawnCellsISM"));
    SpawnCellsISM->SetupAttachment(RootComponent);
    SpawnCellsISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SpawnCellsISM->SetCastShadow(false);
    SpawnCellsISM->SetHiddenInGame(true);
    SpawnCellsISM->bSelectable = false;
#if WITH_EDITOR
    SpawnCellsISM->SetIsVisualizationComponent(true);
#endif

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
    RebuildSpawnCellPreview();
}

#if WITH_EDITOR
void AMythicSettlement::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) {
    Super::PostEditChangeProperty(PropertyChangedEvent);
    RebuildSpawnCellPreview();
}
#endif

void AMythicSettlement::RebuildSpawnCellPreview() {
    if (!SpawnCellsISM) {
        return;
    }
    SpawnCellsISM->ClearInstances();

    if (!bShowSpawnCellsInEditor || !BoundarySpline || !PreviewCellMesh) {
        return;
    }

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

    TArray<FMythicCellCoord> Cells;
    RasterizeSplineCells(BoundarySpline, CellWorldSize, WorldOrigin, GridWidth, GridHeight, Cells);
    if (Cells.Num() == 0) {
        return;
    }

    const float XYScale = CellWorldSize / 100.0f;
    const FVector InstanceScale(XYScale, XYScale, 0.05f);
    const float LiftZ = 50.0f;

    for (const FMythicCellCoord &Cell : Cells) {
        const float CenterX = static_cast<float>(WorldOrigin.X) + (static_cast<float>(Cell.X) + 0.5f) * CellWorldSize;
        const float CenterY = static_cast<float>(WorldOrigin.Y) + (static_cast<float>(Cell.Y) + 0.5f) * CellWorldSize;
        const FVector WorldCenter(CenterX, CenterY, LiftZ);
        SpawnCellsISM->AddInstance(FTransform(FRotator::ZeroRotator, WorldCenter, InstanceScale), true);
    }

    if (PreviewCellMaterial) {
        FLinearColor Tint(0.1f, 0.6f, 1.0f, 0.35f);
        if (InitialFactionTag.IsValid()) {
            const uint32 Hash = GetTypeHash(InitialFactionTag.GetTagName());
            const float Hue = static_cast<float>(Hash % 360);
            Tint = FLinearColor::MakeFromHSV8(static_cast<uint8>(Hue / 360.0f * 255.0f), 200, 255);
            Tint.A = 0.35f;
        }
        UMaterialInstanceDynamic *MID = UMaterialInstanceDynamic::Create(PreviewCellMaterial, this);
        if (MID) {
            MID->SetVectorParameterValue(TEXT("Color"), Tint);
            MID->SetVectorParameterValue(TEXT("BaseColor"), Tint);
            SpawnCellsISM->SetMaterial(0, MID);
        }
    }
}

void AMythicSettlement::BeginPlay() {
    Super::BeginPlay();

    SettlementData.DisplayName = SettlementName;
    SettlementData.MaxPopulationDensity = MaxPopulationDensity;
    SettlementData.SettlementTag = SettlementTag;
    SettlementData.bIsCapital = bIsCapital;
    SettlementData.Economy = Economy;
    SettlementData.bIsHostileCamp = bIsHostileCamp;

    if (UGameInstance *GI = GetGameInstance()) {
        if (UMythicLivingWorldSubsystem *LWS = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
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

            if (const UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid()) {
                RasterizeSplineToCells(Grid);

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

    const float GridCellSize = TerritoryGrid->GetCellSize();
    const FVector OriginCellCenter = TerritoryGrid->CellToWorld(FMythicCellCoord(0, 0));
    const FVector2D GridWorldOrigin(OriginCellCenter.X - 0.5 * GridCellSize, OriginCellCenter.Y - 0.5 * GridCellSize);
    RasterizeSplineCells(BoundarySpline, GridCellSize, GridWorldOrigin,
                         TerritoryGrid->GetWidth(), TerritoryGrid->GetHeight(), SettlementData.RasterizedCells);

    SettlementData.CenterCell = ComputeCenterCell(SettlementData.RasterizedCells);

    UE_LOG(LogMythSettlement, Log, TEXT("Settlement '%s' rasterized to %d cells (center %d,%d)."),
           *SettlementName.ToString(), SettlementData.RasterizedCells.Num(),
           SettlementData.CenterCell.X, SettlementData.CenterCell.Y);
}

EMythicSpawnPointPurpose AMythicSettlement::DerivePurpose(const FMythicCellCoord &Cell, int32 Index, bool bHostile) {
    if (bHostile) {
        return EMythicSpawnPointPurpose::Enemy;
    }

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

    const int32 PerCell = FMath::Max(1, SpawnPointsPerCell);
    const float CellSize = Grid->GetCellSize();

    SettlementData.SpawnPoints.Reserve(SettlementData.RasterizedCells.Num() * PerCell);

    for (const FMythicCellCoord &Cell : SettlementData.RasterizedCells) {
        const FVector CellCenter = Grid->CellToWorld(Cell);

        for (int32 Index = 0; Index < PerCell; ++Index) {
            FMythicPlacementParams Params;
            Params.CellCenterXY = CellCenter;
            Params.ScatterRadius = CellSize * 0.5f;

            FTransform OutTM;
            if (!MythicPlacement::FindValidSpawn(World, Params, OutTM)) {
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

    const int32 NumSplinePoints = Spline->GetNumberOfSplinePoints();
    if (NumSplinePoints < 3) {
        return;
    }

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

    FVector2D PolygonMin(TNumericLimits<double>::Max());
    FVector2D PolygonMax(TNumericLimits<double>::Lowest());

    for (const FVector2D &Point : SplinePolygon) {
        PolygonMin.X = FMath::Min(PolygonMin.X, Point.X);
        PolygonMin.Y = FMath::Min(PolygonMin.Y, Point.Y);
        PolygonMax.X = FMath::Max(PolygonMax.X, Point.X);
        PolygonMax.Y = FMath::Max(PolygonMax.Y, Point.Y);
    }

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

    for (int32 Y = MinCellY; Y <= MaxCellY; ++Y) {
        for (int32 X = MinCellX; X <= MaxCellX; ++X) {
            if (X < 0 || X >= GridWidth || Y < 0 || Y >= GridHeight) {
                continue;
            }
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
        return FMythicCellCoord(0, 0);
    }
    int64 SumX = 0;
    int64 SumY = 0;
    for (const FMythicCellCoord &C : Cells) {
        SumX += C.X;
        SumY += C.Y;
    }
    const double CenX = static_cast<double>(SumX) / Cells.Num();
    const double CenY = static_cast<double>(SumY) / Cells.Num();
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
