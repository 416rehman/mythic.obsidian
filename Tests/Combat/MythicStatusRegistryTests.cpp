
#include "Misc/AutomationTest.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/Effects/MythicStatusEffectDefinition.h"
#include "GAS/Effects/MythicStatusRegistry.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayTagsManager.h"
#include "Settings/MythicDeveloperSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatusLibraryTest,
    "Mythic.Combat.StatusLibrary",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatusLibraryTest::RunTest(const FString &Parameters) {
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    if (!TestNotNull(TEXT("developer settings exist"), Settings)) {
        return false;
    }
    if (!TestFalse(TEXT("StatusEffectLibrary is configured — without it no status can be applied"), Settings->StatusEffectLibrary.IsNull())) {
        return false;
    }

    UMythicStatusEffectLibrary *Library = Settings->StatusEffectLibrary.LoadSynchronous();
    if (!TestNotNull(TEXT("StatusEffectLibrary loads"), Library)) {
        return false;
    }
    TestTrue(TEXT("library authors at least one status"), Library->Statuses.Num() > 0);

    const FString TypePrefix = TEXT("Status.Type.");
    TSet<FGameplayTag> SeenTypes;
    TArray<FGameplayAttribute> CoveredBuildups;

    for (const UMythicStatusEffectDefinition *Definition : Library->Statuses) {
        if (!TestNotNull(TEXT("library holds no null status entries"), Definition)) {
            continue;
        }
        const FString Label = Definition->GetName();

        TestTrue(*FString::Printf(TEXT("%s has a StatusType tag"), *Label), Definition->StatusType.IsValid());
        TestTrue(*FString::Printf(TEXT("%s StatusType lives under Status.Type"), *Label), Definition->StatusType.ToString().StartsWith(TypePrefix));

        // An unregistered tag still round-trips through the asset but resolves to nothing at runtime, so
        // IsValid() alone passes while the status can never be found. Ask the manager directly.
        const FGameplayTag Registered = UGameplayTagsManager::Get().RequestGameplayTag(Definition->StatusType.GetTagName(), false);
        TestTrue(*FString::Printf(TEXT("%s StatusType %s is registered with the tag manager"), *Label, *Definition->StatusType.ToString()),
                 Registered.IsValid());
        TestFalse(*FString::Printf(TEXT("%s StatusType is unique"), *Label), SeenTypes.Contains(Definition->StatusType));
        SeenTypes.Add(Definition->StatusType);

        TestNotNull(*FString::Printf(TEXT("%s has an EffectToApply"), *Label), Definition->EffectToApply.Get());
        TestTrue(*FString::Printf(TEXT("%s has a GrantedStateTag"), *Label), Definition->GrantedStateTag.IsValid());
        TestTrue(*FString::Printf(TEXT("%s has an OnsetCueTag"), *Label), Definition->OnsetCueTag.IsValid());
        TestTrue(*FString::Printf(TEXT("%s has a BuildupAttribute"), *Label), Definition->BuildupAttribute.IsValid());
        TestTrue(*FString::Printf(TEXT("%s has a ResistanceAttribute"), *Label), Definition->ResistanceAttribute.IsValid());
        TestFalse(*FString::Printf(TEXT("%s has a display name"), *Label), Definition->DisplayName.IsEmpty());
        TestFalse(*FString::Printf(TEXT("%s has an icon"), *Label), Definition->Icon.IsNull());

        // GrantedStateTag drives UI and damage modifiers, but the tag is really granted by the effect.
        // If the two drift apart the status silently stops matching anything that reads it.
        if (Definition->EffectToApply && Definition->GrantedStateTag.IsValid()) {
            const UGameplayEffect *EffectCDO = GetDefault<UGameplayEffect>(Definition->EffectToApply);
            const UTargetTagsGameplayEffectComponent *TagComponent =
                EffectCDO ? EffectCDO->FindComponent<UTargetTagsGameplayEffectComponent>() : nullptr;
            if (TestNotNull(*FString::Printf(TEXT("%s effect grants target tags"), *Label), TagComponent)) {
                TestTrue(*FString::Printf(TEXT("%s GrantedStateTag %s matches what the effect actually grants"), *Label,
                                          *Definition->GrantedStateTag.ToString()),
                         TagComponent->GetConfiguredTargetTagChanges().CombinedTags.HasTagExact(Definition->GrantedStateTag));
            }
        }

        for (const FMythicStatusReaction &Reaction : Definition->Reactions) {
            TestTrue(*FString::Printf(TEXT("%s reaction has a RequiredTargetTag"), *Label), Reaction.RequiredTargetTag.IsValid());
        }

        if (Definition->BuildupAttribute.IsValid()) {
            CoveredBuildups.AddUnique(Definition->BuildupAttribute);
        }
    }

    // Every buildup the damage pipeline writes must resolve to a status, or that status silently never fires.
    const TArray<FGameplayAttribute> PipelineBuildups = {
        UMythicAttributeSet_Defense::GetBurnBuildupAttribute(),
        UMythicAttributeSet_Defense::GetBleedBuildupAttribute(),
        UMythicAttributeSet_Defense::GetPoisonBuildupAttribute(),
        UMythicAttributeSet_Defense::GetSlowBuildupAttribute(),
        UMythicAttributeSet_Defense::GetFreezeBuildupAttribute(),
        UMythicAttributeSet_Defense::GetStunBuildupAttribute(),
        UMythicAttributeSet_Defense::GetWeakenBuildupAttribute(),
        UMythicAttributeSet_Defense::GetTerrifyBuildupAttribute(),
    };
    for (const FGameplayAttribute &Buildup : PipelineBuildups) {
        TestTrue(*FString::Printf(TEXT("a status covers the %s the damage pipeline feeds"), *Buildup.GetName()), CoveredBuildups.Contains(Buildup));
    }

    return true;
}
