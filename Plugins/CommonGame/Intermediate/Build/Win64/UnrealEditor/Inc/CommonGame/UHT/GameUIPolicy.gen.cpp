// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/GameUIPolicy.h"
#include "Public/GameUIManagerSubsystem.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGameUIPolicy() {}

// Begin Cross Module References
COMMONGAME_API UClass* Z_Construct_UClass_UGameUIPolicy();
COMMONGAME_API UClass* Z_Construct_UClass_UGameUIPolicy_NoRegister();
COMMONGAME_API UClass* Z_Construct_UClass_UPrimaryGameLayout_NoRegister();
COMMONGAME_API UEnum* Z_Construct_UEnum_CommonGame_ELocalMultiplayerInteractionMode();
COMMONGAME_API UScriptStruct* Z_Construct_UScriptStruct_FRootViewportLayoutInfo();
COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
ENGINE_API UClass* Z_Construct_UClass_ULocalPlayer_NoRegister();
UPackage* Z_Construct_UPackage__Script_CommonGame();
// End Cross Module References

// Begin Enum ELocalMultiplayerInteractionMode
static FEnumRegistrationInfo Z_Registration_Info_UEnum_ELocalMultiplayerInteractionMode;
static UEnum* ELocalMultiplayerInteractionMode_StaticEnum()
{
	if (!Z_Registration_Info_UEnum_ELocalMultiplayerInteractionMode.OuterSingleton)
	{
		Z_Registration_Info_UEnum_ELocalMultiplayerInteractionMode.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_CommonGame_ELocalMultiplayerInteractionMode, (UObject*)Z_Construct_UPackage__Script_CommonGame(), TEXT("ELocalMultiplayerInteractionMode"));
	}
	return Z_Registration_Info_UEnum_ELocalMultiplayerInteractionMode.OuterSingleton;
}
template<> COMMONGAME_API UEnum* StaticEnum<ELocalMultiplayerInteractionMode>()
{
	return ELocalMultiplayerInteractionMode_StaticEnum();
}
struct Z_Construct_UEnum_CommonGame_ELocalMultiplayerInteractionMode_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * \n */" },
#endif
		{ "ModuleRelativePath", "Public/GameUIPolicy.h" },
		{ "PrimaryOnly.Comment", "/**\n * \n */// Fullscreen viewport for the primary player only, regardless of the other player's existence\n" },
		{ "PrimaryOnly.Name", "ELocalMultiplayerInteractionMode::PrimaryOnly" },
		{ "PrimaryOnly.ToolTip", "// Fullscreen viewport for the primary player only, regardless of the other player's existence" },
		{ "Simultaneous.Comment", "// Viewports displayed simultaneously for both players\n" },
		{ "Simultaneous.Name", "ELocalMultiplayerInteractionMode::Simultaneous" },
		{ "Simultaneous.ToolTip", "Viewports displayed simultaneously for both players" },
		{ "SingleToggle.Comment", "// Fullscreen viewport for one player, but players can swap control over who's is displayed and who's is dormant\n" },
		{ "SingleToggle.Name", "ELocalMultiplayerInteractionMode::SingleToggle" },
		{ "SingleToggle.ToolTip", "Fullscreen viewport for one player, but players can swap control over who's is displayed and who's is dormant" },
	};
#endif // WITH_METADATA
	static constexpr UECodeGen_Private::FEnumeratorParam Enumerators[] = {
		{ "ELocalMultiplayerInteractionMode::PrimaryOnly", (int64)ELocalMultiplayerInteractionMode::PrimaryOnly },
		{ "ELocalMultiplayerInteractionMode::SingleToggle", (int64)ELocalMultiplayerInteractionMode::SingleToggle },
		{ "ELocalMultiplayerInteractionMode::Simultaneous", (int64)ELocalMultiplayerInteractionMode::Simultaneous },
	};
	static const UECodeGen_Private::FEnumParams EnumParams;
};
const UECodeGen_Private::FEnumParams Z_Construct_UEnum_CommonGame_ELocalMultiplayerInteractionMode_Statics::EnumParams = {
	(UObject*(*)())Z_Construct_UPackage__Script_CommonGame,
	nullptr,
	"ELocalMultiplayerInteractionMode",
	"ELocalMultiplayerInteractionMode",
	Z_Construct_UEnum_CommonGame_ELocalMultiplayerInteractionMode_Statics::Enumerators,
	RF_Public|RF_Transient|RF_MarkAsNative,
	UE_ARRAY_COUNT(Z_Construct_UEnum_CommonGame_ELocalMultiplayerInteractionMode_Statics::Enumerators),
	EEnumFlags::None,
	(uint8)UEnum::ECppForm::EnumClass,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UEnum_CommonGame_ELocalMultiplayerInteractionMode_Statics::Enum_MetaDataParams), Z_Construct_UEnum_CommonGame_ELocalMultiplayerInteractionMode_Statics::Enum_MetaDataParams)
};
UEnum* Z_Construct_UEnum_CommonGame_ELocalMultiplayerInteractionMode()
{
	if (!Z_Registration_Info_UEnum_ELocalMultiplayerInteractionMode.InnerSingleton)
	{
		UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_ELocalMultiplayerInteractionMode.InnerSingleton, Z_Construct_UEnum_CommonGame_ELocalMultiplayerInteractionMode_Statics::EnumParams);
	}
	return Z_Registration_Info_UEnum_ELocalMultiplayerInteractionMode.InnerSingleton;
}
// End Enum ELocalMultiplayerInteractionMode

