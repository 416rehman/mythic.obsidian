// Mythic — Placeable fragment unit tests
// Covers the pure placement-validation rule shared by the client ghost-preview and the server deploy authority,
// plus the instance-level degrees->cosine slope conversion.
// Run via: Session Frontend → Automation → Mythic.Itemization.Placement

#include "Misc/AutomationTest.h"
#include "Itemization/Inventory/Fragments/Passive/PlaceableFragment.h"
#include "Itemization/Placeable/MythicPlacementModeComponent.h"

namespace PlaceableTestHelpers {
    // Build a placement query inline. Order: hit?, surface normal Z, distance (cm), blocking overlap?
    static FPlaceablePlacementQuery MakeQuery(bool bHit, float NormalZ, float Distance, bool bOverlap) {
        FPlaceablePlacementQuery Q;
        Q.bDidHitSurface = bHit;
        Q.SurfaceNormalZ = NormalZ;
        Q.DistanceFromInstigator = Distance;
        Q.bHasBlockingOverlap = bOverlap;
        return Q;
    }
}

// ─── Core verdicts ────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlaceablePlacementRulesTest,
    "Mythic.Itemization.Placement.CoreRules",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlaceablePlacementRulesTest::RunTest(const FString &Parameters) {
    using namespace PlaceableTestHelpers;
    // Rules: reach 200cm, min surface normal Z 0.8 (~36deg), ground required.

    // Flat ground, within reach, clear -> Valid.
    TestEqual(TEXT("Flat clear in-reach ground is placeable"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(true, 1.0f, 150.0f, false), 200.0f, 0.8f, true),
              EPlaceablePlacementResult::Valid);

    // Aimed at empty space while ground is required -> NoSurface.
    TestEqual(TEXT("No surface hit (ground required) rejects"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(false, 1.0f, 100.0f, false), 200.0f, 0.8f, true),
              EPlaceablePlacementResult::NoSurface);

    // Surface farther than reach -> OutOfReach.
    TestEqual(TEXT("Beyond reach rejects"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(true, 1.0f, 300.0f, false), 200.0f, 0.8f, true),
              EPlaceablePlacementResult::OutOfReach);

    // Steep surface (normal Z 0.5 < 0.8) -> SurfaceTooSteep.
    TestEqual(TEXT("Too-steep surface rejects"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(true, 0.5f, 100.0f, false), 200.0f, 0.8f, true),
              EPlaceablePlacementResult::SurfaceTooSteep);

    // Flat, in reach, but something is in the way -> Obstructed.
    TestEqual(TEXT("Blocking overlap rejects"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(true, 1.0f, 100.0f, true), 200.0f, 0.8f, true),
              EPlaceablePlacementResult::Obstructed);

    return true;
}

// ─── Inclusive boundaries (lock the comparison operators) ─────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlaceablePlacementBoundaryTest,
    "Mythic.Itemization.Placement.Boundaries",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlaceablePlacementBoundaryTest::RunTest(const FString &Parameters) {
    using namespace PlaceableTestHelpers;

    // Distance exactly at the reach limit is allowed (reject is strictly greater-than).
    TestEqual(TEXT("Distance exactly at reach is allowed"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(true, 1.0f, 200.0f, false), 200.0f, 0.8f, true),
              EPlaceablePlacementResult::Valid);
    TestEqual(TEXT("Just beyond reach rejects"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(true, 1.0f, 201.0f, false), 200.0f, 0.8f, true),
              EPlaceablePlacementResult::OutOfReach);

    // Surface normal exactly at the slope limit is allowed (reject is strictly less-than).
    TestEqual(TEXT("Surface exactly at slope limit is allowed"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(true, 0.8f, 100.0f, false), 200.0f, 0.8f, true),
              EPlaceablePlacementResult::Valid);
    TestEqual(TEXT("Surface just past slope limit rejects"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(true, 0.79f, 100.0f, false), 200.0f, 0.8f, true),
              EPlaceablePlacementResult::SurfaceTooSteep);

    return true;
}

