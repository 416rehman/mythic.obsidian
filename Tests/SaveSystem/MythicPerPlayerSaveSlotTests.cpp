
#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/SecureHash.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Subsystem/SaveSystem/MythicSaveGame.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCreateNewCharacterStubTest,
    "Mythic.SaveSystem.CreateNewCharacterStub",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCreateNewCharacterStubTest::RunTest(const FString &Parameters) {
    const FString ExplicitID = TEXT("AutomationStubCharacter");
    const FString DisplayName = TEXT("StubHero");

    if (UGameplayStatics::DoesSaveGameExist(ExplicitID, 0)) {
        UGameplayStatics::DeleteGameInSlot(ExplicitID, 0);
    }

    // The subsystem's ClassWithin is GameInstance, so a bare transient-package outer trips an ensure.
    UGameInstance *OuterGI = NewObject<UGameInstance>(GetTransientPackage());
    UMythicSaveGameSubsystem *SaveSys = NewObject<UMythicSaveGameSubsystem>(OuterGI);
    if (!TestNotNull(TEXT("subsystem instance"), SaveSys)) {
        return false;
    }

    const FString ReturnedID = SaveSys->CreateNewCharacter(DisplayName, TEXT(""), false, ExplicitID);
    TestEqual(TEXT("explicit id is returned unchanged"), ReturnedID, ExplicitID);
    TestTrue(TEXT("stub save exists on disk"), UGameplayStatics::DoesSaveGameExist(ReturnedID, 0));

    UMythicSaveGame *Loaded = Cast<UMythicSaveGame>(UGameplayStatics::LoadGameFromSlot(ReturnedID, 0));
    if (TestNotNull(TEXT("stub save loads back"), Loaded)) {
        TestFalse(TEXT("stub carries a checksum"), Loaded->DataChecksum.IsEmpty());

        const FString StoredChecksum = Loaded->DataChecksum;
        Loaded->DataChecksum = TEXT("");
        TArray<uint8> TempBuffer;
        FMemoryWriter MemWriter(TempBuffer);
        FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
        Loaded->Serialize(Ar);
        FSHAHash Hash;
        FSHA1::HashBuffer(TempBuffer.GetData(), TempBuffer.Num(), Hash.Hash);
        TestTrue(TEXT("checksum validates against a re-serialize"),
                 Hash.ToString().Equals(StoredChecksum, ESearchCase::IgnoreCase));
        Loaded->DataChecksum = StoredChecksum;

        TestEqual(TEXT("display name round-trips as the character name"), Loaded->CharacterData.CharacterName, DisplayName);
        TestEqual(TEXT("stub is stamped with the current save version"),
                  Loaded->CharacterData.DataVersion, static_cast<int32>(CurrentCharacterSaveVersion));

        FString ValidationError;
        TestTrue(TEXT("stub passes character data validation"),
                 UMythicSaveGameSubsystem::ValidateCharacterData(Loaded->CharacterData, ValidationError));
    }

    bool bListed = false;
    for (const FMythicCharacterMetadata &Meta : SaveSys->GetCharacterList()) {
        if (Meta.CharacterID == ReturnedID) {
            bListed = true;
            TestEqual(TEXT("manifest keeps the display name"), Meta.DisplayName, DisplayName);
        }
    }
    TestTrue(TEXT("manifest lists the new character"), bListed);

    TestTrue(TEXT("delete reports success"), SaveSys->DeleteCharacter(ReturnedID));
    TestFalse(TEXT("save file is gone after delete"), UGameplayStatics::DoesSaveGameExist(ReturnedID, 0));

    bool bStillListed = false;
    for (const FMythicCharacterMetadata &Meta : SaveSys->GetCharacterList()) {
        if (Meta.CharacterID == ReturnedID) {
            bStillListed = true;
        }
    }
    TestFalse(TEXT("manifest entry is gone after delete"), bStillListed);

    return true;
}