// Begin ScriptStruct FRootViewportLayoutInfo
static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_RootViewportLayoutInfo;
class UScriptStruct* FRootViewportLayoutInfo::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_RootViewportLayoutInfo.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_RootViewportLayoutInfo.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FRootViewportLayoutInfo, (UObject*)Z_Construct_UPackage__Script_CommonGame(), TEXT("RootViewportLayoutInfo"));
	}
	return Z_Registration_Info_UScriptStruct_RootViewportLayoutInfo.OuterSingleton;
}
template<> COMMONGAME_API UScriptStruct* StaticStruct<FRootViewportLayoutInfo>()
{
	return FRootViewportLayoutInfo::StaticStruct();
}
struct Z_Construct_UScriptStruct_FRootViewportLayoutInfo_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/GameUIPolicy.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LocalPlayer_MetaData[] = {
		{ "ModuleRelativePath", "Public/GameUIPolicy.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_RootLayout_MetaData[] = {
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Public/GameUIPolicy.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_bAddedToViewport_MetaData[] = {
		{ "ModuleRelativePath", "Public/GameUIPolicy.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_LocalPlayer;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_RootLayout;
	static void NewProp_bAddedToViewport_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bAddedToViewport;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static void* NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FRootViewportLayoutInfo>();
	}
	static const UECodeGen_Private::FStructParams StructParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UScriptStruct_FRootViewportLayoutInfo_Statics::NewProp_LocalPlayer = { "LocalPlayer", nullptr, (EPropertyFlags)0x0114000000002000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FRootViewportLayoutInfo, LocalPlayer), Z_Construct_UClass_ULocalPlayer_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LocalPlayer_MetaData), NewProp_LocalPlayer_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UScriptStruct_FRootViewportLayoutInfo_Statics::NewProp_RootLayout = { "RootLayout", nullptr, (EPropertyFlags)0x0114000000082008, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FRootViewportLayoutInfo, RootLayout), Z_Construct_UClass_UPrimaryGameLayout_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_RootLayout_MetaData), NewProp_RootLayout_MetaData) };
void Z_Construct_UScriptStruct_FRootViewportLayoutInfo_Statics::NewProp_bAddedToViewport_SetBit(void* Obj)
{
	((FRootViewportLayoutInfo*)Obj)->bAddedToViewport = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UScriptStruct_FRootViewportLayoutInfo_Statics::NewProp_bAddedToViewport = { "bAddedToViewport", nullptr, (EPropertyFlags)0x0010000000002000, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(FRootViewportLayoutInfo), &Z_Construct_UScriptStruct_FRootViewportLayoutInfo_Statics::NewProp_bAddedToViewport_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_bAddedToViewport_MetaData), NewProp_bAddedToViewport_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FRootViewportLayoutInfo_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FRootViewportLayoutInfo_Statics::NewProp_LocalPlayer,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FRootViewportLayoutInfo_Statics::NewProp_RootLayout,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FRootViewportLayoutInfo_Statics::NewProp_bAddedToViewport,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FRootViewportLayoutInfo_Statics::PropPointers) < 2048);
const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FRootViewportLayoutInfo_Statics::StructParams = {
	(UObject* (*)())Z_Construct_UPackage__Script_CommonGame,
	nullptr,
	&NewStructOps,
	"RootViewportLayoutInfo",
	Z_Construct_UScriptStruct_FRootViewportLayoutInfo_Statics::PropPointers,
	UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FRootViewportLayoutInfo_Statics::PropPointers),
	sizeof(FRootViewportLayoutInfo),
	alignof(FRootViewportLayoutInfo),
	RF_Public|RF_Transient|RF_MarkAsNative,
	EStructFlags(0x00000005),
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FRootViewportLayoutInfo_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FRootViewportLayoutInfo_Statics::Struct_MetaDataParams)
};
UScriptStruct* Z_Construct_UScriptStruct_FRootViewportLayoutInfo()
{
	if (!Z_Registration_Info_UScriptStruct_RootViewportLayoutInfo.InnerSingleton)
	{
		UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_RootViewportLayoutInfo.InnerSingleton, Z_Construct_UScriptStruct_FRootViewportLayoutInfo_Statics::StructParams);
	}
	return Z_Registration_Info_UScriptStruct_RootViewportLayoutInfo.InnerSingleton;
}
// End ScriptStruct FRootViewportLayoutInfo

