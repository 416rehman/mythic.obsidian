
#include "Misc/AutomationTest.h"
#include "World/Trading/MythicTradeLedger.h"

namespace {
    FMythicTradeLedgerEntry MakeEntry(int32 SettlementId, float FoodPrice, double SampledAt = 0.0) {
        FMythicTradeLedgerEntry E;
        E.SettlementId = SettlementId;
        E.GoverningFactionIndex = static_cast<uint8>(SettlementId % 4);
        E.Prices.Food = FoodPrice;
        E.SampledAtSeconds = SampledAt;
        return E;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTradingLedgerMathTest,
    "Mythic.Trading.Ledger.Math",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicTradingLedgerMathTest::RunTest(const FString &Parameters) {
    using namespace MythicTradeLedger;

    TestEqual(TEXT("Differential is To - From"), PriceDifferential(1.0f, 1.6f), 0.6f);
    TestEqual(TEXT("Inverted market is negative"), PriceDifferential(1.6f, 1.0f), -0.6f);

    TestEqual(TEXT("Profit = units × spread"), ExpectedProfit(10, 5.0f, 8.0f), 30.0f);
    TestEqual(TEXT("Loss is negative"), ExpectedProfit(10, 8.0f, 5.0f), -30.0f);
    TestEqual(TEXT("Zero units → zero"), ExpectedProfit(0, 1.0f, 100.0f), 0.0f);
    TestEqual(TEXT("Negative units → zero"), ExpectedProfit(-3, 1.0f, 100.0f), 0.0f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTradingLedgerStalenessTest,
    "Mythic.Trading.Ledger.Staleness",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicTradingLedgerStalenessTest::RunTest(const FString &Parameters) {
    using namespace MythicTradeLedger;

    TestEqual(TEXT("Fresh → 0"), ComputeStaleness(0.0, 600.0f), 0.0f);
    TestEqual(TEXT("Negative age → 0"), ComputeStaleness(-5.0, 600.0f), 0.0f);
    TestTrue(TEXT("One half-life → 0.5"), FMath::IsNearlyEqual(ComputeStaleness(600.0, 600.0f), 0.5f, 1e-4f));
    TestTrue(TEXT("Monotonic in age"), ComputeStaleness(1200.0, 600.0f) > ComputeStaleness(600.0, 600.0f));
    TestTrue(TEXT("Bounded below 1"), ComputeStaleness(1.0e9, 600.0f) <= 1.0f);
    TestTrue(TEXT("Degenerate half-life guarded"), ComputeStaleness(10.0, 0.0f) > 0.0f);

    TestEqual(TEXT("Confidence of fresh = 1"), ConfidenceFromStaleness(0.0f), 1.0f);
    TestEqual(TEXT("Confidence clamps"), ConfidenceFromStaleness(2.0f), 0.0f);

    TestEqual(TEXT("Fresh quote is exact"), QuantizeStalePrice(1.37f, 0.0f), 1.37f);
    const float Stale = QuantizeStalePrice(1.37f, 1.0f, 0.5f);
    TestTrue(TEXT("Stale quote within half a band"), FMath::Abs(Stale - 1.37f) <= 0.25f + KINDA_SMALL_NUMBER);
    TestTrue(TEXT("Stale quote non-negative"), QuantizeStalePrice(0.01f, 1.0f, 0.5f) >= 0.0f);
    TestTrue(TEXT("Ordering across bands preserved"),
             QuantizeStalePrice(0.2f, 1.0f, 0.5f) <= QuantizeStalePrice(1.8f, 1.0f, 0.5f));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTradingLedgerArbitrageTest,
    "Mythic.Trading.Ledger.Arbitrage",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicTradingLedgerArbitrageTest::RunTest(const FString &Parameters) {
    using namespace MythicTradeLedger;

    TArray<FMythicTradeLedgerEntry> Entries;
    Entries.Add(MakeEntry(1, 1.0f));
    Entries.Add(MakeEntry(2, 1.8f));
    Entries.Add(MakeEntry(3, 0.6f));

    int32 FromIdx = INDEX_NONE, ToIdx = INDEX_NONE;
    float Diff = 0.0f;
    TestTrue(TEXT("Arbitrage found"), FindBestArbitrage(Entries, EMythicResourceType::Food, FromIdx, ToIdx, Diff));
    TestEqual(TEXT("Buys at the cheapest market"), Entries[FromIdx].SettlementId, 3);
    TestEqual(TEXT("Sells at the dearest market"), Entries[ToIdx].SettlementId, 2);
    TestTrue(TEXT("Differential = dearest - cheapest"), FMath::IsNearlyEqual(Diff, 1.2f, 1e-4f));

    TArray<FMythicTradeLedgerEntry> Flat;
    Flat.Add(MakeEntry(1, 1.0f));
    Flat.Add(MakeEntry(2, 1.0f));
    TestFalse(TEXT("Flat market → none"), FindBestArbitrage(Flat, EMythicResourceType::Food, FromIdx, ToIdx, Diff));
    TArray<FMythicTradeLedgerEntry> Single;
    Single.Add(MakeEntry(1, 1.0f));
    TestFalse(TEXT("One market → none"), FindBestArbitrage(Single, EMythicResourceType::Food, FromIdx, ToIdx, Diff));

    TArray<FMythicTradeLedgerEntry> WithInvalid;
    WithInvalid.Add(MakeEntry(INDEX_NONE, 99.0f));
    WithInvalid.Add(MakeEntry(1, 1.0f));
    WithInvalid.Add(MakeEntry(2, 1.5f));
    TestTrue(TEXT("Invalid entries skipped"), FindBestArbitrage(WithInvalid, EMythicResourceType::Food, FromIdx, ToIdx, Diff));
    TestEqual(TEXT("Cheapest skips invalid"), WithInvalid[FromIdx].SettlementId, 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTradingLedgerFogOfWarTest,
    "Mythic.Trading.Ledger.FogOfWar",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicTradingLedgerFogOfWarTest::RunTest(const FString &Parameters) {
    using namespace MythicTradeLedger;
    TestTrue(TEXT("POI unlocked → live"), IsLedgerLiveForReader(true, false));
    TestTrue(TEXT("Neutral standing → live"), IsLedgerLiveForReader(false, true));
    TestTrue(TEXT("Both → live"), IsLedgerLiveForReader(true, true));
    TestFalse(TEXT("Neither → stale rumor"), IsLedgerLiveForReader(false, false));
    return true;
}
