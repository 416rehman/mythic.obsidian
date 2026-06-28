// Mythic — co-op item GIFT handshake: the pure decision gates.
// A one-directional give: the giver offers an item to a teammate, who accepts/declines; on accept the server atomically
// moves the whole stack (reusing UMythicInventoryComponent::ServerQuickMoveToInventory, which is loss-safe). These pure
// gates are header-only FORCEINLINE so they unit-test without the controller/world (mirrors MythicObjectiveEvents).

#pragma once

#include "CoreMinimal.h"

// Outcome of a gift, used SERVER-SIDE to compose the giver/recipient beats (a plain enum — not reflected; the beat is sent
// as pre-composed text over a Client RPC). Success = the whole offered stack moved; Partial = some moved (recipient ran out
// of room); NoRoom = nothing moved; Declined = the recipient declined; Unavailable = the offer lapsed (item moved / range).
enum class EMythicGiftResult : uint8 {
    Success,
    Partial,
    NoRoom,
    Declined,
    Unavailable,
};

namespace MythicGift {

    // Pure: classify how a gift MOVE turned out from the stack count before vs how many actually moved. Drives the outcome
    // beats. <=0 moved → NoRoom; the whole stack → Success; some-but-not-all → Partial. Static + unit-testable.
    FORCEINLINE EMythicGiftResult ClassifyGiftMove(int32 StacksBefore, int32 Moved) {
        if (Moved <= 0 || StacksBefore <= 0) {
            return EMythicGiftResult::NoRoom;
        }
        return (Moved >= StacksBefore) ? EMythicGiftResult::Success : EMythicGiftResult::Partial;
    }

    // Can the giver OFFER a gift? The recipient is a valid, DIFFERENT player, within gifting range, and the source slot
    // holds a player-takeable item. (Authority + own-inventory are enforced at the RPC; these are the gameplay gates.)
    FORCEINLINE bool CanOfferGift(bool bRecipientValid, bool bDifferentPlayers, bool bInRange, bool bSourceHasTakeableItem) {
        return bRecipientValid && bDifferentPlayers && bInRange && bSourceHasTakeableItem;
    }

    // Can a pending offer COMPLETE when the recipient accepts? There IS a pending offer that was accepted; the giver is
    // still valid and in range; and the EXACT offered item instance is still in the source slot (guards the giver moving,
    // using, dropping, or swapping the item during the handshake — the recipient never gets a substituted item).
    FORCEINLINE bool CanCompleteGift(bool bHasPendingOffer, bool bAccepted, bool bGiverValid, bool bInRange, bool bOfferedItemStillPresent) {
        return bHasPendingOffer && bAccepted && bGiverValid && bInRange && bOfferedItemStillPresent;
    }
}
