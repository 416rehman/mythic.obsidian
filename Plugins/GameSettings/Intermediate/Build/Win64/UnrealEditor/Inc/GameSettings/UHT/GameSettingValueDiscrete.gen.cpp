// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/GameSettingValueDiscrete.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGameSettingValueDiscrete() {}

// Begin Cross Module References
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingValue();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingValueDiscrete();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingValueDiscrete_NoRegister();
UPackage* Z_Construct_UPackage__Script_GameSettings();
// End Cross Module References

// Begin Class UGameSettingValueDiscrete Function GetDiscreteOptionDefaultIndex
struct Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionDefaultIndex_Statics
{
	struct GameSettingValueDiscrete_eventGetDiscreteOptionDefaultIndex_Parms
	{
		int32 ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Optional */" },
#endif
		{ "ModuleRelativePath", "Public/GameSettingValueDiscrete.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Optional" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FIntPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionDefaultIndex_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(GameSettingValueDiscrete_eventGetDiscreteOptionDefaultIndex_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionDefaultIndex_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionDefaultIndex_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionDefaultIndex_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionDefaultIndex_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UGameSettingValueDiscrete, nullptr, "GetDiscreteOptionDefaultIndex", nullptr, nullptr, Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionDefaultIndex_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionDefaultIndex_Statics::PropPointers), sizeof(Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionDefaultIndex_Statics::GameSettingValueDiscrete_eventGetDiscreteOptionDefaultIndex_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020400, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionDefaultIndex_Statics::Function_MetaDataParams), Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionDefaultIndex_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionDefaultIndex_Statics::GameSettingValueDiscrete_eventGetDiscreteOptionDefaultIndex_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionDefaultIndex()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionDefaultIndex_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UGameSettingValueDiscrete::execGetDiscreteOptionDefaultIndex)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(int32*)Z_Param__Result=P_THIS->GetDiscreteOptionDefaultIndex();
	P_NATIVE_END;
}
// End Class UGameSettingValueDiscrete Function GetDiscreteOptionDefaultIndex

// Begin Class UGameSettingValueDiscrete Function GetDiscreteOptionIndex
struct Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionIndex_Statics
{
	struct GameSettingValueDiscrete_eventGetDiscreteOptionIndex_Parms
	{
		int32 ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/GameSettingValueDiscrete.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FIntPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionIndex_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(GameSettingValueDiscrete_eventGetDiscreteOptionIndex_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionIndex_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionIndex_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionIndex_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionIndex_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UGameSettingValueDiscrete, nullptr, "GetDiscreteOptionIndex", nullptr, nullptr, Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionIndex_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionIndex_Statics::PropPointers), sizeof(Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionIndex_Statics::GameSettingValueDiscrete_eventGetDiscreteOptionIndex_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020400, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionIndex_Statics::Function_MetaDataParams), Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionIndex_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionIndex_Statics::GameSettingValueDiscrete_eventGetDiscreteOptionIndex_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionIndex()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionIndex_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UGameSettingValueDiscrete::execGetDiscreteOptionIndex)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(int32*)Z_Param__Result=P_THIS->GetDiscreteOptionIndex();
	P_NATIVE_END;
}
// End Class UGameSettingValueDiscrete Function GetDiscreteOptionIndex

// Begin Class UGameSettingValueDiscrete Function GetDiscreteOptions
struct Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptions_Statics
{
	struct GameSettingValueDiscrete_eventGetDiscreteOptions_Parms
	{
		TArray<FText> ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/GameSettingValueDiscrete.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FTextPropertyParams NewProp_ReturnValue_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FTextPropertyParams Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptions_Statics::NewProp_ReturnValue_Inner = { "ReturnValue", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptions_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(GameSettingValueDiscrete_eventGetDiscreteOptions_Parms, ReturnValue), EArrayPropertyFlags::None, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptions_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptions_Statics::NewProp_ReturnValue_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptions_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptions_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptions_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UGameSettingValueDiscrete, nullptr, "GetDiscreteOptions", nullptr, nullptr, Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptions_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptions_Statics::PropPointers), sizeof(Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptions_Statics::GameSettingValueDiscrete_eventGetDiscreteOptions_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020400, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptions_Statics::Function_MetaDataParams), Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptions_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptions_Statics::GameSettingValueDiscrete_eventGetDiscreteOptions_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptions()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptions_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UGameSettingValueDiscrete::execGetDiscreteOptions)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(TArray<FText>*)Z_Param__Result=P_THIS->GetDiscreteOptions();
	P_NATIVE_END;
}
// End Class UGameSettingValueDiscrete Function GetDiscreteOptions

// Begin Class UGameSettingValueDiscrete
void UGameSettingValueDiscrete::StaticRegisterNativesUGameSettingValueDiscrete()
{
	UClass* Class = UGameSettingValueDiscrete::StaticClass();
	static const FNameNativePtrPair Funcs[] = {
		{ "GetDiscreteOptionDefaultIndex", &UGameSettingValueDiscrete::execGetDiscreteOptionDefaultIndex },
		{ "GetDiscreteOptionIndex", &UGameSettingValueDiscrete::execGetDiscreteOptionIndex },
		{ "GetDiscreteOptions", &UGameSettingValueDiscrete::execGetDiscreteOptions },
	};
	FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UGameSettingValueDiscrete);
UClass* Z_Construct_UClass_UGameSettingValueDiscrete_NoRegister()
{
	return UGameSettingValueDiscrete::StaticClass();
}
struct Z_Construct_UClass_UGameSettingValueDiscrete_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "IncludePath", "GameSettingValueDiscrete.h" },
		{ "ModuleRelativePath", "Public/GameSettingValueDiscrete.h" },
	};
