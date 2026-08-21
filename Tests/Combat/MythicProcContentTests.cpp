
#include "Misc/AutomationTest.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "GameplayTagsManager.h"

#include "GAS/Abilities/MythicGA_Triggered.h"
#include "Itemization/Inventory/Fragments/FragmentTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicProcContentTest,
    "Mythic.Content.TriggeredProcs",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicProcContentTest::RunTest(const FString &Parameters) {
    FAssetRegistryModule &Module = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry &Registry = Module.Get();
    Registry.SearchAllAssets(true);

    TArray<FAssetData> TalentAssets;
    Registry.GetAssetsByClass(UTalentDefinition::StaticClass()->GetClassPathName(), TalentAssets);
    if (!TestTrue(TEXT("the project has talent definitions to check — an empty scan would pass for the wrong reason"),
                  TalentAssets.Num() > 0)) {
        return false;
    }

    const UGameplayTagsManager &Tags = UGameplayTagsManager::Get();
    int32 ProcTalents = 0;

    for (const FAssetData &Asset : TalentAssets) {
        UTalentDefinition *Talent = Cast<UTalentDefinition>(Asset.GetAsset());
        if (!Talent || !Talent->AbilityDef.Ability) {
            // Talents with no ability at all are tracked separately; this test guards the ones that are wired.
            continue;
        }

        const UMythicGA_Triggered *Proc = Cast<UMythicGA_Triggered>(Talent->AbilityDef.Ability->GetDefaultObject());
        if (!Proc) {
            continue;
        }
        ++ProcTalents;

        const FString Name = Asset.AssetName.ToString();
        if (!TestTrue(*FString::Printf(TEXT("%s has at least one trigger clause"), *Name), Proc->Triggers.Num() > 0)) {
            continue;
        }

        for (int32 Index = 0; Index < Proc->Triggers.Num(); ++Index) {
            const FMythicTriggerSpec &Spec = Proc->Triggers[Index];
            const FString Where = FString::Printf(TEXT("%s clause %d"), *Name, Index);

            // An unregistered tag still passes IsValid() in the editor, so ask the manager.
            TestTrue(*FString::Printf(TEXT("%s trigger event is a registered tag"), *Where),
                     Tags.RequestGameplayTag(Spec.TriggerEvent.GetTagName(), false).IsValid());
            TestTrue(*FString::Printf(TEXT("%s status is a registered tag"), *Where),
                     Tags.RequestGameplayTag(Spec.StatusToApply.GetTagName(), false).IsValid());

            // A clause naming a rolled chance the talent never rolls falls back to its constant without a word,
            // which reads as authored variance that is not there.
            if (Spec.ChanceParameter.IsValid()) {
                TestTrue(*FString::Printf(TEXT("%s chance parameter is rolled by the talent"), *Where),
                         Talent->AbilityDef.ParameterRolls.Contains(Spec.ChanceParameter));
            }

            TestTrue(*FString::Printf(TEXT("%s fallback chance is a probability"), *Where),
                     Spec.Chance >= 0.0f && Spec.Chance <= 1.0f);
            TestTrue(*FString::Printf(TEXT("%s internal cooldown is not negative"), *Where), Spec.InternalCooldown >= 0.0f);
        }
    }

    TestTrue(TEXT("at least one talent is wired to a trigger proc"), ProcTalents > 0);
    return true;
}
