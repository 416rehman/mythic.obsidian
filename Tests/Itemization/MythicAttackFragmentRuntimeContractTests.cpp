#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include <limits>

#include "Animation/AnimMontage.h"
#include "GAS/Abilities/MythicAnimNotify_SphereOverlap.h"
#include "GAS/MythicTags_GAS.h"
#include "Itemization/Inventory/Fragments/Actionable/AttackFragment.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAttackFragmentMontageRateScaleContractTest,
    "Mythic.Itemization.AttackFragment.MontageRateScaleContract",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicAttackFragmentMontageRateScaleContractTest::RunTest(
    const FString &Parameters) {
    TestFalse(TEXT("a null montage cannot own canonical attack cadence"),
              UAttackFragment::HasCanonicalMontageRateScale(nullptr));

    UAnimMontage *Montage = NewObject<UAnimMontage>(GetTransientPackage());
    if (!TestNotNull(TEXT("a transient montage can be constructed"), Montage)) {
        return false;
    }

    Montage->RateScale = 1.0f;
    TestTrue(TEXT("unit Rate Scale leaves cadence exclusively with GAS AttackSpeed"),
             UAttackFragment::HasCanonicalMontageRateScale(Montage));

    Montage->RateScale = 0.0f;
    TestFalse(TEXT("zero Rate Scale cannot stall a canonical attack"),
              UAttackFragment::HasCanonicalMontageRateScale(Montage));

    Montage->RateScale = 0.5f;
    TestFalse(TEXT("a finite non-unit Rate Scale cannot silently alter DPS cadence"),
              UAttackFragment::HasCanonicalMontageRateScale(Montage));

    Montage->RateScale = -1.0f;
    TestFalse(TEXT("a reverse non-unit Rate Scale is rejected"),
              UAttackFragment::HasCanonicalMontageRateScale(Montage));

    Montage->RateScale = std::numeric_limits<float>::quiet_NaN();
    TestFalse(TEXT("NaN Rate Scale is rejected"),
              UAttackFragment::HasCanonicalMontageRateScale(Montage));

    Montage->RateScale = std::numeric_limits<float>::infinity();
    TestFalse(TEXT("infinite Rate Scale is rejected"),
              UAttackFragment::HasCanonicalMontageRateScale(Montage));

    UAttackFragment *Fragment = NewObject<UAttackFragment>(GetTransientPackage());
    Fragment->AttackConfig.AttackMontage = Montage;
    FText RuntimeError;
    TestFalse(TEXT("the runtime cache rejects a corrupt live Rate Scale"),
              Fragment->ResolveRuntimeAttackContract(&RuntimeError));
    TestTrue(TEXT("runtime rejection identifies the cadence owner violation"),
             RuntimeError.ToString().Contains(TEXT("Rate Scale")));

    Montage->RateScale = 1.0f;
    RuntimeError = FText::GetEmpty();
    TestFalse(TEXT("the otherwise empty montage remains invalid after restoring unit scale"),
              Fragment->ResolveRuntimeAttackContract(&RuntimeError));
    TestFalse(TEXT("restoring unit scale invalidates the stale Rate Scale cache entry"),
              RuntimeError.ToString().Contains(TEXT("Rate Scale")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAttackFragmentHitSampleContractTest,
    "Mythic.Itemization.AttackFragment.HitSampleContract",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicAttackFragmentHitSampleContractTest::RunTest(
    const FString &Parameters) {
    UMythicAnimNotify_SphereOverlap *Sample =
        NewObject<UMythicAnimNotify_SphereOverlap>(GetTransientPackage());
    Sample->SendToEventWithTag = GAS_EVENT_HITBOX;
    Sample->HitboxRadius = 100.0f;
    Sample->HitboxLocationOffset = FVector(25.0, 0.0, 50.0);
    Sample->MaxTargets = 0;

    TestTrue(TEXT("a finite positive canonical sample is usable"),
             UAttackFragment::IsCanonicalHitSampleUsable(Sample));

    Sample->HitboxRadius = 0.0f;
    TestFalse(TEXT("a disabled zero-radius sample is rejected"),
              UAttackFragment::IsCanonicalHitSampleUsable(Sample));

    Sample->HitboxRadius = std::numeric_limits<float>::infinity();
    TestFalse(TEXT("an infinite-radius sample is rejected"),
              UAttackFragment::IsCanonicalHitSampleUsable(Sample));

    Sample->HitboxRadius = 100.0f;
    Sample->HitboxLocationOffset.X = std::numeric_limits<double>::quiet_NaN();
    TestFalse(TEXT("a non-finite local offset is rejected"),
              UAttackFragment::IsCanonicalHitSampleUsable(Sample));

    Sample->HitboxLocationOffset = FVector::ZeroVector;
    Sample->MaxTargets = -1;
    TestFalse(TEXT("a negative target cap is rejected"),
              UAttackFragment::IsCanonicalHitSampleUsable(Sample));

    Sample->MaxTargets = 0;
    Sample->SendToEventWithTag = FGameplayTag();
    TestFalse(TEXT("a non-canonical event tag is rejected"),
              UAttackFragment::IsCanonicalHitSampleUsable(Sample));
    return true;
}

#endif
