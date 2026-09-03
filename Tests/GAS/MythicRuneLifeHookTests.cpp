
#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameplayEffect.h"
#include "Misc/ScopeExit.h"
#include "ScalableFloat.h"
#include "TimerManager.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicTags_GAS.h"
#include "Settings/MythicCombatSettings.h"

namespace {

// A character carrying its own ability system, so the life component sees the same authority a server pawn does.
struct FMythicLifeHookFixture {
    UGameInstance *GameInstance = nullptr;
    UWorld *World = nullptr;
    ACharacter *Pawn = nullptr;
    UMythicAbilitySystemComponent *ASC = nullptr;
    UMythicAttributeSet_Life *LifeSet = nullptr;
    UMythicLifeComponent *Life = nullptr;
};

bool BuildLifeHookFixture(FAutomationTestBase &Test, FMythicLifeHookFixture &Out, const bool bPlayerControlled) {
    if (!Test.TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }
    Out.GameInstance = NewObject<UGameInstance>(GEngine);
    Out.GameInstance->InitializeStandalone();
    Out.World = Out.GameInstance->GetWorld();
    if (!Test.TestNotNull(TEXT("standalone world exists"), Out.World)) {
        return false;
    }
    Out.Pawn = Out.World->SpawnActor<ACharacter>();
    if (!Test.TestNotNull(TEXT("the character spawned"), Out.Pawn)) {
        return false;
    }
    if (bPlayerControlled) {
        APlayerController *Controller = Out.World->SpawnActor<APlayerController>();
        if (!Test.TestNotNull(TEXT("the player controller spawned"), Controller)) {
            return false;
        }
        // A controller spawned without a GameMode never gets a PlayerState, and IsPlayerControlled reads it.
        APlayerState *PlayerState = Out.World->SpawnActor<APlayerState>();
        if (!Test.TestNotNull(TEXT("the player state spawned"), PlayerState)) {
            return false;
        }
        Controller->PlayerState = PlayerState;
        Controller->Possess(Out.Pawn);
        if (!Test.TestTrue(TEXT("the character is player controlled"), Out.Pawn->IsPlayerControlled())) {
            return false;
        }
    }

    Out.ASC = NewObject<UMythicAbilitySystemComponent>(Out.Pawn);
    Out.ASC->RegisterComponent();
    Out.ASC->InitAbilityActorInfo(Out.Pawn, Out.Pawn);
    Out.LifeSet = NewObject<UMythicAttributeSet_Life>(Out.Pawn);
    Out.ASC->AddAttributeSetSubobject(Out.LifeSet);
    Out.ASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Defense>(Out.Pawn));
    if (!Test.TestTrue(TEXT("the ability system answers as authority"), Out.ASC->IsOwnerActorAuthoritative())) {
        return false;
    }

    Out.Life = NewObject<UMythicLifeComponent>(Out.Pawn);
    Out.Life->RegisterComponent();
    Out.Life->InitializeWithAbilitySystem(Out.ASC);
    if (!Test.TestTrue(TEXT("the life component initialised"), Out.Life->IsInitialized())) {
        return false;
    }
    return Test.TestEqual(TEXT("the character starts at full health"), Out.Life->GetHealth(), Out.Life->GetMaxHealth());
}

void ApplyInstantModifier(UAbilitySystemComponent *ASC, const FGameplayAttribute &Attribute, const float Amount) {
    UGameplayEffect *Effect = NewObject<UGameplayEffect>(GetTransientPackage());
    Effect->DurationPolicy = EGameplayEffectDurationType::Instant;

    FGameplayModifierInfo Mod;
    Mod.Attribute = Attribute;
    Mod.ModifierOp = EGameplayModOp::Additive;
    Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Amount));
    Effect->Modifiers.Add(Mod);

    ASC->ApplyGameplayEffectToSelf(Effect, 1.0f, ASC->MakeEffectContext());
}

void ApplyLifeHookTestDamage(UAbilitySystemComponent *ASC, const float Amount) {
    ApplyInstantModifier(ASC, UMythicAttributeSet_Life::GetDamageAttribute(), Amount);
}

