#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "Misc/ScopeExit.h"
#include "ScalableFloat.h"
#include "TimerManager.h"

#include "GAS/Abilities/MythicGA_Rune.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "GAS/Effects/MythicCrowdControl.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "GAS/MythicTags_GAS.h"
#include "Itemization/Inventory/Fragments/FragmentTypes.h"
#include "Itemization/Loot/MythicLootManagerSubsystem.h"
#include "Player/MythicPlayerController.h"
#include "Progression/MythicStatLedgerComponent.h"
#include "Progression/MythicTags_MetaProgression.h"
#include "Progression/Runes/MythicRuneDefinition.h"
#include "Progression/Runes/MythicTags_Rune.h"
#include "Rewards/LootReward.h"
#include "Rewards/LootScaling.h"
#include "Settings/MythicCombatSettings.h"
#include "Tests/Combat/MythicTestEffects.h"
#include "Tests/GAS/MythicGA_RuneTestTypes.h"

namespace {

struct FMythicRuneAbilityTestFixture {
    UGameInstance *GameInstance = nullptr;
    UWorld *World = nullptr;
};

bool BuildRuneAbilityTestWorld(FAutomationTestBase &Test, FMythicRuneAbilityTestFixture &Out) {
    if (!Test.TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }
    Out.GameInstance = NewObject<UGameInstance>(GEngine);
    Out.GameInstance->InitializeStandalone();
    Out.World = Out.GameInstance->GetWorld();
    return Test.TestNotNull(TEXT("standalone world exists"), Out.World);
}

float RuneAbilityTestHealthOf(const UAbilitySystemComponent *ASC) {
    return ASC->GetNumericAttribute(UMythicAttributeSet_Life::GetHealthAttribute());
}

float RuneAbilityTestShield(const UAbilitySystemComponent *ASC) {
    return ASC->GetNumericAttribute(UMythicAttributeSet_Defense::GetShieldAttribute());
}

float RuneAbilityTestMaxShield(const UAbilitySystemComponent *ASC) {
    return ASC->GetNumericAttribute(UMythicAttributeSet_Defense::GetMaxShieldAttribute());
}

// The rune base binds the avatar's landing and reaches the life component through the character's own cache, so a
// pawn is the honest fixture rather than a bare actor with an ability system bolted on.
AMythicRuneTestCharacter *SpawnRuneAbilityTestPawn(UWorld *World, float MaxHealth) {
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AMythicRuneTestCharacter *Pawn = World->SpawnActor<AMythicRuneTestCharacter>(AMythicRuneTestCharacter::StaticClass(),
                                                                                  FTransform::Identity, Params);
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
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetMaxHealthAttribute(), MaxHealth);
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(), MaxHealth);
    Pawn->LifeAttributes->RefreshOutOfHealthLatch();
    Pawn->Life->InitializeWithAbilitySystem(ASC);
    return Pawn;
}

// Grants the rune the way UMythicRuneComponent does: definition as the spec's source object, OnSpawn activation.
// A prepared definition carries its own parameters; without one a bare definition is made for the class.
UMythicGA_Rune *GrantRuneAbilityTestRune(UAbilitySystemComponent *ASC, TSubclassOf<UMythicGA_Rune> AbilityClass, FGameplayAbilitySpecHandle &OutHandle,
                          UMythicRuneDefinition *Definition = nullptr) {
    if (!Definition) {
        Definition = NewObject<UMythicRuneDefinition>(ASC);
    }
    Definition->Ability = AbilityClass;
    OutHandle = ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, Definition));
    FGameplayAbilitySpec *Spec = ASC->FindAbilitySpecFromHandle(OutHandle);
    if (!Spec) {
        return nullptr;
    }
    if (!Spec->IsActive()) {
        ASC->TryActivateAbility(OutHandle);
    }
    return Cast<UMythicGA_Rune>(Spec->GetPrimaryInstance());
}

bool IsRuneAbilityTestSpecActive(const UAbilitySystemComponent *ASC, const FGameplayAbilitySpecHandle Handle) {
    const FGameplayAbilitySpec *Spec = ASC->FindAbilitySpecFromHandle(Handle);
    return Spec && Spec->IsActive();
}

bool IsRuneAbilityTestEventBound(const UAbilitySystemComponent *ASC, const FGameplayTag Event) {
    const FGameplayEventMulticastDelegate *Delegate = ASC->GenericGameplayEventCallbacks.Find(Event);
    return Delegate && Delegate->IsBound();
}

void ApplyRuneAbilityTestDamage(UAbilitySystemComponent *ASC, float Amount) {
    UGameplayEffect *Effect = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("RuneTest_Damage")));
    Effect->DurationPolicy = EGameplayEffectDurationType::Instant;

    FGameplayModifierInfo Mod;
    Mod.Attribute = UMythicAttributeSet_Life::GetDamageAttribute();
    Mod.ModifierOp = EGameplayModOp::Additive;
    Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Amount));
    Effect->Modifiers.Add(Mod);

    ASC->ApplyGameplayEffectToSelf(Effect, 1.0f, ASC->MakeEffectContext());
}

// One automation test runs inside a single engine frame. World time only moves through UWorld::Tick, and the world
// settings clamp each step to MaxUndilatedFrameTime, so a long wait is many short frames. LEVELTICK_TimeOnly skips
// actor ticks and the timer manager, so the timers get the same step by hand under a fresh frame counter.
void AdvanceRuneAbilityTestWorld(UWorld *World, float Seconds) {
    constexpr float StepSeconds = 0.2f;
    const int32 Frames = FMath::CeilToInt32(Seconds / StepSeconds);
    for (int32 Frame = 0; Frame < Frames; Frame++) {
        ++GFrameCounter;
        World->Tick(LEVELTICK_TimeOnly, StepSeconds);
        World->GetTimerManager().Tick(StepSeconds);
    }
}

void LandRuneAbilityTestPawn(AMythicRuneTestCharacter *Pawn, float ImpactSpeed) {
    Pawn->GetCharacterMovement()->Velocity = FVector(0.0, 0.0, -ImpactSpeed);
    Pawn->Landed(FHitResult());
}

// The loot roll names a crediting controller, and the rune answers whether that credit is its own. A pawn with no
// controller could never tell the two apart, so the fixture puts a real one behind it before the rune is granted.
AMythicPlayerController *SpawnRuneAbilityTestController(UWorld *World, AMythicRuneTestCharacter *Pawn) {
    AMythicPlayerController *PC = World->SpawnActor<AMythicPlayerController>();
    if (PC && Pawn) {
        Pawn->Controller = PC;
        Pawn->AbilitySystem->InitAbilityActorInfo(Pawn, Pawn);
    }
    return PC;
}

