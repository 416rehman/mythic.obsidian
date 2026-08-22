#include "Misc/AutomationTest.h"

#include "AI/MythicTags_AI.h"
#include "GAS/Effects/MythicEnemyScaling.h"
#include "Itemization/Affixes/MythicAffixTierTypes.h"
#include "Itemization/Sockets/MythicSocketTypes.h"
#include "Settings/MythicDeveloperSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDropItemLevelTest,
    "Mythic.Itemization.DropItemLevel",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicDropItemLevelTest::RunTest(const FString &Parameters) {
    // THE REGRESSION THIS EXISTS FOR: the enemy drop path never set ItemLevel, so every drop was level 0.
    // Zero fails every affix tier gate (the lowest MinItemLevel in any shipped pool is 1) and drives the
    // socket cap to zero, so three affix pools and every gem were invisible in play - and it read as a
    // design choice, because a sword with no affixes still looks like a sword.
    TestTrue(TEXT("a normal kill still drops something an affix pool can gate on"),
             FMythicEnemyScaling::ComputeDropItemLevel(1.0f, AI_TIER_NORMAL) >= 1);

    // Never zero, whatever the world says - including before the tier attribute has replicated.
    TestTrue(TEXT("an unreplicated world base still drops at level 1 or better"),
             FMythicEnemyScaling::ComputeDropItemLevel(0.0f, AI_TIER_NORMAL) >= 1);
    TestTrue(TEXT("a negative world base cannot produce a level 0 drop"),
             FMythicEnemyScaling::ComputeDropItemLevel(-5.0f, AI_TIER_BOSS) >= 1);

    // A tougher kill drops better gear, which is the whole reason tiers escalate.
    const int32 Normal = FMythicEnemyScaling::ComputeDropItemLevel(10.0f, AI_TIER_NORMAL);
    const int32 Superior = FMythicEnemyScaling::ComputeDropItemLevel(10.0f, AI_TIER_SUPERIOR);
    const int32 Elite = FMythicEnemyScaling::ComputeDropItemLevel(10.0f, AI_TIER_ELITE);
    const int32 Champion = FMythicEnemyScaling::ComputeDropItemLevel(10.0f, AI_TIER_CHAMPION);
    const int32 Boss = FMythicEnemyScaling::ComputeDropItemLevel(10.0f, AI_TIER_BOSS);

    TestTrue(TEXT("superior drops above normal"), Superior > Normal);
    TestTrue(TEXT("elite drops above superior"), Elite > Superior);
    TestTrue(TEXT("champion drops above elite"), Champion > Elite);
    TestTrue(TEXT("boss drops above champion"), Boss > Champion);

    // The world ladder moves the whole floor, so pushing world tier raises what every kill drops.
    TestTrue(TEXT("a higher world base raises the drop"),
             FMythicEnemyScaling::ComputeDropItemLevel(50.0f, AI_TIER_NORMAL) >
             FMythicEnemyScaling::ComputeDropItemLevel(10.0f, AI_TIER_NORMAL));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDropRollsContentTest,
    "Mythic.Itemization.DropRollsContent",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicDropRollsContentTest::RunTest(const FString &Parameters) {
    // The point of the fix is not the number, it is that the number now clears the gates that were
    // silently rejecting every drop. Assert against the real gates rather than the level itself.
    const int32 DropLevel = FMythicEnemyScaling::ComputeDropItemLevel(1.0f, AI_TIER_NORMAL);

    // Affix gate: a tier is eligible when MinItemLevel <= ItemLevel. The lowest authored is 1.
    TArray<FMythicAffixTier> Tiers;
    FMythicAffixTier First;
    First.MinItemLevel = 1;
    First.Weight = 1.0f;
    Tiers.Add(First);

    TestNotEqual(TEXT("the weakest kill's drop clears the lowest affix tier"),
                 FMythicAffixTierMath::SelectTierIndex(DropLevel, Tiers, 0.5f), -1);
    TestEqual(TEXT("and a level 0 drop - the old behaviour - clears nothing"),
              FMythicAffixTierMath::SelectTierIndex(0, Tiers, 0.5f), -1);

    // Socket gate: LevelCap = ItemLevel / ItemLevelsPerSocket, and an EffectiveCap of 0 returns no sockets.
    const FGameplayTag ItemType = FGameplayTag::RequestGameplayTag(FName("Item"), false);
    FMythicSocketCountTable Table;
    Table.HardCap = 3;
    Table.ItemLevelsPerSocket = 5;
    FMythicSocketCountRule Rule;
    Rule.ItemTypeParent = ItemType;
    Rule.MaxByRarity = {1, 2, 3, 3, 3};
    Table.Rules.Add(Rule);

    if (ItemType.IsValid()) {
        const int32 HighLevelDrop = FMythicEnemyScaling::ComputeDropItemLevel(30.0f, AI_TIER_ELITE);
        TestTrue(TEXT("a realistic drop level can hold a socket"),
                 FMythicSocketMath::RollSocketCount(ItemType, HighLevelDrop, 0, Table, 1.0f) > 0);
        TestEqual(TEXT("and a level 0 drop - the old behaviour - can hold none"),
                  FMythicSocketMath::RollSocketCount(ItemType, 0, 0, Table, 1.0f), 0);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicEnemyTierLadderTest,
    "Mythic.Combat.EnemyTierLadder",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicEnemyTierLadderTest::RunTest(const FString &Parameters) {
    // The ladder was five rows of magic numbers in a C++ switch, so no designer could retune it. It is
    // project settings now; assert it ships configured rather than empty, or every tier silently
    // flattens to 1x and the tier system stops existing.
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    if (!TestNotNull(TEXT("developer settings resolve"), Settings)) {
        return false;
    }
    TestEqual(TEXT("all five tiers are authored"), Settings->EnemyTierScaling.Num(), 5);

    const FMythicTierScaling Normal = FMythicEnemyScaling::GetTierScaling(AI_TIER_NORMAL);
    const FMythicTierScaling Boss = FMythicEnemyScaling::GetTierScaling(AI_TIER_BOSS);

    TestEqual(TEXT("normal is the unscaled baseline"), Normal.HealthMult, 1.0f);
    TestTrue(TEXT("a boss has more health than a normal"), Boss.HealthMult > Normal.HealthMult);
    TestTrue(TEXT("a boss hits harder than a normal"), Boss.DamageMult > Normal.DamageMult);
    TestTrue(TEXT("a boss is worth more xp than a normal"), Boss.XpMult > Normal.XpMult);
    TestTrue(TEXT("a boss drops better gear than a normal"), Boss.ItemLevelBonus > Normal.ItemLevelBonus);

    // An unknown tag must leave an enemy at its authored strength, not erase it.
    const FMythicTierScaling Unknown = FMythicEnemyScaling::GetTierScaling(FGameplayTag());
    TestEqual(TEXT("an unknown tier does not scale health"), Unknown.HealthMult, 1.0f);
    TestEqual(TEXT("an unknown tier grants no item level"), Unknown.ItemLevelBonus, 0);

    return true;
}
