
#include "Misc/AutomationTest.h"

#include "Misc/ConfigCacheIni.h"

#include "Progression/Runes/MythicRuneDefinition.h"
#include "Progression/Skills/MythicSkillDefinition.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSoftPersistCookTest,
    "Mythic.Content.SoftPersistedTypesCook",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSoftPersistCookTest::RunTest(const FString &Parameters) {
    /**
     * A definition the save stores as a soft path, and that nothing else hard-references, is invisible to the
     * cooker. It loads in the editor and resolves to null in a packaged build, so the player's slots come back
     * empty with nothing but a log line - and every in-editor test passes the whole way.
     *
     * Runes hit this. Skills then shipped without the sibling rule the rune entry exists to provide. Anything
     * persisted the same way belongs in this list.
     */
    const TArray<const UClass *> SoftPersistedTypes = {
        UMythicRuneDefinition::StaticClass(),
        UMythicSkillDefinition::StaticClass(),
    };

    TArray<FString> ScanRules;
    // The +Array rows are folded into the plain key by the time config is loaded.
    GConfig->GetArray(TEXT("/Script/Engine.AssetManagerSettings"), TEXT("PrimaryAssetTypesToScan"),
                      ScanRules, GGameIni);
    if (!TestTrue(TEXT("the project has asset scan rules to check - an empty read would fail every type for the wrong reason"),
                  ScanRules.Num() > 0)) {
        return false;
    }

    for (const UClass *Type : SoftPersistedTypes) {
        const FString ClassName = Type->GetName();
        const FString ClassPath = Type->GetPathName();

        const FString *Match = ScanRules.FindByPredicate([&ClassPath](const FString &Rule) {
            return Rule.Contains(ClassPath, ESearchCase::CaseSensitive);
        });

        if (!TestNotNull(*FString::Printf(TEXT("%s has a PrimaryAssetTypesToScan rule, or it will not cook"),
                                          *ClassName), Match)) {
            continue;
        }

        // A rule that scans but does not cook leaves the same hole; the asset is discoverable in the editor and
        // still absent from the package.
        TestTrue(*FString::Printf(TEXT("%s is marked AlwaysCook"), *ClassName),
                 Match->Contains(TEXT("CookRule=AlwaysCook"), ESearchCase::CaseSensitive));
    }

    AddInfo(FString::Printf(TEXT("scan rules: %d, soft-persisted types checked: %d"),
                            ScanRules.Num(), SoftPersistedTypes.Num()));
    return true;
}
