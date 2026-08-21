
#include "Misc/AutomationTest.h"
#include "Itemization/Cooking/MythicCookingCore.h"
#include "Itemization/Cooking/MythicCookingRecipe.h"
#include "Itemization/Cooking/MythicTags_Cooking.h"
#include "World/Gathering/MythicTags_Gathering.h"
#include "World/Gathering/MythicYieldQuality.h"
#include "Progression/MythicUnlockComponent.h"
#include "Itemization/Inventory/Fragments/Passive/YieldQualityFragment.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCookingPotencyTest,
    "Mythic.Itemization.Cooking.Potency",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCookingPotencyTest::RunTest(const FString &Parameters) {
    using C = FMythicCookingCore;

    TestEqual(TEXT("neutral inputs cook at exactly 1.0"), C::ComputePotency(1.0f, 1.0f, 50, 0.0f), 1.0f);

    TestTrue(TEXT("higher quality raises potency"), C::ComputePotency(1.35f, 1.0f, 0, 0.0f) > C::ComputePotency(1.0f, 1.0f, 0, 0.0f));
    TestTrue(TEXT("lower freshness lowers potency"), C::ComputePotency(1.0f, 0.75f, 0, 0.0f) < C::ComputePotency(1.0f, 1.0f, 0, 0.0f));
    TestTrue(TEXT("cooking level raises potency (when the recipe opts in)"),
             C::ComputePotency(1.0f, 1.0f, 20, 0.01f) > C::ComputePotency(1.0f, 1.0f, 0, 0.01f));

    TestEqual(TEXT("hard clamp: absurd quality"), C::ComputePotency(1000.0f, 1.0f, 0, 0.0f), 2.0f);
    TestEqual(TEXT("hard clamp: absurd level scaling"), C::ComputePotency(1.35f, 1.0f, 1000000, 10.0f), 2.0f);
    TestEqual(TEXT("hard clamp: even a larger caller cap is pinned to 2.0"), C::ComputePotency(1000.0f, 1.0f, 0, 0.0f, 50.0f), 2.0f);
    TestTrue(TEXT("a stricter caller cap applies"), C::ComputePotency(1000.0f, 1.0f, 0, 0.0f, 1.5f) <= 1.5f);

    TestEqual(TEXT("negative quality floors at 0"), C::ComputePotency(-5.0f, 1.0f, 0, 0.0f), 0.0f);
    TestEqual(TEXT("negative freshness floors at 0"), C::ComputePotency(1.0f, -1.0f, 0, 0.0f), 0.0f);

    TestEqual(TEXT("quantized: equal inputs are bit-identical"),
              C::ComputePotency(1.17f, 0.93f, 13, 0.007f), C::ComputePotency(1.17f, 0.93f, 13, 0.007f));

    TestEqual(TEXT("fresh factor at 1.0"), C::FreshnessPotencyFactor(1.0f, 0.75f), 1.0f);
    TestEqual(TEXT("spoiled factor bottoms at the floor"), C::FreshnessPotencyFactor(0.0f, 0.75f), 0.75f);
    TestEqual(TEXT("degenerate floor clamps into [0,1]"), C::FreshnessPotencyFactor(0.0f, 7.0f), 1.0f);

    TestEqual(TEXT("portion crit inert by default"), C::PortionCritChance(100, 0.0f, 0.0f, 0.25f), 0.0f);
    TestTrue(TEXT("portion crit grows with level"), C::PortionCritChance(50, 0.01f, 0.002f, 0.25f) > C::PortionCritChance(0, 0.01f, 0.002f, 0.25f));
    TestEqual(TEXT("portion crit clamps at the recipe max"), C::PortionCritChance(100000, 0.01f, 0.01f, 0.25f), 0.25f);
    TestEqual(TEXT("portion crit never exceeds the 50% ceiling"), C::PortionCritChance(100000, 0.5f, 0.5f, 5.0f), 0.5f);

    FMythicYieldQualityRules Rules;
    const float PristineMult = FMythicYieldQuality::PotencyMultiplierForTierValue(Rules, 3.0f);
    TestTrue(TEXT("an all-Pristine, fresh, high-level pot beats neutral"),
             C::ComputePotency(PristineMult, 1.0f, 50, 0.005f) > 1.0f);
    TestTrue(TEXT("...and still respects the rail"), C::ComputePotency(PristineMult, 1.0f, 50, 0.005f) <= 2.0f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCookingExperimentTest,
    "Mythic.Itemization.Cooking.ExperimentAndDiscovery",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCookingExperimentTest::RunTest(const FString &Parameters) {
    UMythicCookingRecipe *Stew = NewObject<UMythicCookingRecipe>();
    Stew->bExperimentFallback = true;
    Stew->RecipeId = TAG_Itemization_Recipe_Cooking;
    Stew->Process.Priority = -100;

    UMythicCookingRecipe *Hidden = NewObject<UMythicCookingRecipe>();
    Hidden->bHiddenUntilDiscovered = true;
    Hidden->RecipeId = TAG_Station_Cooking_Campfire;
    Hidden->Process.Priority = 5;

    UMythicCookingRecipe *HigherPriority = NewObject<UMythicCookingRecipe>();
    HigherPriority->RecipeId = TAG_Station_Cooking_CookPot;
    HigherPriority->Process.Priority = 10;

    {
        TArray<UMythicCookingRecipe *> None;
        TestNull(TEXT("no match ⇒ fallback stew"), UMythicCookingRecipe::PickBestExperimentCandidate(None));
    }
    {
        TArray<UMythicCookingRecipe *> OnlyStew = {Stew};
        TestNull(TEXT("the fallback never matches itself"), UMythicCookingRecipe::PickBestExperimentCandidate(OnlyStew));
    }
    {
        TArray<UMythicCookingRecipe *> Mixed = {Stew, Hidden, HigherPriority};
        TestEqual(TEXT("highest priority wins"), UMythicCookingRecipe::PickBestExperimentCandidate(Mixed), HigherPriority);
    }
    {
        UMythicCookingRecipe *TieA = NewObject<UMythicCookingRecipe>();
        TieA->RecipeId = TAG_Station_Cooking_Campfire;
        TieA->Process.Priority = 5;
        UMythicCookingRecipe *TieB = NewObject<UMythicCookingRecipe>();
        TieB->RecipeId = TAG_Station_Cooking_CookPot;
        TieB->Process.Priority = 5;
        TArray<UMythicCookingRecipe *> Ties = {TieB, TieA};
        TestEqual(TEXT("priority tie breaks lexicographically by RecipeId"),
                  UMythicCookingRecipe::PickBestExperimentCandidate(Ties), TieA);
        TArray<UMythicCookingRecipe *> TiesReversed = {TieA, TieB};
        TestEqual(TEXT("...and is order-independent"),
                  UMythicCookingRecipe::PickBestExperimentCandidate(TiesReversed), TieA);
    }

    FGameplayTagContainer Granted;
    const FGameplayTag Schematic = TAG_Itemization_Schematic_Cooking;
    TestTrue(TEXT("first learn grants"), UMythicUnlockComponent::ShouldGrantLearn(Granted, Schematic));
    Granted.AddTag(Schematic);
    TestFalse(TEXT("second learn is a no-op (idempotent)"), UMythicUnlockComponent::ShouldGrantLearn(Granted, Schematic));
    TestFalse(TEXT("invalid tag never grants"), UMythicUnlockComponent::ShouldGrantLearn(Granted, FGameplayTag()));

    FGameplayTagContainer NoTags;
    TestTrue(TEXT("hidden recipe with no gate stays visible (undiscoverable-hide is a data error)"), Hidden->IsVisibleTo(NoTags));
    Hidden->Requirements.InstigatorTagQuery = FGameplayTagQuery::MakeQuery_MatchAnyTags(FGameplayTagContainer(Schematic));
    TestFalse(TEXT("hidden + gated + unlearned ⇒ invisible"), Hidden->IsVisibleTo(NoTags));
    FGameplayTagContainer Learned(Schematic);
    TestTrue(TEXT("hidden + gated + learned ⇒ visible"), Hidden->IsVisibleTo(Learned));
    TestTrue(TEXT("plain recipes are always visible"), HigherPriority->IsVisibleTo(NoTags));

    UYieldQualityFragment *FineA = NewObject<UYieldQualityFragment>();
    FineA->QualityTier = EMythicYieldQuality::Fine;
    UYieldQualityFragment *FineB = NewObject<UYieldQualityFragment>();
    FineB->QualityTier = EMythicYieldQuality::Fine;
    UYieldQualityFragment *Pristine = NewObject<UYieldQualityFragment>();
    Pristine->QualityTier = EMythicYieldQuality::Pristine;
    TestTrue(TEXT("same tier stacks"), FineA->CanBeStackedWith(FineB));
    TestFalse(TEXT("different tiers never merge"), FineA->CanBeStackedWith(Pristine));
    TestFalse(TEXT("non-quality fragment never merges"), FineA->CanBeStackedWith(nullptr));
    TestEqual(TEXT("quality tag derives from the tier"), FineA->GetQualityTag(), TAG_Itemization_Quality_Fine.GetTag());

    return true;
}
