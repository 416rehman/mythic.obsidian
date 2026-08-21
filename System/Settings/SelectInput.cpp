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
        SetCurrentOptionIndex(IncomingOptionIndex);
    }
}

void UMythicSelectInput::Reset() {
    SetIncomingOptionIndex(0);
}

void UMythicSelectInput::NextOption() {
    const int32 N = Options.Num();
    if (N <= 0) { return; }
    SetIncomingOptionIndex(static_cast<uint8>((static_cast<int32>(IncomingOptionIndex) + 1) % N));
}

void UMythicSelectInput::PreviousOption() {
    const int32 N = Options.Num();
    if (N <= 0) { return; }
    SetIncomingOptionIndex(static_cast<uint8>((static_cast<int32>(IncomingOptionIndex) - 1 + N) % N));
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
