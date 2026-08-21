
#include "Misc/AutomationTest.h"
#include "Player/MythicGift.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicGiftHandshakeTest,
    "Mythic.Player.GiftHandshake",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicGiftHandshakeTest::RunTest(const FString &Parameters) {
    TestTrue(TEXT("a valid in-range different-player offer of a takeable item is allowed"), MythicGift::CanOfferGift(true, true, true, true));
    TestFalse(TEXT("no recipient → no offer"), MythicGift::CanOfferGift(false, true, true, true));
    TestFalse(TEXT("can't gift to yourself"), MythicGift::CanOfferGift(true, false, true, true));
    TestFalse(TEXT("out of range → no offer"), MythicGift::CanOfferGift(true, true, false, true));
    TestFalse(TEXT("an empty / untakeable source slot → no offer"), MythicGift::CanOfferGift(true, true, true, false));

    TestTrue(TEXT("accept of a live in-range offer whose item is still present completes"), MythicGift::CanCompleteGift(true, true, true, true, true));
    TestFalse(TEXT("no pending offer → nothing to complete"), MythicGift::CanCompleteGift(false, true, true, true, true));
    TestFalse(TEXT("decline → no transfer"), MythicGift::CanCompleteGift(true, false, true, true, true));
    TestFalse(TEXT("giver disconnected → no transfer"), MythicGift::CanCompleteGift(true, true, false, true, true));
    TestFalse(TEXT("drifted out of range before accepting → no transfer"), MythicGift::CanCompleteGift(true, true, true, false, true));
    TestFalse(TEXT("giver moved/used the offered item → no transfer (no substitution)"), MythicGift::CanCompleteGift(true, true, true, true, false));

    TestTrue(TEXT("the whole stack moved → Success"), MythicGift::ClassifyGiftMove(5, 5) == EMythicGiftResult::Success);
    TestTrue(TEXT("some but not all moved → Partial"), MythicGift::ClassifyGiftMove(5, 3) == EMythicGiftResult::Partial);
    TestTrue(TEXT("nothing moved (recipient full) → NoRoom"), MythicGift::ClassifyGiftMove(5, 0) == EMythicGiftResult::NoRoom);
    TestTrue(TEXT("a single-item full move → Success"), MythicGift::ClassifyGiftMove(1, 1) == EMythicGiftResult::Success);
    TestTrue(TEXT("a degenerate empty offer → NoRoom (defensive)"), MythicGift::ClassifyGiftMove(0, 0) == EMythicGiftResult::NoRoom);
    TestTrue(TEXT("moved-clamped-above-before is still Success, not Partial"), MythicGift::ClassifyGiftMove(3, 4) == EMythicGiftResult::Success);

    TestEqual(TEXT("requested <= 0 means the whole stack (back-compat)"), MythicGift::ComputeGiftQuantity(0, 10), 10);
    TestEqual(TEXT("negative requested also means the whole stack"), MythicGift::ComputeGiftQuantity(-3, 10), 10);
    TestEqual(TEXT("a partial request under the stack is honored verbatim"), MythicGift::ComputeGiftQuantity(4, 10), 4);
    TestEqual(TEXT("requesting exactly the stack gives the whole stack"), MythicGift::ComputeGiftQuantity(10, 10), 10);
    TestEqual(TEXT("requesting MORE than the stack is clamped to the stack"), MythicGift::ComputeGiftQuantity(50, 10), 10);
    TestEqual(TEXT("gifting 1 of many is allowed"), MythicGift::ComputeGiftQuantity(1, 10), 1);
    TestEqual(TEXT("an empty stack yields 0 (nothing to give)"), MythicGift::ComputeGiftQuantity(5, 0), 0);
    TestEqual(TEXT("empty stack with 'gift all' still yields 0"), MythicGift::ComputeGiftQuantity(0, 0), 0);

    return true;
}
