#pragma once

#include "CoreMinimal.h"

namespace MythicPOIDiscovery {
    inline bool IsWithinDiscoveryRadius(float DistSq, float RadiusSq) {
        return RadiusSq > 0.0f && DistSq <= RadiusSq;
    }

    inline bool ShouldRegisterDiscovery(bool bHasAuthority, bool bIsPlayer, bool bAlreadyUnlocked) {
        return bHasAuthority && bIsPlayer && !bAlreadyUnlocked;
    }
}
