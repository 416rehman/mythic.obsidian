#include "MythicDeveloperSettings.h"

#include "GAS/MythicAbilityTagRelationshipMapping.h"

UMythicDeveloperSettings::UMythicDeveloperSettings() {}

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
