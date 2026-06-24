// Mythic Living World — Debug Visualization Implementation

#include "World/LivingWorld/Debugging/MythicLivingWorldDebugActor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Settlements/SettlementRegistry.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "Engine/GameInstance.h"

AMythicLivingWorldDebugActor::AMythicLivingWorldDebugActor() {
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.1f; // 10fps update is plenty for debug viz

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(TEXT("/Engine/BasicShapes/Cube"));
    if (MeshAsset.Succeeded()) {
        CellMesh = MeshAsset.Object;
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MatAsset(TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
    if (MatAsset.Succeeded()) {
        CellMaterial = MatAsset.Object;
    }
}

void AMythicLivingWorldDebugActor::BeginPlay() {
    Super::BeginPlay();

    if (UGameInstance *GI = GetGameInstance()) {
        LivingWorldSubsystem = GI->GetSubsystem<UMythicLivingWorldSubsystem>();
    }
}

void AMythicLivingWorldDebugActor::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);

    if (!LivingWorldSubsystem.IsValid()) {
        return;
    }

    const UMythicTerritoryGrid *Grid = LivingWorldSubsystem->GetTerritoryGrid();
    const UMythicFactionDatabase *FactionDB = LivingWorldSubsystem->GetFactionDatabase();
    const UMythicSettlementRegistry *SettlementRegistry = LivingWorldSubsystem->GetSettlementRegistry();

    FVector ViewLocation = FVector::ZeroVector;
    if (APlayerController *PC = UGameplayStatics::GetPlayerController(this, 0)) {
        if (APawn *Pawn = PC->GetPawn()) {
            ViewLocation = Pawn->GetActorLocation();
        }
        else {
            FVector CamLoc;
            FRotator CamRot;
            PC->GetPlayerViewPoint(CamLoc, CamRot);
            ViewLocation = CamLoc;
        }
    }

    if (Grid && FactionDB) {
        DrawTerritoryGrid(Grid, FactionDB, ViewLocation);
    }

    if (SettlementRegistry && FactionDB) {
        DrawSettlements(SettlementRegistry, FactionDB);
    }
}

void AMythicLivingWorldDebugActor::DrawTerritoryGrid(const UMythicTerritoryGrid *Grid, const UMythicFactionDatabase *FactionDB, const FVector &ViewLocation) {
    // Clear all existing instances
    for (auto &Pair : FactionISMs) {
        if (Pair.Value) {
            Pair.Value->ClearInstances();
        }
    }

    // Draw text labels only near the camera (culling is mandatory for text performance)
    const float DrawTextRadius = 5000.0f; // 50m radius for text
    const int32 Range = 15; // Increased range for ISM (15 * 5000 = 750m) which is reasonable for blocked visualization

    FMythicCellCoord CenterCell = Grid->WorldToCell(ViewLocation);
    int32 MinX = FMath::Max(0, CenterCell.X - Range);
    int32 MaxX = FMath::Min(Grid->GetWidth() - 1, CenterCell.X + Range);
    int32 MinY = FMath::Max(0, CenterCell.Y - Range);
    int32 MaxY = FMath::Min(Grid->GetHeight() - 1, CenterCell.Y + Range);

    for (int32 Y = MinY; Y <= MaxY; ++Y) {
        for (int32 X = MinX; X <= MaxX; ++X) {
            FMythicCellCoord Coord(X, Y);
            if (!Grid->IsValidCoord(Coord)) {
                continue;
            }

            FMythicTerritoryCell Cell = Grid->GetCell(Coord);
            if (!Cell.DominantFaction.IsValid()) {
                continue;
            }

            FVector CellCenter = Grid->CellToWorld(Coord);

            // Add Instance
            // Note: We use GetFactionColor to determine color from index, no need to lookup FactionData unless we filter by Alive
            FColor ScaleColor = GetFactionColor(Cell.DominantFaction.Index);
            UInstancedStaticMeshComponent *ISM = GetOrCreateFactionISM((int32)Cell.DominantFaction.Index, ScaleColor);

            if (ISM) {
                // Scale: CellSize is 5000. Cube is 100. XY Scale = 50.
                // Z Scale based on Influence (0..1 -> 0..5000 -> 0..50)
                float ZScale = (Cell.Influence * 50.0f) + 0.1f;
                FVector Scale(50.0f, 50.0f, ZScale);

                // Position: Center. Z is Scale * 50 (Half height)? No, Pivot is center.
                // We want base at 0. So Z = ZScale * 50.
                FVector Loc = CellCenter + FVector(0, 0, ZScale * 50.0f);

                ISM->AddInstance(FTransform(FRotator::ZeroRotator, Loc, Scale), true);
            }


            // Draw Debug String if close
            if (FVector::DistSquared(CellCenter, ViewLocation) < FMath::Square(DrawTextRadius)) {
                float Height = Cell.Influence * 5000.0f + 60.0f;
                DrawDebugString(GetWorld(), CellCenter + FVector(0, 0, Height),
                                FString::Printf(TEXT("(%d,%d)\nInf:%.2f"), X, Y, Cell.Influence),
                                nullptr, FColor::White, 0.0f, false, 1.5f);
            }
        }
    }

    // Batch update transforms if needed? No, AddInstance marks render state dirty automatically.
}

