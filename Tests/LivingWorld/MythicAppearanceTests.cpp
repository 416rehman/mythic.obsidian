// Mythic Living World — Appearance + Faction Color Tests
// Covers the pure, deterministic resolution core delivered in Step 1:
//   - MythicAppearanceDefaults::GetCodeDefaultOutfitSets / palettes (well-formed, >=1 always-eligible)
//   - FMythicAppearanceResolver::Resolve (deterministic, varies by hash, role/wealth gating, demographic unpack)
//   - FMythicAppearanceResolver::WealthTierFromHash (in range, deterministic)
//   - MythicFactionColor::DeterministicColorForId / GetFactionColor (deterministic, hue-separated, override-aware)
// Run via: Session Frontend → Automation → Mythic.LivingWorld.Appearance / Mythic.LivingWorld.FactionColor

#include "Misc/AutomationTest.h"
#include "World/LivingWorld/Appearance/AppearanceTypes.h"
#include "World/LivingWorld/Factions/FactionColor.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h" // native NPC.Role.* tags

namespace {
    // Pack demographic flags the same way FMythicNPCGenerator does: [3b age @5][1b gender @4].
    uint8 PackDemographics(uint8 AgeBracket, bool bFemale) {
        return static_cast<uint8>(((AgeBracket & 0x7u) << 5) | ((bFemale ? 1u : 0u) << 4));
    }

    FMythicAppearance ResolveWith(uint32 NameHash, uint8 Demo, const FGameplayTag& Role, uint8 WealthTier,
                                  TConstArrayView<FMythicOutfitSet> Sets) {
        return FMythicAppearanceResolver::Resolve(
            NameHash, Demo, Role, /*FactionIndex*/ 3, WealthTier,
            FColor::Red, FColor::Blue, Sets,
            MythicAppearanceDefaults::GetCodeDefaultSkinTonePalette(),
            MythicAppearanceDefaults::GetCodeDefaultHairTonePalette());
    }
} // namespace

// ─────────────────────────────────────────────────────────────
// Code defaults are well-formed
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAppearanceDefaultsWellFormedTest,
    "Mythic.LivingWorld.Appearance.Defaults.WellFormed",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAppearanceDefaultsWellFormedTest::RunTest(const FString& Parameters) {
    TConstArrayView<FMythicOutfitSet> Sets = MythicAppearanceDefaults::GetCodeDefaultOutfitSets();
    TestTrue(TEXT("At least one code-default outfit set"), Sets.Num() > 0);

    bool bSawAlwaysEligible = false;
    for (const FMythicOutfitSet& S : Sets) {
        TestTrue(TEXT("Outfit set weight is positive"), S.RelativeWeight > 0.0f);
        TestTrue(TEXT("Outfit set wealth band ordered"), S.MinWealthTier <= S.MaxWealthTier);
        TestTrue(TEXT("Outfit set body-type count >= 1"), S.BodyTypeCount >= 1);
        if (!S.RoleTag.IsValid() && S.MinWealthTier == 0 && S.MaxWealthTier >= 3) {
            bSawAlwaysEligible = true;
        }
    }
    TestTrue(TEXT("There is an always-eligible (any role, full wealth) outfit set"), bSawAlwaysEligible);

    TestTrue(TEXT("Skin palette is non-empty"), MythicAppearanceDefaults::GetCodeDefaultSkinTonePalette().Num() > 0);
    TestTrue(TEXT("Hair palette is non-empty"), MythicAppearanceDefaults::GetCodeDefaultHairTonePalette().Num() > 0);
    return true;
}

