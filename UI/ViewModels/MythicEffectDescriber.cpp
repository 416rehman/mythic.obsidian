// Copyright Stellar Games. All Rights Reserved.

#include "MythicEffectDescriber.h"

#include "GameplayEffect.h"
#include "Itemization/Inventory/Fragments/FragmentTypes.h"

namespace {
bool AttributeIsGoodWhenHigher(const FGameplayAttribute &Attribute) {
    static const TSet<FString> LowerIsBetter = {
        TEXT("IncomingDamageMultiplier"),
        TEXT("BurnBuildup"), TEXT("BleedBuildup"), TEXT("PoisonBuildup"),
        TEXT("SlowBuildup"), TEXT("FreezeBuildup"), TEXT("StunBuildup"),
    };
    return !LowerIsBetter.Contains(Attribute.GetName());
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

    const FMythicStatRule Rule = MythicStatDisplay::GetRule(Attribute);
    const EMythicStatFormat Format = MythicStatDisplay::ResolveRollFormat(Attribute, Op);

    Line.Label = FText::FromString(Rule.Label);
    Line.RawMagnitude = Magnitude;
    Line.Format = Format;

    if (Op == EGameplayModOp::Multiplicitive) {
        Line.Value = MythicStatDisplay::FormatValue(Magnitude, EMythicStatFormat::Multiplier);
        Line.bPositive = (Magnitude >= 1.0f) == AttributeIsGoodWhenHigher(Attribute);
    }
    else if (Op == EGameplayModOp::Override) {
        Line.Value = MythicStatDisplay::FormatValue(Magnitude, Format);
        Line.bPositive = AttributeIsGoodWhenHigher(Attribute);
    }
    else {
        Line.Value = MythicStatDisplay::FormatBonus(Magnitude, Format);
        Line.bPositive = (Magnitude >= 0.0f) == AttributeIsGoodWhenHigher(Attribute);
    }

    BuildRichText(Line);
    return Line;
}

FMythicEffectLine DescribeRolledModifier(const FGameplayAttribute &Attribute, float Value, const FRollDefinition &Roll,
                                         int32 ItemLevel) {
    FMythicEffectLine Line = DescribeModifier(Attribute, Value, Roll.Modifier);

    const EMythicStatFormat Forced = MythicStatDisplay::ResolveRollFormat(Attribute, Roll.Modifier, Roll.bIsPercentage);
    if (Forced != Line.Format) {
        Line.Format = Forced;
        Line.Value = MythicStatDisplay::FormatBonus(Value, Forced);
    }
    if (Roll.bLowerIsBetter) {
        Line.bPositive = Value <= 0.0f;
    }

    const float ScaledMin = Roll.GetScaledMin(ItemLevel);
    const float ScaledMax = Roll.GetScaledMax(ItemLevel);
    if (!FMath::IsNearlyEqual(ScaledMin, ScaledMax, 0.0001f)) {
        const FText MinText = MythicStatDisplay::FormatValue(ScaledMin, Line.Format);
        const FText MaxText = MythicStatDisplay::FormatValue(ScaledMax, Line.Format);
        Line.Range = FText::FromString(FString::Printf(TEXT("[%s-%s]"), *MinText.ToString(), *MaxText.ToString()));
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
