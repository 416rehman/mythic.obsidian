#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Itemization/Inventory/InventoryProfile.h"
#include "Misc/DataValidation.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPlayerLoadoutDomainContentTest,
    "Mythic.Itemization.Inventory.PlayerLoadoutDomains",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicPlayerLoadoutDomainContentTest::RunTest(const FString & /*Parameters*/) {
    const UInventoryProfile *Profile = LoadObject<UInventoryProfile>(
        nullptr,
        TEXT("/Game/Mythic/Itemization/Profiles/Player/DA_InvProfile_Player.DA_InvProfile_Player"));
    if (!TestNotNull(TEXT("the shipped player inventory profile loads"), Profile)) {
        return false;
    }

    // Weapon and Tools sit here beside armor: a tool in its slot is passive gear that is never wielded, so the
    // whole loadout shares the one gear domain.
    const TSet<FName> ExpectedLoadoutGroups = {
        TEXT("Inventory.Group.Equipment.Armor"),
        TEXT("Inventory.Group.Equipment.Accessories"),
        TEXT("Inventory.Group.Equipment.QuickPouch"),
        TEXT("Inventory.Group.Equipment.Weapon"),
        TEXT("Inventory.Group.Equipment.Tools"),
    };

    int32 EquipmentGroupsSeen = 0;
    int32 CarriedGroupsSeen = 0;
    for (const TPair<FGameplayTag, FInventorySlotGroup> &Pair : Profile->SlotGroups) {
        const FName GroupName = Pair.Key.GetTagName();
        if (ExpectedLoadoutGroups.Contains(GroupName)) {
            ++EquipmentGroupsSeen;
            TestEqual(*FString::Printf(TEXT("%s declares the Equipment domain"), *GroupName.ToString()),
                      static_cast<int32>(Pair.Value.SlotDomain),
                      static_cast<int32>(EMythicInventorySlotDomain::Equipment));
            continue;
        }
        ++CarriedGroupsSeen;
        TestEqual(*FString::Printf(TEXT("%s is carried storage"), *GroupName.ToString()),
                  static_cast<int32>(Pair.Value.SlotDomain),
                  static_cast<int32>(EMythicInventorySlotDomain::Carried));
    }

    // Without this the loop above passes vacuously on a profile that lost its equipment groups entirely.
    TestEqual(TEXT("every expected loadout group is present in the shipped profile"),
              EquipmentGroupsSeen, ExpectedLoadoutGroups.Num());
    TestTrue(TEXT("the profile also carries ordinary inventory groups"), CarriedGroupsSeen > 0);

    FDataValidationContext ValidationContext;
    TestEqual(TEXT("the shipped player profile passes its own authoring rules"),
              Profile->IsDataValid(ValidationContext), EDataValidationResult::Valid);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicInventoryProfileValidationTest,
    "Mythic.Itemization.Inventory.EquipmentGroupDomainValidation",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicInventoryProfileValidationTest::RunTest(const FString & /*Parameters*/) {
    UInventoryProfile *Profile = NewObject<UInventoryProfile>(GetTransientPackage());
    if (!TestNotNull(TEXT("a profile fixture constructs"), Profile)) {
        return false;
    }

    FInventorySlotGroup Group;
    FInventoryProfileEntry Entry;
    Entry.SlotDefinition = NewObject<UInventorySlotDefinition>(Profile);
    Entry.Count = 1;
    Group.Slots.Add(Entry);
    Group.SlotDomain = EMythicInventorySlotDomain::Carried;

    const FGameplayTag EquipmentGroup =
        FGameplayTag::RequestGameplayTag(TEXT("Inventory.Group.Equipment.Armor"));
    if (!TestTrue(TEXT("the canonical equipment group tag is registered"), EquipmentGroup.IsValid())) {
        return false;
    }
    Profile->SlotGroups.Add(EquipmentGroup, Group);

    {
        FDataValidationContext Context;
        TestEqual(TEXT("an equipment group left on the Carried default is rejected"),
                  Profile->IsDataValid(Context), EDataValidationResult::Invalid);
        bool bNamedTheDomain = false;
        for (const FDataValidationContext::FIssue &Issue : Context.GetIssues()) {
            bNamedTheDomain |= Issue.Message.ToString().Contains(TEXT("Carried domain"));
        }
        TestTrue(TEXT("the rejection explains which domain is missing"), bNamedTheDomain);
    }

    Profile->SlotGroups[EquipmentGroup].SlotDomain = EMythicInventorySlotDomain::Equipment;
    {
        FDataValidationContext Context;
        TestEqual(TEXT("declaring the Equipment domain satisfies the rule"),
                  Profile->IsDataValid(Context), EDataValidationResult::Valid);
    }

    return true;
}

#endif
