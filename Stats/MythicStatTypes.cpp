// Copyright Stellar Games. All Rights Reserved.

#include "Stats/MythicStatTypes.h"
#include "Engine/AssetManager.h"
#include "Stats/MythicStatCategoryDefinition.h"
#include "Stats/MythicStatDefinition.h"

namespace {
template <typename AssetType>
FPrimaryAssetId ResolveTypedPrimaryAssetId(const TSoftObjectPtr<AssetType>& Asset) {
    if (Asset.IsNull()) {
        return FPrimaryAssetId();
    }
    if (const AssetType* LoadedAsset = Asset.Get()) {
        return LoadedAsset->GetPrimaryAssetId();
    }
    if (const UAssetManager* AssetManager = UAssetManager::GetIfInitialized()) {
        return AssetManager->GetPrimaryAssetIdForPath(Asset.ToSoftObjectPath());
    }
    return FPrimaryAssetId();
}
}

FPrimaryAssetId FMythicStatCategoryDefinitionHandle::GetPrimaryAssetId() const {
    return ResolveTypedPrimaryAssetId(Asset);
}

UMythicStatCategoryDefinition* FMythicStatCategoryDefinitionHandle::GetAsset() const {
    return Asset.Get();
}

void FMythicStatCategoryDefinitionHandle::SetAsset(UMythicStatCategoryDefinition* InAsset) {
    Asset = InAsset;
}

bool FMythicStatCategoryDefinitionHandle::IsValid() const {
    return GetPrimaryAssetId().IsValid();
}

void FMythicStatCategoryDefinitionHandle::Reset() {
    Asset.Reset();
}

FPrimaryAssetId FMythicStatDefinitionHandle::GetPrimaryAssetId() const {
    return ResolveTypedPrimaryAssetId(Asset);
}

UMythicStatDefinition* FMythicStatDefinitionHandle::GetAsset() const {
    return Asset.Get();
}

void FMythicStatDefinitionHandle::SetAsset(UMythicStatDefinition* InAsset) {
    Asset = InAsset;
}

bool FMythicStatDefinitionHandle::IsValid() const {
    return GetPrimaryAssetId().IsValid();
}

void FMythicStatDefinitionHandle::Reset() {
    Asset.Reset();
}

float MythicStatPresentation::GetComparisonEpsilon(const FMythicStatNumberPresentation& Presentation) {
    // Integer and bipolar formatting intentionally force zero decimals. Comparison must use that same effective
    // precision or two values that render identically can still produce a misleading "+0" delta chip.
    const int32 DecimalPlaces =
        Presentation.Format == EMythicStatFormat::Integer
        || Presentation.Format == EMythicStatFormat::Bipolar
            ? 0
            : FMath::Clamp(Presentation.DecimalPlaces, 0, 4);
    const float DisplayScale =
        Presentation.Format == EMythicStatFormat::Percent || Presentation.Format == EMythicStatFormat::Multiplier
            ? 100.0f
            : 1.0f;
    return 0.5f * FMath::Pow(10.0f, -static_cast<float>(DecimalPlaces)) / DisplayScale;
}
