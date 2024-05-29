// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/Widgets/IGameSettingActionInterface.h"
#include "Runtime/GameplayTags/Classes/GameplayTagContainer.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeIGameSettingActionInterface() {}

// Begin Cross Module References
COREUOBJECT_API UClass* Z_Construct_UClass_UInterface();
GAMEPLAYTAGS_API UScriptStruct* Z_Construct_UScriptStruct_FGameplayTag();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSetting_NoRegister();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingActionInterface();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingActionInterface_NoRegister();
UPackage* Z_Construct_UPackage__Script_GameSettings();
// End Cross Module References

// Begin Interface UGameSettingActionInterface Function ExecuteActionForSetting
struct GameSettingActionInterface_eventExecuteActionForSetting_Parms
{
	FGameplayTag ActionTag;
	UGameSetting* InSetting;
	bool ReturnValue;

	/** Constructor, initializes return property only **/
	GameSettingActionInterface_eventExecuteActionForSetting_Parms()
		: ReturnValue(false)
	{
	}
};
bool IGameSettingActionInterface::ExecuteActionForSetting(FGameplayTag ActionTag, UGameSetting* InSetting)
{
	check(0 && "Do not directly call Event functions in Interfaces. Call Execute_ExecuteActionForSetting instead.");
	GameSettingActionInterface_eventExecuteActionForSetting_Parms Parms;
	return Parms.ReturnValue;
}
static FName NAME_UGameSettingActionInterface_ExecuteActionForSetting = FName(TEXT("ExecuteActionForSetting"));
bool IGameSettingActionInterface::Execute_ExecuteActionForSetting(UObject* O, FGameplayTag ActionTag, UGameSetting* InSetting)
{
	check(O != NULL);
	check(O->GetClass()->ImplementsInterface(UGameSettingActionInterface::StaticClass()));
	GameSettingActionInterface_eventExecuteActionForSetting_Parms Parms;
	UFunction* const Func = O->FindFunction(NAME_UGameSettingActionInterface_ExecuteActionForSetting);
	if (Func)
	{
		Parms.ActionTag=ActionTag;
		Parms.InSetting=InSetting;
		O->ProcessEvent(Func, &Parms);
	}
	else if (auto I = (IGameSettingActionInterface*)(O->GetNativeInterfaceAddress(UGameSettingActionInterface::StaticClass())))
	{
		Parms.ReturnValue = I->ExecuteActionForSetting_Implementation(ActionTag,InSetting);
	}
	return Parms.ReturnValue;
}
struct Z_Construct_UFunction_UGameSettingActionInterface_ExecuteActionForSetting_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** */" },
#endif
		{ "ModuleRelativePath", "Public/Widgets/IGameSettingActionInterface.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStructPropertyParams NewProp_ActionTag;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_InSetting;
	static void NewProp_ReturnValue_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_UGameSettingActionInterface_ExecuteActionForSetting_Statics::NewProp_ActionTag = { "ActionTag", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(GameSettingActionInterface_eventExecuteActionForSetting_Parms, ActionTag), Z_Construct_UScriptStruct_FGameplayTag, METADATA_PARAMS(0, nullptr) }; // 1298103297
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UGameSettingActionInterface_ExecuteActionForSetting_Statics::NewProp_InSetting = { "InSetting", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(GameSettingActionInterface_eventExecuteActionForSetting_Parms, InSetting), Z_Construct_UClass_UGameSetting_NoRegister, METADATA_PARAMS(0, nullptr) };
void Z_Construct_UFunction_UGameSettingActionInterface_ExecuteActionForSetting_Statics::NewProp_ReturnValue_SetBit(void* Obj)
{
	((GameSettingActionInterface_eventExecuteActionForSetting_Parms*)Obj)->ReturnValue = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UGameSettingActionInterface_ExecuteActionForSetting_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(GameSettingActionInterface_eventExecuteActionForSetting_Parms), &Z_Construct_UFunction_UGameSettingActionInterface_ExecuteActionForSetting_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UGameSettingActionInterface_ExecuteActionForSetting_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UGameSettingActionInterface_ExecuteActionForSetting_Statics::NewProp_ActionTag,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UGameSettingActionInterface_ExecuteActionForSetting_Statics::NewProp_InSetting,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UGameSettingActionInterface_ExecuteActionForSetting_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSettingActionInterface_ExecuteActionForSetting_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UGameSettingActionInterface_ExecuteActionForSetting_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UGameSettingActionInterface, nullptr, "ExecuteActionForSetting", nullptr, nullptr, Z_Construct_UFunction_UGameSettingActionInterface_ExecuteActionForSetting_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSettingActionInterface_ExecuteActionForSetting_Statics::PropPointers), sizeof(GameSettingActionInterface_eventExecuteActionForSetting_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x0C020C00, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSettingActionInterface_ExecuteActionForSetting_Statics::Function_MetaDataParams), Z_Construct_UFunction_UGameSettingActionInterface_ExecuteActionForSetting_Statics::Function_MetaDataParams) };
