
#include "UI/Settings/MythicSettingRowBase.h"

#include "CommonTextBlock.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "UI/Kit/MythicKitInputs.h"
#include "UI/MythicUIStyle.h"
#include "UI/Settings/MythicSettingAccess.h"
#include "UI/Settings/MythicSettingsScreenBase.h"

void UMythicSettingRowBase::NativeConstruct() {
    Super::NativeConstruct();

    // The label owns the leftover width and truncates with an ellipsis; the value cell hugs its content. Enforced
    // here because a style reapply or a WBP slot left at Auto lets a long label paint into the value.
    if (Text_Label) {
        if (UHorizontalBoxSlot *LabelSlot = Cast<UHorizontalBoxSlot>(Text_Label->Slot)) {
            LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }
        Text_Label->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
    }

    if (Img_Ground) {
        Img_Ground->SetVisibility(ESlateVisibility::HitTestInvisible);
        Img_Ground->SetRenderOpacity(RestingGroundOpacity);
    }

    // A heading is text, not a stop. Letting it take focus makes a pad feel broken: the row lights up and
    // then answers nothing.
    SetIsFocusable(!UMythicSettingsScreenBase::IsGroupHeading(Definition));

    /**
     * The hit areas are for the MOUSE only.
     *
     * A focusable button inside a row hijacks stick navigation: the pad walks into the chevron instead of
     * moving to the next setting, and the row it belongs to can never be left. On a pad, Left and Right on
     * the focused row already do exactly what these buttons do.
     */
    for (UButton *Hit : {Btn_Left.Get(), Btn_Right.Get(), Btn_Switch.Get()}) {
        if (Hit) {
            PRAGMA_DISABLE_DEPRECATION_WARNINGS
            Hit->IsFocusable = false;
            PRAGMA_ENABLE_DEPRECATION_WARNINGS
        }
    }
    if (Slider_Value) {
        // Same reasoning: the row owns pad input and forwards it, so the slider must not steal focus.
        PRAGMA_DISABLE_DEPRECATION_WARNINGS
        Slider_Value->IsFocusable = false;
        PRAGMA_ENABLE_DEPRECATION_WARNINGS
    }

    if (Btn_Left && !Btn_Left->OnClicked.IsBound()) {
        Btn_Left->OnClicked.AddDynamic(this, &UMythicSettingRowBase::HandleStepDown);
    }
    if (Btn_Right && !Btn_Right->OnClicked.IsBound()) {
        Btn_Right->OnClicked.AddDynamic(this, &UMythicSettingRowBase::HandleStepUp);
    }
    if (Btn_Switch && !Btn_Switch->OnClicked.IsBound()) {
        Btn_Switch->OnClicked.AddDynamic(this, &UMythicSettingRowBase::HandleSwitchClicked);
    }
    // A chevron that lights under the cursor is how a player learns it is a button and not an ornament.
    if (Btn_Left && !Btn_Left->OnHovered.IsBound()) {
        Btn_Left->OnHovered.AddDynamic(this, &UMythicSettingRowBase::HandleLeftHovered);
        Btn_Left->OnUnhovered.AddDynamic(this, &UMythicSettingRowBase::HandleChevronUnhovered);
    }
    if (Btn_Right && !Btn_Right->OnHovered.IsBound()) {
        Btn_Right->OnHovered.AddDynamic(this, &UMythicSettingRowBase::HandleRightHovered);
        Btn_Right->OnUnhovered.AddDynamic(this, &UMythicSettingRowBase::HandleChevronUnhovered);
    }
    if (Slider_Value && !Slider_Value->OnValueChanged.IsBound()) {
        Slider_Value->OnValueChanged.AddDynamic(this, &UMythicSettingRowBase::HandleSliderValue);
    }
    if (Edit_Value && !Edit_Value->OnTextCommitted.IsBound()) {
        Edit_Value->OnTextCommitted.AddDynamic(this, &UMythicSettingRowBase::HandleValueTyped);
    }

    PushToWidgets();
}

FReply UMythicSettingRowBase::NativeOnFocusReceived(const FGeometry &Geo, const FFocusEvent &Event) {
    ApplyFocusVisuals(true);
    if (UMythicSettingsScreenBase *Owner = Screen.Get()) {
        Owner->SetFocusedRow(Definition);
    }
    OnFocusChanged(true);
    return Super::NativeOnFocusReceived(Geo, Event);
}

void UMythicSettingRowBase::NativeOnFocusLost(const FFocusEvent &Event) {
    ApplyFocusVisuals(false);
    OnFocusChanged(false);
    Super::NativeOnFocusLost(Event);
}

