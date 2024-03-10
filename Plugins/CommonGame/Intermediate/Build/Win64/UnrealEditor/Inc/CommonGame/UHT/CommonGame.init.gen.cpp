// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeCommonGame_init() {}
	COMMONGAME_API UFunction* Z_Construct_UDelegateFunction_CommonGame_CommonMessagingResultMCDelegate__DelegateSignature();
	COMMONGAME_API UFunction* Z_Construct_UDelegateFunction_CommonGame_CreateWidgetAsyncDelegate__DelegateSignature();
	COMMONGAME_API UFunction* Z_Construct_UDelegateFunction_CommonGame_PushContentToLayerForPlayerAsyncDelegate__DelegateSignature();
	static FPackageRegistrationInfo Z_Registration_Info_UPackage__Script_CommonGame;
	FORCENOINLINE UPackage* Z_Construct_UPackage__Script_CommonGame()
	{
		if (!Z_Registration_Info_UPackage__Script_CommonGame.OuterSingleton)
		{
			static UObject* (*const SingletonFuncArray[])() = {
				(UObject* (*)())Z_Construct_UDelegateFunction_CommonGame_CommonMessagingResultMCDelegate__DelegateSignature,
				(UObject* (*)())Z_Construct_UDelegateFunction_CommonGame_CreateWidgetAsyncDelegate__DelegateSignature,
				(UObject* (*)())Z_Construct_UDelegateFunction_CommonGame_PushContentToLayerForPlayerAsyncDelegate__DelegateSignature,
			};
			static const UECodeGen_Private::FPackageParams PackageParams = {
				"/Script/CommonGame",
				SingletonFuncArray,
				UE_ARRAY_COUNT(SingletonFuncArray),
				PKG_CompiledIn | 0x00000000,
				0xEC033BDC,
				0x2FF9ABBF,
				METADATA_PARAMS(0, nullptr)
			};
			UECodeGen_Private::ConstructUPackage(Z_Registration_Info_UPackage__Script_CommonGame.OuterSingleton, PackageParams);
		}
		return Z_Registration_Info_UPackage__Script_CommonGame.OuterSingleton;
	}
	static FRegisterCompiledInInfo Z_CompiledInDeferPackage_UPackage__Script_CommonGame(Z_Construct_UPackage__Script_CommonGame, TEXT("/Script/CommonGame"), Z_Registration_Info_UPackage__Script_CommonGame, CONSTRUCT_RELOAD_VERSION_INFO(FPackageReloadVersionInfo, 0xEC033BDC, 0x2FF9ABBF));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