// ─── Check precedence (lock the evaluation order) ─────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlaceablePlacementPrecedenceTest,
    "Mythic.Itemization.Placement.Precedence",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlaceablePlacementPrecedenceTest::RunTest(const FString &Parameters) {
    using namespace PlaceableTestHelpers;

    // Out-of-reach is reported before steepness and overlap (a far, steep, blocked spot reads OutOfReach).
    TestEqual(TEXT("Reach failure takes precedence over steep + overlap"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(true, 0.1f, 500.0f, true), 200.0f, 0.8f, true),
              EPlaceablePlacementResult::OutOfReach);

    // Steepness is reported before overlap (an in-reach, steep, blocked spot reads SurfaceTooSteep).
    TestEqual(TEXT("Steepness takes precedence over overlap"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(true, 0.1f, 100.0f, true), 200.0f, 0.8f, true),
              EPlaceablePlacementResult::SurfaceTooSteep);

    return true;
}

// ─── Floating placeables (bRequireGround = false) ─────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlaceableFloatingTest,
    "Mythic.Itemization.Placement.Floating",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlaceableFloatingTest::RunTest(const FString &Parameters) {
    using namespace PlaceableTestHelpers;

    // A floating placeable may be placed in mid-air (no surface hit) when in reach and clear.
    TestEqual(TEXT("Floating placeable allows a mid-air spot"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(false, 1.0f, 150.0f, false), 200.0f, 0.8f, false),
              EPlaceablePlacementResult::Valid);

    // Slope is not enforced for a floating placeable: a vertical-wall hit (normal Z 0) is still Valid.
    TestEqual(TEXT("Floating placeable ignores surface slope"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(true, 0.0f, 150.0f, false), 200.0f, 0.8f, false),
              EPlaceablePlacementResult::Valid);

    // Reach and overlap STILL apply to floating placeables.
    TestEqual(TEXT("Floating placeable still respects reach"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(false, 1.0f, 300.0f, false), 200.0f, 0.8f, false),
              EPlaceablePlacementResult::OutOfReach);
    TestEqual(TEXT("Floating placeable still respects overlap"),
              UPlaceableFragment::EvaluatePlacement(MakeQuery(true, 1.0f, 150.0f, true), 200.0f, 0.8f, false),
              EPlaceablePlacementResult::Obstructed);

    return true;
}

// ─── Instance path: authored degrees -> cosine slope threshold ────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlaceableFragmentInstanceRulesTest,
    "Mythic.Itemization.Placement.InstanceSlopeConversion",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlaceableFragmentInstanceRulesTest::RunTest(const FString &Parameters) {
    using namespace PlaceableTestHelpers;

    UPlaceableFragment *Fragment = NewObject<UPlaceableFragment>();
    Fragment->MaxGroundSlopeDegrees = 60.0f; // cos(60) = 0.5
    Fragment->MaxPlacementReach = 200.0f;
    Fragment->bRequireGroundSurface = true;

    TestNearlyEqual(TEXT("60-degree slope limit yields a 0.5 normal-Z threshold"),
                    Fragment->GetMinSurfaceNormalZ(), 0.5f, 0.001f);

    // A surface above the threshold (normal Z 0.6 > 0.5) is placeable through the instance method.
    TestEqual(TEXT("Surface within authored slope is placeable"),
              Fragment->EvaluatePlacement(MakeQuery(true, 0.6f, 100.0f, false)),
              EPlaceablePlacementResult::Valid);

    // A surface below the threshold (normal Z 0.4 < 0.5) is rejected as too steep.
    TestEqual(TEXT("Surface past authored slope is too steep"),
              Fragment->EvaluatePlacement(MakeQuery(true, 0.4f, 100.0f, false)),
              EPlaceablePlacementResult::SurfaceTooSteep);

    return true;
}

