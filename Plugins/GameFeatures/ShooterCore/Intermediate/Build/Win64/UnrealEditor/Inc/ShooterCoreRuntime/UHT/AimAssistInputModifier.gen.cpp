// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "ShooterCoreRuntime/Public/Input/AimAssistInputModifier.h"
#include "ScalableFloat.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeAimAssistInputModifier() {}
// Cross Module References
	COREUOBJECT_API UClass* Z_Construct_UClass_UClass();
	COREUOBJECT_API UClass* Z_Construct_UClass_UObject_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_UCurveFloat_NoRegister();
	ENHANCEDINPUT_API UClass* Z_Construct_UClass_UInputAction_NoRegister();
	ENHANCEDINPUT_API UClass* Z_Construct_UClass_UInputModifier();
	GAMEPLAYABILITIES_API UScriptStruct* Z_Construct_UScriptStruct_FScalableFloat();
	MYTHIC_API UClass* Z_Construct_UClass_ULyraAimSensitivityData_NoRegister();
	MYTHIC_API UEnum* Z_Construct_UEnum_Mythic_ELyraTargetingType();
	SHOOTERCORERUNTIME_API UClass* Z_Construct_UClass_UAimAssistInputModifier();
	SHOOTERCORERUNTIME_API UClass* Z_Construct_UClass_UAimAssistInputModifier_NoRegister();
	SHOOTERCORERUNTIME_API UScriptStruct* Z_Construct_UScriptStruct_FAimAssistFilter();
	SHOOTERCORERUNTIME_API UScriptStruct* Z_Construct_UScriptStruct_FAimAssistSettings();
	SHOOTERCORERUNTIME_API UScriptStruct* Z_Construct_UScriptStruct_FLyraAimAssistTarget();
	UPackage* Z_Construct_UPackage__Script_ShooterCoreRuntime();
// End Cross Module References
	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_LyraAimAssistTarget;
class UScriptStruct* FLyraAimAssistTarget::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_LyraAimAssistTarget.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_LyraAimAssistTarget.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FLyraAimAssistTarget, (UObject*)Z_Construct_UPackage__Script_ShooterCoreRuntime(), TEXT("LyraAimAssistTarget"));
	}
	return Z_Registration_Info_UScriptStruct_LyraAimAssistTarget.OuterSingleton;
}
template<> SHOOTERCORERUNTIME_API UScriptStruct* StaticStruct<FLyraAimAssistTarget>()
{
	return FLyraAimAssistTarget::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FLyraAimAssistTarget_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FLyraAimAssistTarget_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** A container for keeping the state of targets between frames that can be cached */" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "A container for keeping the state of targets between frames that can be cached" },
#endif
	};
#endif
	void* Z_Construct_UScriptStruct_FLyraAimAssistTarget_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FLyraAimAssistTarget>();
	}
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FLyraAimAssistTarget_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_ShooterCoreRuntime,
		nullptr,
		&NewStructOps,
		"LyraAimAssistTarget",
		nullptr,
		0,
		sizeof(FLyraAimAssistTarget),
		alignof(FLyraAimAssistTarget),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FLyraAimAssistTarget_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FLyraAimAssistTarget_Statics::Struct_MetaDataParams)
	};
	UScriptStruct* Z_Construct_UScriptStruct_FLyraAimAssistTarget()
	{
		if (!Z_Registration_Info_UScriptStruct_LyraAimAssistTarget.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_LyraAimAssistTarget.InnerSingleton, Z_Construct_UScriptStruct_FLyraAimAssistTarget_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_LyraAimAssistTarget.InnerSingleton;
	}
	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_AimAssistFilter;
class UScriptStruct* FAimAssistFilter::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_AimAssistFilter.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_AimAssistFilter.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FAimAssistFilter, (UObject*)Z_Construct_UPackage__Script_ShooterCoreRuntime(), TEXT("AimAssistFilter"));
	}
	return Z_Registration_Info_UScriptStruct_AimAssistFilter.OuterSingleton;
}
template<> SHOOTERCORERUNTIME_API UScriptStruct* StaticStruct<FAimAssistFilter>()
{
	return FAimAssistFilter::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FAimAssistFilter_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bIncludeSameFriendlyTargets_MetaData[];
#endif
		static void NewProp_bIncludeSameFriendlyTargets_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bIncludeSameFriendlyTargets;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bExcludeInstigator_MetaData[];
#endif
		static void NewProp_bExcludeInstigator_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bExcludeInstigator;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bExcludeAllAttachedToInstigator_MetaData[];
#endif
		static void NewProp_bExcludeAllAttachedToInstigator_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bExcludeAllAttachedToInstigator;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bExcludeRequester_MetaData[];
#endif
		static void NewProp_bExcludeRequester_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bExcludeRequester;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bExcludeAllAttachedToRequester_MetaData[];
#endif
		static void NewProp_bExcludeAllAttachedToRequester_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bExcludeAllAttachedToRequester;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bTraceComplexCollision_MetaData[];
#endif
		static void NewProp_bTraceComplexCollision_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bTraceComplexCollision;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bExcludeDeadOrDying_MetaData[];
#endif
		static void NewProp_bExcludeDeadOrDying_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bExcludeDeadOrDying;
		static const UECodeGen_Private::FClassPtrPropertyParams NewProp_ExcludedClasses_ElementProp;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_ExcludedClasses_MetaData[];
#endif
		static const UECodeGen_Private::FSetPropertyParams NewProp_ExcludedClasses;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_TargetRange_MetaData[];
#endif
		static const UECodeGen_Private::FDoublePropertyParams NewProp_TargetRange;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistFilter_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Options for filtering out certain aim assist targets */" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Options for filtering out certain aim assist targets" },
