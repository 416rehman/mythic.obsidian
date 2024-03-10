// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "Widgets/UIExtensionPointWidget.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
class UObject;
class UUserWidget;
#ifdef UIEXTENSION_UIExtensionPointWidget_generated_h
#error "UIExtensionPointWidget.generated.h already included, missing '#pragma once' in UIExtensionPointWidget.h"
#endif
#define UIEXTENSION_UIExtensionPointWidget_generated_h

#define FID_Mythic_Plugins_UIExtension_Source_Public_Widgets_UIExtensionPointWidget_h_25_DELEGATE \
static TSubclassOf<UUserWidget>  FOnGetWidgetClassForData_DelegateWrapper(const FScriptDelegate& OnGetWidgetClassForData, UObject* DataItem);


#define FID_Mythic_Plugins_UIExtension_Source_Public_Widgets_UIExtensionPointWidget_h_26_DELEGATE \
static void FOnConfigureWidgetForData_DelegateWrapper(const FScriptDelegate& OnConfigureWidgetForData, UUserWidget* Widget, UObject* DataItem);


#define FID_Mythic_Plugins_UIExtension_Source_Public_Widgets_UIExtensionPointWidget_h_21_SPARSE_DATA
#define FID_Mythic_Plugins_UIExtension_Source_Public_Widgets_UIExtensionPointWidget_h_21_SPARSE_DATA_PROPERTY_ACCESSORS
#define FID_Mythic_Plugins_UIExtension_Source_Public_Widgets_UIExtensionPointWidget_h_21_EDITOR_ONLY_SPARSE_DATA_PROPERTY_ACCESSORS
#define FID_Mythic_Plugins_UIExtension_Source_Public_Widgets_UIExtensionPointWidget_h_21_RPC_WRAPPERS_NO_PURE_DECLS
#define FID_Mythic_Plugins_UIExtension_Source_Public_Widgets_UIExtensionPointWidget_h_21_ACCESSORS
#define FID_Mythic_Plugins_UIExtension_Source_Public_Widgets_UIExtensionPointWidget_h_21_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUUIExtensionPointWidget(); \
	friend struct Z_Construct_UClass_UUIExtensionPointWidget_Statics; \
public: \
	DECLARE_CLASS(UUIExtensionPointWidget, UDynamicEntryBoxBase, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/UIExtension"), NO_API) \
	DECLARE_SERIALIZER(UUIExtensionPointWidget)


#define FID_Mythic_Plugins_UIExtension_Source_Public_Widgets_UIExtensionPointWidget_h_21_ENHANCED_CONSTRUCTORS \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API UUIExtensionPointWidget(UUIExtensionPointWidget&&); \
	NO_API UUIExtensionPointWidget(const UUIExtensionPointWidget&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UUIExtensionPointWidget); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UUIExtensionPointWidget); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UUIExtensionPointWidget) \
	NO_API virtual ~UUIExtensionPointWidget();


#define FID_Mythic_Plugins_UIExtension_Source_Public_Widgets_UIExtensionPointWidget_h_18_PROLOG
#define FID_Mythic_Plugins_UIExtension_Source_Public_Widgets_UIExtensionPointWidget_h_21_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_UIExtension_Source_Public_Widgets_UIExtensionPointWidget_h_21_SPARSE_DATA \
	FID_Mythic_Plugins_UIExtension_Source_Public_Widgets_UIExtensionPointWidget_h_21_SPARSE_DATA_PROPERTY_ACCESSORS \
	FID_Mythic_Plugins_UIExtension_Source_Public_Widgets_UIExtensionPointWidget_h_21_EDITOR_ONLY_SPARSE_DATA_PROPERTY_ACCESSORS \
	FID_Mythic_Plugins_UIExtension_Source_Public_Widgets_UIExtensionPointWidget_h_21_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Mythic_Plugins_UIExtension_Source_Public_Widgets_UIExtensionPointWidget_h_21_ACCESSORS \
	FID_Mythic_Plugins_UIExtension_Source_Public_Widgets_UIExtensionPointWidget_h_21_INCLASS_NO_PURE_DECLS \
	FID_Mythic_Plugins_UIExtension_Source_Public_Widgets_UIExtensionPointWidget_h_21_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> UIEXTENSION_API UClass* StaticClass<class UUIExtensionPointWidget>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Mythic_Plugins_UIExtension_Source_Public_Widgets_UIExtensionPointWidget_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
