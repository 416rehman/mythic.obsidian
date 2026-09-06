#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/Progression/MythicAbilityTestFixture.h"
#include "Tests/Progression/MythicAbilityWorldFixture.h"
#include "Input/MythicControlsComponent.h"
#include "GAS/MythicTags_GAS.h"
#include "Input/MythicTags_Input.h"
#include "Tests/Combat/MythicAbilityInputTestAbility.h"
#include "Player/MythicPlayerController.h"
#include "UI/Controls/MythicGameplayWheel.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/ScopeExit.h"

struct FMythicControlsJumpHandoffTestAccess {
    static void Initialize(UMythicControlsComponent* Controls,AMythicPlayerController* PC) {
        // No mapping registration or user-profile write. Exercise the authored default physical keys.
        Controls->bInitialized=true;
        Controls->EffectiveBindings=GetDefault<UMythicControlsSettings>()->Bindings;
        PC->OnPossessedPawnChanged.AddUniqueDynamic(Controls,&UMythicControlsComponent::HandlePawnChanged);
    }
    static UMythicGameplayWheel* OpenWheel(UMythicControlsComponent* Controls) {
        auto* Wheel=NewObject<UMythicGameplayWheel>(Controls->GetOwner());
        Controls->Wheel=Wheel; Wheel->ActivateWidget(); return Wheel;
    }
    static int32 JumpOwners(const UMythicControlsComponent* Controls) {
        return Controls->HeldActions.FindRef(EMythicControl::Jump);
    }
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicControlsJumpHandoff,
    "Mythic.Controls.JumpOverlayPhysicalOwner",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FMythicControlsJumpHandoff::RunTest(const FString&) {
    auto* Game=NewObject<UGameInstance>(GEngine); Game->InitializeStandalone();
    ON_SCOPE_EXIT { DestroyOwnedAbilityTestWorld(Game); };
    auto* World=Game->GetWorld();
    if (!World->AreActorsInitialized()) World->InitializeActorsForPlay(FURL());
    auto* PC=World->SpawnActor<AMythicPlayerController>();
    auto* State=World->SpawnActor<AMythicPlayerState>();
    if (!TestNotNull(TEXT("owned local controller"),PC) || !TestNotNull(TEXT("owned GAS state"),State)) return false;
    auto* Controls=PC->FindComponentByClass<UMythicControlsComponent>();
    if (!TestNotNull(TEXT("production input router"),Controls)) return false;
    FMythicControlsJumpHandoffTestAccess::Initialize(Controls,PC);
    InitializeAbilityTestPlayer(State);
    PC->PlayerState=State; State->SetOwner(PC); PC->SetPlayer(NewObject<ULocalPlayer>(GEngine));
    auto* ASC=State->GetMythicAbilitySystemComponent();
    const FKey Keys[]={EKeys::SpaceBar,EKeys::Gamepad_FaceButton_Bottom};
    const auto SpawnCharacter=[&]() -> ACharacter* {
        FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Character=World->SpawnActor<ACharacter>(FVector::ZeroVector,FRotator::ZeroRotator,Params);
        if (Character) {
            Character->JumpMaxHoldTime=.2f;
            Character->GetCharacterMovement()->JumpZVelocity=420.f;
            Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        }
        return Character;
    };
    const auto Possess=[&](ACharacter* Character) {
        PC->Possess(Character); ASC->InitAbilityActorInfo(State,Character);
    };
    const auto Open=[&](bool bWheel,TFunction<void()> DuringClose) {
        if (bWheel) {
            auto* Wheel=FMythicControlsJumpHandoffTestAccess::OpenWheel(Controls);
            Wheel->OnDeactivated().AddLambda([DuringClose]() { DuringClose(); });
        } else {
            Controls->OnCameraAdjustmentChanged.Clear();
            Controls->BeginCameraAdjustment();
            Controls->OnCameraAdjustmentChanged.AddLambda([DuringClose](bool bOpen) { if (!bOpen) DuringClose(); });
        }
    };
    const auto VerifyJump=[&](ACharacter* Character,FKey Key) {
        TestTrue(TEXT("physical press is consumed"),Controls->HandleKey(Key,IE_Pressed));
        TestFalse(TEXT("wheel closed before dispatch"),Controls->IsGameplayWheelOpen());
        TestFalse(TEXT("camera adjustment closed before dispatch"),Controls->IsCameraAdjustmentOpen());
        TestTrue(TEXT("jump remains pressed until a later real release"),Character->bPressedJump);
        TestEqual(TEXT("one physical jump owner"),FMythicControlsJumpHandoffTestAccess::JumpOwners(Controls),1);
        // The old synchronous Jump/StopJumping implementation cannot pass this real engine step.
        Character->CheckJumpInput(1.f/60.f);
        TestTrue(TEXT("Unreal actually applies upward jump velocity"),Character->GetCharacterMovement()->Velocity.Z>0.f);
        TestEqual(TEXT("one actual engine jump"),Character->JumpCurrentCount,1);
        Controls->HandleKey(Key,IE_Pressed);
        TestEqual(TEXT("duplicate key-down cannot add a second owner"),FMythicControlsJumpHandoffTestAccess::JumpOwners(Controls),1);
        TestTrue(TEXT("real release consumed"),Controls->HandleKey(Key,IE_Released));
        TestFalse(TEXT("real release clears jump hold"),Character->bPressedJump);
    };
    // Both actual overlay close callbacks include the controller's synchronous flush.
    for (bool bWheel:{false,true}) for (FKey Key:Keys) {
        auto* Character=SpawnCharacter(); if (!TestNotNull(TEXT("jump character"),Character)) return false;
        Possess(Character); Open(bWheel,[&]() { PC->FlushPressedKeys(); });
        VerifyJump(Character,Key);
    }
    // Explicit cancellation during close must defeat the handoff even though UI flush itself is exempt.
    for (bool bWheel:{false,true}) {
        auto* Character=SpawnCharacter(); if (!Character) return false;
        Possess(Character); Open(bWheel,[&]() { Controls->CancelHeldInput(); });
        Controls->HandleKey(EKeys::SpaceBar,IE_Pressed); Character->CheckJumpInput(1.f/60.f);
        TestFalse(TEXT("explicit cancellation cannot resurrect a jump"),Character->bPressedJump);
        TestEqual(TEXT("cancelled close performs no engine jump"),Character->JumpCurrentCount,0);
        Controls->HandleKey(EKeys::SpaceBar,IE_Released);
    }
    {
        auto* Character=SpawnCharacter(); if (!Character) return false;
        Possess(Character); Open(true,[&]() { ASC->AddLooseGameplayTag(GAS_STATE_DEAD); });
        Controls->HandleKey(EKeys::SpaceBar,IE_Pressed);
        TestFalse(TEXT("death during close rejects traversal"),Character->bPressedJump);
        ASC->RemoveLooseGameplayTag(GAS_STATE_DEAD);
        Controls->HandleKey(EKeys::SpaceBar,IE_Pressed);
        TestFalse(TEXT("held dead-state key cannot become a fresh press"),Character->bPressedJump);
        Controls->HandleKey(EKeys::SpaceBar,IE_Released);
    }
    {
        auto* Character=SpawnCharacter(); if (!Character) return false;
        Possess(Character); Open(false,[&]() { Controls->HandleKey(EKeys::SpaceBar,IE_Released); });
        Controls->HandleKey(EKeys::SpaceBar,IE_Pressed);
        TestFalse(TEXT("a reentrant physical release cannot be replayed as a press"),Character->bPressedJump);
        VerifyJump(Character,EKeys::SpaceBar); // Already observed release must not suppress the next genuine press.
    }
    {
        auto* Original=SpawnCharacter(); auto* Replacement=SpawnCharacter();
        if (!Original || !Replacement) return false;
        Possess(Original); Open(true,[&]() { Possess(Replacement); });
        Controls->HandleKey(EKeys::SpaceBar,IE_Pressed);
        TestFalse(TEXT("handoff never jumps departed pawn"),Original->bPressedJump);
        TestFalse(TEXT("handoff never jumps replacement pawn"),Replacement->bPressedJump);
        Controls->HandleKey(EKeys::SpaceBar,IE_Released);
        Controls->HandleKey(EKeys::SpaceBar,IE_Pressed);
        TestTrue(TEXT("fresh input after replacement owns its real pawn"),Replacement->bPressedJump);
        Possess(Original);
        TestFalse(TEXT("possession cancellation stops captured previous pawn"),Replacement->bPressedJump);
        Original->Jump(); // An independent owner must not be stopped by the departed physical press.
        Controls->HandleKey(EKeys::SpaceBar,IE_Released);
        TestTrue(TEXT("stale release cannot stop new pawn's independent jump"),Original->bPressedJump);
        Original->StopJumping();
    }
    // Real GAS input admission, distinct from the direct Character::Jump route above.
    // No root-motion/terrain claim: the native probe proves this exact physical Dash
    // edge reaches GAS despite the previous frame's real suppressed=true state.
    const auto GiveDashInputProbe=[&]() {
        FGameplayAbilitySpec Spec(UMythicAbilityInputTestAbility::StaticClass(),1,INDEX_NONE,Controls);
        Spec.GetDynamicSpecSourceTags().AddTag(INPUT_ACTION_DODGE);
        return ASC->GiveAbility(Spec);
    };
    for (bool bWheel:{false,true}) for (FKey Key:{EKeys::LeftShift,EKeys::Gamepad_FaceButton_Right}) {
        auto* Character=SpawnCharacter(); if (!Character) return false;
        Possess(Character);
        const auto Handle=GiveDashInputProbe();
        auto* Probe=Cast<UMythicAbilityInputTestAbility>(ASC->FindAbilitySpecFromHandle(Handle)->GetPrimaryInstance());
        if (!TestNotNull(TEXT("real GAS dash-input instance"),Probe)) return false;
        Open(bWheel,[&]() { PC->FlushPressedKeys(); });
        ASC->SetAbilityInputSuppressed(PC->IsGameplayAbilityInputBlocked());
        TestFalse(TEXT("open overlay really suppresses GAS admission"),ASC->CanAcceptAbilityInput());
        TestTrue(TEXT("dash physical press consumed"),Controls->HandleKey(Key,IE_Pressed));
        TestFalse(TEXT("closed overlay permits current controller gameplay"),PC->IsGameplayAbilityInputBlocked());
        TestTrue(TEXT("same edge refreshes stale ASC suppression before the next controller frame"),ASC->CanAcceptAbilityInput());
        ASC->ProcessAbilityInput(1.f/60.f,false);
        TestEqual(TEXT("exactly one real GAS activation after overlay close"),Probe->Trace.Num(),1);
        if (Probe->Trace.Num()==1) TestEqual(TEXT("first dispatch activates"),Probe->Trace[0],FName(TEXT("Activate")));
        Controls->HandleKey(Key,IE_Released); ASC->ProcessAbilityInput(1.f/60.f,false);
        TestEqual(TEXT("real release reaches the same GAS owner"),Probe->Trace.Num(),2);
        if (Probe->Trace.Num()==2) TestEqual(TEXT("second dispatch releases"),Probe->Trace[1],FName(TEXT("Release")));
        ASC->ClearAbility(Handle); Controls->CancelHeldInput();
    }
    {
        auto* Character=SpawnCharacter(); if (!Character) return false;
        Possess(Character); const auto Handle=GiveDashInputProbe();
        auto* Probe=Cast<UMythicAbilityInputTestAbility>(ASC->FindAbilitySpecFromHandle(Handle)->GetPrimaryInstance());
        if (!Probe) return false;
        Open(true,[]() {}); ASC->SetAbilityInputSuppressed(true); ASC->AddLooseGameplayTag(GAS_INPUT_BLOCKED);
        Controls->HandleKey(EKeys::Gamepad_FaceButton_Right,IE_Pressed); ASC->ProcessAbilityInput(1.f/60.f,false);
        TestFalse(TEXT("fresh controller gate never overrides GAS blocked-input tag"),ASC->CanAcceptAbilityInput());
        TestTrue(TEXT("tag-blocked close admits no GAS activation"),Probe->Trace.IsEmpty());
        Controls->HandleKey(EKeys::Gamepad_FaceButton_Right,IE_Released);
        ASC->RemoveLooseGameplayTag(GAS_INPUT_BLOCKED);
        Controls->HandleKey(EKeys::Gamepad_FaceButton_Right,IE_Pressed); ASC->ProcessAbilityInput(1.f/60.f,false);
        TestEqual(TEXT("fresh physical edge works after blocker removed"),Probe->Trace.Num(),1);
        Controls->HandleKey(EKeys::Gamepad_FaceButton_Right,IE_Released); ASC->ProcessAbilityInput(1.f/60.f,false);
        ASC->ClearAbility(Handle); Controls->CancelHeldInput();
    }
    {
        auto* Character=SpawnCharacter(); if (!Character) return false;
        Possess(Character); const auto Handle=GiveDashInputProbe();
        auto* Probe=Cast<UMythicAbilityInputTestAbility>(ASC->FindAbilitySpecFromHandle(Handle)->GetPrimaryInstance());
        if (!Probe) return false;
        Open(true,[]() {}); ASC->SetAbilityInputSuppressed(true);
        bool CanceledDuringProjection=false;
        const auto Notification=ASC->OnAbilityProjectionChanged.AddLambda([&]() {
            if (!CanceledDuringProjection && !PC->IsGameplayAbilityInputBlocked()) {
                CanceledDuringProjection=true; Controls->CancelHeldInput();
            }
        });
        Controls->HandleKey(EKeys::LeftShift,IE_Pressed);
        ASC->OnAbilityProjectionChanged.Remove(Notification);
        ASC->ProcessAbilityInput(1.f/60.f,false);
        TestTrue(TEXT("suppression refresh actually invoked reentrant projection observer"),CanceledDuringProjection);
        TestTrue(TEXT("observer cancellation cannot be resurrected into GAS input"),Probe->Trace.IsEmpty());
        Controls->HandleKey(EKeys::LeftShift,IE_Released);
        Controls->HandleKey(EKeys::LeftShift,IE_Pressed); ASC->ProcessAbilityInput(1.f/60.f,false);
        TestEqual(TEXT("fresh press after canceled handoff still activates"),Probe->Trace.Num(),1);
        Controls->HandleKey(EKeys::LeftShift,IE_Released); ASC->ProcessAbilityInput(1.f/60.f,false);
        ASC->ClearAbility(Handle); Controls->CancelHeldInput();
    }
    Controls->OnCameraAdjustmentChanged.Clear(); Controls->CancelHeldInput(); Controls->CloseWheel(false);
    return true;
}
#endif
