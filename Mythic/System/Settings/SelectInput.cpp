#include "SelectInput.h"


UMythicSelectInput::UMythicSelectInput(): UMythicInput() {
    CurrentOptionIndex = 0;
    IncomingOptionIndex = 0;
}

UMythicSelectInput::UMythicSelectInput(FText InLabel, FText InDescription, TArray<FOptionAndDescription> InOptions): UMythicInput() {
    Label = InLabel;
    Description = InDescription;
    Options = InOptions;
    CurrentOptionIndex = 0;
    IncomingOptionIndex = 0;
}

void UMythicSelectInput::Apply() {
    if (IncomingOptionIndex != CurrentOptionIndex) {
        CurrentOptionIndex = IncomingOptionIndex;
    }
}

void UMythicSelectInput::Reset() {
    IncomingOptionIndex = 0;
}

void UMythicSelectInput::NextOption() {
    IncomingOptionIndex = (IncomingOptionIndex + 1) % Options.Num();
}

void UMythicSelectInput::PreviousOption() {
    IncomingOptionIndex = (IncomingOptionIndex - 1) % Options.Num();
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

const TArray<FOptionAndDescription> & UMythicSelectInput::GetOptions() const {
    return Options;
}

void UMythicSelectInput::SetOptions(const TArray<FOptionAndDescription> &InOptions) {
    Options = InOptions;
}

void UMythicSelectInput::SetCurrentOptionIndex(uint8 InCurrentOptionIndex) {
    CurrentOptionIndex = InCurrentOptionIndex;
}

uint8 UMythicSelectInput::GetCurrentOptionIndex() const {
    return CurrentOptionIndex;
}

void UMythicSelectInput::SetIncomingOptionIndex(uint8 InIncomingOptionIndex) {
    IncomingOptionIndex = InIncomingOptionIndex;
}

uint8 UMythicSelectInput::GetIncomingOptionIndex() const {
    return IncomingOptionIndex;
}
