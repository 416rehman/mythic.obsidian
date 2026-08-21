#include "Misc/AutomationTest.h"
#include "GameplayTagContainer.h"
#include "World/Secrets/MythicSecretTypes.h"
#include "World/Secrets/MythicTags_Secrets.h"
#include "GAS/MythicTags_GAS.h"

namespace {
FGameplayTagContainer MythicSecretsTests_MakeTags(std::initializer_list<FGameplayTag> Tags) {
    FGameplayTagContainer C;
    for (const FGameplayTag &T : Tags) {
        C.AddTag(T);
    }
    return C;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSecretsTest,
    "Mythic.World.Secrets",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSecretsTest::RunTest(const FString &Parameters) {
    const FGameplayTag TagA = GAS_EVENT_KILL;
    const FGameplayTag TagB = GAS_EVENT_TALKED_TO_NPC;
    const FGameplayTag TagC = GAS_EVENT_ITEM_ACQUIRED;

    const FGameplayTagContainer Empty;

    {
        FMythicStoryCondition Ungated;
        TestTrue(TEXT("not-found + ungated → can reveal"), FMythicSecretRules::CanReveal( false, Ungated, Empty));
        TestFalse(TEXT("already-found + ungated → blocked (latch wins)"), FMythicSecretRules::CanReveal( true, Ungated, Empty));

        FMythicStoryCondition Cond;
        Cond.RequireAll = MythicSecretsTests_MakeTags({TagA});
        const FGameplayTagContainer OwnA = MythicSecretsTests_MakeTags({TagA});
        TestFalse(TEXT("already-found + satisfied condition → still blocked"), FMythicSecretRules::CanReveal( true, Cond, OwnA));
    }

    {
        FMythicStoryCondition All;
        All.RequireAll = MythicSecretsTests_MakeTags({TagA, TagB});
        TestTrue(TEXT("RequireAll {A,B} met by {A,B}"), FMythicSecretRules::CanReveal(false, All, MythicSecretsTests_MakeTags({TagA, TagB})));
        TestFalse(TEXT("RequireAll {A,B} unmet by {A}"), FMythicSecretRules::CanReveal(false, All, MythicSecretsTests_MakeTags({TagA})));
        TestFalse(TEXT("RequireAll {A,B} unmet by {}"), FMythicSecretRules::CanReveal(false, All, Empty));

        FMythicStoryCondition Any;
        Any.RequireAny = MythicSecretsTests_MakeTags({TagB, TagC});
        TestTrue(TEXT("RequireAny {B,C} met by {A,B}"), FMythicSecretRules::CanReveal(false, Any, MythicSecretsTests_MakeTags({TagA, TagB})));
        TestFalse(TEXT("RequireAny {B,C} unmet by {A}"), FMythicSecretRules::CanReveal(false, Any, MythicSecretsTests_MakeTags({TagA})));

        FMythicStoryCondition Block;
        Block.BlockAny = MythicSecretsTests_MakeTags({TagC});
        TestTrue(TEXT("BlockAny {C} passes when C absent"), FMythicSecretRules::CanReveal(false, Block, MythicSecretsTests_MakeTags({TagA, TagB})));
        TestFalse(TEXT("BlockAny {C} fails when C present"), FMythicSecretRules::CanReveal(false, Block, MythicSecretsTests_MakeTags({TagA, TagC})));
    }

    {
        const FGameplayTag FoundTag = TAG_Secret_Found;
        FGameplayTagContainer Owned;

        const bool bAlreadyFound1 = Owned.HasTagExact(FoundTag);
        TestFalse(TEXT("first encounter: FoundTag not yet owned"), bAlreadyFound1);
        const bool bReveal1 = FMythicSecretRules::CanReveal(bAlreadyFound1, FMythicStoryCondition(), Owned);
        TestTrue(TEXT("first encounter reveals"), bReveal1);

        Owned.AddTag(FoundTag);

        const bool bAlreadyFound2 = Owned.HasTagExact(FoundTag);
        TestTrue(TEXT("second encounter: FoundTag now owned (latched)"), bAlreadyFound2);
        const bool bReveal2 = FMythicSecretRules::CanReveal(bAlreadyFound2, FMythicStoryCondition(), Owned);
        TestFalse(TEXT("second encounter is blocked by the latch"), bReveal2);
    }

    {
        const FGameplayTag Shrine1Found = TagA;
        const FGameplayTag Shrine2Found = TagB;

        FMythicSecretDef Shrine2;
        Shrine2.FoundTag = Shrine2Found;
        Shrine2.RequireCondition.RequireAll = MythicSecretsTests_MakeTags({Shrine1Found});

        FGameplayTagContainer Owned;

        const bool bS2Blocked = FMythicSecretRules::CanReveal(Owned.HasTagExact(Shrine2Found), Shrine2.RequireCondition, Owned);
        TestFalse(TEXT("shrine 2 blocked before shrine 1 is found (chain gate)"), bS2Blocked);

        Owned.AddTag(Shrine1Found);
        const bool bS2Now = FMythicSecretRules::CanReveal(Owned.HasTagExact(Shrine2Found), Shrine2.RequireCondition, Owned);
        TestTrue(TEXT("shrine 2 reveals once shrine 1 is found"), bS2Now);

        Owned.AddTag(Shrine2Found);
        const bool bS2Again = FMythicSecretRules::CanReveal(Owned.HasTagExact(Shrine2Found), Shrine2.RequireCondition, Owned);
        TestFalse(TEXT("shrine 2 blocked on re-attempt (latched)"), bS2Again);
    }

    return true;
}
