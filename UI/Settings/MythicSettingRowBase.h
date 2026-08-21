
#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "UI/Settings/MythicSettingDefinition.h"
#include "MythicSettingRowBase.generated.h"

class UMythicSettingsScreenBase;

/**
 * Behaviour for one settings row. It constructs NOTHING: the Widget Blueprint that derives from it owns
 * the entire widget tree, the layout, the materials and the animations. This class only holds the
 * definition, reads and writes the value through the catalog, and tells the Blueprint when to redraw.
 *
 * That split is the point. Rows used to be assembled in C++ out of boxes and text, which is why the
 * screen looked like raw Slate and why a setting could exist with no row at all.
 */
UCLASS(Abstract, Blueprintable)
class MYTHIC_API UMythicSettingRowBase : public UCommonUserWidget {
    GENERATED_BODY()

public:
    /** Hands this row the setting it represents. Calls OnDefinitionSet so the Blueprint can redraw. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetDefinition(const FMythicSettingDefinition &InDefinition, UMythicSettingsScreenBase *InScreen);

    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    const FMythicSettingDefinition &GetDefinition() const { return Definition; }

    /** The row's label. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    FText GetLabel() const { return Definition.Label; }

    /** The value as the player should read it, already formatted. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    FText GetValueText() const;

    /** 0..1 across the authored range, for a slider's fill. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    float GetNormalisedValue() const;

    /** Labels of every option this machine can actually use, in order. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    TArray<FText> GetOptionLabels() const;

    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    int32 GetOptionIndex() const;

    /** False when a requirement is unmet: the row shows its value but refuses input. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    bool IsAvailable() const;

    /** True when the value differs from its authored default, so the row can mark itself. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    bool IsChangedFromDefault() const;

    /** Steps a Select by Delta, clamped. Left and right on a stick land here. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void StepOption(int32 Delta);

    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetOptionIndex(int32 Index);

    /** Commits a slider position, 0..1 across the authored range. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetNormalisedValue(float Normalised);

    /** Returns just this row to its authored default. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void ResetToDefault();

    /** Redraw. The Blueprint does the drawing; this class never touches a widget. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Settings")
    void OnDefinitionSet();

    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Settings")
    void OnValueChanged();

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Settings")
    FMythicSettingDefinition Definition;

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Settings")
    TWeakObjectPtr<UMythicSettingsScreenBase> Screen;

    /** Tells the screen a staged change is waiting, and redraws this row. */
    void NotifyChanged();
};
