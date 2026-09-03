#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffectTypes.h"
#include "Misc/ScopeExit.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "GAS/Effects/MythicCrowdControl.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "GAS/MythicTags_GAS.h"
#include "Tests/GAS/MythicFallDamageTestTypes.h"

namespace {

struct FMythicFallDamageWorldFixture {
    UGameInstance *GameInstance = nullptr;
    UWorld *World = nullptr;
};

bool BuildFallDamageWorld(FAutomationTestBase &Test, FMythicFallDamageWorldFixture &Out) {
    if (!Test.TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }
    Out.GameInstance = NewObject<UGameInstance>(GEngine);
    Out.GameInstance->InitializeStandalone();
    Out.World = Out.GameInstance->GetWorld();
    return Test.TestNotNull(TEXT("standalone world exists"), Out.World);
}

constexpr float FallDamagePawnMaxHealth = 1000.0f;

AMythicFallDamageTestCharacter *SpawnFallDamagePawn(UWorld *World) {
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AMythicFallDamageTestCharacter *Pawn = World->SpawnActor<AMythicFallDamageTestCharacter>(
        AMythicFallDamageTestCharacter::StaticClass(), FTransform::Identity, Params);
    if (!Pawn) {
        return nullptr;
    }
    UMythicAbilitySystemComponent *ASC = Pawn->AbilitySystem;
    // Production worlds call InitializeComponent before BeginPlay; that is the step that registers the pawn's
    // default-subobject attribute sets with its ability system.
    if (!ASC->HasBeenInitialized()) {
        ASC->InitializeComponent();
    }
    ASC->InitAbilityActorInfo(Pawn, Pawn);
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetMaxHealthAttribute(), FallDamagePawnMaxHealth);
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(), FallDamagePawnMaxHealth);
    Pawn->LifeAttributes->RefreshOutOfHealthLatch();
    Pawn->Life->InitializeWithAbilitySystem(ASC);
    return Pawn;
}

float FallDamageHealthOf(const AMythicFallDamageTestCharacter *Pawn) {
    return Pawn->AbilitySystem->GetNumericAttribute(UMythicAttributeSet_Life::GetHealthAttribute());
}

float FallDamageTakenOf(const AMythicFallDamageTestCharacter *Pawn) {
    return Pawn->AbilitySystem->GetNumericAttribute(UMythicAttributeSet_Defense::GetFallDamageTakenAttribute());
}

void SetFallDamageTaken(AMythicFallDamageTestCharacter *Pawn, const float Multiplier) {
    Pawn->AbilitySystem->SetNumericAttributeBase(UMythicAttributeSet_Defense::GetFallDamageTakenAttribute(), Multiplier);
}

// The plain number a landing at ImpactSpeed costs this pawn, straight from the authored curve.
float PlainFallDamageFor(const AMythicFallDamageTestCharacter *Pawn, const float ImpactSpeed) {
    return AMythicCharacter::ComputeFallDamage(ImpactSpeed, Pawn->GetSafeFallSpeed(), Pawn->GetFallDamagePerSpeed(),
                                               Pawn->GetMaxFallDamage());
}

void LandFallDamagePawn(AMythicFallDamageTestCharacter *Pawn, const float ImpactSpeed) {
    Pawn->GetCharacterMovement()->Velocity = FVector(0.0, 0.0, -ImpactSpeed);
    Pawn->Landed(FHitResult());
}

// What the last OnFallDamageResolved broadcast said.
struct FMythicFallDamageResolvedRecord {
    int32 Count = 0;
    float ImpactSpeed = 0.0f;
    float Damage = 0.0f;
    bool bPrevented = false;
};

FDelegateHandle BindFallDamageResolved(AMythicFallDamageTestCharacter *Pawn, FMythicFallDamageResolvedRecord &Record) {
    return Pawn->OnFallDamageResolved.AddLambda([&Record](const float ImpactSpeed, const float Damage, const bool bPrevented) {
        Record.Count++;
        Record.ImpactSpeed = ImpactSpeed;
        Record.Damage = Damage;
        Record.bPrevented = bPrevented;
    });
}

