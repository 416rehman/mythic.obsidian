// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/Messaging/CommonGameDialog.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeCommonGameDialog() {}
// Cross Module References
	COMMONGAME_API UClass* Z_Construct_UClass_UCommonGameDialog();
	COMMONGAME_API UClass* Z_Construct_UClass_UCommonGameDialog_NoRegister();
	COMMONGAME_API UClass* Z_Construct_UClass_UCommonGameDialogDescriptor();
	COMMONGAME_API UClass* Z_Construct_UClass_UCommonGameDialogDescriptor_NoRegister();
	COMMONGAME_API UEnum* Z_Construct_UEnum_CommonGame_ECommonMessagingResult();
	COMMONGAME_API UScriptStruct* Z_Construct_UScriptStruct_FConfirmationDialogAction();
	COMMONUI_API UClass* Z_Construct_UClass_UCommonActivatableWidget();
	COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
	UPackage* Z_Construct_UPackage__Script_CommonGame();
// End Cross Module References
	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_ConfirmationDialogAction;
class UScriptStruct* FConfirmationDialogAction::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_ConfirmationDialogAction.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_ConfirmationDialogAction.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FConfirmationDialogAction, (UObject*)Z_Construct_UPackage__Script_CommonGame(), TEXT("ConfirmationDialogAction"));
	}
	return Z_Registration_Info_UScriptStruct_ConfirmationDialogAction.OuterSingleton;
}
template<> COMMONGAME_API UScriptStruct* StaticStruct<FConfirmationDialogAction>()
{
	return FConfirmationDialogAction::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FConfirmationDialogAction_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
		static const UECodeGen_Private::FBytePropertyParams NewProp_Result_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Result_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_Result;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_OptionalDisplayText_MetaData[];
#endif
		static const UECodeGen_Private::FTextPropertyParams NewProp_OptionalDisplayText;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FConfirmationDialogAction_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "Public/Messaging/CommonGameDialog.h" },
	};
#endif
	void* Z_Construct_UScriptStruct_FConfirmationDialogAction_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FConfirmationDialogAction>();
	}
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FConfirmationDialogAction_Statics::NewProp_Result_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FConfirmationDialogAction_Statics::NewProp_Result_MetaData[] = {
		{ "Category", "ConfirmationDialogAction" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Required: The dialog option to provide. */" },
#endif
		{ "ModuleRelativePath", "Public/Messaging/CommonGameDialog.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Required: The dialog option to provide." },
#endif
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FConfirmationDialogAction_Statics::NewProp_Result = { "Result", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FConfirmationDialogAction, Result), Z_Construct_UEnum_CommonGame_ECommonMessagingResult, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FConfirmationDialogAction_Statics::NewProp_Result_MetaData), Z_Construct_UScriptStruct_FConfirmationDialogAction_Statics::NewProp_Result_MetaData) }; // 921893972
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FConfirmationDialogAction_Statics::NewProp_OptionalDisplayText_MetaData[] = {
		{ "Category", "ConfirmationDialogAction" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Optional: Display Text to use instead of the action name associated with the result. */" },
#endif
		{ "ModuleRelativePath", "Public/Messaging/CommonGameDialog.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Optional: Display Text to use instead of the action name associated with the result." },
#endif
	};
#endif
	const UECodeGen_Private::FTextPropertyParams Z_Construct_UScriptStruct_FConfirmationDialogAction_Statics::NewProp_OptionalDisplayText = { "OptionalDisplayText", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FConfirmationDialogAction, OptionalDisplayText), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FConfirmationDialogAction_Statics::NewProp_OptionalDisplayText_MetaData), Z_Construct_UScriptStruct_FConfirmationDialogAction_Statics::NewProp_OptionalDisplayText_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FConfirmationDialogAction_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FConfirmationDialogAction_Statics::NewProp_Result_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FConfirmationDialogAction_Statics::NewProp_Result,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FConfirmationDialogAction_Statics::NewProp_OptionalDisplayText,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FConfirmationDialogAction_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_CommonGame,
		nullptr,
		&NewStructOps,
		"ConfirmationDialogAction",
		Z_Construct_UScriptStruct_FConfirmationDialogAction_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FConfirmationDialogAction_Statics::PropPointers),
		sizeof(FConfirmationDialogAction),
		alignof(FConfirmationDialogAction),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FConfirmationDialogAction_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FConfirmationDialogAction_Statics::Struct_MetaDataParams)
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FConfirmationDialogAction_Statics::PropPointers) < 2048);
	UScriptStruct* Z_Construct_UScriptStruct_FConfirmationDialogAction()
	{
		if (!Z_Registration_Info_UScriptStruct_ConfirmationDialogAction.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_ConfirmationDialogAction.InnerSingleton, Z_Construct_UScriptStruct_FConfirmationDialogAction_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_ConfirmationDialogAction.InnerSingleton;
	}
	void UCommonGameDialogDescriptor::StaticRegisterNativesUCommonGameDialogDescriptor()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UCommonGameDialogDescriptor);
	UClass* Z_Construct_UClass_UCommonGameDialogDescriptor_NoRegister()
	{
		return UCommonGameDialogDescriptor::StaticClass();
	}
	struct Z_Construct_UClass_UCommonGameDialogDescriptor_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Header_MetaData[];