// ─── Placement-mode session brain (DecidePlacementAction, model B + stay-in-mode) ───
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlaceableSessionActionTest,
    "Mythic.Itemization.Placement.SessionAction",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlaceableSessionActionTest::RunTest(const FString &Parameters) {
    using EAct = EMythicPlacementAction;
    auto Decide = &UMythicPlacementModeComponent::DecidePlacementAction; // (cancel, present, confirm, valid)

    // Cancel always exits — even with an otherwise-valid confirm.
    TestEqual(TEXT("cancel exits (masks a valid confirm)"), Decide(true, true, true, true), EAct::Exit);

    // Source item gone (stack exhausted) exits — even on a confirm.
    TestEqual(TEXT("source item gone exits"), Decide(false, false, true, true), EAct::Exit);

    // A valid confirm with stock present deploys (the component then STAYS in mode).
    TestEqual(TEXT("valid confirm + stock → deploy"), Decide(false, true, true, true), EAct::Deploy);

    // A confirm on an INVALID spot does not deploy — just refreshes the ghost.
    TestEqual(TEXT("confirm on invalid spot → update ghost"), Decide(false, true, true, false), EAct::UpdateGhost);

    // No confirm, stock present → just keep the ghost current.
    TestEqual(TEXT("no confirm → update ghost"), Decide(false, true, false, true), EAct::UpdateGhost);

    // Precedence: cancel beats source-gone beats deploy.
    TestEqual(TEXT("cancel beats everything"), Decide(true, false, true, true), EAct::Exit);

    return true;
}

// ─── Server deploy-gate decision (PlanDeploy) ─────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlaceableDeployPlanTest,
    "Mythic.Itemization.Placement.DeployPlan",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlaceableDeployPlanTest::RunTest(const FString &Parameters) {
    // All gates pass -> Deployed.
    TestEqual(TEXT("Authorized + item + class + valid placement deploys"),
              UPlaceableFragment::PlanDeploy(true, true, true, EPlaceablePlacementResult::Valid),
              EPlaceableDeployResult::Deployed);

    // Unauthorized inventory -> NotAuthorized (even with everything else valid).
    TestEqual(TEXT("Unauthorized inventory is rejected"),
              UPlaceableFragment::PlanDeploy(false, true, true, EPlaceablePlacementResult::Valid),
              EPlaceableDeployResult::NotAuthorized);

    // Empty/non-placeable slot -> SlotEmpty.
    TestEqual(TEXT("Empty slot is rejected"),
              UPlaceableFragment::PlanDeploy(true, false, true, EPlaceablePlacementResult::Valid),
              EPlaceableDeployResult::SlotEmpty);

    // Missing DeployedActorClass -> NoDeployedClass (content error).
    TestEqual(TEXT("Missing deployed class is rejected"),
              UPlaceableFragment::PlanDeploy(true, true, false, EPlaceablePlacementResult::Valid),
              EPlaceableDeployResult::NoDeployedClass);

    // Bad placement -> PlacementInvalid (carry through any non-Valid placement verdict).
    TestEqual(TEXT("Invalid placement is rejected"),
              UPlaceableFragment::PlanDeploy(true, true, true, EPlaceablePlacementResult::OutOfReach),
              EPlaceableDeployResult::PlacementInvalid);
    TestEqual(TEXT("Obstructed placement is rejected"),
              UPlaceableFragment::PlanDeploy(true, true, true, EPlaceablePlacementResult::Obstructed),
              EPlaceableDeployResult::PlacementInvalid);

    return true;
}

// ─── Deploy-gate precedence (authority first, then slot, then content, then placement) ───
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlaceableDeployPrecedenceTest,
    "Mythic.Itemization.Placement.DeployPrecedence",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlaceableDeployPrecedenceTest::RunTest(const FString &Parameters) {
    // Authority is checked before anything else — an unauthorized request with EVERYTHING else also failing still
    // reports only NotAuthorized (never leaks that the slot was empty / placement bad).
    TestEqual(TEXT("Authority failure masks all other failures"),
              UPlaceableFragment::PlanDeploy(false, false, false, EPlaceablePlacementResult::NoSurface),
              EPlaceableDeployResult::NotAuthorized);

    // With authority, slot-empty is reported before the content check.
    TestEqual(TEXT("Slot check precedes content check"),
              UPlaceableFragment::PlanDeploy(true, false, false, EPlaceablePlacementResult::NoSurface),
              EPlaceableDeployResult::SlotEmpty);

    // With authority + item, the content check precedes the placement check.
    TestEqual(TEXT("Content check precedes placement check"),
              UPlaceableFragment::PlanDeploy(true, true, false, EPlaceablePlacementResult::NoSurface),
              EPlaceableDeployResult::NoDeployedClass);

    return true;
}

