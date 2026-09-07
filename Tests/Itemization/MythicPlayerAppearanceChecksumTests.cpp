// Appearance archive compatibility and exact original-file integrity checks.
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Subsystem/SaveSystem/MythicSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Misc/SecureHash.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/UnrealType.h"

namespace AppearanceChecksumProbe {
// Mirrors only the two fields absent from v23. This is a differential default-field
// test, not a substitute for the separate real-file test below.
class FWithoutAppearanceFields final : public FObjectAndNameAsStringProxyArchive {
public:
    explicit FWithoutAppearanceFields(FArchive& Inner)
        : FObjectAndNameAsStringProxyArchive(Inner, false) {}
    virtual bool ShouldSkipProperty(const FProperty* Property) const override {
        if (Property && Property->GetOwnerStruct()==FSerializedCharacterData::StaticStruct()
            && (Property->GetFName()==TEXT("AppearanceRecipe")
                || Property->GetFName()==TEXT("bHasAppearanceRecipe"))) return true;
        return FObjectAndNameAsStringProxyArchive::ShouldSkipProperty(Property);
    }
};
static TArray<uint8> SerializeForChecksum(UMythicSaveGame* Save, bool bSkipNewFields=false) {
    TArray<uint8> Bytes;
    FMemoryWriter Writer(Bytes); // Match production: nonpersistent, normal tagged delta.
    if (bSkipNewFields) {
        FWithoutAppearanceFields Archive(Writer); Save->Serialize(Archive);
    } else {
        FObjectAndNameAsStringProxyArchive Archive(Writer,false); Save->Serialize(Archive);
    }
    return Bytes;
}
static FString Digest(const TArray<uint8>& Bytes) {
    FSHAHash Hash;
    FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num(),Hash.Hash);
    return Hash.ToString();
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicAppearanceDefaultChecksum,
    "Mythic.Character.Appearance.DefaultFieldsChecksum",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FMythicAppearanceDefaultChecksum::RunTest(const FString&) {
    using namespace AppearanceChecksumProbe;
    auto* Save=NewObject<UMythicSaveGame>();
    Save->CharacterData.DataVersion=23;
    Save->CharacterData.CharacterID=TEXT("AppearanceChecksumFixture");
    Save->CharacterData.CharacterName=TEXT("Nondefault parent struct");
    Save->CharacterData.Inventories.AddDefaulted();
    Save->CharacterData.Inventories[0].StableContainerId=TEXT("FixtureInventory");
    Save->DataChecksum.Reset();
    const auto DefaultBytes=SerializeForChecksum(Save);
    TestTrue(TEXT("new absent recipe/false presence emit no tags against existing defaults"),
        DefaultBytes==SerializeForChecksum(Save,true));

    // In-flight evidence must not enter either checksum or persistent payload.
    auto& Stamp=Save->CharacterData.AppearanceStamp;
    Stamp.Route=TEXT("FixtureRoute"); Stamp.Session=FGuid::NewGuid(); Stamp.Revision=9;
    Stamp.Recipe.Frame=TEXT("Female");
    FMythicAppearanceItemStamp Item; Item.ItemGuid=FGuid::NewGuid(); Item.Revision=4;
    Stamp.Items.Add(Item); Save->CharacterData.Inventories[0].AppearanceStamps.Add(Item);
    TestTrue(TEXT("native capture stamps do not alter checksum bytes"),
        DefaultBytes==SerializeForChecksum(Save));

    Save->CharacterData.DataVersion=24;
    Save->CharacterData.bHasAppearanceRecipe=true;
    Save->CharacterData.AppearanceRecipe.Frame=TEXT("Female");
    Save->CharacterData.AppearanceRecipe.Head=TEXT("Adventurer");
    Save->CharacterData.AppearanceRecipe.Hair=TEXT("Bald");
    Save->DataChecksum=Digest(SerializeForChecksum(Save));
    const FString ExpectedChecksum=Save->DataChecksum;
    TArray<uint8> DiskBytes;
    if (!TestTrue(TEXT("real persistent save archive"),UGameplayStatics::SaveGameToMemory(Save,DiskBytes))) return false;
    auto* Loaded=Cast<UMythicSaveGame>(UGameplayStatics::LoadGameFromMemory(DiskBytes));
    if (!TestNotNull(TEXT("persistent archive rehydrates correct class"),Loaded)) return false;
    TestEqual(TEXT("stored checksum preserved"),Loaded->DataChecksum,ExpectedChecksum);
    Loaded->DataChecksum.Reset();
    TestEqual(TEXT("persistent roundtrip matches production checksum"),Digest(SerializeForChecksum(Loaded)),ExpectedChecksum);
    TestTrue(TEXT("personal recipe presence survives"),Loaded->CharacterData.bHasAppearanceRecipe);
    TestTrue(TEXT("complete nondefault recipe survives"),Loaded->CharacterData.AppearanceRecipe==Save->CharacterData.AppearanceRecipe);
    TestFalse(TEXT("native session stamp is not persisted"),Loaded->CharacterData.AppearanceStamp.Session.IsValid());
    TestTrue(TEXT("native item coverage stamps are not persisted"),Loaded->CharacterData.Inventories[0].AppearanceStamps.IsEmpty());
    TestTrue(TEXT("source in-flight stamp remains available"),Save->CharacterData.AppearanceStamp.Session==Stamp.Session && Stamp.Items.Num()==1);
    return true;
}

// Explicit opt-in: supply a known, untouched v23 character .sav, not a newly
// manufactured current-schema file. No player, world, restore or save operation.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicAppearanceRealLegacyChecksum,
    "Mythic.Character.Appearance.RealLegacyFileChecksum",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FMythicAppearanceRealLegacyChecksum::RunTest(const FString&) {
    using namespace AppearanceChecksumProbe;
    FString Path;
    FParse::Value(FCommandLine::Get(),TEXT("AppearanceLegacySave="),Path);
    if (Path.IsEmpty()) Path=FPlatformMisc::GetEnvironmentVariable(TEXT("MYTHIC_APPEARANCE_LEGACY_SAVE"));
    if (Path.IsEmpty()) {
        AddError(TEXT("Requires explicit -AppearanceLegacySave=<known pre-appearance character .sav>; no legacy proof was run."));
        return false;
    }
    TArray<uint8> Original;
    if (!TestTrue(TEXT("read explicitly selected original file"),FFileHelper::LoadFileToArray(Original,*Path))) return false;
    auto* Loaded=Cast<UMythicSaveGame>(UGameplayStatics::LoadGameFromMemory(Original));
    if (!TestNotNull(TEXT("old file uses canonical Mythic save class"),Loaded)) return false;
    if (!TestEqual(TEXT("actual pre-appearance schema"),Loaded->CharacterData.DataVersion,23)) return false;
    const FString Stored=Loaded->DataChecksum;
    if (!TestFalse(TEXT("old file already contains integrity digest"),Stored.IsEmpty())) return false;
    TestFalse(TEXT("absent old recipe remains absent before fixup"),Loaded->CharacterData.bHasAppearanceRecipe);
    TestTrue(TEXT("absent recipe retains all-None default"),Loaded->CharacterData.AppearanceRecipe==FMythicAppearanceRecipe{});
    Loaded->DataChecksum.Reset();
    const FString Actual=Digest(SerializeForChecksum(Loaded));
    TestTrue(TEXT("real old checksum remains valid BEFORE FixupData"),Actual.Equals(Stored,ESearchCase::IgnoreCase));
    Loaded->DataChecksum=Stored;
    Loaded->FixupData();
    TestFalse(TEXT("version fixup cannot invent recipe presence"),Loaded->CharacterData.bHasAppearanceRecipe);
    TArray<uint8> After;
    TestTrue(TEXT("original file still readable"),FFileHelper::LoadFileToArray(After,*Path));
    TestTrue(TEXT("read-only proof leaves every original byte intact"),After==Original);
    AddInfo(FString::Printf(TEXT("Legacy checksum input SHA1 %s; checksum equality %s"),*Digest(Original),Actual.Equals(Stored,ESearchCase::IgnoreCase)?TEXT("PASS"):TEXT("FAIL")));
    return true;
}
#endif
