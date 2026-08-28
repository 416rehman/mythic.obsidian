#include "Misc/AutomationTest.h"
#include "Engine/AssetManager.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/MythicStatContribution.h"
#include "Settings/MythicCombatSettings.h"
#include "Stats/MythicStatCategoryDefinition.h"
#include "Stats/MythicStatDefinition.h"
#include "System/MythicAssetManager.h"

namespace {
UMythicStatDefinition* LoadStatDefinition(const FGameplayAttribute& Attribute) {
    if (!Attribute.IsValid()) {
        return nullptr;
    }

    UAssetManager& AssetManager = UAssetManager::Get();
    TArray<FPrimaryAssetId> StatIds;
    AssetManager.GetPrimaryAssetIdList(UMythicAssetManager::StatDefinitionType, StatIds);
    for (const FPrimaryAssetId& StatId : StatIds) {
        UMythicStatDefinition* Definition = Cast<UMythicStatDefinition>(
            AssetManager.GetPrimaryAssetPath(StatId).TryLoad());
        if (Definition && Definition->Attribute == Attribute) {
            return Definition;
        }
    }
    return nullptr;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPrimaryStatTierTest,
    "Mythic.UI.PrimaryStatTier",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPrimaryStatTierTest::RunTest(const FString &Parameters) {
    const FPrimaryAssetId PrimaryCategoryId(
        UMythicAssetManager::StatCategoryDefinitionType, FName(TEXT("Stat.Category.Primary")));
    const UMythicStatDefinition* Power = LoadStatDefinition(UMythicAttributeSet_Offense::GetPowerAttribute());
    const UMythicStatDefinition* Strength = LoadStatDefinition(UMythicAttributeSet_Defense::GetStrengthAttribute());
    if (!TestNotNull(TEXT("Power has a canonical StatDefinition"), Power)
        || !TestNotNull(TEXT("Strength has a canonical StatDefinition"), Strength)) {
        return false;
    }

    TestEqual(TEXT("Power is in the authored Primary category"), Power->Category.GetPrimaryAssetId(), PrimaryCategoryId);
    TestEqual(TEXT("Strength is in the authored Primary category"), Strength->Category.GetPrimaryAssetId(), PrimaryCategoryId);

    // Things a primary derives must NOT themselves be primary, or the tier stops meaning anything.
    const UMythicStatDefinition* Damage = LoadStatDefinition(UMythicAttributeSet_Offense::GetDamagePerHitAttribute());
    const UMythicStatDefinition* Armor = LoadStatDefinition(UMythicAttributeSet_Defense::GetArmorAttribute());
    if (!TestNotNull(TEXT("DamagePerHit has a canonical StatDefinition"), Damage)
        || !TestNotNull(TEXT("Armor has a canonical StatDefinition"), Armor)) {
        return false;
    }
    TestNotEqual(TEXT("damage per hit is derived, not primary"), Damage->Category.GetPrimaryAssetId(), PrimaryCategoryId);
    TestNotEqual(TEXT("armor is derived, not primary"), Armor->Category.GetPrimaryAssetId(), PrimaryCategoryId);

    const UMythicStatCategoryDefinition* PrimaryCategory = Cast<UMythicStatCategoryDefinition>(
        UAssetManager::Get().GetPrimaryAssetPath(PrimaryCategoryId).TryLoad());
    TestNotNull(TEXT("the primary category is a real asset"), PrimaryCategory);
    if (PrimaryCategory) {
        TestFalse(TEXT("the primary category owns its localized heading"), PrimaryCategory->DisplayName.IsEmpty());
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTooltipMatchesGameplayTest,
    "Mythic.UI.TooltipMatchesGameplay",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicTooltipMatchesGameplayTest::RunTest(const FString &Parameters) {
    // THE DEFECT THIS EXISTS FOR: a hand-written tooltip drifts silently the moment a designer retunes a
    // coefficient. The figure a player reads has to be produced by the same rows that decide what hits.
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    if (!Settings || Settings->StatContributions.Contributions.Num() == 0) {
        AddError(TEXT("the primary stat model is not authored"));
        return false;
    }
    const TArray<FMythicStatContribution> &Rows = Settings->StatContributions.Contributions;
    const FGameplayAttribute Power = UMythicAttributeSet_Offense::GetPowerAttribute();

    // Every row the tooltip would show for Power must resolve to what gameplay resolves for the same value.
    const float PowerValue = 42.0f;
    int32 Shown = 0;
    for (const FMythicStatContribution &Row : Rows) {
        if (Row.SourceStat != Power || !FMythicStatContributionRules::IsRowLive(Row)) {
            continue;
        }
        ++Shown;

        const float TooltipFraction = FMythicStatContributionRules::ResolveRow(Row, PowerValue);
        const float GameplayFraction = FMythicStatContributionRules::ResolveTarget(
            Rows, Row.TargetAttribute,
            [PowerValue, &Power](const FGameplayAttribute &Attr) -> float {
                return Attr == Power ? PowerValue : 0.0f;
            });
        TestEqual(TEXT("the tooltip figure is the gameplay figure"), TooltipFraction, GameplayFraction);

        // And it must be a real number a player can act on, not a silent zero.
        TestTrue(TEXT("a primary at 42 is contributing something"), TooltipFraction > 0.0f);

        const UMythicStatDefinition* TargetDefinition = LoadStatDefinition(Row.TargetAttribute);
        TestNotNull(TEXT("every contribution target has a StatDefinition"), TargetDefinition);
        if (TargetDefinition) {
            TestFalse(TEXT("every contribution line uses its localized canonical label"),
                      TargetDefinition->DisplayName.IsEmpty());
        }
    }

    TestTrue(TEXT("Power has contributions to show at all"), Shown > 0);

    return true;
}
