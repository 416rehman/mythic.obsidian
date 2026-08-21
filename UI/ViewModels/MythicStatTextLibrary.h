// Copyright Stellar Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MythicStatTextLibrary.generated.h"

struct FRollDefinition;

UCLASS()
class MYTHIC_API UMythicStatTextLibrary : public UBlueprintFunctionLibrary {
    GENERATED_BODY()

public:
    /**
     * The player-facing name of an attribute: "BonusDaggerDamage" -> "Bonus Dagger Damage".
     * Never returns a raw property name — unmapped attributes are split from camelCase instead.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Stats", meta = (DisplayName = "Get Attribute Label"))
    static FText GetAttributeLabel(FGameplayAttribute Attribute);

    /**
     * A rolled value formatted the way that attribute should read: a 0..1 chance becomes "+15%", a flat stat "+12".
     * Signed, so the line is scannable without reading it.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Stats", meta = (DisplayName = "Format Affix Value"))
    static FText FormatAffixValue(FGameplayAttribute Attribute, float Value, bool bIsPercentage);

    /**
     * The authored spread for a roll, level scaling applied, formatted to MATCH the value beside it.
     * Returns empty when Min and Max are the same, so a fixed affix shows no invented range.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Stats", meta = (DisplayName = "Format Affix Range"))
    static FText FormatAffixRange(FGameplayAttribute Attribute, float Min, float Max, float LevelScaling,
                                  int32 ItemLevel, bool bIsPercentage);

    /**
     * The whole affix as one rich-text line in the project's markup:
     * "<Roll>+15%</><Context>[10-20]</> Bonus Dagger Damage".
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Stats", meta = (DisplayName = "Describe Affix Rich Text"))
    static FText DescribeAffixRichText(FGameplayAttribute Attribute, float Value, float Min, float Max,
                                       float LevelScaling, int32 ItemLevel, bool bIsPercentage);

    /**
     * The number written on an inventory square, or nothing.
     *
     * An empty square must stay empty and a single item must not be labelled "1" — a stack count is only news when
     * there is a stack. The inventory slot used the engine's raw int-to-text conversion, so every empty square in
     * the game stamped a "0" on itself.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Stats", meta = (DisplayName = "Format Stack Count"))
    static FText FormatStackCount(int32 Quantity);

    /**
     * Everything a GameplayEffect does, as one rich-text line. This is what gives a rune a description: they carry
     * only a name, an icon and an effect class, and the effect is where the real numbers live.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Stats", meta = (DisplayName = "Describe Effect"))
    static FText DescribeEffect(TSubclassOf<class UGameplayEffect> EffectClass);
};
