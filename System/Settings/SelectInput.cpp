#include "SelectInput.h"


UMythicSelectInput::UMythicSelectInput() : UMythicInput() {
    CurrentOptionIndex = 0;
    IncomingOptionIndex = 0;
}

UMythicSelectInput::UMythicSelectInput(FText InLabel, FText InDescription, TArray<FOptionAndDescription> InOptions) : UMythicInput() {
    Label = InLabel;
    Description = InDescription;
    Options = InOptions;
    CurrentOptionIndex = 0;
    IncomingOptionIndex = 0;
}

void UMythicSelectInput::Apply() {
    if (IncomingOptionIndex != CurrentOptionIndex) {
        SetCurrentOptionIndex(IncomingOptionIndex); // route through the setter so the FieldNotify broadcast fires
    }
}

void UMythicSelectInput::Reset() {
    SetIncomingOptionIndex(0); // route through the setter so bound UI refreshes
}

void UMythicSelectInput::NextOption() {
    const int32 N = Options.Num();
    if (N <= 0) { return; } // guard modulo-by-zero on an empty select
    SetIncomingOptionIndex(static_cast<uint8>((static_cast<int32>(IncomingOptionIndex) + 1) % N)); // broadcasts via the setter
}

void UMythicSelectInput::PreviousOption() {
    const int32 N = Options.Num();
    if (N <= 0) { return; } // guard modulo-by-zero on an empty select
    // Signed math + (+N): IncomingOptionIndex is uint8, promoted to int it underflows at index 0 to -1, and -1 % N == -1
    // which stored back into the uint8 became 255 (no wrap to the last option). Add N before the modulo so 0 -> N-1.
    SetIncomingOptionIndex(static_cast<uint8>((static_cast<int32>(IncomingOptionIndex) - 1 + N) % N)); // broadcasts via the setter
}

bool UMythicSelectInput::IsDisabled(APlayerController *inPlayerController, FText &Reason) {
    return false;
}

TArray<FOptionAndDescription> UMythicSelectInput::GetOptionDescriptions() {
    return this->Options;
}

bool UMythicSelectInput::IsDirty() const {
    return CurrentOptionIndex != IncomingOptionIndex;
}

const TArray<FOptionAndDescription> &UMythicSelectInput::GetOptions() const {
    return Options;
}

void UMythicSelectInput::SetOptions(const TArray<FOptionAndDescription> &InOptions) {
    // FOptionAndDescription has no operator== (FText fields), so UE_MVVM_SET_PROPERTY_VALUE can't equality-check —
    // assign + broadcast unconditionally so bound settings UI refreshes when the option list changes.
    Options = InOptions;
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Options);
}

void UMythicSelectInput::SetCurrentOptionIndex(uint8 InCurrentOptionIndex) {
    if (UE_MVVM_SET_PROPERTY_VALUE(CurrentOptionIndex, InCurrentOptionIndex)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CurrentOptionIndex);
    }
}

uint8 UMythicSelectInput::GetCurrentOptionIndex() const {
    return CurrentOptionIndex;
}

void UMythicSelectInput::SetIncomingOptionIndex(uint8 InIncomingOptionIndex) {
    if (UE_MVVM_SET_PROPERTY_VALUE(IncomingOptionIndex, InIncomingOptionIndex)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IncomingOptionIndex);
    }
}

uint8 UMythicSelectInput::GetIncomingOptionIndex() const {
    return IncomingOptionIndex;
}
