#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Blueprint/UserWidget.h"
#include "UObject/SoftObjectPath.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMythicLoadingScreenConfigurationTest,
	"Mythic.UI.LoadingScreen.Configuration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMythicLoadingScreenConfigurationTest::RunTest(const FString& Parameters)
{
	static const TCHAR* SettingsSection = TEXT("/Script/CommonLoadingScreen.CommonLoadingScreenSettings");
	static const TCHAR* ExpectedWidgetClass =
		TEXT("/Game/Mythic/UI/Widgets/LoadingScreen/WBP_LoadingScreen.WBP_LoadingScreen_C");

	FString ConfiguredWidgetClass;
	const bool bHasConfiguredWidget = GConfig != nullptr
		&& GConfig->GetString(SettingsSection, TEXT("LoadingScreenWidget"), ConfiguredWidgetClass, GGameIni);

	TestTrue(TEXT("CommonLoadingScreen has an explicit widget class in game configuration"), bHasConfiguredWidget);
	if (!bHasConfiguredWidget)
	{
		return false;
	}

	TestEqual(TEXT("The configured loading screen is the project-owned presentation"), ConfiguredWidgetClass,
		FString(ExpectedWidgetClass));

	const FSoftClassPath ConfiguredClassPath(ConfiguredWidgetClass);
	UClass* LoadedWidgetClass = ConfiguredClassPath.TryLoadClass<UUserWidget>();
	TestNotNull(TEXT("The configured loading-screen class resolves"), LoadedWidgetClass);
	if (LoadedWidgetClass)
	{
		TestTrue(TEXT("The configured class derives from UUserWidget"), LoadedWidgetClass->IsChildOf(UUserWidget::StaticClass()));
	}

	TArray<FString> AlwaysCookDirectories;
	const bool bHasPackagingSettings = GConfig->GetArray(
		TEXT("/Script/UnrealEd.ProjectPackagingSettings"),
		TEXT("DirectoriesToAlwaysCook"),
		AlwaysCookDirectories,
		GGameIni) > 0;
	const bool bLoadingPresentationAlwaysCooks = bHasPackagingSettings && AlwaysCookDirectories.ContainsByPredicate(
		[](const FString& DirectoryEntry)
		{
			return DirectoryEntry.Contains(TEXT("/Game/Mythic/UI/Widgets/LoadingScreen"));
		});
	TestTrue(TEXT("The config-selected widget and its soft presentation assets are always cooked"),
		bLoadingPresentationAlwaysCooks);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
