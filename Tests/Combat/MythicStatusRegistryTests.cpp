
#include "Misc/AutomationTest.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/Effects/MythicStatusEffectDefinition.h"
#include "GAS/Effects/MythicStatusRegistry.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayTagsManager.h"
#include "Settings/MythicDeveloperSettings.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Itemization/Affixes/MythicAffixDefinition.h"
#include "Stats/MythicStatDefinition.h"

namespace MythicTest {
// Every stat reached by a shipped Affix Definition that owns at least one embedded tier progression. Read from the
// registry so a newly authored build stat counts automatically.
static TSet<FString> GatherRollableAttributes() {
    FAssetRegistryModule &Module = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry &Registry = Module.Get();
    Registry.SearchAllAssets(true);

    TSet<FPrimaryAssetId> ReferencedStatIds;
    TArray<FAssetData> DefinitionAssets;
    Registry.GetAssetsByClass(UMythicAffixDefinition::StaticClass()->GetClassPathName(), DefinitionAssets);
    for (const FAssetData &DefinitionAsset : DefinitionAssets) {
        const UMythicAffixDefinition *Definition = Cast<UMythicAffixDefinition>(DefinitionAsset.GetAsset());
        if (!Definition || Definition->TierProgressions.IsEmpty()
            || !Definition->TargetStat.IsValid()) {
            continue;
        }
        ReferencedStatIds.Add(Definition->TargetStat.GetPrimaryAssetId());
    }

    TSet<FString> Out;
    TArray<FAssetData> StatAssets;
    Registry.GetAssetsByClass(UMythicStatDefinition::StaticClass()->GetClassPathName(), StatAssets);
    for (const FAssetData &StatAsset : StatAssets) {
        const UMythicStatDefinition *Stat = Cast<UMythicStatDefinition>(StatAsset.GetAsset());
        if (Stat && ReferencedStatIds.Contains(Stat->GetPrimaryAssetId()) && Stat->Attribute.IsValid()) {
            Out.Add(Stat->Attribute.GetName());
        }
    }
    return Out;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatusLibraryTest,
    "Mythic.Combat.StatusLibrary",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatusLibraryTest::RunTest(const FString &Parameters) {
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    if (!TestNotNull(TEXT("developer settings exist"), Settings)) {
        return false;
    }
    if (!TestFalse(TEXT("StatusEffectLibrary is configured — without it no status can be applied"), Settings->StatusEffectLibrary.IsNull())) {
        return false;
    }

    UMythicStatusEffectLibrary *Library = Settings->StatusEffectLibrary.LoadSynchronous();
    if (!TestNotNull(TEXT("StatusEffectLibrary loads"), Library)) {
        return false;
    }
    TestTrue(TEXT("library authors at least one status"), Library->Statuses.Num() > 0);

    const FString TypePrefix = TEXT("Status.Type.");
    TSet<FGameplayTag> SeenTypes;
    TArray<FGameplayAttribute> CoveredBuildups;

    for (const UMythicStatusEffectDefinition *Definition : Library->Statuses) {
        if (!TestNotNull(TEXT("library holds no null status entries"), Definition)) {
            continue;
        }
        const FString Label = Definition->GetName();

        TestTrue(*FString::Printf(TEXT("%s has a StatusType tag"), *Label), Definition->StatusType.IsValid());
        TestTrue(*FString::Printf(TEXT("%s StatusType lives under Status.Type"), *Label), Definition->StatusType.ToString().StartsWith(TypePrefix));

        // An unregistered tag still round-trips through the asset but resolves to nothing at runtime, so
        // IsValid() alone passes while the status can never be found. Ask the manager directly.
        const FGameplayTag Registered = UGameplayTagsManager::Get().RequestGameplayTag(Definition->StatusType.GetTagName(), false);
        TestTrue(*FString::Printf(TEXT("%s StatusType %s is registered with the tag manager"), *Label, *Definition->StatusType.ToString()),
                 Registered.IsValid());
        TestFalse(*FString::Printf(TEXT("%s StatusType is unique"), *Label), SeenTypes.Contains(Definition->StatusType));
        SeenTypes.Add(Definition->StatusType);

        TestNotNull(*FString::Printf(TEXT("%s has an EffectToApply"), *Label), Definition->EffectToApply.Get());
        TestTrue(*FString::Printf(TEXT("%s has a GrantedStateTag"), *Label), Definition->GrantedStateTag.IsValid());
        TestTrue(*FString::Printf(TEXT("%s has an OnsetCueTag"), *Label), Definition->OnsetCueTag.IsValid());
        TestTrue(*FString::Printf(TEXT("%s has a BuildupAttribute"), *Label), Definition->BuildupAttribute.IsValid());
        TestTrue(*FString::Printf(TEXT("%s has a ResistanceAttribute"), *Label), Definition->ResistanceAttribute.IsValid());
        TestFalse(*FString::Printf(TEXT("%s has a display name"), *Label), Definition->DisplayName.IsEmpty());
        TestFalse(*FString::Printf(TEXT("%s has an icon"), *Label), Definition->Icon.IsNull());

        // GrantedStateTag drives UI and damage modifiers, but the tag is really granted by the effect.
        // If the two drift apart the status silently stops matching anything that reads it.
        if (Definition->EffectToApply && Definition->GrantedStateTag.IsValid()) {
            const UGameplayEffect *EffectCDO = GetDefault<UGameplayEffect>(Definition->EffectToApply);
            const UTargetTagsGameplayEffectComponent *TagComponent =
                EffectCDO ? EffectCDO->FindComponent<UTargetTagsGameplayEffectComponent>() : nullptr;
            if (TestNotNull(*FString::Printf(TEXT("%s effect grants target tags"), *Label), TagComponent)) {
                TestTrue(*FString::Printf(TEXT("%s GrantedStateTag %s matches what the effect actually grants"), *Label,
                                          *Definition->GrantedStateTag.ToString()),
                         TagComponent->GetConfiguredTargetTagChanges().CombinedTags.HasTagExact(Definition->GrantedStateTag));
            }
        }

        for (const FMythicStatusReaction &Reaction : Definition->Reactions) {
            TestTrue(*FString::Printf(TEXT("%s reaction has a RequiredTargetTag"), *Label), Reaction.RequiredTargetTag.IsValid());
        }

        if (Definition->BuildupAttribute.IsValid()) {
            CoveredBuildups.AddUnique(Definition->BuildupAttribute);
        }
    }

    // Every buildup the damage pipeline writes must resolve to a status, or that status silently never fires.
    const TArray<FGameplayAttribute> PipelineBuildups = {
        UMythicAttributeSet_Defense::GetBurnBuildupAttribute(),
        UMythicAttributeSet_Defense::GetBleedBuildupAttribute(),
        UMythicAttributeSet_Defense::GetPoisonBuildupAttribute(),
        UMythicAttributeSet_Defense::GetSlowBuildupAttribute(),
        UMythicAttributeSet_Defense::GetFreezeBuildupAttribute(),
        UMythicAttributeSet_Defense::GetStunBuildupAttribute(),
        UMythicAttributeSet_Defense::GetWeakenBuildupAttribute(),
        UMythicAttributeSet_Defense::GetTerrifyBuildupAttribute(),
    };
    for (const FGameplayAttribute &Buildup : PipelineBuildups) {
        TestTrue(*FString::Printf(TEXT("a status covers the %s the damage pipeline feeds"), *Buildup.GetName()), CoveredBuildups.Contains(Buildup));
    }

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatusRollableTest,
    "Mythic.Combat.StatusRollable",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatusRollableTest::RunTest(const FString &Parameters) {
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    UMythicStatusEffectLibrary *Library = Settings ? Settings->StatusEffectLibrary.LoadSynchronous() : nullptr;
    if (!TestNotNull(TEXT("status library loads"), Library)) {
        return false;
    }

    const TSet<FString> RollableAttributes = MythicTest::GatherRollableAttributes();
    if (!TestTrue(TEXT("the project ships an Affix Definition with embedded tiers"), RollableAttributes.Num() > 0)) {
        return false;
    }

    // A status a player cannot roll for is a status no build can be made of. This is the check that would have
    // caught Poison and Freeze being absent from Affix Definitions while still looking wired up in code.
    for (const UMythicStatusEffectDefinition *Definition : Library->Statuses) {
        if (!Definition || !Definition->StatusType.IsValid()) {
            continue;
        }
        const FString TagPath = Definition->StatusType.ToString();
        int32 Dot = INDEX_NONE;
        const FString Leaf = TagPath.FindLastChar(TCHAR('.'), Dot) ? TagPath.RightChop(Dot + 1) : TagPath;
        const FString ChanceAttribute = FString::Printf(TEXT("Apply%sOnHitChance"), *Leaf);

        TestTrue(*FString::Printf(TEXT("%s can be rolled by loot (%s has an embedded affix tier set)"), *Leaf, *ChanceAttribute),
                 RollableAttributes.Contains(ChanceAttribute));
    }

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicBuildStatsRollableTest,
    "Mythic.Combat.BuildStatsRollable",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicBuildStatsRollableTest::RunTest(const FString &Parameters) {
    const TSet<FString> RollableAttributes = MythicTest::GatherRollableAttributes();
    if (!TestTrue(TEXT("the project ships an Affix Definition with embedded tiers"), RollableAttributes.Num() > 0)) {
        return false;
    }

    /**
     * The stats a build is made of. Each is read by the damage pipeline, so any of them affix data does not roll is a
     * lever the player can see working in code and never obtain. IncreasedDamageToEnemiesUnderStatusEffects and
     * BonusDamageToSuperiorEnemies were both in exactly that state: live in the execution, granted by nothing.
     * This list is the specification — add to it when a new build stat ships.
     */
    const TCHAR *BuildStats[] = {
        TEXT("Power"),
        TEXT("CriticalHitChance"),
        TEXT("CriticalHitDamage"),
        TEXT("BonusSkillDamage"),
        TEXT("StatusBuildupMultiplier"),
        TEXT("IncreasedDamageToEnemiesUnderStatusEffects"),
        TEXT("BonusDamageToSuperiorEnemies"),
        TEXT("DecreasedDamageFromEnemiesUnderStatusEffects"),
        TEXT("MovementSpeedMultiplier"),
        TEXT("Armor"),
        TEXT("MaxHealth"),
    };

    for (const TCHAR *Stat : BuildStats) {
        TestTrue(*FString::Printf(TEXT("%s can be rolled by loot"), Stat), RollableAttributes.Contains(FString(Stat)));
    }

    return true;
}
