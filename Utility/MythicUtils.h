#pragma once

#include "GameplayTagContainer.h"
#include "MythicUtils.generated.h"

UCLASS()
class MYTHIC_API UMythicUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	// Returns the rightmost tag as a string. If the tag is empty, returns the default value. I.e if the tag is "A.B.C", returns "C"
	UFUNCTION(BlueprintCallable, Category = "Mythic Utils", BlueprintPure)
	static FString GetRightmostTagAsString(const FGameplayTag& Tag, const FString& Delimiter = ".", const FString& DefaultValue = "");

	// Returns the leftmost tag as a string. If the tag is empty, returns the default value. I.e if the tag is "A.B.C", returns "A"
	UFUNCTION(BlueprintCallable, Category = "Mythic Utils", BlueprintPure)
	static FString GetLeftmostTagAsString(const FGameplayTag& Tag, const FString& Delimiter = ".", const FString& DefaultValue = "");

    // Returns the rarity tag as a number. If the tag is empty, sets the out param bool to false and returns -1. I.e if the tag is "Rarity.Epic", returns 3
    UFUNCTION(BlueprintCallable, Category = "Mythic Utils", BlueprintPure, meta=(Categories="Itemization.Rarity"))
    static int32 GetRarityTagAsInt(const FGameplayTag& Tag, bool& bSuccess);
};
