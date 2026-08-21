
#include "Misc/AutomationTest.h"
#include "Subsystem/SaveSystem/MythicSaveGameSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPerPlayerSaveSlotTest,
    "Mythic.SaveSystem.PerPlayerSlot",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPerPlayerSaveSlotTest::RunTest(const FString &Parameters) {
    const FString Shared = FString(UMythicSaveGameSubsystem::DebugCharacterSlot);

    TestEqual(TEXT("empty id falls back to the shared debug slot"),
              UMythicSaveGameSubsystem::ResolvePerPlayerCharacterSlot(FString()), Shared);

    const FString SlotA = UMythicSaveGameSubsystem::ResolvePerPlayerCharacterSlot(TEXT("accountA"));
    const FString SlotB = UMythicSaveGameSubsystem::ResolvePerPlayerCharacterSlot(TEXT("accountB"));
    TestNotEqual(TEXT("a real id does NOT use the shared debug slot"), SlotA, Shared);

    TestNotEqual(TEXT("two different players resolve to two different slots"), SlotA, SlotB);

    TestEqual(TEXT("the same id is stable across calls"),
              UMythicSaveGameSubsystem::ResolvePerPlayerCharacterSlot(TEXT("accountA")), SlotA);

    return true;
}