void AMythicLivingWorldDebugActor::DrawSettlements(const UMythicSettlementRegistry *Registry, const UMythicFactionDatabase *FactionDB) {
    // Iterate valid settlement IDs
    // Since Registry doesn't expose a clean iterator for external use easily without copy, 
    // we might need to assume a max ID or add an iterator API. 
    // For now, let's just cheat and iterate 0 to 1000 looking for valid ones if API allows, 
    // OR use GetSettlementData if we know IDs.

    // Actually, looking at Registry.h, `Settlements` is private. 
    // But `GetSettlementData(Id)` exists. 
    // We lack a way to get *all* IDs efficiently publicly?
    // Let's assume there's a getter we added or use `AMythicSettlement` actors in world if needed.
    // Wait, `LivingWorldSubsystem` owns `SettlementRegistry`.

    // Let's use `GetAllSettlements` if it exists (it probably doesn't based on previous read).
    // Alternative: Iterate actors `TActorIterator<AMythicSettlement>` and get their ID.
    // This is slower but fine for debug.

    for (TActorIterator<AMythicSettlement> It(GetWorld()); It; ++It) {
        AMythicSettlement *Settlement = *It;
        if (!Settlement) {
            continue;
        }

        const FMythicSettlementData &Data = Settlement->GetSettlementData();
        FVector Center = Settlement->GetActorLocation();

        FColor Color = FColor::White;
        if (Data.GoverningFaction.IsValid()) {
            Color = GetFactionColor(Data.GoverningFaction.Index);
        }

        DrawDebugSphere(GetWorld(), Center, 500.0f, 16, Color, false, -1.0f, 0, 10.0f);
        const FString DispName = Data.DisplayName.ToString();
        const int32 GovFaction = (int32)Data.GoverningFaction.Index;
        const int32 PopCount = (int32)Data.CurrentEntityCount;

        DrawDebugString(GetWorld(), Center + FVector(0, 0, 600.0f),
                        FString::Printf(TEXT("%s\nFaction: %d\nPop: %d"), *DispName, GovFaction, PopCount),
                        nullptr, Color, 0.11f, false, 3.0f);

        // Draw rasterized cells
        for (const FMythicCellCoord &Cell : Data.RasterizedCells) {
            // Note: This relies on Grid availability, but we don't have grid ref here easily without passing it.
            // Just skipping cell viz for settlement specific unless we want to look up world pos.
            // We already draw these in DrawTerritoryGrid if they are influential.
        }
    }
}

FColor AMythicLivingWorldDebugActor::GetFactionColor(uint8 FactionIndex) const {
    // Generate a deterministic color from the index
    if (FactionIndex == FMythicFactionId::InvalidIndex) {
        return FColor(50, 50, 50);
    }

    // Stable color generation based on index
    switch (FactionIndex % 6) {
    case 0:
        return FColor::Red;
    case 1:
        return FColor::Blue;
    case 2:
        return FColor::Green;
    case 3:
        return FColor::Orange;
    case 4:
        return FColor::Purple;
    case 5:
        return FColor::Cyan;
    default:
        return FColor::White;
    }
}

UInstancedStaticMeshComponent *AMythicLivingWorldDebugActor::GetOrCreateFactionISM(int32 FactionIndex, const FColor &Color) {
    if (TObjectPtr<UInstancedStaticMeshComponent> *FoundISM = FactionISMs.Find(FactionIndex)) {
        return *FoundISM;
    }

    // Create new ISM
    UInstancedStaticMeshComponent *NewISM = NewObject<UInstancedStaticMeshComponent>(this);
    NewISM->SetupAttachment(RootComponent);
    NewISM->RegisterComponent();

    if (CellMesh) {
        NewISM->SetStaticMesh(CellMesh);
    }

    if (CellMaterial) {
        UMaterialInstanceDynamic *MID = UMaterialInstanceDynamic::Create(CellMaterial, NewISM);
        MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(Color));
        MID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(Color)); // Redundant safety
        NewISM->SetMaterial(0, MID);
    }

    NewISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    NewISM->SetCastShadow(false);

    FactionISMs.Add(FactionIndex, NewISM);
    return NewISM;
}
