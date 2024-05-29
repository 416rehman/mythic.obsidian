// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/Players/MediaSubtitlesPlayer.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeMediaSubtitlesPlayer() {}

// Begin Cross Module References
COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
GAMESUBTITLES_API UClass* Z_Construct_UClass_UMediaSubtitlesPlayer();
GAMESUBTITLES_API UClass* Z_Construct_UClass_UMediaSubtitlesPlayer_NoRegister();
MEDIAASSETS_API UClass* Z_Construct_UClass_UMediaPlayer_NoRegister();
OVERLAY_API UClass* Z_Construct_UClass_UOverlays_NoRegister();
UPackage* Z_Construct_UPackage__Script_GameSubtitles();
// End Cross Module References

// Begin Class UMediaSubtitlesPlayer Function BindToMediaPlayer
struct Z_Construct_UFunction_UMediaSubtitlesPlayer_BindToMediaPlayer_Statics
{
	struct MediaSubtitlesPlayer_eventBindToMediaPlayer_Parms
	{
		UMediaPlayer* InMediaPlayer;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Game Subtitles|Subtitles Player" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Binds the subtitle playback to the tick of a media player. */" },
#endif
		{ "ModuleRelativePath", "Public/Players/MediaSubtitlesPlayer.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Binds the subtitle playback to the tick of a media player." },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_InMediaPlayer;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UMediaSubtitlesPlayer_BindToMediaPlayer_Statics::NewProp_InMediaPlayer = { "InMediaPlayer", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(MediaSubtitlesPlayer_eventBindToMediaPlayer_Parms, InMediaPlayer), Z_Construct_UClass_UMediaPlayer_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UMediaSubtitlesPlayer_BindToMediaPlayer_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UMediaSubtitlesPlayer_BindToMediaPlayer_Statics::NewProp_InMediaPlayer,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UMediaSubtitlesPlayer_BindToMediaPlayer_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UMediaSubtitlesPlayer_BindToMediaPlayer_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UMediaSubtitlesPlayer, nullptr, "BindToMediaPlayer", nullptr, nullptr, Z_Construct_UFunction_UMediaSubtitlesPlayer_BindToMediaPlayer_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UMediaSubtitlesPlayer_BindToMediaPlayer_Statics::PropPointers), sizeof(Z_Construct_UFunction_UMediaSubtitlesPlayer_BindToMediaPlayer_Statics::MediaSubtitlesPlayer_eventBindToMediaPlayer_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UMediaSubtitlesPlayer_BindToMediaPlayer_Statics::Function_MetaDataParams), Z_Construct_UFunction_UMediaSubtitlesPlayer_BindToMediaPlayer_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UMediaSubtitlesPlayer_BindToMediaPlayer_Statics::MediaSubtitlesPlayer_eventBindToMediaPlayer_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UMediaSubtitlesPlayer_BindToMediaPlayer()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UMediaSubtitlesPlayer_BindToMediaPlayer_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UMediaSubtitlesPlayer::execBindToMediaPlayer)
{
	P_GET_OBJECT(UMediaPlayer,Z_Param_InMediaPlayer);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->BindToMediaPlayer(Z_Param_InMediaPlayer);
	P_NATIVE_END;
}
// End Class UMediaSubtitlesPlayer Function BindToMediaPlayer

// Begin Class UMediaSubtitlesPlayer Function Play
struct Z_Construct_UFunction_UMediaSubtitlesPlayer_Play_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Game Subtitles|Subtitles Player" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Begins playing the currently set subtitles. */" },
#endif
		{ "ModuleRelativePath", "Public/Players/MediaSubtitlesPlayer.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Begins playing the currently set subtitles." },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UMediaSubtitlesPlayer_Play_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UMediaSubtitlesPlayer, nullptr, "Play", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UMediaSubtitlesPlayer_Play_Statics::Function_MetaDataParams), Z_Construct_UFunction_UMediaSubtitlesPlayer_Play_Statics::Function_MetaDataParams) };
