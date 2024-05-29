// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "CommonUser/Public/CommonSessionSubsystem.h"
#include "CommonUser/Public/CommonUserTypes.h"
#include "Runtime/Engine/Classes/Engine/GameInstance.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeCommonSessionSubsystem() {}

// Begin Cross Module References
COMMONUSER_API UClass* Z_Construct_UClass_UCommonSession_HostSessionRequest();
COMMONUSER_API UClass* Z_Construct_UClass_UCommonSession_HostSessionRequest_NoRegister();
COMMONUSER_API UClass* Z_Construct_UClass_UCommonSession_SearchResult();
COMMONUSER_API UClass* Z_Construct_UClass_UCommonSession_SearchResult_NoRegister();
COMMONUSER_API UClass* Z_Construct_UClass_UCommonSession_SearchSessionRequest();
COMMONUSER_API UClass* Z_Construct_UClass_UCommonSession_SearchSessionRequest_NoRegister();
COMMONUSER_API UClass* Z_Construct_UClass_UCommonSessionSubsystem();
COMMONUSER_API UClass* Z_Construct_UClass_UCommonSessionSubsystem_NoRegister();
COMMONUSER_API UEnum* Z_Construct_UEnum_CommonUser_ECommonSessionInformationState();
COMMONUSER_API UEnum* Z_Construct_UEnum_CommonUser_ECommonSessionOnlineMode();
COMMONUSER_API UFunction* Z_Construct_UDelegateFunction_CommonUser_CommonSession_FindSessionsFinishedDynamic__DelegateSignature();
COMMONUSER_API UFunction* Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnCreateSessionComplete_Dynamic__DelegateSignature();
COMMONUSER_API UFunction* Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnJoinSessionComplete_Dynamic__DelegateSignature();
COMMONUSER_API UFunction* Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnSessionInformationChanged_Dynamic__DelegateSignature();
COMMONUSER_API UFunction* Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnUserRequestedSession_Dynamic__DelegateSignature();
COMMONUSER_API UScriptStruct* Z_Construct_UScriptStruct_FOnlineResultInformation();
COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FPlatformUserId();
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FPrimaryAssetId();
ENGINE_API UClass* Z_Construct_UClass_APlayerController_NoRegister();
ENGINE_API UClass* Z_Construct_UClass_UGameInstanceSubsystem();
UPackage* Z_Construct_UPackage__Script_CommonUser();
// End Cross Module References

// Begin Enum ECommonSessionOnlineMode
static FEnumRegistrationInfo Z_Registration_Info_UEnum_ECommonSessionOnlineMode;
static UEnum* ECommonSessionOnlineMode_StaticEnum()
{
	if (!Z_Registration_Info_UEnum_ECommonSessionOnlineMode.OuterSingleton)
	{
		Z_Registration_Info_UEnum_ECommonSessionOnlineMode.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_CommonUser_ECommonSessionOnlineMode, (UObject*)Z_Construct_UPackage__Script_CommonUser(), TEXT("ECommonSessionOnlineMode"));
	}
	return Z_Registration_Info_UEnum_ECommonSessionOnlineMode.OuterSingleton;
}
template<> COMMONUSER_API UEnum* StaticEnum<ECommonSessionOnlineMode>()
{
	return ECommonSessionOnlineMode_StaticEnum();
}
struct Z_Construct_UEnum_CommonUser_ECommonSessionOnlineMode_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Specifies the online features and connectivity that should be used for a game session */" },
#endif
		{ "LAN.Name", "ECommonSessionOnlineMode::LAN" },
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
		{ "Offline.Name", "ECommonSessionOnlineMode::Offline" },
		{ "Online.Name", "ECommonSessionOnlineMode::Online" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Specifies the online features and connectivity that should be used for a game session" },
#endif
	};
#endif // WITH_METADATA
	static constexpr UECodeGen_Private::FEnumeratorParam Enumerators[] = {
		{ "ECommonSessionOnlineMode::Offline", (int64)ECommonSessionOnlineMode::Offline },
		{ "ECommonSessionOnlineMode::LAN", (int64)ECommonSessionOnlineMode::LAN },
		{ "ECommonSessionOnlineMode::Online", (int64)ECommonSessionOnlineMode::Online },
	};
	static const UECodeGen_Private::FEnumParams EnumParams;
};
const UECodeGen_Private::FEnumParams Z_Construct_UEnum_CommonUser_ECommonSessionOnlineMode_Statics::EnumParams = {
	(UObject*(*)())Z_Construct_UPackage__Script_CommonUser,
	nullptr,
	"ECommonSessionOnlineMode",
	"ECommonSessionOnlineMode",
	Z_Construct_UEnum_CommonUser_ECommonSessionOnlineMode_Statics::Enumerators,
	RF_Public|RF_Transient|RF_MarkAsNative,
	UE_ARRAY_COUNT(Z_Construct_UEnum_CommonUser_ECommonSessionOnlineMode_Statics::Enumerators),
	EEnumFlags::None,
	(uint8)UEnum::ECppForm::EnumClass,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UEnum_CommonUser_ECommonSessionOnlineMode_Statics::Enum_MetaDataParams), Z_Construct_UEnum_CommonUser_ECommonSessionOnlineMode_Statics::Enum_MetaDataParams)
};
UEnum* Z_Construct_UEnum_CommonUser_ECommonSessionOnlineMode()
{
	if (!Z_Registration_Info_UEnum_ECommonSessionOnlineMode.InnerSingleton)
	{
		UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_ECommonSessionOnlineMode.InnerSingleton, Z_Construct_UEnum_CommonUser_ECommonSessionOnlineMode_Statics::EnumParams);
	}
	return Z_Registration_Info_UEnum_ECommonSessionOnlineMode.InnerSingleton;
}
// End Enum ECommonSessionOnlineMode

