#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MythicDataAsset.generated.h"

/**
 * Base class for all Mythic Data Assets.
 * Provides generic JSON import/export functionality using DataConfig.
 */
UCLASS(Abstract)
class MYTHIC_API UMythicDataAsset : public UPrimaryDataAsset {
    GENERATED_BODY()

public:
#if WITH_EDITOR
    // Export to JSON
    void ExportAsJSONString(FString &OutJsonString) const;

    UFUNCTION(CallInEditor, Category = "MythicDataAsset")
    void ExportToJSON() const;

    UFUNCTION(CallInEditor, Category = "MythicDataAsset")
    void CopyJSONToClipboard() const;

    // Import from JSON
    UFUNCTION(CallInEditor, Category = "MythicDataAsset")
    void ImportFromJSON();

    UFUNCTION(CallInEditor, Category = "MythicDataAsset")
    void ImportFromClipboard();

protected:
    void ImportFromJSONString(const FString &JsonString);
#endif
};

UCLASS(BlueprintType)
class MYTHIC_API UMythicDataAssetJSONImportLibrary : public UBlueprintFunctionLibrary {
    GENERATED_BODY()

public:
#if WITH_EDITOR
    /**
     * Imports all JSON files from a selected folder into the specified package path.
     * @param AssetClass The class of asset to create (must inherit from UMythicDataAsset).
     * @param PackagePath The path where assets should be created (e.g. "/Game/Mythic/Itemization/Items/").
     */
    UFUNCTION(Blueprintable, BlueprintCallable, CallInEditor, Category = "Mythic|Editor")
    static void ImportDataAssetsFromFolder(TSubclassOf<UMythicDataAsset> AssetClass, FString PackagePath);
#endif
};
