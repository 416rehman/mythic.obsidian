#pragma once

#include "CoreMinimal.h"
#include "Containers/Set.h"

namespace MythicDigSite {
    inline bool IsAtDigSite(const FVector &PlayerLoc, const FVector &SiteLoc, float ToleranceRadius) {
        return ToleranceRadius > 0.0f && FVector::DistSquared(PlayerLoc, SiteLoc) <= ToleranceRadius * ToleranceRadius;
    }

    inline bool ResolveDigSite(const TSet<int32> &AuthoredSiteIds, int32 SiteId) {
        return SiteId != INDEX_NONE && AuthoredSiteIds.Contains(SiteId);
    }

    inline bool ShouldYieldBuriedFind(bool bSiteExists, bool bAtSite, bool bAlreadyConsumed) {
        return bSiteExists && bAtSite && !bAlreadyConsumed;
    }
}
