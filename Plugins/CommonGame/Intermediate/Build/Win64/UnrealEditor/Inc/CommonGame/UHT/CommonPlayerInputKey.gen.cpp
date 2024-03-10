// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/CommonPlayerInputKey.h"
#include "Fonts/SlateFontInfo.h"
#include "InputCoreTypes.h"
#include "Layout/Margin.h"
#include "Styling/SlateBrush.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeCommonPlayerInputKey() {}
// Cross Module References
	COMMONGAME_API UClass* Z_Construct_UClass_UCommonPlayerInputKey();
	COMMONGAME_API UClass* Z_Construct_UClass_UCommonPlayerInputKey_NoRegister();
	COMMONGAME_API UEnum* Z_Construct_UEnum_CommonGame_ECommonKeybindForcedHoldStatus();
	COMMONGAME_API UScriptStruct* Z_Construct_UScriptStruct_FMeasuredText();
	COMMONINPUT_API UEnum* Z_Construct_UEnum_CommonInput_ECommonInputType();
	COMMONUI_API UClass* Z_Construct_UClass_UCommonUserWidget();
	COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FVector2D();
	ENGINE_API UClass* Z_Construct_UClass_UMaterialInstanceDynamic_NoRegister();
	INPUTCORE_API UScriptStruct* Z_Construct_UScriptStruct_FKey();
	SLATECORE_API UScriptStruct* Z_Construct_UScriptStruct_FMargin();
	SLATECORE_API UScriptStruct* Z_Construct_UScriptStruct_FSlateBrush();
	SLATECORE_API UScriptStruct* Z_Construct_UScriptStruct_FSlateFontInfo();
	UPackage* Z_Construct_UPackage__Script_CommonGame();
// End Cross Module References
	static FEnumRegistrationInfo Z_Registration_Info_UEnum_ECommonKeybindForcedHoldStatus;
	static UEnum* ECommonKeybindForcedHoldStatus_StaticEnum()
	{
		if (!Z_Registration_Info_UEnum_ECommonKeybindForcedHoldStatus.OuterSingleton)
		{
			Z_Registration_Info_UEnum_ECommonKeybindForcedHoldStatus.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_CommonGame_ECommonKeybindForcedHoldStatus, (UObject*)Z_Construct_UPackage__Script_CommonGame(), TEXT("ECommonKeybindForcedHoldStatus"));
		}
		return Z_Registration_Info_UEnum_ECommonKeybindForcedHoldStatus.OuterSingleton;
	}
	template<> COMMONGAME_API UEnum* StaticEnum<ECommonKeybindForcedHoldStatus>()
	{
		return ECommonKeybindForcedHoldStatus_StaticEnum();
	}
	struct Z_Construct_UEnum_CommonGame_ECommonKeybindForcedHoldStatus_Statics
	{
		static const UECodeGen_Private::FEnumeratorParam Enumerators[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[];
#endif
		static const UECodeGen_Private::FEnumParams EnumParams;
	};
	const UECodeGen_Private::FEnumeratorParam Z_Construct_UEnum_CommonGame_ECommonKeybindForcedHoldStatus_Statics::Enumerators[] = {
		{ "ECommonKeybindForcedHoldStatus::NoForcedHold", (int64)ECommonKeybindForcedHoldStatus::NoForcedHold },
		{ "ECommonKeybindForcedHoldStatus::ForcedHold", (int64)ECommonKeybindForcedHoldStatus::ForcedHold },
		{ "ECommonKeybindForcedHoldStatus::NeverShowHold", (int64)ECommonKeybindForcedHoldStatus::NeverShowHold },
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UEnum_CommonGame_ECommonKeybindForcedHoldStatus_Statics::Enum_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ForcedHold.Name", "ECommonKeybindForcedHoldStatus::ForcedHold" },
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
		{ "NeverShowHold.Name", "ECommonKeybindForcedHoldStatus::NeverShowHold" },
		{ "NoForcedHold.Name", "ECommonKeybindForcedHoldStatus::NoForcedHold" },
	};
#endif
	const UECodeGen_Private::FEnumParams Z_Construct_UEnum_CommonGame_ECommonKeybindForcedHoldStatus_Statics::EnumParams = {
		(UObject*(*)())Z_Construct_UPackage__Script_CommonGame,
		nullptr,
		"ECommonKeybindForcedHoldStatus",
		"ECommonKeybindForcedHoldStatus",
		Z_Construct_UEnum_CommonGame_ECommonKeybindForcedHoldStatus_Statics::Enumerators,
		RF_Public|RF_Transient|RF_MarkAsNative,
		UE_ARRAY_COUNT(Z_Construct_UEnum_CommonGame_ECommonKeybindForcedHoldStatus_Statics::Enumerators),
		EEnumFlags::None,
		(uint8)UEnum::ECppForm::EnumClass,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UEnum_CommonGame_ECommonKeybindForcedHoldStatus_Statics::Enum_MetaDataParams), Z_Construct_UEnum_CommonGame_ECommonKeybindForcedHoldStatus_Statics::Enum_MetaDataParams)
	};
	UEnum* Z_Construct_UEnum_CommonGame_ECommonKeybindForcedHoldStatus()
	{
		if (!Z_Registration_Info_UEnum_ECommonKeybindForcedHoldStatus.InnerSingleton)
		{
			UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_ECommonKeybindForcedHoldStatus.InnerSingleton, Z_Construct_UEnum_CommonGame_ECommonKeybindForcedHoldStatus_Statics::EnumParams);
		}
		return Z_Registration_Info_UEnum_ECommonKeybindForcedHoldStatus.InnerSingleton;
	}
	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_MeasuredText;
class UScriptStruct* FMeasuredText::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_MeasuredText.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_MeasuredText.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FMeasuredText, (UObject*)Z_Construct_UPackage__Script_CommonGame(), TEXT("MeasuredText"));
	}
	return Z_Registration_Info_UScriptStruct_MeasuredText.OuterSingleton;
}
template<> COMMONGAME_API UScriptStruct* StaticStruct<FMeasuredText>()
{
	return FMeasuredText::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FMeasuredText_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FMeasuredText_Statics::Struct_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
	};
