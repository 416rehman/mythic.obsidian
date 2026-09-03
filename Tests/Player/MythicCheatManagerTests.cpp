#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameplayEffect.h"
#include "Misc/ScopeExit.h"
#include "ScalableFloat.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GameModes/MythicCheatManager.h"

namespace {

// The cheat manager needs only a controller whose pawn owns an initialised life component; no console is involved.
struct FMythicCheatHealthFixture {
    UGameInstance *GameInstance = nullptr;
    UWorld *World = nullptr;
    APlayerController *Controller = nullptr;
    ACharacter *Pawn = nullptr;
    UMythicAbilitySystemComponent *ASC = nullptr;
    UMythicAttributeSet_Life *LifeSet = nullptr;
    UMythicLifeComponent *Life = nullptr;
    UMythicCheatManager *Cheats = nullptr;
};

bool BuildCheatHealthFixture(FAutomationTestBase &Test, FMythicCheatHealthFixture &Out) {
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
    Out.Controller = Out.World->SpawnActor<APlayerController>();
    if (!Test.TestNotNull(TEXT("the player controller spawned"), Out.Controller)) {
        return false;
    }
    // A controller spawned without a GameMode never gets a PlayerState, and IsPlayerControlled reads it.
    APlayerState *PlayerState = Out.World->SpawnActor<APlayerState>();
    if (!Test.TestNotNull(TEXT("the player state spawned"), PlayerState)) {
        return false;
    }
    Out.Controller->PlayerState = PlayerState;
    Out.Controller->Possess(Out.Pawn);
    if (!Test.TestTrue(TEXT("the controller holds the pawn"), Out.Controller->GetPawn() == Out.Pawn)) {
        return false;
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

    Out.Cheats = NewObject<UMythicCheatManager>(Out.Controller);
    if (!Test.TestNotNull(TEXT("the cheat manager exists"), Out.Cheats)) {
        return false;
    }
    return Test.TestEqual(TEXT("the character starts at full health"), Out.Life->GetHealth(), Out.Life->GetMaxHealth());
}

void ApplyCheatTestDamage(UAbilitySystemComponent *ASC, const float Amount) {
    UGameplayEffect *Effect = NewObject<UGameplayEffect>(GetTransientPackage());
    Effect->DurationPolicy = EGameplayEffectDurationType::Instant;

    FGameplayModifierInfo Mod;
    Mod.Attribute = UMythicAttributeSet_Life::GetDamageAttribute();
    Mod.ModifierOp = EGameplayModOp::Additive;
    Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Amount));
    Effect->Modifiers.Add(Mod);

    ASC->ApplyGameplayEffectToSelf(Effect, 1.0f, ASC->MakeEffectContext());
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCheatSetHealthTest,
    "Mythic.Player.Cheats.SetHealth",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCheatSetHealthTest::RunTest(const FString &Parameters) {
    FMythicCheatHealthFixture Fixture;
    const bool bReady = BuildCheatHealthFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicCheatManager *Cheats = Fixture.Cheats;
    UMythicLifeComponent *Life = Fixture.Life;
    const float MaxHealth = Life->GetMaxHealth();
    if (!TestTrue(TEXT("max health is positive"), MaxHealth > 0.0f)) {
        return false;
    }

    Cheats->MythSetHealth(0.25f);
    TestEqual(TEXT("a fraction sets health to that share of max"), Life->GetHealth(), MaxHealth * 0.25f, KINDA_SMALL_NUMBER);

    Cheats->MythSetHealth(40.0f);
    TestEqual(TEXT("a value above 1 reads as a percent"), Life->GetHealth(), MaxHealth * 0.4f, KINDA_SMALL_NUMBER);

    Cheats->MythSetHealth(250.0f);
    TestEqual(TEXT("a percent above 100 clamps to full"), Life->GetHealth(), MaxHealth, KINDA_SMALL_NUMBER);

    Cheats->MythSetHealth(-0.5f);
    TestEqual(TEXT("a negative fraction clamps to empty"), Life->GetHealth(), 0.0f, KINDA_SMALL_NUMBER);
    TestTrue(TEXT("the death latch follows the write to empty"), Fixture.LifeSet->IsDead());

    // The reason the cheat exists: MythSetAttribute Health leaves the latch where the last hit put it.
    Cheats->MythSetHealth(1.0f);
    TestEqual(TEXT("restoring to full refills health"), Life->GetHealth(), MaxHealth, KINDA_SMALL_NUMBER);
    TestFalse(TEXT("the death latch clears with the restore"), Fixture.LifeSet->IsDead());

    ApplyCheatTestDamage(Fixture.ASC, 10.0f);
    TestEqual(TEXT("an ordinary hit lands after the restore"), Life->GetHealth(), MaxHealth - 10.0f, KINDA_SMALL_NUMBER);

    AddInfo(FString::Printf(TEXT("max health %.1f; fraction, percent, over-100, negative and restore writes checked: 5"),
                            MaxHealth));
    return true;
}
