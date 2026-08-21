#include "ItemizationSubsystem.h"

#include "Mythic.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/Fragments/Passive/CraftableFragment.h"
#include "System/MythicAssetManager.h"

void UItemizationSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);

    UE_LOG(Myth, Log, TEXT("Caching item defs"));
    check(UMythicAssetManager::IsInitialized());

    UMythicAssetManager &AssetManager = UMythicAssetManager::Get();

    AssetManager.GetPrimaryAssetIdList(UMythicAssetManager::ItemDefinitionType, this->AllItemDefIds);

    AssetManager.LoadPrimaryAssets(AllItemDefIds, TArray<FName>(),
                                   FStreamableDelegate::CreateUObject(this, &UItemizationSubsystem::OnAllItemDefsLoaded));

    UE_LOG(Myth, Log, TEXT("Waiting for item defs to load"));
}

void UItemizationSubsystem::OnAllItemDefsLoaded() {
    UE_LOG(Myth, Log, TEXT("Item defs loaded - first pass"));
    UMythicAssetManager &AssetManager = UMythicAssetManager::Get();

    CachedItemDefs.Reset();
    CachedCraftableItems.Reset();
    CachedCraftableItemsByType.Reset();

    TSet<FPrimaryAssetId> RequiredItemAssets;

    for (const FPrimaryAssetId &AssetId : AllItemDefIds) {
        auto UObject = AssetManager.GetPrimaryAssetObject(AssetId);
        if (!UObject) {
            UE_LOG(Myth, Warning, TEXT("Failed to load item for identifier %s!"), *AssetId.ToString());
            continue;
        }

        auto ItemDef = Cast<UItemDefinition>(UObject);
        if (!IsValid(ItemDef)) {
            UE_LOG(Myth, Warning, TEXT("Loaded item for identifier %s is not an item definition!"), *AssetId.ToString());
            continue;
        }

        CachedItemDefs.Add(ItemDef);

        auto CraftableFragment = UCraftableFragment::GetCraftableFragmentFromDefinition(ItemDef);
        if (CraftableFragment) {
            CachedCraftableItems.Add(ItemDef);
            CachedCraftableItemsByType.FindOrAdd(ItemDef->ItemType).Add(ItemDef);

            for (const auto &Requirement : CraftableFragment->CraftingRequirements) {
                if (!Requirement.RequiredItem.IsNull()) {
                    FPrimaryAssetId RequiredAssetId = Requirement.RequiredItem->GetPrimaryAssetId();
                    if (RequiredAssetId.IsValid()) {
                        RequiredItemAssets.Add(RequiredAssetId);
                    }
                }
            }
        }
    }

    RequiredItemAssets = RequiredItemAssets.Difference(TSet<FPrimaryAssetId>(AllItemDefIds));

    if (RequiredItemAssets.Num() > 0) {
        UE_LOG(Myth, Log, TEXT("Loading %d additional crafting requirement assets"), RequiredItemAssets.Num());

        TArray<FPrimaryAssetId> RequiredItemArray = RequiredItemAssets.Array();

        AssetManager.LoadPrimaryAssets(RequiredItemArray, TArray<FName>(),
                                       FStreamableDelegate::CreateUObject(this, &UItemizationSubsystem::OnCraftingRequirementsLoaded));
    }
    else {
        ProcessCraftingRequirements();
    }
}

void UItemizationSubsystem::OnCraftingRequirementsLoaded() {
    UE_LOG(Myth, Log, TEXT("Crafting requirement assets loaded"));
    ProcessCraftingRequirements();
}

void UItemizationSubsystem::ProcessCraftingRequirements() {
    UMythicAssetManager &AssetManager = UMythicAssetManager::Get();

    CraftingIngredientsIds.Reset();

    for (UItemDefinition *ItemDef : CachedItemDefs) {
        for (int32 i = 0; i < ItemDef->Fragments.Num(); i++) {
            if (!ItemDef->Fragments[i]) {
                continue;
            }

            if (auto CraftableFragment = Cast<UCraftableFragment>(ItemDef->Fragments[i])) {
                for (const auto &Requirement : CraftableFragment->CraftingRequirements) {
                    UItemDefinition *RequirementItem = Requirement.RequiredItem.Get();
                    if (!RequirementItem) {
                        FPrimaryAssetId RequiredAssetId = Requirement.RequiredItem->GetPrimaryAssetId();
                        if (RequiredAssetId.IsValid()) {
                            auto RequiredObject = AssetManager.GetPrimaryAssetObject(RequiredAssetId);
                            RequirementItem = Cast<UItemDefinition>(RequiredObject);
                        }
                    }

                    if (!RequirementItem) {
                        UE_LOG(Myth, Error, TEXT("Item %s has an unloaded/invalid requirement: %s"),
                               *ItemDef->GetName(), *Requirement.RequiredItem.ToString());
                        continue;
                    }

                    CraftingIngredientsIds.Add(RequirementItem->GetPrimaryAssetId());
                }
            }
        }
    }

    UE_LOG(Myth, Log, TEXT("Itemization subsystem fully initialized - Cached %d item defs, %d crafting ingredients"),
           CachedItemDefs.Num(), CraftingIngredientsIds.Num());
}

void UItemizationSubsystem::Deinitialize() {
    Super::Deinitialize();

    UMythicAssetManager &AssetManager = UMythicAssetManager::Get();
    AssetManager.UnloadPrimaryAssets(AllItemDefIds);
}

bool UItemizationSubsystem::IsCraftingIngredient(UItemDefinition *Item) const {
    if (!Item) {
        return false;
    }
    return CraftingIngredientsIds.Contains(Item->GetPrimaryAssetId());
}

UItemDefinition *UItemizationSubsystem::GetItemDefinition(FPrimaryAssetId ItemId) const {
    if (!ItemId.IsValid()) {
        return nullptr;
    }

    auto ItemDef = CachedItemDefs.FindByPredicate([ItemId](const UItemDefinition *Item) {
        return Item && Item->GetPrimaryAssetId() == ItemId;
    });

    if (ItemDef) {
        return *ItemDef;
    }

    UE_LOG(Myth, Log, TEXT("Item def not found in cache, trying to load it: %s"), *ItemId.ToString());

    UMythicAssetManager &AssetManager = UMythicAssetManager::Get();
    auto ItemObject = AssetManager.GetPrimaryAssetObject(ItemId);
    if (!ItemObject) {
        UE_LOG(Myth, Warning, TEXT("Failed to load item for identifier %s!"), *ItemId.ToString());
        return nullptr;
    }

    auto LoadedItemDef = Cast<UItemDefinition>(ItemObject);
    if (!LoadedItemDef) {
        UE_LOG(Myth, Warning, TEXT("Object for identifier %s is not an ItemDefinition!"), *ItemId.ToString());
        return nullptr;
    }

    return LoadedItemDef;
}

void UItemizationSubsystem::GetCraftableItemsByType(FGameplayTag ItemType, TArray<UItemDefinition *> &OutItems) const {
    OutItems.Reset();

    if (const auto *Items = CachedCraftableItemsByType.Find(ItemType)) {
        OutItems = *Items;
    }
}
