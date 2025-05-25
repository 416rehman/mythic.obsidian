#include "MythicUtils.h"

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

int32 UMythicUtils::GetRarityTagAsInt(const FGameplayTag &Tag, bool& bSuccess) {
    static const TMap<FName, int32> RarityMap = {
        {FName("Rarity.Common"), 0},
        {FName("Rarity.Uncommon"), 1},
        {FName("Rarity.Rare"), 2},
        {FName("Rarity.Epic"), 3},
        {FName("Rarity.Legendary"), 4},
        {FName("Rarity.Mythic"), 5}
    };
    bSuccess = RarityMap.Contains(Tag.GetTagName());
    return bSuccess ? RarityMap[Tag.GetTagName()] : -1;
}