static_assert(sizeof(GameSettingActionInterface_eventExecuteActionForSetting_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UGameSettingActionInterface_ExecuteActionForSetting()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UGameSettingActionInterface_ExecuteActionForSetting_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(IGameSettingActionInterface::execExecuteActionForSetting)
{
	P_GET_STRUCT(FGameplayTag,Z_Param_ActionTag);
	P_GET_OBJECT(UGameSetting,Z_Param_InSetting);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)Z_Param__Result=P_THIS->ExecuteActionForSetting_Implementation(Z_Param_ActionTag,Z_Param_InSetting);
	P_NATIVE_END;
}
// End Interface UGameSettingActionInterface Function ExecuteActionForSetting

// Begin Interface UGameSettingActionInterface
void UGameSettingActionInterface::StaticRegisterNativesUGameSettingActionInterface()
{
	UClass* Class = UGameSettingActionInterface::StaticClass();
	static const FNameNativePtrPair Funcs[] = {
		{ "ExecuteActionForSetting", &IGameSettingActionInterface::execExecuteActionForSetting },
	};
	FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UGameSettingActionInterface);
UClass* Z_Construct_UClass_UGameSettingActionInterface_NoRegister()
{
	return UGameSettingActionInterface::StaticClass();
}
struct Z_Construct_UClass_UGameSettingActionInterface_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintType", "" },
		{ "ModuleRelativePath", "Public/Widgets/IGameSettingActionInterface.h" },
	};
#endif // WITH_METADATA
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UGameSettingActionInterface_ExecuteActionForSetting, "ExecuteActionForSetting" }, // 2867925820
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<IGameSettingActionInterface>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
UObject* (*const Z_Construct_UClass_UGameSettingActionInterface_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UInterface,
	(UObject* (*)())Z_Construct_UPackage__Script_GameSettings,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingActionInterface_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UGameSettingActionInterface_Statics::ClassParams = {
	&UGameSettingActionInterface::StaticClass,
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
	0x000840A1u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingActionInterface_Statics::Class_MetaDataParams), Z_Construct_UClass_UGameSettingActionInterface_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UGameSettingActionInterface()
{
	if (!Z_Registration_Info_UClass_UGameSettingActionInterface.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UGameSettingActionInterface.OuterSingleton, Z_Construct_UClass_UGameSettingActionInterface_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UGameSettingActionInterface.OuterSingleton;
}
template<> GAMESETTINGS_API UClass* StaticClass<UGameSettingActionInterface>()
{
	return UGameSettingActionInterface::StaticClass();
}
DEFINE_VTABLE_PTR_HELPER_CTOR(UGameSettingActionInterface);
UGameSettingActionInterface::~UGameSettingActionInterface() {}
// End Interface UGameSettingActionInterface

// Begin Registration
struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_IGameSettingActionInterface_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UGameSettingActionInterface, UGameSettingActionInterface::StaticClass, TEXT("UGameSettingActionInterface"), &Z_Registration_Info_UClass_UGameSettingActionInterface, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UGameSettingActionInterface), 3882456604U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_IGameSettingActionInterface_h_2406590174(TEXT("/Script/GameSettings"),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_IGameSettingActionInterface_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_IGameSettingActionInterface_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
