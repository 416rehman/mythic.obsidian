#pragma once

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "MythicAssetManager.generated.h"

class UCraftableFragment;
class UItemDefinition;
class UConversionRecipe;

UCLASS()
class MYTHIC_API UMythicAssetManager : public UAssetManager {
    GENERATED_BODY()

public:
    UMythicAssetManager() {}
    virtual void StartInitialLoading() override;

    static UMythicAssetManager &Get();


    template <typename T, typename CallbackType>
    static void LoadAsync(UObject *Caller, const TSoftObjectPtr<T> &AssetPtr, CallbackType &&OnLoaded);

    template <typename T, typename CallbackType>
    static void LoadAsync(UObject *Caller, const TSoftClassPtr<T> &ClassPtr, CallbackType &&OnLoaded);

    template <typename CallbackType>
    static void LoadAsync(UObject *Caller, const TArray<FSoftObjectPath> &AssetPaths, CallbackType &&OnLoaded);


    static const FPrimaryAssetType ItemDefinitionType;

    static const FPrimaryAssetType ConversionRecipeType;

    static const FPrimaryAssetType StatCategoryDefinitionType;
    static const FPrimaryAssetType StatDefinitionType;
    static const FPrimaryAssetType AffixDefinitionType;
    static const FPrimaryAssetType AffixPoolType;
    static const FPrimaryAssetType AffixRollPolicyType;
    static const FPrimaryAssetType AffixProfileType;
    static const FPrimaryAssetType ItemizationRulesetType;
    static const FPrimaryAssetType HarvestToolTypeDefinitionType;
    static const FPrimaryAssetType HarvestableDefinitionType;
    static const FPrimaryAssetType ProficiencyDefinitionType;

private:
    TMap<FSoftObjectPath, TSharedPtr<FStreamableHandle>> ActiveHandles;

    void StoreHandle(const FSoftObjectPath &AssetPath, TSharedPtr<FStreamableHandle> Handle);

    void LoadAssetInternal(const FSoftObjectPath &AssetPath, TFunction<void(UObject *)> OnLoaded, TAsyncLoadPriority Priority);
    void LoadAssetsInternal(const TArray<FSoftObjectPath> &AssetPaths, TFunction<void(const TArray<UObject *> &)> OnLoaded, TAsyncLoadPriority Priority);
};


template <typename T, typename CallbackType>
void UMythicAssetManager::LoadAsync(UObject *Caller, const TSoftObjectPtr<T> &AssetPtr, CallbackType &&OnLoaded) {
    if (!Caller) {
        return;
    }

    if (T *Asset = AssetPtr.Get()) {
        OnLoaded(Asset);
        return;
    }

    if (AssetPtr.IsNull()) {
        OnLoaded(nullptr);
        return;
    }

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

    if (UClass *Class = ClassPtr.Get()) {
        OnLoaded(TSubclassOf<T>(Class));
        return;
    }

    if (ClassPtr.IsNull()) {
        OnLoaded(TSubclassOf<T>(nullptr));
        return;
    }

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