float UMythicSettingRowBase::TrackAlphaAt(const FPointerEvent &Event) const {
    if (!Img_Track) {
        return -1.0f;
    }
    const FGeometry &Track = Img_Track->GetCachedGeometry();
    const FVector2D Local = Track.AbsoluteToLocal(Event.GetScreenSpacePosition());
    const float Width = Track.GetLocalSize().X;
    if (Width <= KINDA_SMALL_NUMBER) {
        return -1.0f;
    }
    // Generous vertically: a 3 px track is impossible to hit, so the whole row height counts as the grab
    // area once the cursor is within the track's horizontal span.
    if (Local.X < -8.0f || Local.X > Width + 8.0f) {
        return -1.0f;
    }
    return FMath::Clamp(Local.X / Width, 0.0f, 1.0f);
}

FReply UMythicSettingRowBase::NativeOnMouseButtonDown(const FGeometry &Geo, const FPointerEvent &Event) {
    if (!IsAvailable()) {
        return Super::NativeOnMouseButtonDown(Geo, Event);
    }

    if (Definition.Control == EMythicSettingControl::Slider) {
        const float Alpha = TrackAlphaAt(Event);
        if (Alpha >= 0.0f) {
            bDraggingTrack = true;
            DragAlpha = Alpha;
            PreviewNormalised(Alpha);
            return FReply::Handled()
                   .SetUserFocus(TakeWidget(), EFocusCause::Mouse)
                   .CaptureMouse(TakeWidget());
        }
    }
    // Select only. Changing a value from anywhere on the row means you cannot rest the cursor on a
    // setting to read what it does, and it reduces the chevrons to decoration.
    return FReply::Handled().SetUserFocus(TakeWidget(), EFocusCause::Mouse);
}

FReply UMythicSettingRowBase::NativeOnMouseMove(const FGeometry &Geo, const FPointerEvent &Event) {
    if (bDraggingTrack && IsAvailable()) {
        const float Alpha = TrackAlphaAt(Event);
        if (Alpha >= 0.0f) {
            /**
             * Move the handle, do not touch the renderer.
             *
             * Writing on every drag tick pushes a scalability or cvar change dozens of times a second, and
             * the whole screen hitches under the cursor - which is what "it lags on value change" was.
             * The value lands once, on release.
             */
            DragAlpha = Alpha;
            PreviewNormalised(Alpha);
        }
        return FReply::Handled();
    }
    return Super::NativeOnMouseMove(Geo, Event);
}

FReply UMythicSettingRowBase::NativeOnMouseButtonUp(const FGeometry &Geo, const FPointerEvent &Event) {
    if (bDraggingTrack) {
        bDraggingTrack = false;
        if (DragAlpha >= 0.0f) {
            SetNormalisedValue(DragAlpha);   // the one real write of the whole drag
            DragAlpha = -1.0f;
        }
        return FReply::Handled().ReleaseMouseCapture();
    }
    return Super::NativeOnMouseButtonUp(Geo, Event);
}

void UMythicSettingRowBase::PreviewNormalised(float Normalised) {
    // Art only: the handle and the number follow the cursor so the drag feels direct, while the setting
    // itself is untouched until release.
    const float Alpha = FMath::Clamp(Normalised, 0.0f, 1.0f);

    if (Img_Knob) {
        Img_Knob->SetRenderTransformPivot(FVector2D(0.0f, 0.5f));
        FWidgetTransform T;
        T.Translation = FVector2D(Alpha * FMath::Max(0.0f, TrackWidth - KnobSize), 0.0f);
        Img_Knob->SetRenderTransform(T);
    }
    if (Img_Fill) {
        Img_Fill->SetRenderTransformPivot(FVector2D(0.0f, 0.5f));
        FWidgetTransform T;
        T.Scale = FVector2D(Alpha, 1.0f);
        Img_Fill->SetRenderTransform(T);
    }
    if (Edit_Value) {
        const float Scale = Definition.DisplayScale != 0.0f ? Definition.DisplayScale : 1.0f;
        const float Shown = FMath::Lerp(Definition.MinValue, Definition.MaxValue, Alpha) * Scale;
        FNumberFormattingOptions Fmt;
        Fmt.MinimumFractionalDigits = Definition.DisplayDecimals;
        Fmt.MaximumFractionalDigits = Definition.DisplayDecimals;
        Edit_Value->SetText(FText::Format(INVTEXT("{0}{1}"), FText::AsNumber(Shown, &Fmt),
                                          FText::FromString(Definition.DisplaySuffix)));
    }
}