#endif
	};
#endif
	void* Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FAimAssistFilter>();
	}
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bIncludeSameFriendlyTargets_MetaData[] = {
		{ "Category", "AimAssistFilter" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** If true, then we should include any targets even if they are on our team */" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "If true, then we should include any targets even if they are on our team" },
#endif
	};
#endif
	void Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bIncludeSameFriendlyTargets_SetBit(void* Obj)
	{
		((FAimAssistFilter*)Obj)->bIncludeSameFriendlyTargets = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bIncludeSameFriendlyTargets = { "bIncludeSameFriendlyTargets", nullptr, (EPropertyFlags)0x0010000000000015, UECodeGen_Private::EPropertyGenFlags::Bool , RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(uint8), sizeof(FAimAssistFilter), &Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bIncludeSameFriendlyTargets_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bIncludeSameFriendlyTargets_MetaData), Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bIncludeSameFriendlyTargets_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeInstigator_MetaData[] = {
		{ "Category", "TargetSelection" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Exclude 'RequestedBy->Instigator' Actor */" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Exclude 'RequestedBy->Instigator' Actor" },
#endif
	};
#endif
	void Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeInstigator_SetBit(void* Obj)
	{
		((FAimAssistFilter*)Obj)->bExcludeInstigator = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeInstigator = { "bExcludeInstigator", nullptr, (EPropertyFlags)0x0010000000000015, UECodeGen_Private::EPropertyGenFlags::Bool , RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(uint8), sizeof(FAimAssistFilter), &Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeInstigator_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeInstigator_MetaData), Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeInstigator_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeAllAttachedToInstigator_MetaData[] = {
		{ "Category", "TargetSelection" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Exclude all actors attached to 'RequestedBy->Instigator' Actor */" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Exclude all actors attached to 'RequestedBy->Instigator' Actor" },
#endif
	};
#endif
	void Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeAllAttachedToInstigator_SetBit(void* Obj)
	{
		((FAimAssistFilter*)Obj)->bExcludeAllAttachedToInstigator = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeAllAttachedToInstigator = { "bExcludeAllAttachedToInstigator", nullptr, (EPropertyFlags)0x0010000000000015, UECodeGen_Private::EPropertyGenFlags::Bool , RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(uint8), sizeof(FAimAssistFilter), &Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeAllAttachedToInstigator_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeAllAttachedToInstigator_MetaData), Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeAllAttachedToInstigator_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeRequester_MetaData[] = {
		{ "Category", "TargetSelection" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Exclude 'RequestedBy Actor */" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Exclude 'RequestedBy Actor" },
#endif
	};
#endif
	void Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeRequester_SetBit(void* Obj)
	{
		((FAimAssistFilter*)Obj)->bExcludeRequester = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeRequester = { "bExcludeRequester", nullptr, (EPropertyFlags)0x0010000000000015, UECodeGen_Private::EPropertyGenFlags::Bool , RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(uint8), sizeof(FAimAssistFilter), &Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeRequester_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeRequester_MetaData), Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeRequester_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeAllAttachedToRequester_MetaData[] = {
		{ "Category", "TargetSelection" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Exclude all actors attached to 'RequestedBy Actor */" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Exclude all actors attached to 'RequestedBy Actor" },
#endif
	};
#endif
	void Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeAllAttachedToRequester_SetBit(void* Obj)
	{
		((FAimAssistFilter*)Obj)->bExcludeAllAttachedToRequester = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeAllAttachedToRequester = { "bExcludeAllAttachedToRequester", nullptr, (EPropertyFlags)0x0010000000000015, UECodeGen_Private::EPropertyGenFlags::Bool , RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(uint8), sizeof(FAimAssistFilter), &Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeAllAttachedToRequester_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeAllAttachedToRequester_MetaData), Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeAllAttachedToRequester_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bTraceComplexCollision_MetaData[] = {
		{ "Category", "TargetSelection" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Trace against complex collision. */" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Trace against complex collision." },
#endif
	};
#endif
	void Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bTraceComplexCollision_SetBit(void* Obj)
	{
		((FAimAssistFilter*)Obj)->bTraceComplexCollision = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bTraceComplexCollision = { "bTraceComplexCollision", nullptr, (EPropertyFlags)0x0010000000000015, UECodeGen_Private::EPropertyGenFlags::Bool , RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(uint8), sizeof(FAimAssistFilter), &Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bTraceComplexCollision_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bTraceComplexCollision_MetaData), Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bTraceComplexCollision_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeDeadOrDying_MetaData[] = {
		{ "Category", "TargetSelection" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Exclude all dead or dying targets */" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Exclude all dead or dying targets" },
#endif
	};
#endif
	void Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeDeadOrDying_SetBit(void* Obj)
	{
		((FAimAssistFilter*)Obj)->bExcludeDeadOrDying = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeDeadOrDying = { "bExcludeDeadOrDying", nullptr, (EPropertyFlags)0x0010000000000015, UECodeGen_Private::EPropertyGenFlags::Bool , RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(uint8), sizeof(FAimAssistFilter), &Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeDeadOrDying_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeDeadOrDying_MetaData), Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeDeadOrDying_MetaData) };
	const UECodeGen_Private::FClassPtrPropertyParams Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_ExcludedClasses_ElementProp = { "ExcludedClasses", nullptr, (EPropertyFlags)0x0004000000000001, UECodeGen_Private::EPropertyGenFlags::Class | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_UClass, Z_Construct_UClass_UObject_NoRegister, METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_ExcludedClasses_MetaData[] = {
		{ "Category", "AimAssistFilter" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Any target whose owning actor is of this type will be excluded. */" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Any target whose owning actor is of this type will be excluded." },
#endif
	};
#endif
	const UECodeGen_Private::FSetPropertyParams Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_ExcludedClasses = { "ExcludedClasses", nullptr, (EPropertyFlags)0x0014000000000015, UECodeGen_Private::EPropertyGenFlags::Set, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistFilter, ExcludedClasses), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_ExcludedClasses_MetaData), Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_ExcludedClasses_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_TargetRange_MetaData[] = {
		{ "Category", "AimAssistFilter" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Any target outside of this range will be excluded */" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Any target outside of this range will be excluded" },
#endif
	};
#endif
	const UECodeGen_Private::FDoublePropertyParams Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_TargetRange = { "TargetRange", nullptr, (EPropertyFlags)0x0010000000000015, UECodeGen_Private::EPropertyGenFlags::Double, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistFilter, TargetRange), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_TargetRange_MetaData), Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_TargetRange_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FAimAssistFilter_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bIncludeSameFriendlyTargets,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeInstigator,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeAllAttachedToInstigator,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeRequester,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeAllAttachedToRequester,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bTraceComplexCollision,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_bExcludeDeadOrDying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_ExcludedClasses_ElementProp,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_ExcludedClasses,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewProp_TargetRange,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FAimAssistFilter_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_ShooterCoreRuntime,
		nullptr,
		&NewStructOps,
		"AimAssistFilter",
		Z_Construct_UScriptStruct_FAimAssistFilter_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistFilter_Statics::PropPointers),
		sizeof(FAimAssistFilter),
		alignof(FAimAssistFilter),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistFilter_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FAimAssistFilter_Statics::Struct_MetaDataParams)
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistFilter_Statics::PropPointers) < 2048);
	UScriptStruct* Z_Construct_UScriptStruct_FAimAssistFilter()
	{
		if (!Z_Registration_Info_UScriptStruct_AimAssistFilter.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_AimAssistFilter.InnerSingleton, Z_Construct_UScriptStruct_FAimAssistFilter_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_AimAssistFilter.InnerSingleton;
	}
	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_AimAssistSettings;
class UScriptStruct* FAimAssistSettings::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_AimAssistSettings.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_AimAssistSettings.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FAimAssistSettings, (UObject*)Z_Construct_UPackage__Script_ShooterCoreRuntime(), TEXT("AimAssistSettings"));
	}
	return Z_Registration_Info_UScriptStruct_AimAssistSettings.OuterSingleton;
}
template<> SHOOTERCORERUNTIME_API UScriptStruct* StaticStruct<FAimAssistSettings>()
{
	return FAimAssistSettings::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FAimAssistSettings_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_AssistInnerReticleWidth_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_AssistInnerReticleWidth;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_AssistInnerReticleHeight_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_AssistInnerReticleHeight;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_AssistOuterReticleWidth_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_AssistOuterReticleWidth;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_AssistOuterReticleHeight_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_AssistOuterReticleHeight;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_TargetingReticleWidth_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_TargetingReticleWidth;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_TargetingReticleHeight_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_TargetingReticleHeight;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_TargetRange_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_TargetRange;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_TargetWeightCurve_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPtrPropertyParams NewProp_TargetWeightCurve;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PullInnerStrengthHip_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_PullInnerStrengthHip;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PullOuterStrengthHip_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_PullOuterStrengthHip;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PullInnerStrengthAds_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_PullInnerStrengthAds;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PullOuterStrengthAds_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_PullOuterStrengthAds;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PullLerpInRate_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_PullLerpInRate;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PullLerpOutRate_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_PullLerpOutRate;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PullMaxRotationRate_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_PullMaxRotationRate;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SlowInnerStrengthHip_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_SlowInnerStrengthHip;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SlowOuterStrengthHip_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_SlowOuterStrengthHip;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SlowInnerStrengthAds_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_SlowInnerStrengthAds;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SlowOuterStrengthAds_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_SlowOuterStrengthAds;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SlowLerpInRate_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_SlowLerpInRate;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SlowLerpOutRate_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_SlowLerpOutRate;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SlowMinRotationRate_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_SlowMinRotationRate;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_MaxNumberOfTargets_MetaData[];
#endif
		static const UECodeGen_Private::FIntPropertyParams NewProp_MaxNumberOfTargets;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_ReticleDepth_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_ReticleDepth;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_TargetScore_AssistWeight_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_TargetScore_AssistWeight;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_TargetScore_ViewDot_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_TargetScore_ViewDot;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_TargetScore_ViewDotOffset_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_TargetScore_ViewDotOffset;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_TargetScore_ViewDistance_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_TargetScore_ViewDistance;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StrengthScale_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_StrengthScale;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bEnableAsyncVisibilityTrace_MetaData[];
#endif
		static void NewProp_bEnableAsyncVisibilityTrace_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bEnableAsyncVisibilityTrace;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bRequireInput_MetaData[];
#endif
		static void NewProp_bRequireInput_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bRequireInput;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bApplyPull_MetaData[];
#endif
		static void NewProp_bApplyPull_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bApplyPull;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bApplyStrafePullScale_MetaData[];
#endif
		static void NewProp_bApplyStrafePullScale_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bApplyStrafePullScale;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bApplySlowing_MetaData[];
#endif
		static void NewProp_bApplySlowing_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bApplySlowing;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bUseDynamicSlow_MetaData[];
#endif
		static void NewProp_bUseDynamicSlow_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bUseDynamicSlow;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bUseRadialLookRates_MetaData[];
#endif
		static void NewProp_bUseRadialLookRates_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bUseRadialLookRates;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Settings for how aim assist should behave when there are active targets */" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Settings for how aim assist should behave when there are active targets" },
#endif
	};
