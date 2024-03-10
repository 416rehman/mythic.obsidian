// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/PocketCapture.h"
#include "Public/PocketCaptureSubsystem.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodePocketCapture() {}
// Cross Module References
	COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
	ENGINE_API UClass* Z_Construct_UClass_AActor_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_UMaterialInterface_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_USceneCaptureComponent2D_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_UTextureRenderTarget2D_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_UWorld_NoRegister();
	POCKETWORLDS_API UClass* Z_Construct_UClass_UPocketCapture();
	POCKETWORLDS_API UClass* Z_Construct_UClass_UPocketCapture_NoRegister();
	UPackage* Z_Construct_UPackage__Script_PocketWorlds();
// End Cross Module References
	DEFINE_FUNCTION(UPocketCapture::execGetRendererIndex)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		*(int32*)Z_Param__Result=P_THIS->GetRendererIndex();
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UPocketCapture::execReclaimResources)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->ReclaimResources();
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UPocketCapture::execReleaseResources)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->ReleaseResources();
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UPocketCapture::execCaptureEffects)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->CaptureEffects();
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UPocketCapture::execCaptureAlphaMask)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->CaptureAlphaMask();
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UPocketCapture::execCaptureDiffuse)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->CaptureDiffuse();
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UPocketCapture::execSetAlphaMaskedActors)
	{
		P_GET_TARRAY_REF(AActor*,Z_Param_Out_InCaptureTarget);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->SetAlphaMaskedActors(Z_Param_Out_InCaptureTarget);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UPocketCapture::execSetCaptureTarget)
	{
		P_GET_OBJECT(AActor,Z_Param_InCaptureTarget);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->SetCaptureTarget(Z_Param_InCaptureTarget);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UPocketCapture::execGetOrCreateEffectsRenderTarget)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		*(UTextureRenderTarget2D**)Z_Param__Result=P_THIS->GetOrCreateEffectsRenderTarget();
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UPocketCapture::execGetOrCreateAlphaMaskRenderTarget)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		*(UTextureRenderTarget2D**)Z_Param__Result=P_THIS->GetOrCreateAlphaMaskRenderTarget();
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UPocketCapture::execGetOrCreateDiffuseRenderTarget)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		*(UTextureRenderTarget2D**)Z_Param__Result=P_THIS->GetOrCreateDiffuseRenderTarget();
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UPocketCapture::execSetRenderTargetSize)
	{
		P_GET_PROPERTY(FIntProperty,Z_Param_Width);
		P_GET_PROPERTY(FIntProperty,Z_Param_Height);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->SetRenderTargetSize(Z_Param_Width,Z_Param_Height);
		P_NATIVE_END;
	}
	void UPocketCapture::StaticRegisterNativesUPocketCapture()
	{
		UClass* Class = UPocketCapture::StaticClass();
		static const FNameNativePtrPair Funcs[] = {
			{ "CaptureAlphaMask", &UPocketCapture::execCaptureAlphaMask },
			{ "CaptureDiffuse", &UPocketCapture::execCaptureDiffuse },
			{ "CaptureEffects", &UPocketCapture::execCaptureEffects },
			{ "GetOrCreateAlphaMaskRenderTarget", &UPocketCapture::execGetOrCreateAlphaMaskRenderTarget },
			{ "GetOrCreateDiffuseRenderTarget", &UPocketCapture::execGetOrCreateDiffuseRenderTarget },
			{ "GetOrCreateEffectsRenderTarget", &UPocketCapture::execGetOrCreateEffectsRenderTarget },
			{ "GetRendererIndex", &UPocketCapture::execGetRendererIndex },
			{ "ReclaimResources", &UPocketCapture::execReclaimResources },
			{ "ReleaseResources", &UPocketCapture::execReleaseResources },
			{ "SetAlphaMaskedActors", &UPocketCapture::execSetAlphaMaskedActors },
			{ "SetCaptureTarget", &UPocketCapture::execSetCaptureTarget },
			{ "SetRenderTargetSize", &UPocketCapture::execSetRenderTargetSize },
		};
		FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
	}
	struct Z_Construct_UFunction_UPocketCapture_CaptureAlphaMask_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UPocketCapture_CaptureAlphaMask_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/PocketCapture.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UPocketCapture_CaptureAlphaMask_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UPocketCapture, nullptr, "CaptureAlphaMask", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCapture_CaptureAlphaMask_Statics::Function_MetaDataParams), Z_Construct_UFunction_UPocketCapture_CaptureAlphaMask_Statics::Function_MetaDataParams) };
	UFunction* Z_Construct_UFunction_UPocketCapture_CaptureAlphaMask()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UPocketCapture_CaptureAlphaMask_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UPocketCapture_CaptureDiffuse_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UPocketCapture_CaptureDiffuse_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/PocketCapture.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UPocketCapture_CaptureDiffuse_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UPocketCapture, nullptr, "CaptureDiffuse", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCapture_CaptureDiffuse_Statics::Function_MetaDataParams), Z_Construct_UFunction_UPocketCapture_CaptureDiffuse_Statics::Function_MetaDataParams) };
	UFunction* Z_Construct_UFunction_UPocketCapture_CaptureDiffuse()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UPocketCapture_CaptureDiffuse_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UPocketCapture_CaptureEffects_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UPocketCapture_CaptureEffects_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/PocketCapture.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UPocketCapture_CaptureEffects_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UPocketCapture, nullptr, "CaptureEffects", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCapture_CaptureEffects_Statics::Function_MetaDataParams), Z_Construct_UFunction_UPocketCapture_CaptureEffects_Statics::Function_MetaDataParams) };
	UFunction* Z_Construct_UFunction_UPocketCapture_CaptureEffects()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UPocketCapture_CaptureEffects_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UPocketCapture_GetOrCreateAlphaMaskRenderTarget_Statics
	{
		struct PocketCapture_eventGetOrCreateAlphaMaskRenderTarget_Parms
		{
			UTextureRenderTarget2D* ReturnValue;
		};
		static const UECodeGen_Private::FObjectPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UPocketCapture_GetOrCreateAlphaMaskRenderTarget_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(PocketCapture_eventGetOrCreateAlphaMaskRenderTarget_Parms, ReturnValue), Z_Construct_UClass_UTextureRenderTarget2D_NoRegister, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UPocketCapture_GetOrCreateAlphaMaskRenderTarget_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UPocketCapture_GetOrCreateAlphaMaskRenderTarget_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UPocketCapture_GetOrCreateAlphaMaskRenderTarget_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/PocketCapture.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UPocketCapture_GetOrCreateAlphaMaskRenderTarget_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UPocketCapture, nullptr, "GetOrCreateAlphaMaskRenderTarget", nullptr, nullptr, Z_Construct_UFunction_UPocketCapture_GetOrCreateAlphaMaskRenderTarget_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCapture_GetOrCreateAlphaMaskRenderTarget_Statics::PropPointers), sizeof(Z_Construct_UFunction_UPocketCapture_GetOrCreateAlphaMaskRenderTarget_Statics::PocketCapture_eventGetOrCreateAlphaMaskRenderTarget_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCapture_GetOrCreateAlphaMaskRenderTarget_Statics::Function_MetaDataParams), Z_Construct_UFunction_UPocketCapture_GetOrCreateAlphaMaskRenderTarget_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCapture_GetOrCreateAlphaMaskRenderTarget_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UPocketCapture_GetOrCreateAlphaMaskRenderTarget_Statics::PocketCapture_eventGetOrCreateAlphaMaskRenderTarget_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UPocketCapture_GetOrCreateAlphaMaskRenderTarget()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UPocketCapture_GetOrCreateAlphaMaskRenderTarget_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UPocketCapture_GetOrCreateDiffuseRenderTarget_Statics
	{
		struct PocketCapture_eventGetOrCreateDiffuseRenderTarget_Parms
		{
			UTextureRenderTarget2D* ReturnValue;
		};
		static const UECodeGen_Private::FObjectPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UPocketCapture_GetOrCreateDiffuseRenderTarget_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(PocketCapture_eventGetOrCreateDiffuseRenderTarget_Parms, ReturnValue), Z_Construct_UClass_UTextureRenderTarget2D_NoRegister, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UPocketCapture_GetOrCreateDiffuseRenderTarget_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UPocketCapture_GetOrCreateDiffuseRenderTarget_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UPocketCapture_GetOrCreateDiffuseRenderTarget_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/PocketCapture.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UPocketCapture_GetOrCreateDiffuseRenderTarget_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UPocketCapture, nullptr, "GetOrCreateDiffuseRenderTarget", nullptr, nullptr, Z_Construct_UFunction_UPocketCapture_GetOrCreateDiffuseRenderTarget_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCapture_GetOrCreateDiffuseRenderTarget_Statics::PropPointers), sizeof(Z_Construct_UFunction_UPocketCapture_GetOrCreateDiffuseRenderTarget_Statics::PocketCapture_eventGetOrCreateDiffuseRenderTarget_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCapture_GetOrCreateDiffuseRenderTarget_Statics::Function_MetaDataParams), Z_Construct_UFunction_UPocketCapture_GetOrCreateDiffuseRenderTarget_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCapture_GetOrCreateDiffuseRenderTarget_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UPocketCapture_GetOrCreateDiffuseRenderTarget_Statics::PocketCapture_eventGetOrCreateDiffuseRenderTarget_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UPocketCapture_GetOrCreateDiffuseRenderTarget()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UPocketCapture_GetOrCreateDiffuseRenderTarget_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UPocketCapture_GetOrCreateEffectsRenderTarget_Statics
	{
		struct PocketCapture_eventGetOrCreateEffectsRenderTarget_Parms
		{
			UTextureRenderTarget2D* ReturnValue;
		};
		static const UECodeGen_Private::FObjectPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UPocketCapture_GetOrCreateEffectsRenderTarget_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(PocketCapture_eventGetOrCreateEffectsRenderTarget_Parms, ReturnValue), Z_Construct_UClass_UTextureRenderTarget2D_NoRegister, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UPocketCapture_GetOrCreateEffectsRenderTarget_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UPocketCapture_GetOrCreateEffectsRenderTarget_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UPocketCapture_GetOrCreateEffectsRenderTarget_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/PocketCapture.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UPocketCapture_GetOrCreateEffectsRenderTarget_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UPocketCapture, nullptr, "GetOrCreateEffectsRenderTarget", nullptr, nullptr, Z_Construct_UFunction_UPocketCapture_GetOrCreateEffectsRenderTarget_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCapture_GetOrCreateEffectsRenderTarget_Statics::PropPointers), sizeof(Z_Construct_UFunction_UPocketCapture_GetOrCreateEffectsRenderTarget_Statics::PocketCapture_eventGetOrCreateEffectsRenderTarget_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCapture_GetOrCreateEffectsRenderTarget_Statics::Function_MetaDataParams), Z_Construct_UFunction_UPocketCapture_GetOrCreateEffectsRenderTarget_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCapture_GetOrCreateEffectsRenderTarget_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UPocketCapture_GetOrCreateEffectsRenderTarget_Statics::PocketCapture_eventGetOrCreateEffectsRenderTarget_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UPocketCapture_GetOrCreateEffectsRenderTarget()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UPocketCapture_GetOrCreateEffectsRenderTarget_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UPocketCapture_GetRendererIndex_Statics
	{
		struct PocketCapture_eventGetRendererIndex_Parms
		{
			int32 ReturnValue;
		};
		static const UECodeGen_Private::FIntPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_UPocketCapture_GetRendererIndex_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(PocketCapture_eventGetRendererIndex_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UPocketCapture_GetRendererIndex_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UPocketCapture_GetRendererIndex_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UPocketCapture_GetRendererIndex_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/PocketCapture.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UPocketCapture_GetRendererIndex_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UPocketCapture, nullptr, "GetRendererIndex", nullptr, nullptr, Z_Construct_UFunction_UPocketCapture_GetRendererIndex_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCapture_GetRendererIndex_Statics::PropPointers), sizeof(Z_Construct_UFunction_UPocketCapture_GetRendererIndex_Statics::PocketCapture_eventGetRendererIndex_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCapture_GetRendererIndex_Statics::Function_MetaDataParams), Z_Construct_UFunction_UPocketCapture_GetRendererIndex_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCapture_GetRendererIndex_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UPocketCapture_GetRendererIndex_Statics::PocketCapture_eventGetRendererIndex_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UPocketCapture_GetRendererIndex()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UPocketCapture_GetRendererIndex_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UPocketCapture_ReclaimResources_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UPocketCapture_ReclaimResources_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/PocketCapture.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UPocketCapture_ReclaimResources_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UPocketCapture, nullptr, "ReclaimResources", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020400, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCapture_ReclaimResources_Statics::Function_MetaDataParams), Z_Construct_UFunction_UPocketCapture_ReclaimResources_Statics::Function_MetaDataParams) };
	UFunction* Z_Construct_UFunction_UPocketCapture_ReclaimResources()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UPocketCapture_ReclaimResources_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UPocketCapture_ReleaseResources_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UPocketCapture_ReleaseResources_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/PocketCapture.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UPocketCapture_ReleaseResources_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UPocketCapture, nullptr, "ReleaseResources", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020400, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCapture_ReleaseResources_Statics::Function_MetaDataParams), Z_Construct_UFunction_UPocketCapture_ReleaseResources_Statics::Function_MetaDataParams) };
	UFunction* Z_Construct_UFunction_UPocketCapture_ReleaseResources()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UPocketCapture_ReleaseResources_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UPocketCapture_SetAlphaMaskedActors_Statics
	{
		struct PocketCapture_eventSetAlphaMaskedActors_Parms
		{
			TArray<AActor*> InCaptureTarget;
		};
		static const UECodeGen_Private::FObjectPropertyParams NewProp_InCaptureTarget_Inner;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_InCaptureTarget_MetaData[];
#endif
		static const UECodeGen_Private::FArrayPropertyParams NewProp_InCaptureTarget;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UPocketCapture_SetAlphaMaskedActors_Statics::NewProp_InCaptureTarget_Inner = { "InCaptureTarget", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_AActor_NoRegister, METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UPocketCapture_SetAlphaMaskedActors_Statics::NewProp_InCaptureTarget_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif
	const UECodeGen_Private::FArrayPropertyParams Z_Construct_UFunction_UPocketCapture_SetAlphaMaskedActors_Statics::NewProp_InCaptureTarget = { "InCaptureTarget", nullptr, (EPropertyFlags)0x0010000008000182, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(PocketCapture_eventSetAlphaMaskedActors_Parms, InCaptureTarget), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCapture_SetAlphaMaskedActors_Statics::NewProp_InCaptureTarget_MetaData), Z_Construct_UFunction_UPocketCapture_SetAlphaMaskedActors_Statics::NewProp_InCaptureTarget_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UPocketCapture_SetAlphaMaskedActors_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UPocketCapture_SetAlphaMaskedActors_Statics::NewProp_InCaptureTarget_Inner,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UPocketCapture_SetAlphaMaskedActors_Statics::NewProp_InCaptureTarget,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UPocketCapture_SetAlphaMaskedActors_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/PocketCapture.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UPocketCapture_SetAlphaMaskedActors_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UPocketCapture, nullptr, "SetAlphaMaskedActors", nullptr, nullptr, Z_Construct_UFunction_UPocketCapture_SetAlphaMaskedActors_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCapture_SetAlphaMaskedActors_Statics::PropPointers), sizeof(Z_Construct_UFunction_UPocketCapture_SetAlphaMaskedActors_Statics::PocketCapture_eventSetAlphaMaskedActors_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04420401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCapture_SetAlphaMaskedActors_Statics::Function_MetaDataParams), Z_Construct_UFunction_UPocketCapture_SetAlphaMaskedActors_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCapture_SetAlphaMaskedActors_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UPocketCapture_SetAlphaMaskedActors_Statics::PocketCapture_eventSetAlphaMaskedActors_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UPocketCapture_SetAlphaMaskedActors()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UPocketCapture_SetAlphaMaskedActors_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UPocketCapture_SetCaptureTarget_Statics
	{
		struct PocketCapture_eventSetCaptureTarget_Parms
		{
			AActor* InCaptureTarget;
		};
		static const UECodeGen_Private::FObjectPropertyParams NewProp_InCaptureTarget;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UPocketCapture_SetCaptureTarget_Statics::NewProp_InCaptureTarget = { "InCaptureTarget", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(PocketCapture_eventSetCaptureTarget_Parms, InCaptureTarget), Z_Construct_UClass_AActor_NoRegister, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UPocketCapture_SetCaptureTarget_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UPocketCapture_SetCaptureTarget_Statics::NewProp_InCaptureTarget,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UPocketCapture_SetCaptureTarget_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/PocketCapture.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UPocketCapture_SetCaptureTarget_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UPocketCapture, nullptr, "SetCaptureTarget", nullptr, nullptr, Z_Construct_UFunction_UPocketCapture_SetCaptureTarget_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCapture_SetCaptureTarget_Statics::PropPointers), sizeof(Z_Construct_UFunction_UPocketCapture_SetCaptureTarget_Statics::PocketCapture_eventSetCaptureTarget_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCapture_SetCaptureTarget_Statics::Function_MetaDataParams), Z_Construct_UFunction_UPocketCapture_SetCaptureTarget_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCapture_SetCaptureTarget_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UPocketCapture_SetCaptureTarget_Statics::PocketCapture_eventSetCaptureTarget_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UPocketCapture_SetCaptureTarget()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UPocketCapture_SetCaptureTarget_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UPocketCapture_SetRenderTargetSize_Statics
	{
		struct PocketCapture_eventSetRenderTargetSize_Parms
		{
			int32 Width;
			int32 Height;
		};
		static const UECodeGen_Private::FIntPropertyParams NewProp_Width;
		static const UECodeGen_Private::FIntPropertyParams NewProp_Height;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_UPocketCapture_SetRenderTargetSize_Statics::NewProp_Width = { "Width", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(PocketCapture_eventSetRenderTargetSize_Parms, Width), METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_UPocketCapture_SetRenderTargetSize_Statics::NewProp_Height = { "Height", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(PocketCapture_eventSetRenderTargetSize_Parms, Height), METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UPocketCapture_SetRenderTargetSize_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UPocketCapture_SetRenderTargetSize_Statics::NewProp_Width,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UPocketCapture_SetRenderTargetSize_Statics::NewProp_Height,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UPocketCapture_SetRenderTargetSize_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/PocketCapture.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UPocketCapture_SetRenderTargetSize_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UPocketCapture, nullptr, "SetRenderTargetSize", nullptr, nullptr, Z_Construct_UFunction_UPocketCapture_SetRenderTargetSize_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCapture_SetRenderTargetSize_Statics::PropPointers), sizeof(Z_Construct_UFunction_UPocketCapture_SetRenderTargetSize_Statics::PocketCapture_eventSetRenderTargetSize_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCapture_SetRenderTargetSize_Statics::Function_MetaDataParams), Z_Construct_UFunction_UPocketCapture_SetRenderTargetSize_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCapture_SetRenderTargetSize_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UPocketCapture_SetRenderTargetSize_Statics::PocketCapture_eventSetRenderTargetSize_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UPocketCapture_SetRenderTargetSize()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UPocketCapture_SetRenderTargetSize_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UPocketCapture);
	UClass* Z_Construct_UClass_UPocketCapture_NoRegister()
	{
		return UPocketCapture::StaticClass();
	}
	struct Z_Construct_UClass_UPocketCapture_Statics
	{
		static UObject* (*const DependentSingletons[])();
		static const FClassFunctionLinkInfo FuncInfo[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_AlphaMaskMaterial_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPtrPropertyParams NewProp_AlphaMaskMaterial;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_EffectMaskMaterial_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPtrPropertyParams NewProp_EffectMaskMaterial;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PrivateWorld_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPtrPropertyParams NewProp_PrivateWorld;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_RendererIndex_MetaData[];
#endif
		static const UECodeGen_Private::FIntPropertyParams NewProp_RendererIndex;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SurfaceWidth_MetaData[];
#endif
		static const UECodeGen_Private::FIntPropertyParams NewProp_SurfaceWidth;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SurfaceHeight_MetaData[];
#endif
		static const UECodeGen_Private::FIntPropertyParams NewProp_SurfaceHeight;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_DiffuseRT_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPtrPropertyParams NewProp_DiffuseRT;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_AlphaMaskRT_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPtrPropertyParams NewProp_AlphaMaskRT;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_EffectsRT_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPtrPropertyParams NewProp_EffectsRT;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_CaptureComponent_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPtrPropertyParams NewProp_CaptureComponent;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_CaptureTargetPtr_MetaData[];
#endif
		static const UECodeGen_Private::FWeakObjectPropertyParams NewProp_CaptureTargetPtr;
		static const UECodeGen_Private::FWeakObjectPropertyParams NewProp_AlphaMaskActorPtrs_Inner;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_AlphaMaskActorPtrs_MetaData[];
#endif
		static const UECodeGen_Private::FArrayPropertyParams NewProp_AlphaMaskActorPtrs;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UPocketCapture_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UObject,
		(UObject* (*)())Z_Construct_UPackage__Script_PocketWorlds,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketCapture_Statics::DependentSingletons) < 16);
	const FClassFunctionLinkInfo Z_Construct_UClass_UPocketCapture_Statics::FuncInfo[] = {
		{ &Z_Construct_UFunction_UPocketCapture_CaptureAlphaMask, "CaptureAlphaMask" }, // 169072363
		{ &Z_Construct_UFunction_UPocketCapture_CaptureDiffuse, "CaptureDiffuse" }, // 291841003
		{ &Z_Construct_UFunction_UPocketCapture_CaptureEffects, "CaptureEffects" }, // 520859452
		{ &Z_Construct_UFunction_UPocketCapture_GetOrCreateAlphaMaskRenderTarget, "GetOrCreateAlphaMaskRenderTarget" }, // 3327919669
		{ &Z_Construct_UFunction_UPocketCapture_GetOrCreateDiffuseRenderTarget, "GetOrCreateDiffuseRenderTarget" }, // 1060197675
		{ &Z_Construct_UFunction_UPocketCapture_GetOrCreateEffectsRenderTarget, "GetOrCreateEffectsRenderTarget" }, // 3621074146
		{ &Z_Construct_UFunction_UPocketCapture_GetRendererIndex, "GetRendererIndex" }, // 1484234940
		{ &Z_Construct_UFunction_UPocketCapture_ReclaimResources, "ReclaimResources" }, // 2894679673
		{ &Z_Construct_UFunction_UPocketCapture_ReleaseResources, "ReleaseResources" }, // 3488931628
		{ &Z_Construct_UFunction_UPocketCapture_SetAlphaMaskedActors, "SetAlphaMaskedActors" }, // 3440206060
		{ &Z_Construct_UFunction_UPocketCapture_SetCaptureTarget, "SetCaptureTarget" }, // 2988452763
		{ &Z_Construct_UFunction_UPocketCapture_SetRenderTargetSize, "SetRenderTargetSize" }, // 1418377005
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketCapture_Statics::FuncInfo) < 2048);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UPocketCapture_Statics::Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "IncludePath", "PocketCapture.h" },
		{ "IsBlueprintBase", "true" },
		{ "ModuleRelativePath", "Public/PocketCapture.h" },
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UPocketCapture_Statics::NewProp_AlphaMaskMaterial_MetaData[] = {
		{ "Category", "PocketCapture" },
		{ "ModuleRelativePath", "Public/PocketCapture.h" },
	};
#endif
	const UECodeGen_Private::FObjectPtrPropertyParams Z_Construct_UClass_UPocketCapture_Statics::NewProp_AlphaMaskMaterial = { "AlphaMaskMaterial", nullptr, (EPropertyFlags)0x0024080000010001, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPocketCapture, AlphaMaskMaterial), Z_Construct_UClass_UMaterialInterface_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketCapture_Statics::NewProp_AlphaMaskMaterial_MetaData), Z_Construct_UClass_UPocketCapture_Statics::NewProp_AlphaMaskMaterial_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UPocketCapture_Statics::NewProp_EffectMaskMaterial_MetaData[] = {
		{ "Category", "PocketCapture" },
		{ "ModuleRelativePath", "Public/PocketCapture.h" },
	};
