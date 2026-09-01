
#include "UI/Settings/MythicSettingAccess.h"

#include "HAL/IConsoleManager.h"
#include "Mythic/Mythic.h"
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

bool OptionPayloadsEqual(const FMythicSettingOption &A, const FMythicSettingOption &B) {
    if (!FMath::IsNearlyEqual(A.Value, B.Value, 0.001f) ||
        A.ExtraCVars.Num() != B.ExtraCVars.Num()) {
        return false;
    }
    for (const TPair<FName, float> &Extra : A.ExtraCVars) {
        const float *OtherValue = B.ExtraCVars.Find(Extra.Key);
        if (!OtherValue || !FMath::IsNearlyEqual(Extra.Value, *OtherValue, 0.001f)) {
            return false;
        }
    }
    return true;
}

int32 ResolveOptionIndex(const FMythicSettingDefinition &Def, float Current,
                         bool bAllowValueOnlyFallback) {
    const TArray<FMythicSettingOption> Options = UMythicSettingAccess::GetAvailableOptions(Def);

    int32 ValueOnlyMatch = INDEX_NONE;
    int32 BestExactMatch = INDEX_NONE;
    int32 BestSpecificity = INDEX_NONE;
    for (int32 i = 0; i < Options.Num(); ++i) {
        if (!FMath::IsNearlyEqual(Options[i].Value, Current, 0.001f)) {
            continue;
        }
        if (ValueOnlyMatch == INDEX_NONE) {
            ValueOnlyMatch = i;
        }
        bool bExtrasMatch = true;
        for (const TPair<FName, float> &Extra : Options[i].ExtraCVars) {
            const IConsoleVariable *CVar = FindCVar(Extra.Key);
            if (!CVar || !FMath::IsNearlyEqual(CVar->GetFloat(), Extra.Value, 0.001f)) {
                bExtrasMatch = false;
                break;
            }
        }
        // Prefer the profile that proves the most companion state. An empty profile is still a valid
        // exact match, but it must not mask a later Hardware/Software variant sharing the primary value.
        if (bExtrasMatch && Options[i].ExtraCVars.Num() > BestSpecificity) {
            BestExactMatch = i;
            BestSpecificity = Options[i].ExtraCVars.Num();
        }
    }
    if (BestExactMatch != INDEX_NONE) {
        return BestExactMatch;
    }
    return bAllowValueOnlyFallback ? ValueOnlyMatch : INDEX_NONE;
}

int32 ResolveDefaultOptionIndex(const FMythicSettingDefinition &Def) {
    const TArray<FMythicSettingOption> Options = UMythicSettingAccess::GetAvailableOptions(Def);

    int32 MarkedDefault = INDEX_NONE;
    int32 MarkedCount = 0;
    for (int32 Index = 0; Index < Options.Num(); ++Index) {
        if (Options[Index].bIsDefault) {
            MarkedDefault = Index;
            ++MarkedCount;
        }
    }
    if (MarkedCount == 1) {
        return MarkedDefault;
    }
    if (MarkedCount > 1) {
        return INDEX_NONE;
    }

    // Existing unique-value catalogs need no redundant flag. A shared-value default does: choosing the
    // first match would make reorderings silently change which companion profile Reset restores.
    int32 UniqueValueMatch = INDEX_NONE;
    for (int32 Index = 0; Index < Options.Num(); ++Index) {
        if (!FMath::IsNearlyEqual(Options[Index].Value, Def.DefaultValue, 0.001f)) {
            continue;
        }
        if (UniqueValueMatch != INDEX_NONE) {
            return INDEX_NONE;
        }
        UniqueValueMatch = Index;
    }
    return UniqueValueMatch;
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
    // A buffered change is what the player asked for, so it is what the row shows. Reading past it would
    // make every edit appear to do nothing until Apply.
    if (const FPending *Change = PendingChanges.Find(Def.SourceName)) {
        return Change->Value;
    }
    return ReadValueUncached(Def);
}

