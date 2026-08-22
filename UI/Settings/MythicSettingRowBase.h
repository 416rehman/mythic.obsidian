
#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "UI/Settings/MythicSettingDefinition.h"
#include "MythicSettingRowBase.generated.h"

class UCommonTextBlock;
class UButton;
class USlider;
class UEditableTextBox;
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
    void ApplyFocusVisuals(bool bFocused);

    /** Hover is a quieter version of focus: the ground lifts, the bar does not. */
    void ApplyHoverVisuals(bool bHovered);

    UFUNCTION()
    void HandleStepDown();

    UFUNCTION()
    void HandleStepUp();

    UFUNCTION()
    void HandleSwitchClicked();

    UFUNCTION()
    void HandleLeftHovered();

    UFUNCTION()
    void HandleRightHovered();

    UFUNCTION()
    void HandleChevronUnhovered();

    UFUNCTION()
    void HandleSliderValue(float NewValue);

    UFUNCTION()
    void HandleValueTyped(const FText &Text, ETextCommit::Type CommitType);

    bool bHovered = false;

    /** True while PushToWidgets is writing, so a widget callback cannot write straight back. */
    bool bPushingToWidgets = false;

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
    virtual void NativeOnMouseEnter(const FGeometry &Geo, const FPointerEvent &Event) override;
    virtual void NativeOnMouseLeave(const FPointerEvent &Event) override;
    virtual FReply NativeOnMouseMove(const FGeometry &Geo, const FPointerEvent &Event) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry &Geo, const FPointerEvent &Event) override;

    /**
     * The Blueprint owns the tree, the art and the animation; these are only where the values land.
     * All optional, so a row kind that has no value cell or no switch simply leaves it unbound.
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Text_Label;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Text_Value;

    /** Row ground and the focus bar down its left edge. Shown only while this row has focus. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Img_Ground;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Img_FocusBar;

    /** Stepper chevrons. Tinted rather than swapped, so one texture covers idle and focused. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Img_Left;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Img_Right;

    /** Scaled horizontally to the normalised value: a render transform, not a layout change. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Img_Fill;

    /** The switch pill and the slider track share a name because they play the same role. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Img_Track;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Img_TrackRim;

    /** Switch knob and slider handle - the same dot, moved. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Img_Knob;

    /**
     * Hit areas, one per thing a player can actually press.
     *
     * The row itself only SELECTS. Pressing anywhere on a row to change its value means you cannot rest a
     * cursor on a setting to read what it does without altering it, and it leaves the two chevrons as
     * decoration rather than the controls they look like.
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Left;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Right;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UButton> Btn_Switch;

    /**
     * Drag is handled against the track's own geometry rather than by a USlider.
     *
     * A USlider reserved its space and painted nothing here - correct style, correct textures, correct
     * size, no pixels - while plain Images in the same slot drew fine. Rather than keep guessing at it,
     * the track is three Images and the drag is fifteen lines against Img_Track's cached geometry.
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<USlider> Slider_Value;

    bool bDraggingTrack = false;

    /** Where the drag currently is, 0..1, or -1 when not dragging. Committed once on release. */
    float DragAlpha = -1.0f;

    /** Move the handle and the number without touching the setting. */
    void PreviewNormalised(float Normalised);

    /** 0..1 for a cursor position along the track, or -1 when the cursor is not over it. */
    float TrackAlphaAt(const FPointerEvent &Event) const;

    /** Reads as plain text until clicked, then accepts a typed number. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UEditableTextBox> Edit_Value;

    /**
     * Colour comes from the shared UI style, not from here.
     *
     * These were eight literals that duplicated tokens the project already had and then drifted from
     * them. A row asks FMythicUIStyle for a ROLE - label, accent, trough - so a retune lands on every
     * screen at once instead of on whichever ones remembered to copy the new value.
     */

    /** How dark a row sits when it is neither hovered nor focused. Enough to hold text over daylight. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Settings|Palette", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float RestingGroundOpacity = 0.34f;

    /** How far the switch knob slides, in pixels. 46 wide, 16 knob, 4 inset each side. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Settings")
    float SwitchTravel = 22.0f;

    /**
     * Slider track and handle size, in pixels.
     *
     * The handle travels TrackWidth MINUS its own width, because it is aligned to the track's left edge:
     * translating by the full width puts it entirely past the end, which is why it drifted off the track
     * as the value rose. Authored rather than measured, so it does not depend on when layout last ran.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Settings")
    float TrackWidth = 150.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Settings")
    float KnobSize = 12.0f;

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
