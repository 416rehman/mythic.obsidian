// Copyright Stellar Games. All Rights Reserved.

#include "MythicEffectDescriber.h"

#include "GameplayEffect.h"
#include "Itemization/Inventory/Fragments/FragmentTypes.h"
#include "Stats/MythicStatDefinition.h"

namespace {
bool IsEffectDescriberMultiplyOperation(const TEnumAsByte<EGameplayModOp::Type> Op) {
    return Op == EGameplayModOp::MultiplyAdditive || Op == EGameplayModOp::MultiplyCompound;
}

bool IsBeneficial(const UMythicStatDefinition& Definition, float Magnitude,
                  TEnumAsByte<EGameplayModOp::Type> Op) {
    if (Definition.ComparisonDirection == EMythicStatComparisonDirection::Neutral) {
        return false;
    }

    const float Change = IsEffectDescriberMultiplyOperation(Op)
        ? Magnitude - 1.0f
        : Op == EGameplayModOp::Override
            ? Magnitude - Definition.NeutralValue
            : Magnitude;
    return Definition.ComparisonDirection == EMythicStatComparisonDirection::HigherIsBetter
        ? Change >= 0.0f
        : Change <= 0.0f;
}

void BuildRichText(FMythicEffectLine &Line) {
    const FString Roll = FString::Printf(TEXT("<Roll>%s</>"), *Line.Value.ToString());
    const FString Context = Line.Range.IsEmpty() ? FString() : FString::Printf(TEXT("<Context>%s</>"), *Line.Range.ToString());
    Line.RichText = FText::FromString(FString::Printf(TEXT("%s%s %s"), *Roll, *Context, *Line.Label.ToString()));
}
}

