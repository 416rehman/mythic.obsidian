#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/Progression/MythicAbilityTestFixture.h"
#include "Tests/Progression/MythicAbilityWorldFixture.h"
#include "Input/MythicControlsComponent.h"
#include "Input/MythicControlUserSettings.h"
#include "Player/MythicPlayerController.h"
#include "UI/Controls/MythicGameplayWheel.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "GameFramework/Character.h"
#include "Misc/ScopeExit.h"

struct FMythicControlsWheelMappingTestAccess {
    static void Initialize(UMythicControlsComponent* Controls,UEnhancedInputUserSettings* Profile,
        UEnhancedInputLocalPlayerSubsystem* Notifications) {
        Controls->bInitialized=true; Controls->UserSettings=Profile;
        Controls->EffectiveBindings=GetDefault<UMythicControlsSettings>()->Bindings;
        Notifications->ControlMappingsRebuiltDelegate.AddUniqueDynamic(Controls,&UMythicControlsComponent::HandleMappingsRebuilt);
    }
    static FMythicWheelOpenToken AttachOpenedWheel(UMythicControlsComponent* Controls,FKey Key,APawn* Pawn) {
        auto* Press=Controls->Presses.Find(Key); if (!Press) return FMythicWheelOpenToken();
        Press->bWheelOpened=true;
        auto* Wheel=NewObject<UMythicGameplayWheel>(Controls->GetOwner());
        Controls->Wheel=Wheel; Wheel->ActivateWidget();
        FMythicWheelOpenToken Token; Token.InputEpoch=Controls->InputEpoch; Token.Key=Key;
        Token.StartedAt=Press->StartedAt; Token.Control=Press->Control; Token.Pawn=Pawn;
        return Token;
    }
    static bool OwnsOpener(const UMythicControlsComponent* Controls,FKey Key) { return Controls->Presses.Contains(Key); }
    static UMythicGameplayWheel* Wheel(const UMythicControlsComponent* Controls) { return Controls->Wheel.Get(); }
    static uint64 Epoch(const UMythicControlsComponent* Controls) { return Controls->InputEpoch; }
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicControlsWheelMappingOwnership,
    "Mythic.Controls.WheelMappingOwnership",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FMythicControlsWheelMappingOwnership::RunTest(const FString&) {
    auto* Game=NewObject<UGameInstance>(GEngine); Game->InitializeStandalone();
    ON_SCOPE_EXIT { DestroyOwnedAbilityTestWorld(Game); };
    auto* World=Game->GetWorld();
    if (!World->AreActorsInitialized()) World->InitializeActorsForPlay(FURL());
    auto* PC=World->SpawnActor<AMythicPlayerController>();
    auto* State=World->SpawnActor<AMythicPlayerState>();
    if (!TestNotNull(TEXT("owned controller"),PC) || !TestNotNull(TEXT("owned state"),State)) return false;
    auto* Controls=PC->FindComponentByClass<UMythicControlsComponent>();
    if (!TestNotNull(TEXT("production controls"),Controls)) return false;
    // Real in-memory profile; no SaveSettings, no user mapping registration.
    auto* Local=NewObject<ULocalPlayer>(GEngine);
    auto* Settings=NewObject<UMythicControlUserSettings>(); Settings->Initialize(Local);
    auto* Context=NewObject<UInputMappingContext>(); auto* Action=NewObject<UInputAction>();
    auto& Mapping=Context->MapKey(Action,EKeys::Z);
    TestTrue(TEXT("heal mapping metadata"),MythicControlMapping::Configure(Mapping,Context,
        UMythicControlsSettings::MappingName(EMythicControl::Heal,false),FText::GetEmpty()));
    if (!TestTrue(TEXT("in-memory authored mapping registered"),Settings->RegisterInputMappingContext(Context))) return false;
    // Exercise the exact engine notification delegate. This isolates router lifecycle;
    // actual CommonUI context changes and physical Slate delivery still need PIE.
    auto* Notifications=NewObject<UEnhancedInputLocalPlayerSubsystem>(Local);
    FMythicControlsWheelMappingTestAccess::Initialize(Controls,Settings,Notifications);
    ON_SCOPE_EXIT { Notifications->ControlMappingsRebuiltDelegate.Clear(); Controls->CancelHeldInput(); };
    PC->SetPlayer(Local);
    InitializeAbilityTestPlayer(State); PC->PlayerState=State; State->SetOwner(PC);
    auto* Pawn=World->SpawnActor<ACharacter>();
    if (!TestNotNull(TEXT("owned pawn"),Pawn)) return false;
    PC->Possess(Pawn); State->GetMythicAbilitySystemComponent()->InitAbilityActorInfo(State,Pawn);
    const auto Open=[&](FKey Key) {
        if (!TestTrue(TEXT("real opener press accepted"),Controls->HandleKey(Key,IE_Pressed))) return FMythicWheelOpenToken();
        return FMythicControlsWheelMappingTestAccess::AttachOpenedWheel(Controls,Key,Pawn);
    };
    auto Token=Open(EKeys::Z);
    TestTrue(TEXT("initial exact opener generation valid"),Controls->IsWheelOpenCurrent(Token));
    Notifications->ControlMappingsRebuiltDelegate.Broadcast();
    Notifications->ControlMappingsRebuiltDelegate.Broadcast();
    TestTrue(TEXT("unrelated context notifications preserve opener epoch and ownership"),Controls->IsWheelOpenCurrent(Token));
    TestTrue(TEXT("unchanged profile keeps wheel active"),Controls->IsGameplayWheelOpen());
    TestTrue(TEXT("original release still reaches its owner"),Controls->HandleKey(EKeys::Z,IE_Released));
    TestFalse(TEXT("release closes wheel"),Controls->IsGameplayWheelOpen());
    TestFalse(TEXT("released opener retired"),FMythicControlsWheelMappingTestAccess::OwnsOpener(Controls,EKeys::Z));

    Token=Open(EKeys::Z);
    FMapPlayerKeyArgs Args; Args.MappingName=Mapping.GetMappingName(); Args.ProfileIdString=Settings->GetActiveKeyProfileId();
    Args.Slot=EPlayerMappableKeySlot::First; Args.NewKey=EKeys::H; Args.bCreateMatchingSlotIfNeeded=false;
    FGameplayTagContainer Failures; Settings->MapPlayerKey(Args,Failures);
    if (!TestTrue(TEXT("real profile remap accepted"),Failures.IsEmpty())) return false;
    bool SawChangedPublication=false;
    const auto Observe=Controls->OnControlsChanged.AddLambda([&]() {
        if (Controls->GetEffectiveKey(EMythicControl::Heal,false)==EKeys::H) {
            SawChangedPublication=true;
            TestFalse(TEXT("remap observers never see an orphan wheel"),Controls->IsGameplayWheelOpen());
        }
    });
    Notifications->ControlMappingsRebuiltDelegate.Broadcast(); Controls->OnControlsChanged.Remove(Observe);
    TestTrue(TEXT("new key published"),SawChangedPublication);
    TestFalse(TEXT("real remap invalidates old opener token"),Controls->IsWheelOpenCurrent(Token));
    TestFalse(TEXT("real remap closes wheel without commit"),Controls->IsGameplayWheelOpen());
    TestTrue(TEXT("old key release is quarantined"),Controls->HandleKey(EKeys::Z,IE_Released));
    TestFalse(TEXT("stale up never resurrects wheel"),Controls->IsGameplayWheelOpen());

    Token=Open(EKeys::H);
    bool SawCloseCallback=false;
    auto* ClosingWheel=FMythicControlsWheelMappingTestAccess::Wheel(Controls);
    const auto OnClose=ClosingWheel->OnDeactivated().AddLambda([&]() {
        SawCloseCallback=true;
        const uint64 BeforeNested=FMythicControlsWheelMappingTestAccess::Epoch(Controls);
        Controls->CancelHeldInput();
        TestTrue(TEXT("nested cancellation still invalidates pending generations"),
            FMythicControlsWheelMappingTestAccess::Epoch(Controls)>BeforeNested);
        TestTrue(TEXT("reentrant alternate opener is quarantined"),Controls->HandleKey(EKeys::Gamepad_DPad_Up,IE_Pressed));
        TestFalse(TEXT("deactivation cannot create a replacement press owner"),
            FMythicControlsWheelMappingTestAccess::OwnsOpener(Controls,EKeys::Gamepad_DPad_Up));
        TestTrue(TEXT("real reentrant release still drains its key"),Controls->HandleKey(EKeys::Gamepad_DPad_Up,IE_Released));
        TestTrue(TEXT("second reentrant press remains quarantined until a later up"),Controls->HandleKey(EKeys::Gamepad_DPad_Up,IE_Pressed));
    });
    Controls->CancelHeldInput(); ClosingWheel->OnDeactivated().Remove(OnClose);
    TestTrue(TEXT("actual native deactivation callback ran"),SawCloseCallback);
    TestFalse(TEXT("generic cancellation closes wheel"),Controls->IsGameplayWheelOpen());
    TestFalse(TEXT("generic cancellation invalidates opener"),Controls->IsWheelOpenCurrent(Token));
    TestFalse(TEXT("cancellation leaves no alternate input owner"),
        FMythicControlsWheelMappingTestAccess::OwnsOpener(Controls,EKeys::Gamepad_DPad_Up));
    TestTrue(TEXT("canceled release is drained"),Controls->HandleKey(EKeys::H,IE_Released));
    TestTrue(TEXT("late alternate release is drained"),Controls->HandleKey(EKeys::Gamepad_DPad_Up,IE_Released));
    TestFalse(TEXT("repeated release has no new owner"),Controls->HandleKey(EKeys::H,IE_Released));
    Token=Open(EKeys::Gamepad_DPad_Up);
    TestTrue(TEXT("fresh key after cancellation owns a new valid wheel generation"),Controls->IsWheelOpenCurrent(Token));
    TestTrue(TEXT("fresh generation receives its physical release"),Controls->HandleKey(EKeys::Gamepad_DPad_Up,IE_Released));
    TestFalse(TEXT("fresh generation closes normally"),Controls->IsGameplayWheelOpen());
    return true;
}
#endif
