// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "Actions/AsyncAction_CreateWidgetAsync.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
class APlayerController;
class UAsyncAction_CreateWidgetAsync;
class UObject;
class UUserWidget;
#ifdef COMMONGAME_AsyncAction_CreateWidgetAsync_generated_h
#error "AsyncAction_CreateWidgetAsync.generated.h already included, missing '#pragma once' in AsyncAction_CreateWidgetAsync.h"
#endif
#define COMMONGAME_AsyncAction_CreateWidgetAsync_generated_h

#define FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_CreateWidgetAsync_h_17_DELEGATE \
COMMONGAME_API void FCreateWidgetAsyncDelegate_DelegateWrapper(const FMulticastScriptDelegate& CreateWidgetAsyncDelegate, UUserWidget* UserWidget);


#define FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_CreateWidgetAsync_h_25_SPARSE_DATA
#define FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_CreateWidgetAsync_h_25_SPARSE_DATA_PROPERTY_ACCESSORS
#define FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_CreateWidgetAsync_h_25_EDITOR_ONLY_SPARSE_DATA_PROPERTY_ACCESSORS
#define FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_CreateWidgetAsync_h_25_RPC_WRAPPERS \
 \
	DECLARE_FUNCTION(execCreateWidgetAsync);


#define FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_CreateWidgetAsync_h_25_ACCESSORS
#define FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_CreateWidgetAsync_h_25_INCLASS \
private: \
	static void StaticRegisterNativesUAsyncAction_CreateWidgetAsync(); \
	friend struct Z_Construct_UClass_UAsyncAction_CreateWidgetAsync_Statics; \
public: \
	DECLARE_CLASS(UAsyncAction_CreateWidgetAsync, UCancellableAsyncAction, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/CommonGame"), NO_API) \
	DECLARE_SERIALIZER(UAsyncAction_CreateWidgetAsync)


#define FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_CreateWidgetAsync_h_25_STANDARD_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UAsyncAction_CreateWidgetAsync(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UAsyncAction_CreateWidgetAsync) \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UAsyncAction_CreateWidgetAsync); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UAsyncAction_CreateWidgetAsync); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API UAsyncAction_CreateWidgetAsync(UAsyncAction_CreateWidgetAsync&&); \
	NO_API UAsyncAction_CreateWidgetAsync(const UAsyncAction_CreateWidgetAsync&); \
public: \
	NO_API virtual ~UAsyncAction_CreateWidgetAsync();


#define FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_CreateWidgetAsync_h_22_PROLOG
#define FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_CreateWidgetAsync_h_25_GENERATED_BODY_LEGACY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_CreateWidgetAsync_h_25_SPARSE_DATA \
	FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_CreateWidgetAsync_h_25_SPARSE_DATA_PROPERTY_ACCESSORS \
	FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_CreateWidgetAsync_h_25_EDITOR_ONLY_SPARSE_DATA_PROPERTY_ACCESSORS \
	FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_CreateWidgetAsync_h_25_RPC_WRAPPERS \
	FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_CreateWidgetAsync_h_25_ACCESSORS \
	FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_CreateWidgetAsync_h_25_INCLASS \
	FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_CreateWidgetAsync_h_25_STANDARD_CONSTRUCTORS \
public: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> COMMONGAME_API UClass* StaticClass<class UAsyncAction_CreateWidgetAsync>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_CreateWidgetAsync_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
