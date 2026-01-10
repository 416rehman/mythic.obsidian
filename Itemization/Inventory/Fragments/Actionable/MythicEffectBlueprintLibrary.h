#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ConsumableEffectFragment.h"
#include "MythicEffectBlueprintLibrary.generated.h"

/**
 * Blueprint library for working with Mythic Effect Display Data.
 * Allows calculating rich text values using an ASC context from Blueprints.
 */
UCLASS()
class MYTHIC_API UMythicEffectBlueprintLibrary : public UBlueprintFunctionLibrary {
    GENERATED_BODY()

public:
    // Generate rich text for the full effect, optionally calculating values using the ASC
    UFUNCTION(BlueprintPure, Category = "Mythic|Effect", meta = (DisplayName = "Get Rich Text (Full Effect)"))
    static FText GetEffectRichText(const FMythicEffectDisplayData &Data, UAbilitySystemComponent *ASC = nullptr, bool bShowContext = false);

    // Generate rich text for a single modifier
    UFUNCTION(BlueprintPure, Category = "Mythic|Effect", meta = (DisplayName = "Get Rich Text (Modifier)"))
    static FText GetModifierRichText(const FMythicEffectModifierDisplayData &Data, UAbilitySystemComponent *ASC = nullptr, bool bShowContext = false);

    // Generate rich text for duration
    UFUNCTION(BlueprintPure, Category = "Mythic|Effect", meta = (DisplayName = "Get Rich Text (Duration)"))
    static FText GetDurationRichText(const FMythicEffectDurationDisplayData &Data, UAbilitySystemComponent *ASC = nullptr, bool bShowContext = false);

    // Generate rich text for stacking
    UFUNCTION(BlueprintPure, Category = "Mythic|Effect", meta = (DisplayName = "Get Rich Text (Stacking)"))
    static FText GetStackingRichText(const FMythicEffectStackingDisplayData &Data);
};
