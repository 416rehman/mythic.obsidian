// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/CommonLocalPlayer.h"
#include "../../Source/Runtime/Engine/Classes/Engine/Engine.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeCommonLocalPlayer() {}
// Cross Module References
	COMMONGAME_API UClass* Z_Construct_UClass_UCommonLocalPlayer();
	COMMONGAME_API UClass* Z_Construct_UClass_UCommonLocalPlayer_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_ULocalPlayer();
	UPackage* Z_Construct_UPackage__Script_CommonGame();
// End Cross Module References
	void UCommonLocalPlayer::StaticRegisterNativesUCommonLocalPlayer()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UCommonLocalPlayer);
	UClass* Z_Construct_UClass_UCommonLocalPlayer_NoRegister()
	{
		return UCommonLocalPlayer::StaticClass();
	}
	struct Z_Construct_UClass_UCommonLocalPlayer_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UCommonLocalPlayer_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_ULocalPlayer,
		(UObject* (*)())Z_Construct_UPackage__Script_CommonGame,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonLocalPlayer_Statics::DependentSingletons) < 16);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonLocalPlayer_Statics::Class_MetaDataParams[] = {
		{ "IncludePath", "CommonLocalPlayer.h" },
		{ "ModuleRelativePath", "Public/CommonLocalPlayer.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UCommonLocalPlayer_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UCommonLocalPlayer>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UCommonLocalPlayer_Statics::ClassParams = {
		&UCommonLocalPlayer::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		nullptr,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		0,
		0,
		0x001000ACu,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonLocalPlayer_Statics::Class_MetaDataParams), Z_Construct_UClass_UCommonLocalPlayer_Statics::Class_MetaDataParams)
	};
	UClass* Z_Construct_UClass_UCommonLocalPlayer()
	{
		if (!Z_Registration_Info_UClass_UCommonLocalPlayer.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UCommonLocalPlayer.OuterSingleton, Z_Construct_UClass_UCommonLocalPlayer_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UCommonLocalPlayer.OuterSingleton;
	}
	template<> COMMONGAME_API UClass* StaticClass<UCommonLocalPlayer>()
	{
		return UCommonLocalPlayer::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UCommonLocalPlayer);
	UCommonLocalPlayer::~UCommonLocalPlayer() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonLocalPlayer_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonLocalPlayer_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UCommonLocalPlayer, UCommonLocalPlayer::StaticClass, TEXT("UCommonLocalPlayer"), &Z_Registration_Info_UClass_UCommonLocalPlayer, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UCommonLocalPlayer), 607995340U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonLocalPlayer_h_2110536473(TEXT("/Script/CommonGame"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonLocalPlayer_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonLocalPlayer_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
