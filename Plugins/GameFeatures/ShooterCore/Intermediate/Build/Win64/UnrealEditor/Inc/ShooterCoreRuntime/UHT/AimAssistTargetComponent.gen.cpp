// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "ShooterCoreRuntime/Public/Input/AimAssistTargetComponent.h"
#include "ShooterCoreRuntime/Public/Input/IAimAssistTargetInterface.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeAimAssistTargetComponent() {}
// Cross Module References
	ENGINE_API UClass* Z_Construct_UClass_UCapsuleComponent();
	SHOOTERCORERUNTIME_API UClass* Z_Construct_UClass_UAimAssistTaget_NoRegister();
	SHOOTERCORERUNTIME_API UClass* Z_Construct_UClass_UAimAssistTargetComponent();
	SHOOTERCORERUNTIME_API UClass* Z_Construct_UClass_UAimAssistTargetComponent_NoRegister();
	SHOOTERCORERUNTIME_API UScriptStruct* Z_Construct_UScriptStruct_FAimAssistTargetOptions();
	UPackage* Z_Construct_UPackage__Script_ShooterCoreRuntime();
// End Cross Module References
	void UAimAssistTargetComponent::StaticRegisterNativesUAimAssistTargetComponent()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UAimAssistTargetComponent);
	UClass* Z_Construct_UClass_UAimAssistTargetComponent_NoRegister()
	{
		return UAimAssistTargetComponent::StaticClass();
	}
	struct Z_Construct_UClass_UAimAssistTargetComponent_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_TargetData_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_TargetData;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FImplementedInterfaceParams InterfaceParams[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UAimAssistTargetComponent_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UCapsuleComponent,
		(UObject* (*)())Z_Construct_UPackage__Script_ShooterCoreRuntime,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UAimAssistTargetComponent_Statics::DependentSingletons) < 16);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UAimAssistTargetComponent_Statics::Class_MetaDataParams[] = {
		{ "BlueprintSpawnableComponent", "" },
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * This component can be added to any actor to have it register with the Aim Assist Target Manager.\n */" },
#endif
		{ "HideCategories", "Object LOD Lighting TextureStreaming Object LOD Lighting TextureStreaming Activation Components|Activation Trigger VirtualTexture" },
		{ "IncludePath", "Input/AimAssistTargetComponent.h" },
		{ "ModuleRelativePath", "Public/Input/AimAssistTargetComponent.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "This component can be added to any actor to have it register with the Aim Assist Target Manager." },
#endif
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UAimAssistTargetComponent_Statics::NewProp_TargetData_MetaData[] = {
		{ "Category", "AimAssistTargetComponent" },
		{ "ModuleRelativePath", "Public/Input/AimAssistTargetComponent.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UAimAssistTargetComponent_Statics::NewProp_TargetData = { "TargetData", nullptr, (EPropertyFlags)0x0020080000000005, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UAimAssistTargetComponent, TargetData), Z_Construct_UScriptStruct_FAimAssistTargetOptions, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UAimAssistTargetComponent_Statics::NewProp_TargetData_MetaData), Z_Construct_UClass_UAimAssistTargetComponent_Statics::NewProp_TargetData_MetaData) }; // 2137094904
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UAimAssistTargetComponent_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAimAssistTargetComponent_Statics::NewProp_TargetData,
	};
		const UECodeGen_Private::FImplementedInterfaceParams Z_Construct_UClass_UAimAssistTargetComponent_Statics::InterfaceParams[] = {
			{ Z_Construct_UClass_UAimAssistTaget_NoRegister, (int32)VTABLE_OFFSET(UAimAssistTargetComponent, IAimAssistTaget), false },  // 692755740
		};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UAimAssistTargetComponent_Statics::InterfaceParams) < 64);
	const FCppClassTypeInfoStatic Z_Construct_UClass_UAimAssistTargetComponent_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UAimAssistTargetComponent>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UAimAssistTargetComponent_Statics::ClassParams = {
		&UAimAssistTargetComponent::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_UAimAssistTargetComponent_Statics::PropPointers,
		InterfaceParams,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_UAimAssistTargetComponent_Statics::PropPointers),
		UE_ARRAY_COUNT(InterfaceParams),
		0x00B010A4u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UAimAssistTargetComponent_Statics::Class_MetaDataParams), Z_Construct_UClass_UAimAssistTargetComponent_Statics::Class_MetaDataParams)
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UAimAssistTargetComponent_Statics::PropPointers) < 2048);
	UClass* Z_Construct_UClass_UAimAssistTargetComponent()
	{
		if (!Z_Registration_Info_UClass_UAimAssistTargetComponent.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UAimAssistTargetComponent.OuterSingleton, Z_Construct_UClass_UAimAssistTargetComponent_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UAimAssistTargetComponent.OuterSingleton;
	}
	template<> SHOOTERCORERUNTIME_API UClass* StaticClass<UAimAssistTargetComponent>()
	{
		return UAimAssistTargetComponent::StaticClass();
	}
	UAimAssistTargetComponent::UAimAssistTargetComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UAimAssistTargetComponent);
	UAimAssistTargetComponent::~UAimAssistTargetComponent() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_Input_AimAssistTargetComponent_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_Input_AimAssistTargetComponent_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UAimAssistTargetComponent, UAimAssistTargetComponent::StaticClass, TEXT("UAimAssistTargetComponent"), &Z_Registration_Info_UClass_UAimAssistTargetComponent, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UAimAssistTargetComponent), 743625040U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_Input_AimAssistTargetComponent_h_315984115(TEXT("/Script/ShooterCoreRuntime"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_Input_AimAssistTargetComponent_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_Input_AimAssistTargetComponent_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
