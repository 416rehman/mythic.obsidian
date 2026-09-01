// Copyright Stellar Games. All Rights Reserved.

#include "UI/ViewModels/MythicStatDisplay.h"

#include "Engine/AssetManager.h"
#include "Stats/MythicStatDefinition.h"
#include "System/MythicAssetManager.h"

namespace {
bool IsStatDisplayMultiplyOperation(const TEnumAsByte<EGameplayModOp::Type> ModifierOp) {
    return ModifierOp == EGameplayModOp::MultiplyAdditive
        || ModifierOp == EGameplayModOp::MultiplyCompound
        || ModifierOp == EGameplayModOp::DivideAdditive;
}

int32 EffectiveDecimalPlaces(const FMythicStatNumberPresentation& Presentation) {
    if (Presentation.Format == EMythicStatFormat::Integer
        || Presentation.Format == EMythicStatFormat::Bipolar) {
        return 0;
    }
    return FMath::Clamp(Presentation.DecimalPlaces, 0, 4);
}

double ToDisplayNumber(float Value, EMythicStatFormat Format) {
    switch (Format) {
    case EMythicStatFormat::Percent:
        return static_cast<double>(Value) * 100.0;
    case EMythicStatFormat::Multiplier:
        return (static_cast<double>(Value) - 1.0) * 100.0;
    default:
        return static_cast<double>(Value);
    }
}

double ToDisplayDelta(float Delta, EMythicStatFormat Format) {
    return Format == EMythicStatFormat::Percent || Format == EMythicStatFormat::Multiplier
        ? static_cast<double>(Delta) * 100.0
        : static_cast<double>(Delta);
}

FText FormatNumber(double Value, int32 DecimalPlaces) {
    FNumberFormattingOptions Options;
    Options.SetMinimumFractionalDigits(0);
    Options.SetMaximumFractionalDigits(DecimalPlaces);
    return FText::AsNumber(Value, &Options);
}

FText BuiltInSuffix(EMythicStatFormat Format) {
    switch (Format) {
    case EMythicStatFormat::Percent:
    case EMythicStatFormat::Multiplier:
        return FText::FromString(TEXT("%"));
    case EMythicStatFormat::PerSecond:
        return NSLOCTEXT("MythicStats", "PerSecondSuffix", "/s");
    default:
        return FText::GetEmpty();
    }
}

FText JoinNumberAndSuffix(const FText& Number, const FMythicStatNumberPresentation& Presentation) {
    return FText::Format(NSLOCTEXT("MythicStats", "NumberWithSuffix", "{0}{1}{2}"),
                         Number, BuiltInSuffix(Presentation.Format), Presentation.UnitSuffix);
}

FMythicStatNumberPresentation DefaultPresentationForFormat(EMythicStatFormat Format) {
    FMythicStatNumberPresentation Presentation;
    Presentation.Format = Format;
    switch (Format) {
    case EMythicStatFormat::Integer:
    case EMythicStatFormat::Bipolar:
        Presentation.DecimalPlaces = 0;
        break;
    default:
        Presentation.DecimalPlaces = 1;
        break;
    }
    return Presentation;
}
}

