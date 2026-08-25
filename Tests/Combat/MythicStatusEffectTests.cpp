
#include "Misc/AutomationTest.h"

#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/Effects/MythicStatusEffectDefinition.h"
#include "GAS/Effects/MythicStatusRegistry.h"
#include "Settings/MythicCombatSettings.h"
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

    // The threshold is authored, so this asserts the relationship to the settings rather than the old literals.
    const UMythicCombatSettings *CombatSettings = GetDefault<UMythicCombatSettings>();
    if (TestNotNull(TEXT("combat settings resolve"), CombatSettings)) {
        const float Base = CombatSettings->StatusBuildupThreshold;
        const float MaxCut = CombatSettings->MaxStatusThresholdReduction;

        TestEqual(TEXT("no attacker reduction leaves the authored base"), Def::ComputeBuildupThreshold(0.0f), Base);
        TestTrue(TEXT("the base is a designer-reachable number, not a literal"), Base >= 1.0f);

        // The whole point of the stat: an attacker who specialises makes statuses land sooner.
        TestTrue(TEXT("reduction lowers the threshold"), Def::ComputeBuildupThreshold(0.25f) < Base);
        TestEqual(TEXT("reduction is the authored fraction of the base"),
                  Def::ComputeBuildupThreshold(0.25f), Base * 0.75f);

        // Without the ceiling a stacked build reaches zero and every status lands on the first proc.
        TestEqual(TEXT("reduction past the ceiling clamps to the ceiling"),
                  Def::ComputeBuildupThreshold(1.0f), Def::ComputeBuildupThreshold(MaxCut));
        TestTrue(TEXT("even a fully stacked attacker leaves a threshold to cross"),
                 Def::ComputeBuildupThreshold(1.0f) >= 1.0f);
        TestEqual(TEXT("a negative reduction cannot raise the threshold"),
                  Def::ComputeBuildupThreshold(-3.0f), Base);

        TestFalse(TEXT("below the threshold does not cross"), Def::BuildupCrossesThreshold(Base - 1.0f, 0.0f));
        TestTrue(TEXT("exactly at the threshold crosses"), Def::BuildupCrossesThreshold(Base, 0.0f));
        TestTrue(TEXT("above the threshold crosses"), Def::BuildupCrossesThreshold(Base * 1.5f, 0.0f));

        // Resistance is deliberately not a threshold input any more: it already gates every proc through
        // 1 - Resist, so bending the threshold with it as well paid one stat twice.
        TestTrue(TEXT("a reduced threshold is crossed by buildup the base would not have been"),
                 Def::BuildupCrossesThreshold(Base * 0.8f, 0.25f));
    }

    return true;
}
