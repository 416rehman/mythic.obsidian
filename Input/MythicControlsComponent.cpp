#include "MythicControlsComponent.h"
#include "MythicQuickUseComponent.h"
#include "MythicControlUserSettings.h"
#include "MythicTags_Input.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "PlayerMappableKeySettings.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicTags_GAS.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicAbilityIntentComponent.h"
#include "Input/MythicTraversalComponent.h"
#include "Input/MythicCombatUtilityComponent.h"
#include "UI/Controls/MythicGameplayWheel.h"
#include "UI/Controls/MythicControlsPage.h"
#include "UI/MythicHUDLayout.h"
#include "UI/MythicUIManagerSubsystem.h"
#include "UI/Nameplate/MythicNameplateDirector.h"
#include "UI/Settings/MythicUserSettings.h"
#include "World/Harvesting/MythicHarvestFocusComponent.h"
#include "TimerManager.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

UMythicControlsComponent::UMythicControlsComponent() { PrimaryComponentTick.bCanEverTick=false; }
AMythicPlayerController* UMythicControlsComponent::Player() const { return Cast<AMythicPlayerController>(GetOwner()); }
UMythicAbilitySystemComponent* UMythicControlsComponent::AbilitySystem() const { auto* PC=Player(); return PC ? Cast<UMythicAbilitySystemComponent>(PC->GetAbilitySystemComponent()):nullptr; }
void UMythicControlsComponent::BeginPlay() {
    Super::BeginPlay();
    if (auto* PC=Player()) PC->OnPossessedPawnChanged.AddDynamic(this,&ThisClass::HandlePawnChanged);
    InitializeLocalInput();
}
void UMythicControlsComponent::EndPlay(const EEndPlayReason::Type Reason) {
    CancelHeldInput(); CloseWheel(false);
    if (auto* PC=Player()) PC->OnPossessedPawnChanged.RemoveDynamic(this,&ThisClass::HandlePawnChanged);
    if (InputSubsystem.IsValid()) { InputSubsystem->ControlMappingsRebuiltDelegate.RemoveDynamic(this,&ThisClass::HandleMappingsRebuilt); if (MappingContext) InputSubsystem->RemoveMappingContext(MappingContext); }
    // LocalPlayer settings survive controller replacement; retain profile rows, not retired contexts.
    if (UserSettings.IsValid() && MappingContext) UserSettings->UnregisterInputMappingContext(MappingContext);
    Super::EndPlay(Reason);
}
void UMythicControlsComponent::HandlePawnChanged(APawn*,APawn*) { CancelHeldInput(); CloseWheel(false); InitializeLocalInput(); }
void UMythicControlsComponent::InitializeLocalInput() {
    auto* PC=Player(); if (bInitialized || !PC || !PC->IsLocalController() || !PC->GetLocalPlayer()) return;
    auto* Subsystem=PC->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(); if (!Subsystem) return;
    // SetPlayer calls SetupInputComponent before ReceivedPlayer initializes the subsystem's settings.
    // An earlier BeginPlay must leave this pending until the real Enhanced PlayerInput exists.
    if (!Subsystem->GetPlayerInput()) return;
    if (!Subsystem->GetUserSettings()) Subsystem->InitalizeUserSettings();
    auto* Settings=Subsystem->GetUserSettings();
    if (!Settings || !Settings->GetActiveKeyProfile()) return;
    InputSubsystem=Subsystem; UserSettings=Settings;
    MappingContext=NewObject<UInputMappingContext>(this,TEXT("CombatControlMappings"));
    for (const auto& Binding:GetDefault<UMythicControlsSettings>()->Bindings) {
        auto* Action=NewObject<UInputAction>(this,*FString::Printf(TEXT("Control_%d"),static_cast<int32>(Binding.Control)));
        Action->ValueType=EInputActionValueType::Boolean; Action->bConsumeInput=true;
        Actions.Add(Binding.Control,Action);
        for (bool bGamepad:{false,true}) {
            const FKey Key=bGamepad ? Binding.Gamepad:Binding.Keyboard; if (!Key.IsValid()) continue;
            auto& Mapping=MappingContext->MapKey(Action,Key);
            ensure(MythicControlMapping::Configure(Mapping,MappingContext,UMythicControlsSettings::MappingName(Binding.Control,bGamepad),Binding.Label));
        }
    }
    if (UserSettings.IsValid()) UserSettings->RegisterInputMappingContext(MappingContext);
    Subsystem->ControlMappingsRebuiltDelegate.AddUniqueDynamic(this,&ThisClass::HandleMappingsRebuilt);
    FModifyContextOptions Options; Options.bNotifyUserSettings=true; Options.bIgnoreAllPressedKeysUntilRelease=true;
    Subsystem->AddMappingContext(MappingContext,1000,Options);
    bInitialized=true; RefreshMappings();
}
void UMythicControlsComponent::HandleMappingsRebuilt() { if (bEditingMappings) return; RefreshMappings(); }
void UMythicControlsComponent::RefreshMappings() {
    auto Resolved=GetDefault<UMythicControlsSettings>()->Bindings;
    const auto* Profile=UserSettings.IsValid() ? UserSettings->GetActiveKeyProfile():nullptr;
    if (Profile) for (auto& Binding:Resolved) for (bool bGamepad:{false,true}) {
        const auto* Row=Profile->FindKeyMappingRow(UMythicControlsSettings::MappingName(Binding.Control,bGamepad));
        if (Row) for (const auto& Mapping:Row->Mappings) if (Mapping.GetSlot()==EPlayerMappableKeySlot::First) { (bGamepad ? Binding.Gamepad:Binding.Keyboard)=Mapping.GetCurrentKey(); break; }
    }
    // UI/camera context changes also rebuild Enhanced Input. They do not reassign
    // the physical press already owned by this router, including a wheel opener.
    bool bPhysicalBindingsChanged=Resolved.Num()!=EffectiveBindings.Num();
    for (int32 Index=0;!bPhysicalBindingsChanged && Index<Resolved.Num();++Index) {
        const auto& Before=EffectiveBindings[Index]; const auto& After=Resolved[Index];
        bPhysicalBindingsChanged=Before.Control!=After.Control || Before.Keyboard!=After.Keyboard
            || Before.Gamepad!=After.Gamepad || Before.bGamepadAbilityLayer!=After.bGamepadAbilityLayer;
    }
    // Cancellation finishes before publishing the new mapping to observers.
    if (bPhysicalBindingsChanged) CancelHeldInput();
    EffectiveBindings=MoveTemp(Resolved);
    OnControlsChanged.Broadcast();
}
bool UMythicControlsComponent::IsSkill(EMythicControl C) { return C>=EMythicControl::SkillQ && C<=EMythicControl::SkillT; }
bool UMythicControlsComponent::IsMovement(EMythicControl C) { return C>=EMythicControl::MoveForward && C<=EMythicControl::MoveRight; }
bool UMythicControlsComponent::IsWheel(EMythicControl C) { return C==EMythicControl::Heal || C==EMythicControl::Utility || C==EMythicControl::Ping; }
EMythicControl UMythicControlsComponent::ResolvePress(const TArray<FMythicControlBinding>& Bindings,FKey Key,bool bLayerHeld) {
    if (!Key.IsValid()) return EMythicControl::Count;
    if (Key.IsGamepadKey() && bLayerHeld) for (const auto& Binding:Bindings) if (Binding.bGamepadAbilityLayer && Binding.Gamepad==Key) return Binding.Control;
    for (const auto& Binding:Bindings) if (Key.IsGamepadKey() ? (!Binding.bGamepadAbilityLayer && Binding.Gamepad==Key) : Binding.Keyboard==Key) return Binding.Control;
    return EMythicControl::Count;
}
FKey UMythicControlsComponent::GetEffectiveKey(EMythicControl C,bool bGamepad) const { const auto* B=EffectiveBindings.FindByPredicate([C](const auto& B){return B.Control==C;}); return B ? (bGamepad ? B->Gamepad:B->Keyboard):EKeys::Invalid; }
FText UMythicControlsComponent::GetBindingLabel(EMythicControl C,bool bGamepad) const {
    const auto Key=GetEffectiveKey(C,bGamepad); if (!Key.IsValid()) return FText::GetEmpty();
    if (bGamepad && IsSkill(C)) return FText::Format(NSLOCTEXT("Controls","Chord","{0} + {1}"),GetEffectiveKey(EMythicControl::AbilityLayer,true).GetDisplayName(),Key.GetDisplayName());
    return Key.GetDisplayName();
}
const UInputAction* UMythicControlsComponent::GetInputAction(EMythicControl C) const { const auto* Found=Actions.Find(C); return Found ? Found->Get():nullptr; }
bool UMythicControlsComponent::IsControlKey(FKey Key,EMythicControl C) const { return Key.IsValid() && Key==GetEffectiveKey(C,Key.IsGamepadKey()); }
FGameplayTag UMythicControlsComponent::GetAbilityInputTag(EMythicControl C) const {
    switch(C) {
    case EMythicControl::Primary:return INPUT_ACTION_ATTACK; case EMythicControl::Secondary:return INPUT_ACTION_ATTACK_ALT;
    case EMythicControl::Dash:return INPUT_ACTION_DODGE;
    case EMythicControl::SkillQ:return INPUT_ACTION_SKILL_Q; case EMythicControl::SkillE:return INPUT_ACTION_SKILL_E;
    case EMythicControl::SkillR:return INPUT_ACTION_SKILL_R; case EMythicControl::SkillT:return INPUT_ACTION_SKILL_T;
    default:return FGameplayTag();
    }
}
bool UMythicControlsComponent::IsAbilityLayerHeld() const { return HeldActions.FindRef(EMythicControl::AbilityLayer)>0; }
bool UMythicControlsComponent::IsGameplayWheelOpen() const { return bOpeningWheel || (Wheel && Wheel->IsActivated()); }
bool UMythicControlsComponent::HandleKey(FKey Key,EInputEvent Event) {
    if (!bInitialized) InitializeLocalInput();
    // A synchronous UI callback may reenter input while this key has no owner yet.
    if (bTraversalCloseHandoff && Key==TraversalCloseKey) {
        if (Event==IE_Released) bTraversalCloseReleased=true;
        return true;
    }
    if (Event==IE_Released) {
        if (SuppressedKeys.Remove(Key)) return true;
        FMythicControlPress Press; if (!Presses.RemoveAndCopyValue(Key,Press)) return false;
        int32& Count=HeldActions.FindOrAdd(Press.Control); Count=FMath::Max(0,Count-1);
        if (Count==0) {
            if (IsWheel(Press.Control)) { if (Press.bWheelOpened) CloseWheel(true); else if (!IsGameplayWheelOpen() && FPlatformTime::Seconds()-Press.StartedAt<GetDefault<UMythicControlsSettings>()->WheelHoldSeconds) DispatchRelease(Press.Control,false); }
            else DispatchRelease(Press.Control,false);
        }
        return true;
    }
    // Cancellation may synchronously close UI or dispatch gameplay callbacks.
    // New presses cannot replace owners while their old releases are pending.
    // Real releases above still drain quarantine, including reentrant releases.
    if (bCancelingInput) {
        if (ResolvePress(EffectiveBindings,Key,IsAbilityLayerHeld())==EMythicControl::Count) return false;
        SuppressedKeys.Add(Key); return true;
    }
    if (Event!=IE_Pressed || Presses.Contains(Key) || SuppressedKeys.Contains(Key)) return Presses.Contains(Key) || SuppressedKeys.Contains(Key);
    const auto Control=ResolvePress(EffectiveBindings,Key,IsAbilityLayerHeld()); if (Control==EMythicControl::Count) return false;
    auto* PC=Player(); if (!PC) return false;
    if ((bCameraAdjustment || IsGameplayWheelOpen()) && (Control==EMythicControl::Jump || Control==EMythicControl::Dash)) {
        if (!CloseOverlayForTraversal(Key)) return true;
        // Continue into the ordinary physical-key owner below. Release only on the real edge.
    }
    if (bCameraAdjustment) {
        if (Control==EMythicControl::Utility) { EndCameraAdjustment(); SuppressedKeys.Add(Key); return true; }
        if (Control==EMythicControl::CharacterMenu || Control==EMythicControl::SystemMenu) EndCameraAdjustment();
        else if (Control==EMythicControl::Primary || Control==EMythicControl::Secondary || IsMovement(Control)) {
            FMythicControlPress Press; Press.Control=Control; Press.StartedAt=FPlatformTime::Seconds(); Presses.Add(Key,Press); ++HeldActions.FindOrAdd(Control); return true;
        } else { SuppressedKeys.Add(Key); return true; }
    }
    if (IsGameplayWheelOpen()) {
        if (!IsMovement(Control)) { SuppressedKeys.Add(Key); return true; }
    } else if (PC->IsGameplayAbilityInputBlocked() && Control!=EMythicControl::SystemMenu && Control!=EMythicControl::CharacterMenu) { SuppressedKeys.Add(Key); return true; }
    FMythicControlPress Press; Press.Control=Control; Press.StartedAt=FPlatformTime::Seconds(); Presses.Add(Key,Press);
    if (++HeldActions.FindOrAdd(Control)==1) DispatchPress(Control);
    if (IsWheel(Control)) GetWorld()->GetTimerManager().SetTimer(HoldTimer,this,&ThisClass::AdvanceHoldActions,.01f,true);
    return true;
}
void UMythicControlsComponent::DispatchPress(EMythicControl C) {
    if (auto* ASC=AbilitySystem()) { const auto Tag=GetAbilityInputTag(C); if (Tag.IsValid()) { HeldAbilitySystems.Add(C,ASC); ASC->AbilityInputTagPressed(Tag); return; } }
    auto* PC=Player(); if (!PC) return;
    auto* Character=Cast<ACharacter>(PC->GetPawn());
    auto* Traversal=Character ? Character->FindComponentByClass<UMythicTraversalComponent>():nullptr;
    switch(C) {
    case EMythicControl::Jump: HeldJumpCharacter=Character; if (Traversal) Traversal->Jump(); else if (Character) Character->Jump(); break;
    case EMythicControl::Dash: if (Traversal) Traversal->Dash(); break;
    case EMythicControl::Crouch:
        if (Character) {
            const auto* Settings=UMythicUserSettings::Get();
            if (Settings && Settings->GetHoldToCrouch()) { HeldCrouchCharacter=Character; Character->Crouch(); }
            else if (Character->bIsCrouched) Character->UnCrouch(); else Character->Crouch();
        }
        break;
    case EMythicControl::Interact: BeginInteraction(); break;
    case EMythicControl::Inspect: if (auto* LP=PC->GetLocalPlayer()) if (auto* Director=LP->GetSubsystem<UMythicNameplateDirector>()) Director->ToggleFocusedEntityInspect(); break;
    case EMythicControl::CameraMode: if (auto* Intent=PC->FindComponentByClass<UMythicAbilityIntentComponent>()) Intent->CycleCameraMode(); break;
    case EMythicControl::LockTarget: if (auto* Utility=PC->FindComponentByClass<UMythicCombatUtilityComponent>()) Utility->ToggleTargetLock(); break;
    case EMythicControl::CharacterMenu: if (auto* HUD=PC->FindLocalHUDLayout()) HUD->HandleMenuAction(); break;
    case EMythicControl::SystemMenu: if (auto* HUD=PC->FindLocalHUDLayout()) HUD->HandleEscapeAction(); break;
    default:break;
    }
}
void UMythicControlsComponent::DispatchRelease(EMythicControl C,bool bCanceled) {
    if (C==EMythicControl::Crouch) {
        auto* Character=HeldCrouchCharacter.Get(); HeldCrouchCharacter.Reset();
        if (Character && (!Character->GetController() || Character->GetController()==Player())) Character->UnCrouch();
    }
    const auto Tag=GetAbilityInputTag(C);
    if (Tag.IsValid()) {
        TWeakObjectPtr<UMythicAbilitySystemComponent> Captured;
        HeldAbilitySystems.RemoveAndCopyValue(C,Captured);
        if (auto* ASC=Captured.Get()) { if (bCanceled) ASC->AbilityInputTagCanceled(Tag); else ASC->AbilityInputTagReleased(Tag); }
        return;
    }
    auto* PC=Player();
    if (C==EMythicControl::Jump) {
        auto* Character=HeldJumpCharacter.Get(); HeldJumpCharacter.Reset();
        if (Character && (!IsValid(Character->GetController()) || Character->GetController()==PC)) {
            if (auto* Traversal=Character->FindComponentByClass<UMythicTraversalComponent>()) {
                if (bCanceled) Traversal->CancelTraversal(); else Traversal->ReleaseJump();
            } else Character->StopJumping();
        }
    }
    if (!PC) return;
    if (C==EMythicControl::Interact) EndInteraction(bCanceled);
    if (bCanceled) return;
    if (C==EMythicControl::Heal || C==EMythicControl::Utility) if (auto* Quick=PC->FindComponentByClass<UMythicQuickUseComponent>()) Quick->UseSelected(C==EMythicControl::Heal);
    if (C==EMythicControl::Ping) ExecuteWheelCommand(C,0);
}
void UMythicControlsComponent::CancelHeldInput() {
    ++InputEpoch; // Even nested explicit cancellation defeats an in-flight handoff.
    if (bCancelingInput) return;
    TGuardValue<bool> CancelGuard(bCancelingInput,true);
    const auto Old=MoveTemp(Presses); Presses.Reset(); HeldActions.Reset();
    // Quarantine every old key before UI deactivation or gameplay callbacks can
    // reenter. A canceled opener can never leave a modal wheel without an owner.
    for (const auto& Pair:Old) SuppressedKeys.Add(Pair.Key);
    CloseWheel(false);
    TSet<EMythicControl> Canceled;
    for (const auto& Pair:Old) {
        if (!Canceled.Contains(Pair.Value.Control)) { Canceled.Add(Pair.Value.Control); DispatchRelease(Pair.Value.Control,true); }
    }
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(HoldTimer);
    EndInteraction(true); EndCameraAdjustment();
}
FVector UMythicControlsComponent::GetMovementIntent() const {
    const float Forward=(HeldActions.FindRef(EMythicControl::MoveForward)>0 ? 1.f:0.f)-(HeldActions.FindRef(EMythicControl::MoveBackward)>0 ? 1.f:0.f);
    const float Right=(HeldActions.FindRef(EMythicControl::MoveRight)>0 ? 1.f:0.f)-(HeldActions.FindRef(EMythicControl::MoveLeft)>0 ? 1.f:0.f);
    const auto* PC=Player(); if (!PC) return FVector::ZeroVector;
    const FRotator Yaw(0,PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraRotation().Yaw:PC->GetControlRotation().Yaw,0);
    return (Yaw.Vector()*Forward+FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y)*Right).GetClampedToMaxSize(1);
}
void UMythicControlsComponent::AdvanceInput(float DeltaSeconds) {
    auto* PC=Player(); if (!PC) return;
    const auto* ASC=AbilitySystem();
    if (!PC->GetPawn() || (ASC && (ASC->HasMatchingGameplayTag(GAS_STATE_DEAD) || ASC->HasMatchingGameplayTag(GAS_STATE_DOWNED)))) {
        if (!Presses.IsEmpty() || IsGameplayWheelOpen() || bCameraAdjustment) { CancelHeldInput(); CloseWheel(false); }
        return;
    }
    if (InteractionHoldSeconds>0) {
        auto* Director=PC->GetLocalPlayer() ? PC->GetLocalPlayer()->GetSubsystem<UMythicNameplateDirector>():nullptr;
        FMythicNameplateActionRailProjection Rail;
        if (PC->IsGameplayAbilityInputBlocked() || !Director || !Director->GetFocusedActionRailProjection(Rail)
            || Rail.Instance!=InteractionSubject || Rail.Actions.IsEmpty() || Rail.Actions[0].ActionTag!=InteractionAction) EndInteraction(true);
        else OnInteractionHoldProgress.Broadcast(InteractionSubject,InteractionAction,
            FMath::Clamp(static_cast<float>((FPlatformTime::Seconds()-InteractionStartedAt)/InteractionHoldSeconds),0.f,1.f));
    }
    if (bCameraAdjustment) {
        const float Zoom=(HeldActions.FindRef(EMythicControl::Primary)>0 ? 1.f:0.f)-(HeldActions.FindRef(EMythicControl::Secondary)>0 ? 1.f:0.f);
        // The authored ZoomCamera function lives on the possessed player pawn (the IA_Zoom receiver),
        // not its controller. Never search another pawn/CDO or retain this receiver across possession.
        APawn* CameraPawn=PC->GetPawn();
        if (Zoom!=0 && PC->IsLocalController() && IsValid(CameraPawn) && CameraPawn->IsLocallyControlled()
            && CameraPawn->GetController()==PC && FMath::IsFinite(DeltaSeconds) && DeltaSeconds>0) {
            if (UFunction* Function=CameraPawn->FindFunction(TEXT("ZoomCamera"))) {
                if (auto* Property=CastField<FNumericProperty>(Function->FindPropertyByName(TEXT("InOutScalar")));
                    Property && Property->HasAnyPropertyFlags(CPF_Parm) && !Property->HasAnyPropertyFlags(CPF_ReturnParm)
                    && Property->IsFloatingPoint()) {
                    FStructOnScope Params(Function);
                    Property->SetFloatingPointPropertyValue(Property->ContainerPtrToValuePtr<void>(Params.GetStructMemory()),Zoom*DeltaSeconds*8.f);
                    CameraPawn->ProcessEvent(Function,Params.GetStructMemory());
                }
            }
        }
    }
    if (PC->IsGameplayAbilityInputBlocked() && !IsGameplayWheelOpen() && !bCameraAdjustment) return;
    if (APawn* Pawn=PC->GetPawn()) if (!PC->IsMoveInputIgnored()) Pawn->AddMovementInput(GetMovementIntent(),1);
}
void UMythicControlsComponent::AdvanceHoldActions() {
    bool bPending=false; const double Now=FPlatformTime::Seconds();
    for (auto& Pair:Presses) if (IsWheel(Pair.Value.Control) && !Pair.Value.bWheelOpened) {
        bPending=true;
        if (Now-Pair.Value.StartedAt>=GetDefault<UMythicControlsSettings>()->WheelHoldSeconds) { Pair.Value.bWheelOpened=true; StartWheel(Pair.Key); break; }
    }
    if (!bPending) GetWorld()->GetTimerManager().ClearTimer(HoldTimer);
}
bool UMythicControlsComponent::IsWheelOpenCurrent(const FMythicWheelOpenToken& Token) const {
    auto* PC=Player(); const auto* ASC=AbilitySystem();
    const auto* Press=Presses.Find(Token.Key);
    return bInitialized && !bClosingWheel && Token.InputEpoch==InputEpoch && IsWheel(Token.Control)
        && PC && PC->IsLocalController() && Token.Pawn.IsValid() && PC->GetPawn()==Token.Pawn.Get()
        && Token.Pawn->GetController()==PC && ASC && !ASC->HasMatchingGameplayTag(GAS_STATE_DEAD)
        && !ASC->HasMatchingGameplayTag(GAS_STATE_DOWNED) && Press && Press->bWheelOpened
        && Press->Control==Token.Control && Press->StartedAt==Token.StartedAt
        && HeldActions.FindRef(Token.Control)>0 && !SuppressedKeys.Contains(Token.Key);
}
void UMythicControlsComponent::StartWheel(FKey OpenerKey) {
    if (IsGameplayWheelOpen() || bClosingWheel || !Player()) return;
    const auto* OpeningPress=Presses.Find(OpenerKey);
    if (!OpeningPress || !OpeningPress->bWheelOpened || !IsWheel(OpeningPress->Control)) return;
    FMythicWheelOpenToken Token; Token.InputEpoch=InputEpoch; Token.Key=OpenerKey;
    Token.StartedAt=OpeningPress->StartedAt; Token.Control=OpeningPress->Control; Token.Pawn=Player()->GetPawn();
    if (!IsWheelOpenCurrent(Token)) return;
    const EMythicControl C=Token.Control;
    // Cancel combat owners, retaining movement and the original physical opener.
    TArray<FKey> Cancel;
    for (const auto& Pair:Presses) if (!IsMovement(Pair.Value.Control) && Pair.Value.Control!=C) Cancel.Add(Pair.Key);
    TSet<EMythicControl> Canceled;
    for (FKey Key:Cancel) {
        FMythicControlPress Press;
        if (!Presses.RemoveAndCopyValue(Key,Press)) continue;
        HeldActions.Remove(Press.Control); SuppressedKeys.Add(Key);
        if (!Canceled.Contains(Press.Control)) { Canceled.Add(Press.Control); DispatchRelease(Press.Control,true); }
        if (!IsWheelOpenCurrent(Token)) return;
    }
    EndInteraction(true);
    if (!IsWheelOpenCurrent(Token)) return;
    bHasWheelPing=false;
    if (C==EMythicControl::Ping) if (auto* Utility=Player()->FindComponentByClass<UMythicCombatUtilityComponent>()) bHasWheelPing=Utility->TryCapturePingIntent(WheelPingSubject,WheelPingPoint);
    if (!IsWheelOpenCurrent(Token)) return;
    if (!Wheel) Wheel=CreateWidget<UMythicGameplayWheel>(Player(),UMythicGameplayWheel::StaticClass());
    if (Wheel && IsWheelOpenCurrent(Token)) {
        // CommonUI may flush before activation; only this exact retained opener can survive the transition.
        TGuardValue<bool> OpeningGuard(bOpeningWheel,true);
        Wheel->OpenFor(this,Token);
    }
}
void UMythicControlsComponent::CloseWheel(bool bCommit) {
    if (!bCommit) ++InputEpoch; // Explicit cancellation cannot be undone by an in-flight OpenFor.
    if (!Wheel || bClosingWheel) return;
    TGuardValue<bool> Guard(bClosingWheel,true);
    auto* Closing=Wheel.Get();
    if (bCommit) Closing->CommitHighlighted(); else Closing->CloseWithoutCommit();
}
bool UMythicControlsComponent::CloseOverlayForTraversal(FKey Key) {
    const TWeakObjectPtr<AMythicPlayerController> Owner=Player();
    const TWeakObjectPtr<APawn> Pawn=Owner.IsValid() ? Owner->GetPawn():nullptr;
    const TWeakObjectPtr<UMythicAbilitySystemComponent> ASC=AbilitySystem();
    const bool bCloseWheel=IsGameplayWheelOpen();
    // CloseWheel(false) owns exactly one epoch advance. Any additional explicit cancellation wins.
    const uint64 ExpectedEpoch=InputEpoch+(bCloseWheel ? 1:0);
    TGuardValue<bool> Handoff(bTraversalCloseHandoff,true);
    TGuardValue<bool> Released(bTraversalCloseReleased,false);
    TGuardValue<FKey> PhysicalKey(TraversalCloseKey,Key);
    EndCameraAdjustment();
    if (bCloseWheel) CloseWheel(false);
    const auto IsCurrent=[&]() {
        return InputEpoch==ExpectedEpoch && !bTraversalCloseReleased
            && Owner.IsValid() && Owner.Get()==Player() && Owner->IsLocalController()
            && Pawn.IsValid() && Owner->GetPawn()==Pawn.Get() && Pawn->GetController()==Owner.Get()
            && ASC.IsValid() && AbilitySystem()==ASC.Get() && ASC->GetAvatarActor()==Pawn.Get()
            && !bCameraAdjustment && !IsGameplayWheelOpen() && !Owner->IsGameplayAbilityInputBlocked();
    };
    // The previous input frame cached the overlay's suppression. Refresh its actual
    // controller-owned gate before the same physical press reaches GAS, then validate
    // again: projection observers may cancel input, replace the pawn, or open another UI.
    if (IsCurrent()) ASC->SetAbilityInputSuppressed(Owner->IsGameplayAbilityInputBlocked());
    const bool bCurrent=IsCurrent();
    if (!bCurrent && !bTraversalCloseReleased) SuppressedKeys.Add(Key);
    return bCurrent;
}
void UMythicControlsComponent::ExecuteWheelCommand(EMythicControl C,int32 Option) {
    if (C==EMythicControl::Ping) {
        if (auto* Utility=Player() ? Player()->FindComponentByClass<UMythicCombatUtilityComponent>():nullptr) {
            FMythicEntityPresentationInstance Subject; FVector Point=FVector::ZeroVector;
            const bool bCaptured=bHasWheelPing || Utility->TryCapturePingIntent(Subject,Point);
            if (bHasWheelPing) { Subject=WheelPingSubject; Point=WheelPingPoint; bHasWheelPing=false; }
            if (bCaptured) Utility->SubmitPing(static_cast<EMythicPingIntent>(FMath::Clamp(Option,0,7)),Subject,Point);
        }
    } else DispatchPress(C);
}
void UMythicControlsComponent::BeginCameraAdjustment() {
    if (bCameraAdjustment || !Player() || !Player()->IsLocalController()) return;
    CancelHeldInput(); bCameraAdjustment=true;
    if (auto* ASC=AbilitySystem()) ASC->ClearAbilityInput();
    OnCameraAdjustmentChanged.Broadcast(true);
}
void UMythicControlsComponent::EndCameraAdjustment() {
    if (!bCameraAdjustment) return; bCameraAdjustment=false;
    TArray<FKey> Remove; for (const auto& Pair:Presses) if (Pair.Value.Control==EMythicControl::Primary || Pair.Value.Control==EMythicControl::Secondary) Remove.Add(Pair.Key);
    for (const FKey Key:Remove) { HeldActions.Remove(Presses.FindChecked(Key).Control); Presses.Remove(Key); SuppressedKeys.Add(Key); }
    OnCameraAdjustmentChanged.Broadcast(false);
}
void UMythicControlsComponent::BeginInteraction() {
    EndInteraction(true); auto* PC=Player(); auto* Director=PC && PC->GetLocalPlayer() ? PC->GetLocalPlayer()->GetSubsystem<UMythicNameplateDirector>():nullptr;
    FMythicNameplateActionRailProjection Rail;
    if (!Director || !Director->GetFocusedActionRailProjection(Rail) || Rail.Actions.IsEmpty()) {
        if (PC) if (auto* Harvest=PC->FindComponentByClass<UMythicHarvestFocusComponent>()) Harvest->TryControlsInteraction();
        return;
    }
    const auto& Action=Rail.Actions[0]; InteractionSubject=Rail.Instance; InteractionAction=Action.ActionTag; bInteractionRequiresHold=Action.HoldDurationSeconds>0;
    if (bInteractionRequiresHold) {
        if (Director->BeginFocusedContextActionHold(InteractionAction)) {
            InteractionStartedAt=FPlatformTime::Seconds(); InteractionHoldSeconds=Action.HoldDurationSeconds;
            OnInteractionHoldProgress.Broadcast(InteractionSubject,InteractionAction,0.f);
            GetWorld()->GetTimerManager().SetTimer(InteractionTimer,this,&ThisClass::CompleteInteractionHold,Action.HoldDurationSeconds,false);
        } else EndInteraction(true);
    }
    else Director->ExecuteFocusedContextAction(InteractionAction);
}
void UMythicControlsComponent::CompleteInteractionHold() {
    auto* PC=Player(); auto* Director=PC && PC->GetLocalPlayer() ? PC->GetLocalPlayer()->GetSubsystem<UMythicNameplateDirector>():nullptr;
    FMythicNameplateActionRailProjection Rail;
    if (Director && HeldActions.FindRef(EMythicControl::Interact)>0 && Director->GetFocusedActionRailProjection(Rail) && Rail.Instance==InteractionSubject) Director->ExecuteFocusedContextAction(InteractionAction);
    EndInteraction(true);
}
void UMythicControlsComponent::EndInteraction(bool) {
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(InteractionTimer);
    if (InteractionHoldSeconds>0) OnInteractionHoldProgress.Broadcast(InteractionSubject,InteractionAction,0.f);
    if (InteractionAction.IsValid()) if (auto* PC=Player()) if (auto* LP=PC->GetLocalPlayer()) if (auto* Director=LP->GetSubsystem<UMythicNameplateDirector>()) Director->CancelFocusedContextActionHold(InteractionAction);
    InteractionSubject.Reset(); InteractionAction=FGameplayTag(); bInteractionRequiresHold=false; InteractionHoldSeconds=0; InteractionStartedAt=0;
}
bool UMythicControlsComponent::RemapControl(EMythicControl C,bool bGamepad,FKey Key,FText& Error) {
    Error=FText::GetEmpty();
    const auto* Target=EffectiveBindings.FindByPredicate([C](const auto& B){return B.Control==C;});
    const auto* Default=GetDefault<UMythicControlsSettings>()->Find(C);
    const bool bAuthored=Default && (bGamepad ? Default->Gamepad:Default->Keyboard).IsValid();
    if (!Target || !bAuthored || Key.IsAxis1D() || Key.IsAxis2D() || Key.IsAxis3D() || (Key.IsValid() && Key.IsGamepadKey()!=bGamepad) || !UserSettings.IsValid()) { Error=NSLOCTEXT("Controls","InvalidKey","Choose a button on the same input device."); return false; }
    for (const auto& Binding:EffectiveBindings) {
        if (!Key.IsValid() || Binding.Control==C || (bGamepad ? Binding.Gamepad:Binding.Keyboard)!=Key) continue;
        const bool bSameLayer=!bGamepad || Binding.bGamepadAbilityLayer==Target->bGamepadAbilityLayer;
        const bool bModifier=Binding.Control==EMythicControl::AbilityLayer || C==EMythicControl::AbilityLayer;
        const bool bAlwaysAvailable=Binding.Control==EMythicControl::Jump || Binding.Control==EMythicControl::Dash || Binding.Control==EMythicControl::Secondary || C==EMythicControl::Jump || C==EMythicControl::Dash || C==EMythicControl::Secondary;
        if (bSameLayer || bModifier || bAlwaysAvailable) { Error=FText::Format(NSLOCTEXT("Controls","Conflict","Already assigned to {0}."),Binding.Label); return false; }
    }
    CancelHeldInput(); FMapPlayerKeyArgs Args; Args.MappingName=UMythicControlsSettings::MappingName(C,bGamepad); Args.Slot=EPlayerMappableKeySlot::First;
    Args.NewKey=Key; Args.ProfileIdString=UserSettings->GetActiveKeyProfileId(); Args.bCreateMatchingSlotIfNeeded=false;
    FGameplayTagContainer Failures; UserSettings->MapPlayerKey(Args,Failures);
    if (!Failures.IsEmpty()) { Error=NSLOCTEXT("Controls","Rejected","The input profile could not apply this binding."); return false; }
    RefreshMappings(); UserSettings->AsyncSaveSettings(); return true;
}
void UMythicControlsComponent::ResetMappings() {
    if (!UserSettings.IsValid()) return; CancelHeldInput();
    for (const auto& Binding:GetDefault<UMythicControlsSettings>()->Bindings) for (bool bGamepad:{false,true}) if ((bGamepad ? Binding.Gamepad:Binding.Keyboard).IsValid()) {
        FMapPlayerKeyArgs Args; Args.MappingName=UMythicControlsSettings::MappingName(Binding.Control,bGamepad); Args.Slot=EPlayerMappableKeySlot::First; Args.ProfileIdString=UserSettings->GetActiveKeyProfileId();
        FGameplayTagContainer Failures; UserSettings->ResetAllPlayerKeysInRow(Args,Failures);
    }
    RefreshMappings(); UserSettings->AsyncSaveSettings();
}
bool UMythicControlsComponent::SwapControls(EMythicControl First,EMythicControl Second,bool bGamepad,FText& Error) {
    Error=FText::GetEmpty();
    const FKey A=GetEffectiveKey(First,bGamepad),B=GetEffectiveKey(Second,bGamepad);
    if (First==Second || !A.IsValid() || !B.IsValid() || !UserSettings.IsValid()) return false;
    auto Proposed=EffectiveBindings;
    for (auto& Binding:Proposed) if (Binding.Control==First || Binding.Control==Second) (bGamepad ? Binding.Gamepad:Binding.Keyboard)=Binding.Control==First ? B:A;
    for (int32 I=0;I<Proposed.Num();++I) for (int32 J=I+1;J<Proposed.Num();++J) {
        const auto& L=Proposed[I]; const auto& R=Proposed[J]; const FKey LK=bGamepad ? L.Gamepad:L.Keyboard, RK=bGamepad ? R.Gamepad:R.Keyboard;
        if (!LK.IsValid() || LK!=RK) continue;
        const auto Always=[](EMythicControl C){return C==EMythicControl::AbilityLayer || C==EMythicControl::Jump || C==EMythicControl::Dash || C==EMythicControl::Secondary;};
        if (!bGamepad || L.bGamepadAbilityLayer==R.bGamepadAbilityLayer || Always(L.Control) || Always(R.Control)) { Error=NSLOCTEXT("Controls","SwapConflict","This swap would conflict with another action or the ability modifier."); return false; }
    }
    CancelHeldInput();
    bool bSucceeded=false;
    {
        // Both profile writes finish before Enhanced Input's deferred rebuild can expose either mapping.
        TGuardValue<bool> Guard(bEditingMappings,true);
        auto Map=[&](EMythicControl C,FKey Key) {
            FMapPlayerKeyArgs Args; Args.MappingName=UMythicControlsSettings::MappingName(C,bGamepad); Args.Slot=EPlayerMappableKeySlot::First;
            Args.NewKey=Key; Args.ProfileIdString=UserSettings->GetActiveKeyProfileId(); Args.bCreateMatchingSlotIfNeeded=false;
            FGameplayTagContainer Failures; UserSettings->MapPlayerKey(Args,Failures); return Failures.IsEmpty();
        };
        bSucceeded=Map(First,B) && Map(Second,A);
        if (!bSucceeded) { Map(First,A); Map(Second,B); }
    }
    RefreshMappings();
    if (bSucceeded) UserSettings->AsyncSaveSettings(); else Error=NSLOCTEXT("Controls","SwapFailed","Could not apply this swap; the previous bindings were restored.");
    return bSucceeded;
}
void UMythicControlsComponent::OpenControlsPage() { if (auto* PC=Player()) if (auto* HUD=PC->FindLocalHUDLayout()) HUD->OpenMenuOnPage(TEXT("Settings")); }
