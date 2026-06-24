// Mythic — Placeable item fragment implementation

#include "Itemization/Inventory/Fragments/Passive/PlaceableFragment.h"

float UPlaceableFragment::GetMinSurfaceNormalZ() const {
    // cos(slope): a surface whose normal Z is at least this is within the allowed tilt (flat ground = 1.0). Clamp
    // the authored angle defensively so a mis-set value outside [0,90] can't produce a nonsensical threshold.
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
    // A ground-requiring placeable aimed at empty space has nowhere to sit.
    if (bRequireGround && !Query.bDidHitSurface) {
        return EPlaceablePlacementResult::NoSurface;
    }

    // Reach is checked for every placeable (ground or floating): you can't deploy beyond arm's length. Inclusive —
    // a spot exactly at MaxReach is allowed.
    if (Query.DistanceFromInstigator > MaxReach) {
        return EPlaceablePlacementResult::OutOfReach;
    }

    // Slope only constrains ground placeables; a flatter surface has a larger normal Z. Inclusive — a surface
    // exactly at the slope limit is allowed.
    if (bRequireGround && Query.SurfaceNormalZ < MinSurfaceNormalZ) {
        return EPlaceablePlacementResult::SurfaceTooSteep;
    }

    // Finally, the spot must be physically clear.
    if (Query.bHasBlockingOverlap) {
        return EPlaceablePlacementResult::Obstructed;
    }

    return EPlaceablePlacementResult::Valid;
}

EPlaceableDeployResult UPlaceableFragment::PlanDeploy(const bool bAuthorizedInventory,
                                                      const bool bSlotHasPlaceableItem,
                                                      const bool bHasDeployedClass,
                                                      const EPlaceablePlacementResult Placement) {
    // Authority/ownership FIRST: a request from a client that doesn't own (or have open+in-range) the inventory is
    // rejected before we even look at what's in the slot, so a forged request can't probe another player's slots.
    if (!bAuthorizedInventory) {
        return EPlaceableDeployResult::NotAuthorized;
    }

    // The slot must actually hold a deployable item.
    if (!bSlotHasPlaceableItem) {
        return EPlaceableDeployResult::SlotEmpty;
    }

    // The placeable must point at something to spawn (a content/authoring error otherwise).
    if (!bHasDeployedClass) {
        return EPlaceableDeployResult::NoDeployedClass;
    }

    // And the chosen spot must be a legal placement (same rule the client previewed).
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

    // The deployed object sits at the surface hit, or at the aim endpoint when nothing was hit (floating placeable).
    const FVector CandidatePoint = bDidHit ? ImpactPoint : TraceEnd;

    // Slope only matters when a surface was actually hit; a miss is treated as flat (the slope gate is skipped for
    // floating placeables, and a ground-required placeable rejects a miss before the slope check is reached).
    Query.SurfaceNormalZ = bDidHit ? ImpactNormal.Z : 1.0f;

    // Reach is measured to where the object would actually sit, not the raw trace length.
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
