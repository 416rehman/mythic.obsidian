// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/GameSetting.h"
#include "Runtime/GameplayTags/Classes/GameplayTagContainer.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGameSetting() {}

// Begin Cross Module References
COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
ENGINE_API UClass* Z_Construct_UClass_ULocalPlayer_NoRegister();
GAMEPLAYTAGS_API UScriptStruct* Z_Construct_UScriptStruct_FGameplayTagContainer();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSetting();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSetting_NoRegister();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingRegistry_NoRegister();
UMG_API UEnum* Z_Construct_UEnum_UMG_ESlateVisibility();
UPackage* Z_Construct_UPackage__Script_GameSettings();
// End Cross Module References

// Begin Class UGameSetting Function GetDescriptionRichText
struct Z_Construct_UFunction_UGameSetting_GetDescriptionRichText_Statics
{
	struct GameSetting_eventGetDescriptionRichText_Parms
	{
		FText ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/GameSetting.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FTextPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FTextPropertyParams Z_Construct_UFunction_UGameSetting_GetDescriptionRichText_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(GameSetting_eventGetDescriptionRichText_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UGameSetting_GetDescriptionRichText_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UGameSetting_GetDescriptionRichText_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSetting_GetDescriptionRichText_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UGameSetting_GetDescriptionRichText_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UGameSetting, nullptr, "GetDescriptionRichText", nullptr, nullptr, Z_Construct_UFunction_UGameSetting_GetDescriptionRichText_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSetting_GetDescriptionRichText_Statics::PropPointers), sizeof(Z_Construct_UFunction_UGameSetting_GetDescriptionRichText_Statics::GameSetting_eventGetDescriptionRichText_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSetting_GetDescriptionRichText_Statics::Function_MetaDataParams), Z_Construct_UFunction_UGameSetting_GetDescriptionRichText_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UGameSetting_GetDescriptionRichText_Statics::GameSetting_eventGetDescriptionRichText_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UGameSetting_GetDescriptionRichText()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UGameSetting_GetDescriptionRichText_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UGameSetting::execGetDescriptionRichText)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FText*)Z_Param__Result=P_THIS->GetDescriptionRichText();
	P_NATIVE_END;
}
// End Class UGameSetting Function GetDescriptionRichText

// Begin Class UGameSetting Function GetDevName
struct Z_Construct_UFunction_UGameSetting_GetDevName_Statics
{
	struct GameSetting_eventGetDevName_Parms
	{
		FName ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Gets the non-localized developer name for this setting.  This should remain constant, and represent a \n\x09 * unique identifier for this setting inside this settings registry.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/GameSetting.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Gets the non-localized developer name for this setting.  This should remain constant, and represent a\nunique identifier for this setting inside this settings registry." },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FNamePropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FNamePropertyParams Z_Construct_UFunction_UGameSetting_GetDevName_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(GameSetting_eventGetDevName_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UGameSetting_GetDevName_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UGameSetting_GetDevName_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSetting_GetDevName_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UGameSetting_GetDevName_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UGameSetting, nullptr, "GetDevName", nullptr, nullptr, Z_Construct_UFunction_UGameSetting_GetDevName_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSetting_GetDevName_Statics::PropPointers), sizeof(Z_Construct_UFunction_UGameSetting_GetDevName_Statics::GameSetting_eventGetDevName_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSetting_GetDevName_Statics::Function_MetaDataParams), Z_Construct_UFunction_UGameSetting_GetDevName_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UGameSetting_GetDevName_Statics::GameSetting_eventGetDevName_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UGameSetting_GetDevName()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UGameSetting_GetDevName_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UGameSetting::execGetDevName)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FName*)Z_Param__Result=P_THIS->GetDevName();
	P_NATIVE_END;
}
// End Class UGameSetting Function GetDevName

// Begin Class UGameSetting Function GetDisplayName
struct Z_Construct_UFunction_UGameSetting_GetDisplayName_Statics
{
	struct GameSetting_eventGetDisplayName_Parms
	{
		FText ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/GameSetting.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FTextPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FTextPropertyParams Z_Construct_UFunction_UGameSetting_GetDisplayName_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(GameSetting_eventGetDisplayName_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UGameSetting_GetDisplayName_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UGameSetting_GetDisplayName_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSetting_GetDisplayName_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UGameSetting_GetDisplayName_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UGameSetting, nullptr, "GetDisplayName", nullptr, nullptr, Z_Construct_UFunction_UGameSetting_GetDisplayName_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSetting_GetDisplayName_Statics::PropPointers), sizeof(Z_Construct_UFunction_UGameSetting_GetDisplayName_Statics::GameSetting_eventGetDisplayName_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSetting_GetDisplayName_Statics::Function_MetaDataParams), Z_Construct_UFunction_UGameSetting_GetDisplayName_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UGameSetting_GetDisplayName_Statics::GameSetting_eventGetDisplayName_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UGameSetting_GetDisplayName()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UGameSetting_GetDisplayName_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UGameSetting::execGetDisplayName)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FText*)Z_Param__Result=P_THIS->GetDisplayName();
	P_NATIVE_END;
}
// End Class UGameSetting Function GetDisplayName

