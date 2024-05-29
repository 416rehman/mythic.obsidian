// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "PocketCapture.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
class AActor;
class UTextureRenderTarget2D;
#ifdef POCKETWORLDS_PocketCapture_generated_h
#error "PocketCapture.generated.h already included, missing '#pragma once' in PocketCapture.h"
#endif
#define POCKETWORLDS_PocketCapture_generated_h

#define FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketCapture_h_22_RPC_WRAPPERS_NO_PURE_DECLS \
	DECLARE_FUNCTION(execGetRendererIndex); \
	DECLARE_FUNCTION(execReclaimResources); \
	DECLARE_FUNCTION(execReleaseResources); \
	DECLARE_FUNCTION(execCaptureEffects); \
	DECLARE_FUNCTION(execCaptureAlphaMask); \
	DECLARE_FUNCTION(execCaptureDiffuse); \
	DECLARE_FUNCTION(execSetAlphaMaskedActors); \
	DECLARE_FUNCTION(execSetCaptureTarget); \
	DECLARE_FUNCTION(execGetOrCreateEffectsRenderTarget); \
	DECLARE_FUNCTION(execGetOrCreateAlphaMaskRenderTarget); \
	DECLARE_FUNCTION(execGetOrCreateDiffuseRenderTarget); \
	DECLARE_FUNCTION(execSetRenderTargetSize);


#define FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketCapture_h_22_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUPocketCapture(); \
	friend struct Z_Construct_UClass_UPocketCapture_Statics; \
public: \
	DECLARE_CLASS(UPocketCapture, UObject, COMPILED_IN_FLAGS(CLASS_Abstract), CASTCLASS_None, TEXT("/Script/PocketWorlds"), NO_API) \
	DECLARE_SERIALIZER(UPocketCapture) \
	DECLARE_WITHIN(UPocketCaptureSubsystem)


#define FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketCapture_h_22_ENHANCED_CONSTRUCTORS \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	UPocketCapture(UPocketCapture&&); \
	UPocketCapture(const UPocketCapture&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UPocketCapture); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UPocketCapture); \
	DEFINE_ABSTRACT_DEFAULT_CONSTRUCTOR_CALL(UPocketCapture) \
	NO_API virtual ~UPocketCapture();


#define FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketCapture_h_19_PROLOG
#define FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketCapture_h_22_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketCapture_h_22_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketCapture_h_22_INCLASS_NO_PURE_DECLS \
	FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketCapture_h_22_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> POCKETWORLDS_API UClass* StaticClass<class UPocketCapture>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketCapture_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
