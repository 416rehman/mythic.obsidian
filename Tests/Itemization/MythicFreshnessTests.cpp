
#include "Misc/AutomationTest.h"
#include "Itemization/Cooking/MythicFreshnessCore.h"
#include "Itemization/Inventory/Fragments/Passive/PerishableFragment.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicFreshnessCoreTest,
    "Mythic.Itemization.Freshness.Core",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicFreshnessCoreTest::RunTest(const FString &Parameters) {
    namespace MF = MythicFreshness;

    TestEqual(TEXT("0s → bucket 0"), MF::QuantizeToBucket(0.0, 300.0), (int64)0);
    TestEqual(TEXT("299s → bucket 0"), MF::QuantizeToBucket(299.0, 300.0), (int64)0);
    TestEqual(TEXT("300s → bucket 1"), MF::QuantizeToBucket(300.0, 300.0), (int64)1);
    TestEqual(TEXT("3599s → bucket 11"), MF::QuantizeToBucket(3599.0, 300.0), (int64)11);
    TestEqual(TEXT("non-positive width degrades to 1s buckets"), MF::QuantizeToBucket(42.0, 0.0), (int64)42);

    TestEqual(TEXT("merged stamp is the OLDEST"), MF::MergedStampBucket(5, 9), (int64)5);
    TestEqual(TEXT("merge is symmetric"), MF::MergedStampBucket(9, 5), (int64)5);

    TestEqual(TEXT("normal aging: banked + elapsed"), MF::AccrueAgedSeconds(100.0, 1000.0, 1600.0, 1.0f), 700.0);
    TestEqual(TEXT("cold storage 0.5x ages at half speed"), MF::AccrueAgedSeconds(100.0, 1000.0, 1600.0, 0.5f), 400.0);
    TestEqual(TEXT("frozen (0x) banks nothing new"), MF::AccrueAgedSeconds(100.0, 1000.0, 1600.0, 0.0f), 100.0);
    TestEqual(TEXT("time never runs backwards"), MF::AccrueAgedSeconds(100.0, 2000.0, 1600.0, 1.0f), 100.0);
    TestEqual(TEXT("negative multiplier clamps to frozen"), MF::AccrueAgedSeconds(100.0, 1000.0, 1600.0, -2.0f), 100.0);

    TestEqual(TEXT("shelf-life 0 NEVER spoils (inert default)"), MF::FreshnessFraction(1.0e9, 0.0), 1.0f);
    TestEqual(TEXT("unaged is fully fresh"), MF::FreshnessFraction(0.0, 1000.0), 1.0f);
    TestEqual(TEXT("half-aged is half fresh"), MF::FreshnessFraction(500.0, 1000.0), 0.5f);
    TestEqual(TEXT("fully aged is 0"), MF::FreshnessFraction(1000.0, 1000.0), 0.0f);
    TestEqual(TEXT("over-aged clamps at 0"), MF::FreshnessFraction(5000.0, 1000.0), 0.0f);

    TestTrue(TEXT("full fraction is Fresh"), MF::ClassifyWindow(1.0f, 0.5f) == MF::EWindow::Fresh);
    TestTrue(TEXT("at the threshold is Fresh (strict below = Stale)"), MF::ClassifyWindow(0.5f, 0.5f) == MF::EWindow::Fresh);
    TestTrue(TEXT("below the threshold is Stale"), MF::ClassifyWindow(0.49f, 0.5f) == MF::EWindow::Stale);
    TestTrue(TEXT("0 fraction is Spoiled"), MF::ClassifyWindow(0.0f, 0.5f) == MF::EWindow::Spoiled);

    TArray<uint8> Payload;
    MF::SerializeState(Payload, 123.5, 987654.0, 0.25f);
    TestTrue(TEXT("payload has a version byte"), Payload.Num() > 0 && Payload[0] == MF::PerishablePayloadVersion);
    double Banked = -1.0, Anchor = -1.0;
    float Mult = -1.0f;
    TestTrue(TEXT("round-trip deserializes"), MF::DeserializeState(Payload, Banked, Anchor, Mult));
    TestEqual(TEXT("banked round-trips"), Banked, 123.5);
    TestEqual(TEXT("anchor round-trips"), Anchor, 987654.0);
    TestEqual(TEXT("mult round-trips"), Mult, 0.25f);

    Banked = 11.0;
    Anchor = 22.0;
    Mult = 0.5f;
    TestFalse(TEXT("EMPTY payload (v0) falls back to defaults"), MF::DeserializeState(TArray<uint8>(), Banked, Anchor, Mult));
    TestEqual(TEXT("v0: outputs untouched"), Banked, 11.0);

    TArray<uint8> Future = Payload;
    Future[0] = 99;
    TestFalse(TEXT("UNKNOWN version falls back to defaults (never misreads)"), MF::DeserializeState(Future, Banked, Anchor, Mult));
    TestEqual(TEXT("unknown version: outputs untouched"), Banked, 11.0);

    const double Bucket = 300.0, Now = 1000000.0;
    TestTrue(TEXT("same window stacks"),
             MF::CanStackPerishables(0.0, Now - 10.0, 1.0f, 0.0, Now - 250.0, 1.0f, Bucket, Now));
    TestFalse(TEXT("a FRESHER item never merges into an older stack (no refresh)"),
              MF::CanStackPerishables(0.0, Now - 10.0, 1.0f, 0.0, Now - 4000.0, 1.0f, Bucket, Now));
    TestFalse(TEXT("banked age separates windows too"),
              MF::CanStackPerishables(0.0, Now, 1.0f, 10000.0, Now, 1.0f, Bucket, Now));
    TestFalse(TEXT("different aging rates never merge (cold vs warm)"),
              MF::CanStackPerishables(0.0, Now, 1.0f, 0.0, Now, 0.25f, Bucket, Now));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPerishableFragmentStackingTest,
    "Mythic.Itemization.Freshness.FragmentStacking",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPerishableFragmentStackingTest::RunTest(const FString &Parameters) {
    const double Now = UPerishableFragment::UtcNowSeconds();

    auto MakeFrag = [&](double AgeSeconds, float Mult, double ShelfLife) {
        UPerishableFragment *Frag = NewObject<UPerishableFragment>();
        Frag->PerishableConfig.ShelfLifeSeconds = ShelfLife;
        Frag->PerishableConfig.BucketSeconds = 300.0;
        Frag->PerishableRuntimeReplicatedData.AgedBankedSeconds = 0.0;
        Frag->PerishableRuntimeReplicatedData.AnchorUtcSeconds = Now - AgeSeconds;
        Frag->PerishableRuntimeReplicatedData.CurrentPreservationMult = Mult;
        return Frag;
    };

    UPerishableFragment *FreshA = MakeFrag(10.0, 1.0f, 3600.0);
    UPerishableFragment *FreshB = MakeFrag(200.0, 1.0f, 3600.0);
    UPerishableFragment *Old = MakeFrag(4000.0, 1.0f, 3600.0);
    UPerishableFragment *Cold = MakeFrag(10.0, 0.25f, 3600.0);
    UPerishableFragment *OtherCfg = MakeFrag(10.0, 1.0f, 7200.0);

    TestTrue(TEXT("same-window perishables stack"), FreshA->CanBeStackedWith(FreshB));
    TestFalse(TEXT("fresh never merges with old (no refresh exploit)"), FreshA->CanBeStackedWith(Old));
    TestFalse(TEXT("old never merges with fresh (symmetric)"), Old->CanBeStackedWith(FreshA));
    TestFalse(TEXT("cold-stored clock never merges into a warm stack"), FreshA->CanBeStackedWith(Cold));
    TestFalse(TEXT("different perishable configs never merge"), FreshA->CanBeStackedWith(OtherCfg));
    TestFalse(TEXT("non-perishable fragment never merges"), FreshA->CanBeStackedWith(nullptr));

    TestEqual(TEXT("effective age = anchor delta"), FreshA->GetEffectiveAgedSeconds(Now), 10.0);
    TestEqual(TEXT("cold effective age scales by the preservation multiplier"), Cold->GetEffectiveAgedSeconds(Now), 2.5);
    TestTrue(TEXT("old item lost freshness"), Old->GetFreshnessFraction(Now) < 1.0f);
    UPerishableFragment *Immortal = MakeFrag(1.0e8, 1.0f, 0.0);
    TestEqual(TEXT("shelf-life 0 pins fraction at 1 (never spoils)"), Immortal->GetFreshnessFraction(Now), 1.0f);
    TestTrue(TEXT("spoiled past shelf life"), MakeFrag(4000.0, 1.0f, 3600.0)->IsSpoiled(Now));

    return true;
}