// Begin Class UGameSetting Function GetDisplayNameVisibility
struct Z_Construct_UFunction_UGameSetting_GetDisplayNameVisibility_Statics
{
	struct GameSetting_eventGetDisplayNameVisibility_Parms
	{
		ESlateVisibility ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/GameSetting.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FBytePropertyParams NewProp_ReturnValue_Underlying;
	static const UECodeGen_Private::FEnumPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FBytePropertyParams Z_Construct_UFunction_UGameSetting_GetDisplayNameVisibility_Statics::NewProp_ReturnValue_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FEnumPropertyParams Z_Construct_UFunction_UGameSetting_GetDisplayNameVisibility_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(GameSetting_eventGetDisplayNameVisibility_Parms, ReturnValue), Z_Construct_UEnum_UMG_ESlateVisibility, METADATA_PARAMS(0, nullptr) }; // 2974316103
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UGameSetting_GetDisplayNameVisibility_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UGameSetting_GetDisplayNameVisibility_Statics::NewProp_ReturnValue_Underlying,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UGameSetting_GetDisplayNameVisibility_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSetting_GetDisplayNameVisibility_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UGameSetting_GetDisplayNameVisibility_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UGameSetting, nullptr, "GetDisplayNameVisibility", nullptr, nullptr, Z_Construct_UFunction_UGameSetting_GetDisplayNameVisibility_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSetting_GetDisplayNameVisibility_Statics::PropPointers), sizeof(Z_Construct_UFunction_UGameSetting_GetDisplayNameVisibility_Statics::GameSetting_eventGetDisplayNameVisibility_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSetting_GetDisplayNameVisibility_Statics::Function_MetaDataParams), Z_Construct_UFunction_UGameSetting_GetDisplayNameVisibility_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UGameSetting_GetDisplayNameVisibility_Statics::GameSetting_eventGetDisplayNameVisibility_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UGameSetting_GetDisplayNameVisibility()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UGameSetting_GetDisplayNameVisibility_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UGameSetting::execGetDisplayNameVisibility)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(ESlateVisibility*)Z_Param__Result=P_THIS->GetDisplayNameVisibility();
	P_NATIVE_END;
}
// End Class UGameSetting Function GetDisplayNameVisibility