// The guard effects the way the authored assets are shaped, applied by hand in a chosen order.
void ApplyRuneAbilityTestGuardEffect(UAbilitySystemComponent *ASC, TSubclassOf<UGameplayEffect> EffectClass, float Amount, float Seconds) {
    const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, 1.0f, ASC->MakeEffectContext());
    Spec.Data->SetSetByCallerMagnitude(GAS_SETBYCALLER_GENERIC, Amount);
    Spec.Data->SetSetByCallerMagnitude(GAS_SETBYCALLER_DURATION, Seconds);
    ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRunePreventDeathTest,
    "Mythic.GAS.Rune.PreventDeath",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRunePreventDeathTest::RunTest(const FString &Parameters) {
    FMythicRuneAbilityTestFixture Fixture;
    const bool bReady = BuildRuneAbilityTestWorld(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }

    AMythicRuneTestCharacter *Saved = SpawnRuneAbilityTestPawn(Fixture.World, 100.0f);
    AMythicRuneTestCharacter *Control = SpawnRuneAbilityTestPawn(Fixture.World, 100.0f);
    if (!TestNotNull(TEXT("the rune-wearing pawn spawned"), Saved) || !TestNotNull(TEXT("the control pawn spawned"), Control)) {
        return false;
    }
    if (!TestEqual(TEXT("both pawns start at full health"), RuneAbilityTestHealthOf(Saved->AbilitySystem) + RuneAbilityTestHealthOf(Control->AbilitySystem), 200.0f)) {
        return false;
    }

    FGameplayAbilitySpecHandle Handle;
    UMythicRuneTestCheatDeathAbility *Rune = Cast<UMythicRuneTestCheatDeathAbility>(
        GrantRuneAbilityTestRune(Saved->AbilitySystem, UMythicRuneTestCheatDeathAbility::StaticClass(), Handle));
    if (!TestNotNull(TEXT("the rune was granted and instanced"), Rune)) {
        return false;
    }
    if (!TestTrue(TEXT("the rune activated on grant"), IsRuneAbilityTestSpecActive(Saved->AbilitySystem, Handle))) {
        return false;
    }

    // Outside the lethal blow there is no death to prevent; the verb must say so and touch nothing.
    TestFalse(TEXT("PreventDeath outside OnRunePreDeath is refused"), Rune->PreventDeath(0.5f));
    TestEqual(TEXT("and leaves health alone"), RuneAbilityTestHealthOf(Saved->AbilitySystem), 100.0f);
    TestFalse(TEXT("and leaves no handled tag behind"), Saved->AbilitySystem->HasMatchingGameplayTag(GAS_PIPELINE_DEATH_HANDLED));

    ApplyRuneAbilityTestDamage(Saved->AbilitySystem, 500.0f);
    ApplyRuneAbilityTestDamage(Control->AbilitySystem, 500.0f);

    TestEqual(TEXT("the rune heard the lethal blow once"), Rune->PreDeathCount, 1);
    TestTrue(TEXT("PreventDeath inside OnRunePreDeath succeeded"), Rune->bLastPreventResult);
    TestEqual(TEXT("the owner stands at a quarter health"), RuneAbilityTestHealthOf(Saved->AbilitySystem), 25.0f);
    TestFalse(TEXT("GAS.State.Dead was never set on the saved owner"), Saved->AbilitySystem->HasMatchingGameplayTag(GAS_STATE_DEAD));
    TestFalse(TEXT("the pipeline consumed the handled tag"), Saved->AbilitySystem->HasMatchingGameplayTag(GAS_PIPELINE_DEATH_HANDLED));
    TestFalse(TEXT("the saved owner's life set is not latched dead"), Saved->LifeAttributes->IsDead());

    // The denominator: the same blow on the same pawn without the rune is a death.
    TestEqual(TEXT("the control pawn is at zero health"), RuneAbilityTestHealthOf(Control->AbilitySystem), 0.0f);
    TestTrue(TEXT("the control pawn carries GAS.State.Dead"), Control->AbilitySystem->HasMatchingGameplayTag(GAS_STATE_DEAD));

    // A saved owner is a live one: the next hit must still count.
    ApplyRuneAbilityTestDamage(Saved->AbilitySystem, 10.0f);
    TestEqual(TEXT("a later hit still takes health"), RuneAbilityTestHealthOf(Saved->AbilitySystem), 15.0f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneCooldownTest,
    "Mythic.GAS.Rune.Cooldown",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneCooldownTest::RunTest(const FString &Parameters) {
    FMythicRuneAbilityTestFixture Fixture;
    const bool bReady = BuildRuneAbilityTestWorld(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }

    AMythicRuneTestCharacter *Pawn = SpawnRuneAbilityTestPawn(Fixture.World, 100.0f);
    if (!TestNotNull(TEXT("the pawn spawned"), Pawn)) {
        return false;
    }
    FGameplayAbilitySpecHandle Handle;
    UMythicGA_Rune *Rune = GrantRuneAbilityTestRune(Pawn->AbilitySystem, UMythicRuneTestAbility::StaticClass(), Handle);
    if (!TestNotNull(TEXT("the rune was granted and instanced"), Rune)) {
        return false;
    }

    TestFalse(TEXT("a fresh rune is not on cooldown"), Rune->IsRuneOnCooldown());
    TestEqual(TEXT("and reports nothing remaining"), Rune->GetRuneCooldownRemaining(), 0.0f);
    TestFalse(TEXT("a zero-length cooldown is refused"), Rune->StartRuneCooldown(0.0f));
    TestFalse(TEXT("and starts nothing"), Rune->IsRuneOnCooldown());

    if (!TestTrue(TEXT("a five second cooldown starts"), Rune->StartRuneCooldown(5.0f))) {
        return false;
    }
    TestTrue(TEXT("the rune is on cooldown"), Rune->IsRuneOnCooldown());
    TestTrue(TEXT("with the whole five seconds remaining"), FMath::IsNearlyEqual(Rune->GetRuneCooldownRemaining(), 5.0f, 0.01f));

    AdvanceRuneAbilityTestWorld(Fixture.World, 2.0f);
    TestTrue(TEXT("two seconds in, the rune is still on cooldown"), Rune->IsRuneOnCooldown());
    TestTrue(TEXT("with about three seconds left"), FMath::IsNearlyEqual(Rune->GetRuneCooldownRemaining(), 3.0f, 0.25f));

    AdvanceRuneAbilityTestWorld(Fixture.World, 3.2f);
    TestFalse(TEXT("after five seconds of world time the cooldown is over"), Rune->IsRuneOnCooldown());
    TestEqual(TEXT("and nothing remains"), Rune->GetRuneCooldownRemaining(), 0.0f);

    // The denominator: the clock the assertion reads is the one the test advanced.
    TestTrue(TEXT("world time moved at least five seconds"), Fixture.World->GetTimeSeconds() >= 5.0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneCooldownReductionTest,
    "Mythic.GAS.Rune.CooldownReduction",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneCooldownReductionTest::RunTest(const FString &Parameters) {
    // The rule on its own: reduction takes its share, the ceiling binds, garbage reads as none.
    TestEqual(TEXT("30% reduction leaves 70 of 100"), UMythicGA_Rune::ScaleRuneCooldown(100.0f, 0.3f, 0.6f), 70.0f);
    TestEqual(TEXT("reduction past the ceiling is held at the ceiling"), UMythicGA_Rune::ScaleRuneCooldown(100.0f, 0.9f, 0.6f), 40.0f);
    TestEqual(TEXT("negative reduction reads as none"), UMythicGA_Rune::ScaleRuneCooldown(100.0f, -1.0f, 0.6f), 100.0f);
    TestEqual(TEXT("a negative ceiling reads as no reduction"), UMythicGA_Rune::ScaleRuneCooldown(100.0f, 0.3f, -1.0f), 100.0f);

    FMythicRuneAbilityTestFixture Fixture;
    const bool bReady = BuildRuneAbilityTestWorld(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }

    AMythicRuneTestCharacter *Pawn = SpawnRuneAbilityTestPawn(Fixture.World, 100.0f);
    if (!TestNotNull(TEXT("the pawn spawned"), Pawn)) {
        return false;
    }
    UMythicAbilitySystemComponent *ASC = Pawn->AbilitySystem;
    if (!TestTrue(TEXT("the pawn carries the Utility set that holds cooldown reduction"),
                  ASC->HasAttributeSetForAttribute(UMythicAttributeSet_Utility::GetCooldownReductionAttribute()))) {
        return false;
    }
    FGameplayAbilitySpecHandle Handle;
    UMythicGA_Rune *Rune = GrantRuneAbilityTestRune(ASC, UMythicRuneTestAbility::StaticClass(), Handle);
    if (!TestNotNull(TEXT("the rune was granted and instanced"), Rune)) {
        return false;
    }

    ASC->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetCooldownReductionAttribute(), 0.3f);
    if (!TestEqual(TEXT("the owner holds 30% cooldown reduction"),
                   ASC->GetNumericAttribute(UMythicAttributeSet_Utility::GetCooldownReductionAttribute()), 0.3f, 0.001f)) {
        return false;
    }
    if (!TestTrue(TEXT("a hundred second cooldown starts"), Rune->StartRuneCooldown(100.0f))) {
        return false;
    }
    TestTrue(TEXT("the reduced cooldown is seventy seconds"), FMath::IsNearlyEqual(Rune->GetRuneCooldownRemaining(), 70.0f, 0.05f));

    // The denominator: with the reduction gone the same call is the full hundred.
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetCooldownReductionAttribute(), 0.0f);
    Rune->StartRuneCooldown(100.0f);
    TestTrue(TEXT("without reduction the cooldown is the full hundred"), FMath::IsNearlyEqual(Rune->GetRuneCooldownRemaining(), 100.0f, 0.05f));

    AddInfo(FString::Printf(TEXT("attribute ceiling %.2f, authored ceiling %.2f"),
                            ASC->GetNumericAttribute(UMythicAttributeSet_Utility::GetMaxCooldownReductionAttribute()),
                            GetDefault<UMythicCombatSettings>()->MaxCooldownReduction));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneFallDamageImmunityTest,
    "Mythic.GAS.Rune.FallDamageImmunity",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneFallDamageImmunityTest::RunTest(const FString &Parameters) {
    FMythicRuneAbilityTestFixture Fixture;
    const bool bReady = BuildRuneAbilityTestWorld(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }

    AMythicRuneTestCharacter *Immune = SpawnRuneAbilityTestPawn(Fixture.World, 100.0f);
    AMythicRuneTestCharacter *Control = SpawnRuneAbilityTestPawn(Fixture.World, 100.0f);
    if (!TestNotNull(TEXT("the immune pawn spawned"), Immune) || !TestNotNull(TEXT("the control pawn spawned"), Control)) {
        return false;
    }
    const float LethalImpact = Immune->GetSafeFallSpeed() + 2000.0f;

    FGameplayAbilitySpecHandle Handle;
    UMythicRuneTestSeamAbility *Rune = Cast<UMythicRuneTestSeamAbility>(
        GrantRuneAbilityTestRune(Immune->AbilitySystem, UMythicRuneTestSeamAbility::StaticClass(), Handle));
    if (!TestNotNull(TEXT("the rune was granted and instanced"), Rune)) {
        return false;
    }
    TestTrue(TEXT("the rune listens to the avatar's resolved landing"), Immune->OnFallDamageResolved.IsBound());

    Rune->SetRuneStateTag(GAS_IMMUNE_FALLDAMAGE, true);
    if (!TestTrue(TEXT("the immunity tag is on the owner"), Immune->AbilitySystem->HasMatchingGameplayTag(GAS_IMMUNE_FALLDAMAGE))) {
        return false;
    }

    LandRuneAbilityTestPawn(Immune, LethalImpact);
    LandRuneAbilityTestPawn(Control, LethalImpact);

    TestEqual(TEXT("the immune pawn lost no health"), RuneAbilityTestHealthOf(Immune->AbilitySystem), 100.0f);
    TestTrue(TEXT("the identical pawn without the tag was hurt"), RuneAbilityTestHealthOf(Control->AbilitySystem) < 100.0f);
    TestEqual(TEXT("the rune heard the landing once"), Rune->LandedCount, 1);
    TestTrue(TEXT("and was told the damage was prevented"), Rune->bLastPrevented);
    TestTrue(TEXT("and what the landing was worth"), Rune->LastFallDamage > 0.0f);
    TestTrue(TEXT("and how hard it hit"), FMath::IsNearlyEqual(Rune->LastImpactSpeed, LethalImpact, 1.0f));

    // Releasing the tag hands the rule back.
    Rune->SetRuneStateTag(GAS_IMMUNE_FALLDAMAGE, false);
    LandRuneAbilityTestPawn(Immune, LethalImpact);
    TestTrue(TEXT("with the tag released the same landing hurts"), RuneAbilityTestHealthOf(Immune->AbilitySystem) < 100.0f);
    TestEqual(TEXT("the rune heard the second landing"), Rune->LandedCount, 2);
    TestFalse(TEXT("and was told this one landed"), Rune->bLastPrevented);
    TestTrue(TEXT("the health lost is the damage the rune was told"),
             FMath::IsNearlyEqual(100.0f - RuneAbilityTestHealthOf(Immune->AbilitySystem), Rune->LastFallDamage, 0.01f));

    // A rune leaving its socket must not leave its rule behind.
    Rune->SetRuneStateTag(GAS_IMMUNE_FALLDAMAGE, true);
    Immune->AbilitySystem->ClearAbility(Handle);
    TestFalse(TEXT("unequipping releases every tag the rune held"), Immune->AbilitySystem->HasMatchingGameplayTag(GAS_IMMUNE_FALLDAMAGE));
    TestFalse(TEXT("and stops listening to the landing"), Immune->OnFallDamageResolved.IsBound());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneSurvivesCancelAllTest,
    "Mythic.GAS.Rune.SurvivesCancelAll",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneSurvivesCancelAllTest::RunTest(const FString &Parameters) {
    FMythicRuneAbilityTestFixture Fixture;
    const bool bReady = BuildRuneAbilityTestWorld(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }

    AActor *Actor = Fixture.World->SpawnActor<AActor>();
    UMythicAbilitySystemComponent *ASC = NewObject<UMythicAbilitySystemComponent>(Actor);
    ASC->RegisterComponent();
    ASC->InitAbilityActorInfo(Actor, Actor);
    ASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Offense>(Actor));
    if (!TestTrue(TEXT("the ability system answers as authority"), ASC->IsOwnerActorAuthoritative())) {
        return false;
    }

    // The control keeps the engine's default cancelability. It has to fall, or the sweep never ran and the rune
    // surviving proves nothing.
    const FGameplayAbilitySpecHandle ControlHandle = ASC->GiveAbility(FGameplayAbilitySpec(UMythicTestPassiveAbility::StaticClass(), 1, INDEX_NONE, Actor));
    FGameplayAbilitySpecHandle RuneHandle;
    UMythicGA_Rune *Rune = GrantRuneAbilityTestRune(ASC, UMythicRuneTestAbility::StaticClass(), RuneHandle);
    if (!TestTrue(TEXT("both abilities were granted"), ControlHandle.IsValid() && Rune != nullptr)) {
        return false;
    }
    ASC->TryActivateAbility(ControlHandle);

    if (!TestTrue(TEXT("the control passive is active before the cancel"), IsRuneAbilityTestSpecActive(ASC, ControlHandle))) {
        return false;
    }
    if (!TestTrue(TEXT("the rune is active before the cancel"), IsRuneAbilityTestSpecActive(ASC, RuneHandle))) {
        return false;
    }
    if (!TestTrue(TEXT("the rune's death seam is bound before the cancel"), IsRuneAbilityTestEventBound(ASC, GAS_EVENT_DEATH_PRE))) {
        return false;
    }
    TestTrue(TEXT("the rune listens to Item.Acquired"), IsRuneAbilityTestEventBound(ASC, GAS_EVENT_ITEM_ACQUIRED));
    TestTrue(TEXT("the rune listens to Item.Used"), IsRuneAbilityTestEventBound(ASC, GAS_EVENT_ITEM_USED));
    TestTrue(TEXT("the rune listens to Currency.Spent"), IsRuneAbilityTestEventBound(ASC, GAS_EVENT_CURRENCY_SPENT));
    TestTrue(TEXT("the rune listens to Harvest.Struck"), IsRuneAbilityTestEventBound(ASC, GAS_EVENT_HARVEST_STRUCK));
    TestTrue(TEXT("the rune listens to ability activations"), ASC->AbilityActivatedCallbacks.IsBound());
    TestTrue(TEXT("the rune listens to ability ends"), ASC->OnAbilityEnded.IsBound());

    // What EnterDownedState and StartDeath both do.
    ASC->CancelAllAbilities();

    TestFalse(TEXT("CancelAllAbilities ended the control passive, so the sweep really ran"), IsRuneAbilityTestSpecActive(ASC, ControlHandle));
    TestTrue(TEXT("the rune is still active after CancelAllAbilities"), IsRuneAbilityTestSpecActive(ASC, RuneHandle));
    TestTrue(TEXT("its death seam is still bound"), IsRuneAbilityTestEventBound(ASC, GAS_EVENT_DEATH_PRE));
    TestTrue(TEXT("and so is its kill seam"), IsRuneAbilityTestEventBound(ASC, GAS_EVENT_KILL));

    // Refusing cancellation must not refuse removal, or a rune could never leave its socket.
    ASC->ClearAbility(RuneHandle);
    TestNull(TEXT("clearing the rune still removes it"), ASC->FindAbilitySpecFromHandle(RuneHandle));
    TestFalse(TEXT("and unbinds its death seam"), IsRuneAbilityTestEventBound(ASC, GAS_EVENT_DEATH_PRE));
    TestFalse(TEXT("and its kill seam"), IsRuneAbilityTestEventBound(ASC, GAS_EVENT_KILL));
    TestFalse(TEXT("and its harvest seam"), IsRuneAbilityTestEventBound(ASC, GAS_EVENT_HARVEST_STRUCK));
    TestFalse(TEXT("and its ability activation seam"), ASC->AbilityActivatedCallbacks.IsBound());
    TestFalse(TEXT("and its ability end seam"), ASC->OnAbilityEnded.IsBound());

    AddInfo(FString::Printf(TEXT("abilities granted: 2, cancelled by the sweep: %d, survived: %d"),
                            IsRuneAbilityTestSpecActive(ASC, ControlHandle) ? 0 : 1, IsRuneAbilityTestSpecActive(ASC, RuneHandle) ? 1 : 0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneGuardTest,
    "Mythic.GAS.Rune.Guard",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneGuardTest::RunTest(const FString &Parameters) {
    FMythicRuneAbilityTestFixture Fixture;
    const bool bReady = BuildRuneAbilityTestWorld(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }

    UMythicCombatSettings *Settings = GetMutableDefault<UMythicCombatSettings>();
    TGuardValue<TSoftClassPtr<UGameplayEffect>> MaxShieldEffect(Settings->RuneGuardMaxShieldEffect,
                                                                TSoftClassPtr<UGameplayEffect>(UMythicRuneTestGuardMaxShieldEffect::StaticClass()));
    TGuardValue<TSoftClassPtr<UGameplayEffect>> ShieldEffect(Settings->RuneGuardShieldEffect,
                                                             TSoftClassPtr<UGameplayEffect>(UMythicRuneTestGuardShieldEffect::StaticClass()));

    AMythicRuneTestCharacter *Guarded = SpawnRuneAbilityTestPawn(Fixture.World, 100.0f);
    AMythicRuneTestCharacter *Reversed = SpawnRuneAbilityTestPawn(Fixture.World, 100.0f);
    if (!TestNotNull(TEXT("the guarded pawn spawned"), Guarded) || !TestNotNull(TEXT("the reversed-order pawn spawned"), Reversed)) {
        return false;
    }
    for (AMythicRuneTestCharacter *Pawn : {Guarded, Reversed}) {
        Pawn->AbilitySystem->SetNumericAttributeBase(UMythicAttributeSet_Defense::GetMaxShieldAttribute(), 100.0f);
        Pawn->AbilitySystem->SetNumericAttributeBase(UMythicAttributeSet_Defense::GetShieldAttribute(), 50.0f);
    }
    UMythicAbilitySystemComponent *ASC = Guarded->AbilitySystem;
    if (!TestEqual(TEXT("the guarded pawn starts with a 50 of 100 shield"), RuneAbilityTestShield(ASC), 50.0f)) {
        return false;
    }

    FGameplayAbilitySpecHandle Handle;
    UMythicRuneTestSeamAbility *Rune = Cast<UMythicRuneTestSeamAbility>(GrantRuneAbilityTestRune(ASC, UMythicRuneTestSeamAbility::StaticClass(), Handle));
    if (!TestNotNull(TEXT("the rune was granted and instanced"), Rune)) {
        return false;
    }
    TestFalse(TEXT("a fresh rune holds no guard"), Rune->IsRuneGuardActive());

    // A guard ten times the ceiling: only the ceiling rising first lets the whole add land.
    Rune->BeginRuneGuard(1000.0f, 1.0f);
    TestTrue(TEXT("the guard is active"), Rune->IsRuneGuardActive());
    TestEqual(TEXT("MaxShield rose by the guard"), RuneAbilityTestMaxShield(ASC), 1100.0f);
    TestEqual(TEXT("Shield rose by the whole guard on top of what it had"), RuneAbilityTestShield(ASC), 1050.0f);

    // The denominator: the same two effects in the other order clamp the add against the old ceiling.
    ApplyRuneAbilityTestGuardEffect(Reversed->AbilitySystem, UMythicRuneTestGuardShieldEffect::StaticClass(), 1000.0f, 1.0f);
    ApplyRuneAbilityTestGuardEffect(Reversed->AbilitySystem, UMythicRuneTestGuardMaxShieldEffect::StaticClass(), 1000.0f, 1.0f);
    TestTrue(TEXT("Shield before MaxShield loses the add to the clamp"), RuneAbilityTestShield(Reversed->AbilitySystem) < 1050.0f);

    // A hit the guard absorbed must not come out of the Shield the owner had before it.
    ASC->ApplyModToAttribute(UMythicAttributeSet_Defense::GetShieldAttribute(), EGameplayModOp::Additive, -300.0f);
    TestTrue(TEXT("the guard absorbed the hit"), RuneAbilityTestShield(ASC) < 1050.0f);

    Rune->EndRuneGuard();
    TestFalse(TEXT("EndRuneGuard clears the guard"), Rune->IsRuneGuardActive());
    TestEqual(TEXT("the rune heard the guard end once"), Rune->GuardEndedCount, 1);
    TestEqual(TEXT("MaxShield is back to its own"), RuneAbilityTestMaxShield(ASC), 100.0f);
    TestEqual(TEXT("Shield is back to what the owner had before the guard"), RuneAbilityTestShield(ASC), 50.0f);

    // A second EndRuneGuard has nothing to end.
    Rune->EndRuneGuard();
    TestEqual(TEXT("ending a guard that is not there says nothing"), Rune->GuardEndedCount, 1);

    // Expiry ends the guard the same way, from the effect's own clock.
    Rune->BeginRuneGuard(1000.0f, 1.0f);
    TestTrue(TEXT("a second guard starts"), Rune->IsRuneGuardActive());
    AdvanceRuneAbilityTestWorld(Fixture.World, 0.4f);
    TestTrue(TEXT("the guard holds inside its duration"), Rune->IsRuneGuardActive());
    AdvanceRuneAbilityTestWorld(Fixture.World, 1.0f);
    TestFalse(TEXT("the guard ended when its duration ran out"), Rune->IsRuneGuardActive());
    TestEqual(TEXT("the rune heard the expiry"), Rune->GuardEndedCount, 2);
    TestEqual(TEXT("expiry handed MaxShield back"), RuneAbilityTestMaxShield(ASC), 100.0f);
    TestEqual(TEXT("expiry handed the pre-guard Shield back"), RuneAbilityTestShield(ASC), 50.0f);

    // Unsocketing with a guard up hands the Shield back without a word: the rune has already been told it is leaving.
    Rune->BeginRuneGuard(1000.0f, 5.0f);
    ASC->ClearAbility(Handle);
    TestEqual(TEXT("unequip restores MaxShield"), RuneAbilityTestMaxShield(ASC), 100.0f);
    TestEqual(TEXT("unequip restores the pre-guard Shield"), RuneAbilityTestShield(ASC), 50.0f);
    TestEqual(TEXT("and raises no guard-ended event"), Rune->GuardEndedCount, 2);

    AddInfo(FString::Printf(TEXT("guards begun: 3, ended by verb: 1, by expiry: 1, by unequip: 1; reversed-order shield %.0f"),
                            RuneAbilityTestShield(Reversed->AbilitySystem)));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneSelfDamageAndHealTest,
    "Mythic.GAS.Rune.SelfDamageAndHeal",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneSelfDamageAndHealTest::RunTest(const FString &Parameters) {
    FMythicRuneAbilityTestFixture Fixture;
    const bool bReady = BuildRuneAbilityTestWorld(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }

    UMythicCombatSettings *Settings = GetMutableDefault<UMythicCombatSettings>();
    TGuardValue<TSoftClassPtr<UGameplayEffect>> SelfDamage(Settings->RuneSelfDamageEffect,
                                                           TSoftClassPtr<UGameplayEffect>(UMythicRuneTestDamageMetaEffect::StaticClass()));
    TGuardValue<TSoftClassPtr<UGameplayEffect>> Heal(Settings->RuneHealEffect,
                                                     TSoftClassPtr<UGameplayEffect>(UMythicRuneTestHealMetaEffect::StaticClass()));

    AMythicRuneTestCharacter *Pawn = SpawnRuneAbilityTestPawn(Fixture.World, 100.0f);
    if (!TestNotNull(TEXT("the pawn spawned"), Pawn)) {
        return false;
    }
    UMythicAbilitySystemComponent *ASC = Pawn->AbilitySystem;
    FGameplayAbilitySpecHandle Handle;
    UMythicGA_Rune *Rune = GrantRuneAbilityTestRune(ASC, UMythicRuneTestAbility::StaticClass(), Handle);
    if (!TestNotNull(TEXT("the rune was granted and instanced"), Rune)) {
        return false;
    }

    int32 Received = 0;
    float ReceivedMagnitude = 0.0f;
    bool bReceivedHitTag = false;
    bool bReceivedOwnHit = false;
    ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_DMG_RECEIVED).AddLambda(
        [&Received, &ReceivedMagnitude, &bReceivedHitTag, &bReceivedOwnHit, Rune](const FGameplayEventData *Payload) {
            ++Received;
            ReceivedMagnitude = Payload ? Payload->EventMagnitude : 0.0f;
            bReceivedHitTag = Payload && Payload->InstigatorTags.HasTag(GAS_HIT_RUNE_DEBT);
            bReceivedOwnHit = Payload && Rune->IsOwnHit(Payload->ContextHandle);
        });

    Rune->DealRuneSelfDamage(30.0f, GAS_HIT_RUNE_DEBT);
    TestEqual(TEXT("the self-wound took health through the Damage meta"), RuneAbilityTestHealthOf(ASC), 70.0f);
    TestEqual(TEXT("Dmg.Received fired once"), Received, 1);
    TestEqual(TEXT("with the amount dealt"), ReceivedMagnitude, 30.0f);
    TestTrue(TEXT("with the hit tag on the payload"), bReceivedHitTag);
    TestTrue(TEXT("and the rune's own definition as the hit's source"), bReceivedOwnHit);

    Rune->DealRuneSelfDamage(0.0f, GAS_HIT_RUNE_DEBT);
    Rune->DealRuneSelfDamage(-5.0f, GAS_HIT_RUNE_DEBT);
    TestEqual(TEXT("a zero or negative wound deals nothing"), RuneAbilityTestHealthOf(ASC), 70.0f);
    TestEqual(TEXT("and raises nothing"), Received, 1);

    Rune->HealRune(20.0f);
    TestEqual(TEXT("the heal restored health through the Healing meta"), RuneAbilityTestHealthOf(ASC), 90.0f);
    Rune->HealRune(-5.0f);
    TestEqual(TEXT("a negative heal does nothing"), RuneAbilityTestHealthOf(ASC), 90.0f);
    Rune->HealRune(50.0f);
    TestEqual(TEXT("healing stops at max health"), RuneAbilityTestHealthOf(ASC), 100.0f);

    // A hit from somewhere else is not the rune's own.
    TestFalse(TEXT("a plain context is not the rune's own hit"), Rune->IsOwnHit(ASC->MakeEffectContext()));
    TestFalse(TEXT("an empty context is not the rune's own hit"), Rune->IsOwnHit(FGameplayEffectContextHandle()));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneKillRoutingTest,
    "Mythic.GAS.Rune.KillRouting",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneKillRoutingTest::RunTest(const FString &Parameters) {
    FMythicRuneAbilityTestFixture Fixture;
    const bool bReady = BuildRuneAbilityTestWorld(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }

    AMythicRuneTestCharacter *Pawn = SpawnRuneAbilityTestPawn(Fixture.World, 100.0f);
    if (!TestNotNull(TEXT("the pawn spawned"), Pawn)) {
        return false;
    }
    UMythicAbilitySystemComponent *ASC = Pawn->AbilitySystem;
    FGameplayAbilitySpecHandle Handle;
    UMythicRuneTestSeamAbility *Rune = Cast<UMythicRuneTestSeamAbility>(GrantRuneAbilityTestRune(ASC, UMythicRuneTestSeamAbility::StaticClass(), Handle));
    if (!TestNotNull(TEXT("the rune was granted and instanced"), Rune)) {
        return false;
    }

    FGameplayEventData Felled;
    Felled.EventTag = GAS_EVENT_HARVEST_STRUCK;
    Felled.EventMagnitude = 3.0f;
    Felled.InstigatorTags.AddTag(HARVEST_FELLED);
    FGameplayEventData Struck;
    Struck.EventTag = GAS_EVENT_HARVEST_STRUCK;
    Struck.EventMagnitude = 1.0f;
    FGameplayEventData Kill;
    Kill.EventTag = GAS_EVENT_KILL;

    ASC->HandleGameplayEvent(GAS_EVENT_HARVEST_STRUCK, &Felled);
    TestEqual(TEXT("a felled node is not a kill while the rule is not held"), Rune->KillCount, 0);

    ASC->AddLooseGameplayTag(RUNE_RULE_FELLEDISKILL);
    ASC->HandleGameplayEvent(GAS_EVENT_HARVEST_STRUCK, &Felled);
    TestEqual(TEXT("a felled node is a kill while Rune.Rule.FelledIsKill is held"), Rune->KillCount, 1);

    ASC->HandleGameplayEvent(GAS_EVENT_HARVEST_STRUCK, &Struck);
    TestEqual(TEXT("a strike that felled nothing is never a kill"), Rune->KillCount, 1);

    ASC->HandleGameplayEvent(GAS_EVENT_KILL, &Kill);
    TestEqual(TEXT("GAS.Event.Kill is always a kill"), Rune->KillCount, 2);

    ASC->RemoveLooseGameplayTag(RUNE_RULE_FELLEDISKILL);
    ASC->HandleGameplayEvent(GAS_EVENT_HARVEST_STRUCK, &Felled);
    TestEqual(TEXT("releasing the rule stops felled nodes counting"), Rune->KillCount, 2);

    ASC->HandleGameplayEvent(GAS_EVENT_KILL, &Kill);
    TestEqual(TEXT("and leaves real kills alone"), Rune->KillCount, 3);

    AddInfo(FString::Printf(TEXT("events sent: 6, kills routed: %d"), Rune->KillCount));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneReadRolledTest,
    "Mythic.GAS.Rune.ReadRolledMidpoint",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneReadRolledTest::RunTest(const FString &Parameters) {
    FMythicRuneAbilityTestFixture Fixture;
    const bool bReady = BuildRuneAbilityTestWorld(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }

    AMythicRuneTestCharacter *Pawn = SpawnRuneAbilityTestPawn(Fixture.World, 100.0f);
    if (!TestNotNull(TEXT("the pawn spawned"), Pawn)) {
        return false;
    }
    UMythicAbilitySystemComponent *ASC = Pawn->AbilitySystem;

    // Any registered tag serves as a parameter key; the rune's own tags are minted with the content.
    const FGameplayTag Known = GAS_STATE_HEALTH_CRITICAL;
    const FGameplayTag Unknown = GAS_STATE_HEALTH_LOW;
    UMythicRuneDefinition *Definition = NewObject<UMythicRuneDefinition>(ASC);
    FRollDefinition Roll;
    Roll.Min = 2.0f;
    Roll.Max = 4.0f;
    Definition->Parameters.Add(Known, Roll);

    FGameplayAbilitySpecHandle Handle;
    UMythicGA_Rune *Rune = GrantRuneAbilityTestRune(ASC, UMythicRuneTestAbility::StaticClass(), Handle, Definition);
    if (!TestNotNull(TEXT("the rune was granted and instanced"), Rune)) {
        return false;
    }
    if (!TestTrue(TEXT("the rune reads its definition from the spec"), Rune->GetRuneDefinition() == Definition)) {
        return false;
    }
    TestEqual(TEXT("with no roll set the parameter reads as its midpoint"), Rune->ReadRolled(Known, 99.0f), 3.0f);
    TestEqual(TEXT("a parameter the definition lacks reads as the fallback"), Rune->ReadRolled(Unknown, 99.0f), 99.0f);
    TestEqual(TEXT("an empty tag reads as the fallback"), Rune->ReadRolled(FGameplayTag(), 7.0f), 7.0f);
    TestEqual(TEXT("granted outside a rune component the socket is none"), Rune->GetRuneSlotIndex(), static_cast<int32>(INDEX_NONE));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneDeferToNextTickTest,
    "Mythic.GAS.Rune.DeferToNextTick",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneDeferToNextTickTest::RunTest(const FString &Parameters) {
    FMythicRuneAbilityTestFixture Fixture;
    const bool bReady = BuildRuneAbilityTestWorld(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }

    AMythicRuneTestCharacter *Pawn = SpawnRuneAbilityTestPawn(Fixture.World, 100.0f);
    if (!TestNotNull(TEXT("the pawn spawned"), Pawn)) {
        return false;
    }
    FGameplayAbilitySpecHandle Handle;
    UMythicGA_Rune *Rune = GrantRuneAbilityTestRune(Pawn->AbilitySystem, UMythicRuneTestAbility::StaticClass(), Handle);
    if (!TestNotNull(TEXT("the rune was granted and instanced"), Rune)) {
        return false;
    }

    UMythicRuneTestDeferredTarget *Target = NewObject<UMythicRuneTestDeferredTarget>(Pawn);
    FMythicRuneDeferredDelegate Callback;
    Callback.BindDynamic(Target, &UMythicRuneTestDeferredTarget::Fire);

    Rune->DeferToNextTick(Callback);
    TestEqual(TEXT("the deferred work does not run inside the frame that asked for it"), Target->Fires, 0);

    AdvanceRuneAbilityTestWorld(Fixture.World, 0.2f);
    TestEqual(TEXT("it runs on the next tick"), Target->Fires, 1);

    AdvanceRuneAbilityTestWorld(Fixture.World, 0.2f);
    TestEqual(TEXT("and only once"), Target->Fires, 1);

    // Work deferred by a rune that then leaves its socket is dropped with it.
    Rune->DeferToNextTick(Callback);
    Pawn->AbilitySystem->ClearAbility(Handle);
    AdvanceRuneAbilityTestWorld(Fixture.World, 0.2f);
    TestEqual(TEXT("unequipping drops the pending work"), Target->Fires, 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneDashSeamsTest,
    "Mythic.GAS.Rune.DashSeams",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneDashSeamsTest::RunTest(const FString &Parameters) {
    FMythicRuneAbilityTestFixture Fixture;
    const bool bReady = BuildRuneAbilityTestWorld(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }

    AMythicRuneTestCharacter *Pawn = SpawnRuneAbilityTestPawn(Fixture.World, 100.0f);
    if (!TestNotNull(TEXT("the pawn spawned"), Pawn)) {
        return false;
    }
    UMythicAbilitySystemComponent *ASC = Pawn->AbilitySystem;
    FGameplayAbilitySpecHandle Handle;
    UMythicRuneTestSeamAbility *Rune = Cast<UMythicRuneTestSeamAbility>(GrantRuneAbilityTestRune(ASC, UMythicRuneTestSeamAbility::StaticClass(), Handle));
    if (!TestNotNull(TEXT("the rune was granted and instanced"), Rune)) {
        return false;
    }

    UMythicRuneTestDashSkill *Dash = NewObject<UMythicRuneTestDashSkill>(Pawn);
    UMythicRuneTestStandSkill *Stand = NewObject<UMythicRuneTestStandSkill>(Pawn);
    const FVector Start = Pawn->GetActorLocation();

    // The ASC raises these for every ability; the rune keeps only the ones that move the caster.
    ASC->AbilityActivatedCallbacks.Broadcast(Stand);
    TestEqual(TEXT("a skill that stays put is not a dash"), Rune->DashStartedCount, 0);

    ASC->AbilityActivatedCallbacks.Broadcast(Dash);
    TestEqual(TEXT("a moving skill starting is a dash start"), Rune->DashStartedCount, 1);
    TestTrue(TEXT("with the skill that started"), Rune->LastDashSkill.Get() == Dash);
    TestTrue(TEXT("and where the avatar stood"), Rune->LastDashStart.Equals(Start, 0.01f));

    Pawn->SetActorLocation(Start + FVector(300.0f, 0.0f, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
    ASC->OnAbilityEnded.Broadcast(FAbilityEndedData(Stand, FGameplayAbilitySpecHandle(), false, false));
    TestEqual(TEXT("a still skill ending is not a dash end"), Rune->DashEndedCount, 0);

    ASC->OnAbilityEnded.Broadcast(FAbilityEndedData(Dash, FGameplayAbilitySpecHandle(), false, false));
    TestEqual(TEXT("the moving skill ending is a dash end"), Rune->DashEndedCount, 1);
    TestTrue(TEXT("with where the avatar ended"), Rune->LastDashEnd.Equals(Pawn->GetActorLocation(), 0.01f));

    // What the dash costs the owner: the authored ten, then half of it under half reduction.
    TestEqual(TEXT("the dash costs its authored stamina"), Rune->GetSkillStaminaCost(Dash), 10.0f);
    TestEqual(TEXT("a skill with no stamina cost costs nothing"), Rune->GetSkillStaminaCost(Stand), 0.0f);
    TestEqual(TEXT("no skill costs nothing"), Rune->GetSkillStaminaCost(nullptr), 0.0f);
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetStaminaCostReductionAttribute(), 0.5f);
    TestEqual(TEXT("the owner's stamina-cost reduction is what the cost reads after"), Rune->GetSkillStaminaCost(Dash), 5.0f);

    ASC->ClearAbility(Handle);
    ASC->AbilityActivatedCallbacks.Broadcast(Dash);
    ASC->OnAbilityEnded.Broadcast(FAbilityEndedData(Dash, FGameplayAbilitySpecHandle(), false, false));
    TestEqual(TEXT("an unequipped rune hears no dash start"), Rune->DashStartedCount, 1);
    TestEqual(TEXT("nor a dash end"), Rune->DashEndedCount, 1);

    AddInfo(FString::Printf(TEXT("activations raised: 3, dash starts heard: %d; ends raised: 3, dash ends heard: %d"),
                            Rune->DashStartedCount, Rune->DashEndedCount));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneAvatarSeamsTest,
    "Mythic.GAS.Rune.AvatarSeams",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneAvatarSeamsTest::RunTest(const FString &Parameters) {
    FMythicRuneAbilityTestFixture Fixture;
    const bool bReady = BuildRuneAbilityTestWorld(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }

    AMythicRuneTestCharacter *Pawn = SpawnRuneAbilityTestPawn(Fixture.World, 100.0f);
    if (!TestNotNull(TEXT("the pawn spawned"), Pawn)) {
        return false;
    }
    UMythicAbilitySystemComponent *ASC = Pawn->AbilitySystem;
    UMythicLifeComponent *Life = Pawn->Life;
    FGameplayAbilitySpecHandle Handle;
    UMythicRuneTestSeamAbility *Rune = Cast<UMythicRuneTestSeamAbility>(GrantRuneAbilityTestRune(ASC, UMythicRuneTestSeamAbility::StaticClass(), Handle));
    if (!TestNotNull(TEXT("the rune was granted and instanced"), Rune)) {
        return false;
    }

    TestTrue(TEXT("the rune listens to the movement mode"), Pawn->OnMythicMovementModeChange.IsBound());
    TestTrue(TEXT("the rune listens to the resolved landing"), Pawn->OnFallDamageResolved.IsBound());
    TestTrue(TEXT("the rune listens to the still edge"), Life->OnStillBegan.IsBound());
    TestTrue(TEXT("the rune listens to the fall depth"), Life->OnFallDepthSampled.IsBound());

    UCharacterMovementComponent *Move = Pawn->GetCharacterMovement();
    Move->SetMovementMode(MOVE_Falling);
    TestEqual(TEXT("entering Falling is a fall beginning"), Rune->FallBeganCount, 1);
    Move->SetMovementMode(MOVE_Falling);
    TestEqual(TEXT("staying in Falling is not another"), Rune->FallBeganCount, 1);
    Move->SetMovementMode(MOVE_Walking);
    TestEqual(TEXT("leaving Falling is not one either"), Rune->FallBeganCount, 1);
    Move->SetMovementMode(MOVE_Falling);
    TestEqual(TEXT("a second fall is a second beginning"), Rune->FallBeganCount, 2);
    Move->SetMovementMode(MOVE_Walking);

    Life->OnStillBegan.Broadcast();
    TestEqual(TEXT("the life component's still edge reaches the rune"), Rune->StillBeganCount, 1);
    Life->OnFallDepthSampled.Broadcast(3.5f);
    Life->OnFallDepthSampled.Broadcast(7.25f);
    if (TestEqual(TEXT("each fall-depth sample reaches the rune"), Rune->FallDepths.Num(), 2)) {
        TestEqual(TEXT("with the metres sampled"), Rune->FallDepths[1], 7.25f);
    }

    // A rune with no player behind it reads a purse of nothing and mints nothing, without falling over.
    TestEqual(TEXT("no controller means no gold"), Rune->GetCarriedGold(), 0);
    TestEqual(TEXT("and nothing minted"), Rune->GrantGold(5), 0);
    TestFalse(TEXT("and nothing charged"), Rune->TryChargeCurrency(5));
    Rune->RaiseRuneNotice(EMythicNoticeKind::Warning, FText::FromString(TEXT("no one hears this")));
    Rune->ShowRuneCallout(FText::FromString(TEXT("nor this")), FLinearColor::White);
    Rune->PlayRuneCue(FGameplayTag(), false);

    // The ledger on the owner takes the rune's counters.
    Rune->RecordRuneStat(STAT_RUNE_DEATHS_CHEATED, 2);
    TestEqual(TEXT("RecordRuneStat writes the owner's ledger"), Pawn->Ledger->GetCounter(STAT_RUNE_DEATHS_CHEATED), static_cast<int64>(2));
    Rune->RecordRuneStat(STAT_RUNE_DEATHS_CHEATED, 0);
    Rune->RecordRuneStat(FGameplayTag(), 5);
    TestEqual(TEXT("a zero delta or empty tag writes nothing"), Pawn->Ledger->GetCounter(STAT_RUNE_DEATHS_CHEATED), static_cast<int64>(2));

    ASC->ClearAbility(Handle);
    TestFalse(TEXT("unequipping stops listening to the movement mode"), Pawn->OnMythicMovementModeChange.IsBound());
    TestFalse(TEXT("and to the resolved landing"), Pawn->OnFallDamageResolved.IsBound());
    TestFalse(TEXT("and to the still edge"), Life->OnStillBegan.IsBound());
    TestFalse(TEXT("and to the fall depth"), Life->OnFallDepthSampled.IsBound());
    Move->SetMovementMode(MOVE_Falling);
    TestEqual(TEXT("an unequipped rune hears no fall"), Rune->FallBeganCount, 2);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneCueThrottleRuleTest,
    "Mythic.GAS.Rune.CueThrottleRule",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneCueThrottleRuleTest::RunTest(const FString &Parameters) {
    const float Throttle = GetDefault<UMythicCombatSettings>()->RuneCueThrottleSeconds;
    if (!TestTrue(TEXT("a cue throttle is authored"), Throttle > 0.0f)) {
        return false;
    }
    TestTrue(TEXT("a repeat inside the throttle is dropped"), UMythicGA_Rune::ShouldThrottleRuneCue(Throttle * 0.5, Throttle));
    TestTrue(TEXT("a repeat in the same instant is dropped"), UMythicGA_Rune::ShouldThrottleRuneCue(0.0, Throttle));
    TestFalse(TEXT("a repeat on the throttle plays"), UMythicGA_Rune::ShouldThrottleRuneCue(Throttle, Throttle));
    TestFalse(TEXT("a repeat past the throttle plays"), UMythicGA_Rune::ShouldThrottleRuneCue(Throttle * 3.0, Throttle));
    TestFalse(TEXT("no throttle drops nothing"), UMythicGA_Rune::ShouldThrottleRuneCue(0.0, 0.0f));
    AddInfo(FString::Printf(TEXT("authored throttle %.2fs"), Throttle));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRunePreLootRollTest,
    "Mythic.GAS.Rune.PreLootRoll",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRunePreLootRollTest::RunTest(const FString &Parameters) {
    FMythicRuneAbilityTestFixture Fixture;
    const bool bReady = BuildRuneAbilityTestWorld(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }

    UMythicLootManagerSubsystem *LootManager = Fixture.GameInstance->GetSubsystem<UMythicLootManagerSubsystem>();
    if (!TestNotNull(TEXT("the loot manager exists on the authority game instance"), LootManager)) {
        return false;
    }

    AMythicRuneTestCharacter *Wearer = SpawnRuneAbilityTestPawn(Fixture.World, 100.0f);
    if (!TestNotNull(TEXT("the rune-wearing pawn spawned"), Wearer)) {
        return false;
    }
    AMythicPlayerController *Owner = SpawnRuneAbilityTestController(Fixture.World, Wearer);
    AMythicPlayerController *PartyMember = SpawnRuneAbilityTestController(Fixture.World, nullptr);
    if (!TestNotNull(TEXT("the wearer's controller spawned"), Owner)
        || !TestNotNull(TEXT("a second crediting controller spawned"), PartyMember)) {
        return false;
    }

    FGameplayAbilitySpecHandle Handle;
    UMythicRuneTestLootAbility *Rune = Cast<UMythicRuneTestLootAbility>(
        GrantRuneAbilityTestRune(Wearer->AbilitySystem, UMythicRuneTestLootAbility::StaticClass(), Handle));
    if (!TestNotNull(TEXT("the loot rune was granted and instanced"), Rune)) {
        return false;
    }
    if (!TestTrue(TEXT("activating the rune bound the pre-loot-roll seam"), LootManager->OnPreLootRoll.IsBound())) {
        return false;
    }

    // A tier-one credit with no find carries nothing of its own, so every number below is the rune's alone.
    const FLootTierBonus Own = ULootReward::PrepareLootRoll(LootManager, Owner, 1, 0.0f);

    TestEqual(TEXT("the rune heard its owner's credit once"), Rune->PreLootRollCount, 1);
    TestTrue(TEXT("and was told the credit is its own"), Rune->bLastForOwner);
    TestEqual(TEXT("the rune's two extra drops reached the bonus"), Own.ExtraDropCount, 2);
    TestEqual(TEXT("and its rarity floor"), Own.GuaranteedMinRarity, 3);
    TestEqual(TEXT("and its drop scale"), Own.DropCountScale, 0.0f);
    TestEqual(TEXT("so the roll the tables then read pays nothing, extras included"),
              FMythicLootScaling::ResolveDropCount(3, Own, 1.0f), 0);

    // The same credit without the scale shows the extras really are in the bonus rather than lost to the zero.
    FLootTierBonus Unscaled = Own;
    Unscaled.DropCountScale = 1.0f;
    TestEqual(TEXT("the extras alone would have paid two more"), FMythicLootScaling::ResolveDropCount(1, Unscaled, 1.0f), 3);

    // Every rune hears every credit; only the ownership question separates the wearer's kill from a party member's.
    const FLootTierBonus Party = ULootReward::PrepareLootRoll(LootManager, PartyMember, 1, 0.0f);

    TestEqual(TEXT("the rune heard the party member's credit too"), Rune->PreLootRollCount, 2);
    TestTrue(TEXT("and was handed that controller"), Rune->LastCreditedTo.Get() == PartyMember);
    TestFalse(TEXT("but was told the credit is not its own"), Rune->bLastForOwner);
    TestEqual(TEXT("a rune that edits regardless still edits that bonus"), Party.ExtraDropCount, 2);

    AddInfo(FString::Printf(TEXT("credits rolled: 2, heard by the rune: %d, owned by the wearer: 1"), Rune->PreLootRollCount));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRunePreLootRollUnbindTest,
    "Mythic.GAS.Rune.PreLootRollUnbind",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRunePreLootRollUnbindTest::RunTest(const FString &Parameters) {
    FMythicRuneAbilityTestFixture Fixture;
    const bool bReady = BuildRuneAbilityTestWorld(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }

    UMythicLootManagerSubsystem *LootManager = Fixture.GameInstance->GetSubsystem<UMythicLootManagerSubsystem>();
    AMythicRuneTestCharacter *Wearer = SpawnRuneAbilityTestPawn(Fixture.World, 100.0f);
    if (!TestNotNull(TEXT("the loot manager exists"), LootManager) || !TestNotNull(TEXT("the pawn spawned"), Wearer)) {
        return false;
    }
    AMythicPlayerController *Owner = SpawnRuneAbilityTestController(Fixture.World, Wearer);
    if (!TestNotNull(TEXT("the wearer's controller spawned"), Owner)) {
        return false;
    }

    FGameplayAbilitySpecHandle Handle;
    UMythicRuneTestLootAbility *Rune = Cast<UMythicRuneTestLootAbility>(
        GrantRuneAbilityTestRune(Wearer->AbilitySystem, UMythicRuneTestLootAbility::StaticClass(), Handle));
    if (!TestNotNull(TEXT("the loot rune was granted and instanced"), Rune)) {
        return false;
    }

    // Outside the event there is no bonus to edit. Each verb must say so and change nothing that a later roll reads.
    Rune->AddRuneLootDrops(5);
    Rune->SetRuneLootMinRarity(9);
    Rune->SetRuneLootDropScale(4.0f);
    TestFalse(TEXT("IsRuneLootForOwner outside the event owns nothing"), Rune->IsRuneLootForOwner());

    const FLootTierBonus Own = ULootReward::PrepareLootRoll(LootManager, Owner, 1, 0.0f);
    TestEqual(TEXT("the roll carries only the two drops the event asked for"), Own.ExtraDropCount, 2);
    TestEqual(TEXT("only the floor the event asked for"), Own.GuaranteedMinRarity, 3);
    TestEqual(TEXT("and only the scale the event asked for"), Own.DropCountScale, 0.0f);
    TestEqual(TEXT("the rune heard exactly one credit"), Rune->PreLootRollCount, 1);

    // Unsocketing must leave nothing behind on a subsystem that outlives the ability by a whole session.
    Wearer->AbilitySystem->ClearAbility(Handle);
    TestFalse(TEXT("unequipping unbinds the pre-loot-roll seam"), LootManager->OnPreLootRoll.IsBound());

    const FLootTierBonus AfterUnequip = ULootReward::PrepareLootRoll(LootManager, Owner, 1, 0.0f);
    TestEqual(TEXT("an unequipped rune adds no drops"), AfterUnequip.ExtraDropCount, 0);
    TestEqual(TEXT("floors no rarity"), AfterUnequip.GuaranteedMinRarity, 0);
    TestEqual(TEXT("and leaves the drop scale alone"), AfterUnequip.DropCountScale, 1.0f);
    TestEqual(TEXT("and hears nothing"), Rune->PreLootRollCount, 1);

    AddInfo(TEXT("verbs called outside the event: 4, credits they changed: 0; rolls after unequip: 1, heard: 0"));
    return true;
}
