// Copyright Stellar Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayEffectTypes.h"
#include "Templates/SubclassOf.h"
#include "UI/ViewModels/MythicStatDisplay.h"
#include "MythicEffectDescriber.generated.h"

class UGameplayEffect;
struct FRollDefinition;

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicEffectLine {
    GENERATED_BODY()

    /** The stat's friendly name, e.g. "Critical Hit Chance". Never a raw property name. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Describe")
    FText Label;

    /** The signed, formatted magnitude, e.g. "+5%". */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Describe")
    FText Value;

    /** The authored spread when the source rolls a range, e.g. "[8-15]". Empty when the value is fixed. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Describe")
    FText Range;

    /** The whole line in the project's markup: "<Roll>+5%</><Context>[3-8]</> Critical Hit Chance". */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Describe")
    FText RichText;

    /** True when the change helps the player, so the UI can colour it without re-deriving the sign. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Describe")
    bool bPositive = true;

    /** The raw number behind Value, for anything that wants to compare rather than print. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Describe")
    float RawMagnitude = 0.0f;

    /** How Value was formatted, so a range added later cannot disagree with the value beside it. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Describe")
    EMythicStatFormat Format = EMythicStatFormat::Flat;
};

namespace MythicEffectDescriber {
MYTHIC_API FText MakeRollMarkup(const FText &FormattedValue, float Min, float Max, EMythicStatFormat Format);

MYTHIC_API FMythicEffectLine DescribeModifier(const FGameplayAttribute &Attribute, float Magnitude,
                                              TEnumAsByte<EGameplayModOp::Type> Op = EGameplayModOp::Additive);

/**
 * ItemLevel scales the printed range the same way the roll scaled the value. Passing 0 prints the authored
 * band, which only matches the value on an unscaled roll.
 */
MYTHIC_API FMythicEffectLine DescribeRolledModifier(const FGameplayAttribute &Attribute, float Value,
                                                    const FRollDefinition &Roll, int32 ItemLevel = 0);

MYTHIC_API TArray<FMythicEffectLine> DescribeEffect(TSubclassOf<UGameplayEffect> EffectClass);

MYTHIC_API FText SummariseEffect(TSubclassOf<UGameplayEffect> EffectClass);

MYTHIC_API FText SummariseEffectPlain(TSubclassOf<UGameplayEffect> EffectClass);
}
