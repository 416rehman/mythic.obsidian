
#include "Misc/AutomationTest.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "GameFramework/Pawn.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicKillCreditTest,
    "Mythic.Combat.KillCredit",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicKillCreditTest::RunTest(const FString &Parameters) {
    auto Eligible = [&](bool bIsKiller, float DistSq, float RangeSq) {
        return UMythicLifeComponent::IsEligibleForSharedKillCredit(bIsKiller, DistSq, RangeSq);
    };

    TestTrue(TEXT("killer credited within range"), Eligible(true, 100.0f, 10000.0f));
    TestTrue(TEXT("killer credited out of range"), Eligible(true, 9999999.0f, 10000.0f));
    TestTrue(TEXT("killer credited with sharing off (RangeSq=0)"), Eligible(true, 0.0f, 0.0f));

    TestTrue(TEXT("ally within range shares"), Eligible(false, 100.0f, 10000.0f));
    TestTrue(TEXT("ally exactly at the range boundary shares (inclusive)"), Eligible(false, 10000.0f, 10000.0f));
    TestFalse(TEXT("ally beyond range does not share"), Eligible(false, 10001.0f, 10000.0f));

    TestFalse(TEXT("sharing off: ally at the victim (DistSq=0) still does NOT leech"), Eligible(false, 0.0f, 0.0f));
    TestFalse(TEXT("sharing off: distant ally does not share"), Eligible(false, 500.0f, 0.0f));

    AActor *Victim = NewObject<AActor>(GetTransientPackage());
    AActor *Other = NewObject<AActor>(GetTransientPackage());
    APawn *VictimPawn = NewObject<APawn>(GetTransientPackage());
    APawn *OtherPawn = NewObject<APawn>(GetTransientPackage());

    auto Credited = [&](const AActor *V, const AActor *K, const APawn *KP) {
        return UMythicLifeComponent::IsKillCreditedToOther(V, K, KP);
    };

    TestTrue(TEXT("a different killer is credited"), Credited(Victim, Other, OtherPawn));
    TestFalse(TEXT("killing yourself is not credited"), Credited(Victim, Victim, nullptr));

    // A player instigates from their PlayerState, so the instigator never equals the victim pawn. Only the
    // pawn behind it reveals a suicide.
    TestFalse(TEXT("a player suicide is not credited through their player state"), Credited(VictimPawn, Other, VictimPawn));
    TestTrue(TEXT("a player killing someone else is credited"), Credited(VictimPawn, Other, OtherPawn));

    TestFalse(TEXT("no victim is not credited"), Credited(nullptr, Other, OtherPawn));
    TestFalse(TEXT("no killer is not credited"), Credited(Victim, nullptr, nullptr));

    return true;
}