#endif
	const UECodeGen_Private::FObjectPtrPropertyParams Z_Construct_UClass_UPocketCapture_Statics::NewProp_EffectMaskMaterial = { "EffectMaskMaterial", nullptr, (EPropertyFlags)0x0024080000010001, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPocketCapture, EffectMaskMaterial), Z_Construct_UClass_UMaterialInterface_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketCapture_Statics::NewProp_EffectMaskMaterial_MetaData), Z_Construct_UClass_UPocketCapture_Statics::NewProp_EffectMaskMaterial_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UPocketCapture_Statics::NewProp_PrivateWorld_MetaData[] = {
		{ "ModuleRelativePath", "Public/PocketCapture.h" },
	};
#endif
	const UECodeGen_Private::FObjectPtrPropertyParams Z_Construct_UClass_UPocketCapture_Statics::NewProp_PrivateWorld = { "PrivateWorld", nullptr, (EPropertyFlags)0x0024080000002000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPocketCapture, PrivateWorld), Z_Construct_UClass_UWorld_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketCapture_Statics::NewProp_PrivateWorld_MetaData), Z_Construct_UClass_UPocketCapture_Statics::NewProp_PrivateWorld_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UPocketCapture_Statics::NewProp_RendererIndex_MetaData[] = {
		{ "ModuleRelativePath", "Public/PocketCapture.h" },
	};
