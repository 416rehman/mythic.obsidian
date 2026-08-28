// Copyright Stellar Games. All Rights Reserved.

#include "MythicStatTextLibrary.h"

#include "Itemization/Inventory/Fragments/FragmentTypes.h"
#include "Stats/MythicStatDefinition.h"
#include "UI/ViewModels/MythicEffectDescriber.h"
#include "UI/ViewModels/MythicStatDisplay.h"

namespace {
bool PresentationForRoll(FGameplayAttribute Attribute, bool /*bIsPercentage*/,
                         FMythicStatNumberPresentation& OutPresentation) {
    const UMythicStatDefinition* Definition = MythicStatDisplay::FindResidentDefinition(Attribute);
    if (!Definition) {
        return false;
    }
    OutPresentation = MythicStatDisplay::ResolveModifierPresentation(
        *Definition, EGameplayModOp::Additive);
    return true;
}
}

FText UMythicStatTextLibrary::GetAttributeLabel(FGameplayAttribute Attribute) {
    if (!Attribute.IsValid()) {
        return FText::GetEmpty();
    }
    if (const UMythicStatDefinition* Definition = MythicStatDisplay::FindResidentDefinition(Attribute)) {
        return Definition->DisplayName;
    }
    return MythicStatDisplay::GetUnknownStatDiagnostic();
}

FText UMythicStatTextLibrary::FormatAffixValue(FGameplayAttribute Attribute, float Value, bool bIsPercentage) {
    FMythicStatNumberPresentation Presentation;
    return PresentationForRoll(Attribute, bIsPercentage, Presentation)
        ? MythicStatDisplay::FormatBonus(Value, Presentation)
        : FText::GetEmpty();
}

FText UMythicStatTextLibrary::FormatAffixRange(FGameplayAttribute Attribute, float Min, float Max,
                                               float LevelScaling, int32 ItemLevel, bool bIsPercentage) {
    const float Scaled = LevelScaling * static_cast<float>(ItemLevel);
    const float ScaledMin = Min + Scaled;
    const float ScaledMax = Max + Scaled;

    if (FMath::IsNearlyEqual(ScaledMin, ScaledMax, 0.0001f)) {
        return FText::GetEmpty();
    }

    FMythicStatNumberPresentation Presentation;
    if (!PresentationForRoll(Attribute, bIsPercentage, Presentation)) {
        return FText::GetEmpty();
    }
    return FText::Format(NSLOCTEXT("Mythic", "AffixRange", "[{0}-{1}]"),
                         MythicStatDisplay::FormatValue(ScaledMin, Presentation),
                         MythicStatDisplay::FormatValue(ScaledMax, Presentation));
}

FText UMythicStatTextLibrary::DescribeAffixRichText(FGameplayAttribute Attribute, float Value, float Min,
                                                    float Max, float LevelScaling, int32 ItemLevel,
                                                    bool bIsPercentage) {
    if (!Attribute.IsValid()) {
        return FText::GetEmpty();
    }

    const FText ValueText = FormatAffixValue(Attribute, Value, bIsPercentage);
    const FText RangeText = FormatAffixRange(Attribute, Min, Max, LevelScaling, ItemLevel, bIsPercentage);
    const FText Label = GetAttributeLabel(Attribute);

    const FString Roll = FString::Printf(TEXT("<Roll>%s</>"), *ValueText.ToString());
    const FString Context = RangeText.IsEmpty() ? FString() : FString::Printf(TEXT("<Context>%s</>"), *RangeText.ToString());
    return FText::FromString(FString::Printf(TEXT("%s%s %s"), *Roll, *Context, *Label.ToString()));
}

FText UMythicStatTextLibrary::FormatStackCount(int32 Quantity) {
    if (Quantity <= 1) {
        return FText::GetEmpty();
    }
    return FText::FromString(FString::FromInt(Quantity));
}

FText UMythicStatTextLibrary::DescribeEffect(TSubclassOf<UGameplayEffect> EffectClass) {
    return MythicEffectDescriber::SummariseEffect(EffectClass);
}