UFunction* Z_Construct_UFunction_UMediaSubtitlesPlayer_Play()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UMediaSubtitlesPlayer_Play_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UMediaSubtitlesPlayer::execPlay)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->Play();
	P_NATIVE_END;
}
// End Class UMediaSubtitlesPlayer Function Play

// Begin Class UMediaSubtitlesPlayer Function SetSubtitles
struct Z_Construct_UFunction_UMediaSubtitlesPlayer_SetSubtitles_Statics
{
	struct MediaSubtitlesPlayer_eventSetSubtitles_Parms
	{
		UOverlays* Subtitles;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Game Subtitles|Subtitles Player" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Sets the source with the new subtitles set. */" },
#endif
		{ "ModuleRelativePath", "Public/Players/MediaSubtitlesPlayer.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Sets the source with the new subtitles set." },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Subtitles;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UMediaSubtitlesPlayer_SetSubtitles_Statics::NewProp_Subtitles = { "Subtitles", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(MediaSubtitlesPlayer_eventSetSubtitles_Parms, Subtitles), Z_Construct_UClass_UOverlays_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UMediaSubtitlesPlayer_SetSubtitles_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UMediaSubtitlesPlayer_SetSubtitles_Statics::NewProp_Subtitles,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UMediaSubtitlesPlayer_SetSubtitles_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UMediaSubtitlesPlayer_SetSubtitles_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UMediaSubtitlesPlayer, nullptr, "SetSubtitles", nullptr, nullptr, Z_Construct_UFunction_UMediaSubtitlesPlayer_SetSubtitles_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UMediaSubtitlesPlayer_SetSubtitles_Statics::PropPointers), sizeof(Z_Construct_UFunction_UMediaSubtitlesPlayer_SetSubtitles_Statics::MediaSubtitlesPlayer_eventSetSubtitles_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UMediaSubtitlesPlayer_SetSubtitles_Statics::Function_MetaDataParams), Z_Construct_UFunction_UMediaSubtitlesPlayer_SetSubtitles_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UMediaSubtitlesPlayer_SetSubtitles_Statics::MediaSubtitlesPlayer_eventSetSubtitles_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UMediaSubtitlesPlayer_SetSubtitles()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UMediaSubtitlesPlayer_SetSubtitles_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UMediaSubtitlesPlayer::execSetSubtitles)
{
	P_GET_OBJECT(UOverlays,Z_Param_Subtitles);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->SetSubtitles(Z_Param_Subtitles);
	P_NATIVE_END;
}
// End Class UMediaSubtitlesPlayer Function SetSubtitles

// Begin Class UMediaSubtitlesPlayer Function Stop
struct Z_Construct_UFunction_UMediaSubtitlesPlayer_Stop_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Game Subtitles|Subtitles Player" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Stops the subtitle player. */" },
#endif
		{ "ModuleRelativePath", "Public/Players/MediaSubtitlesPlayer.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Stops the subtitle player." },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UMediaSubtitlesPlayer_Stop_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UMediaSubtitlesPlayer, nullptr, "Stop", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UMediaSubtitlesPlayer_Stop_Statics::Function_MetaDataParams), Z_Construct_UFunction_UMediaSubtitlesPlayer_Stop_Statics::Function_MetaDataParams) };
UFunction* Z_Construct_UFunction_UMediaSubtitlesPlayer_Stop()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UMediaSubtitlesPlayer_Stop_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UMediaSubtitlesPlayer::execStop)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->Stop();
	P_NATIVE_END;
}
// End Class UMediaSubtitlesPlayer Function Stop

// Begin Class UMediaSubtitlesPlayer
void UMediaSubtitlesPlayer::StaticRegisterNativesUMediaSubtitlesPlayer()
{
	UClass* Class = UMediaSubtitlesPlayer::StaticClass();
	static const FNameNativePtrPair Funcs[] = {
		{ "BindToMediaPlayer", &UMediaSubtitlesPlayer::execBindToMediaPlayer },
		{ "Play", &UMediaSubtitlesPlayer::execPlay },
		{ "SetSubtitles", &UMediaSubtitlesPlayer::execSetSubtitles },
		{ "Stop", &UMediaSubtitlesPlayer::execStop },
	};
	FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UMediaSubtitlesPlayer);