#endif // WITH_METADATA
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionDefaultIndex, "GetDiscreteOptionDefaultIndex" }, // 1450543490
		{ &Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptionIndex, "GetDiscreteOptionIndex" }, // 2225194349
		{ &Z_Construct_UFunction_UGameSettingValueDiscrete_GetDiscreteOptions, "GetDiscreteOptions" }, // 861247989
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UGameSettingValueDiscrete>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
UObject* (*const Z_Construct_UClass_UGameSettingValueDiscrete_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UGameSettingValue,
	(UObject* (*)())Z_Construct_UPackage__Script_GameSettings,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingValueDiscrete_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UGameSettingValueDiscrete_Statics::ClassParams = {
	&UGameSettingValueDiscrete::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	nullptr,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	0,
	0,
	0x001000A1u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingValueDiscrete_Statics::Class_MetaDataParams), Z_Construct_UClass_UGameSettingValueDiscrete_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UGameSettingValueDiscrete()
{
	if (!Z_Registration_Info_UClass_UGameSettingValueDiscrete.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UGameSettingValueDiscrete.OuterSingleton, Z_Construct_UClass_UGameSettingValueDiscrete_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UGameSettingValueDiscrete.OuterSingleton;
}
template<> GAMESETTINGS_API UClass* StaticClass<UGameSettingValueDiscrete>()
{
	return UGameSettingValueDiscrete::StaticClass();
}
DEFINE_VTABLE_PTR_HELPER_CTOR(UGameSettingValueDiscrete);
UGameSettingValueDiscrete::~UGameSettingValueDiscrete() {}
// End Class UGameSettingValueDiscrete

// Begin Registration
struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueDiscrete_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UGameSettingValueDiscrete, UGameSettingValueDiscrete::StaticClass, TEXT("UGameSettingValueDiscrete"), &Z_Registration_Info_UClass_UGameSettingValueDiscrete, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UGameSettingValueDiscrete), 535713849U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueDiscrete_h_1873134891(TEXT("/Script/GameSettings"),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueDiscrete_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueDiscrete_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
