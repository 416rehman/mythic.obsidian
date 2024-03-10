// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "ShooterCoreRuntime/Public/Messages/ControlPointStatusMessage.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeControlPointStatusMessage() {}
// Cross Module References
	ENGINE_API UClass* Z_Construct_UClass_AActor_NoRegister();
	SHOOTERCORERUNTIME_API UScriptStruct* Z_Construct_UScriptStruct_FLyraControlPointStatusMessage();
	UPackage* Z_Construct_UPackage__Script_ShooterCoreRuntime();
// End Cross Module References
	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_LyraControlPointStatusMessage;
class UScriptStruct* FLyraControlPointStatusMessage::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_LyraControlPointStatusMessage.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_LyraControlPointStatusMessage.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FLyraControlPointStatusMessage, (UObject*)Z_Construct_UPackage__Script_ShooterCoreRuntime(), TEXT("LyraControlPointStatusMessage"));
	}
	return Z_Registration_Info_UScriptStruct_LyraControlPointStatusMessage.OuterSingleton;
}
template<> SHOOTERCORERUNTIME_API UScriptStruct* StaticStruct<FLyraControlPointStatusMessage>()
{
	return FLyraControlPointStatusMessage::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FLyraControlPointStatusMessage_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_ControlPoint_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPtrPropertyParams NewProp_ControlPoint;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_OwnerTeamID_MetaData[];
#endif
		static const UECodeGen_Private::FIntPropertyParams NewProp_OwnerTeamID;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FLyraControlPointStatusMessage_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Message indicating the state of a control point is changing\n" },
#endif
		{ "ModuleRelativePath", "Public/Messages/ControlPointStatusMessage.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Message indicating the state of a control point is changing" },
#endif
	};
#endif
	void* Z_Construct_UScriptStruct_FLyraControlPointStatusMessage_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FLyraControlPointStatusMessage>();
	}
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FLyraControlPointStatusMessage_Statics::NewProp_ControlPoint_MetaData[] = {
		{ "Category", "Gameplay" },
		{ "ModuleRelativePath", "Public/Messages/ControlPointStatusMessage.h" },
	};
#endif
	const UECodeGen_Private::FObjectPtrPropertyParams Z_Construct_UScriptStruct_FLyraControlPointStatusMessage_Statics::NewProp_ControlPoint = { "ControlPoint", nullptr, (EPropertyFlags)0x0014000000000004, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FLyraControlPointStatusMessage, ControlPoint), Z_Construct_UClass_AActor_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FLyraControlPointStatusMessage_Statics::NewProp_ControlPoint_MetaData), Z_Construct_UScriptStruct_FLyraControlPointStatusMessage_Statics::NewProp_ControlPoint_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FLyraControlPointStatusMessage_Statics::NewProp_OwnerTeamID_MetaData[] = {
		{ "Category", "Gameplay" },
		{ "ModuleRelativePath", "Public/Messages/ControlPointStatusMessage.h" },
	};
#endif
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FLyraControlPointStatusMessage_Statics::NewProp_OwnerTeamID = { "OwnerTeamID", nullptr, (EPropertyFlags)0x0010000000000004, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FLyraControlPointStatusMessage, OwnerTeamID), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FLyraControlPointStatusMessage_Statics::NewProp_OwnerTeamID_MetaData), Z_Construct_UScriptStruct_FLyraControlPointStatusMessage_Statics::NewProp_OwnerTeamID_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FLyraControlPointStatusMessage_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FLyraControlPointStatusMessage_Statics::NewProp_ControlPoint,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FLyraControlPointStatusMessage_Statics::NewProp_OwnerTeamID,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FLyraControlPointStatusMessage_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_ShooterCoreRuntime,
		nullptr,
		&NewStructOps,
		"LyraControlPointStatusMessage",
		Z_Construct_UScriptStruct_FLyraControlPointStatusMessage_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FLyraControlPointStatusMessage_Statics::PropPointers),
		sizeof(FLyraControlPointStatusMessage),
		alignof(FLyraControlPointStatusMessage),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FLyraControlPointStatusMessage_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FLyraControlPointStatusMessage_Statics::Struct_MetaDataParams)
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FLyraControlPointStatusMessage_Statics::PropPointers) < 2048);
	UScriptStruct* Z_Construct_UScriptStruct_FLyraControlPointStatusMessage()
	{
		if (!Z_Registration_Info_UScriptStruct_LyraControlPointStatusMessage.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_LyraControlPointStatusMessage.InnerSingleton, Z_Construct_UScriptStruct_FLyraControlPointStatusMessage_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_LyraControlPointStatusMessage.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_Messages_ControlPointStatusMessage_h_Statics
	{
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_Messages_ControlPointStatusMessage_h_Statics::ScriptStructInfo[] = {
		{ FLyraControlPointStatusMessage::StaticStruct, Z_Construct_UScriptStruct_FLyraControlPointStatusMessage_Statics::NewStructOps, TEXT("LyraControlPointStatusMessage"), &Z_Registration_Info_UScriptStruct_LyraControlPointStatusMessage, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FLyraControlPointStatusMessage), 3009026764U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_Messages_ControlPointStatusMessage_h_3516605107(TEXT("/Script/ShooterCoreRuntime"),
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_Messages_ControlPointStatusMessage_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_Messages_ControlPointStatusMessage_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