#endif
	void* Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FAimAssistSettings>();
	}
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_AssistInnerReticleWidth_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Width of aim assist inner reticle in world space.\n" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Width of aim assist inner reticle in world space." },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_AssistInnerReticleWidth = { "AssistInnerReticleWidth", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, AssistInnerReticleWidth), Z_Construct_UScriptStruct_FScalableFloat, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_AssistInnerReticleWidth_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_AssistInnerReticleWidth_MetaData) }; // 4070660376
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_AssistInnerReticleHeight_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Height of aim assist inner reticle in world space.\n" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Height of aim assist inner reticle in world space." },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_AssistInnerReticleHeight = { "AssistInnerReticleHeight", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, AssistInnerReticleHeight), Z_Construct_UScriptStruct_FScalableFloat, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_AssistInnerReticleHeight_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_AssistInnerReticleHeight_MetaData) }; // 4070660376
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_AssistOuterReticleWidth_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Width of aim assist outer reticle in world space.\n" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Width of aim assist outer reticle in world space." },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_AssistOuterReticleWidth = { "AssistOuterReticleWidth", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, AssistOuterReticleWidth), Z_Construct_UScriptStruct_FScalableFloat, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_AssistOuterReticleWidth_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_AssistOuterReticleWidth_MetaData) }; // 4070660376
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_AssistOuterReticleHeight_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Height of aim assist outer reticle in world space.\n" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Height of aim assist outer reticle in world space." },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_AssistOuterReticleHeight = { "AssistOuterReticleHeight", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, AssistOuterReticleHeight), Z_Construct_UScriptStruct_FScalableFloat, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_AssistOuterReticleHeight_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_AssistOuterReticleHeight_MetaData) }; // 4070660376
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetingReticleWidth_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Width of targeting reticle in world space.\n" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Width of targeting reticle in world space." },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetingReticleWidth = { "TargetingReticleWidth", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, TargetingReticleWidth), Z_Construct_UScriptStruct_FScalableFloat, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetingReticleWidth_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetingReticleWidth_MetaData) }; // 4070660376
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetingReticleHeight_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Height of targeting reticle in world space.\n" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Height of targeting reticle in world space." },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetingReticleHeight = { "TargetingReticleHeight", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, TargetingReticleHeight), Z_Construct_UScriptStruct_FScalableFloat, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetingReticleHeight_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetingReticleHeight_MetaData) }; // 4070660376
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetRange_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Range from player's camera used to gather potential targets.\n// Note: This is scaled using the field of view in order to limit targets by their screen size.\n" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Range from player's camera used to gather potential targets.\nNote: This is scaled using the field of view in order to limit targets by their screen size." },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetRange = { "TargetRange", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, TargetRange), Z_Construct_UScriptStruct_FScalableFloat, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetRange_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetRange_MetaData) }; // 4070660376
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetWeightCurve_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// How much weight the target has based on the time it has been targeted.  (0 = None, 1 = Max)\n" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
		{ "NativeConstTemplateArg", "" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "How much weight the target has based on the time it has been targeted.  (0 = None, 1 = Max)" },
