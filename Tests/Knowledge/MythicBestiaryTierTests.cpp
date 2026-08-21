
#include "Misc/AutomationTest.h"
#include "Knowledge/MythicCodexTypes.h"
#include "Knowledge/MythicTags_Knowledge.h"
#include "GAS/MythicTags_GAS.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicBestiaryTierTest,
    "Mythic.Knowledge.Bestiary",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicBestiaryTierTest::RunTest(const FString &Parameters) {
    using Rules = FMythicBestiaryRules;
    using ECodexTier = EMythicCodexTier;

    {
        TestEqual(TEXT("never seen, no kills -> Unknown"), Rules::TierForKills(0, false, 1, 10), ECodexTier::Unknown);
        TestEqual(TEXT("encountered, no kills -> Sighted"), Rules::TierForKills(0, true, 1, 10), ECodexTier::Sighted);
        TestEqual(TEXT("first kill -> Basic (T1 edge)"), Rules::TierForKills(1, true, 1, 10), ECodexTier::Basic);
        TestEqual(TEXT("kills just under T2 -> still Basic"), Rules::TierForKills(9, true, 1, 10), ECodexTier::Basic);
        TestEqual(TEXT("kills at T2 -> Full (T2 edge)"), Rules::TierForKills(10, true, 1, 10), ECodexTier::Full);
        TestEqual(TEXT("kills beyond T2 -> Full"), Rules::TierForKills(500, true, 1, 10), ECodexTier::Full);
        TestEqual(TEXT("kills >= T1 without encounter flag -> Basic"), Rules::TierForKills(1, false, 1, 10), ECodexTier::Basic);
    }

    {
        TestEqual(TEXT("T1=3: 2 kills (encountered) -> Sighted"), Rules::TierForKills(2, true, 3, 5), ECodexTier::Sighted);
        TestEqual(TEXT("T1=3: 2 kills (flag unset) -> Sighted (kills imply sighting)"), Rules::TierForKills(2, false, 3, 5), ECodexTier::Sighted);
        TestEqual(TEXT("T1=3: 3 kills -> Basic"), Rules::TierForKills(3, true, 3, 5), ECodexTier::Basic);
        TestEqual(TEXT("T1=3,T2=5: 4 kills -> Basic"), Rules::TierForKills(4, true, 3, 5), ECodexTier::Basic);
        TestEqual(TEXT("T2=5: 5 kills -> Full"), Rules::TierForKills(5, true, 3, 5), ECodexTier::Full);
    }

    {
        TestEqual(TEXT("T1=0 clamps to 1: 0 kills stays Sighted"), Rules::TierForKills(0, true, 0, 10), ECodexTier::Sighted);
        TestEqual(TEXT("T1=0 clamps to 1: 1 kill is Basic"), Rules::TierForKills(1, true, 0, 10), ECodexTier::Basic);
        TestEqual(TEXT("T2<T1 clamps to T1: kills at T1 -> Full"), Rules::TierForKills(5, true, 5, 2), ECodexTier::Full);
        TestEqual(TEXT("T2<T1 clamps to T1: kills below T1 (encountered) -> Sighted"), Rules::TierForKills(4, true, 5, 2), ECodexTier::Sighted);
    }

    {
        TestFalse(TEXT("Unknown hides resistances"), Rules::RevealsResistances(ECodexTier::Unknown));
        TestFalse(TEXT("Sighted hides resistances"), Rules::RevealsResistances(ECodexTier::Sighted));
        TestFalse(TEXT("Basic hides resistances"), Rules::RevealsResistances(ECodexTier::Basic));
        TestTrue(TEXT("Full reveals resistances"), Rules::RevealsResistances(ECodexTier::Full));
    }

    {
        const FGameplayTag CreatureGeneric = CODEX_BESTIARY_CREATURE_GENERIC;
        const FGameplayTag HumanoidGeneric = CODEX_BESTIARY_HUMANOID_GENERIC;
        const FGameplayTag HumanoidBandit = CODEX_BESTIARY_HUMANOID_BANDIT;
        const FGameplayTag NpcTypeBandit = NPC_TYPE_BANDIT;

        {
            FGameplayTagContainer Owned;
            Owned.AddTag(AI_KIND_CREATURE);
            Owned.AddTag(NpcTypeBandit);
            Owned.AddTag(HumanoidBandit);
            TestEqual(TEXT("explicit Codex.Bestiary.* stamp wins"), Rules::MakeBestiaryKeyFromOwnedTags(Owned), HumanoidBandit);
        }

        {
            FGameplayTagContainer Owned;
            Owned.AddTag(NpcTypeBandit);
            TestEqual(TEXT("NPC.Type.Bandit maps to Codex.Bestiary.Humanoid.Bandit"), Rules::MakeBestiaryKeyFromOwnedTags(Owned), HumanoidBandit);
        }

        {
            FGameplayTagContainer Owned;
            Owned.AddTag(AI_KIND_HUMANOID);
            Owned.AddTag(NpcTypeBandit);
            TestEqual(TEXT("NPC.Type mapping beats AI.Kind fallback"), Rules::MakeBestiaryKeyFromOwnedTags(Owned), HumanoidBandit);
        }

        {
            FGameplayTagContainer Owned;
            Owned.AddTag(AI_KIND_CREATURE);
            TestEqual(TEXT("AI.Kind.Creature falls back to Creature.Generic"), Rules::MakeBestiaryKeyFromOwnedTags(Owned), CreatureGeneric);
        }
        {
            FGameplayTagContainer Owned;
            Owned.AddTag(AI_KIND_HUMANOID);
            TestEqual(TEXT("AI.Kind.Humanoid falls back to Humanoid.Generic"), Rules::MakeBestiaryKeyFromOwnedTags(Owned), HumanoidGeneric);
        }

        {
            const FGameplayTagContainer Owned;
            TestFalse(TEXT("no identity tags -> invalid key"), Rules::MakeBestiaryKeyFromOwnedTags(Owned).IsValid());
        }
    }

    return true;
}