// ─────────────────────────────────────────────────────────────
// Resolve — deterministic, in-range, varies by hash
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAppearanceResolveDeterministicTest,
    "Mythic.LivingWorld.Appearance.Resolve.Deterministic",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAppearanceResolveDeterministicTest::RunTest(const FString& Parameters) {
    TConstArrayView<FMythicOutfitSet> Sets = MythicAppearanceDefaults::GetCodeDefaultOutfitSets();
    const uint8 Demo = PackDemographics(2, false);

    // Determinism: same inputs => byte-identical descriptor.
    const FMythicAppearance A = ResolveWith(0xABCDEF01u, Demo, FGameplayTag(), 1, Sets);
    const FMythicAppearance B = ResolveWith(0xABCDEF01u, Demo, FGameplayTag(), 1, Sets);
    TestTrue(TEXT("Resolve is deterministic for identical inputs"), A == B);

    // Variation: different hashes produce different looks across a sweep (not all identical).
    bool bSawDifference = false;
    for (uint32 i = 1; i < 64 && !bSawDifference; ++i) {
        const uint32 Hash = i * 2654435761u;
        const FMythicAppearance R = ResolveWith(Hash, Demo, FGameplayTag(), 1, Sets);
        if (R != A) {
            bSawDifference = true;
        }
    }
    TestTrue(TEXT("Resolve varies across different name hashes"), bSawDifference);

    // Colors are copied straight through.
    TestEqual(TEXT("Primary color copied from faction primary"), A.PrimaryColor, FColor::Red);
    TestEqual(TEXT("Secondary color copied from faction secondary"), A.SecondaryColor, FColor::Blue);

    // Chosen outfit set index is valid.
    TestTrue(TEXT("OutfitSetId indexes the candidate set list"), Sets.IsValidIndex(A.OutfitSetId));
    return true;
}

// ─────────────────────────────────────────────────────────────
// Resolve — demographic unpack
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAppearanceDemographicUnpackTest,
    "Mythic.LivingWorld.Appearance.Resolve.DemographicUnpack",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAppearanceDemographicUnpackTest::RunTest(const FString& Parameters) {
    TConstArrayView<FMythicOutfitSet> Sets = MythicAppearanceDefaults::GetCodeDefaultOutfitSets();

    {
        const FMythicAppearance R = ResolveWith(0x1234u, PackDemographics(4, true), FGameplayTag(), 0, Sets);
        TestEqual(TEXT("Age bracket unpacked"), (int32)R.AgeBracket, 4);
        TestEqual(TEXT("Female bit unpacked"), (int32)R.bIsFemale, 1);
    }
    {
        const FMythicAppearance R = ResolveWith(0x1234u, PackDemographics(2, false), FGameplayTag(), 0, Sets);
        TestEqual(TEXT("Age bracket unpacked (adult)"), (int32)R.AgeBracket, 2);
        TestEqual(TEXT("Male bit unpacked"), (int32)R.bIsFemale, 0);
    }
    return true;
}

// ─────────────────────────────────────────────────────────────
// Resolve — role + wealth gating selects the right set
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAppearanceGatingTest,
    "Mythic.LivingWorld.Appearance.Resolve.Gating",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAppearanceGatingTest::RunTest(const FString& Parameters) {
    // Build a controlled candidate list: a poor-only set (idx 0) and a rich-only set (idx 1), both any-role.
    TArray<FMythicOutfitSet> Sets;
    {
        FMythicOutfitSet& Poor = Sets.AddDefaulted_GetRef();
        Poor.MinWealthTier = 0;
        Poor.MaxWealthTier = 0;
        Poor.RelativeWeight = 1.0f;
        Poor.BodyTypeCount = 1;

        FMythicOutfitSet& Rich = Sets.AddDefaulted_GetRef();
        Rich.MinWealthTier = 3;
        Rich.MaxWealthTier = 3;
        Rich.RelativeWeight = 1.0f;
        Rich.BodyTypeCount = 1;
    }

    // Wealth tier 0 can only choose the poor set (idx 0) across a hash sweep.
    for (uint32 i = 1; i < 64; ++i) {
        const FMythicAppearance R = ResolveWith(i * 2654435761u, 0, FGameplayTag(), /*WealthTier*/ 0, Sets);
        TestEqual(TEXT("Wealth 0 always lands on the poor set"), (int32)R.OutfitSetId, 0);
    }
    // Wealth tier 3 can only choose the rich set (idx 1).
    for (uint32 i = 1; i < 64; ++i) {
        const FMythicAppearance R = ResolveWith(i * 2654435761u, 0, FGameplayTag(), /*WealthTier*/ 3, Sets);
        TestEqual(TEXT("Wealth 3 always lands on the rich set"), (int32)R.OutfitSetId, 1);
    }

    // Role gating: a role-restricted set is skipped when the NPC's role does not match.
    {
        TArray<FMythicOutfitSet> RoleSets;
        FMythicOutfitSet& AnyRole = RoleSets.AddDefaulted_GetRef();
        AnyRole.MinWealthTier = 0;
        AnyRole.MaxWealthTier = 3;
        AnyRole.RelativeWeight = 1.0f;
        AnyRole.BodyTypeCount = 1;

        FMythicOutfitSet& GuardOnly = RoleSets.AddDefaulted_GetRef();
        GuardOnly.RoleTag = TAG_NPC_ROLE_GUARD;
        GuardOnly.MinWealthTier = 0;
        GuardOnly.MaxWealthTier = 3;
        GuardOnly.RelativeWeight = 1000.0f; // would dominate IF eligible
        GuardOnly.BodyTypeCount = 1;

        // A civilian (role mismatch) can never get the guard-only set even though it has huge weight.
        for (uint32 i = 1; i < 64; ++i) {
            const FMythicAppearance R = ResolveWith(i * 2654435761u, 0, TAG_NPC_ROLE_CIVILIAN, 1, RoleSets);
            TestEqual(TEXT("Non-guard never selects the guard-only set"), (int32)R.OutfitSetId, 0);
        }
    }

    return true;
}