#endif
	void* Z_Construct_UScriptStruct_FMeasuredText_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FMeasuredText>();
	}
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FMeasuredText_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_CommonGame,
		nullptr,
		&NewStructOps,
		"MeasuredText",
		nullptr,
		0,
		sizeof(FMeasuredText),
		alignof(FMeasuredText),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FMeasuredText_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FMeasuredText_Statics::Struct_MetaDataParams)
	};
	UScriptStruct* Z_Construct_UScriptStruct_FMeasuredText()
	{
		if (!Z_Registration_Info_UScriptStruct_MeasuredText.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_MeasuredText.InnerSingleton, Z_Construct_UScriptStruct_FMeasuredText_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_MeasuredText.InnerSingleton;
	}
	DEFINE_FUNCTION(UCommonPlayerInputKey::execIsBoundKeyValid)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		*(bool*)Z_Param__Result=P_THIS->IsBoundKeyValid();
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonPlayerInputKey::execIsHoldKeybind)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		*(bool*)Z_Param__Result=P_THIS->IsHoldKeybind();
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonPlayerInputKey::execStopHoldProgress)
	{
		P_GET_PROPERTY(FNameProperty,Z_Param_HoldActionName);
		P_GET_UBOOL(Z_Param_bCompletedSuccessfully);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->StopHoldProgress(Z_Param_HoldActionName,Z_Param_bCompletedSuccessfully);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonPlayerInputKey::execStartHoldProgress)
	{
		P_GET_PROPERTY(FNameProperty,Z_Param_HoldActionName);
		P_GET_PROPERTY(FFloatProperty,Z_Param_HoldDuration);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->StartHoldProgress(Z_Param_HoldActionName,Z_Param_HoldDuration);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonPlayerInputKey::execSetPresetNameOverride)
	{
		P_GET_PROPERTY(FNameProperty,Z_Param_NewValue);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->SetPresetNameOverride(Z_Param_NewValue);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonPlayerInputKey::execSetAxisScale)
	{
		P_GET_PROPERTY(FFloatProperty,Z_Param_NewValue);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->SetAxisScale(Z_Param_NewValue);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonPlayerInputKey::execSetShowProgressCountDown)
	{
		P_GET_UBOOL(Z_Param_bShow);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->SetShowProgressCountDown(Z_Param_bShow);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonPlayerInputKey::execSetForcedHoldKeybindStatus)
	{
		P_GET_ENUM(ECommonKeybindForcedHoldStatus,Z_Param_InForcedHoldKeybindStatus);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->SetForcedHoldKeybindStatus(ECommonKeybindForcedHoldStatus(Z_Param_InForcedHoldKeybindStatus));
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonPlayerInputKey::execSetForcedHoldKeybind)
	{
		P_GET_UBOOL(Z_Param_InForcedHoldKeybind);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->SetForcedHoldKeybind(Z_Param_InForcedHoldKeybind);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonPlayerInputKey::execSetBoundAction)
	{
		P_GET_PROPERTY(FNameProperty,Z_Param_NewBoundAction);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->SetBoundAction(Z_Param_NewBoundAction);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonPlayerInputKey::execSetBoundKey)
	{
		P_GET_STRUCT(FKey,Z_Param_NewBoundAction);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->SetBoundKey(Z_Param_NewBoundAction);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonPlayerInputKey::execUpdateKeybindWidget)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->UpdateKeybindWidget();
		P_NATIVE_END;
	}
	void UCommonPlayerInputKey::StaticRegisterNativesUCommonPlayerInputKey()
	{
		UClass* Class = UCommonPlayerInputKey::StaticClass();
		static const FNameNativePtrPair Funcs[] = {
			{ "IsBoundKeyValid", &UCommonPlayerInputKey::execIsBoundKeyValid },
			{ "IsHoldKeybind", &UCommonPlayerInputKey::execIsHoldKeybind },
			{ "SetAxisScale", &UCommonPlayerInputKey::execSetAxisScale },
			{ "SetBoundAction", &UCommonPlayerInputKey::execSetBoundAction },
			{ "SetBoundKey", &UCommonPlayerInputKey::execSetBoundKey },
			{ "SetForcedHoldKeybind", &UCommonPlayerInputKey::execSetForcedHoldKeybind },
			{ "SetForcedHoldKeybindStatus", &UCommonPlayerInputKey::execSetForcedHoldKeybindStatus },
			{ "SetPresetNameOverride", &UCommonPlayerInputKey::execSetPresetNameOverride },
			{ "SetShowProgressCountDown", &UCommonPlayerInputKey::execSetShowProgressCountDown },
			{ "StartHoldProgress", &UCommonPlayerInputKey::execStartHoldProgress },
			{ "StopHoldProgress", &UCommonPlayerInputKey::execStopHoldProgress },
			{ "UpdateKeybindWidget", &UCommonPlayerInputKey::execUpdateKeybindWidget },
		};
		FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
	}
	struct Z_Construct_UFunction_UCommonPlayerInputKey_IsBoundKeyValid_Statics
	{
		struct CommonPlayerInputKey_eventIsBoundKeyValid_Parms
		{
			bool ReturnValue;
		};
		static void NewProp_ReturnValue_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	void Z_Construct_UFunction_UCommonPlayerInputKey_IsBoundKeyValid_Statics::NewProp_ReturnValue_SetBit(void* Obj)
	{
		((CommonPlayerInputKey_eventIsBoundKeyValid_Parms*)Obj)->ReturnValue = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UCommonPlayerInputKey_IsBoundKeyValid_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(CommonPlayerInputKey_eventIsBoundKeyValid_Parms), &Z_Construct_UFunction_UCommonPlayerInputKey_IsBoundKeyValid_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonPlayerInputKey_IsBoundKeyValid_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonPlayerInputKey_IsBoundKeyValid_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonPlayerInputKey_IsBoundKeyValid_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonPlayerInputKey_IsBoundKeyValid_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonPlayerInputKey, nullptr, "IsBoundKeyValid", nullptr, nullptr, Z_Construct_UFunction_UCommonPlayerInputKey_IsBoundKeyValid_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_IsBoundKeyValid_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonPlayerInputKey_IsBoundKeyValid_Statics::CommonPlayerInputKey_eventIsBoundKeyValid_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x40020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_IsBoundKeyValid_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonPlayerInputKey_IsBoundKeyValid_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_IsBoundKeyValid_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonPlayerInputKey_IsBoundKeyValid_Statics::CommonPlayerInputKey_eventIsBoundKeyValid_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonPlayerInputKey_IsBoundKeyValid()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonPlayerInputKey_IsBoundKeyValid_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonPlayerInputKey_IsHoldKeybind_Statics
	{
		struct CommonPlayerInputKey_eventIsHoldKeybind_Parms
		{
			bool ReturnValue;
		};
		static void NewProp_ReturnValue_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	void Z_Construct_UFunction_UCommonPlayerInputKey_IsHoldKeybind_Statics::NewProp_ReturnValue_SetBit(void* Obj)
	{
		((CommonPlayerInputKey_eventIsHoldKeybind_Parms*)Obj)->ReturnValue = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UCommonPlayerInputKey_IsHoldKeybind_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(CommonPlayerInputKey_eventIsHoldKeybind_Parms), &Z_Construct_UFunction_UCommonPlayerInputKey_IsHoldKeybind_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonPlayerInputKey_IsHoldKeybind_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonPlayerInputKey_IsHoldKeybind_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonPlayerInputKey_IsHoldKeybind_Statics::Function_MetaDataParams[] = {
		{ "Category", "Keybind Widget" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Get whether this keybind is a hold action. */" },
#endif
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Get whether this keybind is a hold action." },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonPlayerInputKey_IsHoldKeybind_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonPlayerInputKey, nullptr, "IsHoldKeybind", nullptr, nullptr, Z_Construct_UFunction_UCommonPlayerInputKey_IsHoldKeybind_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_IsHoldKeybind_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonPlayerInputKey_IsHoldKeybind_Statics::CommonPlayerInputKey_eventIsHoldKeybind_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_IsHoldKeybind_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonPlayerInputKey_IsHoldKeybind_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_IsHoldKeybind_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonPlayerInputKey_IsHoldKeybind_Statics::CommonPlayerInputKey_eventIsHoldKeybind_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonPlayerInputKey_IsHoldKeybind()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonPlayerInputKey_IsHoldKeybind_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonPlayerInputKey_SetAxisScale_Statics
	{
		struct CommonPlayerInputKey_eventSetAxisScale_Parms
		{
			float NewValue;
		};
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_NewValue_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_NewValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonPlayerInputKey_SetAxisScale_Statics::NewProp_NewValue_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UFunction_UCommonPlayerInputKey_SetAxisScale_Statics::NewProp_NewValue = { "NewValue", nullptr, (EPropertyFlags)0x0010000000000082, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonPlayerInputKey_eventSetAxisScale_Parms, NewValue), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_SetAxisScale_Statics::NewProp_NewValue_MetaData), Z_Construct_UFunction_UCommonPlayerInputKey_SetAxisScale_Statics::NewProp_NewValue_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonPlayerInputKey_SetAxisScale_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonPlayerInputKey_SetAxisScale_Statics::NewProp_NewValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonPlayerInputKey_SetAxisScale_Statics::Function_MetaDataParams[] = {
		{ "Category", "Keybind Widget" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Set the axis scale value for this keybind */" },
#endif
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Set the axis scale value for this keybind" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonPlayerInputKey_SetAxisScale_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonPlayerInputKey, nullptr, "SetAxisScale", nullptr, nullptr, Z_Construct_UFunction_UCommonPlayerInputKey_SetAxisScale_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_SetAxisScale_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonPlayerInputKey_SetAxisScale_Statics::CommonPlayerInputKey_eventSetAxisScale_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_SetAxisScale_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonPlayerInputKey_SetAxisScale_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_SetAxisScale_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonPlayerInputKey_SetAxisScale_Statics::CommonPlayerInputKey_eventSetAxisScale_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonPlayerInputKey_SetAxisScale()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonPlayerInputKey_SetAxisScale_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundAction_Statics
	{
		struct CommonPlayerInputKey_eventSetBoundAction_Parms
		{
			FName NewBoundAction;
		};
		static const UECodeGen_Private::FNamePropertyParams NewProp_NewBoundAction;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FNamePropertyParams Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundAction_Statics::NewProp_NewBoundAction = { "NewBoundAction", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonPlayerInputKey_eventSetBoundAction_Parms, NewBoundAction), METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundAction_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundAction_Statics::NewProp_NewBoundAction,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundAction_Statics::Function_MetaDataParams[] = {
		{ "Category", "Keybind Widget" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Set the bound action for our keybind */" },
#endif
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Set the bound action for our keybind" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundAction_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonPlayerInputKey, nullptr, "SetBoundAction", nullptr, nullptr, Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundAction_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundAction_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundAction_Statics::CommonPlayerInputKey_eventSetBoundAction_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundAction_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundAction_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundAction_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundAction_Statics::CommonPlayerInputKey_eventSetBoundAction_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundAction()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundAction_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundKey_Statics
	{
		struct CommonPlayerInputKey_eventSetBoundKey_Parms
		{
			FKey NewBoundAction;
		};
		static const UECodeGen_Private::FStructPropertyParams NewProp_NewBoundAction;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundKey_Statics::NewProp_NewBoundAction = { "NewBoundAction", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonPlayerInputKey_eventSetBoundKey_Parms, NewBoundAction), Z_Construct_UScriptStruct_FKey, METADATA_PARAMS(0, nullptr) }; // 46000949
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundKey_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundKey_Statics::NewProp_NewBoundAction,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundKey_Statics::Function_MetaDataParams[] = {
		{ "Category", "Keybind Widget" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Set the bound key for our keybind */" },
#endif
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Set the bound key for our keybind" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundKey_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonPlayerInputKey, nullptr, "SetBoundKey", nullptr, nullptr, Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundKey_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundKey_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundKey_Statics::CommonPlayerInputKey_eventSetBoundKey_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundKey_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundKey_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundKey_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundKey_Statics::CommonPlayerInputKey_eventSetBoundKey_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundKey()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundKey_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybind_Statics
	{
		struct CommonPlayerInputKey_eventSetForcedHoldKeybind_Parms
		{
			bool InForcedHoldKeybind;
		};
		static void NewProp_InForcedHoldKeybind_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_InForcedHoldKeybind;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	void Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybind_Statics::NewProp_InForcedHoldKeybind_SetBit(void* Obj)
	{
		((CommonPlayerInputKey_eventSetForcedHoldKeybind_Parms*)Obj)->InForcedHoldKeybind = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybind_Statics::NewProp_InForcedHoldKeybind = { "InForcedHoldKeybind", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(CommonPlayerInputKey_eventSetForcedHoldKeybind_Parms), &Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybind_Statics::NewProp_InForcedHoldKeybind_SetBit, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybind_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybind_Statics::NewProp_InForcedHoldKeybind,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybind_Statics::Function_MetaDataParams[] = {
		{ "Category", "Keybind Widget" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Force this keybind to be a hold keybind */" },
#endif
		{ "DeprecatedFunction", "" },
		{ "DeprecationMessage", "Use SetForcedHoldKeybindStatus instead" },
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Force this keybind to be a hold keybind" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybind_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonPlayerInputKey, nullptr, "SetForcedHoldKeybind", nullptr, nullptr, Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybind_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybind_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybind_Statics::CommonPlayerInputKey_eventSetForcedHoldKeybind_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybind_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybind_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybind_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybind_Statics::CommonPlayerInputKey_eventSetForcedHoldKeybind_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybind()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybind_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybindStatus_Statics
	{
		struct CommonPlayerInputKey_eventSetForcedHoldKeybindStatus_Parms
		{
			ECommonKeybindForcedHoldStatus InForcedHoldKeybindStatus;
		};
		static const UECodeGen_Private::FBytePropertyParams NewProp_InForcedHoldKeybindStatus_Underlying;
		static const UECodeGen_Private::FEnumPropertyParams NewProp_InForcedHoldKeybindStatus;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybindStatus_Statics::NewProp_InForcedHoldKeybindStatus_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybindStatus_Statics::NewProp_InForcedHoldKeybindStatus = { "InForcedHoldKeybindStatus", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonPlayerInputKey_eventSetForcedHoldKeybindStatus_Parms, InForcedHoldKeybindStatus), Z_Construct_UEnum_CommonGame_ECommonKeybindForcedHoldStatus, METADATA_PARAMS(0, nullptr) }; // 2269447710
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybindStatus_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybindStatus_Statics::NewProp_InForcedHoldKeybindStatus_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybindStatus_Statics::NewProp_InForcedHoldKeybindStatus,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybindStatus_Statics::Function_MetaDataParams[] = {
		{ "Category", "Keybind Widget" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Force this keybind to be a hold keybind */" },
#endif
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Force this keybind to be a hold keybind" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybindStatus_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonPlayerInputKey, nullptr, "SetForcedHoldKeybindStatus", nullptr, nullptr, Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybindStatus_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybindStatus_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybindStatus_Statics::CommonPlayerInputKey_eventSetForcedHoldKeybindStatus_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybindStatus_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybindStatus_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybindStatus_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybindStatus_Statics::CommonPlayerInputKey_eventSetForcedHoldKeybindStatus_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybindStatus()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybindStatus_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonPlayerInputKey_SetPresetNameOverride_Statics
	{
		struct CommonPlayerInputKey_eventSetPresetNameOverride_Parms
		{
			FName NewValue;
		};
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_NewValue_MetaData[];
#endif
		static const UECodeGen_Private::FNamePropertyParams NewProp_NewValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonPlayerInputKey_SetPresetNameOverride_Statics::NewProp_NewValue_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif
	const UECodeGen_Private::FNamePropertyParams Z_Construct_UFunction_UCommonPlayerInputKey_SetPresetNameOverride_Statics::NewProp_NewValue = { "NewValue", nullptr, (EPropertyFlags)0x0010000000000082, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonPlayerInputKey_eventSetPresetNameOverride_Parms, NewValue), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_SetPresetNameOverride_Statics::NewProp_NewValue_MetaData), Z_Construct_UFunction_UCommonPlayerInputKey_SetPresetNameOverride_Statics::NewProp_NewValue_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonPlayerInputKey_SetPresetNameOverride_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonPlayerInputKey_SetPresetNameOverride_Statics::NewProp_NewValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonPlayerInputKey_SetPresetNameOverride_Statics::Function_MetaDataParams[] = {
		{ "Category", "Keybind Widget" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Set the preset name override value for this keybind. */" },
#endif
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Set the preset name override value for this keybind." },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonPlayerInputKey_SetPresetNameOverride_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonPlayerInputKey, nullptr, "SetPresetNameOverride", nullptr, nullptr, Z_Construct_UFunction_UCommonPlayerInputKey_SetPresetNameOverride_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_SetPresetNameOverride_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonPlayerInputKey_SetPresetNameOverride_Statics::CommonPlayerInputKey_eventSetPresetNameOverride_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_SetPresetNameOverride_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonPlayerInputKey_SetPresetNameOverride_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_SetPresetNameOverride_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonPlayerInputKey_SetPresetNameOverride_Statics::CommonPlayerInputKey_eventSetPresetNameOverride_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonPlayerInputKey_SetPresetNameOverride()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonPlayerInputKey_SetPresetNameOverride_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonPlayerInputKey_SetShowProgressCountDown_Statics
	{
		struct CommonPlayerInputKey_eventSetShowProgressCountDown_Parms
		{
			bool bShow;
		};
		static void NewProp_bShow_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bShow;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	void Z_Construct_UFunction_UCommonPlayerInputKey_SetShowProgressCountDown_Statics::NewProp_bShow_SetBit(void* Obj)
	{
		((CommonPlayerInputKey_eventSetShowProgressCountDown_Parms*)Obj)->bShow = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UCommonPlayerInputKey_SetShowProgressCountDown_Statics::NewProp_bShow = { "bShow", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(CommonPlayerInputKey_eventSetShowProgressCountDown_Parms), &Z_Construct_UFunction_UCommonPlayerInputKey_SetShowProgressCountDown_Statics::NewProp_bShow_SetBit, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonPlayerInputKey_SetShowProgressCountDown_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonPlayerInputKey_SetShowProgressCountDown_Statics::NewProp_bShow,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonPlayerInputKey_SetShowProgressCountDown_Statics::Function_MetaDataParams[] = {
		{ "Category", "Keybind Widget" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Force this keybind to be a hold keybind */" },
#endif
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Force this keybind to be a hold keybind" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonPlayerInputKey_SetShowProgressCountDown_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonPlayerInputKey, nullptr, "SetShowProgressCountDown", nullptr, nullptr, Z_Construct_UFunction_UCommonPlayerInputKey_SetShowProgressCountDown_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_SetShowProgressCountDown_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonPlayerInputKey_SetShowProgressCountDown_Statics::CommonPlayerInputKey_eventSetShowProgressCountDown_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_SetShowProgressCountDown_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonPlayerInputKey_SetShowProgressCountDown_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_SetShowProgressCountDown_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonPlayerInputKey_SetShowProgressCountDown_Statics::CommonPlayerInputKey_eventSetShowProgressCountDown_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonPlayerInputKey_SetShowProgressCountDown()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonPlayerInputKey_SetShowProgressCountDown_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonPlayerInputKey_StartHoldProgress_Statics
	{
		struct CommonPlayerInputKey_eventStartHoldProgress_Parms
		{
			FName HoldActionName;
			float HoldDuration;
		};
		static const UECodeGen_Private::FNamePropertyParams NewProp_HoldActionName;
		static const UECodeGen_Private::FFloatPropertyParams NewProp_HoldDuration;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FNamePropertyParams Z_Construct_UFunction_UCommonPlayerInputKey_StartHoldProgress_Statics::NewProp_HoldActionName = { "HoldActionName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonPlayerInputKey_eventStartHoldProgress_Parms, HoldActionName), METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UFunction_UCommonPlayerInputKey_StartHoldProgress_Statics::NewProp_HoldDuration = { "HoldDuration", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonPlayerInputKey_eventStartHoldProgress_Parms, HoldDuration), METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonPlayerInputKey_StartHoldProgress_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonPlayerInputKey_StartHoldProgress_Statics::NewProp_HoldActionName,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonPlayerInputKey_StartHoldProgress_Statics::NewProp_HoldDuration,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonPlayerInputKey_StartHoldProgress_Statics::Function_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Called through a delegate when we start hold progress */" },
#endif
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Called through a delegate when we start hold progress" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonPlayerInputKey_StartHoldProgress_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonPlayerInputKey, nullptr, "StartHoldProgress", nullptr, nullptr, Z_Construct_UFunction_UCommonPlayerInputKey_StartHoldProgress_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_StartHoldProgress_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonPlayerInputKey_StartHoldProgress_Statics::CommonPlayerInputKey_eventStartHoldProgress_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_StartHoldProgress_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonPlayerInputKey_StartHoldProgress_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_StartHoldProgress_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonPlayerInputKey_StartHoldProgress_Statics::CommonPlayerInputKey_eventStartHoldProgress_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonPlayerInputKey_StartHoldProgress()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonPlayerInputKey_StartHoldProgress_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonPlayerInputKey_StopHoldProgress_Statics
	{
		struct CommonPlayerInputKey_eventStopHoldProgress_Parms
		{
			FName HoldActionName;
			bool bCompletedSuccessfully;
		};
		static const UECodeGen_Private::FNamePropertyParams NewProp_HoldActionName;
		static void NewProp_bCompletedSuccessfully_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bCompletedSuccessfully;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FNamePropertyParams Z_Construct_UFunction_UCommonPlayerInputKey_StopHoldProgress_Statics::NewProp_HoldActionName = { "HoldActionName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonPlayerInputKey_eventStopHoldProgress_Parms, HoldActionName), METADATA_PARAMS(0, nullptr) };
	void Z_Construct_UFunction_UCommonPlayerInputKey_StopHoldProgress_Statics::NewProp_bCompletedSuccessfully_SetBit(void* Obj)
	{
		((CommonPlayerInputKey_eventStopHoldProgress_Parms*)Obj)->bCompletedSuccessfully = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UCommonPlayerInputKey_StopHoldProgress_Statics::NewProp_bCompletedSuccessfully = { "bCompletedSuccessfully", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(CommonPlayerInputKey_eventStopHoldProgress_Parms), &Z_Construct_UFunction_UCommonPlayerInputKey_StopHoldProgress_Statics::NewProp_bCompletedSuccessfully_SetBit, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonPlayerInputKey_StopHoldProgress_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonPlayerInputKey_StopHoldProgress_Statics::NewProp_HoldActionName,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonPlayerInputKey_StopHoldProgress_Statics::NewProp_bCompletedSuccessfully,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonPlayerInputKey_StopHoldProgress_Statics::Function_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Called through a delegate when we stop hold progress */" },
#endif
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Called through a delegate when we stop hold progress" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonPlayerInputKey_StopHoldProgress_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonPlayerInputKey, nullptr, "StopHoldProgress", nullptr, nullptr, Z_Construct_UFunction_UCommonPlayerInputKey_StopHoldProgress_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_StopHoldProgress_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonPlayerInputKey_StopHoldProgress_Statics::CommonPlayerInputKey_eventStopHoldProgress_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_StopHoldProgress_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonPlayerInputKey_StopHoldProgress_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_StopHoldProgress_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonPlayerInputKey_StopHoldProgress_Statics::CommonPlayerInputKey_eventStopHoldProgress_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonPlayerInputKey_StopHoldProgress()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonPlayerInputKey_StopHoldProgress_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonPlayerInputKey_UpdateKeybindWidget_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonPlayerInputKey_UpdateKeybindWidget_Statics::Function_MetaDataParams[] = {
		{ "Category", "Keybind Widget" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Update the key and associated display based on our current Boundaction */" },
#endif
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Update the key and associated display based on our current Boundaction" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonPlayerInputKey_UpdateKeybindWidget_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonPlayerInputKey, nullptr, "UpdateKeybindWidget", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonPlayerInputKey_UpdateKeybindWidget_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonPlayerInputKey_UpdateKeybindWidget_Statics::Function_MetaDataParams) };
	UFunction* Z_Construct_UFunction_UCommonPlayerInputKey_UpdateKeybindWidget()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonPlayerInputKey_UpdateKeybindWidget_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UCommonPlayerInputKey);
	UClass* Z_Construct_UClass_UCommonPlayerInputKey_NoRegister()
	{
		return UCommonPlayerInputKey::StaticClass();
	}
	struct Z_Construct_UClass_UCommonPlayerInputKey_Statics
	{
		static UObject* (*const DependentSingletons[])();
		static const FClassFunctionLinkInfo FuncInfo[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_BoundAction_MetaData[];
#endif
		static const UECodeGen_Private::FNamePropertyParams NewProp_BoundAction;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_AxisScale_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_AxisScale;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_BoundKeyFallback_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_BoundKeyFallback;
		static const UECodeGen_Private::FBytePropertyParams NewProp_InputTypeOverride_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_InputTypeOverride_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_InputTypeOverride;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PresetNameOverride_MetaData[];
#endif
		static const UECodeGen_Private::FNamePropertyParams NewProp_PresetNameOverride;
		static const UECodeGen_Private::FBytePropertyParams NewProp_ForcedHoldKeybindStatus_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_ForcedHoldKeybindStatus_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_ForcedHoldKeybindStatus;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bIsHoldKeybind_MetaData[];
#endif
		static void NewProp_bIsHoldKeybind_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bIsHoldKeybind;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bShowKeybindBorder_MetaData[];
#endif
		static void NewProp_bShowKeybindBorder_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bShowKeybindBorder;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_FrameSize_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_FrameSize;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bShowTimeCountDown_MetaData[];
#endif
		static void NewProp_bShowTimeCountDown_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bShowTimeCountDown;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_BoundKey_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_BoundKey;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_HoldProgressBrush_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_HoldProgressBrush;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_KeyBindTextBorder_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_KeyBindTextBorder;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bShowUnboundStatus_MetaData[];
#endif
		static void NewProp_bShowUnboundStatus_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bShowUnboundStatus;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_KeyBindTextFont_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_KeyBindTextFont;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_CountdownTextFont_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_CountdownTextFont;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_CountdownText_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_CountdownText;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_KeybindText_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_KeybindText;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_KeybindTextPadding_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_KeybindTextPadding;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_KeybindFrameMinimumSize_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_KeybindFrameMinimumSize;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PercentageMaterialParameterName_MetaData[];
#endif
		static const UECodeGen_Private::FNamePropertyParams NewProp_PercentageMaterialParameterName;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_ProgressPercentageMID_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPtrPropertyParams NewProp_ProgressPercentageMID;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_CachedKeyBrush_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_CachedKeyBrush;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UCommonPlayerInputKey_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UCommonUserWidget,
		(UObject* (*)())Z_Construct_UPackage__Script_CommonGame,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::DependentSingletons) < 16);
	const FClassFunctionLinkInfo Z_Construct_UClass_UCommonPlayerInputKey_Statics::FuncInfo[] = {
		{ &Z_Construct_UFunction_UCommonPlayerInputKey_IsBoundKeyValid, "IsBoundKeyValid" }, // 492874676
		{ &Z_Construct_UFunction_UCommonPlayerInputKey_IsHoldKeybind, "IsHoldKeybind" }, // 639844306
		{ &Z_Construct_UFunction_UCommonPlayerInputKey_SetAxisScale, "SetAxisScale" }, // 3347497120
		{ &Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundAction, "SetBoundAction" }, // 1325002404
		{ &Z_Construct_UFunction_UCommonPlayerInputKey_SetBoundKey, "SetBoundKey" }, // 2507441153
		{ &Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybind, "SetForcedHoldKeybind" }, // 1825458493
		{ &Z_Construct_UFunction_UCommonPlayerInputKey_SetForcedHoldKeybindStatus, "SetForcedHoldKeybindStatus" }, // 1173634535
		{ &Z_Construct_UFunction_UCommonPlayerInputKey_SetPresetNameOverride, "SetPresetNameOverride" }, // 3919201069
		{ &Z_Construct_UFunction_UCommonPlayerInputKey_SetShowProgressCountDown, "SetShowProgressCountDown" }, // 2882391315
		{ &Z_Construct_UFunction_UCommonPlayerInputKey_StartHoldProgress, "StartHoldProgress" }, // 1960695743
		{ &Z_Construct_UFunction_UCommonPlayerInputKey_StopHoldProgress, "StopHoldProgress" }, // 3814648799
		{ &Z_Construct_UFunction_UCommonPlayerInputKey_UpdateKeybindWidget, "UpdateKeybindWidget" }, // 1574901730
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::FuncInfo) < 2048);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonPlayerInputKey_Statics::Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "DisableNativeTick", "" },
		{ "IncludePath", "CommonPlayerInputKey.h" },
		{ "IsBlueprintBase", "true" },
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
		{ "ObjectInitializerConstructorDeclared", "" },
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_BoundAction_MetaData[] = {
		{ "Category", "Keybind Widget" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Our current BoundAction */" },
#endif
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Our current BoundAction" },
#endif
	};
#endif
	const UECodeGen_Private::FNamePropertyParams Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_BoundAction = { "BoundAction", nullptr, (EPropertyFlags)0x0010000000000015, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonPlayerInputKey, BoundAction), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_BoundAction_MetaData), Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_BoundAction_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_AxisScale_MetaData[] = {
		{ "Category", "Keybind Widget" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Scale to read when using an axis Mapping */" },
#endif
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Scale to read when using an axis Mapping" },
#endif
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_AxisScale = { "AxisScale", nullptr, (EPropertyFlags)0x0010000000000015, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonPlayerInputKey, AxisScale), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_AxisScale_MetaData), Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_AxisScale_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_BoundKeyFallback_MetaData[] = {
		{ "Category", "Keybind Widget" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Key this widget is bound to set directly in blueprint. Used when we want to reference a specific key instead of an action. */" },
#endif
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Key this widget is bound to set directly in blueprint. Used when we want to reference a specific key instead of an action." },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_BoundKeyFallback = { "BoundKeyFallback", nullptr, (EPropertyFlags)0x0010000000000015, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonPlayerInputKey, BoundKeyFallback), Z_Construct_UScriptStruct_FKey, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_BoundKeyFallback_MetaData), Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_BoundKeyFallback_MetaData) }; // 46000949
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_InputTypeOverride_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_InputTypeOverride_MetaData[] = {
		{ "Category", "Keybind Widget" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Allows us to set the input type explicitly for the keybind widget. */" },
#endif
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Allows us to set the input type explicitly for the keybind widget." },
#endif
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_InputTypeOverride = { "InputTypeOverride", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonPlayerInputKey, InputTypeOverride), Z_Construct_UEnum_CommonInput_ECommonInputType, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_InputTypeOverride_MetaData), Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_InputTypeOverride_MetaData) }; // 2481650305
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_PresetNameOverride_MetaData[] = {
		{ "Category", "Keybind Widget" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Allows us to set the preset name explicitly for the keybind widget. */" },
#endif
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Allows us to set the preset name explicitly for the keybind widget." },
#endif
	};
#endif
	const UECodeGen_Private::FNamePropertyParams Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_PresetNameOverride = { "PresetNameOverride", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonPlayerInputKey, PresetNameOverride), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_PresetNameOverride_MetaData), Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_PresetNameOverride_MetaData) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_ForcedHoldKeybindStatus_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_ForcedHoldKeybindStatus_MetaData[] = {
		{ "Category", "Keybind Widget" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Setting that can show this keybind as a hold or never show it as a hold (even if it is) */" },
#endif
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Setting that can show this keybind as a hold or never show it as a hold (even if it is)" },
#endif
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_ForcedHoldKeybindStatus = { "ForcedHoldKeybindStatus", nullptr, (EPropertyFlags)0x0010000000000015, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonPlayerInputKey, ForcedHoldKeybindStatus), Z_Construct_UEnum_CommonGame_ECommonKeybindForcedHoldStatus, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_ForcedHoldKeybindStatus_MetaData), Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_ForcedHoldKeybindStatus_MetaData) }; // 2269447710
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bIsHoldKeybind_MetaData[] = {
		{ "Category", "Keybind Widget" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Whether or not this keybind widget is currently set to be a hold keybind */" },
#endif
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
		{ "ScriptName", "IsHoldKeybindValue" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Whether or not this keybind widget is currently set to be a hold keybind" },
#endif
	};
#endif
	void Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bIsHoldKeybind_SetBit(void* Obj)
	{
		((UCommonPlayerInputKey*)Obj)->bIsHoldKeybind = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bIsHoldKeybind = { "bIsHoldKeybind", nullptr, (EPropertyFlags)0x0020080000010015, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(UCommonPlayerInputKey), &Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bIsHoldKeybind_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bIsHoldKeybind_MetaData), Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bIsHoldKeybind_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bShowKeybindBorder_MetaData[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**  */" },
#endif
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
	};
#endif
	void Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bShowKeybindBorder_SetBit(void* Obj)
	{
		((UCommonPlayerInputKey*)Obj)->bShowKeybindBorder = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bShowKeybindBorder = { "bShowKeybindBorder", nullptr, (EPropertyFlags)0x0020080000002000, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(UCommonPlayerInputKey), &Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bShowKeybindBorder_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bShowKeybindBorder_MetaData), Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bShowKeybindBorder_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_FrameSize_MetaData[] = {
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_FrameSize = { "FrameSize", nullptr, (EPropertyFlags)0x0020080000002000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonPlayerInputKey, FrameSize), Z_Construct_UScriptStruct_FVector2D, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_FrameSize_MetaData), Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_FrameSize_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bShowTimeCountDown_MetaData[] = {
		{ "Category", "Keybind Widget" },
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
	};
#endif
	void Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bShowTimeCountDown_SetBit(void* Obj)
	{
		((UCommonPlayerInputKey*)Obj)->bShowTimeCountDown = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bShowTimeCountDown = { "bShowTimeCountDown", nullptr, (EPropertyFlags)0x0020080000000014, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(UCommonPlayerInputKey), &Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bShowTimeCountDown_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bShowTimeCountDown_MetaData), Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bShowTimeCountDown_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_BoundKey_MetaData[] = {
		{ "Category", "Keybind Widget" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Derived Key this widget is bound to */" },
#endif
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Derived Key this widget is bound to" },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_BoundKey = { "BoundKey", nullptr, (EPropertyFlags)0x0020080000000014, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonPlayerInputKey, BoundKey), Z_Construct_UScriptStruct_FKey, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_BoundKey_MetaData), Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_BoundKey_MetaData) }; // 46000949
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_HoldProgressBrush_MetaData[] = {
		{ "Category", "Keybind Widget" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Material for showing Progress */" },
#endif
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Material for showing Progress" },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_HoldProgressBrush = { "HoldProgressBrush", nullptr, (EPropertyFlags)0x0020080000010001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonPlayerInputKey, HoldProgressBrush), Z_Construct_UScriptStruct_FSlateBrush, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_HoldProgressBrush_MetaData), Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_HoldProgressBrush_MetaData) }; // 899956612
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_KeyBindTextBorder_MetaData[] = {
		{ "Category", "Keybind Widget" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** The key bind text border. */" },
#endif
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The key bind text border." },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_KeyBindTextBorder = { "KeyBindTextBorder", nullptr, (EPropertyFlags)0x0020080000010001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonPlayerInputKey, KeyBindTextBorder), Z_Construct_UScriptStruct_FSlateBrush, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_KeyBindTextBorder_MetaData), Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_KeyBindTextBorder_MetaData) }; // 899956612
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bShowUnboundStatus_MetaData[] = {
		{ "Category", "Keybind Widget" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Should this keybinding widget display information that it is currently unbound? */" },
#endif
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Should this keybinding widget display information that it is currently unbound?" },
#endif
	};
#endif
	void Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bShowUnboundStatus_SetBit(void* Obj)
	{
		((UCommonPlayerInputKey*)Obj)->bShowUnboundStatus = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bShowUnboundStatus = { "bShowUnboundStatus", nullptr, (EPropertyFlags)0x0020080000000001, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(UCommonPlayerInputKey), &Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bShowUnboundStatus_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bShowUnboundStatus_MetaData), Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bShowUnboundStatus_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_KeyBindTextFont_MetaData[] = {
		{ "Category", "Font" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** The font to apply at each size */" },
#endif
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The font to apply at each size" },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_KeyBindTextFont = { "KeyBindTextFont", nullptr, (EPropertyFlags)0x0020080000010001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonPlayerInputKey, KeyBindTextFont), Z_Construct_UScriptStruct_FSlateFontInfo, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_KeyBindTextFont_MetaData), Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_KeyBindTextFont_MetaData) }; // 2074891513
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_CountdownTextFont_MetaData[] = {
		{ "Category", "Font" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** The font to apply at each size */" },
#endif
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The font to apply at each size" },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_CountdownTextFont = { "CountdownTextFont", nullptr, (EPropertyFlags)0x0020080000010001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonPlayerInputKey, CountdownTextFont), Z_Construct_UScriptStruct_FSlateFontInfo, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_CountdownTextFont_MetaData), Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_CountdownTextFont_MetaData) }; // 2074891513
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_CountdownText_MetaData[] = {
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_CountdownText = { "CountdownText", nullptr, (EPropertyFlags)0x0020080000002000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonPlayerInputKey, CountdownText), Z_Construct_UScriptStruct_FMeasuredText, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_CountdownText_MetaData), Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_CountdownText_MetaData) }; // 3442505675
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_KeybindText_MetaData[] = {
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_KeybindText = { "KeybindText", nullptr, (EPropertyFlags)0x0020080000002000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonPlayerInputKey, KeybindText), Z_Construct_UScriptStruct_FMeasuredText, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_KeybindText_MetaData), Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_KeybindText_MetaData) }; // 3442505675
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_KeybindTextPadding_MetaData[] = {
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_KeybindTextPadding = { "KeybindTextPadding", nullptr, (EPropertyFlags)0x0020080000002000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonPlayerInputKey, KeybindTextPadding), Z_Construct_UScriptStruct_FMargin, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_KeybindTextPadding_MetaData), Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_KeybindTextPadding_MetaData) }; // 2235102396
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_KeybindFrameMinimumSize_MetaData[] = {
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_KeybindFrameMinimumSize = { "KeybindFrameMinimumSize", nullptr, (EPropertyFlags)0x0020080000002000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonPlayerInputKey, KeybindFrameMinimumSize), Z_Construct_UScriptStruct_FVector2D, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_KeybindFrameMinimumSize_MetaData), Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_KeybindFrameMinimumSize_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_PercentageMaterialParameterName_MetaData[] = {
		{ "Category", "Keybind Widget" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** The material parameter name for hold percentage in the HoldKeybindImage */" },
#endif
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The material parameter name for hold percentage in the HoldKeybindImage" },
#endif
	};
#endif
	const UECodeGen_Private::FNamePropertyParams Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_PercentageMaterialParameterName = { "PercentageMaterialParameterName", nullptr, (EPropertyFlags)0x0020080000010001, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonPlayerInputKey, PercentageMaterialParameterName), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_PercentageMaterialParameterName_MetaData), Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_PercentageMaterialParameterName_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_ProgressPercentageMID_MetaData[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** MID for the progress percentage */" },
#endif
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "MID for the progress percentage" },
#endif
	};
#endif
	const UECodeGen_Private::FObjectPtrPropertyParams Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_ProgressPercentageMID = { "ProgressPercentageMID", nullptr, (EPropertyFlags)0x0024080000002000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonPlayerInputKey, ProgressPercentageMID), Z_Construct_UClass_UMaterialInstanceDynamic_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_ProgressPercentageMID_MetaData), Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_ProgressPercentageMID_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_CachedKeyBrush_MetaData[] = {
		{ "ModuleRelativePath", "Public/CommonPlayerInputKey.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_CachedKeyBrush = { "CachedKeyBrush", nullptr, (EPropertyFlags)0x0040000000002000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonPlayerInputKey, CachedKeyBrush), Z_Construct_UScriptStruct_FSlateBrush, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_CachedKeyBrush_MetaData), Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_CachedKeyBrush_MetaData) }; // 899956612
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UCommonPlayerInputKey_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_BoundAction,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_AxisScale,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_BoundKeyFallback,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_InputTypeOverride_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_InputTypeOverride,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_PresetNameOverride,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_ForcedHoldKeybindStatus_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_ForcedHoldKeybindStatus,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bIsHoldKeybind,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bShowKeybindBorder,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_FrameSize,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bShowTimeCountDown,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_BoundKey,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_HoldProgressBrush,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_KeyBindTextBorder,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_bShowUnboundStatus,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_KeyBindTextFont,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_CountdownTextFont,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_CountdownText,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_KeybindText,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_KeybindTextPadding,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_KeybindFrameMinimumSize,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_PercentageMaterialParameterName,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_ProgressPercentageMID,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonPlayerInputKey_Statics::NewProp_CachedKeyBrush,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_UCommonPlayerInputKey_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UCommonPlayerInputKey>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UCommonPlayerInputKey_Statics::ClassParams = {
		&UCommonPlayerInputKey::StaticClass,
		nullptr,
		&StaticCppClassTypeInfo,
		DependentSingletons,
		FuncInfo,
		Z_Construct_UClass_UCommonPlayerInputKey_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		UE_ARRAY_COUNT(FuncInfo),
		UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::PropPointers),
		0,
		0x00B010A1u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::Class_MetaDataParams), Z_Construct_UClass_UCommonPlayerInputKey_Statics::Class_MetaDataParams)
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonPlayerInputKey_Statics::PropPointers) < 2048);
	UClass* Z_Construct_UClass_UCommonPlayerInputKey()
	{
		if (!Z_Registration_Info_UClass_UCommonPlayerInputKey.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UCommonPlayerInputKey.OuterSingleton, Z_Construct_UClass_UCommonPlayerInputKey_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UCommonPlayerInputKey.OuterSingleton;
	}
	template<> COMMONGAME_API UClass* StaticClass<UCommonPlayerInputKey>()
	{
		return UCommonPlayerInputKey::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UCommonPlayerInputKey);
	UCommonPlayerInputKey::~UCommonPlayerInputKey() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonPlayerInputKey_h_Statics
	{
		static const FEnumRegisterCompiledInInfo EnumInfo[];
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FEnumRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonPlayerInputKey_h_Statics::EnumInfo[] = {
		{ ECommonKeybindForcedHoldStatus_StaticEnum, TEXT("ECommonKeybindForcedHoldStatus"), &Z_Registration_Info_UEnum_ECommonKeybindForcedHoldStatus, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 2269447710U) },
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonPlayerInputKey_h_Statics::ScriptStructInfo[] = {
		{ FMeasuredText::StaticStruct, Z_Construct_UScriptStruct_FMeasuredText_Statics::NewStructOps, TEXT("MeasuredText"), &Z_Registration_Info_UScriptStruct_MeasuredText, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FMeasuredText), 3442505675U) },
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonPlayerInputKey_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UCommonPlayerInputKey, UCommonPlayerInputKey::StaticClass, TEXT("UCommonPlayerInputKey"), &Z_Registration_Info_UClass_UCommonPlayerInputKey, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UCommonPlayerInputKey), 4232001377U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonPlayerInputKey_h_1011404832(TEXT("/Script/CommonGame"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonPlayerInputKey_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonPlayerInputKey_h_Statics::ClassInfo),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonPlayerInputKey_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonPlayerInputKey_h_Statics::ScriptStructInfo),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonPlayerInputKey_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonPlayerInputKey_h_Statics::EnumInfo));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
