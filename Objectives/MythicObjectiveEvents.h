
#pragma once

#include "CoreMinimal.h"

namespace MythicObjectiveEvents {
    FORCEINLINE bool ShouldEmitObjectiveEvent(bool bServerAuthoritativeForPlayer, bool bValidPayloadTag) {
        return bServerAuthoritativeForPlayer && bValidPayloadTag;
    }
}
