// Mythic — co-op item gift handshake unit tests.
// Covers the pure decision gates the offer/accept RPCs are built on. The live RPC flow + the atomic transfer
// (ServerQuickMoveToInventory) are server-driven + PIE-verified; this locks the gate logic — in particular that an offer
// can only COMPLETE when the exact offered item is still present (no mid-handshake substitution).
// Run via: Session Frontend → Automation → Mythic.Player.GiftHandshake

#include "Misc/AutomationTest.h"
#include "Player/MythicGift.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicGiftHandshakeTest,
    "Mythic.Player.GiftHandshake",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicGiftHandshakeTest::RunTest(const FString &Parameters) {
    // ── CanOfferGift(recipientValid, differentPlayers, inRange, sourceHasTakeableItem) — all must hold ──
    TestTrue(TEXT("a valid in-range different-player offer of a takeable item is allowed"), MythicGift::CanOfferGift(true, true, true, true));
    TestFalse(TEXT("no recipient → no offer"), MythicGift::CanOfferGift(false, true, true, true));
    TestFalse(TEXT("can't gift to yourself"), MythicGift::CanOfferGift(true, false, true, true));
    TestFalse(TEXT("out of range → no offer"), MythicGift::CanOfferGift(true, true, false, true));
    TestFalse(TEXT("an empty / untakeable source slot → no offer"), MythicGift::CanOfferGift(true, true, true, false));

    // ── CanCompleteGift(hasPendingOffer, accepted, giverValid, inRange, offeredItemStillPresent) — all must hold ──
    TestTrue(TEXT("accept of a live in-range offer whose item is still present completes"), MythicGift::CanCompleteGift(true, true, true, true, true));
    TestFalse(TEXT("no pending offer → nothing to complete"), MythicGift::CanCompleteGift(false, true, true, true, true));
    TestFalse(TEXT("decline → no transfer"), MythicGift::CanCompleteGift(true, false, true, true, true));
    TestFalse(TEXT("giver disconnected → no transfer"), MythicGift::CanCompleteGift(true, true, false, true, true));
    TestFalse(TEXT("drifted out of range before accepting → no transfer"), MythicGift::CanCompleteGift(true, true, true, false, true));
    TestFalse(TEXT("giver moved/used the offered item → no transfer (no substitution)"), MythicGift::CanCompleteGift(true, true, true, true, false));

    return true;
}