// ─── Trace -> query bridge (BuildPlacementQuery) ──────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlaceableBuildQueryTest,
    "Mythic.Itemization.Placement.BuildQuery",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlaceableBuildQueryTest::RunTest(const FString &Parameters) {
    // A hit: candidate point is the impact, distance measured instigator->impact, normalZ from the impact normal.
    {
        const FPlaceablePlacementQuery Q = UPlaceableFragment::BuildPlacementQuery(
            true, FVector(100.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, 1.0f), FVector(200.0f, 0.0f, 0.0f), FVector::ZeroVector, false);
        TestTrue(TEXT("hit recorded"), Q.bDidHitSurface);
        TestNearlyEqual(TEXT("normalZ from impact"), Q.SurfaceNormalZ, 1.0f, 0.001f);
        TestNearlyEqual(TEXT("distance to impact point"), Q.DistanceFromInstigator, 100.0f, 0.01f);
        TestFalse(TEXT("no overlap"), Q.bHasBlockingOverlap);
    }

    // A miss: candidate point is the trace endpoint, distance measured to the endpoint, normalZ defaults flat.
    {
        const FPlaceablePlacementQuery Q = UPlaceableFragment::BuildPlacementQuery(
            false, FVector::ZeroVector, FVector::ZeroVector, FVector(200.0f, 0.0f, 0.0f), FVector::ZeroVector, false);
        TestFalse(TEXT("miss recorded"), Q.bDidHitSurface);
        TestNearlyEqual(TEXT("distance to trace end"), Q.DistanceFromInstigator, 200.0f, 0.01f);
        TestNearlyEqual(TEXT("miss treated as flat"), Q.SurfaceNormalZ, 1.0f, 0.001f);
    }

    // Distance is instigator->candidate (not from world origin); a steep normal + overlap are carried verbatim.
    {
        const FPlaceablePlacementQuery Q = UPlaceableFragment::BuildPlacementQuery(
            true, FVector(100.0f, 0.0f, 0.0f), FVector(0.7f, 0.0f, 0.5f), FVector(300.0f, 0.0f, 0.0f), FVector(50.0f, 0.0f, 0.0f), true);
        TestNearlyEqual(TEXT("distance instigator->candidate"), Q.DistanceFromInstigator, 50.0f, 0.01f);
        TestNearlyEqual(TEXT("steep normalZ carried"), Q.SurfaceNormalZ, 0.5f, 0.001f);
        TestTrue(TEXT("overlap carried"), Q.bHasBlockingOverlap);
    }

    // Composition: the bridge feeds EvaluatePlacement and produces the right verdict end-to-end.
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

// ─── Verdict -> client preview presentation (DescribePlacement) ───
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlaceablePreviewTest,
    "Mythic.Itemization.Placement.Preview",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlaceablePreviewTest::RunTest(const FString &Parameters) {
    // Valid -> confirmable, green, no reason text.
    {
        const FPlaceablePreview P = UPlaceableFragment::DescribePlacement(EPlaceablePlacementResult::Valid);
        TestTrue(TEXT("valid is confirmable"), P.bCanConfirm);
        TestTrue(TEXT("valid tints green"), P.TintColor.Equals(FLinearColor::Green));
        TestTrue(TEXT("valid has no reason"), P.Reason.IsEmpty());
    }

    // Every rejection -> NOT confirmable, red, with a non-empty player-facing reason.
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
