// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "Messaging/CommonGameDialog.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef COMMONGAME_CommonGameDialog_generated_h
#error "CommonGameDialog.generated.h already included, missing '#pragma once' in CommonGameDialog.h"
#endif
#define COMMONGAME_CommonGameDialog_generated_h

#define FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonGameDialog_h_13_GENERATED_BODY \
	friend struct Z_Construct_UScriptStruct_FConfirmationDialogAction_Statics; \
	COMMONGAME_API static class UScriptStruct* StaticStruct();


template<> COMMONGAME_API UScriptStruct* StaticStruct<struct FConfirmationDialogAction>();

#define FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonGameDialog_h_34_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUCommonGameDialogDescriptor(); \
	friend struct Z_Construct_UClass_UCommonGameDialogDescriptor_Statics; \
public: \
	DECLARE_CLASS(UCommonGameDialogDescriptor, UObject, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/CommonGame"), NO_API) \
	DECLARE_SERIALIZER(UCommonGameDialogDescriptor)


#define FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonGameDialog_h_34_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UCommonGameDialogDescriptor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	UCommonGameDialogDescriptor(UCommonGameDialogDescriptor&&); \
	UCommonGameDialogDescriptor(const UCommonGameDialogDescriptor&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UCommonGameDialogDescriptor); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UCommonGameDialogDescriptor); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UCommonGameDialogDescriptor) \
	NO_API virtual ~UCommonGameDialogDescriptor();


#define FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonGameDialog_h_31_PROLOG
#define FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonGameDialog_h_34_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonGameDialog_h_34_INCLASS_NO_PURE_DECLS \
	FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonGameDialog_h_34_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> COMMONGAME_API UClass* StaticClass<class UCommonGameDialogDescriptor>();

#define FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonGameDialog_h_60_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUCommonGameDialog(); \
	friend struct Z_Construct_UClass_UCommonGameDialog_Statics; \
public: \
	DECLARE_CLASS(UCommonGameDialog, UCommonActivatableWidget, COMPILED_IN_FLAGS(CLASS_Abstract), CASTCLASS_None, TEXT("/Script/CommonGame"), NO_API) \
	DECLARE_SERIALIZER(UCommonGameDialog)


#define FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonGameDialog_h_60_ENHANCED_CONSTRUCTORS \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	UCommonGameDialog(UCommonGameDialog&&); \
	UCommonGameDialog(const UCommonGameDialog&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UCommonGameDialog); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UCommonGameDialog); \
	DEFINE_ABSTRACT_DEFAULT_CONSTRUCTOR_CALL(UCommonGameDialog) \
	NO_API virtual ~UCommonGameDialog();


#define FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonGameDialog_h_57_PROLOG
#define FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonGameDialog_h_60_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonGameDialog_h_60_INCLASS_NO_PURE_DECLS \
	FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonGameDialog_h_60_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> COMMONGAME_API UClass* StaticClass<class UCommonGameDialog>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonGameDialog_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
