// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "TopDownArenaRuntime/Private/TopDownArenaPickupUIData.h"
#include "GameplayEffect.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeTopDownArenaPickupUIData() {}
// Cross Module References
	ENGINE_API UClass* Z_Construct_UClass_USoundBase_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_UTexture2D_NoRegister();
	GAMEPLAYABILITIES_API UClass* Z_Construct_UClass_UGameplayEffectUIData();
	NIAGARA_API UClass* Z_Construct_UClass_UNiagaraSystem_NoRegister();
	TOPDOWNARENARUNTIME_API UClass* Z_Construct_UClass_UTopDownArenaPickupUIData();
	TOPDOWNARENARUNTIME_API UClass* Z_Construct_UClass_UTopDownArenaPickupUIData_NoRegister();
	UPackage* Z_Construct_UPackage__Script_TopDownArenaRuntime();
// End Cross Module References
	void UTopDownArenaPickupUIData::StaticRegisterNativesUTopDownArenaPickupUIData()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UTopDownArenaPickupUIData);
	UClass* Z_Construct_UClass_UTopDownArenaPickupUIData_NoRegister()
	{
		return UTopDownArenaPickupUIData::StaticClass();
	}
	struct Z_Construct_UClass_UTopDownArenaPickupUIData_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Description_MetaData[];
#endif
		static const UECodeGen_Private::FTextPropertyParams NewProp_Description;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_ShortDescriptionForToast_MetaData[];
#endif
		static const UECodeGen_Private::FTextPropertyParams NewProp_ShortDescriptionForToast;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_IconTexture_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPtrPropertyParams NewProp_IconTexture;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PickupVFX_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPtrPropertyParams NewProp_PickupVFX;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PickupSFX_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPtrPropertyParams NewProp_PickupSFX;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UGameplayEffectUIData,
		(UObject* (*)())Z_Construct_UPackage__Script_TopDownArenaRuntime,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::DependentSingletons) < 16);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Icon and display name for pickups in the top-down arena game\n" },
#endif
		{ "IncludePath", "TopDownArenaPickupUIData.h" },
		{ "ModuleRelativePath", "Private/TopDownArenaPickupUIData.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Icon and display name for pickups in the top-down arena game" },
#endif
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::NewProp_Description_MetaData[] = {
		{ "Category", "Data" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// The full description of the pickup\n" },
#endif
		{ "ModuleRelativePath", "Private/TopDownArenaPickupUIData.h" },
		{ "MultiLine", "true" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The full description of the pickup" },
#endif
	};
#endif
	const UECodeGen_Private::FTextPropertyParams Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::NewProp_Description = { "Description", nullptr, (EPropertyFlags)0x0010000000010015, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTopDownArenaPickupUIData, Description), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::NewProp_Description_MetaData), Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::NewProp_Description_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::NewProp_ShortDescriptionForToast_MetaData[] = {
		{ "Category", "Data" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// The short description of the pickup (displayed by the player name when picked up)\n" },
#endif
		{ "ModuleRelativePath", "Private/TopDownArenaPickupUIData.h" },
		{ "MultiLine", "true" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The short description of the pickup (displayed by the player name when picked up)" },
#endif
	};
#endif
	const UECodeGen_Private::FTextPropertyParams Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::NewProp_ShortDescriptionForToast = { "ShortDescriptionForToast", nullptr, (EPropertyFlags)0x0010000000010015, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTopDownArenaPickupUIData, ShortDescriptionForToast), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::NewProp_ShortDescriptionForToast_MetaData), Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::NewProp_ShortDescriptionForToast_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::NewProp_IconTexture_MetaData[] = {
		{ "Category", "Data" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// The icon material used to show the pickup in the world\n" },
#endif
		{ "ModuleRelativePath", "Private/TopDownArenaPickupUIData.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The icon material used to show the pickup in the world" },
#endif
	};
#endif
	const UECodeGen_Private::FObjectPtrPropertyParams Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::NewProp_IconTexture = { "IconTexture", nullptr, (EPropertyFlags)0x0014000000010015, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTopDownArenaPickupUIData, IconTexture), Z_Construct_UClass_UTexture2D_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::NewProp_IconTexture_MetaData), Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::NewProp_IconTexture_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::NewProp_PickupVFX_MetaData[] = {
		{ "Category", "Data" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// The pickup VFX override\n" },
#endif
		{ "ModuleRelativePath", "Private/TopDownArenaPickupUIData.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The pickup VFX override" },
#endif
	};
#endif
	const UECodeGen_Private::FObjectPtrPropertyParams Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::NewProp_PickupVFX = { "PickupVFX", nullptr, (EPropertyFlags)0x0014000000010015, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTopDownArenaPickupUIData, PickupVFX), Z_Construct_UClass_UNiagaraSystem_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::NewProp_PickupVFX_MetaData), Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::NewProp_PickupVFX_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::NewProp_PickupSFX_MetaData[] = {
		{ "Category", "Data" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// The pickup SFX override (if not set, a default will play)\n" },
#endif
		{ "ModuleRelativePath", "Private/TopDownArenaPickupUIData.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The pickup SFX override (if not set, a default will play)" },
#endif
	};
#endif
	const UECodeGen_Private::FObjectPtrPropertyParams Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::NewProp_PickupSFX = { "PickupSFX", nullptr, (EPropertyFlags)0x0014000000010015, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UTopDownArenaPickupUIData, PickupSFX), Z_Construct_UClass_USoundBase_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::NewProp_PickupSFX_MetaData), Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::NewProp_PickupSFX_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::NewProp_Description,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::NewProp_ShortDescriptionForToast,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::NewProp_IconTexture,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::NewProp_PickupVFX,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::NewProp_PickupSFX,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UTopDownArenaPickupUIData>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::ClassParams = {
		&UTopDownArenaPickupUIData::StaticClass,
		nullptr,
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::PropPointers),
		0,
		0x002130A0u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::Class_MetaDataParams), Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::Class_MetaDataParams)
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::PropPointers) < 2048);
	UClass* Z_Construct_UClass_UTopDownArenaPickupUIData()
	{
		if (!Z_Registration_Info_UClass_UTopDownArenaPickupUIData.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UTopDownArenaPickupUIData.OuterSingleton, Z_Construct_UClass_UTopDownArenaPickupUIData_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UTopDownArenaPickupUIData.OuterSingleton;
	}
	template<> TOPDOWNARENARUNTIME_API UClass* StaticClass<UTopDownArenaPickupUIData>()
	{
		return UTopDownArenaPickupUIData::StaticClass();
	}
	UTopDownArenaPickupUIData::UTopDownArenaPickupUIData() {}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UTopDownArenaPickupUIData);
	UTopDownArenaPickupUIData::~UTopDownArenaPickupUIData() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_TopDownArena_Source_TopDownArenaRuntime_Private_TopDownArenaPickupUIData_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_TopDownArena_Source_TopDownArenaRuntime_Private_TopDownArenaPickupUIData_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UTopDownArenaPickupUIData, UTopDownArenaPickupUIData::StaticClass, TEXT("UTopDownArenaPickupUIData"), &Z_Registration_Info_UClass_UTopDownArenaPickupUIData, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UTopDownArenaPickupUIData), 1339049258U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_TopDownArena_Source_TopDownArenaRuntime_Private_TopDownArenaPickupUIData_h_2698608643(TEXT("/Script/TopDownArenaRuntime"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_TopDownArena_Source_TopDownArenaRuntime_Private_TopDownArenaPickupUIData_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_TopDownArena_Source_TopDownArenaRuntime_Private_TopDownArenaPickupUIData_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
