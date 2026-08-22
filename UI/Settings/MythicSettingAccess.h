
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

    /**
     * Staging, so a settings screen is a proposal and NOTHING reaches the game until Apply.
     *
     * A change is buffered: the row shows the value you picked, the renderer never hears about it. Apply
     * replays every buffered change for real and saves; leaving discards them. That is what makes the
     * screen safe to explore - and it is also why changing a heavy setting no longer hitches, because
     * stepping through five quality levels now costs five map writes rather than five scalability
     * rebuilds.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    static void BeginStaging();

    /** True once anything has been changed and not yet committed. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    static bool HasStagedChanges();

    /** Keep everything currently previewing. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    static void CommitStaged();

    /** Put every previewed setting back to the value it had when the screen opened. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    static void RevertStaged();

    /** The value a row should display: the buffered one if the player changed it, else the live one. */
    static float ReadCommittedValue(const FMythicSettingDefinition &Def);

private:
    /** The live value, ignoring anything buffered. */
    static float ReadValueUncached(const FMythicSettingDefinition &Def);

public:

private:
    /** One buffered change, holding enough to replay it for real on Apply. */
    struct FPending {
        FMythicSettingDefinition Def;
        float Value = 0.0f;
        int32 OptionIndex = INDEX_NONE;
    };

    /** Source name -> what the player asked for. Nothing here has touched the game yet. */
    static TMap<FName, FPending> PendingChanges;

    /** The real write. Only Apply calls this. */
    static void ApplyValueForReal(const FMythicSettingDefinition &Def, float Value);

public:
};
