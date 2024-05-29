// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "PocketLevelSystem.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef POCKETWORLDS_PocketLevelSystem_generated_h
#error "PocketLevelSystem.generated.h already included, missing '#pragma once' in PocketLevelSystem.h"
#endif
#define POCKETWORLDS_PocketLevelSystem_generated_h

#define FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketLevelSystem_h_20_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUPocketLevelSubsystem(); \
	friend struct Z_Construct_UClass_UPocketLevelSubsystem_Statics; \
public: \
	DECLARE_CLASS(UPocketLevelSubsystem, UWorldSubsystem, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/PocketWorlds"), NO_API) \
	DECLARE_SERIALIZER(UPocketLevelSubsystem)


#define FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketLevelSystem_h_20_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UPocketLevelSubsystem(); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	UPocketLevelSubsystem(UPocketLevelSubsystem&&); \
	UPocketLevelSubsystem(const UPocketLevelSubsystem&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UPocketLevelSubsystem); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UPocketLevelSubsystem); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(UPocketLevelSubsystem) \
	NO_API virtual ~UPocketLevelSubsystem();


#define FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketLevelSystem_h_17_PROLOG
#define FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketLevelSystem_h_20_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketLevelSystem_h_20_INCLASS_NO_PURE_DECLS \
	FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketLevelSystem_h_20_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> POCKETWORLDS_API UClass* StaticClass<class UPocketLevelSubsystem>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketLevelSystem_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
