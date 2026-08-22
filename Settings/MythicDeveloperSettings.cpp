#include "MythicDeveloperSettings.h"
#include "AI/MythicTags_AI.h"

#include "GAS/MythicAbilityTagRelationshipMapping.h"

UMythicDeveloperSettings::UMythicDeveloperSettings() {
    // Seeded with the ladder that used to be a switch in C++, plus the item level each tier now grants.
    // A designer edits these in Project Settings; nothing reads a constant any more.
    if (EnemyTierScaling.Num() == 0) {
        auto Row = [](const FGameplayTag &Tier, float H, float D, float X, int32 IL) {
            FMythicEnemyTierScaling R;
            R.Tier = Tier;
            R.HealthMultiplier = H;
            R.DamageMultiplier = D;
            R.XpMultiplier = X;
            R.ItemLevelBonus = IL;
            return R;
        };
        EnemyTierScaling.Add(Row(AI_TIER_NORMAL, 1.0f, 1.0f, 1.0f, 0));
        EnemyTierScaling.Add(Row(AI_TIER_SUPERIOR, 1.5f, 1.3f, 2.0f, 2));
        EnemyTierScaling.Add(Row(AI_TIER_ELITE, 2.5f, 1.8f, 4.0f, 5));
        EnemyTierScaling.Add(Row(AI_TIER_CHAMPION, 5.0f, 2.5f, 8.0f, 10));
        EnemyTierScaling.Add(Row(AI_TIER_BOSS, 12.0f, 4.0f, 20.0f, 20));
    }

    // The two families a player takes straight out of the world. Overridable in Project Settings.
    GatheredItemTypes.AddTag(FGameplayTag::RequestGameplayTag(FName("Itemization.Type.Farming"), false));
    GatheredItemTypes.AddTag(FGameplayTag::RequestGameplayTag(FName("Itemization.Type.Mining"), false));
}

UMythicAbilityTagRelationshipMapping *UMythicDeveloperSettings::GetAbilityTagRelationshipMapping() const {
    return DefaultAbilityTagRelationshipMapping.Get();
}

void UMythicDeveloperSettings::GetStartupAssetPaths(TArray<FSoftObjectPath> &OutPaths) const {
    if (!DefaultAbilityTagRelationshipMapping.IsNull()) {
        OutPaths.Add(DefaultAbilityTagRelationshipMapping.ToSoftObjectPath());
    }
    if (!DefaultItemInputAbility.IsNull()) {
        OutPaths.Add(DefaultItemInputAbility.ToSoftObjectPath());
    }
    if (!DefaultAchievementSet.IsNull()) {
        OutPaths.Add(DefaultAchievementSet.ToSoftObjectPath());
    }
    if (!DefaultUnlockRuleSet.IsNull()) {
        OutPaths.Add(DefaultUnlockRuleSet.ToSoftObjectPath());
    }
    if (!DefaultCodexLibrary.IsNull()) {
        OutPaths.Add(DefaultCodexLibrary.ToSoftObjectPath());
    }
    if (!DefaultRenownTierTable.IsNull()) {
        OutPaths.Add(DefaultRenownTierTable.ToSoftObjectPath());
    }
    if (!DefaultTitleRegistry.IsNull()) {
        OutPaths.Add(DefaultTitleRegistry.ToSoftObjectPath());
    }
    if (!LivingWorldSettings.IsNull()) {
        OutPaths.Add(LivingWorldSettings.ToSoftObjectPath());
    }
    if (!DefaultMountClass.IsNull()) {
        OutPaths.Add(DefaultMountClass.ToSoftObjectPath());
    }
    if (!RidingProficiency.IsNull()) {
        OutPaths.Add(RidingProficiency.ToSoftObjectPath());
    }
}
