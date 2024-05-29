// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "Widgets/Responsive/GameResponsivePanel.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
class UGameResponsivePanelSlot;
class UWidget;
#ifdef GAMESETTINGS_GameResponsivePanel_generated_h
#error "GameResponsivePanel.generated.h already included, missing '#pragma once' in GameResponsivePanel.h"
#endif
#define GAMESETTINGS_GameResponsivePanel_generated_h

#define FID_Mythic_Plugins_GameSettings_Source_Private_Widgets_Responsive_GameResponsivePanel_h_19_RPC_WRAPPERS \
	DECLARE_FUNCTION(execAddChildToGameResponsivePanel);


#define FID_Mythic_Plugins_GameSettings_Source_Private_Widgets_Responsive_GameResponsivePanel_h_19_INCLASS \
private: \
	static void StaticRegisterNativesUGameResponsivePanel(); \
	friend struct Z_Construct_UClass_UGameResponsivePanel_Statics; \
public: \
	DECLARE_CLASS(UGameResponsivePanel, UPanelWidget, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/GameSettings"), NO_API) \
	DECLARE_SERIALIZER(UGameResponsivePanel)


#define FID_Mythic_Plugins_GameSettings_Source_Private_Widgets_Responsive_GameResponsivePanel_h_19_STANDARD_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UGameResponsivePanel(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UGameResponsivePanel) \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UGameResponsivePanel); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UGameResponsivePanel); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	UGameResponsivePanel(UGameResponsivePanel&&); \
	UGameResponsivePanel(const UGameResponsivePanel&); \
public: \
	NO_API virtual ~UGameResponsivePanel();


#define FID_Mythic_Plugins_GameSettings_Source_Private_Widgets_Responsive_GameResponsivePanel_h_16_PROLOG
#define FID_Mythic_Plugins_GameSettings_Source_Private_Widgets_Responsive_GameResponsivePanel_h_19_GENERATED_BODY_LEGACY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_GameSettings_Source_Private_Widgets_Responsive_GameResponsivePanel_h_19_RPC_WRAPPERS \
	FID_Mythic_Plugins_GameSettings_Source_Private_Widgets_Responsive_GameResponsivePanel_h_19_INCLASS \
	FID_Mythic_Plugins_GameSettings_Source_Private_Widgets_Responsive_GameResponsivePanel_h_19_STANDARD_CONSTRUCTORS \
public: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> GAMESETTINGS_API UClass* StaticClass<class UGameResponsivePanel>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Mythic_Plugins_GameSettings_Source_Private_Widgets_Responsive_GameResponsivePanel_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
