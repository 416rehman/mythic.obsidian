
#include "UI/Settings/MythicSettingRowBase.h"

#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "UI/Kit/MythicKitInputs.h"
#include "UI/Settings/MythicSettingAccess.h"
#include "UI/Settings/MythicSettingsScreenBase.h"

void UMythicSettingRowBase::NativeConstruct() {
    Super::NativeConstruct();

    // A heading is text, not a stop. Letting it take focus makes a pad feel broken: the row lights up and
    // then answers nothing.
    SetIsFocusable(!UMythicSettingsScreenBase::IsGroupHeading(Definition));
    PushToWidgets();
}

FReply UMythicSettingRowBase::NativeOnFocusReceived(const FGeometry &Geo, const FFocusEvent &Event) {
    if (FocusRing) {
        FocusRing->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
    if (UMythicSettingsScreenBase *Owner = Screen.Get()) {
        Owner->SetFocusedRow(Definition);
    }
    OnFocusChanged(true);
    return Super::NativeOnFocusReceived(Geo, Event);
}

void UMythicSettingRowBase::NativeOnFocusLost(const FFocusEvent &Event) {
    if (FocusRing) {
        FocusRing->SetVisibility(ESlateVisibility::Collapsed);
    }
    OnFocusChanged(false);
    Super::NativeOnFocusLost(Event);
}

FReply UMythicSettingRowBase::NativeOnMouseButtonDown(const FGeometry &Geo, const FPointerEvent &Event) {
    if (!IsAvailable()) {
        return Super::NativeOnMouseButtonDown(Geo, Event);
    }
    ActivateRow();
    return FReply::Handled().SetUserFocus(TakeWidget(), EFocusCause::Mouse);
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
    const bool bHeading = UMythicSettingsScreenBase::IsGroupHeading(Definition);

    if (Text_Label) {
        Text_Label->SetText(Definition.Label);
    }

    if (Text_Value) {
        // A heading has no value; collapsing rather than blanking keeps the cell from reserving width.
        Text_Value->SetVisibility(bHeading ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
        if (!bHeading) {
            Text_Value->SetText(GetValueText());
        }
    }

    if (Img_Switch) {
        const bool bOn = GetOptionIndex() > 0;
        const FSlateBrush &Brush = bOn ? SwitchOnBrush : SwitchOffBrush;
        if (Brush.GetResourceObject()) {
            Img_Switch->SetBrush(Brush);
        }
    }

    if (Img_Fill) {
        // Scale from the left edge so the bar grows rightward instead of from its centre.
        Img_Fill->SetRenderTransformPivot(FVector2D(0.0f, 0.5f));
        FWidgetTransform Transform;
        Transform.Scale = FVector2D(GetNormalisedValue(), 1.0f);
        Img_Fill->SetRenderTransform(Transform);
    }

    if (Img_Thumb) {
        Img_Thumb->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
        FWidgetTransform Transform;
        Transform.Translation = FVector2D(GetNormalisedValue() * ThumbTravel, 0.0f);
        Img_Thumb->SetRenderTransform(Transform);
    }

    SetRenderOpacity(IsAvailable() ? 1.0f : UnavailableTint.A);
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
    if (Definition.bNeedsApply) {
        if (UMythicSettingsScreenBase *Owner = Screen.Get()) {
            Owner->MarkPendingApply();
        }
    }
    OnValueChanged();
}