#endif
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UClass_UPocketCapture_Statics::NewProp_RendererIndex = { "RendererIndex", nullptr, (EPropertyFlags)0x0020080000002000, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPocketCapture, RendererIndex), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketCapture_Statics::NewProp_RendererIndex_MetaData), Z_Construct_UClass_UPocketCapture_Statics::NewProp_RendererIndex_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UPocketCapture_Statics::NewProp_SurfaceWidth_MetaData[] = {
		{ "Category", "PocketCapture" },
		{ "ModuleRelativePath", "Public/PocketCapture.h" },
	};
#endif
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UClass_UPocketCapture_Statics::NewProp_SurfaceWidth = { "SurfaceWidth", nullptr, (EPropertyFlags)0x0020080000020001, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPocketCapture, SurfaceWidth), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketCapture_Statics::NewProp_SurfaceWidth_MetaData), Z_Construct_UClass_UPocketCapture_Statics::NewProp_SurfaceWidth_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UPocketCapture_Statics::NewProp_SurfaceHeight_MetaData[] = {
		{ "Category", "PocketCapture" },
		{ "ModuleRelativePath", "Public/PocketCapture.h" },
	};
#endif
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UClass_UPocketCapture_Statics::NewProp_SurfaceHeight = { "SurfaceHeight", nullptr, (EPropertyFlags)0x0020080000020001, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPocketCapture, SurfaceHeight), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketCapture_Statics::NewProp_SurfaceHeight_MetaData), Z_Construct_UClass_UPocketCapture_Statics::NewProp_SurfaceHeight_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UPocketCapture_Statics::NewProp_DiffuseRT_MetaData[] = {
		{ "Category", "PocketCapture" },
		{ "ModuleRelativePath", "Public/PocketCapture.h" },
	};
