// Copyright Stellar Games. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Itemization/Inventory/MythicInventoryActionTypes.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Player/MythicPlayerController.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicInventoryActionLocatorContractTest,
    "Mythic.Itemization.Inventory.Actions.LocatorContract",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicInventoryActionLocatorContractTest::RunTest(
    const FString &Parameters) {
    UMythicInventoryComponent *Inventory =
        NewObject<UMythicInventoryComponent>(GetTransientPackage());
    if (!TestNotNull(TEXT("the locator fixture inventory exists"), Inventory)) {
        return false;
    }

    FMythicInventorySourceLocator Source;
    TestFalse(TEXT("a default source locator fails closed"),
              Source.IsStructurallyValid());
    Source.Inventory = Inventory;
    Source.SlotIndex = 7;
    Source.ExpectedItemGuid = FGuid::NewGuid();
    Source.ExpectedQuantity = 3;
    TestTrue(TEXT("a source requires inventory, slot, identity, and quantity"),
             Source.IsStructurallyValid());
    Source.ExpectedQuantity = 0;
    TestFalse(TEXT("a zero-quantity source is structurally invalid"),
              Source.IsStructurallyValid());

    FMythicInventoryTargetLocator EmptyTarget;
    EmptyTarget.Inventory = Inventory;
    EmptyTarget.SlotIndex = 8;
    TestTrue(TEXT("an explicitly empty target carries no occupant identity"),
             EmptyTarget.IsStructurallyValid());
    EmptyTarget.ExpectedOccupantQuantity = 1;
    TestFalse(TEXT("an empty target cannot smuggle an occupant quantity"),
              EmptyTarget.IsStructurallyValid());

    FMythicInventoryTargetLocator OccupiedTarget;
    OccupiedTarget.Inventory = Inventory;
    OccupiedTarget.SlotIndex = 9;
    OccupiedTarget.bExpectEmpty = false;
    OccupiedTarget.ExpectedOccupantGuid = FGuid::NewGuid();
    OccupiedTarget.ExpectedOccupantQuantity = 12;
    TestTrue(TEXT("an occupied target requires exact occupant identity"),
             OccupiedTarget.IsStructurallyValid());
    OccupiedTarget.ExpectedOccupantGuid.Invalidate();
    TestFalse(TEXT("an occupied target without a GUID fails closed"),
              OccupiedTarget.IsStructurallyValid());

    FMythicInventoryActionReceipt Receipt;
    TestFalse(TEXT("the default rejected receipt is not successful"),
              Receipt.WasSuccessful());
    Receipt.Result = EMythicInventoryActionResult::Succeeded;
    TestTrue(TEXT("receipt success is derived from its semantic result"),
             Receipt.WasSuccessful());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicInventoryActionRpcContractTest,
    "Mythic.Itemization.Inventory.Actions.RpcContract",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicInventoryActionRpcContractTest::RunTest(
    const FString &Parameters) {
    const UClass *ControllerClass = AMythicPlayerController::StaticClass();
    if (!TestNotNull(TEXT("the player controller class is reflected"),
                     ControllerClass)) {
        return false;
    }

    const FName ServerRequests[] = {
        GET_FUNCTION_NAME_CHECKED(AMythicPlayerController,
                                  ServerRequestInventoryMove),
        GET_FUNCTION_NAME_CHECKED(AMythicPlayerController,
                                  ServerRequestInventorySplit),
        GET_FUNCTION_NAME_CHECKED(AMythicPlayerController,
                                  ServerRequestInventoryDropQuantity),
        GET_FUNCTION_NAME_CHECKED(AMythicPlayerController,
                                  ServerRequestInventoryUse),
        GET_FUNCTION_NAME_CHECKED(AMythicPlayerController,
                                  ServerRequestInventorySetJunk),
        GET_FUNCTION_NAME_CHECKED(AMythicPlayerController,
                                  ServerRequestInventorySort),
    };
    for (const FName FunctionName : ServerRequests) {
        const UFunction *Function =
            ControllerClass->FindFunctionByName(FunctionName);
        if (!TestNotNull(*FString::Printf(TEXT("%s is reflected"),
                                         *FunctionName.ToString()),
                         Function)) {
            continue;
        }
        TestTrue(*FString::Printf(TEXT("%s is a reliable server RPC"),
                                 *FunctionName.ToString()),
                 Function->HasAllFunctionFlags(
                     FUNC_Net | FUNC_NetServer | FUNC_NetReliable));
        TestFalse(*FString::Printf(TEXT("%s uses semantic validation"),
                                  *FunctionName.ToString()),
                  Function->HasAnyFunctionFlags(FUNC_NetValidate));
    }

    const UFunction *ClientReceipt = ControllerClass->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(AMythicPlayerController,
                                  ClientReceiveInventoryActionReceipt));
    if (TestNotNull(TEXT("the owning-client receipt RPC is reflected"),
                    ClientReceipt)) {
        TestTrue(TEXT("the owning-client receipt is reliable"),
                 ClientReceipt->HasAllFunctionFlags(
                     FUNC_Net | FUNC_NetClient | FUNC_NetReliable));
    }

    TestNotNull(
        TEXT("the owning-client receipt delegate is Blueprint assignable"),
        FindFProperty<FMulticastDelegateProperty>(
            ControllerClass,
            GET_MEMBER_NAME_CHECKED(AMythicPlayerController,
                                    OnInventoryActionReceiptReceived)));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
