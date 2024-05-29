// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodePocketWorlds_init() {}
	static FPackageRegistrationInfo Z_Registration_Info_UPackage__Script_PocketWorlds;
	FORCENOINLINE UPackage* Z_Construct_UPackage__Script_PocketWorlds()
	{
		if (!Z_Registration_Info_UPackage__Script_PocketWorlds.OuterSingleton)
		{
			static const UECodeGen_Private::FPackageParams PackageParams = {
				"/Script/PocketWorlds",
				nullptr,
				0,
				PKG_CompiledIn | 0x00000000,
				0x0FACEAFA,
				0x57C61C77,
				METADATA_PARAMS(0, nullptr)
			};
			UECodeGen_Private::ConstructUPackage(Z_Registration_Info_UPackage__Script_PocketWorlds.OuterSingleton, PackageParams);
		}
		return Z_Registration_Info_UPackage__Script_PocketWorlds.OuterSingleton;
	}
	static FRegisterCompiledInInfo Z_CompiledInDeferPackage_UPackage__Script_PocketWorlds(Z_Construct_UPackage__Script_PocketWorlds, TEXT("/Script/PocketWorlds"), Z_Registration_Info_UPackage__Script_PocketWorlds, CONSTRUCT_RELOAD_VERSION_INFO(FPackageReloadVersionInfo, 0x0FACEAFA, 0x57C61C77));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