namespace MythicEffectDescriber {
FText MakeRollMarkup(const FText &FormattedValue, float Min, float Max, EMythicStatFormat Format) {
    const FString Roll = FString::Printf(TEXT("<Roll>%s</>"), *FormattedValue.ToString());
    if (FMath::IsNearlyEqual(Min, Max, 0.0001f)) {
        return FText::FromString(Roll);
    }
    const FText MinText = MythicStatDisplay::FormatValue(Min, Format);
    const FText MaxText = MythicStatDisplay::FormatValue(Max, Format);
    return FText::FromString(FString::Printf(TEXT("%s<Context>[%s-%s]</>"), *Roll, *MinText.ToString(), *MaxText.ToString()));
}

FMythicEffectLine DescribeModifier(const FGameplayAttribute &Attribute, float Magnitude,
                                   TEnumAsByte<EGameplayModOp::Type> Op) {
    FMythicEffectLine Line;
    if (!Attribute.IsValid()) {
        return Line;
    }

    const UMythicStatDefinition* Definition = MythicStatDisplay::FindResidentDefinition(Attribute);
    if (!Definition) {
        Line.Label = MythicStatDisplay::GetUnknownStatDiagnostic();
        return Line;
    }
    const FMythicStatNumberPresentation Presentation =
        MythicStatDisplay::ResolveModifierPresentation(*Definition, Op);

    Line.Label = Definition->DisplayName;
    Line.RawMagnitude = Magnitude;
    Line.Format = Presentation.Format;
    Line.bPositive = IsBeneficial(*Definition, Magnitude, Op);

    if (IsEffectDescriberMultiplyOperation(Op)) {
        Line.Value = MythicStatDisplay::FormatValue(Magnitude, Presentation);
    }
    else if (Op == EGameplayModOp::Override) {
        Line.Value = MythicStatDisplay::FormatValue(Magnitude, Presentation);
    }
    else {
        Line.Value = MythicStatDisplay::FormatBonus(Magnitude, Presentation);
    }

    BuildRichText(Line);
    return Line;
}

FMythicEffectLine DescribeRolledModifier(const FGameplayAttribute &Attribute, float Value, const FRollDefinition &Roll,
                                         int32 ItemLevel) {
    FMythicEffectLine Line = DescribeModifier(Attribute, Value, Roll.Modifier);

    const UMythicStatDefinition* Definition = MythicStatDisplay::FindResidentDefinition(Attribute);
    if (!Definition) {
        return Line;
    }
    const FMythicStatNumberPresentation Presentation = MythicStatDisplay::ResolveModifierPresentation(
        *Definition, Roll.Modifier);
    if (Presentation.Format != Line.Format) {
        Line.Format = Presentation.Format;
        Line.Value = Roll.Modifier == EGameplayModOp::Override
            ? MythicStatDisplay::FormatValue(Value, Presentation)
            : MythicStatDisplay::FormatBonus(Value, Presentation);
    }

    const float ScaledMin = Roll.GetScaledMin(ItemLevel);
    const float ScaledMax = Roll.GetScaledMax(ItemLevel);
    if (!FMath::IsNearlyEqual(ScaledMin, ScaledMax, 0.0001f)) {
        const FText MinText = MythicStatDisplay::FormatValue(ScaledMin, Presentation);
        const FText MaxText = MythicStatDisplay::FormatValue(ScaledMax, Presentation);
        Line.Range = FText::FromString(FString::Printf(TEXT("[%s-%s]"), *MinText.ToString(), *MaxText.ToString()));
    }

    // A roll may invert the stat's own polarity: a cooldown authored -10%..-2% is a reduction, and on a
    // lower-is-better roll the reduction is the good outcome the player wants to read as a gain.
    if (Roll.bLowerIsBetter) {
        const bool bScalesAroundOne = Roll.Modifier == EGameplayModOp::Multiplicitive
            || Roll.Modifier == EGameplayModOp::MultiplyAdditive
            || Roll.Modifier == EGameplayModOp::MultiplyCompound;
        Line.bPositive = bScalesAroundOne ? Value < 1.0f : Value < 0.0f;
    }

    BuildRichText(Line);
    return Line;
}

TArray<FMythicEffectLine> DescribeEffect(TSubclassOf<UGameplayEffect> EffectClass) {
    TArray<FMythicEffectLine> Lines;
    if (!EffectClass) {
        return Lines;
    }

    const UGameplayEffect *Effect = EffectClass->GetDefaultObject<UGameplayEffect>();
    if (!Effect) {
        return Lines;
    }

    for (const FGameplayModifierInfo &Mod : Effect->Modifiers) {
        if (!Mod.Attribute.IsValid()) {
            continue;
        }

        float Magnitude = 0.0f;
        if (!Mod.ModifierMagnitude.GetStaticMagnitudeIfPossible(1.0f, Magnitude)) {
            continue;
        }
        if (FMath::IsNearlyZero(Magnitude, 0.0001f) && Mod.ModifierOp != EGameplayModOp::Override) {
            continue;
        }

        Lines.Add(DescribeModifier(Mod.Attribute, Magnitude, Mod.ModifierOp));
    }

    return Lines;
}

FText SummariseEffect(TSubclassOf<UGameplayEffect> EffectClass) {
    const TArray<FMythicEffectLine> Lines = DescribeEffect(EffectClass);
    if (Lines.Num() == 0) {
        return FText::GetEmpty();
    }

    TArray<FString> Parts;
    Parts.Reserve(Lines.Num());
    for (const FMythicEffectLine &Line : Lines) {
        Parts.Add(Line.RichText.ToString());
    }
    return FText::FromString(FString::Join(Parts, TEXT("  ")));
}

FText SummariseEffectPlain(TSubclassOf<UGameplayEffect> EffectClass) {
    const TArray<FMythicEffectLine> Lines = DescribeEffect(EffectClass);
    if (Lines.Num() == 0) {
        return FText::GetEmpty();
    }

    TArray<FString> Parts;
    Parts.Reserve(Lines.Num());
    for (const FMythicEffectLine &Line : Lines) {
        Parts.Add(FString::Printf(TEXT("%s %s"), *Line.Value.ToString(), *Line.Label.ToString()));
    }
    return FText::FromString(FString::Join(Parts, TEXT(", ")));
}
}