// Begin Class UCommonSession_HostSessionRequest
void UCommonSession_HostSessionRequest::StaticRegisterNativesUCommonSession_HostSessionRequest()
{
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UCommonSession_HostSessionRequest);
UClass* Z_Construct_UClass_UCommonSession_HostSessionRequest_NoRegister()
{
	return UCommonSession_HostSessionRequest::StaticClass();
}
struct Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** A request object that stores the parameters used when hosting a gameplay session */" },
#endif
		{ "IncludePath", "CommonSessionSubsystem.h" },
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "A request object that stores the parameters used when hosting a gameplay session" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_OnlineMode_MetaData[] = {
		{ "Category", "Session" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Indicates if the session is a full online session or a different type */" },
#endif
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Indicates if the session is a full online session or a different type" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_bUseLobbies_MetaData[] = {
		{ "Category", "Session" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** True if this request should create a player-hosted lobbies if available */" },
#endif
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "True if this request should create a player-hosted lobbies if available" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ModeNameForAdvertisement_MetaData[] = {
		{ "Category", "Session" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** String used during matchmaking to specify what type of game mode this is */" },
#endif
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "String used during matchmaking to specify what type of game mode this is" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_MapID_MetaData[] = {
		{ "AllowedTypes", "World" },
		{ "Category", "Session" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** The map that will be loaded at the start of gameplay, this needs to be a valid Primary Asset top-level map */" },
#endif
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The map that will be loaded at the start of gameplay, this needs to be a valid Primary Asset top-level map" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ExtraArgs_MetaData[] = {
		{ "Category", "Session" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Extra arguments passed as URL options to the game */" },
#endif
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Extra arguments passed as URL options to the game" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_MaxPlayerCount_MetaData[] = {
		{ "Category", "Session" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Maximum players allowed per gameplay session */" },
#endif
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Maximum players allowed per gameplay session" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FBytePropertyParams NewProp_OnlineMode_Underlying;
	static const UECodeGen_Private::FEnumPropertyParams NewProp_OnlineMode;
	static void NewProp_bUseLobbies_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bUseLobbies;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ModeNameForAdvertisement;
	static const UECodeGen_Private::FStructPropertyParams NewProp_MapID;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ExtraArgs_ValueProp;
	static const UECodeGen_Private::FStrPropertyParams NewProp_ExtraArgs_Key_KeyProp;
	static const UECodeGen_Private::FMapPropertyParams NewProp_ExtraArgs;
	static const UECodeGen_Private::FIntPropertyParams NewProp_MaxPlayerCount;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UCommonSession_HostSessionRequest>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::NewProp_OnlineMode_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::NewProp_OnlineMode = { "OnlineMode", nullptr, (EPropertyFlags)0x0010000000000004, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonSession_HostSessionRequest, OnlineMode), Z_Construct_UEnum_CommonUser_ECommonSessionOnlineMode, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_OnlineMode_MetaData), NewProp_OnlineMode_MetaData) }; // 460294093
void Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::NewProp_bUseLobbies_SetBit(void* Obj)
{
	((UCommonSession_HostSessionRequest*)Obj)->bUseLobbies = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::NewProp_bUseLobbies = { "bUseLobbies", nullptr, (EPropertyFlags)0x0010000000000004, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(UCommonSession_HostSessionRequest), &Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::NewProp_bUseLobbies_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_bUseLobbies_MetaData), NewProp_bUseLobbies_MetaData) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::NewProp_ModeNameForAdvertisement = { "ModeNameForAdvertisement", nullptr, (EPropertyFlags)0x0010000000000004, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonSession_HostSessionRequest, ModeNameForAdvertisement), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ModeNameForAdvertisement_MetaData), NewProp_ModeNameForAdvertisement_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::NewProp_MapID = { "MapID", nullptr, (EPropertyFlags)0x0010000000000004, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonSession_HostSessionRequest, MapID), Z_Construct_UScriptStruct_FPrimaryAssetId, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_MapID_MetaData), NewProp_MapID_MetaData) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::NewProp_ExtraArgs_ValueProp = { "ExtraArgs", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 1, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::NewProp_ExtraArgs_Key_KeyProp = { "ExtraArgs_Key", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FMapPropertyParams Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::NewProp_ExtraArgs = { "ExtraArgs", nullptr, (EPropertyFlags)0x0010000000000004, UECodeGen_Private::EPropertyGenFlags::Map, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonSession_HostSessionRequest, ExtraArgs), EMapPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ExtraArgs_MetaData), NewProp_ExtraArgs_MetaData) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::NewProp_MaxPlayerCount = { "MaxPlayerCount", nullptr, (EPropertyFlags)0x0010000000000004, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonSession_HostSessionRequest, MaxPlayerCount), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_MaxPlayerCount_MetaData), NewProp_MaxPlayerCount_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::NewProp_OnlineMode_Underlying,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::NewProp_OnlineMode,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::NewProp_bUseLobbies,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::NewProp_ModeNameForAdvertisement,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::NewProp_MapID,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::NewProp_ExtraArgs_ValueProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::NewProp_ExtraArgs_Key_KeyProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::NewProp_ExtraArgs,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::NewProp_MaxPlayerCount,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UObject,
	(UObject* (*)())Z_Construct_UPackage__Script_CommonUser,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::ClassParams = {
	&UCommonSession_HostSessionRequest::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::PropPointers),
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::Class_MetaDataParams), Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UCommonSession_HostSessionRequest()
{
	if (!Z_Registration_Info_UClass_UCommonSession_HostSessionRequest.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UCommonSession_HostSessionRequest.OuterSingleton, Z_Construct_UClass_UCommonSession_HostSessionRequest_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UCommonSession_HostSessionRequest.OuterSingleton;
}
template<> COMMONUSER_API UClass* StaticClass<UCommonSession_HostSessionRequest>()
{
	return UCommonSession_HostSessionRequest::StaticClass();
}
UCommonSession_HostSessionRequest::UCommonSession_HostSessionRequest(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR(UCommonSession_HostSessionRequest);
UCommonSession_HostSessionRequest::~UCommonSession_HostSessionRequest() {}
// End Class UCommonSession_HostSessionRequest

// Begin Class UCommonSession_SearchResult Function GetDescription
struct Z_Construct_UFunction_UCommonSession_SearchResult_GetDescription_Statics
{
	struct CommonSession_SearchResult_eventGetDescription_Parms
	{
		FString ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Session" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Returns an internal description of the session, not meant to be human readable */" },
#endif
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Returns an internal description of the session, not meant to be human readable" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UCommonSession_SearchResult_GetDescription_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonSession_SearchResult_eventGetDescription_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonSession_SearchResult_GetDescription_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonSession_SearchResult_GetDescription_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSession_SearchResult_GetDescription_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonSession_SearchResult_GetDescription_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonSession_SearchResult, nullptr, "GetDescription", nullptr, nullptr, Z_Construct_UFunction_UCommonSession_SearchResult_GetDescription_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSession_SearchResult_GetDescription_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonSession_SearchResult_GetDescription_Statics::CommonSession_SearchResult_eventGetDescription_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSession_SearchResult_GetDescription_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonSession_SearchResult_GetDescription_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UCommonSession_SearchResult_GetDescription_Statics::CommonSession_SearchResult_eventGetDescription_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UCommonSession_SearchResult_GetDescription()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonSession_SearchResult_GetDescription_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UCommonSession_SearchResult::execGetDescription)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(FString*)Z_Param__Result=P_THIS->GetDescription();
	P_NATIVE_END;
}
// End Class UCommonSession_SearchResult Function GetDescription

// Begin Class UCommonSession_SearchResult Function GetIntSetting
struct Z_Construct_UFunction_UCommonSession_SearchResult_GetIntSetting_Statics
{
	struct CommonSession_SearchResult_eventGetIntSetting_Parms
	{
		FName Key;
		int32 Value;
		bool bFoundValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Sessions" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Gets an arbitrary integer setting, bFoundValue will be false if the setting does not exist */" },
#endif
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Gets an arbitrary integer setting, bFoundValue will be false if the setting does not exist" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FNamePropertyParams NewProp_Key;
	static const UECodeGen_Private::FIntPropertyParams NewProp_Value;
	static void NewProp_bFoundValue_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bFoundValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FNamePropertyParams Z_Construct_UFunction_UCommonSession_SearchResult_GetIntSetting_Statics::NewProp_Key = { "Key", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonSession_SearchResult_eventGetIntSetting_Parms, Key), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_UCommonSession_SearchResult_GetIntSetting_Statics::NewProp_Value = { "Value", nullptr, (EPropertyFlags)0x0010000000000180, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonSession_SearchResult_eventGetIntSetting_Parms, Value), METADATA_PARAMS(0, nullptr) };
void Z_Construct_UFunction_UCommonSession_SearchResult_GetIntSetting_Statics::NewProp_bFoundValue_SetBit(void* Obj)
{
	((CommonSession_SearchResult_eventGetIntSetting_Parms*)Obj)->bFoundValue = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UCommonSession_SearchResult_GetIntSetting_Statics::NewProp_bFoundValue = { "bFoundValue", nullptr, (EPropertyFlags)0x0010000000000180, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(CommonSession_SearchResult_eventGetIntSetting_Parms), &Z_Construct_UFunction_UCommonSession_SearchResult_GetIntSetting_Statics::NewProp_bFoundValue_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonSession_SearchResult_GetIntSetting_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonSession_SearchResult_GetIntSetting_Statics::NewProp_Key,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonSession_SearchResult_GetIntSetting_Statics::NewProp_Value,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonSession_SearchResult_GetIntSetting_Statics::NewProp_bFoundValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSession_SearchResult_GetIntSetting_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonSession_SearchResult_GetIntSetting_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonSession_SearchResult, nullptr, "GetIntSetting", nullptr, nullptr, Z_Construct_UFunction_UCommonSession_SearchResult_GetIntSetting_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSession_SearchResult_GetIntSetting_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonSession_SearchResult_GetIntSetting_Statics::CommonSession_SearchResult_eventGetIntSetting_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54420401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSession_SearchResult_GetIntSetting_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonSession_SearchResult_GetIntSetting_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UCommonSession_SearchResult_GetIntSetting_Statics::CommonSession_SearchResult_eventGetIntSetting_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UCommonSession_SearchResult_GetIntSetting()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonSession_SearchResult_GetIntSetting_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UCommonSession_SearchResult::execGetIntSetting)
{
	P_GET_PROPERTY(FNameProperty,Z_Param_Key);
	P_GET_PROPERTY_REF(FIntProperty,Z_Param_Out_Value);
	P_GET_UBOOL_REF(Z_Param_Out_bFoundValue);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->GetIntSetting(Z_Param_Key,Z_Param_Out_Value,Z_Param_Out_bFoundValue);
	P_NATIVE_END;
}
// End Class UCommonSession_SearchResult Function GetIntSetting

// Begin Class UCommonSession_SearchResult Function GetMaxPublicConnections
struct Z_Construct_UFunction_UCommonSession_SearchResult_GetMaxPublicConnections_Statics
{
	struct CommonSession_SearchResult_eventGetMaxPublicConnections_Parms
	{
		int32 ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Sessions" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** The maximum number of publicly available connections that could be available, including already filled connections */" },
#endif
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The maximum number of publicly available connections that could be available, including already filled connections" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FIntPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_UCommonSession_SearchResult_GetMaxPublicConnections_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonSession_SearchResult_eventGetMaxPublicConnections_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonSession_SearchResult_GetMaxPublicConnections_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonSession_SearchResult_GetMaxPublicConnections_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSession_SearchResult_GetMaxPublicConnections_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonSession_SearchResult_GetMaxPublicConnections_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonSession_SearchResult, nullptr, "GetMaxPublicConnections", nullptr, nullptr, Z_Construct_UFunction_UCommonSession_SearchResult_GetMaxPublicConnections_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSession_SearchResult_GetMaxPublicConnections_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonSession_SearchResult_GetMaxPublicConnections_Statics::CommonSession_SearchResult_eventGetMaxPublicConnections_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSession_SearchResult_GetMaxPublicConnections_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonSession_SearchResult_GetMaxPublicConnections_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UCommonSession_SearchResult_GetMaxPublicConnections_Statics::CommonSession_SearchResult_eventGetMaxPublicConnections_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UCommonSession_SearchResult_GetMaxPublicConnections()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonSession_SearchResult_GetMaxPublicConnections_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UCommonSession_SearchResult::execGetMaxPublicConnections)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(int32*)Z_Param__Result=P_THIS->GetMaxPublicConnections();
	P_NATIVE_END;
}
// End Class UCommonSession_SearchResult Function GetMaxPublicConnections

// Begin Class UCommonSession_SearchResult Function GetNumOpenPrivateConnections
struct Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPrivateConnections_Statics
{
	struct CommonSession_SearchResult_eventGetNumOpenPrivateConnections_Parms
	{
		int32 ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Sessions" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** The number of private connections that are available */" },
#endif
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The number of private connections that are available" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FIntPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPrivateConnections_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonSession_SearchResult_eventGetNumOpenPrivateConnections_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPrivateConnections_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPrivateConnections_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPrivateConnections_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPrivateConnections_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonSession_SearchResult, nullptr, "GetNumOpenPrivateConnections", nullptr, nullptr, Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPrivateConnections_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPrivateConnections_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPrivateConnections_Statics::CommonSession_SearchResult_eventGetNumOpenPrivateConnections_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPrivateConnections_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPrivateConnections_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPrivateConnections_Statics::CommonSession_SearchResult_eventGetNumOpenPrivateConnections_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPrivateConnections()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPrivateConnections_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UCommonSession_SearchResult::execGetNumOpenPrivateConnections)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(int32*)Z_Param__Result=P_THIS->GetNumOpenPrivateConnections();
	P_NATIVE_END;
}
// End Class UCommonSession_SearchResult Function GetNumOpenPrivateConnections

// Begin Class UCommonSession_SearchResult Function GetNumOpenPublicConnections
struct Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPublicConnections_Statics
{
	struct CommonSession_SearchResult_eventGetNumOpenPublicConnections_Parms
	{
		int32 ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Sessions" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** The number of publicly available connections that are available */" },
#endif
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The number of publicly available connections that are available" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FIntPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPublicConnections_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonSession_SearchResult_eventGetNumOpenPublicConnections_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPublicConnections_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPublicConnections_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPublicConnections_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPublicConnections_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonSession_SearchResult, nullptr, "GetNumOpenPublicConnections", nullptr, nullptr, Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPublicConnections_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPublicConnections_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPublicConnections_Statics::CommonSession_SearchResult_eventGetNumOpenPublicConnections_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPublicConnections_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPublicConnections_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPublicConnections_Statics::CommonSession_SearchResult_eventGetNumOpenPublicConnections_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPublicConnections()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPublicConnections_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UCommonSession_SearchResult::execGetNumOpenPublicConnections)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(int32*)Z_Param__Result=P_THIS->GetNumOpenPublicConnections();
	P_NATIVE_END;
}
// End Class UCommonSession_SearchResult Function GetNumOpenPublicConnections

// Begin Class UCommonSession_SearchResult Function GetPingInMs
struct Z_Construct_UFunction_UCommonSession_SearchResult_GetPingInMs_Statics
{
	struct CommonSession_SearchResult_eventGetPingInMs_Parms
	{
		int32 ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Sessions" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Ping to the search result, MAX_QUERY_PING is unreachable */" },
#endif
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Ping to the search result, MAX_QUERY_PING is unreachable" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FIntPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_UCommonSession_SearchResult_GetPingInMs_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonSession_SearchResult_eventGetPingInMs_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonSession_SearchResult_GetPingInMs_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonSession_SearchResult_GetPingInMs_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSession_SearchResult_GetPingInMs_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonSession_SearchResult_GetPingInMs_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonSession_SearchResult, nullptr, "GetPingInMs", nullptr, nullptr, Z_Construct_UFunction_UCommonSession_SearchResult_GetPingInMs_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSession_SearchResult_GetPingInMs_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonSession_SearchResult_GetPingInMs_Statics::CommonSession_SearchResult_eventGetPingInMs_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSession_SearchResult_GetPingInMs_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonSession_SearchResult_GetPingInMs_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UCommonSession_SearchResult_GetPingInMs_Statics::CommonSession_SearchResult_eventGetPingInMs_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UCommonSession_SearchResult_GetPingInMs()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonSession_SearchResult_GetPingInMs_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UCommonSession_SearchResult::execGetPingInMs)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(int32*)Z_Param__Result=P_THIS->GetPingInMs();
	P_NATIVE_END;
}
// End Class UCommonSession_SearchResult Function GetPingInMs

// Begin Class UCommonSession_SearchResult Function GetStringSetting
struct Z_Construct_UFunction_UCommonSession_SearchResult_GetStringSetting_Statics
{
	struct CommonSession_SearchResult_eventGetStringSetting_Parms
	{
		FName Key;
		FString Value;
		bool bFoundValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Sessions" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Gets an arbitrary string setting, bFoundValue will be false if the setting does not exist */" },
#endif
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Gets an arbitrary string setting, bFoundValue will be false if the setting does not exist" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FNamePropertyParams NewProp_Key;
	static const UECodeGen_Private::FStrPropertyParams NewProp_Value;
	static void NewProp_bFoundValue_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bFoundValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FNamePropertyParams Z_Construct_UFunction_UCommonSession_SearchResult_GetStringSetting_Statics::NewProp_Key = { "Key", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonSession_SearchResult_eventGetStringSetting_Parms, Key), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UCommonSession_SearchResult_GetStringSetting_Statics::NewProp_Value = { "Value", nullptr, (EPropertyFlags)0x0010000000000180, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonSession_SearchResult_eventGetStringSetting_Parms, Value), METADATA_PARAMS(0, nullptr) };
void Z_Construct_UFunction_UCommonSession_SearchResult_GetStringSetting_Statics::NewProp_bFoundValue_SetBit(void* Obj)
{
	((CommonSession_SearchResult_eventGetStringSetting_Parms*)Obj)->bFoundValue = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UCommonSession_SearchResult_GetStringSetting_Statics::NewProp_bFoundValue = { "bFoundValue", nullptr, (EPropertyFlags)0x0010000000000180, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(CommonSession_SearchResult_eventGetStringSetting_Parms), &Z_Construct_UFunction_UCommonSession_SearchResult_GetStringSetting_Statics::NewProp_bFoundValue_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonSession_SearchResult_GetStringSetting_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonSession_SearchResult_GetStringSetting_Statics::NewProp_Key,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonSession_SearchResult_GetStringSetting_Statics::NewProp_Value,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonSession_SearchResult_GetStringSetting_Statics::NewProp_bFoundValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSession_SearchResult_GetStringSetting_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonSession_SearchResult_GetStringSetting_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonSession_SearchResult, nullptr, "GetStringSetting", nullptr, nullptr, Z_Construct_UFunction_UCommonSession_SearchResult_GetStringSetting_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSession_SearchResult_GetStringSetting_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonSession_SearchResult_GetStringSetting_Statics::CommonSession_SearchResult_eventGetStringSetting_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54420401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSession_SearchResult_GetStringSetting_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonSession_SearchResult_GetStringSetting_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UCommonSession_SearchResult_GetStringSetting_Statics::CommonSession_SearchResult_eventGetStringSetting_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UCommonSession_SearchResult_GetStringSetting()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonSession_SearchResult_GetStringSetting_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UCommonSession_SearchResult::execGetStringSetting)
{
	P_GET_PROPERTY(FNameProperty,Z_Param_Key);
	P_GET_PROPERTY_REF(FStrProperty,Z_Param_Out_Value);
	P_GET_UBOOL_REF(Z_Param_Out_bFoundValue);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->GetStringSetting(Z_Param_Key,Z_Param_Out_Value,Z_Param_Out_bFoundValue);
	P_NATIVE_END;
}
// End Class UCommonSession_SearchResult Function GetStringSetting

// Begin Class UCommonSession_SearchResult
void UCommonSession_SearchResult::StaticRegisterNativesUCommonSession_SearchResult()
{
	UClass* Class = UCommonSession_SearchResult::StaticClass();
	static const FNameNativePtrPair Funcs[] = {
		{ "GetDescription", &UCommonSession_SearchResult::execGetDescription },
		{ "GetIntSetting", &UCommonSession_SearchResult::execGetIntSetting },
		{ "GetMaxPublicConnections", &UCommonSession_SearchResult::execGetMaxPublicConnections },
		{ "GetNumOpenPrivateConnections", &UCommonSession_SearchResult::execGetNumOpenPrivateConnections },
		{ "GetNumOpenPublicConnections", &UCommonSession_SearchResult::execGetNumOpenPublicConnections },
		{ "GetPingInMs", &UCommonSession_SearchResult::execGetPingInMs },
		{ "GetStringSetting", &UCommonSession_SearchResult::execGetStringSetting },
	};
	FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UCommonSession_SearchResult);
UClass* Z_Construct_UClass_UCommonSession_SearchResult_NoRegister()
{
	return UCommonSession_SearchResult::StaticClass();
}
struct Z_Construct_UClass_UCommonSession_SearchResult_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** A result object returned from the online system that describes a joinable game session */" },
#endif
		{ "IncludePath", "CommonSessionSubsystem.h" },
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "A result object returned from the online system that describes a joinable game session" },
#endif
	};
#endif // WITH_METADATA
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UCommonSession_SearchResult_GetDescription, "GetDescription" }, // 608170585
		{ &Z_Construct_UFunction_UCommonSession_SearchResult_GetIntSetting, "GetIntSetting" }, // 2928586244
		{ &Z_Construct_UFunction_UCommonSession_SearchResult_GetMaxPublicConnections, "GetMaxPublicConnections" }, // 272758922
		{ &Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPrivateConnections, "GetNumOpenPrivateConnections" }, // 3833257865
		{ &Z_Construct_UFunction_UCommonSession_SearchResult_GetNumOpenPublicConnections, "GetNumOpenPublicConnections" }, // 1939368779
		{ &Z_Construct_UFunction_UCommonSession_SearchResult_GetPingInMs, "GetPingInMs" }, // 2436490444
		{ &Z_Construct_UFunction_UCommonSession_SearchResult_GetStringSetting, "GetStringSetting" }, // 2404706193
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UCommonSession_SearchResult>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
UObject* (*const Z_Construct_UClass_UCommonSession_SearchResult_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UObject,
	(UObject* (*)())Z_Construct_UPackage__Script_CommonUser,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonSession_SearchResult_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UCommonSession_SearchResult_Statics::ClassParams = {
	&UCommonSession_SearchResult::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	nullptr,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	0,
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonSession_SearchResult_Statics::Class_MetaDataParams), Z_Construct_UClass_UCommonSession_SearchResult_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UCommonSession_SearchResult()
{
	if (!Z_Registration_Info_UClass_UCommonSession_SearchResult.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UCommonSession_SearchResult.OuterSingleton, Z_Construct_UClass_UCommonSession_SearchResult_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UCommonSession_SearchResult.OuterSingleton;
}
template<> COMMONUSER_API UClass* StaticClass<UCommonSession_SearchResult>()
{
	return UCommonSession_SearchResult::StaticClass();
}
UCommonSession_SearchResult::UCommonSession_SearchResult(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR(UCommonSession_SearchResult);
UCommonSession_SearchResult::~UCommonSession_SearchResult() {}
// End Class UCommonSession_SearchResult

// Begin Delegate FCommonSession_FindSessionsFinishedDynamic
struct Z_Construct_UDelegateFunction_CommonUser_CommonSession_FindSessionsFinishedDynamic__DelegateSignature_Statics
{
	struct _Script_CommonUser_eventCommonSession_FindSessionsFinishedDynamic_Parms
	{
		bool bSucceeded;
		FText ErrorMessage;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
	};
#endif // WITH_METADATA
	static void NewProp_bSucceeded_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bSucceeded;
	static const UECodeGen_Private::FTextPropertyParams NewProp_ErrorMessage;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
void Z_Construct_UDelegateFunction_CommonUser_CommonSession_FindSessionsFinishedDynamic__DelegateSignature_Statics::NewProp_bSucceeded_SetBit(void* Obj)
{
	((_Script_CommonUser_eventCommonSession_FindSessionsFinishedDynamic_Parms*)Obj)->bSucceeded = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonSession_FindSessionsFinishedDynamic__DelegateSignature_Statics::NewProp_bSucceeded = { "bSucceeded", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(_Script_CommonUser_eventCommonSession_FindSessionsFinishedDynamic_Parms), &Z_Construct_UDelegateFunction_CommonUser_CommonSession_FindSessionsFinishedDynamic__DelegateSignature_Statics::NewProp_bSucceeded_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FTextPropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonSession_FindSessionsFinishedDynamic__DelegateSignature_Statics::NewProp_ErrorMessage = { "ErrorMessage", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_CommonUser_eventCommonSession_FindSessionsFinishedDynamic_Parms, ErrorMessage), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UDelegateFunction_CommonUser_CommonSession_FindSessionsFinishedDynamic__DelegateSignature_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonSession_FindSessionsFinishedDynamic__DelegateSignature_Statics::NewProp_bSucceeded,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonSession_FindSessionsFinishedDynamic__DelegateSignature_Statics::NewProp_ErrorMessage,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonSession_FindSessionsFinishedDynamic__DelegateSignature_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UDelegateFunction_CommonUser_CommonSession_FindSessionsFinishedDynamic__DelegateSignature_Statics::FuncParams = { (UObject*(*)())Z_Construct_UPackage__Script_CommonUser, nullptr, "CommonSession_FindSessionsFinishedDynamic__DelegateSignature", nullptr, nullptr, Z_Construct_UDelegateFunction_CommonUser_CommonSession_FindSessionsFinishedDynamic__DelegateSignature_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonSession_FindSessionsFinishedDynamic__DelegateSignature_Statics::PropPointers), sizeof(Z_Construct_UDelegateFunction_CommonUser_CommonSession_FindSessionsFinishedDynamic__DelegateSignature_Statics::_Script_CommonUser_eventCommonSession_FindSessionsFinishedDynamic_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00130000, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonSession_FindSessionsFinishedDynamic__DelegateSignature_Statics::Function_MetaDataParams), Z_Construct_UDelegateFunction_CommonUser_CommonSession_FindSessionsFinishedDynamic__DelegateSignature_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UDelegateFunction_CommonUser_CommonSession_FindSessionsFinishedDynamic__DelegateSignature_Statics::_Script_CommonUser_eventCommonSession_FindSessionsFinishedDynamic_Parms) < MAX_uint16);
UFunction* Z_Construct_UDelegateFunction_CommonUser_CommonSession_FindSessionsFinishedDynamic__DelegateSignature()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UDelegateFunction_CommonUser_CommonSession_FindSessionsFinishedDynamic__DelegateSignature_Statics::FuncParams);
	}
	return ReturnFunction;
}
void FCommonSession_FindSessionsFinishedDynamic_DelegateWrapper(const FMulticastScriptDelegate& CommonSession_FindSessionsFinishedDynamic, bool bSucceeded, const FText& ErrorMessage)
{
	struct _Script_CommonUser_eventCommonSession_FindSessionsFinishedDynamic_Parms
	{
		bool bSucceeded;
		FText ErrorMessage;
	};
	_Script_CommonUser_eventCommonSession_FindSessionsFinishedDynamic_Parms Parms;
	Parms.bSucceeded=bSucceeded ? true : false;
	Parms.ErrorMessage=ErrorMessage;
	CommonSession_FindSessionsFinishedDynamic.ProcessMulticastDelegate<UObject>(&Parms);
}
// End Delegate FCommonSession_FindSessionsFinishedDynamic

// Begin Class UCommonSession_SearchSessionRequest
void UCommonSession_SearchSessionRequest::StaticRegisterNativesUCommonSession_SearchSessionRequest()
{
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UCommonSession_SearchSessionRequest);
UClass* Z_Construct_UClass_UCommonSession_SearchSessionRequest_NoRegister()
{
	return UCommonSession_SearchSessionRequest::StaticClass();
}
struct Z_Construct_UClass_UCommonSession_SearchSessionRequest_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Request object describing a session search, this object will be updated once the search has completed */" },
#endif
		{ "IncludePath", "CommonSessionSubsystem.h" },
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Request object describing a session search, this object will be updated once the search has completed" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_OnlineMode_MetaData[] = {
		{ "Category", "Session" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Indicates if the this is looking for full online games or a different type like LAN */" },
#endif
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Indicates if the this is looking for full online games or a different type like LAN" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_bUseLobbies_MetaData[] = {
		{ "Category", "Session" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** True if this request should look for player-hosted lobbies if they are available, false will only search for registered server sessions */" },
#endif
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "True if this request should look for player-hosted lobbies if they are available, false will only search for registered server sessions" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Results_MetaData[] = {
		{ "Category", "Session" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** List of all found sessions, will be valid when OnSearchFinished is called */" },
#endif
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "List of all found sessions, will be valid when OnSearchFinished is called" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_K2_OnSearchFinished_MetaData[] = {
		{ "AllowPrivateAccess", "TRUE" },
		{ "Category", "Events" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Delegate called when a session search completes */" },
#endif
		{ "DisplayName", "On Search Finished" },
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Delegate called when a session search completes" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FBytePropertyParams NewProp_OnlineMode_Underlying;
	static const UECodeGen_Private::FEnumPropertyParams NewProp_OnlineMode;
	static void NewProp_bUseLobbies_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bUseLobbies;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Results_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_Results;
	static const UECodeGen_Private::FMulticastDelegatePropertyParams NewProp_K2_OnSearchFinished;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UCommonSession_SearchSessionRequest>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_UCommonSession_SearchSessionRequest_Statics::NewProp_OnlineMode_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_UCommonSession_SearchSessionRequest_Statics::NewProp_OnlineMode = { "OnlineMode", nullptr, (EPropertyFlags)0x0010000000000004, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonSession_SearchSessionRequest, OnlineMode), Z_Construct_UEnum_CommonUser_ECommonSessionOnlineMode, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_OnlineMode_MetaData), NewProp_OnlineMode_MetaData) }; // 460294093
void Z_Construct_UClass_UCommonSession_SearchSessionRequest_Statics::NewProp_bUseLobbies_SetBit(void* Obj)
{
	((UCommonSession_SearchSessionRequest*)Obj)->bUseLobbies = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_UCommonSession_SearchSessionRequest_Statics::NewProp_bUseLobbies = { "bUseLobbies", nullptr, (EPropertyFlags)0x0010000000000004, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(UCommonSession_SearchSessionRequest), &Z_Construct_UClass_UCommonSession_SearchSessionRequest_Statics::NewProp_bUseLobbies_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_bUseLobbies_MetaData), NewProp_bUseLobbies_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UCommonSession_SearchSessionRequest_Statics::NewProp_Results_Inner = { "Results", nullptr, (EPropertyFlags)0x0104000000000000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_UCommonSession_SearchResult_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_UCommonSession_SearchSessionRequest_Statics::NewProp_Results = { "Results", nullptr, (EPropertyFlags)0x0114000000000014, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonSession_SearchSessionRequest, Results), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Results_MetaData), NewProp_Results_MetaData) };
const UECodeGen_Private::FMulticastDelegatePropertyParams Z_Construct_UClass_UCommonSession_SearchSessionRequest_Statics::NewProp_K2_OnSearchFinished = { "K2_OnSearchFinished", nullptr, (EPropertyFlags)0x0040000010080000, UECodeGen_Private::EPropertyGenFlags::InlineMulticastDelegate, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonSession_SearchSessionRequest, K2_OnSearchFinished), Z_Construct_UDelegateFunction_CommonUser_CommonSession_FindSessionsFinishedDynamic__DelegateSignature, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_K2_OnSearchFinished_MetaData), NewProp_K2_OnSearchFinished_MetaData) }; // 1036648746
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UCommonSession_SearchSessionRequest_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonSession_SearchSessionRequest_Statics::NewProp_OnlineMode_Underlying,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonSession_SearchSessionRequest_Statics::NewProp_OnlineMode,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonSession_SearchSessionRequest_Statics::NewProp_bUseLobbies,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonSession_SearchSessionRequest_Statics::NewProp_Results_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonSession_SearchSessionRequest_Statics::NewProp_Results,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonSession_SearchSessionRequest_Statics::NewProp_K2_OnSearchFinished,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonSession_SearchSessionRequest_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UCommonSession_SearchSessionRequest_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UObject,
	(UObject* (*)())Z_Construct_UPackage__Script_CommonUser,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonSession_SearchSessionRequest_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UCommonSession_SearchSessionRequest_Statics::ClassParams = {
	&UCommonSession_SearchSessionRequest::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UCommonSession_SearchSessionRequest_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UCommonSession_SearchSessionRequest_Statics::PropPointers),
	0,
	0x009000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonSession_SearchSessionRequest_Statics::Class_MetaDataParams), Z_Construct_UClass_UCommonSession_SearchSessionRequest_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UCommonSession_SearchSessionRequest()
{
	if (!Z_Registration_Info_UClass_UCommonSession_SearchSessionRequest.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UCommonSession_SearchSessionRequest.OuterSingleton, Z_Construct_UClass_UCommonSession_SearchSessionRequest_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UCommonSession_SearchSessionRequest.OuterSingleton;
}
template<> COMMONUSER_API UClass* StaticClass<UCommonSession_SearchSessionRequest>()
{
	return UCommonSession_SearchSessionRequest::StaticClass();
}
UCommonSession_SearchSessionRequest::UCommonSession_SearchSessionRequest(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR(UCommonSession_SearchSessionRequest);
UCommonSession_SearchSessionRequest::~UCommonSession_SearchSessionRequest() {}
// End Class UCommonSession_SearchSessionRequest

// Begin Delegate FCommonSessionOnUserRequestedSession_Dynamic
struct Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnUserRequestedSession_Dynamic__DelegateSignature_Statics
{
	struct FPlatformUserId
	{
		int32 InternalId;
	};

	struct _Script_CommonUser_eventCommonSessionOnUserRequestedSession_Dynamic_Parms
	{
		FPlatformUserId LocalPlatformUserId;
		UCommonSession_SearchResult* RequestedSession;
		FOnlineResultInformation RequestedSessionResult;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LocalPlatformUserId_MetaData[] = {
		{ "NativeConst", "" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_RequestedSessionResult_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStructPropertyParams NewProp_LocalPlatformUserId;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_RequestedSession;
	static const UECodeGen_Private::FStructPropertyParams NewProp_RequestedSessionResult;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FStructPropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnUserRequestedSession_Dynamic__DelegateSignature_Statics::NewProp_LocalPlatformUserId = { "LocalPlatformUserId", nullptr, (EPropertyFlags)0x0010000008000182, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_CommonUser_eventCommonSessionOnUserRequestedSession_Dynamic_Parms, LocalPlatformUserId), Z_Construct_UScriptStruct_FPlatformUserId, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LocalPlatformUserId_MetaData), NewProp_LocalPlatformUserId_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnUserRequestedSession_Dynamic__DelegateSignature_Statics::NewProp_RequestedSession = { "RequestedSession", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_CommonUser_eventCommonSessionOnUserRequestedSession_Dynamic_Parms, RequestedSession), Z_Construct_UClass_UCommonSession_SearchResult_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnUserRequestedSession_Dynamic__DelegateSignature_Statics::NewProp_RequestedSessionResult = { "RequestedSessionResult", nullptr, (EPropertyFlags)0x0010000008000182, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_CommonUser_eventCommonSessionOnUserRequestedSession_Dynamic_Parms, RequestedSessionResult), Z_Construct_UScriptStruct_FOnlineResultInformation, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_RequestedSessionResult_MetaData), NewProp_RequestedSessionResult_MetaData) }; // 2359200286
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnUserRequestedSession_Dynamic__DelegateSignature_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnUserRequestedSession_Dynamic__DelegateSignature_Statics::NewProp_LocalPlatformUserId,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnUserRequestedSession_Dynamic__DelegateSignature_Statics::NewProp_RequestedSession,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnUserRequestedSession_Dynamic__DelegateSignature_Statics::NewProp_RequestedSessionResult,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnUserRequestedSession_Dynamic__DelegateSignature_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnUserRequestedSession_Dynamic__DelegateSignature_Statics::FuncParams = { (UObject*(*)())Z_Construct_UPackage__Script_CommonUser, nullptr, "CommonSessionOnUserRequestedSession_Dynamic__DelegateSignature", nullptr, nullptr, Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnUserRequestedSession_Dynamic__DelegateSignature_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnUserRequestedSession_Dynamic__DelegateSignature_Statics::PropPointers), sizeof(Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnUserRequestedSession_Dynamic__DelegateSignature_Statics::_Script_CommonUser_eventCommonSessionOnUserRequestedSession_Dynamic_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00130000, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnUserRequestedSession_Dynamic__DelegateSignature_Statics::Function_MetaDataParams), Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnUserRequestedSession_Dynamic__DelegateSignature_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnUserRequestedSession_Dynamic__DelegateSignature_Statics::_Script_CommonUser_eventCommonSessionOnUserRequestedSession_Dynamic_Parms) < MAX_uint16);
UFunction* Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnUserRequestedSession_Dynamic__DelegateSignature()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnUserRequestedSession_Dynamic__DelegateSignature_Statics::FuncParams);
	}
	return ReturnFunction;
}
void FCommonSessionOnUserRequestedSession_Dynamic_DelegateWrapper(const FMulticastScriptDelegate& CommonSessionOnUserRequestedSession_Dynamic, FPlatformUserId const& LocalPlatformUserId, UCommonSession_SearchResult* RequestedSession, FOnlineResultInformation const& RequestedSessionResult)
{
	struct _Script_CommonUser_eventCommonSessionOnUserRequestedSession_Dynamic_Parms
	{
		FPlatformUserId LocalPlatformUserId;
		UCommonSession_SearchResult* RequestedSession;
		FOnlineResultInformation RequestedSessionResult;
	};
	_Script_CommonUser_eventCommonSessionOnUserRequestedSession_Dynamic_Parms Parms;
	Parms.LocalPlatformUserId=LocalPlatformUserId;
	Parms.RequestedSession=RequestedSession;
	Parms.RequestedSessionResult=RequestedSessionResult;
	CommonSessionOnUserRequestedSession_Dynamic.ProcessMulticastDelegate<UObject>(&Parms);
}
// End Delegate FCommonSessionOnUserRequestedSession_Dynamic