void UMythicSettingRowBase::NativeOnMouseEnter(const FGeometry &Geo, const FPointerEvent &Event) {
    Super::NativeOnMouseEnter(Geo, Event);
    bHovered = true;
    ApplyHoverVisuals(true);
}

void UMythicSettingRowBase::NativeOnMouseLeave(const FPointerEvent &Event) {
    Super::NativeOnMouseLeave(Event);
    bHovered = false;
    ApplyHoverVisuals(false);
}

void UMythicSettingRowBase::HandleStepDown() {
    if (IsAvailable()) {
        Nudge(-1);
    }
}

void UMythicSettingRowBase::HandleStepUp() {
    if (IsAvailable()) {
        Nudge(1);
    }
}

void UMythicSettingRowBase::HandleSwitchClicked() {
    if (IsAvailable()) {
        ActivateRow();
    }
}

void UMythicSettingRowBase::HandleLeftHovered() {
    if (Img_Left && IsAvailable()) {
        Img_Left->SetColorAndOpacity(FMythicUIStyle::Get().Ink);
    }
}

void UMythicSettingRowBase::HandleRightHovered() {
    if (Img_Right && IsAvailable()) {
        Img_Right->SetColorAndOpacity(FMythicUIStyle::Get().Ink);
    }
}

void UMythicSettingRowBase::HandleChevronUnhovered() {
    // Back to whatever the row's focus state says, not unconditionally to idle: a focused row keeps gold.
    ApplyFocusVisuals(HasAnyUserFocus() || HasUserFocusedDescendants(GetOwningPlayer()));
}

void UMythicSettingRowBase::HandleSliderValue(float NewValue) {
    if (!IsAvailable() || bPushingToWidgets) {
        return;
    }
    // The slider reports 0..1; the setting lives in its own authored range.
    SetNormalisedValue(NewValue);
}

void UMythicSettingRowBase::HandleValueTyped(const FText &Text, ETextCommit::Type CommitType) {
    if (CommitType == ETextCommit::OnCleared || !IsAvailable()) {
        PushToWidgets();
        return;
    }
    // What the player typed is in DISPLAY units, which are not the stored units whenever a row carries a
    // DisplayScale. Sharpness reads 0-100% and stores 0-4, so typing 50 must write 2, not 50.
    FString Cleaned = Text.ToString().Replace(TEXT("%"), TEXT("")).TrimStartAndEnd();
    float Typed = 0.0f;
    if (LexTryParseString(Typed, *Cleaned)) {
        const float Scale = Definition.DisplayScale != 0.0f ? Definition.DisplayScale : 1.0f;
        UMythicSettingAccess::WriteValue(
            Definition, FMath::Clamp(Typed / Scale, Definition.MinValue, Definition.MaxValue));
        NotifyChanged();
    }
    else {
        // Reject silently by restoring the real value: a settings field should never keep nonsense.
        PushToWidgets();
    }
}

void UMythicSettingRowBase::ApplyHoverVisuals(bool bHovering) {
    // Hover lifts the ground; focus adds the bar. A focused row that is also hovered must not lose its bar.
    if (Img_Ground && !HasUserFocusedDescendants(GetOwningPlayer()) && !HasAnyUserFocus()) {
        Img_Ground->SetRenderOpacity(bHovering ? 0.62f : RestingGroundOpacity);
    }
    if (Text_Label) {
        Text_Label->SetColorAndOpacity(bHovering ? FMythicUIStyle::Get().Ink : FMythicUIStyle::Get().InkLabel);
    }
}

FReply UMythicSettingRowBase::NativeOnKeyDown(const FGeometry &Geo, const FKeyEvent &Event) {
    const FMythicInputStep Input = FMythicInputStep::FromKey(Event.GetKey());
    if (!Input.IsHandled() || !IsAvailable()) {
        return Super::NativeOnKeyDown(Geo, Event);
    }
    if (Input.bAccept) {
        ActivateRow();
    }
    else {
        Nudge(Input.Delta);
    }
    // Handled either way, so left and right adjust the value instead of walking Slate navigation out of
    // the list.
    return FReply::Handled();
}

void UMythicSettingRowBase::Redraw() {
    PushToWidgets();
    OnValueChanged();
}

