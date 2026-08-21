
#include "Misc/AutomationTest.h"
#include "World/Fishing/MythicFishStockRules.h"
#include "World/Fishing/MythicFishingTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicFishStockTest,
    "Mythic.Fishing.Stocks",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicFishStockTest::RunTest(const FString &Parameters) {
    using Rules = FMythicFishStockRules;
    constexpr float RegenPerUnit = 100.0f;
    constexpr int32 MaxStock = 5;

    {
        int32 Stock = 2;
        double Anchor = 1000.0;

        TestEqual(TEXT("no gap → no regen"), Rules::Resolve(Stock, Anchor, 1000.0, RegenPerUnit, MaxStock), 2);

        Stock = 2;
        Anchor = 1000.0;
        TestEqual(TEXT("250s → +2 units"), Rules::Resolve(Stock, Anchor, 1250.0, RegenPerUnit, MaxStock), 4);
        TestEqual(TEXT("anchor advanced by whole units only"), Anchor, 1200.0);
        TestEqual(TEXT("remainder carries into the next unit"), Rules::Resolve(Stock, Anchor, 1300.0, RegenPerUnit, MaxStock), 5);

        Stock = 4;
        Anchor = 0.0;
        TestEqual(TEXT("regen clamps at max"), Rules::Resolve(Stock, Anchor, 100000.0, RegenPerUnit, MaxStock), MaxStock);
        TestEqual(TEXT("full stock parks the anchor at now"), Anchor, 100000.0);

        Stock = 2;
        Anchor = 1000.0;
        TestEqual(TEXT("negative gap → no regen"), Rules::Resolve(Stock, Anchor, 500.0, RegenPerUnit, MaxStock), 2);
    }

    {
        int32 Stock = 2;
        double Anchor = 0.0;
        bool bExhausted = false;

        TestTrue(TEXT("consume from 2 → ok"), Rules::ConsumeOne(Stock, Anchor, 0.0, RegenPerUnit, MaxStock, bExhausted));
        TestEqual(TEXT("stock now 1"), Stock, 1);
        TestFalse(TEXT("not exhausted yet"), bExhausted);

        TestTrue(TEXT("consume from 1 → ok"), Rules::ConsumeOne(Stock, Anchor, 0.0, RegenPerUnit, MaxStock, bExhausted));
        TestEqual(TEXT("stock now 0"), Stock, 0);
        TestTrue(TEXT("THIS consume exhausted the spot (edge)"), bExhausted);

        TestFalse(TEXT("consume from 0 → refused"), Rules::ConsumeOne(Stock, Anchor, 0.0, RegenPerUnit, MaxStock, bExhausted));
        TestFalse(TEXT("no repeat exhaustion edge"), bExhausted);

        Stock = MaxStock;
        Anchor = 0.0;
        Rules::ConsumeOne(Stock, Anchor, 5000.0, RegenPerUnit, MaxStock, bExhausted);
        TestEqual(TEXT("leaving full re-anchors the clock"), Anchor, 5000.0);
        TestEqual(TEXT("full-1"), Stock, MaxStock - 1);

        TestEqual(TEXT("one regen period later it's back"), Rules::Resolve(Stock, Anchor, 5100.0, RegenPerUnit, MaxStock), MaxStock);
    }

    {
        FMythicCatchTableEntry Fish;
        FMythicCatchTableEntry Boot;
        Boot.bTrashTier = true;

        FMythicCatchContext Healthy;
        TestTrue(TEXT("healthy spot: fish eligible"), MythicFishing::IsEntryEligible(Fish, Healthy));
        TestTrue(TEXT("healthy spot: trash eligible too"), MythicFishing::IsEntryEligible(Boot, Healthy));

        FMythicCatchContext Exhausted;
        Exhausted.bTrashOnly = true;
        TestFalse(TEXT("exhausted spot: fish INELIGIBLE (degraded table)"), MythicFishing::IsEntryEligible(Fish, Exhausted));
        TestTrue(TEXT("exhausted spot: trash still bites"), MythicFishing::IsEntryEligible(Boot, Exhausted));

        FMythicCatchTableEntry NightFish;
        NightFish.RequiredConditions.AddTag(FGameplayTag::RequestGameplayTag(FName("Environment.Time.Night"), false));
        FMythicCatchContext Ctx;
        if (NightFish.RequiredConditions.IsValid() && NightFish.RequiredConditions.Num() > 0 && NightFish.RequiredConditions.First().IsValid()) {
            TestFalse(TEXT("condition-gated entry refuses without the tag"), MythicFishing::IsEntryEligible(NightFish, Ctx));
            Ctx.Conditions = NightFish.RequiredConditions;
            TestTrue(TEXT("condition-gated entry passes with the tag current"), MythicFishing::IsEntryEligible(NightFish, Ctx));
        }
        TestTrue(TEXT("legacy entry (empty conditions) always passes"), MythicFishing::IsEntryEligible(Fish, Ctx));
    }

    {
        TArray<uint8> Bytes;
        Rules::EncodeStockSave(Bytes, 3, 42.5f);
        TestTrue(TEXT("payload carries the version byte"), Bytes.Num() > 0 && Bytes[0] == Rules::SaveVersion);

        int32 Units = -1;
        float Toward = -1.0f;
        TestTrue(TEXT("decode succeeds"), Rules::DecodeStockSave(Bytes, Units, Toward));
        TestEqual(TEXT("units round-trip"), Units, 3);
        TestEqual(TEXT("remainder seconds round-trip"), Toward, 42.5f);

        TestFalse(TEXT("empty payload → defaults"), Rules::DecodeStockSave(TArray<uint8>(), Units, Toward));

        TArray<uint8> Garbage;
        Garbage.Add(0);
        TestFalse(TEXT("version-0 payload → defaults"), Rules::DecodeStockSave(Garbage, Units, Toward));

        TArray<uint8> Truncated;
        Truncated.Add(Rules::SaveVersion);
        Truncated.Add(7);
        TestFalse(TEXT("truncated payload → defaults"), Rules::DecodeStockSave(Truncated, Units, Toward));

        TArray<uint8> Negative;
        Rules::EncodeStockSave(Negative, -5, -10.0f);
        TestTrue(TEXT("negative payload decodes"), Rules::DecodeStockSave(Negative, Units, Toward));
        TestEqual(TEXT("negative units clamp to 0"), Units, 0);
        TestEqual(TEXT("negative remainder clamps to 0"), Toward, 0.0f);
    }

    return true;
}
