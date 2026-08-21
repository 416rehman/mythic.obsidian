
#include "Misc/AutomationTest.h"
#include "World/LivingWorld/Encounters/MythicReputationEncounterMath.h"
#include "GAS/Progression/MythicRenownRules.h"

namespace {
FMythicPartyReputation MakeRep(EMythicRenownTier MaxTier, float Heat, int32 NumPlayers = 1) {
    FMythicPartyReputation R;
    R.MaxTier = static_cast<int32>(MaxTier);
    R.AvgTier = static_cast<float>(R.MaxTier);
    R.VendettaHeat = Heat;
    R.NumPlayers = NumPlayers;
    return R;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicReputationEncounterTest,
    "Mythic.LivingWorld.ReputationEncounter",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicReputationEncounterTest::RunTest(const FString &Parameters) {
    using Math = FMythicReputationEncounterMath;

    const FGameplayTag Feared = TAG_REPUTATION_BAND_FEARED.GetTag();
    const FGameplayTag Renowned = TAG_REPUTATION_BAND_RENOWNED.GetTag();
    const FGameplayTag Empty;

    const float Base = 0.2f;

    TestEqual(TEXT("empty band -> weight unchanged (exalted party, big scale)"),
              Math::ScaleWeight(Base, Empty, MakeRep(EMythicRenownTier::Exalted, 0.f), 5.f), Base);
    TestEqual(TEXT("empty band -> weight unchanged (no party)"),
              Math::ScaleWeight(Base, Empty, FMythicPartyReputation{}, 1.f), Base);

    float Prev = -1.0f;
    for (int32 T = static_cast<int32>(EMythicRenownTier::Hated); T <= static_cast<int32>(EMythicRenownTier::Exalted); ++T) {
        const float W = Math::ScaleWeight(Base, Renowned, MakeRep(static_cast<EMythicRenownTier>(T), 0.f), 1.f);
        TestTrue(TEXT("renowned danger weight is monotonic non-decreasing in MaxTier"), W >= Prev - KINDA_SMALL_NUMBER);
        Prev = W;
    }
    const float WHonored = Math::ScaleWeight(Base, Renowned, MakeRep(EMythicRenownTier::Honored, 0.f), 1.f);
    const float WExalted = Math::ScaleWeight(Base, Renowned, MakeRep(EMythicRenownTier::Exalted, 0.f), 1.f);
    TestTrue(TEXT("renowned floor (Honored) is amplified above base"), WHonored > Base);
    TestTrue(TEXT("renowned amplifies strictly more at Exalted than Honored"), WExalted > WHonored);

    TestEqual(TEXT("renowned band, hostile party -> gated out"),
              Math::ScaleWeight(Base, Renowned, MakeRep(EMythicRenownTier::Hostile, 0.f), 1.f), 0.0f);
    TestEqual(TEXT("feared band, exalted un-hunted party -> gated out"),
              Math::ScaleWeight(Base, Feared, MakeRep(EMythicRenownTier::Exalted, 0.f), 1.f), 0.0f);
    TestEqual(TEXT("neutral, un-hunted party matches neither band (feared gated)"),
              Math::ScaleWeight(Base, Feared, MakeRep(EMythicRenownTier::Neutral, 0.f), 1.f), 0.0f);

    TestTrue(TEXT("feared band, hated party -> amplified above base"),
             Math::ScaleWeight(Base, Feared, MakeRep(EMythicRenownTier::Hated, 0.f), 1.f) > Base);
    TestTrue(TEXT("feared band, neutral-standing but HUNTED (high heat) -> amplified above base"),
             Math::ScaleWeight(Base, Feared, MakeRep(EMythicRenownTier::Neutral, 100.f), 1.f) > Base);

    TestEqual(TEXT("feared band, no party -> gated out"),
              Math::ScaleWeight(Base, Feared, FMythicPartyReputation{}, 1.f), 0.0f);
    TestEqual(TEXT("renowned band, no party -> gated out"),
              Math::ScaleWeight(Base, Renowned, FMythicPartyReputation{}, 1.f), 0.0f);

    TestEqual(TEXT("matched band, scale 0 -> base weight (gate only, no amplification)"),
              Math::ScaleWeight(Base, Renowned, MakeRep(EMythicRenownTier::Exalted, 0.f), 0.f), Base);

    TestEqual(TEXT("empty band -> pack unchanged"),
              Math::ScalePackSize(3, Empty, MakeRep(EMythicRenownTier::Exalted, 0.f), 5.f, 20), 3);
    TestTrue(TEXT("renowned exalted -> pack grows"),
             Math::ScalePackSize(3, Renowned, MakeRep(EMythicRenownTier::Exalted, 0.f), 4.f, 20) > 3);
    TestEqual(TEXT("pack growth clamped to MaxCount"),
              Math::ScalePackSize(10, Renowned, MakeRep(EMythicRenownTier::Exalted, 0.f), 50.f, 12), 12);
    TestEqual(TEXT("mismatch -> pack unchanged (never shrinks)"),
              Math::ScalePackSize(4, Renowned, MakeRep(EMythicRenownTier::Hostile, 0.f), 5.f, 20), 4);

    return true;
}
