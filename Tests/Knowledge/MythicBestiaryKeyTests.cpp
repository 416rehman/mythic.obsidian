#include "Misc/AutomationTest.h"

#include "Knowledge/MythicCodexTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace {
    FGameplayTagContainer Owned(std::initializer_list<const TCHAR *> Names) {
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicBestiaryKeyTest,
                                 "Mythic.Knowledge.BestiaryKey",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

/**
 * A kill credits the most specific bestiary page that exists, not the most specific tag the NPC owns.
 *
 * The bestiary has a page for Bandit, not for every variant of bandit. Mapping a variant straight to
 * Codex.Bestiary.Humanoid.<whole suffix> asks for a page nobody wrote, so every Ambusher and Raider kill fell
 * through to the generic humanoid entry and the codex never filled in.
 */
bool FMythicBestiaryKeyTest::RunTest(const FString &Parameters) {
    using Rules = FMythicBestiaryRules;

    const FGameplayTag Bandit = FGameplayTag::RequestGameplayTag(FName(TEXT("Codex.Bestiary.Humanoid.Bandit")), false);
    const FGameplayTag HumanoidGeneric = FGameplayTag::RequestGameplayTag(FName(TEXT("Codex.Bestiary.Humanoid.Generic")), false);
    if (!TestTrue(TEXT("the bandit page and the generic humanoid page are both registered"),
                  Bandit.IsValid() && HumanoidGeneric.IsValid())) {
        return false;
    }

    // The defect: a variant tag that is registered, whose codex page is not.
    const FGameplayTag Ambusher = FGameplayTag::RequestGameplayTag(FName(TEXT("NPC.Type.Bandit.Ambusher")), false);
    if (TestTrue(TEXT("the variant tag is registered, so this is a real case"), Ambusher.IsValid())) {
        TestFalse(TEXT("and its own codex page deliberately does not exist"),
                  FGameplayTag::RequestGameplayTag(FName(TEXT("Codex.Bestiary.Humanoid.Bandit.Ambusher")), false).IsValid());
        TestEqual(TEXT("a variant kill credits its parent's page"),
                  Rules::MakeBestiaryKeyFromOwnedTags(Owned({TEXT("NPC.Type.Bandit.Ambusher")})), Bandit);
    }

    TestEqual(TEXT("a raider credits the same page as an ambusher"),
              Rules::MakeBestiaryKeyFromOwnedTags(Owned({TEXT("NPC.Type.Bandit.Raider")})), Bandit);

    TestEqual(TEXT("the exact parent type still maps directly"),
              Rules::MakeBestiaryKeyFromOwnedTags(Owned({TEXT("NPC.Type.Bandit")})), Bandit);

    // A type with no page anywhere up its chain still has to land somewhere rather than returning nothing.
    TestEqual(TEXT("a type with no page falls back to the generic humanoid"),
              Rules::MakeBestiaryKeyFromOwnedTags(Owned({TEXT("NPC.Type.Merchant")})), HumanoidGeneric);

    // An authored override is the more specific statement, so it beats whatever the type would derive.
    TestEqual(TEXT("an explicit codex tag wins over the derived one"),
              Rules::MakeBestiaryKeyFromOwnedTags(Owned({TEXT("Codex.Bestiary.Humanoid.Bandit"), TEXT("NPC.Type.Merchant")})),
              Bandit);

    // The branch follows what the thing is. A creature carrying a type tag was forced down the humanoid branch
    // and could never reach a creature page, however it was authored.
    const FGameplayTag CreatureGeneric = FGameplayTag::RequestGameplayTag(FName(TEXT("Codex.Bestiary.Creature.Generic")), false);
    if (TestTrue(TEXT("the creature page is registered"), CreatureGeneric.IsValid())) {
        TestEqual(TEXT("a creature with a type tag lands on a creature page, not a humanoid one"),
                  Rules::MakeBestiaryKeyFromOwnedTags(Owned({TEXT("AI.Kind.Creature"), TEXT("NPC.Type.Merchant")})),
                  CreatureGeneric);
    }

    return true;
}

#endif
