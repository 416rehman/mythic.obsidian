
#include "World/Fishing/MythicWaterQuery.h"

#include "Engine/World.h"
#include "Engine/HitResult.h"
#include "CollisionQueryParams.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Physics/PhysicalMaterialWithTags.h"
#include "World/Fishing/MythicTags_Fishing.h"

bool MythicWaterQuery::TraceWaterDown(UWorld *World, const FVector &From, float Depth, FHitResult &OutHit) {
    OutHit = FHitResult();
    if (!World) {
        return false;
    }

    const FVector Start = From + FVector(0.0f, 0.0f, 50.0f);
    const FVector End = From - FVector(0.0f, 0.0f, FMath::Max(1.0f, Depth));

    FCollisionQueryParams Params(SCENE_QUERY_STAT(MythicWaterQuery), true);
    Params.bReturnPhysicalMaterial = true;

    if (!World->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, Params)) {
        return false;
    }

    const UPhysicalMaterialWithTags *PhysMat = Cast<UPhysicalMaterialWithTags>(OutHit.PhysMaterial.Get());
    return PhysMat && PhysMat->Tags.HasTag(TAG_Surface_Water);
}

bool MythicWaterQuery::IsPointOverWater(UWorld *World, const FVector &Point, float Depth) {
    FHitResult Hit;
    return TraceWaterDown(World, Point, Depth, Hit);
}
