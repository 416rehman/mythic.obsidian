#include "MythicAssetManager.h"

#include "AbilitySystemGlobals.h"
#include "Mythic.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Settings/MythicDeveloperSettings.h"

const FPrimaryAssetType UMythicAssetManager::ItemDefinitionType = TEXT("ItemDefinition");

void UMythicAssetManager::StartInitialLoading() {
    Super::StartInitialLoading();

    UAbilitySystemGlobals::Get().InitGlobalData();

    // Preload startup assets from developer settings
    TArray<FSoftObjectPath> StartupPaths;
    if (const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>()) {
        Settings->GetStartupAssetPaths(StartupPaths);
    }

    if (!StartupPaths.IsEmpty()) {
        UE_LOG(Myth, Log, TEXT("PreloadStartupAssets: Preloading %d startup assets"), StartupPaths.Num());
        LoadAssetsInternal(
            StartupPaths,
            [](const TArray<UObject *> &LoadedAssets) {
                int32 SuccessCount = 0;
                for (UObject *Asset : LoadedAssets) {
                    if (Asset) {
                        SuccessCount++;
                    }
                }
                UE_LOG(Myth, Log, TEXT("PreloadStartupAssets: Loaded %d/%d"), SuccessCount, LoadedAssets.Num());
            },
            FStreamableManager::AsyncLoadHighPriority);
    }
}

UMythicAssetManager &UMythicAssetManager::Get() {
    auto This = Cast<UMythicAssetManager>(GEngine->AssetManager);

    if (This) {
        return *This;
    }
    else {
        UE_LOG(Myth, Fatal, TEXT("Invalid AssetManager in DefaultEngine.ini, must be MythicAssetManager!"));
        return *NewObject<UMythicAssetManager>(); // never calls this
    }
}

void UMythicAssetManager::StoreHandle(const FSoftObjectPath &AssetPath, TSharedPtr<FStreamableHandle> Handle) {
    if (Handle.IsValid()) {
        ActiveHandles.Add(AssetPath, Handle);
    }
}

void UMythicAssetManager::LoadAssetInternal(
    const FSoftObjectPath &AssetPath,
    TFunction<void(UObject *)> OnLoaded,
    TAsyncLoadPriority Priority) {
    if (!AssetPath.IsValid()) {
        UE_LOG(Myth, Warning, TEXT("LoadAssetInternal: Invalid asset path"));
        OnLoaded(nullptr);
        return;
    }

    // Check if already loaded
    if (UObject *LoadedAsset = AssetPath.ResolveObject()) {
        OnLoaded(LoadedAsset);
        return;
    }

    // Async load
    FSoftObjectPath PathCopy = AssetPath;
    TSharedPtr<FStreamableHandle> Handle = GetStreamableManager().RequestAsyncLoad(
        AssetPath,
        FStreamableDelegate::CreateLambda([PathCopy, OnLoaded = MoveTemp(OnLoaded)]() {
            UObject *LoadedAsset = PathCopy.ResolveObject();
            if (!LoadedAsset) {
                UE_LOG(Myth, Warning, TEXT("LoadAssetInternal: Failed to load %s"), *PathCopy.ToString());
            }
            OnLoaded(LoadedAsset);
        }),
        Priority,
        false,
        false,
        TEXT("MythicAsyncLoad"));

    if (Handle.IsValid()) {
        StoreHandle(AssetPath, Handle);
    }
}

void UMythicAssetManager::LoadAssetsInternal(
    const TArray<FSoftObjectPath> &AssetPaths,
    TFunction<void(const TArray<UObject *> &)> OnLoaded,
    TAsyncLoadPriority Priority) {
    if (AssetPaths.IsEmpty()) {
        OnLoaded(TArray<UObject *>());
        return;
    }

    // Filter to only paths that need loading
    TArray<FSoftObjectPath> PathsToLoad;
    TArray<FSoftObjectPath> AllPaths = AssetPaths;

    for (const FSoftObjectPath &Path : AssetPaths) {
        if (Path.IsValid() && !Path.ResolveObject()) {
            PathsToLoad.Add(Path);
        }
    }

    // If everything is already loaded, callback immediately
    if (PathsToLoad.IsEmpty()) {
        TArray<UObject *> Results;
        Results.Reserve(AssetPaths.Num());
        for (const FSoftObjectPath &Path : AssetPaths) {
            Results.Add(Path.ResolveObject());
        }
        OnLoaded(Results);
        return;
    }

    // Async load the batch
    TSharedPtr<FStreamableHandle> Handle = GetStreamableManager().RequestAsyncLoad(
        PathsToLoad,
        FStreamableDelegate::CreateLambda([AllPaths, OnLoaded = MoveTemp(OnLoaded)]() {
            TArray<UObject *> Results;
            Results.Reserve(AllPaths.Num());
            for (const FSoftObjectPath &Path : AllPaths) {
                Results.Add(Path.ResolveObject());
            }
            OnLoaded(Results);
        }),
        Priority,
        false,
        false,
        TEXT("MythicAsyncLoadBatch"));

    if (Handle.IsValid()) {
        for (const FSoftObjectPath &Path : PathsToLoad) {
            StoreHandle(Path, Handle);
        }
    }
}
