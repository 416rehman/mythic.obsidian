
#include "World/Digging/MythicDigSite.h"

#include "World/Digging/MythicDigSiteRules.h"

bool UMythicDigSiteRegistry::FindSiteById(int32 SiteId, FMythicDigSiteEntry &OutEntry) const {
    if (SiteId < 0) {
        return false;
    }
    for (const FMythicDigSiteEntry &Site : Sites) {
        if (Site.SiteId == SiteId) {
            OutEntry = Site;
            return true;
        }
    }
    return false;
}

bool UMythicDigSiteRegistry::FindSiteAtLocation(const FVector &DigLoc, FMythicDigSiteEntry &OutEntry) const {
    bool bFound = false;
    float BestDistSq = TNumericLimits<float>::Max();
    for (const FMythicDigSiteEntry &Site : Sites) {
        if (!MythicDigSite::IsAtDigSite(DigLoc, Site.Anchor, Site.ToleranceRadius)) {
            continue;
        }
        const float DistSq = FVector::DistSquared(DigLoc, Site.Anchor);
        if (DistSq < BestDistSq) {
            BestDistSq = DistSq;
            OutEntry = Site;
            bFound = true;
        }
    }
    return bFound;
}

void UMythicDigSiteRegistry::GetAllSiteIds(TSet<int32> &OutIds) const {
    OutIds.Reset();
    for (const FMythicDigSiteEntry &Site : Sites) {
        if (Site.SiteId >= 0) {
            OutIds.Add(Site.SiteId);
        }
    }
}
