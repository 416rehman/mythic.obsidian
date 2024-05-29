// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "Actions/AsyncAction_ShowConfirmation.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
class UAsyncAction_ShowConfirmation;
class UCommonGameDialogDescriptor;
class UObject;
enum class ECommonMessagingResult : uint8;
#ifdef COMMONGAME_AsyncAction_ShowConfirmation_generated_h
#error "AsyncAction_ShowConfirmation.generated.h already included, missing '#pragma once' in AsyncAction_ShowConfirmation.h"
#endif
#define COMMONGAME_AsyncAction_ShowConfirmation_generated_h

#define FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_ShowConfirmation_h_17_DELEGATE \
COMMONGAME_API void FCommonMessagingResultMCDelegate_DelegateWrapper(const FMulticastScriptDelegate& CommonMessagingResultMCDelegate, ECommonMessagingResult Result);


#define FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_ShowConfirmation_h_25_RPC_WRAPPERS \
	DECLARE_FUNCTION(execShowConfirmationCustom); \
	DECLARE_FUNCTION(execShowConfirmationOkCancel); \
	DECLARE_FUNCTION(execShowConfirmationYesNo);


#define FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_ShowConfirmation_h_25_INCLASS \
private: \
	static void StaticRegisterNativesUAsyncAction_ShowConfirmation(); \
	friend struct Z_Construct_UClass_UAsyncAction_ShowConfirmation_Statics; \
public: \
	DECLARE_CLASS(UAsyncAction_ShowConfirmation, UBlueprintAsyncActionBase, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/CommonGame"), NO_API) \
	DECLARE_SERIALIZER(UAsyncAction_ShowConfirmation)


#define FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_ShowConfirmation_h_25_STANDARD_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UAsyncAction_ShowConfirmation(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UAsyncAction_ShowConfirmation) \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UAsyncAction_ShowConfirmation); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UAsyncAction_ShowConfirmation); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	UAsyncAction_ShowConfirmation(UAsyncAction_ShowConfirmation&&); \
	UAsyncAction_ShowConfirmation(const UAsyncAction_ShowConfirmation&); \
public: \
	NO_API virtual ~UAsyncAction_ShowConfirmation();


#define FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_ShowConfirmation_h_22_PROLOG
#define FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_ShowConfirmation_h_25_GENERATED_BODY_LEGACY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_ShowConfirmation_h_25_RPC_WRAPPERS \
	FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_ShowConfirmation_h_25_INCLASS \
	FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_ShowConfirmation_h_25_STANDARD_CONSTRUCTORS \
public: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> COMMONGAME_API UClass* StaticClass<class UAsyncAction_ShowConfirmation>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_ShowConfirmation_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
