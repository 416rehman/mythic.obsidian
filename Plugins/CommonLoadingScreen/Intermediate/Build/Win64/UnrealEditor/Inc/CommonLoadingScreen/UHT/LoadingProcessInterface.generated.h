// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "LoadingProcessInterface.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef COMMONLOADINGSCREEN_LoadingProcessInterface_generated_h
#error "LoadingProcessInterface.generated.h already included, missing '#pragma once' in LoadingProcessInterface.h"
#endif
#define COMMONLOADINGSCREEN_LoadingProcessInterface_generated_h

#define FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_14_SPARSE_DATA
#define FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_14_SPARSE_DATA_PROPERTY_ACCESSORS
#define FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_14_EDITOR_ONLY_SPARSE_DATA_PROPERTY_ACCESSORS
#define FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_14_RPC_WRAPPERS_NO_PURE_DECLS
#define FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_14_ACCESSORS
#define FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_14_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API ULoadingProcessInterface(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API ULoadingProcessInterface(ULoadingProcessInterface&&); \
	NO_API ULoadingProcessInterface(const ULoadingProcessInterface&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, ULoadingProcessInterface); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(ULoadingProcessInterface); \
	DEFINE_ABSTRACT_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(ULoadingProcessInterface) \
	NO_API virtual ~ULoadingProcessInterface();


#define FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_14_GENERATED_UINTERFACE_BODY() \
private: \
	static void StaticRegisterNativesULoadingProcessInterface(); \
	friend struct Z_Construct_UClass_ULoadingProcessInterface_Statics; \
public: \
	DECLARE_CLASS(ULoadingProcessInterface, UInterface, COMPILED_IN_FLAGS(CLASS_Abstract | CLASS_Interface), CASTCLASS_None, TEXT("/Script/CommonLoadingScreen"), NO_API) \
	DECLARE_SERIALIZER(ULoadingProcessInterface)


#define FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_14_GENERATED_BODY \
	PRAGMA_DISABLE_DEPRECATION_WARNINGS \
	FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_14_GENERATED_UINTERFACE_BODY() \
	FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_14_ENHANCED_CONSTRUCTORS \
private: \
	PRAGMA_ENABLE_DEPRECATION_WARNINGS


#define FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_14_INCLASS_IINTERFACE_NO_PURE_DECLS \
protected: \
	virtual ~ILoadingProcessInterface() {} \
public: \
	typedef ULoadingProcessInterface UClassType; \
	typedef ILoadingProcessInterface ThisClass; \
	virtual UObject* _getUObject() const { return nullptr; }


#define FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_11_PROLOG
#define FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_19_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_14_SPARSE_DATA \
	FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_14_SPARSE_DATA_PROPERTY_ACCESSORS \
	FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_14_EDITOR_ONLY_SPARSE_DATA_PROPERTY_ACCESSORS \
	FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_14_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_14_ACCESSORS \
	FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_14_INCLASS_IINTERFACE_NO_PURE_DECLS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> COMMONLOADINGSCREEN_API UClass* StaticClass<class ULoadingProcessInterface>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
