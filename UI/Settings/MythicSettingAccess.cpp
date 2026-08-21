
#include "UI/Settings/MythicSettingAccess.h"

#include "HAL/IConsoleManager.h"
#include "RenderUtils.h"
#include "Scalability.h"
#include "UI/Settings/MythicUserSettings.h"

namespace {

IConsoleVariable *FindCVar(FName Name) {
    return Name.IsNone() ? nullptr : IConsoleManager::Get().FindConsoleVariable(*Name.ToString());
}

FProperty *FindSettingProperty(FName Name, UMythicUserSettings *&OutSettings) {
    OutSettings = UMythicUserSettings::Get();
    if (!OutSettings || Name.IsNone()) {
        return nullptr;
    }
    return OutSettings->GetClass()->FindPropertyByName(Name);
}

// The nine engine scalability groups, addressed by the name a designer would author.
int32 ReadScalability(FName Group) {
    const Scalability::FQualityLevels Q = Scalability::GetQualityLevels();
    if (Group == TEXT("ViewDistance")) { return Q.ViewDistanceQuality; }
    if (Group == TEXT("Shadows")) { return Q.ShadowQuality; }
    if (Group == TEXT("GlobalIllumination")) { return Q.GlobalIlluminationQuality; }
    if (Group == TEXT("Reflections")) { return Q.ReflectionQuality; }
    if (Group == TEXT("Textures")) { return Q.TextureQuality; }
    if (Group == TEXT("Effects")) { return Q.EffectsQuality; }
    if (Group == TEXT("PostProcess")) { return Q.PostProcessQuality; }
    if (Group == TEXT("Foliage")) { return Q.FoliageQuality; }
    if (Group == TEXT("Shading")) { return Q.ShadingQuality; }
    return INDEX_NONE;
}

bool WriteScalability(FName Group, int32 Value) {
    Scalability::FQualityLevels Q = Scalability::GetQualityLevels();
    if (Group == TEXT("ViewDistance")) { Q.ViewDistanceQuality = Value; }
    else if (Group == TEXT("Shadows")) { Q.ShadowQuality = Value; }
    else if (Group == TEXT("GlobalIllumination")) { Q.GlobalIlluminationQuality = Value; }
    else if (Group == TEXT("Reflections")) { Q.ReflectionQuality = Value; }
    else if (Group == TEXT("Textures")) { Q.TextureQuality = Value; }
    else if (Group == TEXT("Effects")) { Q.EffectsQuality = Value; }
    else if (Group == TEXT("PostProcess")) { Q.PostProcessQuality = Value; }
    else if (Group == TEXT("Foliage")) { Q.FoliageQuality = Value; }
    else if (Group == TEXT("Shading")) { Q.ShadingQuality = Value; }
    else { return false; }
    Scalability::SetQualityLevels(Q);
    return true;
}

} // namespace

bool UMythicSettingAccess::IsRequirementMet(FGameplayTag Requirement) {
    if (!Requirement.IsValid()) {
        return true;
    }
    const FString Name = Requirement.ToString();
    if (Name.EndsWith(TEXT("Nvidia"))) { return UMythicUserSettings::IsNvidiaGpu(); }
    if (Name.EndsWith(TEXT("DLSS"))) { return UMythicUserSettings::IsDLSSAvailable(); }
    if (Name.EndsWith(TEXT("FrameGeneration"))) { return UMythicUserSettings::IsFrameGenerationAvailable(); }
    if (Name.EndsWith(TEXT("RayReconstruction"))) { return UMythicUserSettings::IsRayReconstructionAvailable(); }
    if (Name.EndsWith(TEXT("HardwareRayTracing"))) { return UMythicUserSettings::IsHardwareRayTracingAvailable(); }
    if (Name.EndsWith(TEXT("Reflex"))) { return UMythicUserSettings::IsReflexAvailable(); }

    // An unrecognised requirement fails OPEN. A mistyped tag should surface a working row, never silently
    // hide a setting the player came here looking for.
    return true;
}

bool UMythicSettingAccess::IsSettingAvailable(const FMythicSettingDefinition &Def) {
    return IsRequirementMet(Def.Requires);
}

TArray<FMythicSettingOption> UMythicSettingAccess::GetAvailableOptions(const FMythicSettingDefinition &Def) {
    TArray<FMythicSettingOption> Out;
    for (const FMythicSettingOption &Option : Def.Options) {
        if (IsRequirementMet(Option.Requires)) {
            Out.Add(Option);
        }
    }
    return Out;
}

float UMythicSettingAccess::ReadValue(const FMythicSettingDefinition &Def) {
    switch (Def.Source) {
        case EMythicSettingSource::CVar: {
            const IConsoleVariable *CVar = FindCVar(Def.SourceName);
            return CVar ? CVar->GetFloat() : Def.DefaultValue;
        }
        case EMythicSettingSource::Property: {
            UMythicUserSettings *Settings = nullptr;
            if (const FProperty *Prop = FindSettingProperty(Def.SourceName, Settings)) {
                if (const FFloatProperty *F = CastField<FFloatProperty>(Prop)) {
                    return F->GetPropertyValue_InContainer(Settings);
                }
                if (const FIntProperty *I = CastField<FIntProperty>(Prop)) {
                    return static_cast<float>(I->GetPropertyValue_InContainer(Settings));
                }
                if (const FBoolProperty *B = CastField<FBoolProperty>(Prop)) {
                    return B->GetPropertyValue_InContainer(Settings) ? 1.0f : 0.0f;
                }
            }
            return Def.DefaultValue;
        }
        case EMythicSettingSource::Scalability: {
            const int32 Level = ReadScalability(Def.SourceName);
            return Level == INDEX_NONE ? Def.DefaultValue : static_cast<float>(Level);
        }
        default:
            return Def.DefaultValue;
    }
}

