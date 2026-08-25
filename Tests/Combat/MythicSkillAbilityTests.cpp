// Copyright Stellar Games. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/ScopeExit.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameplayAbilitySpec.h"

#include "GAS/Abilities/MythicGA_Skill.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/MythicAbilitySystemComponent.h"

namespace {
FMythicSkillShape MakeShape(const EMythicSkillShape Kind, const float Radius, const float AngleDegrees = 90.0f, const int32 MaxTargets = 0) {
    FMythicSkillShape Shape;
    Shape.Shape = Kind;
    Shape.Radius = Radius;
    Shape.AngleDegrees = AngleDegrees;
    Shape.MaxTargets = MaxTargets;
    return Shape;
}

// A point Distance away, Degrees off the caster's facing in the horizontal plane.
FVector AtAngle(const float Degrees, const float Distance) {
    const float Radians = FMath::DegreesToRadians(Degrees);
    return FVector(FMath::Cos(Radians) * Distance, FMath::Sin(Radians) * Distance, 0.0);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicSkillSphereTargetingTest,
                                 "Mythic.Combat.SkillTargeting.Sphere",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillSphereTargetingTest::RunTest(const FString &Parameters) {
    const FVector Origin = FVector::ZeroVector;
    const FVector Forward = FVector::ForwardVector;
    const FMythicSkillShape Sphere = MakeShape(EMythicSkillShape::Sphere, 300.0f);

    TestTrue(TEXT("a target inside the radius is taken"), FMythicSkillTargeting::IsInside(Sphere, Origin, Forward, FVector(200, 0, 0)));
    TestFalse(TEXT("a target past the radius is not"), FMythicSkillTargeting::IsInside(Sphere, Origin, Forward, FVector(400, 0, 0)));

    // The edge belongs to the shape: the refusal is "further than Radius", not "at least Radius". Both 300 and
    // 300*300 are exact in floating point, so this is the authored decision and not float noise.
    TestTrue(TEXT("a target exactly on the edge is inside"), FMythicSkillTargeting::IsInside(Sphere, Origin, Forward, FVector(300, 0, 0)));

    // The one shape that ignores facing entirely - a Nova that spared whatever stood behind you would be an arc.
    TestTrue(TEXT("a sphere reaches behind the caster"), FMythicSkillTargeting::IsInside(Sphere, Origin, Forward, FVector(-200, 0, 0)));
    TestTrue(TEXT("and above it"), FMythicSkillTargeting::IsInside(Sphere, Origin, Forward, FVector(0, 0, 250)));

    const FMythicSkillShape NoReach = MakeShape(EMythicSkillShape::Sphere, 0.0f);
    TestFalse(TEXT("a shape with no radius holds nothing, not even the caster's own square"),
              FMythicSkillTargeting::IsInside(NoReach, Origin, Forward, Origin));

    FMythicSkillShape Ahead = MakeShape(EMythicSkillShape::Sphere, 300.0f);
    Ahead.ForwardOffset = 500.0f;
    const FVector AheadOrigin = FMythicSkillTargeting::ResolveOrigin(Ahead, Origin, Forward);
    TestEqual(TEXT("the query sits ForwardOffset ahead of the caster"), AheadOrigin, FVector(500, 0, 0));
    TestTrue(TEXT("an offset sphere covers the ground it was thrown at"),
             FMythicSkillTargeting::IsInside(Ahead, AheadOrigin, Forward, FVector(500, 0, 0)));
    TestFalse(TEXT("and no longer covers the caster's own feet"), FMythicSkillTargeting::IsInside(Ahead, AheadOrigin, Forward, Origin));

    // A pawn mid-frame can hand us a zero facing. The query has to stay on the caster rather than fly to NaN.
    TestEqual(TEXT("no facing leaves the query on the caster"),
              FMythicSkillTargeting::ResolveOrigin(Ahead, FVector(10, 20, 30), FVector::ZeroVector), FVector(10, 20, 30));

    const TArray<FVector> Candidates = {
        FVector(100, 0, 0),   // in, nearest
        FVector(400, 0, 0),   // out
        FVector(-250, 0, 0),  // in, behind
        FVector(0, 0, 290),   // in, above
        FVector(0, 0, 310),   // out
    };
    TArray<int32> Selected;
    Selected.Add(77); // A stale index left in the output would silently damage whatever it now points at.
    FMythicSkillTargeting::SelectTargets(Sphere, Origin, Forward, Candidates, Selected);
    AddInfo(FString::Printf(TEXT("sphere r=300 over %d candidates: %d inside"), Candidates.Num(), Selected.Num()));
    if (!TestEqual(TEXT("three of the five candidates are inside, and the output was cleared first"), Selected.Num(), 3)) {
        return false;
    }
    TestEqual(TEXT("nearest first"), Selected[0], 0);
    TestEqual(TEXT("then the next nearest"), Selected[1], 2);
    TestEqual(TEXT("then the furthest"), Selected[2], 3);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicSkillArcConeTargetingTest,
                                 "Mythic.Combat.SkillTargeting.ArcAndCone",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillArcConeTargetingTest::RunTest(const FString &Parameters) {
    const FVector Origin = FVector::ZeroVector;
    const FVector Forward = FVector::ForwardVector;

    // AngleDegrees is the full width, so a 90 degree swing allows 45 either side. Reading it as the half-angle
    // would make every authored arc twice as wide as the designer typed.
    TestEqual(TEXT("a 90 degree width bounds at 45 degrees off-centre"),
              FMythicSkillTargeting::CosineHalfAngle(90.0f), FMath::Cos(FMath::DegreesToRadians(45.0f)), 0.0001f);
    TestEqual(TEXT("a full turn takes every direction"), FMythicSkillTargeting::CosineHalfAngle(360.0f), -1.0f, 0.0001f);
    TestEqual(TEXT("no width is dead ahead only"), FMythicSkillTargeting::CosineHalfAngle(0.0f), 1.0f, 0.0001f);
    // Unclamped, a width past a full turn wraps back around to a needle.
    TestEqual(TEXT("wider than a full turn is still a full turn"), FMythicSkillTargeting::CosineHalfAngle(720.0f), -1.0f, 0.0001f);
    TestEqual(TEXT("a negative width is dead ahead, not everything"), FMythicSkillTargeting::CosineHalfAngle(-90.0f), 1.0f, 0.0001f);

    const FMythicSkillShape Arc = MakeShape(EMythicSkillShape::Arc, 500.0f, 90.0f);
    const FMythicSkillShape Cone = MakeShape(EMythicSkillShape::Cone, 500.0f, 90.0f);

    auto BothNarrowByAngle = [this, &Origin, &Forward](const TCHAR *Label, const FMythicSkillShape &Shape) {
        TestTrue(FString::Printf(TEXT("%s takes a target in front"), Label),
                 FMythicSkillTargeting::IsInside(Shape, Origin, Forward, AtAngle(0.0f, 200.0f)));
        // The same target, the same distance, the wrong side of the caster.
        TestFalse(FString::Printf(TEXT("%s refuses the same target behind"), Label),
                  FMythicSkillTargeting::IsInside(Shape, Origin, Forward, AtAngle(180.0f, 200.0f)));
        TestTrue(FString::Printf(TEXT("%s takes a target just inside the half-angle"), Label),
                 FMythicSkillTargeting::IsInside(Shape, Origin, Forward, AtAngle(40.0f, 200.0f)));
        TestFalse(FString::Printf(TEXT("%s refuses a target just outside the half-angle"), Label),
                  FMythicSkillTargeting::IsInside(Shape, Origin, Forward, AtAngle(50.0f, 200.0f)));
        TestFalse(FString::Printf(TEXT("%s refuses a target broadside"), Label),
                  FMythicSkillTargeting::IsInside(Shape, Origin, Forward, AtAngle(90.0f, 200.0f)));
        // Facing is a narrowing, never a widening: the radius still binds dead ahead.
        TestFalse(FString::Printf(TEXT("%s refuses a target dead ahead but out of reach"), Label),
                  FMythicSkillTargeting::IsInside(Shape, Origin, Forward, AtAngle(0.0f, 600.0f)));
        // A target standing on the caster has no direction to judge, and a caster mid-frame can have no facing.
        TestTrue(FString::Printf(TEXT("%s takes a target standing on the caster"), Label),
                 FMythicSkillTargeting::IsInside(Shape, Origin, Forward, Origin));
        TestTrue(FString::Printf(TEXT("%s with no facing takes what is in range"), Label),
                 FMythicSkillTargeting::IsInside(Shape, Origin, FVector::ZeroVector, AtAngle(180.0f, 200.0f)));
    };
    BothNarrowByAngle(TEXT("arc"), Arc);
    BothNarrowByAngle(TEXT("cone"), Cone);

    // The whole reason both shapes exist. Uphill target, dead ahead in yaw, 56 degrees up: a swing reaches it,
    // a cone does not.
    const FVector Uphill(200, 0, 300);
    TestTrue(TEXT("an arc ignores height, so a swing reaches up a slope"), FMythicSkillTargeting::IsInside(Arc, Origin, Forward, Uphill));
    TestFalse(TEXT("a cone is bounded in every direction, so it does not"), FMythicSkillTargeting::IsInside(Cone, Origin, Forward, Uphill));

    // 179 rather than 180: a dot product of exactly -1 against a cosine of exactly -1 is a coin toss on the last
    // bit, and a test that flips on rounding is not a test.
    const FMythicSkillShape FullTurn = MakeShape(EMythicSkillShape::Arc, 500.0f, 360.0f);
    TestTrue(TEXT("a 360 arc takes what stands behind the caster"),
             FMythicSkillTargeting::IsInside(FullTurn, Origin, Forward, AtAngle(179.0f, 200.0f)));

    const TArray<FVector> Candidates = {
        AtAngle(180.0f, 100.0f), // behind, nearest of all - the cap must not reward it
        AtAngle(20.0f, 300.0f),  // in
        AtAngle(90.0f, 150.0f),  // broadside, out
        AtAngle(0.0f, 200.0f),   // in, nearer than the 300
        AtAngle(0.0f, 900.0f),   // in front but out of reach
    };
    TArray<int32> Selected;
    FMythicSkillTargeting::SelectTargets(Arc, Origin, Forward, Candidates, Selected);
    AddInfo(FString::Printf(TEXT("90 degree arc r=500 over %d candidates: %d inside"), Candidates.Num(), Selected.Num()));
    if (!TestEqual(TEXT("two of the five candidates are inside the arc"), Selected.Num(), 2)) {
        return false;
    }
    TestEqual(TEXT("the nearer of the two comes first"), Selected[0], 3);
    TestEqual(TEXT("and the further second"), Selected[1], 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicSkillTargetCapTest,
                                 "Mythic.Combat.SkillTargeting.TargetCap",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillTargetCapTest::RunTest(const FString &Parameters) {
    const FVector Origin = FVector::ZeroVector;
    const FVector Forward = FVector::ForwardVector;

    // The three out-of-reach candidates come first on purpose. A cap that trimmed the candidate list instead of
    // the selection would find nothing at all here, and a skill would silently stop hitting anyone whenever the
    // scene handed back a few distant pawns first.
    const TArray<FVector> Candidates = {
        FVector(900, 0, 0), // out
        FVector(800, 0, 0), // out
        FVector(700, 0, 0), // out
        FVector(250, 0, 0), // in, third nearest
        FVector(100, 0, 0), // in, nearest
        FVector(200, 0, 0), // in, second nearest
    };

    FMythicSkillShape Capped = MakeShape(EMythicSkillShape::Sphere, 300.0f, 90.0f, 2);
    TArray<int32> Selected;
    FMythicSkillTargeting::SelectTargets(Capped, Origin, Forward, Candidates, Selected);
    AddInfo(FString::Printf(TEXT("cap 2 over %d candidates, 3 of them inside: %d taken"), Candidates.Num(), Selected.Num()));
    if (!TestEqual(TEXT("the cap counts what was selected, not what was considered"), Selected.Num(), 2)) {
        return false;
    }
    // Ordering before capping is what makes a capped skill hit what you are standing next to.
    TestEqual(TEXT("a capped skill keeps the nearest"), Selected[0], 4);
    TestEqual(TEXT("then the second nearest, not whatever the query returned first"), Selected[1], 5);

    Capped.MaxTargets = 0;
    FMythicSkillTargeting::SelectTargets(Capped, Origin, Forward, Candidates, Selected);
    TestEqual(TEXT("no cap takes everyone inside the shape"), Selected.Num(), 3);

    Capped.MaxTargets = 10;
    FMythicSkillTargeting::SelectTargets(Capped, Origin, Forward, Candidates, Selected);
    TestEqual(TEXT("a cap above the count changes nothing"), Selected.Num(), 3);

    Capped.MaxTargets = 1;
    FMythicSkillTargeting::SelectTargets(Capped, Origin, Forward, Candidates, Selected);
    if (!TestEqual(TEXT("a cap of one takes one"), Selected.Num(), 1)) {
        return false;
    }
    TestEqual(TEXT("and it is the nearest"), Selected[0], 4);

    TArray<int32> Empty;
    FMythicSkillTargeting::SelectTargets(Capped, Origin, Forward, TArray<FVector>(), Empty);
    TestEqual(TEXT("a shape that found nobody takes nobody"), Empty.Num(), 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicSkillBonusScalingTest,
                                 "Mythic.Combat.SkillTargeting.BonusScaling",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillBonusScalingTest::RunTest(const FString &Parameters) {
    const FVector Origin = FVector::ZeroVector;
    const FVector Forward = FVector::ForwardVector;

    TestEqual(TEXT("radius gear adds centimetres to the authored reach"), UMythicGA_Skill::ScaleRadius(400.0f, 50.0f), 450.0f);
    TestEqual(TEXT("cursed radius gear takes them away"), UMythicGA_Skill::ScaleRadius(400.0f, -50.0f), 350.0f);
    // A negative reach would be a shape whose comparisons all invert.
    TestEqual(TEXT("but never past zero"), UMythicGA_Skill::ScaleRadius(100.0f, -500.0f), 0.0f);

    FMythicSkillShape Reach = MakeShape(EMythicSkillShape::Sphere, 400.0f);
    const FVector JustOutOfReach(430, 0, 0);
    TestFalse(TEXT("the authored reach refuses a target at 430"), FMythicSkillTargeting::IsInside(Reach, Origin, Forward, JustOutOfReach));
    Reach.Radius = UMythicGA_Skill::ScaleRadius(400.0f, 50.0f);
    TestTrue(TEXT("+50 of radius gear reaches it"), FMythicSkillTargeting::IsInside(Reach, Origin, Forward, JustOutOfReach));
    Reach.Radius = UMythicGA_Skill::ScaleRadius(400.0f, -1000.0f);
    TestFalse(TEXT("a radius floored at zero holds nobody rather than everybody"),
              FMythicSkillTargeting::IsInside(Reach, Origin, Forward, Origin));

    TestEqual(TEXT("target-count gear raises the cap"), UMythicGA_Skill::ScaleTargetCount(2, 3.0f), 5);
    // Nothing to add to "everyone", and treating 0 as a number would cap an uncapped Whirlwind at the bonus.
    TestEqual(TEXT("an uncapped skill stays uncapped"), UMythicGA_Skill::ScaleTargetCount(0, 5.0f), 0);
    // Left unfloored, a cursed count stat goes negative and reads as uncapped - cursed gear would widen the skill.
    TestEqual(TEXT("a capped skill never falls below one target"), UMythicGA_Skill::ScaleTargetCount(3, -10.0f), 1);
    TestEqual(TEXT("a fractional bonus rounds to the nearer whole target"), UMythicGA_Skill::ScaleTargetCount(2, 0.6f), 3);
    TestEqual(TEXT("and rounds down below the half"), UMythicGA_Skill::ScaleTargetCount(2, 0.4f), 2);

    const TArray<FVector> Crowd = {
        FVector(100, 0, 0), FVector(200, 0, 0), FVector(300, 0, 0), FVector(400, 0, 0), FVector(450, 0, 0)
    };
    FMythicSkillShape Cleave = MakeShape(EMythicSkillShape::Sphere, 500.0f, 90.0f, 2);
    TArray<int32> Selected;
    FMythicSkillTargeting::SelectTargets(Cleave, Origin, Forward, Crowd, Selected);
    AddInfo(FString::Printf(TEXT("cap 2 then cap 5 over %d candidates, all inside"), Crowd.Num()));
    TestEqual(TEXT("the authored cap takes two of the five"), Selected.Num(), 2);
    Cleave.MaxTargets = UMythicGA_Skill::ScaleTargetCount(2, 3.0f);
    FMythicSkillTargeting::SelectTargets(Cleave, Origin, Forward, Crowd, Selected);
    TestEqual(TEXT("+3 of target-count gear takes all five"), Selected.Num(), 5);

    TestEqual(TEXT("duration gear adds seconds"), UMythicGA_Skill::ScaleDuration(6.0f, 2.0f), 8.0f);
    // Both ends of this stat used to invert. Crushing a stance returned 0, which the caller reads as "leave the
    // effect alone" and GAS reads as "instant" - a heavy reduction handed back the FULL authored duration.
    TestEqual(TEXT("a crushing reduction leaves a sliver, never a zero that means instant"),
              UMythicGA_Skill::ScaleDuration(6.0f, -10.0f), UMythicGA_Skill::MinScaledDuration);
    TestTrue(TEXT("and that sliver is shorter than the stance it cut down"),
             UMythicGA_Skill::ScaleDuration(6.0f, -10.0f) < 6.0f);
    // The 0 sentinel means "whatever the effect authors". Scaling it as if it were a real 6s stance is what turned
    // a 15s guard into a 2s one the moment the player equipped +2s.
    TestEqual(TEXT("the authored-0 sentinel survives duration gear untouched"),
              UMythicGA_Skill::ScaleDuration(0.0f, 2.0f), 0.0f);

    TestEqual(TEXT("a dash lands MovementDistance ahead"),
              UMythicGA_Skill::ComputeMovementDestination(FVector(100, 0, 0), FVector(0, 2, 0), 300.0f), FVector(100, 300, 0));
    TestEqual(TEXT("no facing means no travel"),
              UMythicGA_Skill::ComputeMovementDestination(FVector(100, 0, 0), FVector::ZeroVector, 300.0f), FVector(100, 0, 0));
    TestEqual(TEXT("a negative distance never dashes backwards"),
              UMythicGA_Skill::ComputeMovementDestination(FVector(100, 0, 0), FVector::ForwardVector, -300.0f), FVector(100, 0, 0));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicSkillResolvedShapeTest,
                                 "Mythic.Combat.SkillTargeting.ResolvedShape",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkillResolvedShapeTest::RunTest(const FString &Parameters) {
    if (!TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }

    UGameInstance *GameInstance = NewObject<UGameInstance>(GEngine);
    GameInstance->InitializeStandalone();
    UWorld *World = GameInstance->GetWorld();
    if (!TestNotNull(TEXT("standalone world exists"), World)) {
        return false;
    }
    ON_SCOPE_EXIT {
        GameInstance->Shutdown();
    };

    AActor *Caster = World->SpawnActor<AActor>();
    UMythicAbilitySystemComponent *ASC = NewObject<UMythicAbilitySystemComponent>(Caster);
    ASC->RegisterComponent();
    ASC->InitAbilityActorInfo(Caster, Caster);
    ASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Offense>(Caster));

    FGameplayAbilitySpec Spec(UMythicGA_Skill::StaticClass(), 1, INDEX_NONE, Caster);
    const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
    if (!TestTrue(TEXT("the skill was granted"), Handle.IsValid())) {
        return false;
    }
    const FGameplayAbilitySpec *Granted = ASC->FindAbilitySpecFromHandle(Handle);
    if (!TestNotNull(TEXT("the granted spec is found"), Granted)) {
        return false;
    }
    UMythicGA_Skill *Skill = Cast<UMythicGA_Skill>(Granted->GetPrimaryInstance());
    if (!TestNotNull(TEXT("the skill is instanced per actor, so it has an owner to read stats off"), Skill)) {
        return false;
    }

    const FGameplayAttribute RadiusBonus = UMythicAttributeSet_Offense::GetSkillRadiusBonusAttribute();
    const FGameplayAttribute CountBonus = UMythicAttributeSet_Offense::GetSkillTargetCountBonusAttribute();
    const FGameplayAttribute DurationBonus = UMythicAttributeSet_Offense::GetSkillDurationBonusAttribute();

    Skill->Shape = MakeShape(EMythicSkillShape::Sphere, 400.0f, 90.0f, 2);
    Skill->SelfEffectDuration = 6.0f;

    // Bare character: the authored numbers are what the skill queries, so a stat wired to the wrong default would
    // move every skill in the game before a single item existed.
    TestEqual(TEXT("no gear queries the authored radius"), Skill->ResolveShape().Radius, 400.0f);
    TestEqual(TEXT("no gear queries the authored cap"), Skill->ResolveShape().MaxTargets, 2);
    TestEqual(TEXT("no gear keeps the authored duration"), Skill->ResolveSelfEffectDuration(), 6.0f);

    ASC->SetNumericAttributeBase(RadiusBonus, 50.0f);
    TestEqual(TEXT("radius gear widens the query"), Skill->ResolveShape().Radius, 450.0f);
    // Each bonus moves one quantifier. Crossed wiring would show up nowhere else.
    TestEqual(TEXT("and leaves the cap alone"), Skill->ResolveShape().MaxTargets, 2);
    TestEqual(TEXT("and the duration alone"), Skill->ResolveSelfEffectDuration(), 6.0f);

    ASC->SetNumericAttributeBase(RadiusBonus, -1000.0f);
    TestEqual(TEXT("cursed radius gear shrinks the skill rather than inverting it"), Skill->ResolveShape().Radius, 0.0f);
    ASC->SetNumericAttributeBase(RadiusBonus, 0.0f);

    ASC->SetNumericAttributeBase(CountBonus, 3.0f);
    TestEqual(TEXT("target-count gear raises the queried cap"), Skill->ResolveShape().MaxTargets, 5);
    TestEqual(TEXT("and leaves the radius alone"), Skill->ResolveShape().Radius, 400.0f);

    Skill->Shape.MaxTargets = 0;
    TestEqual(TEXT("an uncapped skill stays uncapped however much count gear is worn"), Skill->ResolveShape().MaxTargets, 0);

    // A thrust is one target because it is a thrust, whatever the asset typed - and count gear is what turns it
    // into a piercing thrust.
    Skill->Shape.Shape = EMythicSkillShape::Single;
    Skill->Shape.MaxTargets = 0;
    TestEqual(TEXT("a single-target skill wearing +3 count pierces to four"), Skill->ResolveShape().MaxTargets, 4);
    ASC->SetNumericAttributeBase(CountBonus, 0.0f);
    TestEqual(TEXT("and hits exactly one with no gear on"), Skill->ResolveShape().MaxTargets, 1);

    ASC->SetNumericAttributeBase(DurationBonus, 2.0f);
    TestEqual(TEXT("duration gear lengthens the stance"), Skill->ResolveSelfEffectDuration(), 8.0f);
    ASC->SetNumericAttributeBase(DurationBonus, -10.0f);
    const float Cursed = Skill->ResolveSelfEffectDuration();
    TestTrue(TEXT("cursed duration gear cannot invert the stance"), Cursed > 0.0f);
    TestTrue(TEXT("but it does genuinely shorten it"), Cursed < 6.0f);

    // Single narrows by count, never by geometry: it holds everything a sphere would, and the cap is what leaves
    // one standing.
    const FMythicSkillShape Resolved = Skill->ResolveShape();
    const FVector Origin = FVector::ZeroVector;
    const FVector Forward = FVector::ForwardVector;
    TestTrue(TEXT("a single-target shape still holds what stands behind the caster"),
             FMythicSkillTargeting::IsInside(Resolved, Origin, Forward, FVector(-200, 0, 0)));
    const TArray<FVector> Crowd = {FVector(300, 0, 0), FVector(100, 0, 0), FVector(-50, 0, 0), FVector(200, 0, 0)};
    TArray<int32> Selected;
    FMythicSkillTargeting::SelectTargets(Resolved, Origin, Forward, Crowd, Selected);
    AddInfo(FString::Printf(TEXT("single-target skill among %d candidates, all in reach: %d taken"), Crowd.Num(), Selected.Num()));
    if (!TestEqual(TEXT("a single-target skill takes one of the four"), Selected.Num(), 1)) {
        return false;
    }
    TestEqual(TEXT("and it is the nearest"), Selected[0], 2);

    return true;
}

#endif