void UMythicSettingRowBase::PushToWidgets() {
    // Writing the slider fires OnValueChanged, which would write straight back and fight the source.
    TGuardValue<bool> Guard(bPushingToWidgets, true);

    const bool bHeading = UMythicSettingsScreenBase::IsGroupHeading(Definition);

    if (Text_Label) {
        Text_Label->SetText(Definition.Label);
        // After the CommonUI style applies: a long label truncates with an ellipsis instead of painting into
        // the value cell ("Global Illumination Met|Hardware RT" was the 10-foot type ramp's first casualty).
        Text_Label->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
    }

    if (Text_Value) {
        // A heading has no value; collapsing rather than blanking keeps the cell from reserving width.
        Text_Value->SetVisibility(bHeading ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
        if (!bHeading) {
            Text_Value->SetText(GetValueText());
        }
    }

    if (Img_Track) {
        Img_Track->SetColorAndOpacity(GetOptionIndex() > 0 ? FMythicUIStyle::Get().Trough : FMythicUIStyle::Get().Hairline);
    }

    if (Img_TrackRim) {
        Img_TrackRim->SetColorAndOpacity(GetOptionIndex() > 0 ? FMythicUIStyle::Get().Accent : FMythicUIStyle::Get().Hairline);
    }

    if (Img_Fill) {
        // Scale from the left edge so the bar grows rightward instead of from its centre.
        Img_Fill->SetRenderTransformPivot(FVector2D(0.0f, 0.5f));
        FWidgetTransform Transform;
        Transform.Scale = FVector2D(GetNormalisedValue(), 1.0f);
        Img_Fill->SetRenderTransform(Transform);
    }

    if (Slider_Value) {
        Slider_Value->SetValue(GetNormalisedValue());
    }

    if (Img_Track && Definition.Control == EMythicSettingControl::Slider) {
        const float Width = Img_Track->GetCachedGeometry().GetLocalSize().X;
        if (Img_Knob && Width > KINDA_SMALL_NUMBER) {
            Img_Knob->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
            FWidgetTransform T;
            T.Translation = FVector2D(GetNormalisedValue() * Width, 0.0f);
            Img_Knob->SetRenderTransform(T);
        }
    }

    if (Edit_Value) {
        Edit_Value->SetText(GetValueText());
    }

    if (Img_Knob) {
        // One dot serves both controls: a switch snaps it between two ends, a slider rides it along the
        // track. Travel excludes the knob's own width so it stays ON the track at 100%.
        const bool bSwitch = Definition.Control == EMythicSettingControl::Toggle;
        const float Travel = bSwitch ? SwitchTravel : FMath::Max(0.0f, TrackWidth - KnobSize);
        const float Alpha = bSwitch ? (GetOptionIndex() > 0 ? 1.0f : 0.0f) : GetNormalisedValue();

        Img_Knob->SetRenderTransformPivot(FVector2D(0.0f, 0.5f));
        FWidgetTransform Transform;
        Transform.Translation = FVector2D(Alpha * Travel, 0.0f);
        Img_Knob->SetRenderTransform(Transform);

        if (bSwitch) {
            Img_Knob->SetColorAndOpacity(GetOptionIndex() > 0 ? FMythicUIStyle::Get().AccentBright : FMythicUIStyle::Get().InkSubtle);
        }
    }

    SetRenderOpacity(IsAvailable() ? 1.0f : UnavailableTint.A);
}

void UMythicSettingRowBase::ApplyFocusVisuals(bool bFocused) {
    // Focus is a ground lift and a bar down the left edge, not a box drawn round the row. A border reads
    // as a control the row is not; a bar reads as a cursor, which is what it is.
    if (Img_Ground) {
        /**
         * The row ground is ALWAYS drawn, faintly.
         *
         * A settings screen you can see the world through has no fixed backdrop: a label can land on a
         * dark sky or on sunlit grass, and one global scrim strong enough for the grass would blot out the
         * world it exists to reveal. A per-row band gives every label the same local contrast wherever the
         * scene happens to be bright, and doubles as the structure that separates one setting from the next.
         */
        Img_Ground->SetVisibility(ESlateVisibility::HitTestInvisible);
        Img_Ground->SetRenderOpacity(bFocused ? 1.0f : (bHovered ? 0.62f : RestingGroundOpacity));
    }
    if (Img_FocusBar) {
        Img_FocusBar->SetVisibility(bFocused ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
    if (Text_Label) {
        Text_Label->SetColorAndOpacity(bFocused ? FMythicUIStyle::Get().Ink : FMythicUIStyle::Get().InkLabel);
    }
    const FLinearColor Accent = bFocused ? FMythicUIStyle::Get().Accent : FMythicUIStyle::Get().InkSubtle;
    if (Img_Left) {
        Img_Left->SetColorAndOpacity(Accent);
    }
    if (Img_Right) {
        Img_Right->SetColorAndOpacity(Accent);
    }
}

void UMythicSettingRowBase::Nudge(int32 Delta) {
    switch (Definition.Control) {
        case EMythicSettingControl::Slider: {
            const float Step = Definition.StepSize > KINDA_SMALL_NUMBER ? Definition.StepSize : 0.05f;
            const float Current = UMythicSettingAccess::ReadValue(Definition);
            UMythicSettingAccess::WriteValue(
                Definition,
                FMath::Clamp(Current + Step * static_cast<float>(Delta), Definition.MinValue, Definition.MaxValue));
            NotifyChanged();
            break;
        }
        case EMythicSettingControl::Select:
        case EMythicSettingControl::Toggle:
            StepOption(Delta);
            break;
        default:
            break;
    }
}

void UMythicSettingRowBase::ActivateRow() {
    switch (Definition.Control) {
        case EMythicSettingControl::Toggle: {
            // Accept flips, so a toggle answers the same button as everything else on the page. Wraps,
            // because with two options there is no far end to run into.
            const int32 Count = UMythicSettingAccess::GetAvailableOptions(Definition).Num();
            if (Count > 0) {
                SetOptionIndex((GetOptionIndex() + 1) % Count);
            }
            break;
        }
        case EMythicSettingControl::Select:
            StepOption(1);
            break;
        case EMythicSettingControl::Action:
        case EMythicSettingControl::Keybind:
            OnActionTriggered();
            break;
        default:
            break;
    }
}

void UMythicSettingRowBase::SetDefinition(const FMythicSettingDefinition &InDefinition,
                                          UMythicSettingsScreenBase *InScreen) {
    Definition = InDefinition;
    Screen = InScreen;
    PushToWidgets();
    OnDefinitionSet();
}

FText UMythicSettingRowBase::GetValueText() const {
    return UMythicSettingAccess::GetDisplayText(Definition);
}

float UMythicSettingRowBase::GetNormalisedValue() const {
    const float Span = Definition.MaxValue - Definition.MinValue;
    if (Span <= KINDA_SMALL_NUMBER) {
        return 0.0f;
    }
    return FMath::Clamp((UMythicSettingAccess::ReadValue(Definition) - Definition.MinValue) / Span, 0.0f, 1.0f);
}

TArray<FText> UMythicSettingRowBase::GetOptionLabels() const {
    TArray<FText> Labels;
    for (const FMythicSettingOption &Option : UMythicSettingAccess::GetAvailableOptions(Definition)) {
        Labels.Add(Option.Label);
    }
    return Labels;
}

int32 UMythicSettingRowBase::GetOptionIndex() const {
    return UMythicSettingAccess::ReadOptionIndex(Definition);
}

bool UMythicSettingRowBase::IsAvailable() const {
    return UMythicSettingAccess::IsSettingAvailable(Definition);
}

bool UMythicSettingRowBase::IsChangedFromDefault() const {
    return !UMythicSettingAccess::IsAtDefault(Definition);
}

void UMythicSettingRowBase::StepOption(int32 Delta) {
    if (!IsAvailable()) {
        return;
    }
    const int32 Count = UMythicSettingAccess::GetAvailableOptions(Definition).Num();
    if (Count <= 0) {
        return;
    }
    SetOptionIndex(FMath::Clamp(GetOptionIndex() + Delta, 0, Count - 1));
}

void UMythicSettingRowBase::SetOptionIndex(int32 Index) {
    if (!IsAvailable()) {
        return;
    }
    UMythicSettingAccess::WriteOptionIndex(Definition, Index);
    NotifyChanged();
}

void UMythicSettingRowBase::SetNormalisedValue(float Normalised) {
    if (!IsAvailable()) {
        return;
    }
    const float Value = FMath::Lerp(Definition.MinValue, Definition.MaxValue, FMath::Clamp(Normalised, 0.0f, 1.0f));
    UMythicSettingAccess::WriteValue(Definition, Value);
    NotifyChanged();
}

void UMythicSettingRowBase::ResetToDefault() {
    UMythicSettingAccess::WriteValue(Definition, Definition.DefaultValue);
    NotifyChanged();
}

void UMythicSettingRowBase::NotifyChanged() {
    PushToWidgets();
    if (UMythicSettingsScreenBase *Owner = Screen.Get()) {
        // Unconditional: WriteValue buffers every setting without exception, so every change is one the
        // player has to confirm. Marking only some of them meant 21 of 33 settings staged silently - you
        // changed them, saw the new value, were never told to Apply, and lost it on the way out.
        Owner->MarkPendingApply();
        // Re-publish to the detail panel. It was only told on focus, so the panel kept showing the value
        // the setting had when you arrived at it while the row beside it showed the new one.
        Owner->SetFocusedRow(Definition);
    }
    OnValueChanged();
}
