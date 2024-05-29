// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "GameUIPolicy.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef COMMONGAME_GameUIPolicy_generated_h
#error "GameUIPolicy.generated.h already included, missing '#pragma once' in GameUIPolicy.h"
#endif
#define COMMONGAME_GameUIPolicy_generated_h

#define FID_Mythic_Plugins_CommonGame_Source_Public_GameUIPolicy_h_33_GENERATED_BODY \
	friend struct Z_Construct_UScriptStruct_FRootViewportLayoutInfo_Statics; \
	COMMONGAME_API static class UScriptStruct* StaticStruct();


template<> COMMONGAME_API UScriptStruct* StaticStruct<struct FRootViewportLayoutInfo>();

#define FID_Mythic_Plugins_CommonGame_Source_Public_GameUIPolicy_h_57_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUGameUIPolicy(); \
	friend struct Z_Construct_UClass_UGameUIPolicy_Statics; \
public: \
	DECLARE_CLASS(UGameUIPolicy, UObject, COMPILED_IN_FLAGS(CLASS_Abstract), CASTCLASS_None, TEXT("/Script/CommonGame"), NO_API) \
	DECLARE_SERIALIZER(UGameUIPolicy) \
	DECLARE_WITHIN(UGameUIManagerSubsystem)


#define FID_Mythic_Plugins_CommonGame_Source_Public_GameUIPolicy_h_57_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UGameUIPolicy(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	UGameUIPolicy(UGameUIPolicy&&); \
	UGameUIPolicy(const UGameUIPolicy&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UGameUIPolicy); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UGameUIPolicy); \
	DEFINE_ABSTRACT_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UGameUIPolicy) \
	NO_API virtual ~UGameUIPolicy();


#define FID_Mythic_Plugins_CommonGame_Source_Public_GameUIPolicy_h_54_PROLOG
#define FID_Mythic_Plugins_CommonGame_Source_Public_GameUIPolicy_h_57_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_CommonGame_Source_Public_GameUIPolicy_h_57_INCLASS_NO_PURE_DECLS \
	FID_Mythic_Plugins_CommonGame_Source_Public_GameUIPolicy_h_57_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> COMMONGAME_API UClass* StaticClass<class UGameUIPolicy>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Mythic_Plugins_CommonGame_Source_Public_GameUIPolicy_h


#define FOREACH_ENUM_ELOCALMULTIPLAYERINTERACTIONMODE(op) \
	op(ELocalMultiplayerInteractionMode::PrimaryOnly) \
	op(ELocalMultiplayerInteractionMode::SingleToggle) \
	op(ELocalMultiplayerInteractionMode::Simultaneous) 

enum class ELocalMultiplayerInteractionMode : uint8;
template<> struct TIsUEnumClass<ELocalMultiplayerInteractionMode> { enum { Value = true }; };
template<> COMMONGAME_API UEnum* StaticEnum<ELocalMultiplayerInteractionMode>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
