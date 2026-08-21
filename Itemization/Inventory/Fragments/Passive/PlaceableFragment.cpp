
#include "Itemization/Inventory/Fragments/Passive/PlaceableFragment.h"

float UPlaceableFragment::GetMinSurfaceNormalZ() const {
    const float ClampedDegrees = FMath::Clamp(MaxGroundSlopeDegrees, 0.0f, 90.0f);
    return FMath::Cos(FMath::DegreesToRadians(ClampedDegrees));
}

EPlaceablePlacementResult UPlaceableFragment::EvaluatePlacement(const FPlaceablePlacementQuery &Query) const {
    return EvaluatePlacement(Query, MaxPlacementReach, GetMinSurfaceNormalZ(), bRequireGroundSurface);
}

EPlaceablePlacementResult UPlaceableFragment::EvaluatePlacement(const FPlaceablePlacementQuery &Query,
                                                                const float MaxReach,
                                                                const float MinSurfaceNormalZ,
                                                                const bool bRequireGround) {
    if (bRequireGround && !Query.bDidHitSurface) {
        return EPlaceablePlacementResult::NoSurface;
    }

    if (Query.DistanceFromInstigator > MaxReach) {
        return EPlaceablePlacementResult::OutOfReach;
    }

    if (bRequireGround && Query.SurfaceNormalZ < MinSurfaceNormalZ) {
        return EPlaceablePlacementResult::SurfaceTooSteep;
    }

    if (Query.bHasBlockingOverlap) {
        return EPlaceablePlacementResult::Obstructed;
    }

    return EPlaceablePlacementResult::Valid;
}

EPlaceableDeployResult UPlaceableFragment::PlanDeploy(const bool bAuthorizedInventory,
                                                      const bool bSlotHasPlaceableItem,
                                                      const bool bHasDeployedClass,
                                                      const EPlaceablePlacementResult Placement) {
    if (!bAuthorizedInventory) {
        return EPlaceableDeployResult::NotAuthorized;
    }

    if (!bSlotHasPlaceableItem) {
        return EPlaceableDeployResult::SlotEmpty;
    }

    if (!bHasDeployedClass) {
        return EPlaceableDeployResult::NoDeployedClass;
    }

    if (Placement != EPlaceablePlacementResult::Valid) {
        return EPlaceableDeployResult::PlacementInvalid;
    }

    return EPlaceableDeployResult::Deployed;
}

FPlaceablePlacementQuery UPlaceableFragment::BuildPlacementQuery(const bool bDidHit,
                                                                const FVector &ImpactPoint,
                                                                const FVector &ImpactNormal,
                                                                const FVector &TraceEnd,
                                                                const FVector &InstigatorLocation,
                                                                const bool bHasBlockingOverlap) {
    FPlaceablePlacementQuery Query;
    Query.bDidHitSurface = bDidHit;

    const FVector CandidatePoint = bDidHit ? ImpactPoint : TraceEnd;

    Query.SurfaceNormalZ = bDidHit ? ImpactNormal.Z : 1.0f;

    Query.DistanceFromInstigator = FVector::Dist(InstigatorLocation, CandidatePoint);

    Query.bHasBlockingOverlap = bHasBlockingOverlap;
    return Query;
}

FPlaceablePreview UPlaceableFragment::DescribePlacement(const EPlaceablePlacementResult Result) {
    FPlaceablePreview Preview;
    switch (Result) {
    case EPlaceablePlacementResult::Valid:
        Preview.bCanConfirm = true;
        Preview.TintColor = FLinearColor::Green;
        Preview.Reason = FText::GetEmpty();
        break;
    case EPlaceablePlacementResult::NoSurface:
        Preview.bCanConfirm = false;
        Preview.TintColor = FLinearColor::Red;
        Preview.Reason = NSLOCTEXT("Placeable", "PlacementNoSurface", "No surface to place on");
        break;
    case EPlaceablePlacementResult::OutOfReach:
        Preview.bCanConfirm = false;
        Preview.TintColor = FLinearColor::Red;
        Preview.Reason = NSLOCTEXT("Placeable", "PlacementOutOfReach", "Out of reach");
        break;
    case EPlaceablePlacementResult::SurfaceTooSteep:
        Preview.bCanConfirm = false;
        Preview.TintColor = FLinearColor::Red;
        Preview.Reason = NSLOCTEXT("Placeable", "PlacementTooSteep", "Surface too steep");
        break;
    case EPlaceablePlacementResult::Obstructed:
        Preview.bCanConfirm = false;
        Preview.TintColor = FLinearColor::Red;
        Preview.Reason = NSLOCTEXT("Placeable", "PlacementObstructed", "Blocked");
        break;
    default:
        Preview.bCanConfirm = false;
        Preview.TintColor = FLinearColor::Red;
        break;
    }
    return Preview;
}

FText UPlaceableFragment::DescribeDeployFailure(const EPlaceableDeployResult DeployResult,
                                                const EPlaceablePlacementResult PlacementResult) {
    switch (DeployResult) {
    case EPlaceableDeployResult::NotAuthorized:
        return NSLOCTEXT("Placeable", "DeployNotAuthorized", "You can't build here");
    case EPlaceableDeployResult::PlacementInvalid:
        return DescribePlacement(PlacementResult).Reason;
    case EPlaceableDeployResult::Deployed:
    case EPlaceableDeployResult::SlotEmpty:
    case EPlaceableDeployResult::NoDeployedClass:
    default:
        return FText::GetEmpty();
    }
}
