// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "Actions/AsyncAction_PushContentToLayerForPlayer.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
class APlayerController;
class UAsyncAction_PushContentToLayerForPlayer;
class UCommonActivatableWidget;
struct FGameplayTag;
#ifdef COMMONGAME_AsyncAction_PushContentToLayerForPlayer_generated_h
#error "AsyncAction_PushContentToLayerForPlayer.generated.h already included, missing '#pragma once' in AsyncAction_PushContentToLayerForPlayer.h"
#endif
#define COMMONGAME_AsyncAction_PushContentToLayerForPlayer_generated_h

#define FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_PushContentToLayerForPlayer_h_17_DELEGATE \
COMMONGAME_API void FPushContentToLayerForPlayerAsyncDelegate_DelegateWrapper(const FMulticastScriptDelegate& PushContentToLayerForPlayerAsyncDelegate, UCommonActivatableWidget* UserWidget);


#define FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_PushContentToLayerForPlayer_h_25_SPARSE_DATA
#define FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_PushContentToLayerForPlayer_h_25_SPARSE_DATA_PROPERTY_ACCESSORS
#define FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_PushContentToLayerForPlayer_h_25_EDITOR_ONLY_SPARSE_DATA_PROPERTY_ACCESSORS
#define FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_PushContentToLayerForPlayer_h_25_RPC_WRAPPERS \
 \
	DECLARE_FUNCTION(execPushContentToLayerForPlayer);


#define FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_PushContentToLayerForPlayer_h_25_ACCESSORS
#define FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_PushContentToLayerForPlayer_h_25_INCLASS \
private: \
	static void StaticRegisterNativesUAsyncAction_PushContentToLayerForPlayer(); \
	friend struct Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_Statics; \
public: \
	DECLARE_CLASS(UAsyncAction_PushContentToLayerForPlayer, UCancellableAsyncAction, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/CommonGame"), NO_API) \
	DECLARE_SERIALIZER(UAsyncAction_PushContentToLayerForPlayer)


#define FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_PushContentToLayerForPlayer_h_25_STANDARD_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UAsyncAction_PushContentToLayerForPlayer(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UAsyncAction_PushContentToLayerForPlayer) \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UAsyncAction_PushContentToLayerForPlayer); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UAsyncAction_PushContentToLayerForPlayer); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API UAsyncAction_PushContentToLayerForPlayer(UAsyncAction_PushContentToLayerForPlayer&&); \
	NO_API UAsyncAction_PushContentToLayerForPlayer(const UAsyncAction_PushContentToLayerForPlayer&); \
public: \
	NO_API virtual ~UAsyncAction_PushContentToLayerForPlayer();


#define FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_PushContentToLayerForPlayer_h_22_PROLOG
#define FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_PushContentToLayerForPlayer_h_25_GENERATED_BODY_LEGACY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_PushContentToLayerForPlayer_h_25_SPARSE_DATA \
	FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_PushContentToLayerForPlayer_h_25_SPARSE_DATA_PROPERTY_ACCESSORS \
	FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_PushContentToLayerForPlayer_h_25_EDITOR_ONLY_SPARSE_DATA_PROPERTY_ACCESSORS \
	FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_PushContentToLayerForPlayer_h_25_RPC_WRAPPERS \
	FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_PushContentToLayerForPlayer_h_25_ACCESSORS \
	FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_PushContentToLayerForPlayer_h_25_INCLASS \
	FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_PushContentToLayerForPlayer_h_25_STANDARD_CONSTRUCTORS \
public: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> COMMONGAME_API UClass* StaticClass<class UAsyncAction_PushContentToLayerForPlayer>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_PushContentToLayerForPlayer_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