void UMythicSettingAccess::WriteValue(const FMythicSettingDefinition &Def, float Value) {
    switch (Def.Source) {
        case EMythicSettingSource::CVar: {
            if (IConsoleVariable *CVar = FindCVar(Def.SourceName)) {
                CVar->Set(Value, ECVF_SetByGameOverride);
            }
            break;
        }
        case EMythicSettingSource::Property: {
            UMythicUserSettings *Settings = nullptr;
            if (const FProperty *Prop = FindSettingProperty(Def.SourceName, Settings)) {
                if (const FFloatProperty *F = CastField<FFloatProperty>(Prop)) {
                    F->SetPropertyValue_InContainer(Settings, Value);
                }
                else if (const FIntProperty *I = CastField<FIntProperty>(Prop)) {
                    I->SetPropertyValue_InContainer(Settings, FMath::RoundToInt(Value));
                }
                else if (const FBoolProperty *B = CastField<FBoolProperty>(Prop)) {
                    B->SetPropertyValue_InContainer(Settings, Value > 0.5f);
                }
                Settings->ApplySettings(false);
            }
            break;
        }
        case EMythicSettingSource::Scalability:
            WriteScalability(Def.SourceName, FMath::RoundToInt(Value));
            break;
        default:
            break;
    }
}

int32 UMythicSettingAccess::ReadOptionIndex(const FMythicSettingDefinition &Def) {
    const TArray<FMythicSettingOption> Options = GetAvailableOptions(Def);
    const float Current = ReadValue(Def);
    for (int32 i = 0; i < Options.Num(); ++i) {
        if (FMath::IsNearlyEqual(Options[i].Value, Current, 0.001f)) {
            return i;
        }
    }
    return Options.Num() > 0 ? 0 : INDEX_NONE;
}

void UMythicSettingAccess::WriteOptionIndex(const FMythicSettingDefinition &Def, int32 Index) {
    const TArray<FMythicSettingOption> Options = GetAvailableOptions(Def);
    if (!Options.IsValidIndex(Index)) {
        return;
    }
    WriteValue(Def, Options[Index].Value);

    // Extra cvars are how a profile rides an option: choosing GTAO also sets its angle count and spatial
    // filter, authored beside the option rather than written into code.
    for (const TPair<FName, float> &Extra : Options[Index].ExtraCVars) {
        if (IConsoleVariable *CVar = FindCVar(Extra.Key)) {
            CVar->Set(Extra.Value, ECVF_SetByGameOverride);
        }
    }
}

FText UMythicSettingAccess::GetDisplayText(const FMythicSettingDefinition &Def) {
    if (Def.Control == EMythicSettingControl::Select || Def.Control == EMythicSettingControl::Toggle) {
        const TArray<FMythicSettingOption> Options = GetAvailableOptions(Def);
        const int32 Index = ReadOptionIndex(Def);
        return Options.IsValidIndex(Index) ? Options[Index].Label : FText::GetEmpty();
    }

    const float Scale = Def.DisplayScale != 0.0f ? Def.DisplayScale : 1.0f;
    const float Shown = ReadValue(Def) * Scale;

    FNumberFormattingOptions Format;
    Format.MinimumFractionalDigits = Def.DisplayDecimals;
    Format.MaximumFractionalDigits = Def.DisplayDecimals;
    const FText Number = FText::AsNumber(Shown, &Format);

    return Def.DisplaySuffix.IsEmpty()
               ? Number
               : FText::Format(INVTEXT("{0}{1}"), Number, FText::FromString(Def.DisplaySuffix));
}

bool UMythicSettingAccess::IsAtDefault(const FMythicSettingDefinition &Def) {
    return FMath::IsNearlyEqual(ReadValue(Def), Def.DefaultValue, 0.001f);
}

void UMythicSettingAccess::RestoreDefaults(const UMythicSettingsCatalog *Catalog) {
    if (!Catalog) {
        return;
    }
    // One loop over the catalog, so a setting cannot be left out of Restore Defaults the way nine of them
    // were when each had to be remembered by hand.
    for (const FMythicSettingDefinition &Def : Catalog->Settings) {
        WriteValue(Def, Def.DefaultValue);
    }
    if (UMythicUserSettings *Settings = UMythicUserSettings::Get()) {
        Settings->ApplySettings(false);
        Settings->SaveSettings();
    }
}

bool UMythicSettingAccess::ResolvesSource(const FMythicSettingDefinition &Def, FString &OutWhy) {
    switch (Def.Source) {
        case EMythicSettingSource::CVar:
            if (!FindCVar(Def.SourceName)) {
                OutWhy = FString::Printf(TEXT("console variable '%s' does not exist"), *Def.SourceName.ToString());
                return false;
            }
            return true;
        case EMythicSettingSource::Property: {
            UMythicUserSettings *Settings = nullptr;
            if (!FindSettingProperty(Def.SourceName, Settings)) {
                OutWhy = FString::Printf(TEXT("UMythicUserSettings has no property '%s'"), *Def.SourceName.ToString());
                return false;
            }
            return true;
        }
        case EMythicSettingSource::Scalability:
            if (ReadScalability(Def.SourceName) == INDEX_NONE) {
                OutWhy = FString::Printf(TEXT("'%s' is not a scalability group"), *Def.SourceName.ToString());
                return false;
            }
            return true;
        default:
            return true;
    }
}
