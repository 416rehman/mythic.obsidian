
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UI/Settings/MythicSettingDefinition.h"
#include "MythicSettingAccess.generated.h"

/**
 * Reads and writes an authored setting without knowing what it is.
 *
 * This is what makes the catalog work: four generic doors - console variable, reflected property,
 * scalability group, and a short list of genuine specials - cover every setting in the game. Adding a
 * setting becomes a data edit, and a setting that exists in the catalog cannot fail to reach the screen.
 * Hand-written rows could: the ambient occlusion setting had working backing and no row for hours,
 * and nothing could detect it.
 */
UCLASS()
class MYTHIC_API UMythicSettingAccess : public UBlueprintFunctionLibrary {
    GENERATED_BODY()

public:
    /** Current raw value in the units the source stores. Falls back to DefaultValue when the source is missing. */
    UFUNCTION(BlueprintPure, Category = "Settings")
    static float ReadValue(const FMythicSettingDefinition &Def);

    UFUNCTION(BlueprintCallable, Category = "Settings")
    static void WriteValue(const FMythicSettingDefinition &Def, float Value);

    /** The option index a Select currently sits on, or INDEX_NONE when it has no options. */
    UFUNCTION(BlueprintPure, Category = "Settings")
    static int32 ReadOptionIndex(const FMythicSettingDefinition &Def);

    /** Chooses an option by index, applying the extra cvars that ride with it. */
    UFUNCTION(BlueprintCallable, Category = "Settings")
    static void WriteOptionIndex(const FMythicSettingDefinition &Def, int32 Index);

    /** Options this machine can actually use, in order. Unmet requirements are dropped, not offered greyed. */
    UFUNCTION(BlueprintPure, Category = "Settings")
    static TArray<FMythicSettingOption> GetAvailableOptions(const FMythicSettingDefinition &Def);

    /** Whether a named requirement is met. An unknown tag is MET, so a typo shows a working row. */
    UFUNCTION(BlueprintPure, Category = "Settings")
    static bool IsRequirementMet(FGameplayTag Requirement);

    UFUNCTION(BlueprintPure, Category = "Settings")
    static bool IsSettingAvailable(const FMythicSettingDefinition &Def);

    /** The player-facing string for the current value, honouring DisplayFormat and DisplayScale. */
    UFUNCTION(BlueprintPure, Category = "Settings")
    static FText GetDisplayText(const FMythicSettingDefinition &Def);

    /** True when the value is its authored default, so a row can mark itself as changed. */
    UFUNCTION(BlueprintPure, Category = "Settings")
    static bool IsAtDefault(const FMythicSettingDefinition &Def);

    /** Returns every setting to its authored default. One loop, so no setting can be forgotten. */
    UFUNCTION(BlueprintCallable, Category = "Settings")
    static void RestoreDefaults(const UMythicSettingsCatalog *Catalog);

    /** Whether the source this setting names actually resolves. Validation uses this to catch dead settings. */
    static bool ResolvesSource(const FMythicSettingDefinition &Def, FString &OutWhy);
};
