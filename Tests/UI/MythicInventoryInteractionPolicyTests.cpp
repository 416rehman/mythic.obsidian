// Copyright Stellar Games. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "UI/Inventory/MythicInventoryInteractionPolicy.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicInventoryInteractionPolicyTest,
    "Mythic.UI.Inventory.InteractionPolicy",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicInventoryInteractionPolicyTest::RunTest(const FString &Parameters) {
    (void)Parameters;
    FMythicInventoryInteractionPolicyInput Full;
    Full.bEquippable = true;
    Full.bPrimaryEnabled = true;
    Full.bMoveEnabled = true;
    Full.bCanToggleManualJunk = true;
    Full.bCanDrop = true;
    Full.Quantity = 5;
    Full.Rarity = EItemRarity::Rare;

    const TArray<FMythicInventoryContextAction> FullActions =
        FMythicInventoryInteractionPolicy::BuildContextActions(Full);
    if (TestEqual(TEXT("the shallow context never exceeds five rows"), FullActions.Num(), 5)) {
        TestEqual(TEXT("primary is first"), FullActions[0].Verb,
                  EMythicInventoryContextVerb::Equip);
        TestEqual(TEXT("split is second"), FullActions[1].Verb,
                  EMythicInventoryContextVerb::Split);
        TestEqual(TEXT("move is third"), FullActions[2].Verb,
                  EMythicInventoryContextVerb::Move);
        TestEqual(TEXT("manual junk is fourth"), FullActions[3].Verb,
                  EMythicInventoryContextVerb::ToggleManualJunk);
        TestEqual(TEXT("drop is fifth"), FullActions[4].Verb,
                  EMythicInventoryContextVerb::Drop);
        TestEqual(TEXT("split cannot consume the complete stack"),
                  FullActions[1].MaximumQuantity, 4);
        TestTrue(TEXT("rare unmarked drops use held confirmation"),
                 FullActions[4].bRequiresHold);
    }

    FMythicInventoryInteractionPolicyInput Pending = Full;
    Pending.bMutationPending = true;
    const TArray<FMythicInventoryContextAction> PendingActions =
        FMythicInventoryInteractionPolicy::BuildContextActions(Pending);
    TestEqual(TEXT("pending state preserves stable row positions"),
              PendingActions.Num(), FullActions.Num());
    for (const FMythicInventoryContextAction &Action : PendingActions) {
        TestFalse(TEXT("pending state disables every mutation"), Action.bEnabled);
    }

    FMythicInventoryInteractionPolicyInput Equipped;
    Equipped.bIsEquipped = true;
    Equipped.bPrimaryEnabled = true;
    Equipped.bMoveEnabled = true;
    Equipped.Quantity = 1;
    const TArray<FMythicInventoryContextAction> EquippedActions =
        FMythicInventoryInteractionPolicy::BuildContextActions(Equipped);
    if (TestEqual(TEXT("equipped items omit split, junk, and drop"), EquippedActions.Num(), 2)) {
        TestEqual(TEXT("equipped primary is unequip"), EquippedActions[0].Verb,
                  EMythicInventoryContextVerb::Unequip);
    }

    FMythicInventoryInteractionPolicyInput Usable;
    Usable.bUsable = true;
    Usable.bPrimaryEnabled = true;
    Usable.bMoveRelevant = false;
    Usable.Quantity = 1;
    const TArray<FMythicInventoryContextAction> UsableActions =
        FMythicInventoryInteractionPolicy::BuildContextActions(Usable);
    if (TestEqual(TEXT("a consumable gets one direct primary action"), UsableActions.Num(), 1)) {
        TestEqual(TEXT("consumable primary is use"), UsableActions[0].Verb,
                  EMythicInventoryContextVerb::Use);
    }

    Full.bManualJunk = true;
    const TArray<FMythicInventoryContextAction> ManuallyJunkedActions =
        FMythicInventoryInteractionPolicy::BuildContextActions(Full);
    const FMythicInventoryContextAction *JunkAction =
        ManuallyJunkedActions.FindByPredicate([](const FMythicInventoryContextAction &Action) {
            return Action.Verb == EMythicInventoryContextVerb::ToggleManualJunk;
        });
    TestNotNull(TEXT("manual junk remains independently toggleable"), JunkAction);
    if (JunkAction) {
        TestEqual(TEXT("manual junk toggles to an explicit unmark label"),
                  JunkAction->Label.ToString(), FString(TEXT("Unmark Junk")));
    }

    return true;
}