// Begin Delegate FCommonSessionOnJoinSessionComplete_Dynamic
struct Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnJoinSessionComplete_Dynamic__DelegateSignature_Statics
{
	struct _Script_CommonUser_eventCommonSessionOnJoinSessionComplete_Dynamic_Parms
	{
		FOnlineResultInformation Result;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Result_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStructPropertyParams NewProp_Result;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FStructPropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnJoinSessionComplete_Dynamic__DelegateSignature_Statics::NewProp_Result = { "Result", nullptr, (EPropertyFlags)0x0010000008000182, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_CommonUser_eventCommonSessionOnJoinSessionComplete_Dynamic_Parms, Result), Z_Construct_UScriptStruct_FOnlineResultInformation, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Result_MetaData), NewProp_Result_MetaData) }; // 2359200286
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnJoinSessionComplete_Dynamic__DelegateSignature_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnJoinSessionComplete_Dynamic__DelegateSignature_Statics::NewProp_Result,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnJoinSessionComplete_Dynamic__DelegateSignature_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnJoinSessionComplete_Dynamic__DelegateSignature_Statics::FuncParams = { (UObject*(*)())Z_Construct_UPackage__Script_CommonUser, nullptr, "CommonSessionOnJoinSessionComplete_Dynamic__DelegateSignature", nullptr, nullptr, Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnJoinSessionComplete_Dynamic__DelegateSignature_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnJoinSessionComplete_Dynamic__DelegateSignature_Statics::PropPointers), sizeof(Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnJoinSessionComplete_Dynamic__DelegateSignature_Statics::_Script_CommonUser_eventCommonSessionOnJoinSessionComplete_Dynamic_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00130000, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnJoinSessionComplete_Dynamic__DelegateSignature_Statics::Function_MetaDataParams), Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnJoinSessionComplete_Dynamic__DelegateSignature_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnJoinSessionComplete_Dynamic__DelegateSignature_Statics::_Script_CommonUser_eventCommonSessionOnJoinSessionComplete_Dynamic_Parms) < MAX_uint16);
UFunction* Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnJoinSessionComplete_Dynamic__DelegateSignature()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnJoinSessionComplete_Dynamic__DelegateSignature_Statics::FuncParams);
	}
	return ReturnFunction;
}
void FCommonSessionOnJoinSessionComplete_Dynamic_DelegateWrapper(const FMulticastScriptDelegate& CommonSessionOnJoinSessionComplete_Dynamic, FOnlineResultInformation const& Result)
{
	struct _Script_CommonUser_eventCommonSessionOnJoinSessionComplete_Dynamic_Parms
	{
		FOnlineResultInformation Result;
	};
	_Script_CommonUser_eventCommonSessionOnJoinSessionComplete_Dynamic_Parms Parms;
	Parms.Result=Result;
	CommonSessionOnJoinSessionComplete_Dynamic.ProcessMulticastDelegate<UObject>(&Parms);
}
// End Delegate FCommonSessionOnJoinSessionComplete_Dynamic

