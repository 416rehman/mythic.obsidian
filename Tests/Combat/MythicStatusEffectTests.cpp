
#include "Misc/AutomationTest.h"

#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/Effects/MythicStatusEffectDefinition.h"
#include "GAS/Effects/MythicStatusRegistry.h"
#include "Settings/MythicDeveloperSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatusEffectTest,
    "Mythic.Combat.StatusEffects",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatusEffectTest::RunTest(const FString &Parameters) {
    using Def = UMythicAttributeSet_Defense;

    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    UMythicStatusEffectLibrary *Library = Settings ? Settings->StatusEffectLibrary.LoadSynchronous() : nullptr;
    if (!TestNotNull(TEXT("status library loads"), Library)) {
        return false;
    }

    for (const UMythicStatusEffectDefinition *Definition : Library->Statuses) {
        if (!Definition) {
            continue;
        }
        const FString Label = Definition->GetName();
        const UGameplayEffect *Effect = Definition->EffectToApply ? GetDefault<UGameplayEffect>(Definition->EffectToApply) : nullptr;
        if (!TestNotNull(*FString::Printf(TEXT("%s resolves its effect"), *Label), Effect)) {
            continue;
        }

        // A status the player wears has to expire on its own. Instant cannot be "on" you, and Infinite never lifts.
        TestTrue(*FString::Printf(TEXT("%s lasts for a duration"), *Label),
                 Effect->DurationPolicy == EGameplayEffectDurationType::HasDuration);
        float DurationSeconds = 0.0f;
        if (Effect->DurationMagnitude.GetStaticMagnitudeIfPossible(1.0f, DurationSeconds)) {
            TestTrue(*FString::Printf(TEXT("%s lasts a non-zero time"), *Label), DurationSeconds > 0.0f);
        }

        // A damage-over-time needs a tick, or the modifier lands once and the status is a flat hit wearing a timer.
        if (Effect->Modifiers.Num() > 0) {
            TestTrue(*FString::Printf(TEXT("%s ticks, since it modifies an attribute over time"), *Label),
                     Effect->Period.GetValueAtLevel(0.0f) > 0.0f);
            TestFalse(*FString::Printf(TEXT("%s does not tick on application, so the first tick is a beat later"), *Label),
                      Effect->bExecutePeriodicEffectOnApplication);
            for (const FGameplayModifierInfo &Mod : Effect->Modifiers) {
                TestTrue(*FString::Printf(TEXT("%s modifies an attribute that exists"), *Label), Mod.Attribute.IsValid());
            }
        }

        // What the definition claims it grants has to be what the effect really grants, or every reader of the tag
        // silently stops matching.
        const UTargetTagsGameplayEffectComponent *TagComponent = Effect->FindComponent<UTargetTagsGameplayEffectComponent>();
        if (TestNotNull(*FString::Printf(TEXT("%s grants target tags"), *Label), TagComponent)) {
            TestTrue(*FString::Printf(TEXT("%s grants %s"), *Label, *Definition->GrantedStateTag.ToString()),
                     TagComponent->GetConfiguredTargetTagChanges().CombinedTags.HasTagExact(Definition->GrantedStateTag));
        }
    }

    TestEqual(TEXT("threshold @0 resistance = 100"), Def::ComputeBuildupThreshold(0.0f), 100.0f);
    TestEqual(TEXT("threshold @full resistance = 102"), Def::ComputeBuildupThreshold(1.0f), 102.0f);
    TestEqual(TEXT("threshold @0.5 resistance = 101"), Def::ComputeBuildupThreshold(0.5f), 101.0f);
    TestEqual(TEXT("threshold clamps resistance > 1 to 102"), Def::ComputeBuildupThreshold(5.0f), 102.0f);
    TestEqual(TEXT("threshold clamps negative resistance to 100"), Def::ComputeBuildupThreshold(-3.0f), 100.0f);

    TestFalse(TEXT("below base threshold does not cross"), Def::BuildupCrossesThreshold(99.0f, 0.0f));
    TestTrue(TEXT("exactly at base threshold crosses"), Def::BuildupCrossesThreshold(100.0f, 0.0f));
    TestTrue(TEXT("above base threshold crosses"), Def::BuildupCrossesThreshold(150.0f, 0.0f));
    TestFalse(TEXT("101 does not cross the 102 threshold at full resistance"), Def::BuildupCrossesThreshold(101.0f, 1.0f));
    TestTrue(TEXT("102 crosses the 102 threshold at full resistance"), Def::BuildupCrossesThreshold(102.0f, 1.0f));

    return true;
}
