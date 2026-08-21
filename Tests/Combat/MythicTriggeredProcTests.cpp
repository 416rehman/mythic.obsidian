
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

    // The class must stay passive: talent validation rejects anything else, so a wrong default would make every
    // proc silently un-grantable.
    {
        const UMythicGA_Triggered *CDO = GetDefault<UMythicGA_Triggered>();
        TestEqual(TEXT("procs activate on spawn"), CDO->GetActivationPolicy(), EMythicAbilityActivationPolicy::OnSpawn);
    }

    return true;
}