namespace MythicStatDisplay {
float QuantizeValueToDisplayPrecision(
    const float Value,
    const FMythicStatNumberPresentation& Presentation) {
    if (!FMath::IsFinite(Value)) {
        return Value;
    }

    const int32 DecimalPlaces = EffectiveDecimalPlaces(Presentation);
    const double PrecisionScale = FMath::Pow(10.0, static_cast<double>(DecimalPlaces));
    const double DisplayValue = ToDisplayNumber(Value, Presentation.Format);
    const double QuantizedDisplay = FMath::RoundHalfToEven(DisplayValue * PrecisionScale)
        / PrecisionScale;

    switch (Presentation.Format) {
    case EMythicStatFormat::Percent:
        return static_cast<float>(QuantizedDisplay / 100.0);
    case EMythicStatFormat::Multiplier:
        return static_cast<float>((QuantizedDisplay / 100.0) + 1.0);
    default:
        return static_cast<float>(QuantizedDisplay);
    }
}

FText FormatValue(float Value, const FMythicStatNumberPresentation& Presentation) {
    if (!FMath::IsFinite(Value)) {
        return FText::GetEmpty();
    }

    const double DisplayValue = ToDisplayNumber(Value, Presentation.Format);
    return JoinNumberAndSuffix(FormatNumber(DisplayValue, EffectiveDecimalPlaces(Presentation)), Presentation);
}

FText FormatBonus(float Delta, const FMythicStatNumberPresentation& Presentation) {
    if (!FMath::IsFinite(Delta)
        || FMath::Abs(Delta) <= MythicStatPresentation::GetComparisonEpsilon(Presentation)) {
        return FText::GetEmpty();
    }

    const FText Magnitude = FormatNumber(
        FMath::Abs(ToDisplayDelta(Delta, Presentation.Format)), EffectiveDecimalPlaces(Presentation));
    const FText Signed = FText::Format(
        NSLOCTEXT("MythicStats", "SignedNumber", "{0}{1}"),
        Delta > 0.0f ? FText::FromString(TEXT("+")) : FText::FromString(TEXT("-")), Magnitude);
    return JoinNumberAndSuffix(Signed, Presentation);
}

FText FormatValue(float Value, EMythicStatFormat Format) {
    return FormatValue(Value, DefaultPresentationForFormat(Format));
}

FText FormatBonus(float Delta, EMythicStatFormat Format) {
    return FormatBonus(Delta, DefaultPresentationForFormat(Format));
}

FMythicStatNumberPresentation ResolveModifierPresentation(
    const UMythicStatDefinition& Definition,
    TEnumAsByte<EGameplayModOp::Type> ModifierOp) {
    FMythicStatNumberPresentation Presentation = Definition.NumberPresentation;
    if (IsStatDisplayMultiplyOperation(ModifierOp)) {
        Presentation.Format = EMythicStatFormat::Multiplier;
    }
    else if (Presentation.Format == EMythicStatFormat::Multiplier) {
        // An additive modifier on a multiplier-valued stat contributes percentage points.
        Presentation.Format = EMythicStatFormat::Percent;
    }
    return Presentation;
}

float GetModifierContributionIdentity(
    const TEnumAsByte<EGameplayModOp::Type> ModifierOp,
    const float FinalStatNeutralValue) {
    switch (ModifierOp.GetValue()) {
    case EGameplayModOp::AddBase:
    case EGameplayModOp::AddFinal:
        return 0.0f;
    case EGameplayModOp::MultiplyAdditive:
    case EGameplayModOp::MultiplyCompound:
    case EGameplayModOp::DivideAdditive:
        return 1.0f;
    case EGameplayModOp::Override:
    default:
        return FinalStatNeutralValue;
    }
}

bool ShouldRender(const UMythicStatDefinition& Definition, float BaseValue, float CurrentValue) {
    switch (Definition.SheetVisibility) {
    case EMythicStatSheetVisibility::Hidden:
        return false;
    case EMythicStatSheetVisibility::Always:
        return true;
    case EMythicStatSheetVisibility::WhenModifiedOrNonNeutral: {
        if (!FMath::IsFinite(BaseValue) || !FMath::IsFinite(CurrentValue)) {
            return false;
        }
        const float Epsilon = MythicStatPresentation::GetComparisonEpsilon(Definition.NumberPresentation);
        return FMath::Abs(CurrentValue - BaseValue) > Epsilon
            || FMath::Abs(BaseValue - Definition.NeutralValue) > Epsilon
            || FMath::Abs(CurrentValue - Definition.NeutralValue) > Epsilon;
    }
    default:
        return false;
    }
}

const UMythicStatDefinition* FindResidentDefinition(const FGameplayAttribute& Attribute) {
    if (!Attribute.IsValid() || !IsInGameThread()) {
        return nullptr;
    }

    UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
    if (!AssetManager) {
        return nullptr;
    }

    TArray<FPrimaryAssetId> StatIds;
    AssetManager->GetPrimaryAssetIdList(UMythicAssetManager::StatDefinitionType, StatIds);

    const UMythicStatDefinition* Match = nullptr;
    for (const FPrimaryAssetId& StatId : StatIds) {
        const UMythicStatDefinition* Candidate =
            Cast<UMythicStatDefinition>(AssetManager->GetPrimaryAssetObject(StatId));
        if (!Candidate || Candidate->Attribute != Attribute) {
            continue;
        }
        if (Match && Match != Candidate) {
#if !UE_BUILD_SHIPPING
            UE_LOG(LogTemp, Error,
                   TEXT("Duplicate loaded StatDefinitions map GAS attribute '%s': %s and %s."),
                   *Attribute.GetName(), *GetNameSafe(Match), *GetNameSafe(Candidate));
#endif
            return nullptr;
        }
        Match = Candidate;
    }
    return Match;
}

FText GetUnknownStatDiagnostic() {
#if UE_BUILD_SHIPPING
    return FText::GetEmpty();
#else
    return NSLOCTEXT("MythicStats", "UnknownStatDiagnostic", "Unknown Stat");
#endif
}
}
