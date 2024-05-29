// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "BPFunctionLibrary.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
class UMaterialInterface;
class UStaticMesh;
#ifdef LYRAEXTTOOL_BPFunctionLibrary_generated_h
#error "BPFunctionLibrary.generated.h already included, missing '#pragma once' in BPFunctionLibrary.h"
#endif
#define LYRAEXTTOOL_BPFunctionLibrary_generated_h

#define FID_Mythic_Plugins_LyraExtTool_Source_LyraExtTool_Public_BPFunctionLibrary_h_21_RPC_WRAPPERS_NO_PURE_DECLS \
	DECLARE_FUNCTION(execChangeMeshMaterials);


#define FID_Mythic_Plugins_LyraExtTool_Source_LyraExtTool_Public_BPFunctionLibrary_h_21_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUBPFunctionLibrary(); \
	friend struct Z_Construct_UClass_UBPFunctionLibrary_Statics; \
public: \
	DECLARE_CLASS(UBPFunctionLibrary, UBlueprintFunctionLibrary, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/LyraExtTool"), NO_API) \
	DECLARE_SERIALIZER(UBPFunctionLibrary)


#define FID_Mythic_Plugins_LyraExtTool_Source_LyraExtTool_Public_BPFunctionLibrary_h_21_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UBPFunctionLibrary(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	UBPFunctionLibrary(UBPFunctionLibrary&&); \
	UBPFunctionLibrary(const UBPFunctionLibrary&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UBPFunctionLibrary); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UBPFunctionLibrary); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UBPFunctionLibrary) \
	NO_API virtual ~UBPFunctionLibrary();


#define FID_Mythic_Plugins_LyraExtTool_Source_LyraExtTool_Public_BPFunctionLibrary_h_18_PROLOG
#define FID_Mythic_Plugins_LyraExtTool_Source_LyraExtTool_Public_BPFunctionLibrary_h_21_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_LyraExtTool_Source_LyraExtTool_Public_BPFunctionLibrary_h_21_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Mythic_Plugins_LyraExtTool_Source_LyraExtTool_Public_BPFunctionLibrary_h_21_INCLASS_NO_PURE_DECLS \
	FID_Mythic_Plugins_LyraExtTool_Source_LyraExtTool_Public_BPFunctionLibrary_h_21_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> LYRAEXTTOOL_API UClass* StaticClass<class UBPFunctionLibrary>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Mythic_Plugins_LyraExtTool_Source_LyraExtTool_Public_BPFunctionLibrary_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
