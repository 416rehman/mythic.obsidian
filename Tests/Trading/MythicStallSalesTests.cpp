
#include "Misc/AutomationTest.h"
#include "World/Trading/MythicStallSales.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTradingStallSaleChanceTest,
    "Mythic.Trading.Stall.SaleChance",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicTradingStallSaleChanceTest::RunTest(const FString &Parameters) {
    using namespace MythicStallSales;
    const float Base = 0.25f;
    const float Ceiling = 2.0f;

    TestTrue(TEXT("At fair price → base chance"),
             FMath::IsNearlyEqual(ComputeSaleChance(100.0f, 100.0f, Base, Ceiling), Base, 1e-4f));
    TestEqual(TEXT("At the ceiling ratio → 0"), ComputeSaleChance(200.0f, 100.0f, Base, Ceiling), 0.0f);
    TestEqual(TEXT("Above the ceiling → 0"), ComputeSaleChance(300.0f, 100.0f, Base, Ceiling), 0.0f);

    TestTrue(TEXT("Half price → 1.5× base"),
             FMath::IsNearlyEqual(ComputeSaleChance(50.0f, 100.0f, Base, Ceiling), 1.5f * Base, 1e-4f));
    TestTrue(TEXT("Deep undercut caps at 2× base"),
             ComputeSaleChance(1.0f, 100.0f, Base, Ceiling) <= 2.0f * Base + KINDA_SMALL_NUMBER);

    TestTrue(TEXT("Monotonic in listed price"),
             ComputeSaleChance(80.0f, 100.0f, Base, Ceiling) >= ComputeSaleChance(120.0f, 100.0f, Base, Ceiling));

    TestEqual(TEXT("Fair <= 0 → 0"), ComputeSaleChance(50.0f, 0.0f, Base, Ceiling), 0.0f);
    TestEqual(TEXT("Listed <= 0 → 0"), ComputeSaleChance(0.0f, 100.0f, Base, Ceiling), 0.0f);
    TestEqual(TEXT("Base 0 → 0"), ComputeSaleChance(100.0f, 100.0f, 0.0f, Ceiling), 0.0f);

    TestEqual(TEXT("Sample under chance sells 1"), RollUnitsSold(5, 0.5f, 0.4f), 1);
    TestEqual(TEXT("Sample at/over chance sells 0"), RollUnitsSold(5, 0.5f, 0.5f), 0);
    TestEqual(TEXT("Empty stack sells 0"), RollUnitsSold(0, 1.0f, 0.0f), 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTradingStallAccrualTest,
    "Mythic.Trading.Stall.AwayAccrual",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicTradingStallAccrualTest::RunTest(const FString &Parameters) {
    using namespace MythicStallSales;

    TestEqual(TEXT("Two intervals → two passes"), ComputeAccruedDrains(600.0, 300.0f, 48), 2);
    TestEqual(TEXT("Partial interval floors"), ComputeAccruedDrains(599.0, 300.0f, 48), 1);
    TestEqual(TEXT("Under one interval → 0"), ComputeAccruedDrains(299.0, 300.0f, 48), 0);
    TestEqual(TEXT("A week away caps at MaxAccrued"), ComputeAccruedDrains(604800.0, 300.0f, 48), 48);
    TestEqual(TEXT("Zero elapsed → 0"), ComputeAccruedDrains(0.0, 300.0f, 48), 0);
    TestEqual(TEXT("Negative elapsed (clock skew) → 0"), ComputeAccruedDrains(-100.0, 300.0f, 48), 0);
    TestEqual(TEXT("Zero interval guarded"), ComputeAccruedDrains(600.0, 0.0f, 48), 0);
    TestEqual(TEXT("Zero cap → 0"), ComputeAccruedDrains(600.0, 300.0f, 0), 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTradingStallPayloadTest,
    "Mythic.Trading.Stall.VersionedPayload",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicTradingStallPayloadTest::RunTest(const FString &Parameters) {
    using namespace MythicStallSales;

    TArray<uint8> Base;
    Base.Add(11);
    Base.Add(22);
    Base.Add(33);
    TArray<uint8> Payload;
    SerializeStallState(Payload, 1234, 1720000000LL, 1.25f, TEXT("player-abc"), Base);

    int32 Till = -1;
    int64 Anchor = -1;
    float Mult = -1.0f;
    FString OwnerKey;
    TArray<uint8> BaseOut;
    TestTrue(TEXT("v1 payload parses"), DeserializeStallState(Payload, Till, Anchor, Mult, OwnerKey, BaseOut));
    TestEqual(TEXT("Till round-trips"), Till, 1234);
    TestEqual(TEXT("Anchor round-trips"), Anchor, static_cast<int64>(1720000000LL));
    TestTrue(TEXT("Multiplier round-trips"), FMath::IsNearlyEqual(Mult, 1.25f, 1e-4f));
    TestEqual(TEXT("Owner key round-trips"), OwnerKey, FString(TEXT("player-abc")));
    TestEqual(TEXT("Base blob length round-trips"), BaseOut.Num(), 3);
    TestTrue(TEXT("Base blob bytes round-trip"), BaseOut.Num() == 3 && BaseOut[0] == 11 && BaseOut[1] == 22 && BaseOut[2] == 33);

    TArray<uint8> Dirty;
    SerializeStallState(Dirty, -50, 0, -2.0f, FString(), TArray<uint8>());
    TestTrue(TEXT("Dirty payload parses"), DeserializeStallState(Dirty, Till, Anchor, Mult, OwnerKey, BaseOut));
    TestEqual(TEXT("Negative till clamps to 0"), Till, 0);
    TestTrue(TEXT("Negative multiplier clamps to 0"), Mult >= 0.0f);
    TestEqual(TEXT("Empty base stays empty"), BaseOut.Num(), 0);

    TestFalse(TEXT("Empty payload → defaults"), DeserializeStallState(TArray<uint8>(), Till, Anchor, Mult, OwnerKey, BaseOut));
    TestEqual(TEXT("Defaults: till 0"), Till, 0);
    TestTrue(TEXT("Defaults: multiplier 1"), FMath::IsNearlyEqual(Mult, 1.0f, 1e-4f));

    TArray<uint8> Future = Payload;
    Future[0] = 99;
    TestFalse(TEXT("Unknown version → defaults"), DeserializeStallState(Future, Till, Anchor, Mult, OwnerKey, BaseOut));

    TArray<uint8> Truncated = Payload;
    Truncated.SetNum(6);
    TestFalse(TEXT("Truncated payload → defaults"), DeserializeStallState(Truncated, Till, Anchor, Mult, OwnerKey, BaseOut));

    return true;
}
