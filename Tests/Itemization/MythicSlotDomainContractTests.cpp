#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Itemization/Inventory/InventoryProfile.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Subsystem/SaveSystem/Character/CharacterData.h"
#include "Subsystem/SaveSystem/Character/SavedInventory.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicInventorySlotDomainContractTest,
    "Mythic.Itemization.Inventory.SlotDomainContract",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicInventorySlotDomainContractTest::RunTest(
    const FString &Parameters) {
    const UEnum *DomainEnum = StaticEnum<EMythicInventorySlotDomain>();
    if (!TestNotNull(TEXT("the slot domain enum is reflected"), DomainEnum)) {
        return false;
    }

    TArray<FString> Authored;
    for (int32 EnumIndex = 0; EnumIndex < DomainEnum->NumEnums(); ++EnumIndex) {
        FString EnumeratorName = DomainEnum->GetNameStringByIndex(EnumIndex);
        // UHT appends a synthetic _MAX enumerator that no designer ever authors.
        if (!EnumeratorName.EndsWith(TEXT("_MAX"))) {
            Authored.Add(MoveTemp(EnumeratorName));
        }
    }
    TestEqual(TEXT("gear has exactly two slot domains"), Authored.Num(), 2);
    TestTrue(TEXT("carried storage is still a domain"),
             Authored.Contains(TEXT("Carried")));
    TestTrue(TEXT("equipment is still a domain"),
             Authored.Contains(TEXT("Equipment")));
    TestEqual(TEXT("the Tool Belt domain is gone"),
              DomainEnum->GetValueByNameString(TEXT("ToolBelt")),
              static_cast<int64>(INDEX_NONE));
    // Without this control the lookup above would report the same clean result on a renamed enum.
    TestTrue(TEXT("the same lookup finds a domain that does exist"),
             DomainEnum->GetValueByNameString(TEXT("Equipment")) != INDEX_NONE);

    FMythicInventorySlotEntry Carried;
    TestEqual(TEXT("an unauthored slot defaults to carried storage"),
              static_cast<int32>(Carried.SlotDomain),
              static_cast<int32>(EMythicInventorySlotDomain::Carried));
    TestFalse(TEXT("carried storage is not gear"), Carried.IsGearSlot());

    FMythicInventorySlotEntry Equipment;
    Equipment.SlotDomain = EMythicInventorySlotDomain::Equipment;
    TestTrue(TEXT("the equipment domain is the one gear predicate"),
             Equipment.IsGearSlot());

    TestNull(TEXT("legacy boolean profile authority is removed"),
             FindFProperty<FProperty>(FInventorySlotGroup::StaticStruct(),
                                      TEXT("bIsEquipmentGroup")));
    TestNotNull(TEXT("profile exposes the typed slot domain"),
                FindFProperty<FProperty>(FInventorySlotGroup::StaticStruct(),
                                         TEXT("SlotDomain")));
    TestNull(TEXT("save slots cannot override current profile domains"),
             FindFProperty<FProperty>(FSerializedSlotData::StaticStruct(),
                                      TEXT("bIsActive")));
    TestNull(TEXT("the character save no longer persists a readied item"),
             FindFProperty<FProperty>(FSerializedCharacterData::StaticStruct(),
                                      TEXT("ReadiedItemGuid")));
    // Without this control a struct that lost every property would pass the two null checks above.
    TestNotNull(TEXT("the same lookup finds a save field that does exist"),
                FindFProperty<FProperty>(FSerializedCharacterData::StaticStruct(),
                                         TEXT("CharacterID")));

    TestNull(TEXT("the readied equipment class is gone from the module"),
             FindObject<UClass>(
                 nullptr,
                 TEXT("/Script/Mythic.MythicReadiedEquipmentComponent")));
    TestNull(TEXT("no readied equipment class survives under any package"),
             FindFirstObject<UClass>(TEXT("MythicReadiedEquipmentComponent")));
    // Without this control a broken class lookup would report the deletion as done.
    TestNotNull(TEXT("the same lookup finds a component class that does exist"),
                FindFirstObject<UClass>(TEXT("MythicInventoryComponent")));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
