// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "LoadingProcessTask.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
class ULoadingProcessTask;
class UObject;
#ifdef COMMONLOADINGSCREEN_LoadingProcessTask_generated_h
#error "LoadingProcessTask.generated.h already included, missing '#pragma once' in LoadingProcessTask.h"
#endif
#define COMMONLOADINGSCREEN_LoadingProcessTask_generated_h

#define FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessTask_h_15_RPC_WRAPPERS_NO_PURE_DECLS \
	DECLARE_FUNCTION(execSetShowLoadingScreenReason); \
	DECLARE_FUNCTION(execUnregister); \
	DECLARE_FUNCTION(execCreateLoadingScreenProcessTask);


#define FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessTask_h_15_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesULoadingProcessTask(); \
	friend struct Z_Construct_UClass_ULoadingProcessTask_Statics; \
public: \
	DECLARE_CLASS(ULoadingProcessTask, UObject, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/CommonLoadingScreen"), NO_API) \
	DECLARE_SERIALIZER(ULoadingProcessTask) \
	virtual UObject* _getUObject() const override { return const_cast<ULoadingProcessTask*>(this); }


#define FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessTask_h_15_ENHANCED_CONSTRUCTORS \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	ULoadingProcessTask(ULoadingProcessTask&&); \
	ULoadingProcessTask(const ULoadingProcessTask&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, ULoadingProcessTask); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(ULoadingProcessTask); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(ULoadingProcessTask) \
	NO_API virtual ~ULoadingProcessTask();


#define FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessTask_h_12_PROLOG
#define FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessTask_h_15_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessTask_h_15_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessTask_h_15_INCLASS_NO_PURE_DECLS \
	FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessTask_h_15_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> COMMONLOADINGSCREEN_API UClass* StaticClass<class ULoadingProcessTask>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessTask_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