// Begin Delegate FCommonSessionOnCreateSessionComplete_Dynamic
struct Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnCreateSessionComplete_Dynamic__DelegateSignature_Statics
{
	struct _Script_CommonUser_eventCommonSessionOnCreateSessionComplete_Dynamic_Parms
	{
		FOnlineResultInformation Result;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Result_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStructPropertyParams NewProp_Result;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FStructPropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnCreateSessionComplete_Dynamic__DelegateSignature_Statics::NewProp_Result = { "Result", nullptr, (EPropertyFlags)0x0010000008000182, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_CommonUser_eventCommonSessionOnCreateSessionComplete_Dynamic_Parms, Result), Z_Construct_UScriptStruct_FOnlineResultInformation, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Result_MetaData), NewProp_Result_MetaData) }; // 2359200286
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnCreateSessionComplete_Dynamic__DelegateSignature_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnCreateSessionComplete_Dynamic__DelegateSignature_Statics::NewProp_Result,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnCreateSessionComplete_Dynamic__DelegateSignature_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnCreateSessionComplete_Dynamic__DelegateSignature_Statics::FuncParams = { (UObject*(*)())Z_Construct_UPackage__Script_CommonUser, nullptr, "CommonSessionOnCreateSessionComplete_Dynamic__DelegateSignature", nullptr, nullptr, Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnCreateSessionComplete_Dynamic__DelegateSignature_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnCreateSessionComplete_Dynamic__DelegateSignature_Statics::PropPointers), sizeof(Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnCreateSessionComplete_Dynamic__DelegateSignature_Statics::_Script_CommonUser_eventCommonSessionOnCreateSessionComplete_Dynamic_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00130000, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnCreateSessionComplete_Dynamic__DelegateSignature_Statics::Function_MetaDataParams), Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnCreateSessionComplete_Dynamic__DelegateSignature_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnCreateSessionComplete_Dynamic__DelegateSignature_Statics::_Script_CommonUser_eventCommonSessionOnCreateSessionComplete_Dynamic_Parms) < MAX_uint16);
UFunction* Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnCreateSessionComplete_Dynamic__DelegateSignature()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnCreateSessionComplete_Dynamic__DelegateSignature_Statics::FuncParams);
	}
	return ReturnFunction;
}
void FCommonSessionOnCreateSessionComplete_Dynamic_DelegateWrapper(const FMulticastScriptDelegate& CommonSessionOnCreateSessionComplete_Dynamic, FOnlineResultInformation const& Result)
{
	struct _Script_CommonUser_eventCommonSessionOnCreateSessionComplete_Dynamic_Parms
	{
		FOnlineResultInformation Result;
	};
	_Script_CommonUser_eventCommonSessionOnCreateSessionComplete_Dynamic_Parms Parms;
	Parms.Result=Result;
	CommonSessionOnCreateSessionComplete_Dynamic.ProcessMulticastDelegate<UObject>(&Parms);
}
// End Delegate FCommonSessionOnCreateSessionComplete_Dynamic

