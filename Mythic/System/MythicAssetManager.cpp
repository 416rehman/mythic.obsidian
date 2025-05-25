// 

#include "MythicAssetManager.h"
#include "AbilitySystemGlobals.h"
#include "Mythic.h"
#include "Itemization/Inventory/ItemDefinition.h"

const FPrimaryAssetType UMythicAssetManager::ItemDefinitionType = TEXT("ItemDefinition");

void UMythicAssetManager::StartInitialLoading() {
    Super::StartInitialLoading();

    UAbilitySystemGlobals::Get().InitGlobalData();
}

UMythicAssetManager &UMythicAssetManager::Get() {
    auto This = Cast<UMythicAssetManager>(GEngine->AssetManager);

    if (This) {
        return *This;
    }
    else {
        UE_LOG(Mythic, Fatal, TEXT("Invalid AssetManager in DefaultEngine.ini, must be MythicAssetManager!"));
        return *NewObject<UMythicAssetManager>(); // never calls this
    }
}

UItemDefinition *UMythicAssetManager::ForceLoadItemDefinition(const FPrimaryAssetId &PrimaryAssetId, bool bLogWarning) {
    FSoftObjectPath ItemPath = GetPrimaryAssetPath(PrimaryAssetId);
    // This does a synchronous load and may hitch
    UItemDefinition *LoadedItemDef = Cast<UItemDefinition>(ItemPath.TryLoad());

    if (bLogWarning && LoadedItemDef == nullptr) {
        UE_LOG(Mythic, Warning, TEXT("Failed to load item for identifier %s!"), *PrimaryAssetId.ToString());
    }

    return LoadedItemDef;
}