#pragma once

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "MythicAssetManager.generated.h"

class UCraftableFragment;
class UItemDefinition;

/**
 * Game implementation of asset manager with async loading utilities.
 * Set via AssetManagerClassName in DefaultEngine.ini
 *
 * ASYNC LOADING - Simple static API with automatic weak pointer safety:
 *
 *   UMythicAssetManager::LoadAsync(this, SoftObjectPtr, [this](UMyAsset* Asset) {
 *       // Callback only fires if 'this' is still valid
 *   });
 *
 *   UMythicAssetManager::LoadAsync(this, SoftClassPtr, [this](TSubclassOf<AActor> Class) {
 *       // Use the class
 *   });
 */
UCLASS()
class MYTHIC_API UMythicAssetManager : public UAssetManager {
    GENERATED_BODY()

public:
    UMythicAssetManager() {}
    virtual void StartInitialLoading() override;

    /** Returns the current AssetManager singleton */
    static UMythicAssetManager &Get();

    // ==================== ASYNC LOADING API ====================
    // Static methods with automatic weak pointer safety

    /**
     * Load a single asset asynchronously. Callback only fires if Caller is still valid.
     * @param Caller The UObject requesting the load (used for weak ref safety)
     * @param AssetPtr Soft object pointer to load
     * @param OnLoaded Callback when loading completes
     */
    template <typename T, typename CallbackType>
    static void LoadAsync(UObject *Caller, const TSoftObjectPtr<T> &AssetPtr, CallbackType &&OnLoaded);

    /**
     * Load a single class asynchronously. Callback only fires if Caller is still valid.
     * @param Caller The UObject requesting the load
     * @param ClassPtr Soft class pointer to load
     * @param OnLoaded Callback when loading completes
     */
    template <typename T, typename CallbackType>
    static void LoadAsync(UObject *Caller, const TSoftClassPtr<T> &ClassPtr, CallbackType &&OnLoaded);

    /**
     * Load multiple assets asynchronously. Callback only fires if Caller is still valid.
     * @param Caller The UObject requesting the load
     * @param AssetPaths Array of soft object paths to load
     * @param OnLoaded Callback when all loading completes
     */
    template <typename CallbackType>
    static void LoadAsync(UObject *Caller, const TArray<FSoftObjectPath> &AssetPaths, CallbackType &&OnLoaded);

    // ==================== PRIMARY ASSETS ====================

    /** Static types for items */
    static const FPrimaryAssetType ItemDefinitionType;

private:
    /** Active load handles to keep assets in memory */
    TMap<FSoftObjectPath, TSharedPtr<FStreamableHandle>> ActiveHandles;

    /** Store a handle to keep the asset loaded */
    void StoreHandle(const FSoftObjectPath &AssetPath, TSharedPtr<FStreamableHandle> Handle);

    /** Internal load implementations */
    void LoadAssetInternal(const FSoftObjectPath &AssetPath, TFunction<void(UObject *)> OnLoaded, TAsyncLoadPriority Priority);
    void LoadAssetsInternal(const TArray<FSoftObjectPath> &AssetPaths, TFunction<void(const TArray<UObject *> &)> OnLoaded, TAsyncLoadPriority Priority);
};

// ==================== Template Implementations ====================

template <typename T, typename CallbackType>
void UMythicAssetManager::LoadAsync(UObject *Caller, const TSoftObjectPtr<T> &AssetPtr, CallbackType &&OnLoaded) {
    if (!Caller) {
        return;
    }

    // If already loaded, callback immediately
    if (T *Asset = AssetPtr.Get()) {
        OnLoaded(Asset);
        return;
    }

    // If null, callback with nullptr
    if (AssetPtr.IsNull()) {
        OnLoaded(nullptr);
        return;
    }

    // Async load with weak pointer safety
    TWeakObjectPtr<UObject> WeakCaller(Caller);
    FSoftObjectPath Path = AssetPtr.ToSoftObjectPath();

    Get().LoadAssetInternal(Path, [WeakCaller, Callback = Forward<CallbackType>(OnLoaded)](UObject *LoadedAsset) mutable {
        if (WeakCaller.IsValid()) {
            Callback(Cast<T>(LoadedAsset));
        }
    }, FStreamableManager::DefaultAsyncLoadPriority);
}

template <typename T, typename CallbackType>
void UMythicAssetManager::LoadAsync(UObject *Caller, const TSoftClassPtr<T> &ClassPtr, CallbackType &&OnLoaded) {
    if (!Caller) {
        return;
    }

    // If already loaded, callback immediately
    if (UClass *Class = ClassPtr.Get()) {
        OnLoaded(TSubclassOf<T>(Class));
        return;
    }

    // If null, callback with nullptr
    if (ClassPtr.IsNull()) {
        OnLoaded(TSubclassOf<T>(nullptr));
        return;
    }

    // Async load with weak pointer safety
    TWeakObjectPtr<UObject> WeakCaller(Caller);
    FSoftObjectPath Path = ClassPtr.ToSoftObjectPath();

    Get().LoadAssetInternal(Path, [WeakCaller, Callback = Forward<CallbackType>(OnLoaded)](UObject *LoadedAsset) mutable {
        if (WeakCaller.IsValid()) {
            Callback(TSubclassOf<T>(Cast<UClass>(LoadedAsset)));
        }
    }, FStreamableManager::DefaultAsyncLoadPriority);
}

template <typename CallbackType>
void UMythicAssetManager::LoadAsync(UObject *Caller, const TArray<FSoftObjectPath> &AssetPaths, CallbackType &&OnLoaded) {
    if (!Caller) {
        return;
    }

    if (AssetPaths.IsEmpty()) {
        OnLoaded(TArray<UObject *>());
        return;
    }

    TWeakObjectPtr<UObject> WeakCaller(Caller);

    Get().LoadAssetsInternal(AssetPaths, [WeakCaller, Callback = Forward<CallbackType>(OnLoaded)](const TArray<UObject *> &LoadedAssets) mutable {
        if (WeakCaller.IsValid()) {
            Callback(LoadedAssets);
        }
    }, FStreamableManager::DefaultAsyncLoadPriority);
}