// Begin Enum ECommonSessionInformationState
static FEnumRegistrationInfo Z_Registration_Info_UEnum_ECommonSessionInformationState;
static UEnum* ECommonSessionInformationState_StaticEnum()
{
	if (!Z_Registration_Info_UEnum_ECommonSessionInformationState.OuterSingleton)
	{
		Z_Registration_Info_UEnum_ECommonSessionInformationState.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_CommonUser_ECommonSessionInformationState, (UObject*)Z_Construct_UPackage__Script_CommonUser(), TEXT("ECommonSessionInformationState"));
	}
	return Z_Registration_Info_UEnum_ECommonSessionInformationState.OuterSingleton;
}
template<> COMMONUSER_API UEnum* StaticEnum<ECommonSessionInformationState>()
{
	return ECommonSessionInformationState_StaticEnum();
}
struct Z_Construct_UEnum_CommonUser_ECommonSessionInformationState_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Event triggered at different points in the session ecosystem that represent a user-presentable state of the session.\n * This should not be used for online functionality (use OnCreateSessionComplete or OnJoinSessionComplete for those) but for features such as rich presence\n */" },
#endif
		{ "InGame.Name", "ECommonSessionInformationState::InGame" },
		{ "Matchmaking.Name", "ECommonSessionInformationState::Matchmaking" },
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
		{ "OutOfGame.Name", "ECommonSessionInformationState::OutOfGame" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Event triggered at different points in the session ecosystem that represent a user-presentable state of the session.\nThis should not be used for online functionality (use OnCreateSessionComplete or OnJoinSessionComplete for those) but for features such as rich presence" },
#endif
	};
