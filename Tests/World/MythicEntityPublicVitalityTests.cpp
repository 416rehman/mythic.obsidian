#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameplayEffect.h"
#include "World/Entity/MythicEntityPresentationComponent.h"

struct FMythicEntityPresentationComponentTestAccess {
    static bool GrantsProjectedStatus(
        const FGameplayEffectSpec &EffectSpec,
        const FGameplayTagContainer &ProjectedStateTags) {
        return UMythicEntityPresentationComponent::
            GameplayEffectGrantsProjectedStatus(
                EffectSpec, ProjectedStateTags);
    }
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicEntityPublicVitalityQuantizationTest,
    "Mythic.World.Entity.Presentation.PublicVitality",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicEntityPublicVitalityQuantizationTest::RunTest(
    const FString &Parameters) {
    TestEqual(TEXT("empty health clamps to zero"),
              UMythicEntityPresentationComponent::QuantizePublicHealthFraction(
                  -1.0f),
              static_cast<uint8>(0));
    TestEqual(TEXT("full health maps to the transport maximum"),
              UMythicEntityPresentationComponent::QuantizePublicHealthFraction(
                  1.0f),
              static_cast<uint8>(255));
    TestEqual(TEXT("overheal clamps without exposing the raw value"),
              UMythicEntityPresentationComponent::QuantizePublicHealthFraction(
                  3.0f),
              static_cast<uint8>(255));

    const uint8 QuantizedHalf =
        UMythicEntityPresentationComponent::QuantizePublicHealthFraction(0.5f);
    TestTrue(TEXT("round-trip error stays below one transport step"),
             FMath::Abs(
                 UMythicEntityPresentationComponent::
                     DequantizePublicHealthFraction(QuantizedHalf)
                 - 0.5f)
                 <= (1.0f / 255.0f));

    FMythicPublicVitalitySnapshot Snapshot;
    TestFalse(TEXT("an unbound snapshot never presents vitality"),
              Snapshot.IsCurrentFor(FMythicEntityPresentationInstance()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicEntityProjectedStatusEffectClassificationTest,
    "Mythic.World.Entity.Presentation.ProjectedStatusEffectClassification",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicEntityProjectedStatusEffectClassificationTest::RunTest(
    const FString &Parameters) {
    const FGameplayTag Burning = FGameplayTag::RequestGameplayTag(
        FName(TEXT("GAS.Debuff.Burning")), false);
    const FGameplayTag Bleeding = FGameplayTag::RequestGameplayTag(
        FName(TEXT("GAS.Debuff.Bleeding")), false);
    if (!TestTrue(TEXT("canonical projected state tags are registered"),
                  Burning.IsValid() && Bleeding.IsValid())) {
        return false;
    }

    FGameplayTagContainer ProjectedStateTags(Burning);
    FGameplayEffectSpec BurnSpec;
    BurnSpec.DynamicGrantedTags.AddTag(Burning);
    TestTrue(
        TEXT("a GameplayEffect granting a projected status is tracked"),
        FMythicEntityPresentationComponentTestAccess::GrantsProjectedStatus(
            BurnSpec, ProjectedStateTags));

    FGameplayEffectSpec BleedSpec;
    BleedSpec.DynamicGrantedTags.AddTag(Bleeding);
    TestFalse(
        TEXT("an unrelated GameplayEffect does not receive three change delegates"),
        FMythicEntityPresentationComponentTestAccess::GrantsProjectedStatus(
            BleedSpec, ProjectedStateTags));

    TestFalse(
        TEXT("an empty public-status registry fails closed"),
        FMythicEntityPresentationComponentTestAccess::GrantsProjectedStatus(
            BurnSpec, FGameplayTagContainer()));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