// Begin Class UGameUIPolicy
void UGameUIPolicy::StaticRegisterNativesUGameUIPolicy()
{
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UGameUIPolicy);
UClass* Z_Construct_UClass_UGameUIPolicy_NoRegister()
{
	return UGameUIPolicy::StaticClass();
}
struct Z_Construct_UClass_UGameUIPolicy_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "IncludePath", "GameUIPolicy.h" },
		{ "IsBlueprintBase", "true" },
		{ "ModuleRelativePath", "Public/GameUIPolicy.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LayoutClass_MetaData[] = {
		{ "Category", "GameUIPolicy" },
		{ "ModuleRelativePath", "Public/GameUIPolicy.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_RootViewportLayouts_MetaData[] = {
		{ "ModuleRelativePath", "Public/GameUIPolicy.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FSoftClassPropertyParams NewProp_LayoutClass;
	static const UECodeGen_Private::FStructPropertyParams NewProp_RootViewportLayouts_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_RootViewportLayouts;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UGameUIPolicy>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FSoftClassPropertyParams Z_Construct_UClass_UGameUIPolicy_Statics::NewProp_LayoutClass = { "LayoutClass", nullptr, (EPropertyFlags)0x0044000000000001, UECodeGen_Private::EPropertyGenFlags::SoftClass, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UGameUIPolicy, LayoutClass), Z_Construct_UClass_UPrimaryGameLayout_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LayoutClass_MetaData), NewProp_LayoutClass_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UGameUIPolicy_Statics::NewProp_RootViewportLayouts_Inner = { "RootViewportLayouts", nullptr, (EPropertyFlags)0x0000008000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FRootViewportLayoutInfo, METADATA_PARAMS(0, nullptr) }; // 2028361924
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_UGameUIPolicy_Statics::NewProp_RootViewportLayouts = { "RootViewportLayouts", nullptr, (EPropertyFlags)0x0040008000002000, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UGameUIPolicy, RootViewportLayouts), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_RootViewportLayouts_MetaData), NewProp_RootViewportLayouts_MetaData) }; // 2028361924
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UGameUIPolicy_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGameUIPolicy_Statics::NewProp_LayoutClass,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGameUIPolicy_Statics::NewProp_RootViewportLayouts_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGameUIPolicy_Statics::NewProp_RootViewportLayouts,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameUIPolicy_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UGameUIPolicy_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UObject,
	(UObject* (*)())Z_Construct_UPackage__Script_CommonGame,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameUIPolicy_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UGameUIPolicy_Statics::ClassParams = {
	&UGameUIPolicy::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UGameUIPolicy_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UGameUIPolicy_Statics::PropPointers),
	0,
	0x009000A1u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UGameUIPolicy_Statics::Class_MetaDataParams), Z_Construct_UClass_UGameUIPolicy_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UGameUIPolicy()
{
	if (!Z_Registration_Info_UClass_UGameUIPolicy.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UGameUIPolicy.OuterSingleton, Z_Construct_UClass_UGameUIPolicy_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UGameUIPolicy.OuterSingleton;
}
template<> COMMONGAME_API UClass* StaticClass<UGameUIPolicy>()
{
	return UGameUIPolicy::StaticClass();
}
UGameUIPolicy::UGameUIPolicy(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR(UGameUIPolicy);
UGameUIPolicy::~UGameUIPolicy() {}
// End Class UGameUIPolicy

// Begin Registration
struct Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_GameUIPolicy_h_Statics
{
	static constexpr FEnumRegisterCompiledInInfo EnumInfo[] = {
		{ ELocalMultiplayerInteractionMode_StaticEnum, TEXT("ELocalMultiplayerInteractionMode"), &Z_Registration_Info_UEnum_ELocalMultiplayerInteractionMode, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 2849029618U) },
	};
	static constexpr FStructRegisterCompiledInInfo ScriptStructInfo[] = {
		{ FRootViewportLayoutInfo::StaticStruct, Z_Construct_UScriptStruct_FRootViewportLayoutInfo_Statics::NewStructOps, TEXT("RootViewportLayoutInfo"), &Z_Registration_Info_UScriptStruct_RootViewportLayoutInfo, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FRootViewportLayoutInfo), 2028361924U) },
	};
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UGameUIPolicy, UGameUIPolicy::StaticClass, TEXT("UGameUIPolicy"), &Z_Registration_Info_UClass_UGameUIPolicy, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UGameUIPolicy), 1339460388U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_GameUIPolicy_h_521022291(TEXT("/Script/CommonGame"),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_GameUIPolicy_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_GameUIPolicy_h_Statics::ClassInfo),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_GameUIPolicy_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_GameUIPolicy_h_Statics::ScriptStructInfo),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_GameUIPolicy_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_GameUIPolicy_h_Statics::EnumInfo));
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