#endif // WITH_METADATA
	static constexpr UECodeGen_Private::FEnumeratorParam Enumerators[] = {
		{ "ECommonSessionInformationState::OutOfGame", (int64)ECommonSessionInformationState::OutOfGame },
		{ "ECommonSessionInformationState::Matchmaking", (int64)ECommonSessionInformationState::Matchmaking },
		{ "ECommonSessionInformationState::InGame", (int64)ECommonSessionInformationState::InGame },
	};
	static const UECodeGen_Private::FEnumParams EnumParams;
};
const UECodeGen_Private::FEnumParams Z_Construct_UEnum_CommonUser_ECommonSessionInformationState_Statics::EnumParams = {
	(UObject*(*)())Z_Construct_UPackage__Script_CommonUser,
	nullptr,
	"ECommonSessionInformationState",
	"ECommonSessionInformationState",
	Z_Construct_UEnum_CommonUser_ECommonSessionInformationState_Statics::Enumerators,
	RF_Public|RF_Transient|RF_MarkAsNative,
	UE_ARRAY_COUNT(Z_Construct_UEnum_CommonUser_ECommonSessionInformationState_Statics::Enumerators),
	EEnumFlags::None,
	(uint8)UEnum::ECppForm::EnumClass,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UEnum_CommonUser_ECommonSessionInformationState_Statics::Enum_MetaDataParams), Z_Construct_UEnum_CommonUser_ECommonSessionInformationState_Statics::Enum_MetaDataParams)
};
UEnum* Z_Construct_UEnum_CommonUser_ECommonSessionInformationState()
{
	if (!Z_Registration_Info_UEnum_ECommonSessionInformationState.InnerSingleton)
	{
		UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_ECommonSessionInformationState.InnerSingleton, Z_Construct_UEnum_CommonUser_ECommonSessionInformationState_Statics::EnumParams);
	}
	return Z_Registration_Info_UEnum_ECommonSessionInformationState.InnerSingleton;
}
// End Enum ECommonSessionInformationState

// Begin Delegate FCommonSessionOnSessionInformationChanged_Dynamic
struct Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnSessionInformationChanged_Dynamic__DelegateSignature_Statics
{
	struct _Script_CommonUser_eventCommonSessionOnSessionInformationChanged_Dynamic_Parms
	{
		ECommonSessionInformationState SessionStatus;
		FString GameMode;
		FString MapName;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_GameMode_MetaData[] = {
		{ "NativeConst", "" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_MapName_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FBytePropertyParams NewProp_SessionStatus_Underlying;
	static const UECodeGen_Private::FEnumPropertyParams NewProp_SessionStatus;
	static const UECodeGen_Private::FStrPropertyParams NewProp_GameMode;
	static const UECodeGen_Private::FStrPropertyParams NewProp_MapName;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FBytePropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnSessionInformationChanged_Dynamic__DelegateSignature_Statics::NewProp_SessionStatus_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FEnumPropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnSessionInformationChanged_Dynamic__DelegateSignature_Statics::NewProp_SessionStatus = { "SessionStatus", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_CommonUser_eventCommonSessionOnSessionInformationChanged_Dynamic_Parms, SessionStatus), Z_Construct_UEnum_CommonUser_ECommonSessionInformationState, METADATA_PARAMS(0, nullptr) }; // 2137357761
const UECodeGen_Private::FStrPropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnSessionInformationChanged_Dynamic__DelegateSignature_Statics::NewProp_GameMode = { "GameMode", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_CommonUser_eventCommonSessionOnSessionInformationChanged_Dynamic_Parms, GameMode), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_GameMode_MetaData), NewProp_GameMode_MetaData) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnSessionInformationChanged_Dynamic__DelegateSignature_Statics::NewProp_MapName = { "MapName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_CommonUser_eventCommonSessionOnSessionInformationChanged_Dynamic_Parms, MapName), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_MapName_MetaData), NewProp_MapName_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnSessionInformationChanged_Dynamic__DelegateSignature_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnSessionInformationChanged_Dynamic__DelegateSignature_Statics::NewProp_SessionStatus_Underlying,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnSessionInformationChanged_Dynamic__DelegateSignature_Statics::NewProp_SessionStatus,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnSessionInformationChanged_Dynamic__DelegateSignature_Statics::NewProp_GameMode,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnSessionInformationChanged_Dynamic__DelegateSignature_Statics::NewProp_MapName,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnSessionInformationChanged_Dynamic__DelegateSignature_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnSessionInformationChanged_Dynamic__DelegateSignature_Statics::FuncParams = { (UObject*(*)())Z_Construct_UPackage__Script_CommonUser, nullptr, "CommonSessionOnSessionInformationChanged_Dynamic__DelegateSignature", nullptr, nullptr, Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnSessionInformationChanged_Dynamic__DelegateSignature_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnSessionInformationChanged_Dynamic__DelegateSignature_Statics::PropPointers), sizeof(Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnSessionInformationChanged_Dynamic__DelegateSignature_Statics::_Script_CommonUser_eventCommonSessionOnSessionInformationChanged_Dynamic_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00130000, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnSessionInformationChanged_Dynamic__DelegateSignature_Statics::Function_MetaDataParams), Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnSessionInformationChanged_Dynamic__DelegateSignature_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnSessionInformationChanged_Dynamic__DelegateSignature_Statics::_Script_CommonUser_eventCommonSessionOnSessionInformationChanged_Dynamic_Parms) < MAX_uint16);
UFunction* Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnSessionInformationChanged_Dynamic__DelegateSignature()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnSessionInformationChanged_Dynamic__DelegateSignature_Statics::FuncParams);
	}
	return ReturnFunction;
}
void FCommonSessionOnSessionInformationChanged_Dynamic_DelegateWrapper(const FMulticastScriptDelegate& CommonSessionOnSessionInformationChanged_Dynamic, ECommonSessionInformationState SessionStatus, const FString& GameMode, const FString& MapName)
{
	struct _Script_CommonUser_eventCommonSessionOnSessionInformationChanged_Dynamic_Parms
	{
		ECommonSessionInformationState SessionStatus;
		FString GameMode;
		FString MapName;
	};
	_Script_CommonUser_eventCommonSessionOnSessionInformationChanged_Dynamic_Parms Parms;
	Parms.SessionStatus=SessionStatus;
	Parms.GameMode=GameMode;
	Parms.MapName=MapName;
	CommonSessionOnSessionInformationChanged_Dynamic.ProcessMulticastDelegate<UObject>(&Parms);
}
// End Delegate FCommonSessionOnSessionInformationChanged_Dynamic

// Begin Class UCommonSessionSubsystem Function CleanUpSessions
struct Z_Construct_UFunction_UCommonSessionSubsystem_CleanUpSessions_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Session" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Clean up any active sessions, called from cases like returning to the main menu */" },
#endif
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Clean up any active sessions, called from cases like returning to the main menu" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonSessionSubsystem_CleanUpSessions_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonSessionSubsystem, nullptr, "CleanUpSessions", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020400, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSessionSubsystem_CleanUpSessions_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonSessionSubsystem_CleanUpSessions_Statics::Function_MetaDataParams) };
UFunction* Z_Construct_UFunction_UCommonSessionSubsystem_CleanUpSessions()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonSessionSubsystem_CleanUpSessions_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UCommonSessionSubsystem::execCleanUpSessions)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->CleanUpSessions();
	P_NATIVE_END;
}
// End Class UCommonSessionSubsystem Function CleanUpSessions

// Begin Class UCommonSessionSubsystem Function CreateOnlineHostSessionRequest
struct Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineHostSessionRequest_Statics
{
	struct CommonSessionSubsystem_eventCreateOnlineHostSessionRequest_Parms
	{
		UCommonSession_HostSessionRequest* ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Session" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Creates a host session request with default options for online games, this can be modified after creation */" },
#endif
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Creates a host session request with default options for online games, this can be modified after creation" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineHostSessionRequest_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonSessionSubsystem_eventCreateOnlineHostSessionRequest_Parms, ReturnValue), Z_Construct_UClass_UCommonSession_HostSessionRequest_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineHostSessionRequest_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineHostSessionRequest_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineHostSessionRequest_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineHostSessionRequest_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonSessionSubsystem, nullptr, "CreateOnlineHostSessionRequest", nullptr, nullptr, Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineHostSessionRequest_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineHostSessionRequest_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineHostSessionRequest_Statics::CommonSessionSubsystem_eventCreateOnlineHostSessionRequest_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020400, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineHostSessionRequest_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineHostSessionRequest_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineHostSessionRequest_Statics::CommonSessionSubsystem_eventCreateOnlineHostSessionRequest_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineHostSessionRequest()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineHostSessionRequest_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UCommonSessionSubsystem::execCreateOnlineHostSessionRequest)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(UCommonSession_HostSessionRequest**)Z_Param__Result=P_THIS->CreateOnlineHostSessionRequest();
	P_NATIVE_END;
}
// End Class UCommonSessionSubsystem Function CreateOnlineHostSessionRequest

// Begin Class UCommonSessionSubsystem Function CreateOnlineSearchSessionRequest
struct Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineSearchSessionRequest_Statics
{
	struct CommonSessionSubsystem_eventCreateOnlineSearchSessionRequest_Parms
	{
		UCommonSession_SearchSessionRequest* ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Session" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Creates a session search object with default options to look for default online games, this can be modified after creation */" },
#endif
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Creates a session search object with default options to look for default online games, this can be modified after creation" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineSearchSessionRequest_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonSessionSubsystem_eventCreateOnlineSearchSessionRequest_Parms, ReturnValue), Z_Construct_UClass_UCommonSession_SearchSessionRequest_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineSearchSessionRequest_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineSearchSessionRequest_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineSearchSessionRequest_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineSearchSessionRequest_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonSessionSubsystem, nullptr, "CreateOnlineSearchSessionRequest", nullptr, nullptr, Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineSearchSessionRequest_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineSearchSessionRequest_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineSearchSessionRequest_Statics::CommonSessionSubsystem_eventCreateOnlineSearchSessionRequest_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020400, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineSearchSessionRequest_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineSearchSessionRequest_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineSearchSessionRequest_Statics::CommonSessionSubsystem_eventCreateOnlineSearchSessionRequest_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineSearchSessionRequest()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineSearchSessionRequest_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UCommonSessionSubsystem::execCreateOnlineSearchSessionRequest)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(UCommonSession_SearchSessionRequest**)Z_Param__Result=P_THIS->CreateOnlineSearchSessionRequest();
	P_NATIVE_END;
}
// End Class UCommonSessionSubsystem Function CreateOnlineSearchSessionRequest

