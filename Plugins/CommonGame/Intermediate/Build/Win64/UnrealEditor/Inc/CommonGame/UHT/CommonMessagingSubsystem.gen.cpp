// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/Messaging/CommonMessagingSubsystem.h"
#include "Runtime/Engine/Classes/Engine/LocalPlayer.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeCommonMessagingSubsystem() {}

// Begin Cross Module References
COMMONGAME_API UClass* Z_Construct_UClass_UCommonMessagingSubsystem();
COMMONGAME_API UClass* Z_Construct_UClass_UCommonMessagingSubsystem_NoRegister();
COMMONGAME_API UEnum* Z_Construct_UEnum_CommonGame_ECommonMessagingResult();
ENGINE_API UClass* Z_Construct_UClass_ULocalPlayerSubsystem();
UPackage* Z_Construct_UPackage__Script_CommonGame();
// End Cross Module References

// Begin Enum ECommonMessagingResult
static FEnumRegistrationInfo Z_Registration_Info_UEnum_ECommonMessagingResult;
static UEnum* ECommonMessagingResult_StaticEnum()
{
	if (!Z_Registration_Info_UEnum_ECommonMessagingResult.OuterSingleton)
	{
		Z_Registration_Info_UEnum_ECommonMessagingResult.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_CommonGame_ECommonMessagingResult, (UObject*)Z_Construct_UPackage__Script_CommonGame(), TEXT("ECommonMessagingResult"));
	}
	return Z_Registration_Info_UEnum_ECommonMessagingResult.OuterSingleton;
}
template<> COMMONGAME_API UEnum* StaticEnum<ECommonMessagingResult>()
{
	return ECommonMessagingResult_StaticEnum();
}
struct Z_Construct_UEnum_CommonGame_ECommonMessagingResult_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "Cancelled.Comment", "/** The \"ignore/cancel\" button was pressed */" },
		{ "Cancelled.Name", "ECommonMessagingResult::Cancelled" },
		{ "Cancelled.ToolTip", "The \"ignore/cancel\" button was pressed" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Possible results from a dialog */" },
#endif
		{ "Confirmed.Comment", "/** The \"yes\" button was pressed */" },
		{ "Confirmed.Name", "ECommonMessagingResult::Confirmed" },
		{ "Confirmed.ToolTip", "The \"yes\" button was pressed" },
		{ "Declined.Comment", "/** The \"no\" button was pressed */" },
		{ "Declined.Name", "ECommonMessagingResult::Declined" },
		{ "Declined.ToolTip", "The \"no\" button was pressed" },
		{ "Killed.Comment", "/** The dialog was explicitly killed (no user input) */" },
		{ "Killed.Name", "ECommonMessagingResult::Killed" },
		{ "Killed.ToolTip", "The dialog was explicitly killed (no user input)" },
		{ "ModuleRelativePath", "Public/Messaging/CommonMessagingSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Possible results from a dialog" },
#endif
		{ "Unknown.Hidden", "" },
		{ "Unknown.Name", "ECommonMessagingResult::Unknown" },
	};
