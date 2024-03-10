#include "UMythicUtils.h"
FString UMythicUtils::GetRightmostTagAsString(const FGameplayTag& Tag, const FString& Delimiter, const FString& DefaultValue) {
	if (Tag == FGameplayTag::EmptyTag) {
		return DefaultValue;
	}
	
	auto TagString = Tag.ToString();
	FString Value;
	TagString.Split(Delimiter, nullptr, &Value, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	return Value.IsEmpty() ? DefaultValue : Value;
	
}

FString UMythicUtils::GetLeftmostTagAsString(const FGameplayTag& Tag, const FString& Delimiter, const FString& DefaultValue) {
	if (Tag == FGameplayTag::EmptyTag) {
		return DefaultValue;
	}
	
	auto TagString = Tag.ToString();
	FString Value;
	TagString.Split(Delimiter, &Value, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromStart);
	return Value.IsEmpty() ? DefaultValue : Value;	
}