#endif
	const UECodeGen_Private::FObjectPtrPropertyParams Z_Construct_UClass_UPocketCapture_Statics::NewProp_DiffuseRT = { "DiffuseRT", nullptr, (EPropertyFlags)0x0024080000020001, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPocketCapture, DiffuseRT), Z_Construct_UClass_UTextureRenderTarget2D_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketCapture_Statics::NewProp_DiffuseRT_MetaData), Z_Construct_UClass_UPocketCapture_Statics::NewProp_DiffuseRT_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UPocketCapture_Statics::NewProp_AlphaMaskRT_MetaData[] = {
		{ "Category", "PocketCapture" },
		{ "ModuleRelativePath", "Public/PocketCapture.h" },
	};
#endif
	const UECodeGen_Private::FObjectPtrPropertyParams Z_Construct_UClass_UPocketCapture_Statics::NewProp_AlphaMaskRT = { "AlphaMaskRT", nullptr, (EPropertyFlags)0x0024080000020001, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPocketCapture, AlphaMaskRT), Z_Construct_UClass_UTextureRenderTarget2D_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketCapture_Statics::NewProp_AlphaMaskRT_MetaData), Z_Construct_UClass_UPocketCapture_Statics::NewProp_AlphaMaskRT_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UPocketCapture_Statics::NewProp_EffectsRT_MetaData[] = {
		{ "Category", "PocketCapture" },
		{ "ModuleRelativePath", "Public/PocketCapture.h" },
	};
