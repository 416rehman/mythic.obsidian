
#include "Misc/AutomationTest.h"

#include "GAS/Abilities/MythicGA_Triggered.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTriggeredProcTest,
    "Mythic.Combat.TriggeredProc",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicTriggeredProcTest::RunTest(const FString &Parameters) {
    using GA = UMythicGA_Triggered;

    // Chance, with no cooldown configured.
    TestTrue(TEXT("a certain proc fires on the worst roll"), GA::ShouldProc(1.0f, 0.0f, 100.0, 0.0, 0.999999f));
    TestFalse(TEXT("a zero chance never fires"), GA::ShouldProc(0.0f, 0.0f, 100.0, 0.0, 0.0f));
    TestTrue(TEXT("a 25% proc fires on a roll inside the band"), GA::ShouldProc(0.25f, 0.0f, 100.0, 0.0, 0.20f));
    TestFalse(TEXT("a 25% proc does not fire on a roll outside it"), GA::ShouldProc(0.25f, 0.0f, 100.0, 0.0, 0.30f));

    // Rolled chances come from item affixes, so a stacked one can arrive above 1 exactly as dodge did.
    TestFalse(TEXT("a stacked chance is still bounded, not free"), GA::ShouldProc(8.0f, 5.0f, 101.0, 100.0, 0.0f));

    // Internal cooldown. Without it attack speed alone would carry a proc build.
    TestFalse(TEXT("a proc inside its cooldown does not fire"), GA::ShouldProc(1.0f, 5.0f, 102.0, 100.0, 0.0f));
    TestTrue(TEXT("a proc past its cooldown fires again"), GA::ShouldProc(1.0f, 5.0f, 106.0, 100.0, 0.0f));
    TestTrue(TEXT("a proc exactly at its cooldown fires"), GA::ShouldProc(1.0f, 5.0f, 105.0, 100.0, 0.0f));

    // A clause that has never fired must not be treated as on cooldown at world time zero.
    TestTrue(TEXT("a clause that never fired is not on cooldown"), GA::ShouldProc(1.0f, 5.0f, 0.0, 0.0, 0.0f));

    // Target resolution. Both damage events carry the other party in Target, so one rule covers dealing and taking.
    {
        AActor *Owner = reinterpret_cast<AActor *>(0x1);
        FGameplayEventData Payload;
        TestEqual(TEXT("Self resolves the owner"),
                  GA::ResolveTarget({FGameplayTag(), FGameplayTag(), 1.0f, FGameplayTag(), EMythicTriggerTarget::Self, 0.0f}, &Payload, Owner),
                  Owner);

        FMythicTriggerSpec OtherSpec;
        OtherSpec.Target = EMythicTriggerTarget::Other;
        TestNull(TEXT("Other with an empty payload target resolves nothing"), GA::ResolveTarget(OtherSpec, &Payload, Owner));
        TestNull(TEXT("Other with no payload resolves nothing"), GA::ResolveTarget(OtherSpec, nullptr, Owner));
    }

    // Conditions. Every field defaults to "no gate", so a clause that authors none behaves as one without.
    {
        const FGameplayTagContainer Empty;
        FGameplayTagContainer World;
        World.AddTag(FGameplayTag::RequestGameplayTag(FName("Environment.Weather.Snow"), false));
        World.AddTag(FGameplayTag::RequestGameplayTag(FName("Environment.Time.Night"), false));

        FMythicTriggerCondition None;
        TestTrue(TEXT("a default condition is no gate"), GA::PassesCondition(None, World, Empty, Empty, 1.0f, 1.0f));
        TestTrue(TEXT("a default condition ignores health entirely"), GA::PassesCondition(None, Empty, Empty, Empty, 0.01f, 0.01f));

        FMythicTriggerCondition Snow;
        Snow.RequiredWorldTag = FGameplayTag::RequestGameplayTag(FName("Environment.Weather.Snow"), false);
        TestTrue(TEXT("the snow tag is registered, or this whole block proves nothing"), Snow.RequiredWorldTag.IsValid());
        TestTrue(TEXT("a weather gate opens in that weather"), GA::PassesCondition(Snow, World, Empty, Empty, 1.0f, 1.0f));
        TestFalse(TEXT("a weather gate stays shut with no world state"), GA::PassesCondition(Snow, Empty, Empty, Empty, 1.0f, 1.0f));

        FMythicTriggerCondition Rain;
        Rain.RequiredWorldTag = FGameplayTag::RequestGameplayTag(FName("Environment.Weather.Rain"), false);
        TestFalse(TEXT("a rain gate stays shut while it snows"), GA::PassesCondition(Rain, World, Empty, Empty, 1.0f, 1.0f));

        // Authoring the parent gates on any weather, matching FMythicWeatherDamageMod.
        FMythicTriggerCondition AnyWeather;
        AnyWeather.RequiredWorldTag = FGameplayTag::RequestGameplayTag(FName("Environment.Weather"), false);
        TestTrue(TEXT("the parent tag gates on any weather"), GA::PassesCondition(AnyWeather, World, Empty, Empty, 1.0f, 1.0f));

        // Cornered Beast: "Below half your life, every blow tears deeper."
        FMythicTriggerCondition Cornered;
        Cornered.SourceHealthMax = 0.5f;
        TestTrue(TEXT("a wounded owner passes a below-half gate"), GA::PassesCondition(Cornered, Empty, Empty, Empty, 0.4f, 1.0f));
        TestFalse(TEXT("a healthy owner fails a below-half gate"), GA::PassesCondition(Cornered, Empty, Empty, Empty, 0.9f, 1.0f));
        TestTrue(TEXT("exactly half passes a below-half gate"), GA::PassesCondition(Cornered, Empty, Empty, Empty, 0.5f, 1.0f));

        // Executioner: "Anything already dying takes a killing blow."
        FMythicTriggerCondition Dying;
        Dying.TargetHealthMax = 0.2f;
        TestTrue(TEXT("a dying target passes an execute gate"), GA::PassesCondition(Dying, Empty, Empty, Empty, 1.0f, 0.15f));
        TestFalse(TEXT("a healthy target fails an execute gate"), GA::PassesCondition(Dying, Empty, Empty, Empty, 1.0f, 0.5f));

        // Envious Edge: "The first blow against an unbloodied foe bites far deeper."
        FMythicTriggerCondition Unbloodied;
        Unbloodied.TargetHealthMin = 1.0f;
        TestTrue(TEXT("an untouched target passes an unbloodied gate"), GA::PassesCondition(Unbloodied, Empty, Empty, Empty, 1.0f, 1.0f));
        TestFalse(TEXT("a scratched target fails an unbloodied gate"), GA::PassesCondition(Unbloodied, Empty, Empty, Empty, 1.0f, 0.99f));

        // An actor with no health reads as full, so a below-half gate cannot fire on something that cannot bleed.
        TestFalse(TEXT("something with no health fails a below-half gate"),
                  GA::PassesCondition(Cornered, Empty, Empty, Empty, GA::GetHealthFraction(nullptr), 1.0f));
    }

    // The class must stay passive: talent validation rejects anything else, so a wrong default would make every
    // proc silently un-grantable.
    {
        const UMythicGA_Triggered *CDO = GetDefault<UMythicGA_Triggered>();
        TestEqual(TEXT("procs activate on spawn"), CDO->GetActivationPolicy(), EMythicAbilityActivationPolicy::OnSpawn);
    }

    return true;
}
