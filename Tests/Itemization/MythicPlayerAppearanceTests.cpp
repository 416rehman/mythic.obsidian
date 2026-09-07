#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Character/Appearance/MythicAppearanceCatalog.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Subsystem/SaveSystem/MythicSaveGame.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"
#include "Character/Appearance/MythicAppearanceMeshComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/GarbageCollection.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicAppearanceOcclusionMaterialTest,"Mythic.Appearance.OcclusionMaterialLifetime",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FMythicAppearanceOcclusionMaterialTest::RunTest(const FString&) {
    TStrongObjectPtr<UMythicAppearanceMeshComponent> Mesh(NewObject<UMythicAppearanceMeshComponent>());
    auto* Base=UMaterial::GetDefaultMaterial(MD_Surface);
    auto* Original=Mesh->GetPaletteMaterial(0,Base);
    TWeakObjectPtr<UMaterialInstanceDynamic> WeakOriginal(Original);
    auto* Occlusion=UMaterialInstanceDynamic::Create(Base,Mesh.Get());
    Mesh->SetMaterial(0,Occlusion);
    CollectGarbage(RF_NoFlags);
    if (!TestTrue(TEXT("Camera-displaced original survives garbage collection"),WeakOriginal.IsValid())) return false;
    auto* Editing=Mesh->GetPaletteMaterial(0,Base);
    const FName DyeParameter(TEXT("AppearanceOcclusionTest"));
    Editing->SetScalarParameterValue(DyeParameter,0.37f);
    TestTrue(TEXT("Palette edits target the exact original the camera will restore"),Editing==WeakOriginal.Get());
    TestTrue(TEXT("Palette edits preserve the active camera override"),Mesh->GetMaterial(0)==Occlusion);
    Mesh->SetMaterial(0,WeakOriginal.Get());
    TestTrue(TEXT("Camera can safely restore the edited palette"),Mesh->GetMaterial(0)==Editing);
    float RestoredValue=0;
    TestTrue(TEXT("Restored material retains the dye edited while occluded"),
        Editing->GetScalarParameterValue(FMaterialParameterInfo(DyeParameter),RestoredValue) && FMath::IsNearlyEqual(RestoredValue,0.37f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicAppearanceSaveTest,"Mythic.Appearance.Persistence",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FMythicAppearanceSaveTest::RunTest(const FString&) {
    auto* Save=NewObject<UMythicSaveGame>();
    Save->CharacterData.DataVersion=static_cast<int32>(CurrentCharacterSaveVersion);
    Save->CharacterData.bHasAppearanceRecipe=true;
    Save->CharacterData.AppearanceRecipe={FName("Female"),FName("Adventurer"),FName("Bald"),FName("Skin.A"),FName("Hair.A"),FName("Eyes.A")};
    Save->CharacterData.AppearanceStamp.Session=FGuid::NewGuid();
    Save->CharacterData.AppearanceStamp.Route=TEXT("PrivateRoute");
    Save->CharacterData.AppearanceStamp.Items.Add({FGuid::NewGuid(),8});
    FSerializedInventoryData Inventory; Inventory.AppearanceStamps=Save->CharacterData.AppearanceStamp.Items;
    Save->CharacterData.Inventories.Add(Inventory);
    auto ChecksumBytes=[](UMythicSaveGame* Object) {TArray<uint8> Bytes; FMemoryWriter Writer(Bytes);FObjectAndNameAsStringProxyArchive Archive(Writer,false);Object->Serialize(Archive);return Bytes;};
    const auto Before=ChecksumBytes(Save);
    TArray<uint8> Disk;TestTrue(TEXT("Real save-game archive writes"),UGameplayStatics::SaveGameToMemory(Save,Disk));
    auto* Loaded=Cast<UMythicSaveGame>(UGameplayStatics::LoadGameFromMemory(Disk));
    if(!TestNotNull(TEXT("Real save-game archive reads"),Loaded))return false;
    TestTrue(TEXT("Checksum bytes identical after actual disk format round trip"),Before==ChecksumBytes(Loaded));
    TestTrue(TEXT("Complete personal recipe survives"),Loaded->CharacterData.AppearanceRecipe==Save->CharacterData.AppearanceRecipe);
    TestTrue(TEXT("Presence survives version fixup"),Loaded->CharacterData.bHasAppearanceRecipe);
    TestFalse(TEXT("Save request session excluded from payload"),Loaded->CharacterData.AppearanceStamp.Session.IsValid());
    TestTrue(TEXT("Item capture stamps excluded from payload"),Loaded->CharacterData.Inventories[0].AppearanceStamps.IsEmpty());
    auto* Legacy=NewObject<UMythicSaveGame>(); Legacy->CharacterData.DataVersion=static_cast<int32>(EMythicCharacterSaveVersion::PreAppearance);
    Legacy->FixupData();
    TestEqual(TEXT("Legacy save migrates"),Legacy->CharacterData.DataVersion,static_cast<int32>(CurrentCharacterSaveVersion));
    TestFalse(TEXT("Legacy absence remains explicit after version is stamped current"),Legacy->CharacterData.bHasAppearanceRecipe);

    auto* Item=NewObject<UMythicItemInstance>();
    auto* Field=FindFProperty<FStructProperty>(UMythicItemInstance::StaticClass(),TEXT("AppearanceData"));
    if(!TestNotNull(TEXT("Canonical physical item owns appearance"),Field))return false;
    auto* Data=Field->ContainerPtrToValuePtr<FMythicItemAppearanceData>(Item);
    Data->Revision=7;Data->Colors={{FName("ClothPrimary"),FName("UnavailableFuturePalette")}};
    TArray<uint8> ItemBytes;FMemoryWriter Writer(ItemBytes,true);FObjectAndNameAsStringProxyArchive Out(Writer,false);Out.ArIsSaveGame=true;Item->Serialize(Out);
    TestFalse(TEXT("Canonical item writer accepted data"),Out.IsError());
    auto* Copy=NewObject<UMythicItemInstance>();FMemoryReader Reader(ItemBytes,true);FObjectAndNameAsStringProxyArchive In(Reader,true);In.ArIsSaveGame=true;Copy->Serialize(In);
    TestFalse(TEXT("Canonical item reader accepted data"),In.IsError());
    TestTrue(TEXT("Unknown authored IDs retained for future recovery"),Copy->GetAppearanceData().HasSameColors(*Data));
    TestEqual(TEXT("Item color revision survives"),Copy->GetAppearanceData().Revision,7);
    TestFalse(TEXT("Differently colored physical items cannot silently stack"),Copy->isStackableWith(NewObject<UMythicItemInstance>()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicAppearanceBoundsTest,"Mythic.Appearance.Validation",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FMythicAppearanceBoundsTest::RunTest(const FString&) {
    FMythicItemAppearanceData A,B;
    A.Colors={{FName("Metal"),FName("Iron")},{FName("Cloth"),FName("Blue")}};
    B.Colors={{FName("Cloth"),FName("Blue")},{FName("Metal"),FName("Iron")}};
    TestTrue(TEXT("Order cannot change physical color identity"),A.HasSameColors(B));
    B.Colors.Add({FName("Cloth"),FName("Blue")});
    TestFalse(TEXT("Duplicate channel rejected in either direction"),A.HasSameColors(B)||B.HasSameColors(A));
    FMythicAppearanceEditRequest Request;Request.RequestId=FGuid::NewGuid();Request.Session=FGuid::NewGuid();
    Request.Items.Add({FGuid::NewGuid(),0,{{FName("Cloth"),FName("Blue")}}});
    TestTrue(TEXT("Bounded ID-only command accepted"),Request.IsBounded());
    const auto Duplicate=Request.Items[0];Request.Items.Add(Duplicate);TestFalse(TEXT("Duplicate physical GUID rejected"),Request.IsBounded());
    Request.Items.Reset();Request.Personal.Frame=FName(*FString::ChrN(65,TEXT('X')));
    TestFalse(TEXT("Oversized authored ID rejected"),Request.IsBounded());
    return true;
}
#endif
