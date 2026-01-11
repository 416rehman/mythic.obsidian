#include "MythicDeveloperSettings.h"

#include "GAS/MythicAbilityTagRelationshipMapping.h"

UMythicDeveloperSettings::UMythicDeveloperSettings() {}

UMythicAbilityTagRelationshipMapping *UMythicDeveloperSettings::GetAbilityTagRelationshipMapping() const {
    // Get() returns the already-loaded asset, or nullptr if not loaded yet
    // The async preloading happens in MythicAsyncLoadingSubsystem::Initialize()
    return DefaultAbilityTagRelationshipMapping.Get();
}

void UMythicDeveloperSettings::GetStartupAssetPaths(TArray<FSoftObjectPath> &OutPaths) const {
    if (!DefaultAbilityTagRelationshipMapping.IsNull()) {
        OutPaths.Add(DefaultAbilityTagRelationshipMapping.ToSoftObjectPath());
    }
    if (!DefaultItemInputAbility.IsNull()) {
        OutPaths.Add(DefaultItemInputAbility.ToSoftObjectPath());
    }
}