// Fall damage and hazards write Health itself, skipping the Damage meta attribute.
void ApplyDirectHealthLoss(UAbilitySystemComponent *ASC, const float Amount) {
    ApplyInstantModifier(ASC, UMythicAttributeSet_Life::GetHealthAttribute(), -Amount);
}

// A whole test body runs inside one engine frame, where the timer manager delivers nothing: it refuses a second
// tick per frame and only fires a timer once time has passed its expiry. Moving the frame counter and passing a
// delta just past one interval restores both, so every sample below is a real timer delivery.
void AdvanceOneSample(UWorld &World, const float IntervalSeconds) {
    ++GFrameCounter;
    World.GetTimerManager().Tick(IntervalSeconds * 1.01f);
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneDeathPreClaimSurvivesTest,
    "Mythic.GAS.RuneHooks.DeathPreClaimSurvives",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneDeathPreClaimSurvivesTest::RunTest(const FString &Parameters) {
    FMythicLifeHookFixture Fixture;
    const bool bReady = BuildLifeHookFixture(*this, Fixture, false);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicAbilitySystemComponent *ASC = Fixture.ASC;
    UMythicLifeComponent *Life = Fixture.Life;

    int32 Deaths = 0;
    Life->OnDeathNative.AddLambda([&Deaths](AActor *) { ++Deaths; });
    int32 PreCalls = 0;
    int32 PostCalls = 0;
    ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_DEATH_POST).AddLambda(
        [&PostCalls](const FGameplayEventData *) { ++PostCalls; });

    // What a cheat-death rune does inside Death.Pre: claim the death and put health back.
    const FDelegateHandle Claim = ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_DEATH_PRE).AddLambda(
        [ASC, Life, &PreCalls](const FGameplayEventData *) {
            ++PreCalls;
            ASC->AddLooseGameplayTag(GAS_PIPELINE_DEATH_HANDLED);
            Life->ServerSetHealthFraction(0.5f);
        });

    const float MaxHealth = Life->GetMaxHealth();
    ApplyLifeHookTestDamage(ASC, MaxHealth * 5.0f);
    TestEqual(TEXT("Death.Pre fired once for the lethal damage hit"), PreCalls, 1);
    TestEqual(TEXT("the claim restored health on the Damage path"), Life->GetHealth(), MaxHealth * 0.5f);
    TestFalse(TEXT("GAS.State.Dead stays unset on the Damage path"), ASC->HasMatchingGameplayTag(GAS_STATE_DEAD));
    TestFalse(TEXT("the death latch stays clear on the Damage path"), Fixture.LifeSet->IsDead());
    TestEqual(TEXT("OnDeath did not broadcast on the Damage path"), Deaths, 0);
    TestEqual(TEXT("Death.Post did not fire on the Damage path"), PostCalls, 0);
    TestFalse(TEXT("the pipeline cleared the Handled tag after the Damage path"),
              ASC->HasMatchingGameplayTag(GAS_PIPELINE_DEATH_HANDLED));

    // A later ordinary hit must still land, or the claim left the latch set and made the pawn invulnerable.
    ApplyLifeHookTestDamage(ASC, 20.0f);
    TestEqual(TEXT("an ordinary hit after the claim still takes health"), Life->GetHealth(), MaxHealth * 0.5f - 20.0f);

    ApplyDirectHealthLoss(ASC, MaxHealth * 5.0f);
    TestEqual(TEXT("Death.Pre fired for the direct Health loss too"), PreCalls, 2);
    TestEqual(TEXT("the claim restored health on the direct Health path"), Life->GetHealth(), MaxHealth * 0.5f);
    TestFalse(TEXT("GAS.State.Dead stays unset on the direct Health path"), ASC->HasMatchingGameplayTag(GAS_STATE_DEAD));
    TestFalse(TEXT("the death latch stays clear on the direct Health path"), Fixture.LifeSet->IsDead());
    TestEqual(TEXT("OnDeath did not broadcast on the direct Health path"), Deaths, 0);
    TestEqual(TEXT("Death.Post did not fire on the direct Health path"), PostCalls, 0);
    TestFalse(TEXT("the pipeline cleared the Handled tag after the direct Health path"),
              ASC->HasMatchingGameplayTag(GAS_PIPELINE_DEATH_HANDLED));

    // The claim only means something if the same hit kills without it.
    ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_DEATH_PRE).Remove(Claim);
    ApplyLifeHookTestDamage(ASC, MaxHealth * 5.0f);
    TestEqual(TEXT("without a listener the lethal hit empties health"), Life->GetHealth(), 0.0f);
    TestTrue(TEXT("without a listener GAS.State.Dead is set"), ASC->HasMatchingGameplayTag(GAS_STATE_DEAD));
    TestEqual(TEXT("without a listener OnDeath broadcasts once"), Deaths, 1);
    TestEqual(TEXT("Death.Post closes the pipeline"), PostCalls, 1);

    AddInfo(FString::Printf(TEXT("Death.Pre sends: %d, deaths prevented: 2, real deaths: %d"), PreCalls, Deaths));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneDeathHandledClearedTest,
    "Mythic.GAS.RuneHooks.DeathHandledCleared",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneDeathHandledClearedTest::RunTest(const FString &Parameters) {
    FMythicLifeHookFixture Fixture;
    const bool bReady = BuildLifeHookFixture(*this, Fixture, false);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicAbilitySystemComponent *ASC = Fixture.ASC;
    UMythicLifeComponent *Life = Fixture.Life;

    int32 Deaths = 0;
    Life->OnDeathNative.AddLambda([&Deaths](AActor *) { ++Deaths; });

    // A claim that restores nothing still has to leave the pawn alive: the pipeline owes it the one-point floor.
    const FDelegateHandle Claim = ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_DEATH_PRE).AddLambda(
        [ASC](const FGameplayEventData *) { ASC->AddLooseGameplayTag(GAS_PIPELINE_DEATH_HANDLED); });

    const float MaxHealth = Life->GetMaxHealth();
    ApplyLifeHookTestDamage(ASC, MaxHealth * 5.0f);
    TestEqual(TEXT("a bare claim leaves the pawn at the one-point floor"), Life->GetHealth(), 1.0f);
    TestFalse(TEXT("a bare claim keeps the pawn alive"), ASC->HasMatchingGameplayTag(GAS_STATE_DEAD));
    TestEqual(TEXT("OnDeath did not broadcast"), Deaths, 0);
    TestFalse(TEXT("the Handled tag is consumed by the pipeline"), ASC->HasMatchingGameplayTag(GAS_PIPELINE_DEATH_HANDLED));

    // The tag does not linger: a second lethal hit with nobody listening is a real death.
    ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_DEATH_PRE).Remove(Claim);
    ApplyLifeHookTestDamage(ASC, MaxHealth * 5.0f);
    TestEqual(TEXT("the second lethal hit empties health"), Life->GetHealth(), 0.0f);
    TestTrue(TEXT("the second lethal hit sets GAS.State.Dead"), ASC->HasMatchingGameplayTag(GAS_STATE_DEAD));
    TestTrue(TEXT("the second lethal hit latches death"), Fixture.LifeSet->IsDead());
    TestEqual(TEXT("the second lethal hit broadcasts OnDeath once"), Deaths, 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneMovementSignalTest,
    "Mythic.GAS.RuneHooks.MovementSignal",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneMovementSignalTest::RunTest(const FString &Parameters) {
    FMythicLifeHookFixture Fixture;
    const bool bReady = BuildLifeHookFixture(*this, Fixture, true);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UWorld *World = Fixture.World;
    ACharacter *Pawn = Fixture.Pawn;
    UMythicAbilitySystemComponent *ASC = Fixture.ASC;
    UMythicLifeComponent *Life = Fixture.Life;

    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    const float Interval = Settings->MovementSampleIntervalSeconds;
    const float Threshold = Settings->MovingSpeedThresholdCmPerSec;
    const float Grace = Settings->StillGraceSeconds;
    if (!TestTrue(TEXT("the sample interval is authored"), Interval > 0.0f)
        || !TestTrue(TEXT("the still grace outlasts one sample"), Grace > Interval)) {
        return false;
    }

    float MovedTotal = 0.0f;
    int32 MovedRaises = 0;
    ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_MOVED).AddLambda(
        [&MovedTotal, &MovedRaises](const FGameplayEventData *Payload) {
            ++MovedRaises;
            MovedTotal += Payload ? Payload->EventMagnitude : 0.0f;
        });

    // A timer set before the manager has ever ticked stays pending until that first tick ends, so prime it once.
    AdvanceOneSample(*World, 0.0f);

    // Standing at spawn: the sampler records where the pawn is and says nothing.
    AdvanceOneSample(*World, Interval);
    TestFalse(TEXT("a pawn that has not moved is not Moving"), ASC->HasMatchingGameplayTag(GAS_STATE_MOVING));
    TestEqual(TEXT("no distance before moving"), Life->GetDistanceSinceStill(), 0.0f);
    TestTrue(TEXT("still time accrues while standing"), Life->GetSecondsStill() > 0.0f);

    // Travel: velocity says moving, and the location moves under it.
    constexpr float StepCm = 60.0f;
    constexpr int32 Steps = 5;
    Pawn->GetCharacterMovement()->Velocity = FVector(Threshold + StepCm / 0.2f, 0.0f, 0.0f);
    for (int32 Step = 0; Step < Steps; ++Step) {
        Pawn->SetActorLocation(Pawn->GetActorLocation() + FVector(StepCm, 0.0f, 0.0f));
        AdvanceOneSample(*World, Interval);
    }
    TestTrue(TEXT("travel past the threshold sets GAS.State.Moving"), ASC->HasMatchingGameplayTag(GAS_STATE_MOVING));
    TestEqual(TEXT("GAS.Event.Moved fired once per travelling sample"), MovedRaises, Steps);
    TestEqual(TEXT("GAS.Event.Moved carried the centimetres accrued"), MovedTotal, StepCm * Steps, 0.01f);
    TestEqual(TEXT("the odometer holds the same distance"), Life->GetDistanceSinceStill(), StepCm * Steps, 0.01f);
    TestEqual(TEXT("seconds still reads 0 while moving"), Life->GetSecondsStill(), 0.0f);

    // A pause shorter than the grace is still one journey.
    Pawn->GetCharacterMovement()->Velocity = FVector::ZeroVector;
    AdvanceOneSample(*World, Interval);
    TestTrue(TEXT("one still sample keeps GAS.State.Moving through the grace"), ASC->HasMatchingGameplayTag(GAS_STATE_MOVING));
    TestEqual(TEXT("a short pause keeps the odometer"), Life->GetDistanceSinceStill(), StepCm * Steps, 0.01f);
    TestTrue(TEXT("seconds still counts during the grace"), Life->GetSecondsStill() > 0.0f);

    Pawn->GetCharacterMovement()->Velocity = FVector(Threshold + StepCm / 0.2f, 0.0f, 0.0f);
    Pawn->SetActorLocation(Pawn->GetActorLocation() + FVector(0.0f, StepCm, 0.0f));
    AdvanceOneSample(*World, Interval);
    TestEqual(TEXT("moving again keeps accruing on the same journey"), Life->GetDistanceSinceStill(), StepCm * (Steps + 1), 0.01f);
    TestEqual(TEXT("seconds still resets on the first travelling sample"), Life->GetSecondsStill(), 0.0f);

    // Standing past the grace ends the journey.
    Pawn->GetCharacterMovement()->Velocity = FVector::ZeroVector;
    const int32 MaxStillSamples = FMath::CeilToInt(Grace / Interval) + 3;
    int32 StillSamples = 0;
    while (ASC->HasMatchingGameplayTag(GAS_STATE_MOVING) && StillSamples < MaxStillSamples) {
        AdvanceOneSample(*World, Interval);
        ++StillSamples;
    }
    TestFalse(TEXT("standing past the grace clears GAS.State.Moving"), ASC->HasMatchingGameplayTag(GAS_STATE_MOVING));
    TestTrue(TEXT("the tag did not clear before the grace elapsed"), StillSamples * Interval >= Grace - Interval * 0.5f);
    TestTrue(TEXT("the tag cleared on the first sample past the grace"), StillSamples * Interval <= Grace + Interval * 1.5f);
    TestEqual(TEXT("clearing the tag resets the odometer"), Life->GetDistanceSinceStill(), 0.0f);
    TestTrue(TEXT("seconds still has reached the grace"), Life->GetSecondsStill() >= Grace - KINDA_SMALL_NUMBER);

    AddInfo(FString::Printf(TEXT("interval %.2fs, grace %.2fs, moved raises %d for %.0f cm, cleared after %d still samples"),
                            Interval, Grace, MovedRaises, MovedTotal, StillSamples));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneStillBeganEdgeTest,
    "Mythic.GAS.RuneHooks.StillBeganEdge",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneStillBeganEdgeTest::RunTest(const FString &Parameters) {
    FMythicLifeHookFixture Fixture;
    const bool bReady = BuildLifeHookFixture(*this, Fixture, true);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UWorld *World = Fixture.World;
    ACharacter *Pawn = Fixture.Pawn;
    UMythicAbilitySystemComponent *ASC = Fixture.ASC;
    UMythicLifeComponent *Life = Fixture.Life;

    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    const float Interval = Settings->MovementSampleIntervalSeconds;
    const float Threshold = Settings->MovingSpeedThresholdCmPerSec;
    const float Grace = Settings->StillGraceSeconds;
    if (!TestTrue(TEXT("the sample interval is authored"), Interval > 0.0f)
        || !TestTrue(TEXT("the still grace outlasts one sample"), Grace > Interval)) {
        return false;
    }

    int32 StillBegan = 0;
    Life->OnStillBegan.AddLambda([&StillBegan]() { ++StillBegan; });

    constexpr float StepCm = 60.0f;
    const FVector TravelVelocity(Threshold + StepCm / 0.2f, 0.0f, 0.0f);
    auto Travel = [&](const int32 Samples) {
        Pawn->GetCharacterMovement()->Velocity = TravelVelocity;
        for (int32 Step = 0; Step < Samples; ++Step) {
            Pawn->SetActorLocation(Pawn->GetActorLocation() + FVector(StepCm, 0.0f, 0.0f));
            AdvanceOneSample(*World, Interval);
        }
    };
    auto Stand = [&](const int32 Samples) {
        Pawn->GetCharacterMovement()->Velocity = FVector::ZeroVector;
        for (int32 Step = 0; Step < Samples; ++Step) {
            AdvanceOneSample(*World, Interval);
        }
    };

    AdvanceOneSample(*World, 0.0f);
    Stand(3);
    TestEqual(TEXT("standing at spawn is not a stop: nothing has travelled yet"), StillBegan, 0);

    Travel(3);
    TestEqual(TEXT("travel raises no still edge"), StillBegan, 0);

    Stand(1);
    TestEqual(TEXT("the first still sample after travel raises OnStillBegan"), StillBegan, 1);
    TestTrue(TEXT("the edge fires while GAS.State.Moving still holds through the grace"), ASC->HasMatchingGameplayTag(GAS_STATE_MOVING));
    TestTrue(TEXT("the edge fires before the grace has elapsed"), Life->GetSecondsStill() < Grace);

    const int32 GraceSamples = FMath::CeilToInt(Grace / Interval) + 3;
    Stand(GraceSamples);
    TestFalse(TEXT("standing past the grace clears GAS.State.Moving"), ASC->HasMatchingGameplayTag(GAS_STATE_MOVING));
    TestEqual(TEXT("the grace resolving does not raise a second edge"), StillBegan, 1);
    Stand(3);
    TestEqual(TEXT("standing on after the grace raises nothing"), StillBegan, 1);

    // A stutter-step inside the grace is one journey but two stops: the rune hears every real halt.
    Travel(2);
    Stand(1);
    TestEqual(TEXT("the second stop raises its own edge"), StillBegan, 2);
    Travel(1);
    Stand(1);
    TestEqual(TEXT("a halt inside the grace after travel resumed is a new stop"), StillBegan, 3);
    Stand(GraceSamples);
    TestEqual(TEXT("the grace resolving still adds nothing"), StillBegan, 3);

    AddInfo(FString::Printf(TEXT("interval %.2fs, grace %.2fs, still edges %d for 3 stops"), Interval, Grace, StillBegan));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneFallDepthTest,
    "Mythic.GAS.RuneHooks.FallDepth",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneFallDepthTest::RunTest(const FString &Parameters) {
    FMythicLifeHookFixture Fixture;
    const bool bReady = BuildLifeHookFixture(*this, Fixture, true);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UWorld *World = Fixture.World;
    ACharacter *Pawn = Fixture.Pawn;
    UMythicLifeComponent *Life = Fixture.Life;
    UCharacterMovementComponent *Move = Pawn->GetCharacterMovement();

    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    const float Interval = Settings->MovementSampleIntervalSeconds;
    if (!TestTrue(TEXT("the sample interval is authored"), Interval > 0.0f)) {
        return false;
    }

    int32 DepthSamples = 0;
    float LastDepth = -1.0f;
    Life->OnFallDepthSampled.AddLambda([&DepthSamples, &LastDepth](const float Metres) {
        ++DepthSamples;
        LastDepth = Metres;
    });

    AdvanceOneSample(*World, 0.0f);
    Move->SetMovementMode(MOVE_Walking);
    AdvanceOneSample(*World, Interval);
    TestEqual(TEXT("no depth sample on the ground"), DepthSamples, 0);
    TestEqual(TEXT("no depth on the ground"), Life->GetFallDepthMetres(), 0.0f);

    // Leaving the ground: the first falling sample latches where the fall began.
    Move->SetMovementMode(MOVE_Falling);
    if (!TestTrue(TEXT("the movement component reports Falling"), Move->IsFalling())) {
        return false;
    }
    const FVector Start = Pawn->GetActorLocation();
    AdvanceOneSample(*World, Interval);
    TestEqual(TEXT("the first falling sample fires the depth delegate"), DepthSamples, 1);
    TestEqual(TEXT("the first falling sample reads no depth"), LastDepth, 0.0f, 0.001f);

    // The rise of a jump is not depth, and the top of the arc becomes the new start.
    Pawn->SetActorLocation(Start + FVector(0.0f, 0.0f, 120.0f));
    AdvanceOneSample(*World, Interval);
    TestEqual(TEXT("rising reads no depth"), LastDepth, 0.0f, 0.001f);
    TestEqual(TEXT("rising still counts as a falling sample"), DepthSamples, 2);

    Pawn->SetActorLocation(Start - FVector(0.0f, 0.0f, 280.0f));
    AdvanceOneSample(*World, Interval);
    TestEqual(TEXT("depth is measured from the top of the arc"), LastDepth, 4.0f, 0.001f);
    TestEqual(TEXT("the getter agrees with the sample"), Life->GetFallDepthMetres(), 4.0f, 0.001f);

    // A landing listener runs between samples, while the mode is still Falling, and must see the whole drop.
    Pawn->SetActorLocation(Start - FVector(0.0f, 0.0f, 530.0f));
    TestEqual(TEXT("the getter reads live between samples"), Life->GetFallDepthMetres(), 6.5f, 0.001f);
    AdvanceOneSample(*World, Interval);
    TestEqual(TEXT("the next sample carries the live depth"), LastDepth, 6.5f, 0.001f);
    TestEqual(TEXT("one depth sample per falling sample"), DepthSamples, 4);

    // Landing resets before anything else runs.
    Move->SetMovementMode(MOVE_Walking);
    TestEqual(TEXT("landing resets the depth at once"), Life->GetFallDepthMetres(), 0.0f);
    AdvanceOneSample(*World, Interval);
    AdvanceOneSample(*World, Interval);
    TestEqual(TEXT("no depth samples on the ground after landing"), DepthSamples, 4);

    // A second fall measures from its own edge, not from the last fall.
    Move->SetMovementMode(MOVE_Falling);
    AdvanceOneSample(*World, Interval);
    Pawn->SetActorLocation(Pawn->GetActorLocation() - FVector(0.0f, 0.0f, 150.0f));
    AdvanceOneSample(*World, Interval);
    TestEqual(TEXT("the second fall measures only its own drop"), LastDepth, 1.5f, 0.001f);
    TestEqual(TEXT("the second fall fires per sample"), DepthSamples, 6);
    Move->SetMovementMode(MOVE_Walking);
    TestEqual(TEXT("the second landing resets too"), Life->GetFallDepthMetres(), 0.0f);

    AddInfo(FString::Printf(TEXT("interval %.2fs, depth samples %d across 2 falls, deepest 6.5 m"), Interval, DepthSamples));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneCreditedTravelTest,
    "Mythic.GAS.RuneHooks.CreditedTravel",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneCreditedTravelTest::RunTest(const FString &Parameters) {
    FMythicLifeHookFixture Fixture;
    const bool bReady = BuildLifeHookFixture(*this, Fixture, true);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UWorld *World = Fixture.World;
    ACharacter *Pawn = Fixture.Pawn;
    UMythicAbilitySystemComponent *ASC = Fixture.ASC;
    UMythicLifeComponent *Life = Fixture.Life;

    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    const float Interval = Settings->MovementSampleIntervalSeconds;
    const float Threshold = Settings->MovingSpeedThresholdCmPerSec;
    const float Grace = Settings->StillGraceSeconds;
    if (!TestTrue(TEXT("the sample interval is authored"), Interval > 0.0f)
        || !TestTrue(TEXT("the still grace outlasts one sample"), Grace > Interval)) {
        return false;
    }

    int32 MovedRaises = 0;
    float LastMoved = 0.0f;
    ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_MOVED).AddLambda(
        [&MovedRaises, &LastMoved](const FGameplayEventData *Payload) {
            ++MovedRaises;
            LastMoved = Payload ? Payload->EventMagnitude : 0.0f;
        });

    AdvanceOneSample(*World, 0.0f);
    AdvanceOneSample(*World, Interval);

    constexpr float CreditCm = 250.0f;
    Life->AddDistanceSinceStill(CreditCm);
    TestEqual(TEXT("credited travel is visible to the next read"), Life->GetDistanceSinceStill(), CreditCm, 0.01f);
    TestEqual(TEXT("credited travel raises GAS.Event.Moved once"), MovedRaises, 1);
    TestEqual(TEXT("the raise carries the credited centimetres"), LastMoved, CreditCm, 0.01f);
    TestFalse(TEXT("a credit is not a velocity: GAS.State.Moving stays off"), ASC->HasMatchingGameplayTag(GAS_STATE_MOVING));

    Life->AddDistanceSinceStill(-50.0f);
    Life->AddDistanceSinceStill(0.0f);
    TestEqual(TEXT("a non-positive credit changes nothing"), Life->GetDistanceSinceStill(), CreditCm, 0.01f);
    TestEqual(TEXT("a non-positive credit raises nothing"), MovedRaises, 1);

    // Real travel stacks on the credit inside one journey.
    constexpr float StepCm = 60.0f;
    Pawn->GetCharacterMovement()->Velocity = FVector(Threshold + StepCm / 0.2f, 0.0f, 0.0f);
    Pawn->SetActorLocation(Pawn->GetActorLocation() + FVector(StepCm, 0.0f, 0.0f));
    AdvanceOneSample(*World, Interval);
    TestEqual(TEXT("sampled travel adds to the credit"), Life->GetDistanceSinceStill(), CreditCm + StepCm, 0.01f);
    TestEqual(TEXT("sampled travel raises its own event"), MovedRaises, 2);
    TestEqual(TEXT("the sampled raise carries only the sampled step"), LastMoved, StepCm, 0.01f);

    Life->AddDistanceSinceStill(CreditCm);
    TestEqual(TEXT("a credit mid-journey stacks"), Life->GetDistanceSinceStill(), CreditCm * 2.0f + StepCm, 0.01f);

    // The credit belongs to the journey and goes with it.
    Pawn->GetCharacterMovement()->Velocity = FVector::ZeroVector;
    const int32 MaxStillSamples = FMath::CeilToInt(Grace / Interval) + 3;
    for (int32 Step = 0; Step < MaxStillSamples && ASC->HasMatchingGameplayTag(GAS_STATE_MOVING); ++Step) {
        AdvanceOneSample(*World, Interval);
    }
    TestFalse(TEXT("standing past the grace clears GAS.State.Moving"), ASC->HasMatchingGameplayTag(GAS_STATE_MOVING));
    TestEqual(TEXT("the grace resolving drops the credit with the odometer"), Life->GetDistanceSinceStill(), 0.0f);

    AddInfo(FString::Printf(TEXT("credits 2 x %.0f cm, sampled %.0f cm, moved raises %d"), CreditCm, StepCm, MovedRaises));
    return true;
}