#endif // WITH_METADATA
	static constexpr UECodeGen_Private::FEnumeratorParam Enumerators[] = {
		{ "ECommonMessagingResult::Confirmed", (int64)ECommonMessagingResult::Confirmed },
		{ "ECommonMessagingResult::Declined", (int64)ECommonMessagingResult::Declined },
		{ "ECommonMessagingResult::Cancelled", (int64)ECommonMessagingResult::Cancelled },
		{ "ECommonMessagingResult::Killed", (int64)ECommonMessagingResult::Killed },
		{ "ECommonMessagingResult::Unknown", (int64)ECommonMessagingResult::Unknown },
	};
	static const UECodeGen_Private::FEnumParams EnumParams;
};
const UECodeGen_Private::FEnumParams Z_Construct_UEnum_CommonGame_ECommonMessagingResult_Statics::EnumParams = {
	(UObject*(*)())Z_Construct_UPackage__Script_CommonGame,
	nullptr,
	"ECommonMessagingResult",
	"ECommonMessagingResult",
	Z_Construct_UEnum_CommonGame_ECommonMessagingResult_Statics::Enumerators,
	RF_Public|RF_Transient|RF_MarkAsNative,
	UE_ARRAY_COUNT(Z_Construct_UEnum_CommonGame_ECommonMessagingResult_Statics::Enumerators),
	EEnumFlags::None,
	(uint8)UEnum::ECppForm::EnumClass,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UEnum_CommonGame_ECommonMessagingResult_Statics::Enum_MetaDataParams), Z_Construct_UEnum_CommonGame_ECommonMessagingResult_Statics::Enum_MetaDataParams)
};
UEnum* Z_Construct_UEnum_CommonGame_ECommonMessagingResult()
{
	if (!Z_Registration_Info_UEnum_ECommonMessagingResult.InnerSingleton)
	{
		UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_ECommonMessagingResult.InnerSingleton, Z_Construct_UEnum_CommonGame_ECommonMessagingResult_Statics::EnumParams);
	}
	return Z_Registration_Info_UEnum_ECommonMessagingResult.InnerSingleton;
}
// End Enum ECommonMessagingResult

// Begin Class UCommonMessagingSubsystem
void UCommonMessagingSubsystem::StaticRegisterNativesUCommonMessagingSubsystem()
{
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UCommonMessagingSubsystem);
UClass* Z_Construct_UClass_UCommonMessagingSubsystem_NoRegister()
{
	return UCommonMessagingSubsystem::StaticClass();
}
struct Z_Construct_UClass_UCommonMessagingSubsystem_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * \n */" },
#endif
		{ "IncludePath", "Messaging/CommonMessagingSubsystem.h" },
		{ "ModuleRelativePath", "Public/Messaging/CommonMessagingSubsystem.h" },
	};
#endif // WITH_METADATA
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UCommonMessagingSubsystem>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
UObject* (*const Z_Construct_UClass_UCommonMessagingSubsystem_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_ULocalPlayerSubsystem,
	(UObject* (*)())Z_Construct_UPackage__Script_CommonGame,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonMessagingSubsystem_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UCommonMessagingSubsystem_Statics::ClassParams = {
	&UCommonMessagingSubsystem::StaticClass,
	"Game",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	nullptr,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	0,
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonMessagingSubsystem_Statics::Class_MetaDataParams), Z_Construct_UClass_UCommonMessagingSubsystem_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UCommonMessagingSubsystem()
{
	if (!Z_Registration_Info_UClass_UCommonMessagingSubsystem.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UCommonMessagingSubsystem.OuterSingleton, Z_Construct_UClass_UCommonMessagingSubsystem_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UCommonMessagingSubsystem.OuterSingleton;
}
template<> COMMONGAME_API UClass* StaticClass<UCommonMessagingSubsystem>()
{
	return UCommonMessagingSubsystem::StaticClass();
}
DEFINE_VTABLE_PTR_HELPER_CTOR(UCommonMessagingSubsystem);
UCommonMessagingSubsystem::~UCommonMessagingSubsystem() {}
// End Class UCommonMessagingSubsystem

// Begin Registration
struct Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonMessagingSubsystem_h_Statics
{
	static constexpr FEnumRegisterCompiledInInfo EnumInfo[] = {
		{ ECommonMessagingResult_StaticEnum, TEXT("ECommonMessagingResult"), &Z_Registration_Info_UEnum_ECommonMessagingResult, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 3451816972U) },
	};
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UCommonMessagingSubsystem, UCommonMessagingSubsystem::StaticClass, TEXT("UCommonMessagingSubsystem"), &Z_Registration_Info_UClass_UCommonMessagingSubsystem, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UCommonMessagingSubsystem), 3160180513U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonMessagingSubsystem_h_2953471966(TEXT("/Script/CommonGame"),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonMessagingSubsystem_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonMessagingSubsystem_h_Statics::ClassInfo),
	nullptr, 0,
	Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonMessagingSubsystem_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonMessagingSubsystem_h_Statics::EnumInfo));
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
