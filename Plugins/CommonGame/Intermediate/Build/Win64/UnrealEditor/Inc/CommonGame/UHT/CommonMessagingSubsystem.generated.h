// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "Messaging/CommonMessagingSubsystem.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef COMMONGAME_CommonMessagingSubsystem_generated_h
#error "CommonMessagingSubsystem.generated.h already included, missing '#pragma once' in CommonMessagingSubsystem.h"
#endif
#define COMMONGAME_CommonMessagingSubsystem_generated_h

#define FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonMessagingSubsystem_h_36_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUCommonMessagingSubsystem(); \
	friend struct Z_Construct_UClass_UCommonMessagingSubsystem_Statics; \
public: \
	DECLARE_CLASS(UCommonMessagingSubsystem, ULocalPlayerSubsystem, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/CommonGame"), NO_API) \
	DECLARE_SERIALIZER(UCommonMessagingSubsystem) \
	static const TCHAR* StaticConfigName() {return TEXT("Game");} \



#define FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonMessagingSubsystem_h_36_ENHANCED_CONSTRUCTORS \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	UCommonMessagingSubsystem(UCommonMessagingSubsystem&&); \
	UCommonMessagingSubsystem(const UCommonMessagingSubsystem&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UCommonMessagingSubsystem); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UCommonMessagingSubsystem); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(UCommonMessagingSubsystem) \
	NO_API virtual ~UCommonMessagingSubsystem();


#define FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonMessagingSubsystem_h_33_PROLOG
#define FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonMessagingSubsystem_h_36_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonMessagingSubsystem_h_36_INCLASS_NO_PURE_DECLS \
	FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonMessagingSubsystem_h_36_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> COMMONGAME_API UClass* StaticClass<class UCommonMessagingSubsystem>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonMessagingSubsystem_h


#define FOREACH_ENUM_ECOMMONMESSAGINGRESULT(op) \
	op(ECommonMessagingResult::Confirmed) \
	op(ECommonMessagingResult::Declined) \
	op(ECommonMessagingResult::Cancelled) \
	op(ECommonMessagingResult::Killed) \
	op(ECommonMessagingResult::Unknown) 

enum class ECommonMessagingResult : uint8;
template<> struct TIsUEnumClass<ECommonMessagingResult> { enum { Value = true }; };
template<> COMMONGAME_API UEnum* StaticEnum<ECommonMessagingResult>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
