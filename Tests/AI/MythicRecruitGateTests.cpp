#include "Misc/AutomationTest.h"

#include "AI/NPCs/MythicRecruitRules.h"
#include "World/LivingWorld/LivingWorldSettings.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace {
    FGameplayTagContainer Tags(std::initializer_list<const TCHAR *> Names) {
        FGameplayTagContainer Container;
        for (const TCHAR *Name : Names) {
            const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(Name), false);
            if (Tag.IsValid()) {
                Container.AddTag(Tag);
            }
        }
        return Container;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicRecruitGateTest,
                                 "Mythic.AI.RecruitGates",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

/**
 * The two gates UNPCDefinition documents, which nothing checked.
 *
 * Recruitment validated only bRecruitable, so an NPC that demanded tags joined anyone, and one whose faction
 * despises you joined anyway. Both gates lived only inside a server RPC, which is exactly why neither was ever
 * exercised - there was nowhere to call them from that was not a live session.
 */
bool FMythicRecruitGateTest::RunTest(const FString &Parameters) {
    using Rules = FMythicRecruitRules;

    const UMythicLivingWorldSettings *Settings = GetDefault<UMythicLivingWorldSettings>();
    if (!TestNotNull(TEXT("living world settings resolve"), Settings)) {
        return false;
    }
    const float Threshold = Settings->RecruitStandingThreshold;
    AddInfo(FString::Printf(TEXT("authored recruit standing threshold: %.1f"), Threshold));

    // Authored, not a constant: the rule reads whatever a designer set rather than a literal 70.
    TestTrue(TEXT("the threshold sits on the standing component's scale"), Threshold >= -100.0f && Threshold <= 100.0f);

    // --- the standing gate ---
    TestTrue(TEXT("standing exactly at the threshold recruits"), Rules::MeetsStandingGate(Threshold, Threshold));
    TestTrue(TEXT("standing above the threshold recruits"), Rules::MeetsStandingGate(Threshold + 1.0f, Threshold));
    TestFalse(TEXT("standing just below the threshold does not"), Rules::MeetsStandingGate(Threshold - 1.0f, Threshold));
    TestFalse(TEXT("a faction that hates you does not follow you"), Rules::MeetsStandingGate(-100.0f, Threshold));
    TestFalse(TEXT("and neutral standing is not enough on its own"), Rules::MeetsStandingGate(0.0f, Threshold));

    // --- the tag gate ---
    const FGameplayTagContainer None;
    const FGameplayTagContainer Demanded = Tags({TEXT("Achievement.Slayer")});
    const FGameplayTagContainer Held = Tags({TEXT("Achievement.Slayer")});
    const FGameplayTagContainer Other = Tags({TEXT("Achievement.Cook")});

    if (TestFalse(TEXT("the demanded tag is registered, so this is a real case"), Demanded.IsEmpty())) {
        TestTrue(TEXT("holding the demanded tag passes"), Rules::MeetsTagGate(Held, Demanded));
        TestFalse(TEXT("holding a different tag does not"), Rules::MeetsTagGate(Other, Demanded));
        TestFalse(TEXT("holding nothing does not"), Rules::MeetsTagGate(None, Demanded));
    }
    TestTrue(TEXT("an NPC that demands nothing asks nothing"), Rules::MeetsTagGate(None, None));

    // --- both, because passing one is not passing both ---
    TestTrue(TEXT("meeting both gates recruits"), Rules::CanRecruit(Held, Demanded, Threshold, Threshold));
    TestFalse(TEXT("the right tags with the wrong standing does not"),
              Rules::CanRecruit(Held, Demanded, Threshold - 1.0f, Threshold));
    TestFalse(TEXT("the right standing with the wrong tags does not"),
              Rules::CanRecruit(Other, Demanded, Threshold, Threshold));

    return true;
}

#endif