#endif
	};
#endif
	const UECodeGen_Private::FObjectPtrPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetWeightCurve = { "TargetWeightCurve", nullptr, (EPropertyFlags)0x0014000000000001, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, TargetWeightCurve), Z_Construct_UClass_UCurveFloat_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetWeightCurve_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetWeightCurve_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullInnerStrengthHip_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// How much target and player movement contributes to the aim assist pull when target is under the inner reticle. (0 = None, 1 = Max)\n" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "How much target and player movement contributes to the aim assist pull when target is under the inner reticle. (0 = None, 1 = Max)" },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullInnerStrengthHip = { "PullInnerStrengthHip", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, PullInnerStrengthHip), Z_Construct_UScriptStruct_FScalableFloat, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullInnerStrengthHip_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullInnerStrengthHip_MetaData) }; // 4070660376
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullOuterStrengthHip_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// How much target and player movement contributes to the aim assist pull when target is under the outer reticle. (0 = None, 1 = Max)\n" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "How much target and player movement contributes to the aim assist pull when target is under the outer reticle. (0 = None, 1 = Max)" },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullOuterStrengthHip = { "PullOuterStrengthHip", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, PullOuterStrengthHip), Z_Construct_UScriptStruct_FScalableFloat, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullOuterStrengthHip_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullOuterStrengthHip_MetaData) }; // 4070660376
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullInnerStrengthAds_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// How much target and player movement contributes to the aim assist pull when target is under the inner reticle. (0 = None, 1 = Max)\n" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "How much target and player movement contributes to the aim assist pull when target is under the inner reticle. (0 = None, 1 = Max)" },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullInnerStrengthAds = { "PullInnerStrengthAds", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, PullInnerStrengthAds), Z_Construct_UScriptStruct_FScalableFloat, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullInnerStrengthAds_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullInnerStrengthAds_MetaData) }; // 4070660376
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullOuterStrengthAds_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// How much target and player movement contributes to the aim assist pull when target is under the outer reticle. (0 = None, 1 = Max)\n" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "How much target and player movement contributes to the aim assist pull when target is under the outer reticle. (0 = None, 1 = Max)" },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullOuterStrengthAds = { "PullOuterStrengthAds", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, PullOuterStrengthAds), Z_Construct_UScriptStruct_FScalableFloat, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullOuterStrengthAds_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullOuterStrengthAds_MetaData) }; // 4070660376
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullLerpInRate_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Exponential interpolation rate used to ramp up the pull strength.  Set to '0' to disable.\n" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Exponential interpolation rate used to ramp up the pull strength.  Set to '0' to disable." },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullLerpInRate = { "PullLerpInRate", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, PullLerpInRate), Z_Construct_UScriptStruct_FScalableFloat, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullLerpInRate_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullLerpInRate_MetaData) }; // 4070660376
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullLerpOutRate_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Exponential interpolation rate used to ramp down the pull strength.  Set to '0' to disable.\n" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Exponential interpolation rate used to ramp down the pull strength.  Set to '0' to disable." },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullLerpOutRate = { "PullLerpOutRate", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, PullLerpOutRate), Z_Construct_UScriptStruct_FScalableFloat, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullLerpOutRate_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullLerpOutRate_MetaData) }; // 4070660376
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullMaxRotationRate_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Rotation rate maximum cap on amount of aim assist pull.  Set to '0' to disable.\n// Note: This is scaled based on the field of view so it feels the same regardless of zoom.\n" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Rotation rate maximum cap on amount of aim assist pull.  Set to '0' to disable.\nNote: This is scaled based on the field of view so it feels the same regardless of zoom." },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullMaxRotationRate = { "PullMaxRotationRate", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, PullMaxRotationRate), Z_Construct_UScriptStruct_FScalableFloat, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullMaxRotationRate_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullMaxRotationRate_MetaData) }; // 4070660376
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowInnerStrengthHip_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Amount of aim assist slow applied to desired turn rate when target is under the inner reticle. (0 = None, 1 = Max)\n" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Amount of aim assist slow applied to desired turn rate when target is under the inner reticle. (0 = None, 1 = Max)" },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowInnerStrengthHip = { "SlowInnerStrengthHip", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, SlowInnerStrengthHip), Z_Construct_UScriptStruct_FScalableFloat, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowInnerStrengthHip_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowInnerStrengthHip_MetaData) }; // 4070660376
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowOuterStrengthHip_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Amount of aim assist slow applied to desired turn rate when target is under the outer reticle. (0 = None, 1 = Max)\n" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Amount of aim assist slow applied to desired turn rate when target is under the outer reticle. (0 = None, 1 = Max)" },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowOuterStrengthHip = { "SlowOuterStrengthHip", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, SlowOuterStrengthHip), Z_Construct_UScriptStruct_FScalableFloat, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowOuterStrengthHip_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowOuterStrengthHip_MetaData) }; // 4070660376
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowInnerStrengthAds_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Amount of aim assist slow applied to desired turn rate when target is under the inner reticle. (0 = None, 1 = Max)\n" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Amount of aim assist slow applied to desired turn rate when target is under the inner reticle. (0 = None, 1 = Max)" },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowInnerStrengthAds = { "SlowInnerStrengthAds", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, SlowInnerStrengthAds), Z_Construct_UScriptStruct_FScalableFloat, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowInnerStrengthAds_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowInnerStrengthAds_MetaData) }; // 4070660376
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowOuterStrengthAds_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Amount of aim assist slow applied to desired turn rate when target is under the outer reticle. (0 = None, 1 = Max)\n" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Amount of aim assist slow applied to desired turn rate when target is under the outer reticle. (0 = None, 1 = Max)" },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowOuterStrengthAds = { "SlowOuterStrengthAds", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, SlowOuterStrengthAds), Z_Construct_UScriptStruct_FScalableFloat, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowOuterStrengthAds_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowOuterStrengthAds_MetaData) }; // 4070660376
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowLerpInRate_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Exponential interpolation rate used to ramp up the slow strength.  Set to '0' to disable.\n" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Exponential interpolation rate used to ramp up the slow strength.  Set to '0' to disable." },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowLerpInRate = { "SlowLerpInRate", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, SlowLerpInRate), Z_Construct_UScriptStruct_FScalableFloat, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowLerpInRate_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowLerpInRate_MetaData) }; // 4070660376
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowLerpOutRate_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Exponential interpolation rate used to ramp down the slow strength.  Set to '0' to disable.\n" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Exponential interpolation rate used to ramp down the slow strength.  Set to '0' to disable." },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowLerpOutRate = { "SlowLerpOutRate", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, SlowLerpOutRate), Z_Construct_UScriptStruct_FScalableFloat, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowLerpOutRate_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowLerpOutRate_MetaData) }; // 4070660376
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowMinRotationRate_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Rotation rate minimum cap on amount to aim assist slow.  Set to '0' to disable.\n// Note: This is scaled based on the field of view so it feels the same regardless of zoom.\n" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Rotation rate minimum cap on amount to aim assist slow.  Set to '0' to disable.\nNote: This is scaled based on the field of view so it feels the same regardless of zoom." },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowMinRotationRate = { "SlowMinRotationRate", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, SlowMinRotationRate), Z_Construct_UScriptStruct_FScalableFloat, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowMinRotationRate_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowMinRotationRate_MetaData) }; // 4070660376
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_MaxNumberOfTargets_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** The maximum number of targets that can be considered during a given frame. */" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The maximum number of targets that can be considered during a given frame." },
#endif
	};
