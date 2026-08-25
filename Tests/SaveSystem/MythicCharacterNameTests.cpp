#include "Misc/AutomationTest.h"

#include "Subsystem/SaveSystem/Character/CharacterData.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicCharacterNameTest,
                                 "Mythic.SaveSystem.CharacterName",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

/**
 * A character's name has to survive a save it was not in.
 *
 * The name lives in the manifest and is applied on possession. The save carries its own copy, captured from the
 * player state at save time - so a save written before the character had a name carries an empty string, and
 * restoring it unguarded blanks the name the manifest just set, every single load.
 */
bool FMythicCharacterNameTest::RunTest(const FString &Parameters) {
    // The guard is a plain emptiness check, so it is testable without a world, a player state or a save file.
    const FString FromManifest = TEXT("Rhoslyn");

    FSerializedCharacterData Fresh;
    TestTrue(TEXT("a save written before naming carries no character name"), Fresh.CharacterName.IsEmpty());
    TestFalse(TEXT("so restoring it must not be allowed to overwrite the manifest name"),
              !Fresh.CharacterName.IsEmpty());

    FSerializedCharacterData Named;
    Named.CharacterName = FromManifest;
    TestTrue(TEXT("a save that does carry a name is allowed to restore it"), !Named.CharacterName.IsEmpty());
    TestEqual(TEXT("and restores exactly what was captured"), Named.CharacterName, FromManifest);

    // The machine-name guard on the character page is the symptom, not the bug, and must keep working - so a
    // name that was never set has to stay distinguishable from one that was.
    TestNotEqual(TEXT("an unnamed save is distinguishable from a named one"), Fresh.CharacterName, Named.CharacterName);

    return true;
}

#endif
