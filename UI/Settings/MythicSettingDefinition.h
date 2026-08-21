
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "MythicSettingDefinition.generated.h"

/** What the player manipulates. Decides which widget the row uses, nothing else. */
UENUM(BlueprintType)
enum class EMythicSettingControl : uint8 {
    Toggle,
    Select,
    Slider,
    Action,
    Keybind,
};

/**
 * Where the value actually lives. This is what makes a setting authorable: the row does not need code to
 * know how to read or write it, only which of these four doors to knock on.
 */
UENUM(BlueprintType)
enum class EMythicSettingSource : uint8 {
    // A console variable, by name. Covers nearly every graphics setting with no code at all.
    CVar,
    // A UPROPERTY on UMythicUserSettings, by name, reached through reflection.
    Property,
    // An engine scalability group: ViewDistance, Shadows, GlobalIllumination, Reflections, Textures,
    // Effects, PostProcess, Foliage, Shading.
    Scalability,
    // Resolution, window mode, key rebinding - the handful that genuinely cannot be expressed generically.
    Special,
};

/** One entry in a Select control. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicSettingOption {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setting")
    FText Label;

    // The value written to the source when this option is chosen.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setting")
    float Value = 0.0f;

    /**
     * Extra console variables set alongside the main one when this option is chosen. This is how a profile
     * rides an option - picking GTAO also sets its angle count and filter - without a line of code.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setting")
    TMap<FName, float> ExtraCVars;

    /** Named requirement this option needs. Unmet, the option is not offered at all. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setting", meta = (Categories = "Settings.Requires"))
    FGameplayTag Requires;
};

/**
 * One authored setting. Adding a setting to the game is adding one of these to the catalog: no C++, no
 * hand-written row, and nothing that can silently fail to appear.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicSettingDefinition {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setting", meta = (Categories = "Settings.Id"))
    FGameplayTag Id;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setting")
    FText Label;

    /** Shown for the focused row. Say what it does and what it costs, in plain English. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setting", meta = (MultiLine = true))
    FText Description;

    /** The tab this belongs to. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setting", meta = (Categories = "Settings.Category"))
    FGameplayTag Category;

    /** The sub-heading within the tab, so a tab reads as sections rather than a list. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setting")
    FText Group;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setting")
    EMythicSettingControl Control = EMythicSettingControl::Select;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setting")
    EMythicSettingSource Source = EMythicSettingSource::CVar;

    /** The cvar name, property name, scalability group, or special key - whichever Source names. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setting")
    FName SourceName;

    /** Select only. Order is the order the player steps through. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setting")
    TArray<FMythicSettingOption> Options;

    /** Slider only. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setting")
    float MinValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setting")
    float MaxValue = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setting", meta = (ClampMin = "0.001"))
    float StepSize = 0.05f;

    /** Appended to the readout, e.g. "%" or " fps". A suffix and a decimal count beat printf syntax here. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setting")
    FString DisplaySuffix;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setting", meta = (ClampMin = "0", ClampMax = "3"))
    int32 DisplayDecimals = 0;

    /** Multiplies the stored value for display only, so a 0..1 stored value can read as 0..100. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setting")
    float DisplayScale = 1.0f;

    /** Staged behind Apply rather than taking effect as it changes. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setting")
    bool bNeedsApply = false;

    /** Named requirement for the whole row. Unmet, the row is shown disabled rather than hidden. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setting", meta = (Categories = "Settings.Requires"))
    FGameplayTag Requires;

    /** The value Restore Defaults returns this to. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setting")
    float DefaultValue = 0.0f;
};

/** A tab, and the order its groups appear in. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicSettingCategory {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setting", meta = (Categories = "Settings.Category"))
    FGameplayTag Id;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setting")
    FText Label;

    /** Group order within this tab. A group not listed here falls to the end in authoring order. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setting")
    TArray<FText> GroupOrder;
};

/**
 * Every setting in the game, as data. The settings screen renders whatever this holds, so adding a setting
 * is an edit here and nothing else - which is the whole point: a setting that exists cannot fail to appear.
 */
UCLASS(BlueprintType)
class MYTHIC_API UMythicSettingsCatalog : public UDataAsset {
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
    TArray<FMythicSettingCategory> Categories;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
    TArray<FMythicSettingDefinition> Settings;

    /** Settings in this category, in authored order. */
    UFUNCTION(BlueprintPure, Category = "Settings")
    TArray<FMythicSettingDefinition> GetSettingsInCategory(FGameplayTag CategoryId) const;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(class FDataValidationContext &Context) const override;
#endif
};
