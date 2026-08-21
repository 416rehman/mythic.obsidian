
#include "Misc/AutomationTest.h"
#include "GameplayTagContainer.h"
#include "World/EnvironmentController/MythicEnvironmentHazardComponent.h"
#include "GAS/MythicTags_GAS.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHazardSuppressionTest,
    "Mythic.World.HazardSuppression",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicHazardSuppressionTest::RunTest(const FString &Parameters) {
    const FGameplayTag WarmTag = GAS_BUFF_FORTIFY;
    const FGameplayTag OtherTag = GAS_BUFF_HEALING;
    const FGameplayTag DebuffParent = GAS_DEBUFF;
    const FGameplayTag DebuffChild = GAS_DEBUFF_BLEEDING;

    {
        FGameplayTagContainer Owned;
        Owned.AddTag(WarmTag);
        TestFalse(TEXT("empty suppression list → not suppressed"),
                  UMythicEnvironmentHazardComponent::IsHazardSuppressed(Owned, {}));
    }

    {
        FGameplayTagContainer Owned;
        Owned.AddTag(WarmTag);
        TestTrue(TEXT("owns the suppressor → suppressed"),
                 UMythicEnvironmentHazardComponent::IsHazardSuppressed(Owned, {WarmTag}));
    }

    {
        FGameplayTagContainer Owned;
        Owned.AddTag(OtherTag);
        TestFalse(TEXT("owns only an unrelated tag → not suppressed"),
                  UMythicEnvironmentHazardComponent::IsHazardSuppressed(Owned, {WarmTag}));
    }

    {
        FGameplayTagContainer Owned;
        Owned.AddTag(OtherTag);
        TestTrue(TEXT("owns one of several suppressors → suppressed"),
                 UMythicEnvironmentHazardComponent::IsHazardSuppressed(Owned, {WarmTag, OtherTag}));
    }

    {
        FGameplayTagContainer Owned;
        Owned.AddTag(DebuffChild);
        TestTrue(TEXT("owns a child of the suppressor (hierarchical) → suppressed"),
                 UMythicEnvironmentHazardComponent::IsHazardSuppressed(Owned, {DebuffParent}));
    }

    {
        FGameplayTagContainer Owned;
        Owned.AddTag(WarmTag);
        TestFalse(TEXT("invalid suppressor entry is skipped"),
                  UMythicEnvironmentHazardComponent::IsHazardSuppressed(Owned, {FGameplayTag(), OtherTag}));
    }

    {
        FGameplayTagContainer Owned;
        TestFalse(TEXT("player owns no tags → not suppressed"),
                  UMythicEnvironmentHazardComponent::IsHazardSuppressed(Owned, {WarmTag, OtherTag}));
    }

    return true;
}