float UMythicSettingAccess::ReadValueUncached(const FMythicSettingDefinition &Def) {
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

TMap<FName, UMythicSettingAccess::FPending> UMythicSettingAccess::PendingChanges;

void UMythicSettingAccess::BeginStaging() {
    PendingChanges.Empty();
}

bool UMythicSettingAccess::HasStagedChanges() {
    return PendingChanges.Num() > 0;
}

void UMythicSettingAccess::CommitStaged() {
    // Replay every buffered change for real, in one pass, then let the caller save. Doing the whole set
    // at once is also why Apply costs one scalability rebuild instead of one per keystroke.
    for (const TPair<FName, FPending> &Pair : PendingChanges) {
        const FPending &Change = Pair.Value;
        ApplyValueForReal(Change.Def, Change.Value);

        if (Change.bHasOption) {
            // The authored payload is copied into the transaction. Re-filtering the option list on Apply
            // would make a hardware availability change shift an index onto a different profile.
            for (const TPair<FName, float> &Extra : Change.Option.ExtraCVars) {
                if (IConsoleVariable *CVar = FindCVar(Extra.Key)) {
                    CVar->Set(Extra.Value, ECVF_SetByGameOverride);
                }
            }
        }
    }
    PendingChanges.Empty();
}

void UMythicSettingAccess::RevertStaged() {
    // Nothing was ever applied, so discarding is just forgetting. No restore pass, and therefore no way
    // for a restore to disagree with what was actually set.
    PendingChanges.Empty();
}

float UMythicSettingAccess::ReadCommittedValue(const FMythicSettingDefinition &Def) {
    return ReadValueUncached(Def);
}

void UMythicSettingAccess::WriteValue(const FMythicSettingDefinition &Def, float Value) {
    // BUFFERED, not applied. The row will read this back and show it, but the game does not hear about it
    // until Apply. Settings that reach the renderer the instant you touch them cannot be explored.
    if (Def.SourceName.IsNone()) {
        return;
    }

    // Dirty means different from the committed value, not merely "the row was touched". Returning a
    // slider or toggle to where it started must disable Apply and make Back read as Back again.
    if (FMath::IsNearlyEqual(Value, ReadValueUncached(Def), 0.001f)) {
        PendingChanges.Remove(Def.SourceName);
        return;
    }

    FPending &Change = PendingChanges.FindOrAdd(Def.SourceName);
    Change.Def = Def;
    Change.Value = Value;
    Change.Option = FMythicSettingOption();
    Change.bHasOption = false;
}

void UMythicSettingAccess::ApplyValueForReal(const FMythicSettingDefinition &Def, float Value) {
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
    if (const FPending *Change = PendingChanges.Find(Def.SourceName)) {
        if (Change->bHasOption) {
            const TArray<FMythicSettingOption> Options = GetAvailableOptions(Def);
            return Options.IndexOfByPredicate(
                [Change](const FMythicSettingOption &Option) {
                    return OptionPayloadsEqual(Option, Change->Option);
                });
        }
        return ResolveOptionIndex(Def, Change->Value, /*bAllowValueOnlyFallback*/ true);
    }
    /**
     * Two options can share a primary value and differ only in the profile that rides them. Exact extra
     * cvar matches therefore win over a bare value match; a live value matching nothing reports NONE.
     */
    return ResolveOptionIndex(Def, ReadValueUncached(Def), /*bAllowValueOnlyFallback*/ true);
}

void UMythicSettingAccess::WriteOptionIndex(const FMythicSettingDefinition &Def, int32 Index) {
    const TArray<FMythicSettingOption> Options = GetAvailableOptions(Def);
    if (!Options.IsValidIndex(Index) || Def.SourceName.IsNone()) {
        return;
    }

    if (Index == ResolveOptionIndex(Def, ReadValueUncached(Def),
                                    /*bAllowValueOnlyFallback*/ false)) {
        PendingChanges.Remove(Def.SourceName);
        return;
    }
    // Copy the complete payload: two options can share a primary value and differ only by their companion
    // cvars, and replaying a filtered-array index later could target a different option.
    FPending &Change = PendingChanges.FindOrAdd(Def.SourceName);
    Change.Def = Def;
    Change.Value = Options[Index].Value;
    Change.Option = Options[Index];
    Change.bHasOption = true;
}

FText UMythicSettingAccess::GetDisplayText(const FMythicSettingDefinition &Def) {
    if (Def.Control == EMythicSettingControl::Select || Def.Control == EMythicSettingControl::Toggle) {
        const TArray<FMythicSettingOption> Options = GetAvailableOptions(Def);
        const int32 Index = ReadOptionIndex(Def);
        return Options.IsValidIndex(Index) ? Options[Index].Label : FText::GetEmpty();
    }

    const float Scale = Def.DisplayScale != 0.0f ? Def.DisplayScale : 1.0f;

    /**
     * Clamped to the authored range before it is shown.
     *
     * A cvar can sit outside the range a designer exposed - r.Tonemapper.Sharpen reads 2.0 against an
     * authored 0..1 - and the row then draws its handle pinned at the far end while printing 2.00 beside
     * it. The number and the handle disagreeing is worse than either being slightly wrong, because the
     * player cannot tell which one the game believes.
     */
    const float Raw = ReadValue(Def);
    const float Clamped = Def.MaxValue > Def.MinValue ? FMath::Clamp(Raw, Def.MinValue, Def.MaxValue) : Raw;
    const float Shown = (Clamped + Def.DisplayBias) * Scale;

    FNumberFormattingOptions Format;
    Format.MinimumFractionalDigits = Def.DisplayDecimals;
    Format.MaximumFractionalDigits = Def.DisplayDecimals;
    const FText Number = FText::AsNumber(Shown, &Format);

    return Def.DisplaySuffix.IsEmpty()
               ? Number
               : FText::Format(INVTEXT("{0}{1}"), Number, FText::FromString(Def.DisplaySuffix));
}

bool UMythicSettingAccess::IsAtDefault(const FMythicSettingDefinition &Def) {
    if (Def.Control == EMythicSettingControl::Select ||
        Def.Control == EMythicSettingControl::Toggle) {
        const int32 DefaultIndex = ResolveDefaultOptionIndex(Def);
        if (DefaultIndex == INDEX_NONE) {
            return false;
        }
        if (const FPending *Change = PendingChanges.Find(Def.SourceName)) {
            const TArray<FMythicSettingOption> Options = GetAvailableOptions(Def);
            if (Change->bHasOption) {
                return Options.IsValidIndex(DefaultIndex) &&
                       OptionPayloadsEqual(Change->Option, Options[DefaultIndex]);
            }
            return Options.IsValidIndex(DefaultIndex) &&
                   Options[DefaultIndex].ExtraCVars.IsEmpty() &&
                   FMath::IsNearlyEqual(Change->Value, Options[DefaultIndex].Value, 0.001f);
        }
        return ResolveOptionIndex(Def, ReadValueUncached(Def),
                                  /*bAllowValueOnlyFallback*/ false) == DefaultIndex;
    }
    return FMath::IsNearlyEqual(ReadValue(Def), Def.DefaultValue, 0.001f);
}

void UMythicSettingAccess::StageDefault(const FMythicSettingDefinition &Def) {
    if (Def.Control == EMythicSettingControl::Select ||
        Def.Control == EMythicSettingControl::Toggle) {
        const int32 DefaultIndex = ResolveDefaultOptionIndex(Def);
        if (DefaultIndex != INDEX_NONE) {
            WriteOptionIndex(Def, DefaultIndex);
            return;
        }
        UE_LOG(Myth, Error,
               TEXT("Setting '%s' has no unambiguous available default option; refusing a partial reset"),
               *Def.SourceName.ToString());
        return;
    }
    WriteValue(Def, Def.DefaultValue);
}

void UMythicSettingAccess::RestoreDefaults(const UMythicSettingsCatalog *Catalog) {
    if (!Catalog) {
        return;
    }
    // Reset is one proposal like every other edit. It must remain reversible until Apply, otherwise the
    // screen's Cancel button lies: pressing Reset and then Cancel would already have saved the reset.
    PendingChanges.Empty();
    for (const FMythicSettingDefinition &Def : Catalog->Settings) {
        StageDefault(Def);
    }
}

bool UMythicSettingAccess::HasNonDefaultValues(const UMythicSettingsCatalog *Catalog) {
    if (!Catalog) {
        return false;
    }
    for (const FMythicSettingDefinition &Def : Catalog->Settings) {
        if (!IsAtDefault(Def)) {
            return true;
        }
    }
    return false;
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
