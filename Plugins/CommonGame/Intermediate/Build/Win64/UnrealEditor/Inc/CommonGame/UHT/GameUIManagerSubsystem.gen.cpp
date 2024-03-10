// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/GameUIManagerSubsystem.h"
#include "../../Source/Runtime/Engine/Classes/Engine/GameInstance.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGameUIManagerSubsystem() {}
// Cross Module References
	COMMONGAME_API UClass* Z_Construct_UClass_UGameUIManagerSubsystem();
	COMMONGAME_API UClass* Z_Construct_UClass_UGameUIManagerSubsystem_NoRegister();
	COMMONGAME_API UClass* Z_Construct_UClass_UGameUIPolicy_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_UGameInstanceSubsystem();
	UPackage* Z_Construct_UPackage__Script_CommonGame();
// End Cross Module References
	void UGameUIManagerSubsystem::StaticRegisterNativesUGameUIManagerSubsystem()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UGameUIManagerSubsystem);
	UClass* Z_Construct_UClass_UGameUIManagerSubsystem_NoRegister()
	{
		return UGameUIManagerSubsystem::StaticClass();
	}
	struct Z_Construct_UClass_UGameUIManagerSubsystem_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_CurrentPolicy_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPtrPropertyParams NewProp_CurrentPolicy;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_DefaultUIPolicyClass_MetaData[];
#endif
		static const UECodeGen_Private::FSoftClassPropertyParams NewProp_DefaultUIPolicyClass;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UGameUIManagerSubsystem_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UGameInstanceSubsystem,
		(UObject* (*)())Z_Construct_UPackage__Script_CommonGame,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameUIManagerSubsystem_Statics::DependentSingletons) < 16);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UGameUIManagerSubsystem_Statics::Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * This manager is intended to be replaced by whatever your game needs to\n * actually create, so this class is abstract to prevent it from being created.\n * \n * If you just need the basic functionality you will start by sublcassing this\n * subsystem in your own game.\n */" },
#endif
		{ "IncludePath", "GameUIManagerSubsystem.h" },
		{ "ModuleRelativePath", "Public/GameUIManagerSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "This manager is intended to be replaced by whatever your game needs to\nactually create, so this class is abstract to prevent it from being created.\n\nIf you just need the basic functionality you will start by sublcassing this\nsubsystem in your own game." },
#endif
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UGameUIManagerSubsystem_Statics::NewProp_CurrentPolicy_MetaData[] = {
		{ "ModuleRelativePath", "Public/GameUIManagerSubsystem.h" },
	};
#endif
	const UECodeGen_Private::FObjectPtrPropertyParams Z_Construct_UClass_UGameUIManagerSubsystem_Statics::NewProp_CurrentPolicy = { "CurrentPolicy", nullptr, (EPropertyFlags)0x0044000000002000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UGameUIManagerSubsystem, CurrentPolicy), Z_Construct_UClass_UGameUIPolicy_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UGameUIManagerSubsystem_Statics::NewProp_CurrentPolicy_MetaData), Z_Construct_UClass_UGameUIManagerSubsystem_Statics::NewProp_CurrentPolicy_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UGameUIManagerSubsystem_Statics::NewProp_DefaultUIPolicyClass_MetaData[] = {
		{ "Category", "GameUIManagerSubsystem" },
		{ "ModuleRelativePath", "Public/GameUIManagerSubsystem.h" },
	};
#endif
	const UECodeGen_Private::FSoftClassPropertyParams Z_Construct_UClass_UGameUIManagerSubsystem_Statics::NewProp_DefaultUIPolicyClass = { "DefaultUIPolicyClass", nullptr, (EPropertyFlags)0x0044000000004001, UECodeGen_Private::EPropertyGenFlags::SoftClass, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UGameUIManagerSubsystem, DefaultUIPolicyClass), Z_Construct_UClass_UGameUIPolicy_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UGameUIManagerSubsystem_Statics::NewProp_DefaultUIPolicyClass_MetaData), Z_Construct_UClass_UGameUIManagerSubsystem_Statics::NewProp_DefaultUIPolicyClass_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UGameUIManagerSubsystem_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGameUIManagerSubsystem_Statics::NewProp_CurrentPolicy,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGameUIManagerSubsystem_Statics::NewProp_DefaultUIPolicyClass,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_UGameUIManagerSubsystem_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UGameUIManagerSubsystem>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UGameUIManagerSubsystem_Statics::ClassParams = {
		&UGameUIManagerSubsystem::StaticClass,
		"Game",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_UGameUIManagerSubsystem_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_UGameUIManagerSubsystem_Statics::PropPointers),
		0,
		0x001000A5u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UGameUIManagerSubsystem_Statics::Class_MetaDataParams), Z_Construct_UClass_UGameUIManagerSubsystem_Statics::Class_MetaDataParams)
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameUIManagerSubsystem_Statics::PropPointers) < 2048);
	UClass* Z_Construct_UClass_UGameUIManagerSubsystem()
	{
		if (!Z_Registration_Info_UClass_UGameUIManagerSubsystem.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UGameUIManagerSubsystem.OuterSingleton, Z_Construct_UClass_UGameUIManagerSubsystem_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UGameUIManagerSubsystem.OuterSingleton;
	}
	template<> COMMONGAME_API UClass* StaticClass<UGameUIManagerSubsystem>()
	{
		return UGameUIManagerSubsystem::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UGameUIManagerSubsystem);
	UGameUIManagerSubsystem::~UGameUIManagerSubsystem() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_GameUIManagerSubsystem_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_GameUIManagerSubsystem_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UGameUIManagerSubsystem, UGameUIManagerSubsystem::StaticClass, TEXT("UGameUIManagerSubsystem"), &Z_Registration_Info_UClass_UGameUIManagerSubsystem, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UGameUIManagerSubsystem), 2274174812U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_GameUIManagerSubsystem_h_1020028788(TEXT("/Script/CommonGame"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_GameUIManagerSubsystem_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_GameUIManagerSubsystem_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
