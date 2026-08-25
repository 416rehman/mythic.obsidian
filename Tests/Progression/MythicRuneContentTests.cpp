
#include "Misc/AutomationTest.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "GameplayTagsManager.h"

#include "GAS/Abilities/MythicGA_Triggered.h"
#include "GAS/Effects/MythicStatusEffectDefinition.h"
#include "Progression/Runes/MythicRuneDefinition.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneContentTest,
    "Mythic.Content.Runes",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneContentTest::RunTest(const FString &Parameters) {
    FAssetRegistryModule &Module = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry &Registry = Module.Get();
    Registry.SearchAllAssets(true);

    TArray<FAssetData> RuneAssets;
    Registry.GetAssetsByClass(UMythicRuneDefinition::StaticClass()->GetClassPathName(), RuneAssets);
    if (!TestTrue(TEXT("the project has runes to check - an empty scan would pass for the wrong reason"),
                  RuneAssets.Num() > 0)) {
        return false;
    }

    // StatusToApply is matched against the registry's StatusType, not the GAS.Debuff.* state an afflicted actor
    // ends up owning. Both are registered tags, so naming the wrong one passes every tag check and applies nothing.
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

    const UGameplayTagsManager &Tags = UGameplayTagsManager::Get();
    int32 WorldGated = 0;

    for (const FAssetData &Asset : RuneAssets) {
        const UMythicRuneDefinition *Rune = Cast<UMythicRuneDefinition>(Asset.GetAsset());
        const FString Name = Asset.AssetName.ToString();
        if (!TestNotNull(*FString::Printf(TEXT("%s loads"), *Name), Rune)) {
            continue;
        }

        // A rune with no deed is a rune every character owns from birth, which is not what a rune is.
        TestTrue(*FString::Printf(TEXT("%s names a deed to earn it"), *Name), Rune->RequiredTag.IsValid());
        TestTrue(*FString::Printf(TEXT("%s deed tag is registered"), *Name),
                 Tags.RequestGameplayTag(Rune->RequiredTag.GetTagName(), false).IsValid());

        // Without a hint the player is told a rune exists and never how to earn it.
        TestFalse(*FString::Printf(TEXT("%s tells the player how to earn it"), *Name), Rune->Hint.IsEmpty());
        TestFalse(*FString::Printf(TEXT("%s is named"), *Name), Rune->Name.IsEmpty());
        TestFalse(*FString::Printf(TEXT("%s says what it does"), *Name), Rune->Description.IsEmpty());

        // The UI colour-codes from these, so an uncategorised rune has no colour to draw.
        TestTrue(*FString::Printf(TEXT("%s carries a category"), *Name), Rune->CategoryTags.Num() > 0);

        if (!TestTrue(*FString::Printf(TEXT("%s grants an ability"), *Name), Rune->HasPayload())) {
            continue;
        }

        const UMythicGA_Triggered *Proc = Cast<UMythicGA_Triggered>(Rune->Ability->GetDefaultObject());
        if (!TestNotNull(*FString::Printf(TEXT("%s ability is a trigger ability"), *Name), Proc)) {
            continue;
        }
        if (!TestTrue(*FString::Printf(TEXT("%s has a trigger clause"), *Name), Proc->Triggers.Num() > 0)) {
            continue;
        }

        for (int32 Index = 0; Index < Proc->Triggers.Num(); ++Index) {
            const FMythicTriggerSpec &Spec = Proc->Triggers[Index];
            const FString Where = FString::Printf(TEXT("%s clause %d"), *Name, Index);

            TestTrue(*FString::Printf(TEXT("%s applies a status or an effect"), *Where),
                     UMythicGA_Triggered::HasPayload(Spec));
            if (Spec.StatusToApply.IsValid()) {
                TestTrue(*FString::Printf(TEXT("%s names a status the registry can apply"), *Where),
                         StatusKeys.Contains(Spec.StatusToApply));
            }

            // The world container holds only weather, time and season, so a gate outside Environment.* can never
            // open and the rune would be authored and permanently inert.
            if (Spec.Condition.RequiredWorldTag.IsValid()) {
                ++WorldGated;
                TestTrue(*FString::Printf(TEXT("%s world gate is an Environment tag"), *Where),
                         Spec.Condition.RequiredWorldTag.ToString().StartsWith(TEXT("Environment.")));
                TestTrue(*FString::Printf(TEXT("%s world gate is registered"), *Where),
                         Tags.RequestGameplayTag(Spec.Condition.RequiredWorldTag.GetTagName(), false).IsValid());
            }
        }
    }

    AddInfo(FString::Printf(TEXT("runes: %d, status keys: %d, world-gated clauses: %d"),
                            RuneAssets.Num(), StatusKeys.Num(), WorldGated));

    // World gating is what separates a rune from a talent. If none of them use it the set has drifted.
    TestTrue(TEXT("at least one rune is gated on the world"), WorldGated > 0);
    return true;
}
