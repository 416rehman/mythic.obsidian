#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "InputAction.h"
#include "InputMappingContext.h"
#include "UObject/UnrealType.h"
#include "World/Harvesting/MythicHarvestFocusComponent.h"
#include "World/Harvesting/MythicHarvestSettings.h"
#include "World/Harvesting/MythicHarvestToolTypeDefinition.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestFocusToolSlotOccupancyTest,
    "Mythic.World.Harvesting.Focus.ToolSlotOccupancy",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestFocusToolSlotOccupancyTest::RunTest(
    const FString &Parameters) {
    UMythicHarvestToolTypeDefinition *Axe =
        NewObject<UMythicHarvestToolTypeDefinition>(GetTransientPackage());
    UMythicHarvestToolTypeDefinition *DifferentAxe =
        NewObject<UMythicHarvestToolTypeDefinition>(GetTransientPackage());
    Axe->DisplayName = FText::FromString(TEXT("Axe"));
    DifferentAxe->DisplayName = FText::FromString(TEXT("Axe"));

    int32 SelectedIndex = INDEX_NONE;
    int32 EquipIndex = INDEX_NONE;
    TArray<FMythicHarvestToolEligibilityProbe> Candidates;
    TestEqual(TEXT("an empty loadout requires the tool"),
              FMythicHarvestFocusRules::EvaluateToolSelection(
                  Axe, 1, Candidates, SelectedIndex, EquipIndex),
              EMythicHarvestFocusAvailability::RequiresTool);
    TestEqual(TEXT("no candidate is selected without the tool"),
              SelectedIndex, INDEX_NONE);

    FMythicHarvestToolEligibilityProbe SameNameWrongAsset;
    SameNameWrongAsset.ToolType = DifferentAxe;
    SameNameWrongAsset.ToolTier = 99;
    SameNameWrongAsset.bInGearSlot = true;
    Candidates.Add(SameNameWrongAsset);
    TestEqual(TEXT("equal display text never substitutes for direct asset identity"),
              FMythicHarvestFocusRules::EvaluateToolSelection(
                  Axe, 1, Candidates, SelectedIndex, EquipIndex),
              EMythicHarvestFocusAvailability::RequiresTool);
    TestEqual(TEXT("a foreign tool family is never selected"),
              SelectedIndex, INDEX_NONE);

    FMythicHarvestToolEligibilityProbe Carried;
    Carried.ToolType = Axe;
    Carried.ToolTier = 3;
    Candidates.Add(Carried);
    TestEqual(TEXT("an axe in the bag is not an axe in the axe slot"),
              FMythicHarvestFocusRules::EvaluateToolSelection(
                  Axe, 1, Candidates, SelectedIndex, EquipIndex),
              EMythicHarvestFocusAvailability::EquipRequired);
    TestEqual(TEXT("carried stock is never selected as the harvest source"),
              SelectedIndex, INDEX_NONE);
    TestEqual(TEXT("the carried axe is offered as the tool to slot"),
              EquipIndex, 1);

    Candidates[1].bInGearSlot = true;
    TestEqual(TEXT("a sufficient nonbroken axe in the axe slot is ready"),
              FMythicHarvestFocusRules::EvaluateToolSelection(
                  Axe, 1, Candidates, SelectedIndex, EquipIndex),
              EMythicHarvestFocusAvailability::Ready);
    TestEqual(TEXT("the slotted tool is the selected wear target"),
              SelectedIndex, 1);

    Candidates[1].bHasDurabilityFragment = false;
    TestEqual(TEXT("a slotted tool that cannot take wear is invalid provenance"),
              FMythicHarvestFocusRules::EvaluateToolSelection(
                  Axe, 1, Candidates, SelectedIndex, EquipIndex),
              EMythicHarvestFocusAvailability::InvalidSource);

    Candidates[1].bHasDurabilityFragment = true;
    Candidates[1].ToolTier = 0;
    TestEqual(TEXT("the slotted tool still enforces the authored tier"),
              FMythicHarvestFocusRules::EvaluateToolSelection(
                  Axe, 1, Candidates, SelectedIndex, EquipIndex),
              EMythicHarvestFocusAvailability::ToolTierTooLow);
    TestEqual(TEXT("the failing slotted tool is still named to the player"),
              SelectedIndex, 1);

    Candidates[1].ToolTier = 3;
    Candidates[1].bBroken = true;
    TestEqual(TEXT("a broken slotted tool asks for repair"),
              FMythicHarvestFocusRules::EvaluateToolSelection(
                  Axe, 1, Candidates, SelectedIndex, EquipIndex),
              EMythicHarvestFocusAvailability::ToolBroken);
    Candidates[1].bBroken = false;

    FMythicHarvestToolEligibilityProbe SecondSlotted = Candidates[1];
    Candidates.Add(SecondSlotted);
    TestEqual(TEXT("one family cannot occupy two tool slots at once"),
              FMythicHarvestFocusRules::EvaluateToolSelection(
                  Axe, 1, Candidates, SelectedIndex, EquipIndex),
              EMythicHarvestFocusAvailability::InvalidSource);
    TestEqual(TEXT("an ambiguous slot occupant selects nothing"),
              SelectedIndex, INDEX_NONE);
    Candidates.SetNum(2);

    TestEqual(TEXT("a null required family cannot name a wear target"),
              FMythicHarvestFocusRules::EvaluateToolSelection(
                  nullptr, 0, Candidates, SelectedIndex, EquipIndex),
              EMythicHarvestFocusAvailability::InvalidSource);
    TestEqual(TEXT("a null required family selects nothing"),
              SelectedIndex, INDEX_NONE);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestFocusPresentationIsolationTest,
    "Mythic.World.Harvesting.Focus.PresentationIsolation",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestFocusPresentationIsolationTest::RunTest(
    const FString &Parameters) {
    const UEnum *AvailabilityEnum =
        StaticEnum<EMythicHarvestFocusAvailability>();
    if (!TestNotNull(TEXT("the availability enum is reflected"),
                     AvailabilityEnum)) {
        return false;
    }
    TestEqual(TEXT("nothing is readied any more"),
              AvailabilityEnum->GetValueByNameString(TEXT("ReadyRequired")),
              static_cast<int64>(INDEX_NONE));
    // Moving a carried axe into the axe slot is ordinary equipping and survives; only readying into hands is gone.
    TestNotEqual(TEXT("offering to slot a carried tool survives"),
                 AvailabilityEnum->GetValueByNameString(TEXT("EquipRequired")),
                 static_cast<int64>(INDEX_NONE));
    // Without this control a renamed enum would report both absences as satisfied.
    TestTrue(TEXT("the missing-tool answer the player actually sees survives"),
             AvailabilityEnum->GetValueByNameString(TEXT("RequiresTool"))
                 != INDEX_NONE);

    const UMythicHarvestFocusComponent *ComponentCDO =
        GetDefault<UMythicHarvestFocusComponent>();
    TestNotNull(TEXT("focus component has a class default object"), ComponentCDO);
    if (ComponentCDO) {
        TestFalse(TEXT("focus component is deliberately nonreplicated"),
                  ComponentCDO->GetIsReplicated());
        TestFalse(TEXT("focus scanning does not consume an actor-component tick"),
                  ComponentCDO->PrimaryComponentTick.bCanEverTick);
    }

    int32 ReflectedMembersSeen = 0;
    for (TFieldIterator<FProperty> PropertyIt(
             UMythicHarvestFocusComponent::StaticClass(),
             EFieldIteratorFlags::ExcludeSuper);
         PropertyIt; ++PropertyIt) {
        ++ReflectedMembersSeen;
        const FString MemberName = PropertyIt->GetName();
        TestFalse(*FString::Printf(
                      TEXT("focus property %s carries no readied state"),
                      *MemberName),
                  MemberName.Contains(TEXT("Readied")));
    }
    for (TFieldIterator<UFunction> FunctionIt(
             UMythicHarvestFocusComponent::StaticClass(),
             EFieldIteratorFlags::ExcludeSuper);
         FunctionIt; ++FunctionIt) {
        ++ReflectedMembersSeen;
        const FString MemberName = FunctionIt->GetName();
        TestFalse(*FString::Printf(
                      TEXT("focus function %s carries no readied state"),
                      *MemberName),
                  MemberName.Contains(TEXT("Readied")));
        TestFalse(*FString::Printf(
                      TEXT("focus function %s is local and not an RPC"),
                      *MemberName),
                  FunctionIt->HasAnyFunctionFlags(FUNC_Net));
    }
    // Without this the two scans above would pass on a class that reflects nothing at all.
    TestTrue(TEXT("the scan actually walked the component's reflected members"),
             ReflectedMembersSeen >= 2);

    const FProperty *FocusProperty = FindFProperty<FProperty>(
        UMythicHarvestFocusComponent::StaticClass(), TEXT("CurrentFocus"));
    const FProperty *FocusDelegate = FindFProperty<FProperty>(
        UMythicHarvestFocusComponent::StaticClass(), TEXT("OnFocusChanged"));
    TestNotNull(TEXT("focus DTO is reflected for presentation"), FocusProperty);
    TestNotNull(TEXT("focus change delegate is reflected for presentation"),
                FocusDelegate);
    if (FocusProperty) {
        TestTrue(TEXT("Blueprint can read the immutable focus DTO"),
                 FocusProperty->HasAnyPropertyFlags(CPF_BlueprintVisible));
        TestTrue(TEXT("Blueprint cannot write the focus DTO"),
                 FocusProperty->HasAnyPropertyFlags(CPF_BlueprintReadOnly));
        TestFalse(TEXT("focus DTO never crosses the network"),
                  FocusProperty->HasAnyPropertyFlags(CPF_Net));
    }

    const FSoftObjectProperty *ActionProperty =
        FindFProperty<FSoftObjectProperty>(
            UMythicHarvestSettings::StaticClass(),
            GET_MEMBER_NAME_CHECKED(UMythicHarvestSettings,
                                    ContextInteractAction));
    const FSoftObjectProperty *MappingProperty =
        FindFProperty<FSoftObjectProperty>(
            UMythicHarvestSettings::StaticClass(),
            GET_MEMBER_NAME_CHECKED(UMythicHarvestSettings,
                                    ContextMappingContext));
    TestNotNull(TEXT("context interaction uses a typed soft input action"),
                ActionProperty);
    TestNotNull(TEXT("context interaction uses a typed soft mapping context"),
                MappingProperty);
    if (ActionProperty) {
        TestEqual(TEXT("context action cannot reference an arbitrary asset class"),
                  ActionProperty->PropertyClass.Get(), UInputAction::StaticClass());
    }
    if (MappingProperty) {
        TestEqual(TEXT("context mapping cannot reference an arbitrary asset class"),
                  MappingProperty->PropertyClass.Get(),
                  UInputMappingContext::StaticClass());
    }
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