#endif
	const UECodeGen_Private::FObjectPtrPropertyParams Z_Construct_UClass_UPocketCapture_Statics::NewProp_EffectsRT = { "EffectsRT", nullptr, (EPropertyFlags)0x0024080000020001, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPocketCapture, EffectsRT), Z_Construct_UClass_UTextureRenderTarget2D_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketCapture_Statics::NewProp_EffectsRT_MetaData), Z_Construct_UClass_UPocketCapture_Statics::NewProp_EffectsRT_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UPocketCapture_Statics::NewProp_CaptureComponent_MetaData[] = {
		{ "Category", "PocketCapture" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Public/PocketCapture.h" },
	};
#endif
	const UECodeGen_Private::FObjectPtrPropertyParams Z_Construct_UClass_UPocketCapture_Statics::NewProp_CaptureComponent = { "CaptureComponent", nullptr, (EPropertyFlags)0x00240800000a0009, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPocketCapture, CaptureComponent), Z_Construct_UClass_USceneCaptureComponent2D_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketCapture_Statics::NewProp_CaptureComponent_MetaData), Z_Construct_UClass_UPocketCapture_Statics::NewProp_CaptureComponent_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UPocketCapture_Statics::NewProp_CaptureTargetPtr_MetaData[] = {
		{ "Category", "PocketCapture" },
		{ "ModuleRelativePath", "Public/PocketCapture.h" },
	};
