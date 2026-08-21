#pragma once

#include "CoreMinimal.h"
#include "Containers/Set.h"

namespace MythicFastTravel {
    inline bool CanFastTravelTo(const TSet<int32> &Unlocked, int32 DestId, bool bBlocked) {
        return DestId != INDEX_NONE && !bBlocked && Unlocked.Contains(DestId);
    }

    inline bool CanFastTravelBetween(const TSet<int32> &Unlocked, int32 SourceId, int32 DestId, bool bBlocked) {
        return CanFastTravelTo(Unlocked, DestId, bBlocked) && SourceId != INDEX_NONE && Unlocked.Contains(SourceId);
    }

    inline bool CanFastTravelWithCargo(bool bBetweenOk, bool bOverloaded) {
        return bBetweenOk && !bOverloaded;
    }
}