#endif
		static const UECodeGen_Private::FTextPropertyParams NewProp_Header;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Body_MetaData[];
#endif
		static const UECodeGen_Private::FTextPropertyParams NewProp_Body;
		static const UECodeGen_Private::FStructPropertyParams NewProp_ButtonActions_Inner;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_ButtonActions_MetaData[];
#endif
		static const UECodeGen_Private::FArrayPropertyParams NewProp_ButtonActions;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UObject,
		(UObject* (*)())Z_Construct_UPackage__Script_CommonGame,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::DependentSingletons) < 16);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::Class_MetaDataParams[] = {
		{ "IncludePath", "Messaging/CommonGameDialog.h" },
		{ "ModuleRelativePath", "Public/Messaging/CommonGameDialog.h" },
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::NewProp_Header_MetaData[] = {
		{ "Category", "CommonGameDialogDescriptor" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** The header of the message to display */" },
#endif
		{ "ModuleRelativePath", "Public/Messaging/CommonGameDialog.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The header of the message to display" },
#endif
	};
#endif
	const UECodeGen_Private::FTextPropertyParams Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::NewProp_Header = { "Header", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonGameDialogDescriptor, Header), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::NewProp_Header_MetaData), Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::NewProp_Header_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::NewProp_Body_MetaData[] = {
		{ "Category", "CommonGameDialogDescriptor" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** The body of the message to display */" },
#endif
		{ "ModuleRelativePath", "Public/Messaging/CommonGameDialog.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The body of the message to display" },
#endif
	};
#endif
	const UECodeGen_Private::FTextPropertyParams Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::NewProp_Body = { "Body", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonGameDialogDescriptor, Body), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::NewProp_Body_MetaData), Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::NewProp_Body_MetaData) };
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::NewProp_ButtonActions_Inner = { "ButtonActions", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FConfirmationDialogAction, METADATA_PARAMS(0, nullptr) }; // 2413928803
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::NewProp_ButtonActions_MetaData[] = {
		{ "Category", "CommonGameDialogDescriptor" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** The confirm button's input action to use. */" },
#endif
		{ "ModuleRelativePath", "Public/Messaging/CommonGameDialog.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The confirm button's input action to use." },
#endif
	};
#endif
	const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::NewProp_ButtonActions = { "ButtonActions", nullptr, (EPropertyFlags)0x0010000000000004, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonGameDialogDescriptor, ButtonActions), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::NewProp_ButtonActions_MetaData), Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::NewProp_ButtonActions_MetaData) }; // 2413928803
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::NewProp_Header,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::NewProp_Body,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::NewProp_ButtonActions_Inner,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::NewProp_ButtonActions,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UCommonGameDialogDescriptor>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::ClassParams = {
		&UCommonGameDialogDescriptor::StaticClass,
		nullptr,
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::PropPointers),
		0,
		0x001000A0u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::Class_MetaDataParams), Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::Class_MetaDataParams)
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::PropPointers) < 2048);
	UClass* Z_Construct_UClass_UCommonGameDialogDescriptor()
	{
		if (!Z_Registration_Info_UClass_UCommonGameDialogDescriptor.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UCommonGameDialogDescriptor.OuterSingleton, Z_Construct_UClass_UCommonGameDialogDescriptor_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UCommonGameDialogDescriptor.OuterSingleton;
	}
	template<> COMMONGAME_API UClass* StaticClass<UCommonGameDialogDescriptor>()
	{
		return UCommonGameDialogDescriptor::StaticClass();
	}
	UCommonGameDialogDescriptor::UCommonGameDialogDescriptor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UCommonGameDialogDescriptor);
	UCommonGameDialogDescriptor::~UCommonGameDialogDescriptor() {}
	void UCommonGameDialog::StaticRegisterNativesUCommonGameDialog()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UCommonGameDialog);
	UClass* Z_Construct_UClass_UCommonGameDialog_NoRegister()
	{
		return UCommonGameDialog::StaticClass();
	}
	struct Z_Construct_UClass_UCommonGameDialog_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UCommonGameDialog_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UCommonActivatableWidget,
		(UObject* (*)())Z_Construct_UPackage__Script_CommonGame,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonGameDialog_Statics::DependentSingletons) < 16);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonGameDialog_Statics::Class_MetaDataParams[] = {
		{ "IncludePath", "Messaging/CommonGameDialog.h" },
		{ "ModuleRelativePath", "Public/Messaging/CommonGameDialog.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UCommonGameDialog_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UCommonGameDialog>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UCommonGameDialog_Statics::ClassParams = {
		&UCommonGameDialog::StaticClass,
		nullptr,
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		nullptr,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		0,
		0,
		0x00B010A1u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonGameDialog_Statics::Class_MetaDataParams), Z_Construct_UClass_UCommonGameDialog_Statics::Class_MetaDataParams)
	};
	UClass* Z_Construct_UClass_UCommonGameDialog()
	{
		if (!Z_Registration_Info_UClass_UCommonGameDialog.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UCommonGameDialog.OuterSingleton, Z_Construct_UClass_UCommonGameDialog_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UCommonGameDialog.OuterSingleton;
	}
	template<> COMMONGAME_API UClass* StaticClass<UCommonGameDialog>()
	{
		return UCommonGameDialog::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UCommonGameDialog);
	UCommonGameDialog::~UCommonGameDialog() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonGameDialog_h_Statics
	{
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonGameDialog_h_Statics::ScriptStructInfo[] = {
		{ FConfirmationDialogAction::StaticStruct, Z_Construct_UScriptStruct_FConfirmationDialogAction_Statics::NewStructOps, TEXT("ConfirmationDialogAction"), &Z_Registration_Info_UScriptStruct_ConfirmationDialogAction, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FConfirmationDialogAction), 2413928803U) },
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonGameDialog_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UCommonGameDialogDescriptor, UCommonGameDialogDescriptor::StaticClass, TEXT("UCommonGameDialogDescriptor"), &Z_Registration_Info_UClass_UCommonGameDialogDescriptor, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UCommonGameDialogDescriptor), 2871568633U) },
		{ Z_Construct_UClass_UCommonGameDialog, UCommonGameDialog::StaticClass, TEXT("UCommonGameDialog"), &Z_Registration_Info_UClass_UCommonGameDialog, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UCommonGameDialog), 2562450144U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonGameDialog_h_2704068558(TEXT("/Script/CommonGame"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonGameDialog_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonGameDialog_h_Statics::ClassInfo),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonGameDialog_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Messaging_CommonGameDialog_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
