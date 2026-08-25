
#include "Misc/AutomationTest.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"

#include "GAS/Abilities/MythicGA_Triggered.h"
#include "GAS/Effects/MythicStatusEffectDefinition.h"
#include "Itemization/Inventory/Fragments/FragmentTypes.h"
#include "Itemization/Inventory/Fragments/Passive/TalentFragment.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTalentContentTest,
    "Mythic.Content.TalentPools",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicTalentContentTest::RunTest(const FString &Parameters) {
    FAssetRegistryModule &Module = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry &Registry = Module.Get();
    Registry.SearchAllAssets(true);

    TArray<FAssetData> PoolAssets;
    Registry.GetAssetsByClass(UTalentPool::StaticClass()->GetClassPathName(), PoolAssets);
    if (!TestTrue(TEXT("the project has talent pools to check - an empty scan would pass for the wrong reason"),
                  PoolAssets.Num() > 0)) {
        return false;
    }

    // Every status a talent may name, keyed the way StatusToApply is: the registry's StatusType, not the
    // GAS.Debuff.* state an afflicted actor ends up owning. Both are registered tags, so a talent naming the
    // wrong one passes every tag check and then applies nothing.
    TSet<FGameplayTag> StatusKeys;
    TArray<FAssetData> StatusAssets;
    Registry.GetAssetsByClass(UMythicStatusEffectDefinition::StaticClass()->GetClassPathName(), StatusAssets);
    for (const FAssetData &Asset : StatusAssets) {
        if (const UMythicStatusEffectDefinition *Status = Cast<UMythicStatusEffectDefinition>(Asset.GetAsset())) {
            if (Status->StatusType.IsValid()) {
                StatusKeys.Add(Status->StatusType);
            }
        }
    }
    if (!TestTrue(TEXT("the status registry has keys to match against"), StatusKeys.Num() > 0)) {
        return false;
    }

    int32 TotalEntries = 0;
    for (const FAssetData &Asset : PoolAssets) {
        const UTalentPool *Pool = Cast<UTalentPool>(Asset.GetAsset());
        const FString PoolName = Asset.AssetName.ToString();
        if (!Pool) {
            AddError(FString::Printf(TEXT("%s did not load"), *PoolName));
            continue;
        }

        // An item pointing at an empty pool rolls the talent count its rarity promises and gets nothing.
        if (!TestTrue(*FString::Printf(TEXT("%s has at least one talent"), *PoolName), Pool->TalentDefs.Num() > 0)) {
            continue;
        }

        for (int32 Index = 0; Index < Pool->TalentDefs.Num(); ++Index) {
            const UTalentDefinition *Talent = Pool->TalentDefs[Index];
            const FString Where = FString::Printf(TEXT("%s entry %d"), *PoolName, Index);
            if (!TestNotNull(*FString::Printf(TEXT("%s resolves"), *Where), Talent)) {
                continue;
            }
            ++TotalEntries;

            TestTrue(*FString::Printf(TEXT("%s carries a payload"), *Where), Talent->HasAnyPayload());
            TestFalse(*FString::Printf(TEXT("%s is named"), *Where), Talent->Name.IsEmpty());

            const FString TalentName = Talent->GetName();
            const FText &Rich = Talent->AbilityDef.RichText;
            const FString Text = Rich.ToString();

            // A placeholder with no roll renders as literal markup, and a roll no placeholder shows is variance
            // the player is never told about. Both directions matter.
            for (const TPair<FGameplayTag, FRollDefinition> &Roll : Talent->AbilityDef.ParameterRolls) {
                TestTrue(*FString::Printf(TEXT("%s shows its %s roll"), *TalentName, *Roll.Key.ToString()),
                         Text.Contains(FString::Printf(TEXT("<#%s>"), *Roll.Key.ToString())));
            }

            int32 SearchFrom = 0;
            while (true) {
                const int32 Open = Text.Find(TEXT("<#"), ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchFrom);
                if (Open == INDEX_NONE) {
                    break;
                }
                const int32 Close = Text.Find(TEXT(">"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Open);
                if (Close == INDEX_NONE) {
                    AddError(FString::Printf(TEXT("%s has an unclosed placeholder"), *TalentName));
                    break;
                }
                const FString Placeholder = Text.Mid(Open + 2, Close - Open - 2);
                TestTrue(*FString::Printf(TEXT("%s rolls the value its text shows for <#%s>"), *TalentName, *Placeholder),
                         Talent->AbilityDef.ParameterRolls.Contains(FGameplayTag::RequestGameplayTag(*Placeholder, false)));
                SearchFrom = Close + 1;
            }

            if (!Talent->AbilityDef.Ability) {
                continue;
            }
            const UMythicGA_Triggered *Proc = Cast<UMythicGA_Triggered>(Talent->AbilityDef.Ability->GetDefaultObject());
            if (!Proc) {
                continue;
            }
            for (int32 ClauseIndex = 0; ClauseIndex < Proc->Triggers.Num(); ++ClauseIndex) {
                const FMythicTriggerSpec &Spec = Proc->Triggers[ClauseIndex];
                if (Spec.StatusToApply.IsValid()) {
                    TestTrue(*FString::Printf(TEXT("%s clause %d names a status the registry can apply"),
                                              *TalentName, ClauseIndex),
                             StatusKeys.Contains(Spec.StatusToApply));
                }
            }
        }
    }

    AddInfo(FString::Printf(TEXT("talent pools: %d, entries: %d, status keys: %d"),
                            PoolAssets.Num(), TotalEntries, StatusKeys.Num()));
    TestTrue(TEXT("the pools hold talents between them"), TotalEntries > 0);
    return true;
}
