// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "SubtitleDisplayOptions.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef GAMESUBTITLES_SubtitleDisplayOptions_generated_h
#error "SubtitleDisplayOptions.generated.h already included, missing '#pragma once' in SubtitleDisplayOptions.h"
#endif
#define GAMESUBTITLES_SubtitleDisplayOptions_generated_h

#define FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplayOptions_h_58_SPARSE_DATA
#define FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplayOptions_h_58_SPARSE_DATA_PROPERTY_ACCESSORS
#define FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplayOptions_h_58_EDITOR_ONLY_SPARSE_DATA_PROPERTY_ACCESSORS
#define FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplayOptions_h_58_RPC_WRAPPERS_NO_PURE_DECLS
#define FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplayOptions_h_58_ACCESSORS
#define FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplayOptions_h_58_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUSubtitleDisplayOptions(); \
	friend struct Z_Construct_UClass_USubtitleDisplayOptions_Statics; \
public: \
	DECLARE_CLASS(USubtitleDisplayOptions, UDataAsset, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/GameSubtitles"), NO_API) \
	DECLARE_SERIALIZER(USubtitleDisplayOptions)


#define FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplayOptions_h_58_ENHANCED_CONSTRUCTORS \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API USubtitleDisplayOptions(USubtitleDisplayOptions&&); \
	NO_API USubtitleDisplayOptions(const USubtitleDisplayOptions&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, USubtitleDisplayOptions); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(USubtitleDisplayOptions); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(USubtitleDisplayOptions) \
	NO_API virtual ~USubtitleDisplayOptions();


#define FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplayOptions_h_55_PROLOG
#define FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplayOptions_h_58_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplayOptions_h_58_SPARSE_DATA \
	FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplayOptions_h_58_SPARSE_DATA_PROPERTY_ACCESSORS \
	FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplayOptions_h_58_EDITOR_ONLY_SPARSE_DATA_PROPERTY_ACCESSORS \
	FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplayOptions_h_58_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplayOptions_h_58_ACCESSORS \
	FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplayOptions_h_58_INCLASS_NO_PURE_DECLS \
	FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplayOptions_h_58_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> GAMESUBTITLES_API UClass* StaticClass<class USubtitleDisplayOptions>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplayOptions_h


#define FOREACH_ENUM_ESUBTITLEDISPLAYTEXTSIZE(op) \
	op(ESubtitleDisplayTextSize::ExtraSmall) \
	op(ESubtitleDisplayTextSize::Small) \
	op(ESubtitleDisplayTextSize::Medium) \
	op(ESubtitleDisplayTextSize::Large) \
	op(ESubtitleDisplayTextSize::ExtraLarge) 

enum class ESubtitleDisplayTextSize : uint8;
template<> struct TIsUEnumClass<ESubtitleDisplayTextSize> { enum { Value = true }; };
template<> GAMESUBTITLES_API UEnum* StaticEnum<ESubtitleDisplayTextSize>();

#define FOREACH_ENUM_ESUBTITLEDISPLAYTEXTCOLOR(op) \
	op(ESubtitleDisplayTextColor::White) \
	op(ESubtitleDisplayTextColor::Yellow) 

enum class ESubtitleDisplayTextColor : uint8;
template<> struct TIsUEnumClass<ESubtitleDisplayTextColor> { enum { Value = true }; };
template<> GAMESUBTITLES_API UEnum* StaticEnum<ESubtitleDisplayTextColor>();

#define FOREACH_ENUM_ESUBTITLEDISPLAYTEXTBORDER(op) \
	op(ESubtitleDisplayTextBorder::None) \
	op(ESubtitleDisplayTextBorder::Outline) \
	op(ESubtitleDisplayTextBorder::DropShadow) 

enum class ESubtitleDisplayTextBorder : uint8;
template<> struct TIsUEnumClass<ESubtitleDisplayTextBorder> { enum { Value = true }; };
template<> GAMESUBTITLES_API UEnum* StaticEnum<ESubtitleDisplayTextBorder>();

#define FOREACH_ENUM_ESUBTITLEDISPLAYBACKGROUNDOPACITY(op) \
	op(ESubtitleDisplayBackgroundOpacity::Clear) \
	op(ESubtitleDisplayBackgroundOpacity::Low) \
	op(ESubtitleDisplayBackgroundOpacity::Medium) \
	op(ESubtitleDisplayBackgroundOpacity::High) \
	op(ESubtitleDisplayBackgroundOpacity::Solid) 

enum class ESubtitleDisplayBackgroundOpacity : uint8;
template<> struct TIsUEnumClass<ESubtitleDisplayBackgroundOpacity> { enum { Value = true }; };
template<> GAMESUBTITLES_API UEnum* StaticEnum<ESubtitleDisplayBackgroundOpacity>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
