#include "Misc/AutomationTest.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/ScopeExit.h"

#include "GAS/Abilities/MythicWeaponAttackAbility.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicTags_GAS.h"
#include "World/Harvesting/MythicHarvestTypes.h"

namespace {

FMythicHarvestResult MakeHarvestStruckResult(const EMythicHarvestOutcome Outcome, const double WorkUnits) {
    FMythicHarvestResult Result;
    Result.Outcome = Outcome;
    Result.RejectReason = Outcome == EMythicHarvestOutcome::Rejected
        ? EMythicHarvestRejectReason::NodeDepleted
        : EMythicHarvestRejectReason::None;
    FMythicHarvestWork::TryFromWorkUnits(WorkUnits, Result.AppliedWork);
    return Result;
}

// The striker's ability system is what a rune listens on; the resource is whatever actor the node's ISM belongs to.
struct FMythicHarvestStruckFixture {
    UGameInstance *GameInstance = nullptr;
    UWorld *World = nullptr;
    AActor *Striker = nullptr;
    AActor *Resource = nullptr;
    UMythicAbilitySystemComponent *ASC = nullptr;
};

bool BuildHarvestStruckFixture(FAutomationTestBase &Test, FMythicHarvestStruckFixture &Out) {
    if (!Test.TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }
    Out.GameInstance = NewObject<UGameInstance>(GEngine);
    Out.GameInstance->InitializeStandalone();
    Out.World = Out.GameInstance->GetWorld();
    if (!Test.TestNotNull(TEXT("standalone world exists"), Out.World)) {
        return false;
    }
    Out.Striker = Out.World->SpawnActor<AActor>();
    Out.Resource = Out.World->SpawnActor<AActor>();
    if (!Test.TestNotNull(TEXT("the striker spawned"), Out.Striker)
        || !Test.TestNotNull(TEXT("the resource actor spawned"), Out.Resource)) {
        return false;
    }
    Out.ASC = NewObject<UMythicAbilitySystemComponent>(Out.Striker);
    Out.ASC->RegisterComponent();
    Out.ASC->InitAbilityActorInfo(Out.Striker, Out.Striker);
    return Test.TestTrue(TEXT("the ability system answers as authority"), Out.ASC->IsOwnerActorAuthoritative());
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestStruckPayloadTest,
    "Mythic.Combat.HarvestStruck.Payload",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestStruckPayloadTest::RunTest(const FString &Parameters) {
    AActor *Striker = NewObject<AActor>(GetTransientPackage());
    AActor *Resource = NewObject<AActor>(GetTransientPackage());
    FGameplayEventData Event;

    TestFalse(TEXT("a rejected contact builds no event"),
              UMythicWeaponAttackAbility::MakeHarvestStruckEvent(
                  MakeHarvestStruckResult(EMythicHarvestOutcome::Rejected, 0.0), Striker, Resource, Event));

    TestTrue(TEXT("an accepted contact builds the event"),
             UMythicWeaponAttackAbility::MakeHarvestStruckEvent(
                 MakeHarvestStruckResult(EMythicHarvestOutcome::Accepted, 2.5), Striker, Resource, Event));
    TestTrue(TEXT("the event is Harvest.Struck"), Event.EventTag == GAS_EVENT_HARVEST_STRUCK);
    TestTrue(TEXT("the striker is the instigator"), Event.Instigator.Get() == Striker);
    TestTrue(TEXT("the resource actor is the target"), Event.Target.Get() == Resource);
    TestEqual(TEXT("the magnitude is the work the node took"), Event.EventMagnitude, 2.5f, UE_KINDA_SMALL_NUMBER);
    TestFalse(TEXT("a bite that leaves the node standing carries no Harvest.Felled"),
              Event.InstigatorTags.HasTagExact(HARVEST_FELLED));

    TestTrue(TEXT("a felling contact builds the event"),
             UMythicWeaponAttackAbility::MakeHarvestStruckEvent(
                 MakeHarvestStruckResult(EMythicHarvestOutcome::Completed, 1.0), Striker, Resource, Event));
    TestTrue(TEXT("the felling strike carries Harvest.Felled"), Event.InstigatorTags.HasTagExact(HARVEST_FELLED));
    TestTrue(TEXT("a felling strike is still Harvest.Struck"), Event.EventTag == GAS_EVENT_HARVEST_STRUCK);
    TestTrue(TEXT("the felled resource is still the target"), Event.Target.Get() == Resource);
    TestEqual(TEXT("the felling bite reports its own work"), Event.EventMagnitude, 1.0f, UE_KINDA_SMALL_NUMBER);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestStruckReachesStrikerTest,
    "Mythic.Combat.HarvestStruck.ReachesTheStriker",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestStruckReachesStrikerTest::RunTest(const FString &Parameters) {
    FMythicHarvestStruckFixture Fixture;
    const bool bReady = BuildHarvestStruckFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicAbilitySystemComponent *ASC = Fixture.ASC;

    int32 Raised = 0;
    FGameplayEventData Seen;
    ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_HARVEST_STRUCK).AddLambda(
        [&Raised, &Seen](const FGameplayEventData *Payload) {
            ++Raised;
            if (Payload) {
                Seen = *Payload;
            }
        });

    TestFalse(TEXT("a rejected contact raises nothing"),
              UMythicWeaponAttackAbility::RaiseHarvestStruck(
                  ASC, Fixture.Striker, Fixture.Resource, MakeHarvestStruckResult(EMythicHarvestOutcome::Rejected, 0.0)));
    TestEqual(TEXT("the striker heard nothing for the rejected contact"), Raised, 0);

    TestTrue(TEXT("an accepted contact raises Harvest.Struck"),
             UMythicWeaponAttackAbility::RaiseHarvestStruck(
                 ASC, Fixture.Striker, Fixture.Resource, MakeHarvestStruckResult(EMythicHarvestOutcome::Accepted, 3.0)));
    TestEqual(TEXT("the striker heard the accepted contact once"), Raised, 1);
    TestTrue(TEXT("the striker sees the resource actor as the target"), Seen.Target.Get() == Fixture.Resource);
    TestTrue(TEXT("the striker sees itself as the instigator"), Seen.Instigator.Get() == Fixture.Striker);
    TestEqual(TEXT("the striker sees the work applied"), Seen.EventMagnitude, 3.0f, UE_KINDA_SMALL_NUMBER);
    TestFalse(TEXT("a standing node is not reported felled"), Seen.InstigatorTags.HasTagExact(HARVEST_FELLED));

    TestTrue(TEXT("a felling contact raises Harvest.Struck"),
             UMythicWeaponAttackAbility::RaiseHarvestStruck(
                 ASC, Fixture.Striker, Fixture.Resource, MakeHarvestStruckResult(EMythicHarvestOutcome::Completed, 1.0)));
    TestEqual(TEXT("the striker heard the felling contact"), Raised, 2);
    TestTrue(TEXT("the felling contact carries Harvest.Felled"), Seen.InstigatorTags.HasTagExact(HARVEST_FELLED));
    TestTrue(TEXT("the felled node is still the target"), Seen.Target.Get() == Fixture.Resource);

    TestFalse(TEXT("no ability system means nothing to raise on"),
              UMythicWeaponAttackAbility::RaiseHarvestStruck(
                  nullptr, Fixture.Striker, Fixture.Resource, MakeHarvestStruckResult(EMythicHarvestOutcome::Accepted, 1.0)));
    TestEqual(TEXT("a missing ability system raised nothing"), Raised, 2);

    AddInfo(FString::Printf(TEXT("Harvest.Struck raised %d times: 1 bite, 1 felling; rejected and system-less contacts raised none"), Raised));
    return true;
}
