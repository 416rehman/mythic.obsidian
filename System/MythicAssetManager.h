#pragma once

#include "Engine/AssetManager.h"
#include "MythicAssetManager.generated.h"

class UCraftableFragment;
class UItemDefinition;
/**
 * Game implementation of asset manager, overrides functionality and stores game-specific types
 * It is expected that most games will want to override AssetManager as it provides a good place for game-specific loading logic
 * This is used by setting AssetManagerClassName in DefaultEngine.ini
 *
 * LoadPrimaryAssets (To load from disk into mem) -> GetPrimaryAssetObject (To get the object) -> Cast<UItemDefinition> (To cast to the desired type)
 */
UCLASS()
class MYTHIC_API UMythicAssetManager : public UAssetManager
{
    GENERATED_BODY()

public:
    // Constructor and overrides
    UMythicAssetManager() {}
    virtual void StartInitialLoading() override;

    /** Static types for items */
    static const FPrimaryAssetType	ItemDefinitionType;
    static const FPrimaryAssetType	CraftingSchematicType;

    /** Returns the current AssetManager object */
    static UMythicAssetManager& Get();

    /**
     * Synchronously loads an ItemDefinition subclass, this can hitch but is useful when you cannot wait for an async load
     * This does not maintain a reference to the item so it will garbage collect if not loaded some other way
     *
     * @param PrimaryAssetId The asset identifier to load
     * @param bLogWarning If true, this will log a warning if the item failed to load
     */
    UItemDefinition* ForceLoadItemDefinition(const FPrimaryAssetId& PrimaryAssetId, bool bLogWarning = true);
};