// ─────────────────────────────────────────────────────────────
// WealthTierFromHash — in range, deterministic
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAppearanceWealthTierTest,
    "Mythic.LivingWorld.Appearance.WealthTier.RangeDeterministic",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAppearanceWealthTierTest::RunTest(const FString& Parameters) {
    bool bSaw0 = false, bSaw3 = false;
    for (uint32 i = 0; i < 512; ++i) {
        const uint32 Hash = i * 2654435761u + 1u;
        const uint8 Tier = FMythicAppearanceResolver::WealthTierFromHash(Hash);
        TestTrue(TEXT("Wealth tier in [0,3]"), Tier <= 3);
        TestEqual(TEXT("WealthTierFromHash is deterministic"), Tier, FMythicAppearanceResolver::WealthTierFromHash(Hash));
        bSaw0 |= (Tier == 0);
        bSaw3 |= (Tier == 3);
    }
    TestTrue(TEXT("Wealth tier reaches 0 across the sweep"), bSaw0);
    TestTrue(TEXT("Wealth tier reaches 3 across the sweep"), bSaw3);
    return true;
}

// ─────────────────────────────────────────────────────────────
// Faction color — deterministic, hue-separated, override-aware
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicFactionColorTest,
    "Mythic.LivingWorld.FactionColor.DeterministicAndOverride",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicFactionColorTest::RunTest(const FString& Parameters) {
    // Deterministic per index.
    for (int32 i = 0; i < 32; ++i) {
        const uint8 Idx = static_cast<uint8>(i);
        TestEqual(TEXT("DeterministicColorForId is stable"),
                  MythicFactionColor::DeterministicColorForId(Idx),
                  MythicFactionColor::DeterministicColorForId(Idx));
    }

    // Adjacent indices land on distinguishable colors (golden-ratio hue separation).
    bool bAdjacentDiffer = true;
    for (int32 i = 0; i < 16; ++i) {
        const FColor A = MythicFactionColor::DeterministicColorForId(static_cast<uint8>(i));
        const FColor B = MythicFactionColor::DeterministicColorForId(static_cast<uint8>(i + 1));
        if (A == B) {
            bAdjacentDiffer = false;
            break;
        }
    }
    TestTrue(TEXT("Adjacent faction indices have distinct colors"), bAdjacentDiffer);

    // Colors are opaque.
    TestEqual(TEXT("Deterministic faction color is opaque"),
              (int32)MythicFactionColor::DeterministicColorForId(7).A, 255);

    // GetFactionColor: no override => deterministic-from-id path.
    {
        FMythicFactionData Data;
        Data.bOverrideFactionColor = false;
        TestEqual(TEXT("No override uses the deterministic color"),
                  MythicFactionColor::GetFactionColor(Data, 5),
                  MythicFactionColor::DeterministicColorForId(5));
    }
    // GetFactionColor: override => authored color, regardless of index.
    {
        FMythicFactionData Data;
        Data.bOverrideFactionColor = true;
        Data.FactionColor = FColor(10, 20, 30, 255);
        TestEqual(TEXT("Override returns the authored color"),
                  MythicFactionColor::GetFactionColor(Data, 5), FColor(10, 20, 30, 255));
    }
    return true;
}