#endif
	const UECodeGen_Private::FWeakObjectPropertyParams Z_Construct_UClass_UPocketCapture_Statics::NewProp_CaptureTargetPtr = { "CaptureTargetPtr", nullptr, (EPropertyFlags)0x0024080000020001, UECodeGen_Private::EPropertyGenFlags::WeakObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPocketCapture, CaptureTargetPtr), Z_Construct_UClass_AActor_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketCapture_Statics::NewProp_CaptureTargetPtr_MetaData), Z_Construct_UClass_UPocketCapture_Statics::NewProp_CaptureTargetPtr_MetaData) };
	const UECodeGen_Private::FWeakObjectPropertyParams Z_Construct_UClass_UPocketCapture_Statics::NewProp_AlphaMaskActorPtrs_Inner = { "AlphaMaskActorPtrs", nullptr, (EPropertyFlags)0x0004000000020000, UECodeGen_Private::EPropertyGenFlags::WeakObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_AActor_NoRegister, METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UPocketCapture_Statics::NewProp_AlphaMaskActorPtrs_MetaData[] = {
		{ "Category", "PocketCapture" },
		{ "ModuleRelativePath", "Public/PocketCapture.h" },
	};
#endif
	const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_UPocketCapture_Statics::NewProp_AlphaMaskActorPtrs = { "AlphaMaskActorPtrs", nullptr, (EPropertyFlags)0x0024080000020001, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPocketCapture, AlphaMaskActorPtrs), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketCapture_Statics::NewProp_AlphaMaskActorPtrs_MetaData), Z_Construct_UClass_UPocketCapture_Statics::NewProp_AlphaMaskActorPtrs_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UPocketCapture_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPocketCapture_Statics::NewProp_AlphaMaskMaterial,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPocketCapture_Statics::NewProp_EffectMaskMaterial,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPocketCapture_Statics::NewProp_PrivateWorld,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPocketCapture_Statics::NewProp_RendererIndex,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPocketCapture_Statics::NewProp_SurfaceWidth,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPocketCapture_Statics::NewProp_SurfaceHeight,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPocketCapture_Statics::NewProp_DiffuseRT,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPocketCapture_Statics::NewProp_AlphaMaskRT,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPocketCapture_Statics::NewProp_EffectsRT,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPocketCapture_Statics::NewProp_CaptureComponent,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPocketCapture_Statics::NewProp_CaptureTargetPtr,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPocketCapture_Statics::NewProp_AlphaMaskActorPtrs_Inner,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPocketCapture_Statics::NewProp_AlphaMaskActorPtrs,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_UPocketCapture_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UPocketCapture>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UPocketCapture_Statics::ClassParams = {
		&UPocketCapture::StaticClass,
		nullptr,
		&StaticCppClassTypeInfo,
		DependentSingletons,
		FuncInfo,
		Z_Construct_UClass_UPocketCapture_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		UE_ARRAY_COUNT(FuncInfo),
		UE_ARRAY_COUNT(Z_Construct_UClass_UPocketCapture_Statics::PropPointers),
		0,
		0x009000A1u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketCapture_Statics::Class_MetaDataParams), Z_Construct_UClass_UPocketCapture_Statics::Class_MetaDataParams)
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketCapture_Statics::PropPointers) < 2048);
	UClass* Z_Construct_UClass_UPocketCapture()
	{
		if (!Z_Registration_Info_UClass_UPocketCapture.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UPocketCapture.OuterSingleton, Z_Construct_UClass_UPocketCapture_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UPocketCapture.OuterSingleton;
	}
	template<> POCKETWORLDS_API UClass* StaticClass<UPocketCapture>()
	{
		return UPocketCapture::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UPocketCapture);
	UPocketCapture::~UPocketCapture() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketCapture_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketCapture_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UPocketCapture, UPocketCapture::StaticClass, TEXT("UPocketCapture"), &Z_Registration_Info_UClass_UPocketCapture, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UPocketCapture), 1965709980U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketCapture_h_627019378(TEXT("/Script/PocketWorlds"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketCapture_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketCapture_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
