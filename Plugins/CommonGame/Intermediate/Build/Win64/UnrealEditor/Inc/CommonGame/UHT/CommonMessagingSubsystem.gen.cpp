// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/Messaging/CommonMessagingSubsystem.h"
#include "../../Source/Runtime/Engine/Classes/Engine/LocalPlayer.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeCommonMessagingSubsystem() {}
// Cross Module References
	COMMONGAME_API UClass* Z_Construct_UClass_UCommonMessagingSubsystem();
	COMMONGAME_API UClass* Z_Construct_UClass_UCommonMessagingSubsystem_NoRegister();
	COMMONGAME_API UEnum* Z_Construct_UEnum_CommonGame_ECommonMessagingResult();
	ENGINE_API UClass* Z_Construct_UClass_ULocalPlayerSubsystem();
	UPackage* Z_Construct_UPackage__Script_CommonGame();
// End Cross Module References
	static FEnumRegistrationInfo Z_Registration_Info_UEnum_ECommonMessagingResult;
	static UEnum* ECommonMessagingResult_StaticEnum()
	{
		if (!Z_Registration_Info_UEnum_ECommonMessagingResult.OuterSingleton)
		{
			Z_Registration_Info_UEnum_ECommonMessagingResult.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_CommonGame_ECommonMessagingResult, (UObject*)Z_Construct_UPackage__Script_CommonGame(), TEXT("ECommonMessagingResult"));
		}
		return Z_Registration_Info_UEnum_ECommonMessagingResult.OuterSingleton;
	}
	template<> COMMONGAME_API UEnum* StaticEnum<ECommonMessagingResult>()
	{
		return ECommonMessagingResult_StaticEnum();
	}
	struct Z_Construct_UEnum_CommonGame_ECommonMessagingResult_Statics
	{
		static const UECodeGen_Private::FEnumeratorParam Enumerators[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[];
#endif
		static const UECodeGen_Private::FEnumParams EnumParams;
	};
	const UECodeGen_Private::FEnumeratorParam Z_Construct_UEnum_CommonGame_ECommonMessagingResult_Statics::Enumerators[] = {
		{ "ECommonMessagingResult::Confirmed", (int64)ECommonMessagingResult::Confirmed },
		{ "ECommonMessagingResult::Declined", (int64)ECommonMessagingResult::Declined },
		{ "ECommonMessagingResult::Cancelled", (int64)ECommonMessagingResult::Cancelled },
		{ "ECommonMessagingResult::Killed", (int64)ECommonMessagingResult::Killed },
		{ "ECommonMessagingResult::Unknown", (int64)ECommonMessagingResult::Unknown },
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UEnum_CommonGame_ECommonMessagingResult_Statics::Enum_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "Cancelled.Comment", "/** The \"ignore/cancel\" button was pressed */" },
		{ "Cancelled.Name", "ECommonMessagingResult::Cancelled" },
		{ "Cancelled.ToolTip", "The \"ignore/cancel\" button was pressed" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Possible results from a dialog */" },
#endif
		{ "Confirmed.Comment", "/** The \"yes\" button was pressed */" },
		{ "Confirmed.Name", "ECommonMessagingResult::Confirmed" },
		{ "Confirmed.ToolTip", "The \"yes\" button was pressed" },
		{ "Declined.Comment", "/** The \"no\" button was pressed */" },
		{ "Declined.Name", "ECommonMessagingResult::Declined" },
		{ "Declined.ToolTip", "The \"no\" button was pressed" },
		{ "Killed.Comment", "/** The dialog was explicitly killed (no user input) */" },
		{ "Killed.Name", "ECommonMessagingResult::Killed" },
		{ "Killed.ToolTip", "The dialog was explicitly killed (no user input)" },
		{ "ModuleRelativePath", "Public/Messaging/CommonMessagingSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Possible results from a dialog" },
#endif
		{ "Unknown.Hidden", "" },
		{ "Unknown.Name", "ECommonMessagingResult::Unknown" },
	};
#endif
	const UECodeGen_Private::FEnumParams Z_Construct_UEnum_CommonGame_ECommonMessagingResult_Statics::EnumParams = {
		(UObject*(*)())Z_Construct_UPackage__Script_CommonGame,
		nullptr,
		"ECommonMessagingResult",
		"ECommonMessagingResult",
		Z_Construct_UEnum_CommonGame_ECommonMessagingResult_Statics::Enumerators,
		RF_Public|RF_Transient|RF_MarkAsNative,
		UE_ARRAY_COUNT(Z_Construct_UEnum_CommonGame_ECommonMessagingResult_Statics::Enumerators),
		EEnumFlags::None,
		(uint8)UEnum::ECppForm::EnumClass,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UEnum_CommonGame_ECommonMessagingResult_Statics::Enum_MetaDataParams), Z_Construct_UEnum_CommonGame_ECommonMessagingResult_Statics::Enum_MetaDataParams)
	};
	UEnum* Z_Construct_UEnum_CommonGame_ECommonMessagingResult()
	{
		if (!Z_Registration_Info_UEnum_ECommonMessagingResult.InnerSingleton)
		{
			UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_ECommonMessagingResult.InnerSingleton, Z_Construct_UEnum_CommonGame_ECommonMessagingResult_Statics::EnumParams);
		}
		return Z_Registration_Info_UEnum_ECommonMessagingResult.InnerSingleton;
	}
	void UCommonMessagingSubsystem::StaticRegisterNativesUCommonMessagingSubsystem()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UCommonMessagingSubsystem);
	UClass* Z_Construct_UClass_UCommonMessagingSubsystem_NoRegister()
	{
		return UCommonMessagingSubsystem::StaticClass();
	}
	struct Z_Construct_UClass_UCommonMessagingSubsystem_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UCommonMessagingSubsystem_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_ULocalPlayerSubsystem,
		(UObject* (*)())Z_Construct_UPackage__Script_CommonGame,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonMessagingSubsystem_Statics::DependentSingletons) < 16);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonMessagingSubsystem_Statics::Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * \n */" },
