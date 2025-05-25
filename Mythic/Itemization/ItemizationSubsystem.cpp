// 


#include "ItemizationSubsystem.h"

#include "Mythic.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/Fragments/Passive/CraftableFragment.h"
#include "System/MythicAssetManager.h"


void UItemizationSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);

    UE_LOG(Mythic, Log, TEXT("Caching item defs"));
    check(UMythicAssetManager::IsInitialized());

    UMythicAssetManager &AssetManager = UMythicAssetManager::Get();

    // Gather IDs of all item defs
    AssetManager.GetPrimaryAssetIdList(UMythicAssetManager::ItemDefinitionType, this->LoadedItemDefIds);

    // Load all the IDs - which then calls OnAssetsLoaded where we can get the actual item defs and cache them
    AssetManager.LoadPrimaryAssets(LoadedItemDefIds, TArray<FName>(),
                                   FStreamableDelegate::CreateUObject(this, &UItemizationSubsystem::OnAssetsLoaded));

    UE_LOG(Mythic, Log, TEXT("Waiting for item defs to load"));
}


// Items have been loaded, now we can use their AssetIDs to get the actual items
void UItemizationSubsystem::OnAssetsLoaded() {
    UE_LOG(Mythic, Log, TEXT("Item defs loaded"));
    UMythicAssetManager &AssetManager = UMythicAssetManager::Get();

    // Get all the item defs
    for (const FPrimaryAssetId &AssetId : LoadedItemDefIds) {
        // Get the Object for this ID
        auto UObject = AssetManager.GetPrimaryAssetObject(AssetId);
        if (!UObject) {
            UE_LOG(Mythic, Warning, TEXT("Failed to load item for identifier %s!"), *AssetId.ToString());
            continue;
        }

        // Cast the object to an item definition
        auto ItemDef = Cast<UItemDefinition>(UObject);
        if (!IsValid(ItemDef)) {
            UE_LOG(Mythic, Warning, TEXT("Loaded item for identifier %s is not an item definition!"), *AssetId.ToString());
            continue;
        }

        // Cache the item definition
        CachedItemDefs.Add(ItemDef);

        // Process Item Fragments
        for (int i = 0; i < ItemDef->Fragments.Num(); i++) {
            if (!ItemDef->Fragments[i]) {
                UE_LOG(Mythic, Error, TEXT("Item %s has an invalid fragment at index %d"), *ItemDef->GetName(), i);
                continue;
            }

            // CraftableFragment
            if (auto CraftableFragment = Cast<UCraftableFragment>(ItemDef->Fragments[i])) {
                // for of
                for (auto &Requirement : CraftableFragment->CraftingRequirements) {
                    // Get the item definition for the requirement
                    TSoftObjectPtr<UItemDefinition> RequirementItem = Requirement.RequiredItem.LoadSynchronous();
                    if (!RequirementItem.IsValid()) {
                        UE_LOG(Mythic, Error, TEXT("Item %s has an invalid requirement at index %d"), *ItemDef->GetName(), i);
                        continue;
                    }

                    // Add the asset ID to the crafting items set
                    CraftingIngredientsIds.Add(RequirementItem->GetPrimaryAssetId());
                }
            }
        }

        UE_LOG(Mythic, Log, TEXT("Cached %d item defs"), CachedItemDefs.Num());
    }
}

void UItemizationSubsystem::Deinitialize() {
    Super::Deinitialize();

    // Unload all the item defs
    UMythicAssetManager &AssetManager = UMythicAssetManager::Get();
    AssetManager.UnloadPrimaryAssets(LoadedItemDefIds);
}

bool UItemizationSubsystem::IsCraftingIngredient(UItemDefinition *Item) const {
    return CraftingIngredientsIds.Contains(Item->GetPrimaryAssetId());
}

UItemDefinition *UItemizationSubsystem::GetItemDefinition(FPrimaryAssetId ItemId) const {
    auto ItemDef = CachedItemDefs.FindByPredicate([ItemId](UItemDefinition *Item) {
        return Item->GetPrimaryAssetId() == ItemId;
    });

    if (ItemDef) {
        return *ItemDef;
    }

    UE_LOG(Mythic, Log, TEXT("Item def not found in cache, trying to load it"));

    // Try loading the item def
    UMythicAssetManager &AssetManager = UMythicAssetManager::Get();
    auto ItemObject = AssetManager.GetPrimaryAssetObject(ItemId);
    if (!ItemObject) {
        UE_LOG(Mythic, Warning, TEXT("Failed to load item for identifier %s!"), *ItemId.ToString());
        return nullptr;
    }

    return Cast<UItemDefinition>(ItemObject);
}
