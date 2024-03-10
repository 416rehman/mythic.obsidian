#pragma once

#include "GameplayTagContainer.h"
#include "UMythicUtils.generated.h"

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
};