// Begin Class UGameSetting Function GetDynamicDetails
struct Z_Construct_UFunction_UGameSetting_GetDynamicDetails_Statics
{
	struct GameSetting_eventGetDynamicDetails_Parms
	{
		FText ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Gets the dynamic details about this setting.  This may be information like, how many refunds are remaining \n\x09 * on their account, or the account number.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/GameSetting.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Gets the dynamic details about this setting.  This may be information like, how many refunds are remaining\non their account, or the account number." },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FTextPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FTextPropertyParams Z_Construct_UFunction_UGameSetting_GetDynamicDetails_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(GameSetting_eventGetDynamicDetails_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UGameSetting_GetDynamicDetails_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UGameSetting_GetDynamicDetails_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSetting_GetDynamicDetails_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UGameSetting_GetDynamicDetails_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UGameSetting, nullptr, "GetDynamicDetails", nullptr, nullptr, Z_Construct_UFunction_UGameSetting_GetDynamicDetails_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSetting_GetDynamicDetails_Statics::PropPointers), sizeof(Z_Construct_UFunction_UGameSetting_GetDynamicDetails_Statics::GameSetting_eventGetDynamicDetails_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSetting_GetDynamicDetails_Statics::Function_MetaDataParams), Z_Construct_UFunction_UGameSetting_GetDynamicDetails_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UGameSetting_GetDynamicDetails_Statics::GameSetting_eventGetDynamicDetails_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UGameSetting_GetDynamicDetails()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UGameSetting_GetDynamicDetails_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UGameSetting::execGetDynamicDetails)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FText*)Z_Param__Result=P_THIS->GetDynamicDetails();
	P_NATIVE_END;
}
// End Class UGameSetting Function GetDynamicDetails

// Begin Class UGameSetting Function GetTags
struct Z_Construct_UFunction_UGameSetting_GetTags_Statics
{
	struct GameSetting_eventGetTags_Parms
	{
		FGameplayTagContainer ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/GameSetting.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ReturnValue_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStructPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_UGameSetting_GetTags_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000008000582, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(GameSetting_eventGetTags_Parms, ReturnValue), Z_Construct_UScriptStruct_FGameplayTagContainer, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ReturnValue_MetaData), NewProp_ReturnValue_MetaData) }; // 3352185621
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UGameSetting_GetTags_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UGameSetting_GetTags_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSetting_GetTags_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UGameSetting_GetTags_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UGameSetting, nullptr, "GetTags", nullptr, nullptr, Z_Construct_UFunction_UGameSetting_GetTags_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSetting_GetTags_Statics::PropPointers), sizeof(Z_Construct_UFunction_UGameSetting_GetTags_Statics::GameSetting_eventGetTags_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSetting_GetTags_Statics::Function_MetaDataParams), Z_Construct_UFunction_UGameSetting_GetTags_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UGameSetting_GetTags_Statics::GameSetting_eventGetTags_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UGameSetting_GetTags()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UGameSetting_GetTags_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UGameSetting::execGetTags)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FGameplayTagContainer*)Z_Param__Result=P_THIS->GetTags();
	P_NATIVE_END;
}
// End Class UGameSetting Function GetTags

// Begin Class UGameSetting Function GetWarningRichText
struct Z_Construct_UFunction_UGameSetting_GetWarningRichText_Statics
{
	struct GameSetting_eventGetWarningRichText_Parms
	{
		FText ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/GameSetting.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FTextPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FTextPropertyParams Z_Construct_UFunction_UGameSetting_GetWarningRichText_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(GameSetting_eventGetWarningRichText_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UGameSetting_GetWarningRichText_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UGameSetting_GetWarningRichText_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSetting_GetWarningRichText_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UGameSetting_GetWarningRichText_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UGameSetting, nullptr, "GetWarningRichText", nullptr, nullptr, Z_Construct_UFunction_UGameSetting_GetWarningRichText_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSetting_GetWarningRichText_Statics::PropPointers), sizeof(Z_Construct_UFunction_UGameSetting_GetWarningRichText_Statics::GameSetting_eventGetWarningRichText_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSetting_GetWarningRichText_Statics::Function_MetaDataParams), Z_Construct_UFunction_UGameSetting_GetWarningRichText_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UGameSetting_GetWarningRichText_Statics::GameSetting_eventGetWarningRichText_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UGameSetting_GetWarningRichText()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UGameSetting_GetWarningRichText_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UGameSetting::execGetWarningRichText)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FText*)Z_Param__Result=P_THIS->GetWarningRichText();
	P_NATIVE_END;
}
// End Class UGameSetting Function GetWarningRichText

// Begin Class UGameSetting
void UGameSetting::StaticRegisterNativesUGameSetting()
{
	UClass* Class = UGameSetting::StaticClass();
	static const FNameNativePtrPair Funcs[] = {
		{ "GetDescriptionRichText", &UGameSetting::execGetDescriptionRichText },
		{ "GetDevName", &UGameSetting::execGetDevName },
		{ "GetDisplayName", &UGameSetting::execGetDisplayName },
		{ "GetDisplayNameVisibility", &UGameSetting::execGetDisplayNameVisibility },
		{ "GetDynamicDetails", &UGameSetting::execGetDynamicDetails },
		{ "GetTags", &UGameSetting::execGetTags },
		{ "GetWarningRichText", &UGameSetting::execGetWarningRichText },
	};
	FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UGameSetting);