#endif
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_MaxNumberOfTargets = { "MaxNumberOfTargets", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, MaxNumberOfTargets), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_MaxNumberOfTargets_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_MaxNumberOfTargets_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_ReticleDepth_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**  */" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_ReticleDepth = { "ReticleDepth", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, ReticleDepth), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_ReticleDepth_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_ReticleDepth_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetScore_AssistWeight_MetaData[] = {
		{ "Category", "AimAssistSettings" },
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetScore_AssistWeight = { "TargetScore_AssistWeight", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, TargetScore_AssistWeight), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetScore_AssistWeight_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetScore_AssistWeight_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetScore_ViewDot_MetaData[] = {
		{ "Category", "AimAssistSettings" },
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetScore_ViewDot = { "TargetScore_ViewDot", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, TargetScore_ViewDot), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetScore_ViewDot_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetScore_ViewDot_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetScore_ViewDotOffset_MetaData[] = {
		{ "Category", "AimAssistSettings" },
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetScore_ViewDotOffset = { "TargetScore_ViewDotOffset", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, TargetScore_ViewDotOffset), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetScore_ViewDotOffset_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetScore_ViewDotOffset_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetScore_ViewDistance_MetaData[] = {
		{ "Category", "AimAssistSettings" },
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetScore_ViewDistance = { "TargetScore_ViewDistance", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, TargetScore_ViewDistance), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetScore_ViewDistance_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetScore_ViewDistance_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_StrengthScale_MetaData[] = {
		{ "Category", "AimAssistSettings" },
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_StrengthScale = { "StrengthScale", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistSettings, StrengthScale), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_StrengthScale_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_StrengthScale_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bEnableAsyncVisibilityTrace_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Enabled/Disable asynchronous visibility traces. */" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Enabled/Disable asynchronous visibility traces." },
#endif
	};
#endif
	void Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bEnableAsyncVisibilityTrace_SetBit(void* Obj)
	{
		((FAimAssistSettings*)Obj)->bEnableAsyncVisibilityTrace = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bEnableAsyncVisibilityTrace = { "bEnableAsyncVisibilityTrace", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool , RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(uint8), sizeof(FAimAssistSettings), &Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bEnableAsyncVisibilityTrace_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bEnableAsyncVisibilityTrace_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bEnableAsyncVisibilityTrace_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bRequireInput_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Whether or not we require input for aim assist to be applied */" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Whether or not we require input for aim assist to be applied" },
#endif
	};
#endif
	void Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bRequireInput_SetBit(void* Obj)
	{
		((FAimAssistSettings*)Obj)->bRequireInput = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bRequireInput = { "bRequireInput", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool , RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(uint8), sizeof(FAimAssistSettings), &Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bRequireInput_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bRequireInput_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bRequireInput_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bApplyPull_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Whether or not pull should be applied to aim assist */" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Whether or not pull should be applied to aim assist" },
#endif
	};
#endif
	void Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bApplyPull_SetBit(void* Obj)
	{
		((FAimAssistSettings*)Obj)->bApplyPull = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bApplyPull = { "bApplyPull", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool , RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(uint8), sizeof(FAimAssistSettings), &Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bApplyPull_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bApplyPull_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bApplyPull_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bApplyStrafePullScale_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Whether or not to apply a strafe pull based off of movement input */" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Whether or not to apply a strafe pull based off of movement input" },
#endif
	};
#endif
	void Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bApplyStrafePullScale_SetBit(void* Obj)
	{
		((FAimAssistSettings*)Obj)->bApplyStrafePullScale = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bApplyStrafePullScale = { "bApplyStrafePullScale", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool , RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(uint8), sizeof(FAimAssistSettings), &Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bApplyStrafePullScale_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bApplyStrafePullScale_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bApplyStrafePullScale_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bApplySlowing_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Whether or not to apply a slowing effect during aim assist */" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Whether or not to apply a slowing effect during aim assist" },
#endif
	};
#endif
	void Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bApplySlowing_SetBit(void* Obj)
	{
		((FAimAssistSettings*)Obj)->bApplySlowing = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bApplySlowing = { "bApplySlowing", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool , RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(uint8), sizeof(FAimAssistSettings), &Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bApplySlowing_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bApplySlowing_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bApplySlowing_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bUseDynamicSlow_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Whether or not to apply a dynamic slow effect based off of look input */" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Whether or not to apply a dynamic slow effect based off of look input" },
#endif
	};
#endif
	void Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bUseDynamicSlow_SetBit(void* Obj)
	{
		((FAimAssistSettings*)Obj)->bUseDynamicSlow = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bUseDynamicSlow = { "bUseDynamicSlow", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool , RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(uint8), sizeof(FAimAssistSettings), &Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bUseDynamicSlow_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bUseDynamicSlow_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bUseDynamicSlow_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bUseRadialLookRates_MetaData[] = {
		{ "Category", "AimAssistSettings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Whether or not look rates should blend between yaw and pitch based on stick deflection using radial look rates */" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Whether or not look rates should blend between yaw and pitch based on stick deflection using radial look rates" },
#endif
	};
#endif
	void Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bUseRadialLookRates_SetBit(void* Obj)
	{
		((FAimAssistSettings*)Obj)->bUseRadialLookRates = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bUseRadialLookRates = { "bUseRadialLookRates", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool , RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(uint8), sizeof(FAimAssistSettings), &Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bUseRadialLookRates_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bUseRadialLookRates_MetaData), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bUseRadialLookRates_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FAimAssistSettings_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_AssistInnerReticleWidth,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_AssistInnerReticleHeight,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_AssistOuterReticleWidth,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_AssistOuterReticleHeight,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetingReticleWidth,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetingReticleHeight,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetRange,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetWeightCurve,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullInnerStrengthHip,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullOuterStrengthHip,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullInnerStrengthAds,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullOuterStrengthAds,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullLerpInRate,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullLerpOutRate,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_PullMaxRotationRate,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowInnerStrengthHip,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowOuterStrengthHip,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowInnerStrengthAds,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowOuterStrengthAds,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowLerpInRate,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowLerpOutRate,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_SlowMinRotationRate,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_MaxNumberOfTargets,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_ReticleDepth,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetScore_AssistWeight,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetScore_ViewDot,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetScore_ViewDotOffset,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_TargetScore_ViewDistance,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_StrengthScale,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bEnableAsyncVisibilityTrace,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bRequireInput,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bApplyPull,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bApplyStrafePullScale,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bApplySlowing,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bUseDynamicSlow,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewProp_bUseRadialLookRates,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FAimAssistSettings_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_ShooterCoreRuntime,
		nullptr,
		&NewStructOps,
		"AimAssistSettings",
		Z_Construct_UScriptStruct_FAimAssistSettings_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::PropPointers),
		sizeof(FAimAssistSettings),
		alignof(FAimAssistSettings),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FAimAssistSettings_Statics::Struct_MetaDataParams)
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistSettings_Statics::PropPointers) < 2048);
	UScriptStruct* Z_Construct_UScriptStruct_FAimAssistSettings()
	{
		if (!Z_Registration_Info_UScriptStruct_AimAssistSettings.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_AimAssistSettings.InnerSingleton, Z_Construct_UScriptStruct_FAimAssistSettings_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_AimAssistSettings.InnerSingleton;
	}
	void UAimAssistInputModifier::StaticRegisterNativesUAimAssistInputModifier()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UAimAssistInputModifier);
	UClass* Z_Construct_UClass_UAimAssistInputModifier_NoRegister()
	{
		return UAimAssistInputModifier::StaticClass();
	}
	struct Z_Construct_UClass_UAimAssistInputModifier_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Settings_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_Settings;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Filter_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_Filter;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_MoveInputAction_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPtrPropertyParams NewProp_MoveInputAction;
		static const UECodeGen_Private::FBytePropertyParams NewProp_TargetingType_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_TargetingType_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_TargetingType;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SensitivityLevelTable_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPtrPropertyParams NewProp_SensitivityLevelTable;
		static const UECodeGen_Private::FStructPropertyParams NewProp_TargetCache0_Inner;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_TargetCache0_MetaData[];
#endif
		static const UECodeGen_Private::FArrayPropertyParams NewProp_TargetCache0;
		static const UECodeGen_Private::FStructPropertyParams NewProp_TargetCache1_Inner;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_TargetCache1_MetaData[];
#endif
		static const UECodeGen_Private::FArrayPropertyParams NewProp_TargetCache1;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UAimAssistInputModifier_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UInputModifier,
		(UObject* (*)())Z_Construct_UPackage__Script_ShooterCoreRuntime,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UAimAssistInputModifier_Statics::DependentSingletons) < 16);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UAimAssistInputModifier_Statics::Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * An input modifier to help gamepad players have better targeting.\n */" },
