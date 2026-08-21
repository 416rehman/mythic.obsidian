
#include "UI/Settings/MythicSettingRowBase.h"

#include "UI/Settings/MythicSettingAccess.h"
#include "UI/Settings/MythicSettingsScreenBase.h"

void UMythicSettingRowBase::SetDefinition(const FMythicSettingDefinition &InDefinition,
                                          UMythicSettingsScreenBase *InScreen) {
    Definition = InDefinition;
    Screen = InScreen;
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
    if (Definition.bNeedsApply) {
        if (UMythicSettingsScreenBase *Owner = Screen.Get()) {
            Owner->MarkPendingApply();
        }
    }
    OnValueChanged();
}
