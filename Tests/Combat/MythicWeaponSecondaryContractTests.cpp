#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include <limits>
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/Fragments/Actionable/AttackFragment.h"
#include "Itemization/Inventory/Fragments/Actionable/WeaponSecondaryAttackFragment.h"
#include "Itemization/Inventory/Fragments/Actionable/WeaponBlockFragment.h"
#include "Itemization/Inventory/Fragments/Actionable/MythicWeaponActionContract.h"
#include "Input/MythicTags_Input.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicWeaponSecondaryContractTest,
    "Mythic.Combat.Weapon.SecondaryDefinitionContract",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMythicWeaponSecondaryContractTest::RunTest(const FString&) {
    TStrongObjectPtr<UItemDefinition> Definition(NewObject<UItemDefinition>());
    Definition->ItemType=ITEMIZATION_TYPE_EQUIPMENT_WEAPON_SWORD;
    auto* Primary=NewObject<UAttackFragment>(Definition.Get());
    auto* Heavy=NewObject<UWeaponSecondaryAttackFragment>(Definition.Get());
    auto* Guard=NewObject<UWeaponBlockFragment>(Definition.Get());
    Definition->Fragments={Heavy,Primary};
    TestTrue(TEXT("default heavy values are valid"),Heavy->HasValidHeavyTuning());
    TestEqual(TEXT("heavy damage defaults to 1.75"),Heavy->DamageMultiplier,1.75f);
    TestEqual(TEXT("heavy rate defaults to 0.70"),Heavy->AttackRateMultiplier,0.70f);
    Heavy->AttackRateMultiplier=0;
    TestFalse(TEXT("zero heavy cadence rejects definition before granting"),MythicWeaponAction::ValidateDefinition(Definition.Get()));
    Heavy->AttackRateMultiplier=0.7f;
    Heavy->DamageMultiplier=std::numeric_limits<float>::infinity();
    TestFalse(TEXT("nonfinite heavy damage rejects definition"),MythicWeaponAction::ValidateDefinition(Definition.Get()));
    Heavy->DamageMultiplier=1.75f;
    TestTrue(TEXT("Primary is canonical Attack"),Primary->GetCanonicalInputTag()==INPUT_ACTION_ATTACK);
    TestTrue(TEXT("Heavy is canonical AttackAlt"),Heavy->GetCanonicalInputTag()==INPUT_ACTION_ATTACK_ALT);
    TestTrue(TEXT("Secondary-first array remains a legal two-action shape"),MythicWeaponAction::ValidateDefinition(Definition.Get()));
    TestTrue(TEXT("Primary lookup cannot select the preceding derived heavy"),UAttackFragment::FindPrimaryOnDefinition(Definition.Get())==Primary);
    TestFalse(TEXT("Identical montage/config cannot make primary and heavy stack-compatible"),Primary->CanBeStackedWith(Heavy));
    Definition->Fragments.Add(Guard);
    TestFalse(TEXT("Heavy plus block is rejected"),MythicWeaponAction::ValidateDefinition(Definition.Get()));
    Definition->Fragments={Primary,Guard};
    TestTrue(TEXT("Canonical Fortify has a separately owned persistent handle"),Guard->HasUsableEffect());
    TestTrue(TEXT("Primary plus guard is legal"),MythicWeaponAction::ValidateDefinition(Definition.Get()));
    Definition->ItemType=ITEMIZATION_TYPE_EQUIPMENT_TOOL;
    TestFalse(TEXT("A tool cannot publish a weapon secondary"),MythicWeaponAction::ValidateDefinition(Definition.Get()));
    Definition->Fragments={Primary};
    TestTrue(TEXT("Unchanged tool primary remains legal"),MythicWeaponAction::ValidateDefinition(Definition.Get()));
    Definition->Fragments.Add(NewObject<UAttackFragment>(Definition.Get()));
    TestFalse(TEXT("Two primaries fail rather than depend on array order"),MythicWeaponAction::ValidateDefinition(Definition.Get()));
    TestNull(TEXT("Ambiguous primary lookup fails closed"),UAttackFragment::FindPrimaryOnDefinition(Definition.Get()));
    return true;
}
#endif
