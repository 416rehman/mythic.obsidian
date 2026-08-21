
#pragma once

#include "CoreMinimal.h"

enum class EMythicGiftResult : uint8 {
    Success,
    Partial,
    NoRoom,
    Declined,
    Unavailable,
};

namespace MythicGift {
    FORCEINLINE EMythicGiftResult ClassifyGiftMove(int32 StacksBefore, int32 Moved) {
        if (Moved <= 0 || StacksBefore <= 0) {
            return EMythicGiftResult::NoRoom;
        }
        return (Moved >= StacksBefore) ? EMythicGiftResult::Success : EMythicGiftResult::Partial;
    }

    FORCEINLINE bool CanOfferGift(bool bRecipientValid, bool bDifferentPlayers, bool bInRange, bool bSourceHasTakeableItem) {
        return bRecipientValid && bDifferentPlayers && bInRange && bSourceHasTakeableItem;
    }

    FORCEINLINE bool CanCompleteGift(bool bHasPendingOffer, bool bAccepted, bool bGiverValid, bool bInRange, bool bOfferedItemStillPresent) {
        return bHasPendingOffer && bAccepted && bGiverValid && bInRange && bOfferedItemStillPresent;
    }

    FORCEINLINE int32 ComputeGiftQuantity(int32 Requested, int32 Available) {
        if (Available <= 0) {
            return 0;
        }
        if (Requested <= 0) {
            return Available;
        }
        return FMath::Min(Requested, Available);
    }
}
