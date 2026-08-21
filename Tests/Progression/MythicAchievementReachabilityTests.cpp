
#include "Misc/AutomationTest.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "GameplayTagsManager.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "Progression/MythicAchievementDefinition.h"

namespace MythicTest {
// Every .cpp in the module, concatenated. A counter with no RecordStat call site fails silently at runtime —
// no error, no warning, no log — so the only place the truth exists is the source.
static bool LoadModuleSource(FString &OutSource) {
    const FString Root = FPaths::Combine(FPaths::ProjectDir(), TEXT("Source"), TEXT("Mythic"));
    if (!IFileManager::Get().DirectoryExists(*Root)) {
        return false;
    }
    TArray<FString> Files;
    IFileManager::Get().FindFilesRecursive(Files, *Root, TEXT("*.cpp"), true, false);
    for (const FString &File : Files) {
        // The tag definition file names every counter symbol, so including it would make each one look recorded.
        if (FPaths::GetCleanFilename(File).StartsWith(TEXT("MythicTags_"))) {
            continue;
        }
        // The tests themselves reference counters without recording them.
        if (File.Contains(TEXT("Tests/")) || File.Contains(TEXT("Tests\\"))) {
            continue;
        }
        FString Text;
        if (FFileHelper::LoadFileToString(Text, *File)) {
            OutSource += Text;
        }
    }
    return true;
}

// "Stat.Item.Looted" is written at its call site as STAT_ITEM_LOOTED or TAG_Stat_Item_Looted, depending on which
// convention the owning system used. Accept either, or the literal tag name.
static bool HasRecorderFor(const FString &Source, const FGameplayTag &Counter) {
    const FString Name = Counter.ToString();

    // Stat.Cooking.DishesCooked is declared STAT_COOKING_DISHES_COOKED: dots become underscores and so do the
    // word breaks inside a leaf, so build the snake form as well as the flat one.
    FString Snake;
    for (int32 i = 0; i < Name.Len(); ++i) {
        const TCHAR C = Name[i];
        if (C == TEXT('.')) {
            Snake.AppendChar(TEXT('_'));
            continue;
        }
        if (i > 0 && FChar::IsUpper(C) && FChar::IsLower(Name[i - 1])) {
            Snake.AppendChar(TEXT('_'));
        }
        Snake.AppendChar(C);
    }

    const FString Flat = Name.Replace(TEXT("."), TEXT("_")).ToUpper();
    const FString Mixed = TEXT("TAG_") + Name.Replace(TEXT("."), TEXT("_"));

    return Source.Contains(Flat) || Source.Contains(Snake.ToUpper()) || Source.Contains(Mixed)
        || Source.Contains(FString::Printf(TEXT("\"%s\""), *Name));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAchievementReachabilityTest,
    "Mythic.Progression.AchievementReachability",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAchievementReachabilityTest::RunTest(const FString &Parameters) {
    FString Source;
    if (!MythicTest::LoadModuleSource(Source)) {
        // A packaged build has no Source tree. Nothing to check rather than a spurious failure.
        AddInfo(TEXT("Module source not present; skipping recorder check."));
        return true;
    }
    if (!TestTrue(TEXT("module source loaded, or every counter would look unrecorded"), Source.Len() > 10000)) {
        return false;
    }
    // Prove the scan can find a recorder at all before trusting any negative it produces.
    TestTrue(TEXT("the scan finds a known recorder"), Source.Contains(TEXT("RecordStat(STAT_KILL_GENERIC")));

    const UGameplayTagsManager &TagsForSelfCheck = UGameplayTagsManager::Get();
    // Self-check the negative path. Stat.Deed.Mercy is registered and has no producer, so the scan MUST report it
    // missing — otherwise this whole test could be passing vacuously.
    {
        const FGameplayTag Unrecorded = TagsForSelfCheck.RequestGameplayTag(FName("Stat.Deed.Mercy"), false);
        if (TestTrue(TEXT("the unrecorded control counter is registered"), Unrecorded.IsValid())) {
            TestFalse(TEXT("the scan reports a counter nothing records as missing"),
                      MythicTest::HasRecorderFor(Source, Unrecorded));
        }
    }

    FAssetRegistryModule &Module = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry &Registry = Module.Get();
    Registry.SearchAllAssets(true);

    TArray<FAssetData> Assets;
    Registry.GetAssetsByClass(UMythicAchievementDefinition::StaticClass()->GetClassPathName(), Assets);
    if (!TestTrue(TEXT("the project has achievements to check"), Assets.Num() > 0)) {
        return false;
    }

    const UGameplayTagsManager &Tags = TagsForSelfCheck;
    int32 Checked = 0;

    for (const FAssetData &Asset : Assets) {
        const UMythicAchievementDefinition *Achievement = Cast<UMythicAchievementDefinition>(Asset.GetAsset());
        if (!Achievement) {
            continue;
        }
        const FString Name = Asset.AssetName.ToString();

        for (const FMythicStatRequirement &Req : Achievement->Condition.StatRequirements) {
            if (!Req.StatTag.IsValid()) {
                continue;
            }
            ++Checked;
            const FString Counter = Req.StatTag.ToString();

            TestTrue(*FString::Printf(TEXT("%s requires a registered counter (%s)"), *Name, *Counter),
                     Tags.RequestGameplayTag(Req.StatTag.GetTagName(), false).IsValid());

            // The failure this guards: an achievement whose counter nothing ever writes can never be earned, and
            // says nothing at runtime because an unrecorded counter simply reads zero.
            TestTrue(*FString::Printf(TEXT("%s requires %s, which something must record"), *Name, *Counter),
                     MythicTest::HasRecorderFor(Source, Req.StatTag));
        }
    }

    TestTrue(TEXT("at least one achievement threshold was checked"), Checked > 0);
    return true;
}