#endif
		{ "IncludePath", "Input/AimAssistInputModifier.h" },
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "An input modifier to help gamepad players have better targeting." },
#endif
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_Settings_MetaData[] = {
		{ "Category", "Settings" },
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_Settings = { "Settings", nullptr, (EPropertyFlags)0x0010000000004805, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UAimAssistInputModifier, Settings), Z_Construct_UScriptStruct_FAimAssistSettings, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_Settings_MetaData), Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_Settings_MetaData) }; // 2014922237
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_Filter_MetaData[] = {
		{ "Category", "Settings" },
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_Filter = { "Filter", nullptr, (EPropertyFlags)0x0010000000004805, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UAimAssistInputModifier, Filter), Z_Construct_UScriptStruct_FAimAssistFilter, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_Filter_MetaData), Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_Filter_MetaData) }; // 3045985027
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_MoveInputAction_MetaData[] = {
		{ "Category", "Settings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** The input action that represents the actual movement of the player */" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
		{ "NativeConstTemplateArg", "" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The input action that represents the actual movement of the player" },
#endif
	};
#endif
	const UECodeGen_Private::FObjectPtrPropertyParams Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_MoveInputAction = { "MoveInputAction", nullptr, (EPropertyFlags)0x0014000000000805, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UAimAssistInputModifier, MoveInputAction), Z_Construct_UClass_UInputAction_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_MoveInputAction_MetaData), Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_MoveInputAction_MetaData) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_TargetingType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_TargetingType_MetaData[] = {
		{ "Category", "Settings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** The type of targeting to use for this Sensitivity */" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The type of targeting to use for this Sensitivity" },
#endif
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_TargetingType = { "TargetingType", nullptr, (EPropertyFlags)0x0010000000004805, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UAimAssistInputModifier, TargetingType), Z_Construct_UEnum_Mythic_ELyraTargetingType, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_TargetingType_MetaData), Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_TargetingType_MetaData) }; // 83134651
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_SensitivityLevelTable_MetaData[] = {
		{ "AssetBundles", "Client,Server" },
		{ "Category", "AimAssistInputModifier" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Asset that gives us access to the float scalar value being used for sensitivty */" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
		{ "NativeConstTemplateArg", "" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Asset that gives us access to the float scalar value being used for sensitivty" },
#endif
	};
#endif
	const UECodeGen_Private::FObjectPtrPropertyParams Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_SensitivityLevelTable = { "SensitivityLevelTable", nullptr, (EPropertyFlags)0x0014000000000015, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UAimAssistInputModifier, SensitivityLevelTable), Z_Construct_UClass_ULyraAimSensitivityData_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_SensitivityLevelTable_MetaData), Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_SensitivityLevelTable_MetaData) };
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_TargetCache0_Inner = { "TargetCache0", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FLyraAimAssistTarget, METADATA_PARAMS(0, nullptr) }; // 495039271
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_TargetCache0_MetaData[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Tracking of the current and previous frame's targets\n" },
#endif
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Tracking of the current and previous frame's targets" },
#endif
	};