// What the last Dmg.Received payload on the pawn carried.
struct FMythicFallDamageReceivedRecord {
    int32 Count = 0;
    float Magnitude = 0.0f;
    FGameplayTagContainer InstigatorTags;
    FGameplayEffectContextHandle Context;
    TWeakObjectPtr<const AActor> Instigator;
    TWeakObjectPtr<const UObject> Effect;
};

FDelegateHandle BindFallDamageReceived(AMythicFallDamageTestCharacter *Pawn, FMythicFallDamageReceivedRecord &Record) {
    return Pawn->AbilitySystem->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_DMG_RECEIVED).AddLambda(
        [&Record](const FGameplayEventData *Payload) {
            Record.Count++;
            if (!Payload) {
                return;
            }
            Record.Magnitude = Payload->EventMagnitude;
            Record.InstigatorTags = Payload->InstigatorTags;
            Record.Context = Payload->ContextHandle;
            Record.Instigator = Payload->Instigator.Get();
            Record.Effect = Payload->OptionalObject.Get();
        });
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicFallDamageTakenZeroPreventsTest,
    "Mythic.GAS.FallDamage.TakenZeroPrevents",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicFallDamageTakenZeroPreventsTest::RunTest(const FString &Parameters) {
    FMythicFallDamageWorldFixture Fixture;
    const bool bReady = BuildFallDamageWorld(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }

    AMythicFallDamageTestCharacter *Feathered = SpawnFallDamagePawn(Fixture.World);
    AMythicFallDamageTestCharacter *Control = SpawnFallDamagePawn(Fixture.World);
    if (!TestNotNull(TEXT("the feathered pawn spawned"), Feathered) || !TestNotNull(TEXT("the control pawn spawned"), Control)) {
        return false;
    }
    TestEqual(TEXT("FallDamageTaken starts at the plain baseline"), FallDamageTakenOf(Feathered), 1.0f);

    const float Impact = Feathered->GetSafeFallSpeed() + 1000.0f;
    const float Expected = PlainFallDamageFor(Feathered, Impact);
    if (!TestTrue(TEXT("the landing under test would hurt a plain pawn"), Expected > 0.0f && Expected < FallDamagePawnMaxHealth)) {
        return false;
    }

    SetFallDamageTaken(Feathered, 0.0f);
    if (!TestEqual(TEXT("FallDamageTaken reads 0"), FallDamageTakenOf(Feathered), 0.0f)) {
        return false;
    }

    FMythicFallDamageResolvedRecord FeatheredResolved;
    FMythicFallDamageResolvedRecord ControlResolved;
    FMythicFallDamageReceivedRecord FeatheredReceived;
    BindFallDamageResolved(Feathered, FeatheredResolved);
    BindFallDamageResolved(Control, ControlResolved);
    BindFallDamageReceived(Feathered, FeatheredReceived);

    LandFallDamagePawn(Feathered, Impact);
    LandFallDamagePawn(Control, Impact);

    TestEqual(TEXT("the feathered landing still resolved once"), FeatheredResolved.Count, 1);
    TestTrue(TEXT("and was reported as prevented"), FeatheredResolved.bPrevented);
    TestEqual(TEXT("with no damage to apply"), FeatheredResolved.Damage, 0.0f);
    TestEqual(TEXT("and the impact speed it landed at"), FeatheredResolved.ImpactSpeed, Impact, 0.01f);
    TestEqual(TEXT("the feathered pawn lost no health"), FallDamageHealthOf(Feathered), FallDamagePawnMaxHealth);
    TestEqual(TEXT("no Dmg.Received reached the feathered pawn"), FeatheredReceived.Count, 0);
    TestEqual(TEXT("the fall hooks were not run for a landing with nothing to apply"), Feathered->ComputedHookCalls, 0);

    // The denominator: the same landing on the same pawn at the baseline is a hit.
    TestEqual(TEXT("the control landing resolved once"), ControlResolved.Count, 1);
    TestFalse(TEXT("and was applied"), ControlResolved.bPrevented);
    TestEqual(TEXT("at the plain damage"), ControlResolved.Damage, Expected, 0.01f);
    TestEqual(TEXT("the control pawn lost the plain damage"), FallDamageHealthOf(Control), FallDamagePawnMaxHealth - Expected, 0.01f);

    // The attribute cannot go negative and turn a landing into a heal.
    SetFallDamageTaken(Feathered, -0.5f);
    TestEqual(TEXT("a negative FallDamageTaken clamps to 0"), FallDamageTakenOf(Feathered), 0.0f);

    AddInfo(FString::Printf(TEXT("impact %.0f cm/s, plain damage %.2f, feathered damage %.2f, control health %.1f"),
                            Impact, Expected, FeatheredResolved.Damage, FallDamageHealthOf(Control)));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicFallDamageTakenHalvesDamageTest,
    "Mythic.GAS.FallDamage.TakenHalvesDamage",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicFallDamageTakenHalvesDamageTest::RunTest(const FString &Parameters) {
    FMythicFallDamageWorldFixture Fixture;
    const bool bReady = BuildFallDamageWorld(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }

    AMythicFallDamageTestCharacter *Halved = SpawnFallDamagePawn(Fixture.World);
    AMythicFallDamageTestCharacter *Control = SpawnFallDamagePawn(Fixture.World);
    if (!TestNotNull(TEXT("the halved pawn spawned"), Halved) || !TestNotNull(TEXT("the control pawn spawned"), Control)) {
        return false;
    }

    const float Impact = Halved->GetSafeFallSpeed() + 1000.0f;
    const float Expected = PlainFallDamageFor(Halved, Impact);
    if (!TestTrue(TEXT("the landing under test would hurt a plain pawn"), Expected > 0.0f && Expected < FallDamagePawnMaxHealth)) {
        return false;
    }

    SetFallDamageTaken(Halved, 0.5f);
    if (!TestEqual(TEXT("FallDamageTaken reads 0.5"), FallDamageTakenOf(Halved), 0.5f)) {
        return false;
    }

    FMythicFallDamageResolvedRecord HalvedResolved;
    FMythicFallDamageReceivedRecord HalvedReceived;
    BindFallDamageResolved(Halved, HalvedResolved);
    BindFallDamageReceived(Halved, HalvedReceived);

    LandFallDamagePawn(Halved, Impact);
    LandFallDamagePawn(Control, Impact);

    TestEqual(TEXT("the halved landing resolved once"), HalvedResolved.Count, 1);
    TestFalse(TEXT("and was applied"), HalvedResolved.bPrevented);
    TestEqual(TEXT("at half the plain damage"), HalvedResolved.Damage, Expected * 0.5f, 0.01f);
    TestEqual(TEXT("the halved pawn lost half the plain damage"), FallDamageHealthOf(Halved), FallDamagePawnMaxHealth - Expected * 0.5f, 0.01f);
    TestEqual(TEXT("Dmg.Received carried the halved number"), HalvedReceived.Magnitude, Expected * 0.5f, 0.01f);

    // The multiply happens before the hooks, so a Blueprint's last word sees the number the attribute made.
    TestEqual(TEXT("the fall hook ran once"), Halved->ComputedHookCalls, 1);
    TestEqual(TEXT("and was handed the halved damage"), Halved->LastHookDamage, Expected * 0.5f, 0.01f);

    // The denominator: the baseline pawn lost the whole number from the identical landing.
    TestEqual(TEXT("the control pawn lost the plain damage"), FallDamageHealthOf(Control), FallDamagePawnMaxHealth - Expected, 0.01f);
    const float HalvedLoss = FallDamagePawnMaxHealth - FallDamageHealthOf(Halved);
    const float ControlLoss = FallDamagePawnMaxHealth - FallDamageHealthOf(Control);
    TestEqual(TEXT("the ratio between the two losses is the attribute"), HalvedLoss / ControlLoss, 0.5f, 0.001f);

    AddInfo(FString::Printf(TEXT("impact %.0f cm/s, plain damage %.2f, halved loss %.2f, control loss %.2f"),
                            Impact, Expected, HalvedLoss, ControlLoss));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicFallDamageImmuneTagReportsComputedTest,
    "Mythic.GAS.FallDamage.ImmuneTagReportsComputed",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicFallDamageImmuneTagReportsComputedTest::RunTest(const FString &Parameters) {
    FMythicFallDamageWorldFixture Fixture;
    const bool bReady = BuildFallDamageWorld(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }

    AMythicFallDamageTestCharacter *Pawn = SpawnFallDamagePawn(Fixture.World);
    if (!TestNotNull(TEXT("the pawn spawned"), Pawn)) {
        return false;
    }

    const float Impact = Pawn->GetSafeFallSpeed() + 1000.0f;
    const float Expected = PlainFallDamageFor(Pawn, Impact);
    if (!TestTrue(TEXT("the landing under test would hurt a plain pawn"), Expected > 0.0f && Expected < FallDamagePawnMaxHealth)) {
        return false;
    }

    FMythicFallDamageResolvedRecord Resolved;
    FMythicFallDamageReceivedRecord Received;
    BindFallDamageResolved(Pawn, Resolved);
    BindFallDamageReceived(Pawn, Received);

    Pawn->AbilitySystem->AddLooseGameplayTag(GAS_IMMUNE_FALLDAMAGE);
    if (!TestTrue(TEXT("the immunity tag is on the pawn"), Pawn->AbilitySystem->HasMatchingGameplayTag(GAS_IMMUNE_FALLDAMAGE))) {
        return false;
    }
    LandFallDamagePawn(Pawn, Impact);

    TestEqual(TEXT("the immune landing resolved once"), Resolved.Count, 1);
    TestTrue(TEXT("and was reported as prevented"), Resolved.bPrevented);
    TestEqual(TEXT("with the damage it refused, not zero"), Resolved.Damage, Expected, 0.01f);
    TestEqual(TEXT("and the impact speed"), Resolved.ImpactSpeed, Impact, 0.01f);
    TestEqual(TEXT("the immune pawn lost no health"), FallDamageHealthOf(Pawn), FallDamagePawnMaxHealth);
    TestEqual(TEXT("no Dmg.Received reached the immune pawn"), Received.Count, 0);
    TestEqual(TEXT("the fall hooks still priced the landing"), Pawn->ComputedHookCalls, 1);
    TestEqual(TEXT("and saw the plain damage"), Pawn->LastHookDamage, Expected, 0.01f);

    // Releasing the tag hands the rule back: the identical landing now lands.
    Pawn->AbilitySystem->RemoveLooseGameplayTag(GAS_IMMUNE_FALLDAMAGE);
    LandFallDamagePawn(Pawn, Impact);

    TestEqual(TEXT("the second landing resolved"), Resolved.Count, 2);
    TestFalse(TEXT("and was applied"), Resolved.bPrevented);
    TestEqual(TEXT("at the plain damage"), Resolved.Damage, Expected, 0.01f);
    TestEqual(TEXT("without the tag the same landing hurts"), FallDamageHealthOf(Pawn), FallDamagePawnMaxHealth - Expected, 0.01f);
    TestEqual(TEXT("and raises Dmg.Received once"), Received.Count, 1);

    AddInfo(FString::Printf(TEXT("impact %.0f cm/s, refused %.2f, then applied %.2f"), Impact, Expected, Resolved.Damage));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicFallDamagePlainLandingDamageMetaTest,
    "Mythic.GAS.FallDamage.PlainLandingRoutesThroughDamageMeta",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicFallDamagePlainLandingDamageMetaTest::RunTest(const FString &Parameters) {
    FMythicFallDamageWorldFixture Fixture;
    const bool bReady = BuildFallDamageWorld(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }

    AMythicFallDamageTestCharacter *Pawn = SpawnFallDamagePawn(Fixture.World);
    if (!TestNotNull(TEXT("the pawn spawned"), Pawn)) {
        return false;
    }

    FMythicFallDamageResolvedRecord Resolved;
    FMythicFallDamageReceivedRecord Received;
    BindFallDamageResolved(Pawn, Resolved);
    BindFallDamageReceived(Pawn, Received);

    // A harmless hop still reports, so a rune can read every landing; it just has nothing to apply.
    const float SoftImpact = Pawn->GetSafeFallSpeed() - 100.0f;
    LandFallDamagePawn(Pawn, SoftImpact);
    TestEqual(TEXT("a harmless landing still resolved"), Resolved.Count, 1);
    TestTrue(TEXT("and was reported as prevented"), Resolved.bPrevented);
    TestEqual(TEXT("with no damage"), Resolved.Damage, 0.0f);
    TestEqual(TEXT("and its impact speed"), Resolved.ImpactSpeed, SoftImpact, 0.01f);
    TestEqual(TEXT("a harmless landing costs no health"), FallDamageHealthOf(Pawn), FallDamagePawnMaxHealth);
    TestEqual(TEXT("and raises no Dmg.Received"), Received.Count, 0);
    TestEqual(TEXT("and runs no fall hook"), Pawn->ComputedHookCalls, 0);

    const float Impact = Pawn->GetSafeFallSpeed() + 1000.0f;
    const float Expected = PlainFallDamageFor(Pawn, Impact);
    if (!TestTrue(TEXT("the landing under test would hurt a plain pawn"), Expected > 0.0f && Expected < FallDamagePawnMaxHealth)) {
        return false;
    }
    LandFallDamagePawn(Pawn, Impact);

    TestEqual(TEXT("the hurting landing resolved"), Resolved.Count, 2);
    TestFalse(TEXT("and was applied"), Resolved.bPrevented);
    TestEqual(TEXT("at the plain damage"), Resolved.Damage, Expected, 0.01f);
    TestEqual(TEXT("the pawn lost exactly the resolved damage"), FallDamageHealthOf(Pawn), FallDamagePawnMaxHealth - Expected, 0.01f);
    TestEqual(TEXT("the Damage meta attribute was consumed"),
              Pawn->AbilitySystem->GetNumericAttribute(UMythicAttributeSet_Life::GetDamageAttribute()), 0.0f);

    // The damage pipeline heard it as a hit, so Dmg.Received, Death.Pre and combat text all get their turn.
    if (!TestEqual(TEXT("Dmg.Received fired once"), Received.Count, 1)) {
        return false;
    }
    TestEqual(TEXT("with the SetByCaller magnitude Landed wrote"), Received.Magnitude, Expected, 0.01f);
    TestTrue(TEXT("the payload's instigator tags carry GAS.Hit.Fall"), Received.InstigatorTags.HasTagExact(GAS_HIT_FALL));
    TestTrue(TEXT("the landing is self-inflicted"), Received.Instigator.Get() == Pawn);
    TestTrue(TEXT("the vehicle was the authored fall effect"),
             Received.Effect.IsValid() && Received.Effect->IsA(UMythicFallDamageTestEffect::StaticClass()));

    const FMythicGameplayEffectContext *Context = FMythicGameplayEffectContext::ExtractEffectContext(Received.Context);
    if (TestNotNull(TEXT("the fall context is a Mythic context"), Context)) {
        TestTrue(TEXT("stamped with GAS.Hit.Fall"), Context->GetHitTags().HasTagExact(GAS_HIT_FALL));
        TestEqual(TEXT("and with no other hit tag"), Context->GetHitTags().Num(), 1);
    }

    AddInfo(FString::Printf(TEXT("soft impact %.0f cm/s reported %.2f; hard impact %.0f cm/s applied %.2f through Dmg.Received %.2f"),
                            SoftImpact, 0.0f, Impact, Resolved.Damage, Received.Magnitude));
    return true;
}