// Begin Class UCommonSessionSubsystem Function FindSessions
struct Z_Construct_UFunction_UCommonSessionSubsystem_FindSessions_Statics
{
	struct CommonSessionSubsystem_eventFindSessions_Parms
	{
		APlayerController* SearchingPlayer;
		UCommonSession_SearchSessionRequest* Request;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Session" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Queries online system for the list of joinable sessions matching the search request */" },
#endif
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Queries online system for the list of joinable sessions matching the search request" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_SearchingPlayer;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Request;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonSessionSubsystem_FindSessions_Statics::NewProp_SearchingPlayer = { "SearchingPlayer", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonSessionSubsystem_eventFindSessions_Parms, SearchingPlayer), Z_Construct_UClass_APlayerController_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonSessionSubsystem_FindSessions_Statics::NewProp_Request = { "Request", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonSessionSubsystem_eventFindSessions_Parms, Request), Z_Construct_UClass_UCommonSession_SearchSessionRequest_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonSessionSubsystem_FindSessions_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonSessionSubsystem_FindSessions_Statics::NewProp_SearchingPlayer,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonSessionSubsystem_FindSessions_Statics::NewProp_Request,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSessionSubsystem_FindSessions_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonSessionSubsystem_FindSessions_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonSessionSubsystem, nullptr, "FindSessions", nullptr, nullptr, Z_Construct_UFunction_UCommonSessionSubsystem_FindSessions_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSessionSubsystem_FindSessions_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonSessionSubsystem_FindSessions_Statics::CommonSessionSubsystem_eventFindSessions_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020400, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSessionSubsystem_FindSessions_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonSessionSubsystem_FindSessions_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UCommonSessionSubsystem_FindSessions_Statics::CommonSessionSubsystem_eventFindSessions_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UCommonSessionSubsystem_FindSessions()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonSessionSubsystem_FindSessions_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UCommonSessionSubsystem::execFindSessions)
{
	P_GET_OBJECT(APlayerController,Z_Param_SearchingPlayer);
	P_GET_OBJECT(UCommonSession_SearchSessionRequest,Z_Param_Request);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->FindSessions(Z_Param_SearchingPlayer,Z_Param_Request);
	P_NATIVE_END;
}
// End Class UCommonSessionSubsystem Function FindSessions

// Begin Class UCommonSessionSubsystem Function HostSession
struct Z_Construct_UFunction_UCommonSessionSubsystem_HostSession_Statics
{
	struct CommonSessionSubsystem_eventHostSession_Parms
	{
		APlayerController* HostingPlayer;
		UCommonSession_HostSessionRequest* Request;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Session" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Creates a new online game using the session request information, if successful this will start a hard map transfer */" },
#endif
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Creates a new online game using the session request information, if successful this will start a hard map transfer" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_HostingPlayer;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Request;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonSessionSubsystem_HostSession_Statics::NewProp_HostingPlayer = { "HostingPlayer", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonSessionSubsystem_eventHostSession_Parms, HostingPlayer), Z_Construct_UClass_APlayerController_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonSessionSubsystem_HostSession_Statics::NewProp_Request = { "Request", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonSessionSubsystem_eventHostSession_Parms, Request), Z_Construct_UClass_UCommonSession_HostSessionRequest_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonSessionSubsystem_HostSession_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonSessionSubsystem_HostSession_Statics::NewProp_HostingPlayer,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonSessionSubsystem_HostSession_Statics::NewProp_Request,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSessionSubsystem_HostSession_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonSessionSubsystem_HostSession_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonSessionSubsystem, nullptr, "HostSession", nullptr, nullptr, Z_Construct_UFunction_UCommonSessionSubsystem_HostSession_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSessionSubsystem_HostSession_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonSessionSubsystem_HostSession_Statics::CommonSessionSubsystem_eventHostSession_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020400, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSessionSubsystem_HostSession_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonSessionSubsystem_HostSession_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UCommonSessionSubsystem_HostSession_Statics::CommonSessionSubsystem_eventHostSession_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UCommonSessionSubsystem_HostSession()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonSessionSubsystem_HostSession_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UCommonSessionSubsystem::execHostSession)
{
	P_GET_OBJECT(APlayerController,Z_Param_HostingPlayer);
	P_GET_OBJECT(UCommonSession_HostSessionRequest,Z_Param_Request);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->HostSession(Z_Param_HostingPlayer,Z_Param_Request);
	P_NATIVE_END;
}
// End Class UCommonSessionSubsystem Function HostSession

// Begin Class UCommonSessionSubsystem Function JoinSession
struct Z_Construct_UFunction_UCommonSessionSubsystem_JoinSession_Statics
{
	struct CommonSessionSubsystem_eventJoinSession_Parms
	{
		APlayerController* JoiningPlayer;
		UCommonSession_SearchResult* Request;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Session" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Starts process to join an existing session, if successful this will connect to the specified server */" },
#endif
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Starts process to join an existing session, if successful this will connect to the specified server" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_JoiningPlayer;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Request;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonSessionSubsystem_JoinSession_Statics::NewProp_JoiningPlayer = { "JoiningPlayer", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonSessionSubsystem_eventJoinSession_Parms, JoiningPlayer), Z_Construct_UClass_APlayerController_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonSessionSubsystem_JoinSession_Statics::NewProp_Request = { "Request", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonSessionSubsystem_eventJoinSession_Parms, Request), Z_Construct_UClass_UCommonSession_SearchResult_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonSessionSubsystem_JoinSession_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonSessionSubsystem_JoinSession_Statics::NewProp_JoiningPlayer,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonSessionSubsystem_JoinSession_Statics::NewProp_Request,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSessionSubsystem_JoinSession_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonSessionSubsystem_JoinSession_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonSessionSubsystem, nullptr, "JoinSession", nullptr, nullptr, Z_Construct_UFunction_UCommonSessionSubsystem_JoinSession_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSessionSubsystem_JoinSession_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonSessionSubsystem_JoinSession_Statics::CommonSessionSubsystem_eventJoinSession_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020400, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSessionSubsystem_JoinSession_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonSessionSubsystem_JoinSession_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UCommonSessionSubsystem_JoinSession_Statics::CommonSessionSubsystem_eventJoinSession_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UCommonSessionSubsystem_JoinSession()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonSessionSubsystem_JoinSession_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UCommonSessionSubsystem::execJoinSession)
{
	P_GET_OBJECT(APlayerController,Z_Param_JoiningPlayer);
	P_GET_OBJECT(UCommonSession_SearchResult,Z_Param_Request);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->JoinSession(Z_Param_JoiningPlayer,Z_Param_Request);
	P_NATIVE_END;
}
// End Class UCommonSessionSubsystem Function JoinSession

// Begin Class UCommonSessionSubsystem Function QuickPlaySession
struct Z_Construct_UFunction_UCommonSessionSubsystem_QuickPlaySession_Statics
{
	struct CommonSessionSubsystem_eventQuickPlaySession_Parms
	{
		APlayerController* JoiningOrHostingPlayer;
		UCommonSession_HostSessionRequest* Request;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Session" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Starts a process to look for existing sessions or create a new one if no viable sessions are found */" },
#endif
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Starts a process to look for existing sessions or create a new one if no viable sessions are found" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_JoiningOrHostingPlayer;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Request;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonSessionSubsystem_QuickPlaySession_Statics::NewProp_JoiningOrHostingPlayer = { "JoiningOrHostingPlayer", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonSessionSubsystem_eventQuickPlaySession_Parms, JoiningOrHostingPlayer), Z_Construct_UClass_APlayerController_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonSessionSubsystem_QuickPlaySession_Statics::NewProp_Request = { "Request", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonSessionSubsystem_eventQuickPlaySession_Parms, Request), Z_Construct_UClass_UCommonSession_HostSessionRequest_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonSessionSubsystem_QuickPlaySession_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonSessionSubsystem_QuickPlaySession_Statics::NewProp_JoiningOrHostingPlayer,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonSessionSubsystem_QuickPlaySession_Statics::NewProp_Request,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSessionSubsystem_QuickPlaySession_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonSessionSubsystem_QuickPlaySession_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonSessionSubsystem, nullptr, "QuickPlaySession", nullptr, nullptr, Z_Construct_UFunction_UCommonSessionSubsystem_QuickPlaySession_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSessionSubsystem_QuickPlaySession_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonSessionSubsystem_QuickPlaySession_Statics::CommonSessionSubsystem_eventQuickPlaySession_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020400, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonSessionSubsystem_QuickPlaySession_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonSessionSubsystem_QuickPlaySession_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UCommonSessionSubsystem_QuickPlaySession_Statics::CommonSessionSubsystem_eventQuickPlaySession_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UCommonSessionSubsystem_QuickPlaySession()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonSessionSubsystem_QuickPlaySession_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UCommonSessionSubsystem::execQuickPlaySession)
{
	P_GET_OBJECT(APlayerController,Z_Param_JoiningOrHostingPlayer);
	P_GET_OBJECT(UCommonSession_HostSessionRequest,Z_Param_Request);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->QuickPlaySession(Z_Param_JoiningOrHostingPlayer,Z_Param_Request);
	P_NATIVE_END;
}
// End Class UCommonSessionSubsystem Function QuickPlaySession

// Begin Class UCommonSessionSubsystem
void UCommonSessionSubsystem::StaticRegisterNativesUCommonSessionSubsystem()
{
	UClass* Class = UCommonSessionSubsystem::StaticClass();
	static const FNameNativePtrPair Funcs[] = {
		{ "CleanUpSessions", &UCommonSessionSubsystem::execCleanUpSessions },
		{ "CreateOnlineHostSessionRequest", &UCommonSessionSubsystem::execCreateOnlineHostSessionRequest },
		{ "CreateOnlineSearchSessionRequest", &UCommonSessionSubsystem::execCreateOnlineSearchSessionRequest },
		{ "FindSessions", &UCommonSessionSubsystem::execFindSessions },
		{ "HostSession", &UCommonSessionSubsystem::execHostSession },
		{ "JoinSession", &UCommonSessionSubsystem::execJoinSession },
		{ "QuickPlaySession", &UCommonSessionSubsystem::execQuickPlaySession },
	};
	FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UCommonSessionSubsystem);
UClass* Z_Construct_UClass_UCommonSessionSubsystem_NoRegister()
{
	return UCommonSessionSubsystem::StaticClass();
}
struct Z_Construct_UClass_UCommonSessionSubsystem_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** \n * Game subsystem that handles requests for hosting and joining online games.\n * One subsystem is created for each game instance and can be accessed from blueprints or C++ code.\n * If a game-specific subclass exists, this base subsystem will not be created.\n */" },
#endif
		{ "IncludePath", "CommonSessionSubsystem.h" },
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Game subsystem that handles requests for hosting and joining online games.\nOne subsystem is created for each game instance and can be accessed from blueprints or C++ code.\nIf a game-specific subclass exists, this base subsystem will not be created." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_K2_OnUserRequestedSessionEvent_MetaData[] = {
		{ "Category", "Events" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Event broadcast when a local user has accepted an invite */" },
#endif
		{ "DisplayName", "On User Requested Session" },
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Event broadcast when a local user has accepted an invite" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_K2_OnJoinSessionCompleteEvent_MetaData[] = {
		{ "Category", "Events" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Event broadcast when a JoinSession call has completed */" },
#endif
		{ "DisplayName", "On Join Session Complete" },
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Event broadcast when a JoinSession call has completed" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_K2_OnCreateSessionCompleteEvent_MetaData[] = {
		{ "Category", "Events" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Event broadcast when a CreateSession call has completed */" },
#endif
		{ "DisplayName", "On Create Session Complete" },
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Event broadcast when a CreateSession call has completed" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_K2_OnSessionInformationChangedEvent_MetaData[] = {
		{ "Category", "Events" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Event broadcast when the presentable session information has changed */" },
#endif
		{ "DisplayName", "On Session Information Changed" },
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Event broadcast when the presentable session information has changed" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_bUseLobbiesDefault_MetaData[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Sets the default value of bUseLobbies for session search and host requests */" },
#endif
		{ "ModuleRelativePath", "Public/CommonSessionSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Sets the default value of bUseLobbies for session search and host requests" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FMulticastDelegatePropertyParams NewProp_K2_OnUserRequestedSessionEvent;
	static const UECodeGen_Private::FMulticastDelegatePropertyParams NewProp_K2_OnJoinSessionCompleteEvent;
	static const UECodeGen_Private::FMulticastDelegatePropertyParams NewProp_K2_OnCreateSessionCompleteEvent;
	static const UECodeGen_Private::FMulticastDelegatePropertyParams NewProp_K2_OnSessionInformationChangedEvent;
	static void NewProp_bUseLobbiesDefault_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bUseLobbiesDefault;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UCommonSessionSubsystem_CleanUpSessions, "CleanUpSessions" }, // 2106133469
		{ &Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineHostSessionRequest, "CreateOnlineHostSessionRequest" }, // 2791135289
		{ &Z_Construct_UFunction_UCommonSessionSubsystem_CreateOnlineSearchSessionRequest, "CreateOnlineSearchSessionRequest" }, // 4038963213
		{ &Z_Construct_UFunction_UCommonSessionSubsystem_FindSessions, "FindSessions" }, // 4268636026
		{ &Z_Construct_UFunction_UCommonSessionSubsystem_HostSession, "HostSession" }, // 3567579256
		{ &Z_Construct_UFunction_UCommonSessionSubsystem_JoinSession, "JoinSession" }, // 2376255695
		{ &Z_Construct_UFunction_UCommonSessionSubsystem_QuickPlaySession, "QuickPlaySession" }, // 3191143596
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UCommonSessionSubsystem>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FMulticastDelegatePropertyParams Z_Construct_UClass_UCommonSessionSubsystem_Statics::NewProp_K2_OnUserRequestedSessionEvent = { "K2_OnUserRequestedSessionEvent", nullptr, (EPropertyFlags)0x0010000010080000, UECodeGen_Private::EPropertyGenFlags::InlineMulticastDelegate, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonSessionSubsystem, K2_OnUserRequestedSessionEvent), Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnUserRequestedSession_Dynamic__DelegateSignature, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_K2_OnUserRequestedSessionEvent_MetaData), NewProp_K2_OnUserRequestedSessionEvent_MetaData) }; // 2188633137
const UECodeGen_Private::FMulticastDelegatePropertyParams Z_Construct_UClass_UCommonSessionSubsystem_Statics::NewProp_K2_OnJoinSessionCompleteEvent = { "K2_OnJoinSessionCompleteEvent", nullptr, (EPropertyFlags)0x0010000010080000, UECodeGen_Private::EPropertyGenFlags::InlineMulticastDelegate, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonSessionSubsystem, K2_OnJoinSessionCompleteEvent), Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnJoinSessionComplete_Dynamic__DelegateSignature, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_K2_OnJoinSessionCompleteEvent_MetaData), NewProp_K2_OnJoinSessionCompleteEvent_MetaData) }; // 1201277882
const UECodeGen_Private::FMulticastDelegatePropertyParams Z_Construct_UClass_UCommonSessionSubsystem_Statics::NewProp_K2_OnCreateSessionCompleteEvent = { "K2_OnCreateSessionCompleteEvent", nullptr, (EPropertyFlags)0x0010000010080000, UECodeGen_Private::EPropertyGenFlags::InlineMulticastDelegate, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonSessionSubsystem, K2_OnCreateSessionCompleteEvent), Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnCreateSessionComplete_Dynamic__DelegateSignature, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_K2_OnCreateSessionCompleteEvent_MetaData), NewProp_K2_OnCreateSessionCompleteEvent_MetaData) }; // 1271285163
const UECodeGen_Private::FMulticastDelegatePropertyParams Z_Construct_UClass_UCommonSessionSubsystem_Statics::NewProp_K2_OnSessionInformationChangedEvent = { "K2_OnSessionInformationChangedEvent", nullptr, (EPropertyFlags)0x0010000010080000, UECodeGen_Private::EPropertyGenFlags::InlineMulticastDelegate, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonSessionSubsystem, K2_OnSessionInformationChangedEvent), Z_Construct_UDelegateFunction_CommonUser_CommonSessionOnSessionInformationChanged_Dynamic__DelegateSignature, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_K2_OnSessionInformationChangedEvent_MetaData), NewProp_K2_OnSessionInformationChangedEvent_MetaData) }; // 2591526047
void Z_Construct_UClass_UCommonSessionSubsystem_Statics::NewProp_bUseLobbiesDefault_SetBit(void* Obj)
{
	((UCommonSessionSubsystem*)Obj)->bUseLobbiesDefault = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_UCommonSessionSubsystem_Statics::NewProp_bUseLobbiesDefault = { "bUseLobbiesDefault", nullptr, (EPropertyFlags)0x0010000000004000, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(UCommonSessionSubsystem), &Z_Construct_UClass_UCommonSessionSubsystem_Statics::NewProp_bUseLobbiesDefault_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_bUseLobbiesDefault_MetaData), NewProp_bUseLobbiesDefault_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UCommonSessionSubsystem_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonSessionSubsystem_Statics::NewProp_K2_OnUserRequestedSessionEvent,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonSessionSubsystem_Statics::NewProp_K2_OnJoinSessionCompleteEvent,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonSessionSubsystem_Statics::NewProp_K2_OnCreateSessionCompleteEvent,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonSessionSubsystem_Statics::NewProp_K2_OnSessionInformationChangedEvent,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonSessionSubsystem_Statics::NewProp_bUseLobbiesDefault,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonSessionSubsystem_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UCommonSessionSubsystem_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UGameInstanceSubsystem,
	(UObject* (*)())Z_Construct_UPackage__Script_CommonUser,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonSessionSubsystem_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UCommonSessionSubsystem_Statics::ClassParams = {
	&UCommonSessionSubsystem::StaticClass,
	"Engine",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	Z_Construct_UClass_UCommonSessionSubsystem_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	UE_ARRAY_COUNT(Z_Construct_UClass_UCommonSessionSubsystem_Statics::PropPointers),
	0,
	0x009000A4u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonSessionSubsystem_Statics::Class_MetaDataParams), Z_Construct_UClass_UCommonSessionSubsystem_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UCommonSessionSubsystem()
{
	if (!Z_Registration_Info_UClass_UCommonSessionSubsystem.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UCommonSessionSubsystem.OuterSingleton, Z_Construct_UClass_UCommonSessionSubsystem_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UCommonSessionSubsystem.OuterSingleton;
}
template<> COMMONUSER_API UClass* StaticClass<UCommonSessionSubsystem>()
{
	return UCommonSessionSubsystem::StaticClass();
}
DEFINE_VTABLE_PTR_HELPER_CTOR(UCommonSessionSubsystem);
UCommonSessionSubsystem::~UCommonSessionSubsystem() {}
// End Class UCommonSessionSubsystem

// Begin Registration
struct Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonSessionSubsystem_h_Statics
{
	static constexpr FEnumRegisterCompiledInInfo EnumInfo[] = {
		{ ECommonSessionOnlineMode_StaticEnum, TEXT("ECommonSessionOnlineMode"), &Z_Registration_Info_UEnum_ECommonSessionOnlineMode, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 460294093U) },
		{ ECommonSessionInformationState_StaticEnum, TEXT("ECommonSessionInformationState"), &Z_Registration_Info_UEnum_ECommonSessionInformationState, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 2137357761U) },
	};
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UCommonSession_HostSessionRequest, UCommonSession_HostSessionRequest::StaticClass, TEXT("UCommonSession_HostSessionRequest"), &Z_Registration_Info_UClass_UCommonSession_HostSessionRequest, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UCommonSession_HostSessionRequest), 1860404630U) },
		{ Z_Construct_UClass_UCommonSession_SearchResult, UCommonSession_SearchResult::StaticClass, TEXT("UCommonSession_SearchResult"), &Z_Registration_Info_UClass_UCommonSession_SearchResult, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UCommonSession_SearchResult), 2195757788U) },
		{ Z_Construct_UClass_UCommonSession_SearchSessionRequest, UCommonSession_SearchSessionRequest::StaticClass, TEXT("UCommonSession_SearchSessionRequest"), &Z_Registration_Info_UClass_UCommonSession_SearchSessionRequest, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UCommonSession_SearchSessionRequest), 2569571193U) },
		{ Z_Construct_UClass_UCommonSessionSubsystem, UCommonSessionSubsystem::StaticClass, TEXT("UCommonSessionSubsystem"), &Z_Registration_Info_UClass_UCommonSessionSubsystem, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UCommonSessionSubsystem), 634752007U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonSessionSubsystem_h_2578273388(TEXT("/Script/CommonUser"),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonSessionSubsystem_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonSessionSubsystem_h_Statics::ClassInfo),
	nullptr, 0,
	Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonSessionSubsystem_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonSessionSubsystem_h_Statics::EnumInfo));
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
