
#include "Misc/AutomationTest.h"
#include "World/LivingWorld/MythicPlayerEconomyDelta.h"

namespace {
    FMythicPendingResourceDelta MakeDelta(uint8 FactionIndex, EMythicResourceType Axis, float Delta) {
        FMythicPendingResourceDelta Row;
        Row.FactionId.Index = FactionIndex;
        Row.Axis = Axis;
        Row.Delta = Delta;
        return Row;
    }

    struct FApplied {
        TMap<uint32, float> ByKey;
        float Total = 0.0f;
        void Add(const FMythicFactionId &Id, EMythicResourceType Axis, float Delta) {
            ByKey.FindOrAdd(MythicPlayerEconomyDelta::MakeKey(Id, Axis)) += Delta;
            Total += Delta;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTradingReserveInjectionClampTest,
    "Mythic.Trading.ReserveInjection.PerTickClamp",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicTradingReserveInjectionClampTest::RunTest(const FString &Parameters) {
    using namespace MythicPlayerEconomyDelta;
    const float Ceiling = 5.0f;

    {
        TArray<FMythicPendingResourceDelta> Pending;
        for (int32 i = 0; i < 10; ++i) {
            Pending.Add(MakeDelta(2, EMythicResourceType::Food, 3.0f));
        }
        FApplied Applied;
        const TArray<FMythicPendingResourceDelta> Carry = DrainClamped(
            Pending, Ceiling, [&](const FMythicFactionId &Id, EMythicResourceType Axis, float D) { Applied.Add(Id, Axis, D); });
        TestTrue(TEXT("One drain applies exactly the ceiling"), FMath::IsNearlyEqual(Applied.Total, Ceiling, 1e-4f));
        TestEqual(TEXT("The remainder carries as one coalesced row"), Carry.Num(), 1);
        TestTrue(TEXT("Conservation: applied + carry == enqueued"),
                 FMath::IsNearlyEqual(Applied.Total + Carry[0].Delta, 30.0f, 1e-3f));
    }

    {
        TArray<FMythicPendingResourceDelta> Pending;
        Pending.Add(MakeDelta(2, EMythicResourceType::Food, 23.0f));
        float TotalApplied = 0.0f;
        int32 Drains = 0;
        while (Pending.Num() > 0 && Drains++ < 100) {
            FApplied Applied;
            Pending = DrainClamped(Pending, Ceiling,
                                   [&](const FMythicFactionId &Id, EMythicResourceType Axis, float D) { Applied.Add(Id, Axis, D); });
            TestTrue(TEXT("No drain ever exceeds the ceiling"), Applied.Total <= Ceiling + 1e-4f);
            TotalApplied += Applied.Total;
        }
        TestEqual(TEXT("23 at ceiling 5 takes 5 drains"), Drains, 5);
        TestTrue(TEXT("Everything eventually lands"), FMath::IsNearlyEqual(TotalApplied, 23.0f, 1e-3f));
    }

    {
        TArray<FMythicPendingResourceDelta> Pending;
        Pending.Add(MakeDelta(1, EMythicResourceType::Food, 20.0f));
        Pending.Add(MakeDelta(1, EMythicResourceType::Arms, 20.0f));
        Pending.Add(MakeDelta(3, EMythicResourceType::Food, 2.0f));
        FApplied Applied;
        const TArray<FMythicPendingResourceDelta> Carry = DrainClamped(
            Pending, Ceiling, [&](const FMythicFactionId &Id, EMythicResourceType Axis, float D) { Applied.Add(Id, Axis, D); });
        FMythicFactionId F1, F3;
        F1.Index = 1;
        F3.Index = 3;
        TestTrue(TEXT("Faction 1 Food clamped"), FMath::IsNearlyEqual(Applied.ByKey[MakeKey(F1, EMythicResourceType::Food)], 5.0f, 1e-4f));
        TestTrue(TEXT("Faction 1 Arms clamped independently"),
                 FMath::IsNearlyEqual(Applied.ByKey[MakeKey(F1, EMythicResourceType::Arms)], 5.0f, 1e-4f));
        TestTrue(TEXT("A small delta applies whole"),
                 FMath::IsNearlyEqual(Applied.ByKey[MakeKey(F3, EMythicResourceType::Food)], 2.0f, 1e-4f));
        TestEqual(TEXT("Two over-ceiling keys carry"), Carry.Num(), 2);
    }

    {
        TArray<FMythicPendingResourceDelta> Pending;
        Pending.Add(MakeDelta(1, EMythicResourceType::Wealth, -12.0f));
        FApplied Applied;
        const TArray<FMythicPendingResourceDelta> Carry = DrainClamped(
            Pending, Ceiling, [&](const FMythicFactionId &Id, EMythicResourceType Axis, float D) { Applied.Add(Id, Axis, D); });
        TestTrue(TEXT("Negative applies at -ceiling"), FMath::IsNearlyEqual(Applied.Total, -5.0f, 1e-4f));
        TestEqual(TEXT("Negative remainder carries"), Carry.Num(), 1);
        TestTrue(TEXT("Negative conservation"), FMath::IsNearlyEqual(Applied.Total + Carry[0].Delta, -12.0f, 1e-3f));
    }

    {
        TArray<FMythicPendingResourceDelta> Pending;
        Pending.Add(MakeDelta(1, EMythicResourceType::Food, 4.0f));
        Pending.Add(MakeDelta(1, EMythicResourceType::Food, -3.0f));
        FApplied Applied;
        const TArray<FMythicPendingResourceDelta> Carry = DrainClamped(
            Pending, Ceiling, [&](const FMythicFactionId &Id, EMythicResourceType Axis, float D) { Applied.Add(Id, Axis, D); });
        TestTrue(TEXT("Opposite signs net out before the clamp"), FMath::IsNearlyEqual(Applied.Total, 1.0f, 1e-4f));
        TestEqual(TEXT("Netted delta carries nothing"), Carry.Num(), 0);
    }

    {
        TArray<FMythicPendingResourceDelta> Pending;
        Pending.Add(MakeDelta(1, EMythicResourceType::Food, 10.0f));
        FApplied Applied;
        const TArray<FMythicPendingResourceDelta> Carry = DrainClamped(
            Pending, 0.0f, [&](const FMythicFactionId &Id, EMythicResourceType Axis, float D) { Applied.Add(Id, Axis, D); });
        TestEqual(TEXT("Zero ceiling applies nothing"), Applied.Total, 0.0f);
        TestEqual(TEXT("Everything defers"), Carry.Num(), 1);
        TestTrue(TEXT("Deferred delta intact"), FMath::IsNearlyEqual(Carry[0].Delta, 10.0f, 1e-4f));
    }

    {
        TArray<FMythicPendingResourceDelta> Pending;
        Pending.Add(MakeDelta(FMythicFactionId::InvalidIndex, EMythicResourceType::Food, 10.0f));
        FApplied Applied;
        const TArray<FMythicPendingResourceDelta> Carry = DrainClamped(
            Pending, Ceiling, [&](const FMythicFactionId &Id, EMythicResourceType Axis, float D) { Applied.Add(Id, Axis, D); });
        TestEqual(TEXT("Invalid faction applies nothing"), Applied.Total, 0.0f);
        TestEqual(TEXT("Invalid faction carries nothing"), Carry.Num(), 0);
    }

    return true;
}