UClass* Z_Construct_UClass_UMediaSubtitlesPlayer_NoRegister()
{
	return UMediaSubtitlesPlayer::StaticClass();
}
struct Z_Construct_UClass_UMediaSubtitlesPlayer_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * A Game-specific player for media subtitles. This needs to exist next to Media Players\n * and have its Play() / Pause() / Stop() methods called at the same time as the media players'\n * methods.\n */" },
#endif
		{ "IncludePath", "Players/MediaSubtitlesPlayer.h" },
		{ "ModuleRelativePath", "Public/Players/MediaSubtitlesPlayer.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "A Game-specific player for media subtitles. This needs to exist next to Media Players\nand have its Play() / Pause() / Stop() methods called at the same time as the media players'\nmethods." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_SourceSubtitles_MetaData[] = {
		{ "Category", "Subtitles Source" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** The subtitles to use for this player. */" },
#endif
		{ "ModuleRelativePath", "Public/Players/MediaSubtitlesPlayer.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The subtitles to use for this player." },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_SourceSubtitles;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UMediaSubtitlesPlayer_BindToMediaPlayer, "BindToMediaPlayer" }, // 2921213486
		{ &Z_Construct_UFunction_UMediaSubtitlesPlayer_Play, "Play" }, // 2460212252
		{ &Z_Construct_UFunction_UMediaSubtitlesPlayer_SetSubtitles, "SetSubtitles" }, // 3950086787
		{ &Z_Construct_UFunction_UMediaSubtitlesPlayer_Stop, "Stop" }, // 2591699617
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UMediaSubtitlesPlayer>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UMediaSubtitlesPlayer_Statics::NewProp_SourceSubtitles = { "SourceSubtitles", nullptr, (EPropertyFlags)0x0114000000000005, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UMediaSubtitlesPlayer, SourceSubtitles), Z_Construct_UClass_UOverlays_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_SourceSubtitles_MetaData), NewProp_SourceSubtitles_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UMediaSubtitlesPlayer_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UMediaSubtitlesPlayer_Statics::NewProp_SourceSubtitles,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UMediaSubtitlesPlayer_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UMediaSubtitlesPlayer_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UObject,
	(UObject* (*)())Z_Construct_UPackage__Script_GameSubtitles,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UMediaSubtitlesPlayer_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UMediaSubtitlesPlayer_Statics::ClassParams = {
	&UMediaSubtitlesPlayer::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	Z_Construct_UClass_UMediaSubtitlesPlayer_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	UE_ARRAY_COUNT(Z_Construct_UClass_UMediaSubtitlesPlayer_Statics::PropPointers),
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UMediaSubtitlesPlayer_Statics::Class_MetaDataParams), Z_Construct_UClass_UMediaSubtitlesPlayer_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UMediaSubtitlesPlayer()
{
	if (!Z_Registration_Info_UClass_UMediaSubtitlesPlayer.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UMediaSubtitlesPlayer.OuterSingleton, Z_Construct_UClass_UMediaSubtitlesPlayer_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UMediaSubtitlesPlayer.OuterSingleton;
}
template<> GAMESUBTITLES_API UClass* StaticClass<UMediaSubtitlesPlayer>()
{
	return UMediaSubtitlesPlayer::StaticClass();
}
DEFINE_VTABLE_PTR_HELPER_CTOR(UMediaSubtitlesPlayer);
UMediaSubtitlesPlayer::~UMediaSubtitlesPlayer() {}
// End Class UMediaSubtitlesPlayer

// Begin Registration
struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_Players_MediaSubtitlesPlayer_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UMediaSubtitlesPlayer, UMediaSubtitlesPlayer::StaticClass, TEXT("UMediaSubtitlesPlayer"), &Z_Registration_Info_UClass_UMediaSubtitlesPlayer, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UMediaSubtitlesPlayer), 182314167U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_Players_MediaSubtitlesPlayer_h_1470369624(TEXT("/Script/GameSubtitles"),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_Players_MediaSubtitlesPlayer_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_Players_MediaSubtitlesPlayer_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
