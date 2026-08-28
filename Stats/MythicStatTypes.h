// Copyright Stellar Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManagerTypes.h"
#include "UObject/SoftObjectPtr.h"
#include "MythicStatTypes.generated.h"

class UTexture2D;
class UMythicStatCategoryDefinition;
class UMythicStatDefinition;

/**
 * Canonical player-facing number formats.
 *
 * These explicit values preserve the serialized values of the enum that originally lived in
 * MythicStatDisplay.h. Stat and affix presentation must both consume this type; there is no parallel
 * percentage flag or UI-owned format enum.
 */
UENUM(BlueprintType)
enum class EMythicStatFormat : uint8 {
    Flat = 0,
    Integer = 1,
    Percent = 2,
    Multiplier = 3,
    PerSecond = 4,
    Bipolar = 5
};

/** Declares how the stat sheet evaluates a larger or smaller player-facing value. */
UENUM(BlueprintType)
enum class EMythicStatComparisonDirection : uint8 {
    HigherIsBetter,
    LowerIsBetter,
    Neutral
};

/** Identifies a stat as standalone, a current resource, or the capacity paired with that resource. */
UENUM(BlueprintType)
enum class EMythicStatPairRole : uint8 {
    None,
    Current,
    Capacity
};

/** Data-driven rule controlling whether a stat definition may appear on the general character stat sheet. */
UENUM(BlueprintType)
enum class EMythicStatSheetVisibility : uint8 {
    Always,
    WhenModifiedOrNonNeutral,
    Hidden
};

/** Typed authoring reference to a canonical stat-category definition. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicStatCategoryDefinitionHandle {
    GENERATED_BODY()

    /** Canonical category asset selected by designers; runtime identity is derived from this reference. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat",
              meta = (AssetBundles = "Runtime", DisplayName = "Stat Category Definition"))
    TSoftObjectPtr<UMythicStatCategoryDefinition> Asset;

    /** Returns the canonical Asset Manager identity without synchronously loading the referenced asset. */
    FPrimaryAssetId GetPrimaryAssetId() const;

    /** Returns the category asset only when it is already loaded; this function never synchronously loads it. */
    UMythicStatCategoryDefinition* GetAsset() const;

    /** Sets the canonical typed asset reference. */
    void SetAsset(UMythicStatCategoryDefinition* InAsset);

    bool IsValid() const;

    void Reset();

    friend bool operator==(const FMythicStatCategoryDefinitionHandle& Left,
                           const FMythicStatCategoryDefinitionHandle& Right) {
        return Left.Asset.ToSoftObjectPath() == Right.Asset.ToSoftObjectPath();
    }

    friend bool operator!=(const FMythicStatCategoryDefinitionHandle& Left,
                           const FMythicStatCategoryDefinitionHandle& Right) {
        return !(Left == Right);
    }
};

FORCEINLINE uint32 GetTypeHash(const FMythicStatCategoryDefinitionHandle& Handle) {
    return GetTypeHash(Handle.Asset.ToSoftObjectPath());
}

/** Typed authoring reference to a canonical stat definition. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicStatDefinitionHandle {
    GENERATED_BODY()

    /** Canonical stat asset selected by designers; runtime identity is derived from this reference. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat",
              meta = (AssetBundles = "Runtime", DisplayName = "Stat Definition"))
    TSoftObjectPtr<UMythicStatDefinition> Asset;

    /** Returns the canonical Asset Manager identity without synchronously loading the referenced asset. */
    FPrimaryAssetId GetPrimaryAssetId() const;

    /** Returns the stat asset only when it is already loaded; this function never synchronously loads it. */
    UMythicStatDefinition* GetAsset() const;

    /** Sets the canonical typed asset reference. */
    void SetAsset(UMythicStatDefinition* InAsset);

    bool IsValid() const;

    void Reset();

    friend bool operator==(const FMythicStatDefinitionHandle& Left, const FMythicStatDefinitionHandle& Right) {
        return Left.Asset.ToSoftObjectPath() == Right.Asset.ToSoftObjectPath();
    }

    friend bool operator!=(const FMythicStatDefinitionHandle& Left, const FMythicStatDefinitionHandle& Right) {
        return !(Left == Right);
    }
};

FORCEINLINE uint32 GetTypeHash(const FMythicStatDefinitionHandle& Handle) {
    return GetTypeHash(Handle.Asset.ToSoftObjectPath());
}

/** Canonical numeric formatting, precision, and suffix rules shared by stat and affix presentation. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicStatNumberPresentation {
    GENERATED_BODY()

    /** Player-facing numeric format used wherever this stat or one of its affix modifiers is displayed. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat")
    EMythicStatFormat Format = EMythicStatFormat::Flat;

    /** Number of fractional digits shown after applying Format. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat", meta = (ClampMin = "0", ClampMax = "4"))
    int32 DecimalPlaces = 0;

    /** Optional localized unit appended to the formatted value, such as metres or seconds. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat")
    FText UnitSuffix;
};

/** Data-authored stat-category layout and interaction treatment consumed by the stat sheet. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicStatCategoryStyle {
    GENERATED_BODY()

    /** Accent colour exposed to stat-sheet presentation for headings and category decoration. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat")
    FLinearColor AccentColor = FLinearColor::White;

    /** Data-authored row emphasis; avoids teaching the widget special category-tag names. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat")
    bool bEmphasizeRows = false;

    /** Enables the primary-stat contribution drill-down for rows in this category. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat")
    bool bEnableContributionDrilldown = false;

    /** Optional category icon loaded with the UI asset bundle. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat", meta = (AssetBundles = "UI"))
    TSoftObjectPtr<UTexture2D> Icon;
};

namespace MythicStatPresentation {
    /** Smallest source-value change that can affect this presentation at its authored precision. */
    MYTHIC_API float GetComparisonEpsilon(const FMythicStatNumberPresentation& Presentation);
}
