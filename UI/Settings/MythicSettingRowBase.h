
#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "UI/Settings/MythicSettingDefinition.h"
#include "MythicSettingRowBase.generated.h"

class UCommonTextBlock;
class UImage;
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
    /** Re-reads the live value and tells the Blueprint to redraw. Used after Restore Defaults. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void Redraw();

    /**
     * Left or right on the row: steps a select, nudges a slider by one authored step, flips a toggle.
     * One focus stop per row rather than one per control - you move down the list, not into it.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void Nudge(int32 Delta);

    /** Accept on the row: flips a toggle, advances a select, fires an action. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void ActivateRow();

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

    /** Draw the focus state. The Blueprint owns what focus looks like; this class only reports it. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Settings")
    void OnFocusChanged(bool bFocused);

    /** A row the player cannot act on: an action fired, or a keybind waiting for a key. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Settings")
    void OnActionTriggered();

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnFocusReceived(const FGeometry &Geo, const FFocusEvent &Event) override;
    virtual void NativeOnFocusLost(const FFocusEvent &Event) override;
    virtual FReply NativeOnKeyDown(const FGeometry &Geo, const FKeyEvent &Event) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry &Geo, const FPointerEvent &Event) override;

    /**
     * The Blueprint owns the tree, the art and the animation; these are only where the values land.
     * All optional, so a row kind that has no value cell or no switch simply leaves it unbound.
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Text_Label;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Text_Value;

    /** Scaled horizontally to the normalised value: a render transform, not a layout change. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Img_Fill;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Img_Thumb;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Img_Switch;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> FocusRing;

    /** Brushes for the two switch states, so a toggle reads at a glance instead of by its words. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Settings")
    FSlateBrush SwitchOnBrush;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Settings")
    FSlateBrush SwitchOffBrush;

    /** How far the slider thumb travels, in pixels: the trough width less the thumb. Authored per row kind. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Settings")
    float ThumbTravel = 86.0f;

    /** Dimmed when a requirement is unmet, so an unavailable row still reads but looks inert. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Settings")
    FLinearColor UnavailableTint = FLinearColor(1.0f, 1.0f, 1.0f, 0.35f);

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Settings")
    FMythicSettingDefinition Definition;

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Settings")
    TWeakObjectPtr<UMythicSettingsScreenBase> Screen;

    /** Tells the screen a staged change is waiting, and redraws this row. */
    void NotifyChanged();

    /** Pushes label, value and control state into whichever of the bound widgets exist. */
    void PushToWidgets();
};
