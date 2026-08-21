
#include "Misc/AutomationTest.h"

#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"

#include "Progression/MythicStatLedgerComponent.h"
#include "Settings/MythicDeveloperSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicItemStatTest,
    "Mythic.Progression.ItemStats",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicItemStatTest::RunTest(const FString &Parameters) {
    using Ledger = UMythicStatLedgerComponent;
    const UGameplayTagsManager &Tags = UGameplayTagsManager::Get();

    auto Tag = [&Tags](const TCHAR *Name) { return Tags.RequestGameplayTag(FName(Name), false); };

    const FGameplayTag Mining = Tag(TEXT("Itemization.Type.Mining"));
    const FGameplayTag Ore = Tag(TEXT("Itemization.Type.Mining.Ore"));
    const FGameplayTag Farming = Tag(TEXT("Itemization.Type.Farming"));
    const FGameplayTag Sword = Tag(TEXT("Itemization.Type.Equipment.Weapon.Sword"));

    if (!TestTrue(TEXT("the item type tags are registered, or this proves nothing"),
                  Mining.IsValid() && Ore.IsValid() && Farming.IsValid() && Sword.IsValid())) {
        return false;
    }

    FGameplayTagContainer Gathered;
    Gathered.AddTag(Mining);
    Gathered.AddTag(Farming);

    {
        FGameplayTagContainer OreItem;
        OreItem.AddTag(Ore);
        TestTrue(TEXT("a child of a gathering family counts as gathered"), Ledger::IsGatheredAcquisition(OreItem, Gathered));

        FGameplayTagContainer SwordItem;
        SwordItem.AddTag(Sword);
        TestFalse(TEXT("equipment is looted, not gathered"), Ledger::IsGatheredAcquisition(SwordItem, Gathered));

        FGameplayTagContainer Untyped;
        TestFalse(TEXT("an item with no tags is looted"), Ledger::IsGatheredAcquisition(Untyped, Gathered));

        // An empty setting must mean "nothing is gathered", not "everything is".
        FGameplayTagContainer None;
        TestFalse(TEXT("no configured gathering families means nothing counts as gathered"),
                  Ledger::IsGatheredAcquisition(OreItem, None));
    }

    // The two counters must be disjoint, or an achievement counting one is inflated by the other.
    {
        FGameplayTagContainer OreItem;
        OreItem.AddTag(Ore);
        const bool bGathered = Ledger::IsGatheredAcquisition(OreItem, Gathered);
        FGameplayTagContainer SwordItem;
        SwordItem.AddTag(Sword);
        const bool bSwordGathered = Ledger::IsGatheredAcquisition(SwordItem, Gathered);
        TestTrue(TEXT("one acquisition is classified exactly one way"), bGathered != bSwordGathered);
    }

    // Stack sizes arrive as a float magnitude and must land as whole items.
    {
        TestEqual(TEXT("a single item counts once"), Ledger::QuantityFromEvent(1.0f), (int64)1);
        TestEqual(TEXT("a stack counts as its size"), Ledger::QuantityFromEvent(12.0f), (int64)12);
        TestEqual(TEXT("a fractional magnitude rounds"), Ledger::QuantityFromEvent(2.6f), (int64)3);
        TestEqual(TEXT("zero still counts as one acquisition"), Ledger::QuantityFromEvent(0.0f), (int64)1);
        TestEqual(TEXT("a negative cannot subtract from the counter"), Ledger::QuantityFromEvent(-5.0f), (int64)1);
    }

    // The shipped setting must actually name families, or every acquisition silently reads as looted.
    {
        const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
        if (TestNotNull(TEXT("developer settings exist"), Settings)) {
            TestFalse(TEXT("the shipped build names at least one gathering family"),
                      Settings->GatheredItemTypes.IsEmpty());
        }
    }

    return true;
}