#endif
	const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_TargetCache0 = { "TargetCache0", nullptr, (EPropertyFlags)0x0020080000000000, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UAimAssistInputModifier, TargetCache0), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_TargetCache0_MetaData), Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_TargetCache0_MetaData) }; // 495039271
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_TargetCache1_Inner = { "TargetCache1", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FLyraAimAssistTarget, METADATA_PARAMS(0, nullptr) }; // 495039271
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_TargetCache1_MetaData[] = {
		{ "ModuleRelativePath", "Public/Input/AimAssistInputModifier.h" },
	};
#endif
	const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_TargetCache1 = { "TargetCache1", nullptr, (EPropertyFlags)0x0020080000000000, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UAimAssistInputModifier, TargetCache1), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_TargetCache1_MetaData), Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_TargetCache1_MetaData) }; // 495039271
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UAimAssistInputModifier_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_Settings,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_Filter,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_MoveInputAction,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_TargetingType_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_TargetingType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_SensitivityLevelTable,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_TargetCache0_Inner,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_TargetCache0,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_TargetCache1_Inner,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAimAssistInputModifier_Statics::NewProp_TargetCache1,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_UAimAssistInputModifier_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UAimAssistInputModifier>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UAimAssistInputModifier_Statics::ClassParams = {
		&UAimAssistInputModifier::StaticClass,
		"Input",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_UAimAssistInputModifier_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_UAimAssistInputModifier_Statics::PropPointers),
		0,
		0x400030A6u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UAimAssistInputModifier_Statics::Class_MetaDataParams), Z_Construct_UClass_UAimAssistInputModifier_Statics::Class_MetaDataParams)
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UAimAssistInputModifier_Statics::PropPointers) < 2048);
	UClass* Z_Construct_UClass_UAimAssistInputModifier()
	{
		if (!Z_Registration_Info_UClass_UAimAssistInputModifier.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UAimAssistInputModifier.OuterSingleton, Z_Construct_UClass_UAimAssistInputModifier_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UAimAssistInputModifier.OuterSingleton;
	}
	template<> SHOOTERCORERUNTIME_API UClass* StaticClass<UAimAssistInputModifier>()
	{
		return UAimAssistInputModifier::StaticClass();
	}
	UAimAssistInputModifier::UAimAssistInputModifier(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UAimAssistInputModifier);
	UAimAssistInputModifier::~UAimAssistInputModifier() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_Input_AimAssistInputModifier_h_Statics
	{
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_Input_AimAssistInputModifier_h_Statics::ScriptStructInfo[] = {
		{ FLyraAimAssistTarget::StaticStruct, Z_Construct_UScriptStruct_FLyraAimAssistTarget_Statics::NewStructOps, TEXT("LyraAimAssistTarget"), &Z_Registration_Info_UScriptStruct_LyraAimAssistTarget, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FLyraAimAssistTarget), 495039271U) },
		{ FAimAssistFilter::StaticStruct, Z_Construct_UScriptStruct_FAimAssistFilter_Statics::NewStructOps, TEXT("AimAssistFilter"), &Z_Registration_Info_UScriptStruct_AimAssistFilter, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FAimAssistFilter), 3045985027U) },
		{ FAimAssistSettings::StaticStruct, Z_Construct_UScriptStruct_FAimAssistSettings_Statics::NewStructOps, TEXT("AimAssistSettings"), &Z_Registration_Info_UScriptStruct_AimAssistSettings, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FAimAssistSettings), 2014922237U) },
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_Input_AimAssistInputModifier_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UAimAssistInputModifier, UAimAssistInputModifier::StaticClass, TEXT("UAimAssistInputModifier"), &Z_Registration_Info_UClass_UAimAssistInputModifier, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UAimAssistInputModifier), 3061818895U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_Input_AimAssistInputModifier_h_3090293981(TEXT("/Script/ShooterCoreRuntime"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_Input_AimAssistInputModifier_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_Input_AimAssistInputModifier_h_Statics::ClassInfo),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_Input_AimAssistInputModifier_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_Input_AimAssistInputModifier_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
