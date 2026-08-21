// Copyright Stellar Games. All Rights Reserved.

#include "MythicKitInputs.h"

#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace {
const FName P_Kind(TEXT("Kind"));
const FName P_Value(TEXT("Value"));
const FName P_State(TEXT("State"));
const FName P_Steps(TEXT("Steps"));

constexpr float State_Normal = 0.0f;
constexpr float State_Hover = 1.0f;
constexpr float State_Disabled = 3.0f;

const TCHAR *DefaultInputMaterial = TEXT("/Game/Mythic/UI/Globals/materials/M_UI_HandInput.M_UI_HandInput");
}


FMythicInputStep FMythicInputStep::FromKey(const FKey &Key) {
    FMythicInputStep Step;
    if (Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_LeftStick_Left) {
        Step.Delta = -1;
    }
    else if (Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_LeftStick_Right) {
        Step.Delta = 1;
    }
    else if (Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::Enter || Key == EKeys::SpaceBar) {
        Step.bAccept = true;
    }
    return Step;
}


void UMythicKitInputBase::NativePreConstruct() {
    Super::NativePreConstruct();

    if (!InputMaterial) {
        InputMaterial = Cast<UMaterialInterface>(FSoftObjectPath(DefaultInputMaterial).TryLoad());
    }
    if (Visual && InputMaterial) {
        if (Visual->GetBrush().GetResourceObject() != InputMaterial) {
            FSlateBrush Brush;
            Brush.SetResourceObject(InputMaterial);
            Brush.DrawAs = ESlateBrushDrawType::Image;
            Brush.ImageSize = FVector2D(160.0f, 32.0f);
            Visual->SetBrush(Brush);
        }
        Material = Visual->GetDynamicMaterial();
        PushToMaterial();
    }
}

void UMythicKitInputBase::NativeConstruct() {
    Super::NativeConstruct();

    SetIsFocusable(true);

    if (Visual) {
        Material = Visual->GetDynamicMaterial();
    }
    PushToMaterial();
}

void UMythicKitInputBase::SetValue(float InValue) {
    const float Clamped = FMath::Clamp(InValue, 0.0f, 1.0f);
    if (FMath::IsNearlyEqual(Clamped, Value)) {
        return;
    }
    Value = Clamped;
    PushToMaterial();
}

void UMythicKitInputBase::PushToMaterial() {
    if (!Material) {
        return;
    }
    Material->SetScalarParameterValue(P_Kind, static_cast<float>(Kind));
    Material->SetScalarParameterValue(P_Value, Value);

    float State = State_Normal;
    if (!GetIsEnabled()) {
        State = State_Disabled;
    }
    else if (bFocused || bHovered) {
        State = State_Hover;
    }
    Material->SetScalarParameterValue(P_State, State);
}

void UMythicKitInputBase::NativeOnMouseEnter(const FGeometry &Geo, const FPointerEvent &Event) {
    Super::NativeOnMouseEnter(Geo, Event);
    bHovered = true;
    PushToMaterial();
}

void UMythicKitInputBase::NativeOnMouseLeave(const FPointerEvent &Event) {
    Super::NativeOnMouseLeave(Event);
    bHovered = false;
    PushToMaterial();
}

FReply UMythicKitInputBase::NativeOnFocusReceived(const FGeometry &Geo, const FFocusEvent &Event) {
    bFocused = true;
    PushToMaterial();
    return Super::NativeOnFocusReceived(Geo, Event);
}

void UMythicKitInputBase::NativeOnFocusLost(const FFocusEvent &Event) {
    bFocused = false;
    PushToMaterial();
    Super::NativeOnFocusLost(Event);
}

FReply UMythicKitInputBase::NativeOnKeyDown(const FGeometry &Geo, const FKeyEvent &Event) {
    if (!GetIsEnabled()) {
        return Super::NativeOnKeyDown(Geo, Event);
    }

    const FMythicInputStep Input = FMythicInputStep::FromKey(Event.GetKey());
    if (Input.IsHandled()) {
        Step(Input.bAccept ? 1 : Input.Delta);
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(Geo, Event);
}


UMythicKitSlider::UMythicKitSlider() {
    Kind = EMythicKitInputKind::Slider;
}

void UMythicKitSlider::Step(int32 Delta) {
    const float Next = FMath::Clamp(Value + StepSize * static_cast<float>(Delta), 0.0f, 1.0f);
    if (FMath::IsNearlyEqual(Next, Value)) {
        return;
    }
    Value = Next;
    PushToMaterial();
    OnValueChanged.Broadcast(Value);
}


UMythicKitCheck::UMythicKitCheck() {
    Kind = EMythicKitInputKind::Check;
}

void UMythicKitCheck::SetChecked(bool bInChecked) {
    SetValue(bInChecked ? 1.0f : 0.0f);
}

void UMythicKitCheck::Step(int32 Delta) {
    Value = IsChecked() ? 0.0f : 1.0f;
    PushToMaterial();
    OnValueChanged.Broadcast(Value);
}


UMythicKitSelect::UMythicKitSelect() {
    Kind = EMythicKitInputKind::Stepper;
}

void UMythicKitSelect::NativePreConstruct() {
    Super::NativePreConstruct();
    if (Material) {
        Material->SetScalarParameterValue(P_Steps, static_cast<float>(FMath::Max(OptionCount, 2)));
    }
}

void UMythicKitSelect::SetOptionCount(int32 InCount) {
    OptionCount = FMath::Max(InCount, 2);
    if (Material) {
        Material->SetScalarParameterValue(P_Steps, static_cast<float>(OptionCount));
    }
    PushToMaterial();
}

int32 UMythicKitSelect::GetIndex() const {
    const int32 Count = FMath::Max(OptionCount, 2);
    return FMath::Clamp(FMath::RoundToInt(Value * static_cast<float>(Count - 1)), 0, Count - 1);
}

void UMythicKitSelect::Step(int32 Delta) {
    const int32 Count = FMath::Max(OptionCount, 2);
    const int32 Raw = GetIndex() + Delta;
    const int32 Next = bWrap ? ((Raw % Count) + Count) % Count : FMath::Clamp(Raw, 0, Count - 1);
    const float NewValue = static_cast<float>(Next) / static_cast<float>(Count - 1);
    if (FMath::IsNearlyEqual(NewValue, Value)) {
        return;
    }
    Value = NewValue;
    PushToMaterial();
    OnValueChanged.Broadcast(Value);
}
