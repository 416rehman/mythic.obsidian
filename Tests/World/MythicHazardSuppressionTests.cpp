// Mythic — environmental hazard suppression unit tests.
// Covers the pure gate the server-side hazard evaluator (UMythicEnvironmentHazardComponent::EvaluateCondition) is built
// on: a sheltered/warm player (owning a suppression tag, hierarchical) suppresses an otherwise-matching hazard. The live
// apply/remove + the responsive tag-listener re-eval are server-driven; this locks the decision.
// Run via: Session Frontend → Automation → Mythic.World.HazardSuppression

#include "Misc/AutomationTest.h"
#include "GameplayTagContainer.h"
#include "World/EnvironmentController/MythicEnvironmentHazardComponent.h"
#include "GAS/MythicTags_GAS.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHazardSuppressionTest,
    "Mythic.World.HazardSuppression",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicHazardSuppressionTest::RunTest(const FString &Parameters) {
    // Use registered native tags as stand-ins: GAS_BUFF_FORTIFY ~ "warm/sheltered", and the GAS_DEBUFF (parent) /
    // GAS_DEBUFF_BLEEDING (child) pair to exercise hierarchical matching.
    const FGameplayTag WarmTag = GAS_BUFF_FORTIFY;
    const FGameplayTag OtherTag = GAS_BUFF_HEALING;
    const FGameplayTag DebuffParent = GAS_DEBUFF;
    const FGameplayTag DebuffChild = GAS_DEBUFF_BLEEDING;

    // No suppression tags configured → never suppressed (the default; byte-identical to the prior behaviour).
    {
        FGameplayTagContainer Owned;
        Owned.AddTag(WarmTag);
        TestFalse(TEXT("empty suppression list → not suppressed"),
                  UMythicEnvironmentHazardComponent::IsHazardSuppressed(Owned, {}));
    }

    // Player owns the suppression tag → suppressed.
    {
        FGameplayTagContainer Owned;
        Owned.AddTag(WarmTag);
        TestTrue(TEXT("owns the suppressor → suppressed"),
                 UMythicEnvironmentHazardComponent::IsHazardSuppressed(Owned, {WarmTag}));
    }

    // Player owns a DIFFERENT tag → not suppressed.
    {
        FGameplayTagContainer Owned;
        Owned.AddTag(OtherTag);
        TestFalse(TEXT("owns only an unrelated tag → not suppressed"),
                  UMythicEnvironmentHazardComponent::IsHazardSuppressed(Owned, {WarmTag}));
    }

    // Multiple suppressors; player owns one of them → suppressed.
    {
        FGameplayTagContainer Owned;
        Owned.AddTag(OtherTag);
        TestTrue(TEXT("owns one of several suppressors → suppressed"),
                 UMythicEnvironmentHazardComponent::IsHazardSuppressed(Owned, {WarmTag, OtherTag}));
    }

    // Hierarchical: the suppressor is a PARENT and the player owns a CHILD → HasTag matches → suppressed.
    {
        FGameplayTagContainer Owned;
        Owned.AddTag(DebuffChild);
        TestTrue(TEXT("owns a child of the suppressor (hierarchical) → suppressed"),
                 UMythicEnvironmentHazardComponent::IsHazardSuppressed(Owned, {DebuffParent}));
    }

    // An invalid entry in the suppression list is skipped (no crash, no false positive).
    {
        FGameplayTagContainer Owned;
        Owned.AddTag(WarmTag);
        TestFalse(TEXT("invalid suppressor entry is skipped"),
                  UMythicEnvironmentHazardComponent::IsHazardSuppressed(Owned, {FGameplayTag(), OtherTag}));
    }

    // Empty owned tags → never suppressed.
    {
        FGameplayTagContainer Owned;
        TestFalse(TEXT("player owns no tags → not suppressed"),
                  UMythicEnvironmentHazardComponent::IsHazardSuppressed(Owned, {WarmTag, OtherTag}));
    }

    return true;
}