#endif
		{ "IncludePath", "Messaging/CommonMessagingSubsystem.h" },
		{ "ModuleRelativePath", "Public/Messaging/CommonMessagingSubsystem.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UCommonMessagingSubsystem_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UCommonMessagingSubsystem>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UCommonMessagingSubsystem_Statics::ClassParams = {
		&UCommonMessagingSubsystem::StaticClass,
		"Game",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		nullptr,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		0,
		0,
		0x001000A0u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonMessagingSubsystem_Statics::Class_MetaDataParams), Z_Construct_UClass_UCommonMessagingSubsystem_Statics::Class_MetaDataParams)
	};
	UClass* Z_Construct_UClass_UCommonMessagingSubsystem()
	{
		if (!Z_Registration_Info_UClass_UCommonMessagingSubsystem.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UCommonMessagingSubsystem.OuterSingleton, Z_Construct_UClass_UCommonMessagingSubsystem_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UCommonMessagingSubsystem.OuterSingleton;
	}
	template<> COMMONGAME_API UClass* StaticClass<UCommonMessagingSubsystem>()
	{
		return UCommonMessagingSubsystem::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UCommonMessagingSubsystem);
	UCommonMessagingSubsystem::~UCommonMessagingSubsystem() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonMessagingSubsystem_h_Statics
	{
		static const FEnumRegisterCompiledInInfo EnumInfo[];
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FEnumRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonMessagingSubsystem_h_Statics::EnumInfo[] = {
		{ ECommonMessagingResult_StaticEnum, TEXT("ECommonMessagingResult"), &Z_Registration_Info_UEnum_ECommonMessagingResult, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 921893972U) },
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonMessagingSubsystem_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UCommonMessagingSubsystem, UCommonMessagingSubsystem::StaticClass, TEXT("UCommonMessagingSubsystem"), &Z_Registration_Info_UClass_UCommonMessagingSubsystem, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UCommonMessagingSubsystem), 1203416807U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonMessagingSubsystem_h_3536183976(TEXT("/Script/CommonGame"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonMessagingSubsystem_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonMessagingSubsystem_h_Statics::ClassInfo),
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonMessagingSubsystem_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonMessagingSubsystem_h_Statics::EnumInfo));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