UClass* Z_Construct_UClass_UGameSetting_NoRegister()
{
	return UGameSetting::StaticClass();
}
struct Z_Construct_UClass_UGameSetting_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * \n */" },
#endif
		{ "IncludePath", "GameSetting.h" },
		{ "ModuleRelativePath", "Public/GameSetting.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LocalPlayer_MetaData[] = {
		{ "ModuleRelativePath", "Public/GameSetting.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_SettingParent_MetaData[] = {
		{ "ModuleRelativePath", "Public/GameSetting.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_OwningRegistry_MetaData[] = {
		{ "ModuleRelativePath", "Public/GameSetting.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_LocalPlayer;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_SettingParent;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_OwningRegistry;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UGameSetting_GetDescriptionRichText, "GetDescriptionRichText" }, // 1710720675
		{ &Z_Construct_UFunction_UGameSetting_GetDevName, "GetDevName" }, // 542514903
		{ &Z_Construct_UFunction_UGameSetting_GetDisplayName, "GetDisplayName" }, // 242604848
		{ &Z_Construct_UFunction_UGameSetting_GetDisplayNameVisibility, "GetDisplayNameVisibility" }, // 2298409419
		{ &Z_Construct_UFunction_UGameSetting_GetDynamicDetails, "GetDynamicDetails" }, // 407856293
		{ &Z_Construct_UFunction_UGameSetting_GetTags, "GetTags" }, // 4154832269
		{ &Z_Construct_UFunction_UGameSetting_GetWarningRichText, "GetWarningRichText" }, // 824359185
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UGameSetting>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UGameSetting_Statics::NewProp_LocalPlayer = { "LocalPlayer", nullptr, (EPropertyFlags)0x0124080000002000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UGameSetting, LocalPlayer), Z_Construct_UClass_ULocalPlayer_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LocalPlayer_MetaData), NewProp_LocalPlayer_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UGameSetting_Statics::NewProp_SettingParent = { "SettingParent", nullptr, (EPropertyFlags)0x0124080000002000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UGameSetting, SettingParent), Z_Construct_UClass_UGameSetting_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_SettingParent_MetaData), NewProp_SettingParent_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UGameSetting_Statics::NewProp_OwningRegistry = { "OwningRegistry", nullptr, (EPropertyFlags)0x0124080000002000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UGameSetting, OwningRegistry), Z_Construct_UClass_UGameSettingRegistry_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_OwningRegistry_MetaData), NewProp_OwningRegistry_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UGameSetting_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGameSetting_Statics::NewProp_LocalPlayer,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGameSetting_Statics::NewProp_SettingParent,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGameSetting_Statics::NewProp_OwningRegistry,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSetting_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UGameSetting_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UObject,
	(UObject* (*)())Z_Construct_UPackage__Script_GameSettings,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSetting_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UGameSetting_Statics::ClassParams = {
	&UGameSetting::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	Z_Construct_UClass_UGameSetting_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	UE_ARRAY_COUNT(Z_Construct_UClass_UGameSetting_Statics::PropPointers),
	0,
	0x001000A1u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSetting_Statics::Class_MetaDataParams), Z_Construct_UClass_UGameSetting_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UGameSetting()
{
	if (!Z_Registration_Info_UClass_UGameSetting.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UGameSetting.OuterSingleton, Z_Construct_UClass_UGameSetting_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UGameSetting.OuterSingleton;
}
template<> GAMESETTINGS_API UClass* StaticClass<UGameSetting>()
{
	return UGameSetting::StaticClass();
}
DEFINE_VTABLE_PTR_HELPER_CTOR(UGameSetting);
UGameSetting::~UGameSetting() {}
// End Class UGameSetting

// Begin Registration
struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSetting_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UGameSetting, UGameSetting::StaticClass, TEXT("UGameSetting"), &Z_Registration_Info_UClass_UGameSetting, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UGameSetting), 2281187960U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSetting_h_2391022911(TEXT("/Script/GameSettings"),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSetting_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSetting_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
