
#include "Misc/AutomationTest.h"
#include "Itemization/Inventory/Fragments/Passive/PlaceableFragment.h"
#include "Itemization/Placeable/MythicPlacementModeComponent.h"
#include "Player/MythicPlayerController.h"

namespace PlaceableTestHelpers {
    static FPlaceablePlacementQuery MakeQuery(bool bHit, float NormalZ, float Distance, bool bOverlap) {
        FPlaceablePlacementQuery Q;
        Q.bDidHitSurface = bHit;
        Q.SurfaceNormalZ = NormalZ;
        Q.DistanceFromInstigator = Distance;
        Q.bHasBlockingOverlap = bOverlap;
        return Q;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlaceablePlacementRulesTest,
    "Mythic.Itemization.Placement.CoreRules",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlaceablePlacementRulesTest::RunTest(const FString &Parameters) {
    using namespace PlaceableTestHelpers;

    TestEqual(TEXT("Flat clear in-reach ground is placeable"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(true, 1.0f, 150.0f, false), 200.0f, 0.8f, true),
              EPlaceablePlacementResult::Valid);

    TestEqual(TEXT("No surface hit (ground required) rejects"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(false, 1.0f, 100.0f, false), 200.0f, 0.8f, true),
              EPlaceablePlacementResult::NoSurface);

    TestEqual(TEXT("Beyond reach rejects"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(true, 1.0f, 300.0f, false), 200.0f, 0.8f, true),
              EPlaceablePlacementResult::OutOfReach);

    TestEqual(TEXT("Too-steep surface rejects"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(true, 0.5f, 100.0f, false), 200.0f, 0.8f, true),
              EPlaceablePlacementResult::SurfaceTooSteep);

    TestEqual(TEXT("Blocking overlap rejects"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(true, 1.0f, 100.0f, true), 200.0f, 0.8f, true),
              EPlaceablePlacementResult::Obstructed);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlaceablePlacementBoundaryTest,
    "Mythic.Itemization.Placement.Boundaries",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlaceablePlacementBoundaryTest::RunTest(const FString &Parameters) {
    using namespace PlaceableTestHelpers;

    TestEqual(TEXT("Distance exactly at reach is allowed"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(true, 1.0f, 200.0f, false), 200.0f, 0.8f, true),
              EPlaceablePlacementResult::Valid);
    TestEqual(TEXT("Just beyond reach rejects"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(true, 1.0f, 201.0f, false), 200.0f, 0.8f, true),
              EPlaceablePlacementResult::OutOfReach);

    TestEqual(TEXT("Surface exactly at slope limit is allowed"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(true, 0.8f, 100.0f, false), 200.0f, 0.8f, true),
              EPlaceablePlacementResult::Valid);
    TestEqual(TEXT("Surface just past slope limit rejects"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(true, 0.79f, 100.0f, false), 200.0f, 0.8f, true),
              EPlaceablePlacementResult::SurfaceTooSteep);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlaceablePlacementPrecedenceTest,
    "Mythic.Itemization.Placement.Precedence",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlaceablePlacementPrecedenceTest::RunTest(const FString &Parameters) {
    using namespace PlaceableTestHelpers;

    TestEqual(TEXT("Reach failure takes precedence over steep + overlap"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(true, 0.1f, 500.0f, true), 200.0f, 0.8f, true),
              EPlaceablePlacementResult::OutOfReach);

    TestEqual(TEXT("Steepness takes precedence over overlap"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(true, 0.1f, 100.0f, true), 200.0f, 0.8f, true),
              EPlaceablePlacementResult::SurfaceTooSteep);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlaceableFloatingTest,
    "Mythic.Itemization.Placement.Floating",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlaceableFloatingTest::RunTest(const FString &Parameters) {
    using namespace PlaceableTestHelpers;

    TestEqual(TEXT("Floating placeable allows a mid-air spot"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(false, 1.0f, 150.0f, false), 200.0f, 0.8f, false),
              EPlaceablePlacementResult::Valid);

    TestEqual(TEXT("Floating placeable ignores surface slope"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(true, 0.0f, 150.0f, false), 200.0f, 0.8f, false),
              EPlaceablePlacementResult::Valid);

    TestEqual(TEXT("Floating placeable still respects reach"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(false, 1.0f, 300.0f, false), 200.0f, 0.8f, false),
              EPlaceablePlacementResult::OutOfReach);
    TestEqual(TEXT("Floating placeable still respects overlap"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(true, 1.0f, 150.0f, true), 200.0f, 0.8f, false),
              EPlaceablePlacementResult::Obstructed);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlaceableFragmentInstanceRulesTest,
    "Mythic.Itemization.Placement.InstanceSlopeConversion",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlaceableFragmentInstanceRulesTest::RunTest(const FString &Parameters) {
    using namespace PlaceableTestHelpers;

    UPlaceableFragment *Fragment = NewObject<UPlaceableFragment>();
    Fragment->MaxGroundSlopeDegrees = 60.0f;
    Fragment->MaxPlacementReach = 200.0f;
    Fragment->bRequireGroundSurface = true;

    TestNearlyEqual(TEXT("60-degree slope limit yields a 0.5 normal-Z threshold"),
                    Fragment->GetMinSurfaceNormalZ(), 0.5f, 0.001f);

    TestEqual(TEXT("Surface within authored slope is placeable"),
              Fragment->EvaluatePlacement(MakeQuery(true, 0.6f, 100.0f, false)),
              EPlaceablePlacementResult::Valid);

    TestEqual(TEXT("Surface past authored slope is too steep"),
              Fragment->EvaluatePlacement(MakeQuery(true, 0.4f, 100.0f, false)),
              EPlaceablePlacementResult::SurfaceTooSteep);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlaceableSessionActionTest,
    "Mythic.Itemization.Placement.SessionAction",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlaceableSessionActionTest::RunTest(const FString &Parameters) {
    using EAct = EMythicPlacementAction;
    auto Decide = &UMythicPlacementModeComponent::DecidePlacementAction;

    TestEqual(TEXT("cancel exits (masks a valid confirm)"), Decide(true, true, true, true), EAct::Exit);

    TestEqual(TEXT("source item gone exits"), Decide(false, false, true, true), EAct::Exit);

    TestEqual(TEXT("valid confirm + stock → deploy"), Decide(false, true, true, true), EAct::Deploy);

    TestEqual(TEXT("confirm on invalid spot → update ghost"), Decide(false, true, true, false), EAct::UpdateGhost);

    TestEqual(TEXT("no confirm → update ghost"), Decide(false, true, false, true), EAct::UpdateGhost);

    TestEqual(TEXT("cancel beats everything"), Decide(true, false, true, true), EAct::Exit);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlaceableDeployPlanTest,
    "Mythic.Itemization.Placement.DeployPlan",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlaceableDeployPlanTest::RunTest(const FString &Parameters) {
    TestEqual(TEXT("Authorized + item + class + valid placement deploys"),
              UPlaceableFragment::PlanDeploy(true, true, true, EPlaceablePlacementResult::Valid),
              EPlaceableDeployResult::Deployed);

    TestEqual(TEXT("Unauthorized inventory is rejected"),
              UPlaceableFragment::PlanDeploy(false, true, true, EPlaceablePlacementResult::Valid),
              EPlaceableDeployResult::NotAuthorized);

    TestEqual(TEXT("Empty slot is rejected"),
              UPlaceableFragment::PlanDeploy(true, false, true, EPlaceablePlacementResult::Valid),
              EPlaceableDeployResult::SlotEmpty);

    TestEqual(TEXT("Missing deployed class is rejected"),
              UPlaceableFragment::PlanDeploy(true, true, false, EPlaceablePlacementResult::Valid),
              EPlaceableDeployResult::NoDeployedClass);

    TestEqual(TEXT("Invalid placement is rejected"),
              UPlaceableFragment::PlanDeploy(true, true, true, EPlaceablePlacementResult::OutOfReach),
              EPlaceableDeployResult::PlacementInvalid);
    TestEqual(TEXT("Obstructed placement is rejected"),
              UPlaceableFragment::PlanDeploy(true, true, true, EPlaceablePlacementResult::Obstructed),
              EPlaceableDeployResult::PlacementInvalid);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlaceableDeployPrecedenceTest,
    "Mythic.Itemization.Placement.DeployPrecedence",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlaceableDeployPrecedenceTest::RunTest(const FString &Parameters) {
    TestEqual(TEXT("Authority failure masks all other failures"),
              UPlaceableFragment::PlanDeploy(false, false, false, EPlaceablePlacementResult::NoSurface),
              EPlaceableDeployResult::NotAuthorized);

    TestEqual(TEXT("Slot check precedes content check"),
              UPlaceableFragment::PlanDeploy(true, false, false, EPlaceablePlacementResult::NoSurface),
              EPlaceableDeployResult::SlotEmpty);

    TestEqual(TEXT("Content check precedes placement check"),
              UPlaceableFragment::PlanDeploy(true, true, false, EPlaceablePlacementResult::NoSurface),
              EPlaceableDeployResult::NoDeployedClass);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlaceableBuildQueryTest,
    "Mythic.Itemization.Placement.BuildQuery",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlaceableBuildQueryTest::RunTest(const FString &Parameters) {
    {
        const FPlaceablePlacementQuery Q = UPlaceableFragment::BuildPlacementQuery(
            true, FVector(100.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, 1.0f), FVector(200.0f, 0.0f, 0.0f), FVector::ZeroVector, false);
        TestTrue(TEXT("hit recorded"), Q.bDidHitSurface);
        TestNearlyEqual(TEXT("normalZ from impact"), Q.SurfaceNormalZ, 1.0f, 0.001f);
        TestNearlyEqual(TEXT("distance to impact point"), Q.DistanceFromInstigator, 100.0f, 0.01f);
        TestFalse(TEXT("no overlap"), Q.bHasBlockingOverlap);
    }

    {
        const FPlaceablePlacementQuery Q = UPlaceableFragment::BuildPlacementQuery(
            false, FVector::ZeroVector, FVector::ZeroVector, FVector(200.0f, 0.0f, 0.0f), FVector::ZeroVector, false);
        TestFalse(TEXT("miss recorded"), Q.bDidHitSurface);
        TestNearlyEqual(TEXT("distance to trace end"), Q.DistanceFromInstigator, 200.0f, 0.01f);
        TestNearlyEqual(TEXT("miss treated as flat"), Q.SurfaceNormalZ, 1.0f, 0.001f);
    }

    {
        const FPlaceablePlacementQuery Q = UPlaceableFragment::BuildPlacementQuery(
            true, FVector(100.0f, 0.0f, 0.0f), FVector(0.7f, 0.0f, 0.5f), FVector(300.0f, 0.0f, 0.0f), FVector(50.0f, 0.0f, 0.0f), true);
        TestNearlyEqual(TEXT("distance instigator->candidate"), Q.DistanceFromInstigator, 50.0f, 0.01f);
        TestNearlyEqual(TEXT("steep normalZ carried"), Q.SurfaceNormalZ, 0.5f, 0.001f);
        TestTrue(TEXT("overlap carried"), Q.bHasBlockingOverlap);
    }

    {
        const FPlaceablePlacementQuery Near = UPlaceableFragment::BuildPlacementQuery(
            true, FVector(150.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, 1.0f), FVector(200.0f, 0.0f, 0.0f), FVector::ZeroVector, false);
        TestEqual(TEXT("flat in-reach hit -> Valid"),
                  UPlaceableFragment::EvaluatePlacement(Near, 200.0f, 0.8f, true),
                  EPlaceablePlacementResult::Valid);

        const FPlaceablePlacementQuery Far = UPlaceableFragment::BuildPlacementQuery(
            true, FVector(300.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, 1.0f), FVector(400.0f, 0.0f, 0.0f), FVector::ZeroVector, false);
        TestEqual(TEXT("far hit -> OutOfReach"),
                  UPlaceableFragment::EvaluatePlacement(Far, 200.0f, 0.8f, true),
                  EPlaceablePlacementResult::OutOfReach);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlaceablePreviewTest,
    "Mythic.Itemization.Placement.Preview",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlaceablePreviewTest::RunTest(const FString &Parameters) {
    {
        const FPlaceablePreview P = UPlaceableFragment::DescribePlacement(EPlaceablePlacementResult::Valid);
        TestTrue(TEXT("valid is confirmable"), P.bCanConfirm);
        TestTrue(TEXT("valid tints green"), P.TintColor.Equals(FLinearColor::Green));
        TestTrue(TEXT("valid has no reason"), P.Reason.IsEmpty());
    }

    const EPlaceablePlacementResult Rejections[] = {
        EPlaceablePlacementResult::NoSurface,
        EPlaceablePlacementResult::OutOfReach,
        EPlaceablePlacementResult::SurfaceTooSteep,
        EPlaceablePlacementResult::Obstructed,
    };
    for (const EPlaceablePlacementResult R : Rejections) {
        const FPlaceablePreview P = UPlaceableFragment::DescribePlacement(R);
        TestFalse(TEXT("rejection is not confirmable"), P.bCanConfirm);
        TestTrue(TEXT("rejection tints red"), P.TintColor.Equals(FLinearColor::Red));
        TestFalse(TEXT("rejection has a reason"), P.Reason.IsEmpty());
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlaceableDeployCapTest,
    "Mythic.Itemization.Placement.DeployCap",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlaceableDeployCapTest::RunTest(const FString &Parameters) {
    auto Can = &AMythicPlayerController::CanDeployMore;

    TestTrue(TEXT("cap 0 is unlimited (none placed)"), Can(0, 0));
    TestTrue(TEXT("cap 0 is unlimited (many placed)"), Can(999, 0));
    TestTrue(TEXT("negative cap is unlimited"), Can(50, -1));

    TestTrue(TEXT("under the cap → allowed"), Can(0, 3));
    TestTrue(TEXT("one below the cap → allowed"), Can(2, 3));
    TestFalse(TEXT("exactly at the cap → blocked"), Can(3, 3));
    TestFalse(TEXT("over the cap (defensive) → blocked"), Can(4, 3));

    TestTrue(TEXT("cap 1, none placed → allowed"), Can(0, 1));
    TestFalse(TEXT("cap 1, one placed → blocked"), Can(1, 1));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlaceableDeployFeedbackTest,
    "Mythic.Itemization.Placement.DeployFeedback",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlaceableDeployFeedbackTest::RunTest(const FString &Parameters) {
    using D = EPlaceableDeployResult;
    using P = EPlaceablePlacementResult;
    auto Msg = &UPlaceableFragment::DescribeDeployFailure;

    TestTrue(TEXT("Deployed → no message"), Msg(D::Deployed, P::Valid).IsEmpty());

    TestTrue(TEXT("SlotEmpty → no message (UI shouldn't offer it)"), Msg(D::SlotEmpty, P::Valid).IsEmpty());
    TestTrue(TEXT("NoDeployedClass → no message (content error, logged)"), Msg(D::NoDeployedClass, P::Valid).IsEmpty());

    TestFalse(TEXT("NotAuthorized → a player-facing line"), Msg(D::NotAuthorized, P::Valid).IsEmpty());

    TestTrue(TEXT("PlacementInvalid(OutOfReach) reuses the placement reason"),
             Msg(D::PlacementInvalid, P::OutOfReach).EqualTo(UPlaceableFragment::DescribePlacement(P::OutOfReach).Reason));
    TestTrue(TEXT("PlacementInvalid(Obstructed) reuses the placement reason"),
             Msg(D::PlacementInvalid, P::Obstructed).EqualTo(UPlaceableFragment::DescribePlacement(P::Obstructed).Reason));
    TestFalse(TEXT("PlacementInvalid(SurfaceTooSteep) is a non-empty reason"),
              Msg(D::PlacementInvalid, P::SurfaceTooSteep).IsEmpty());

    return true;
}
