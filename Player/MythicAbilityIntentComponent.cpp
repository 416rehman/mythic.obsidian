#include "Player/MythicAbilityIntentComponent.h"
#include "Player/MythicPlayerController.h"
#include "Input/MythicCombatUtilityComponent.h"
#include "UI/Settings/MythicUserSettings.h"
#include "Settings/MythicDeveloperSettings.h"
#include "UI/MythicHUDLayout.h"
#include "Input/CommonUIActionRouterBase.h"
#include "CommonInputSubsystem.h"
#include "Core/CameraRigAsset.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "SceneView.h"

UMythicAbilityIntentComponent::UMythicAbilityIntentComponent() { PrimaryComponentTick.bCanEverTick=false; }

void UMythicAbilityIntentComponent::BeginPlay() {
    Super::BeginPlay();
    if (auto* PC=Cast<AMythicPlayerController>(GetOwner()); PC && PC->IsLocalController()) {
        OwnerHandle=PC->OnLocalGameplayOwnerChanged.AddUObject(this,&ThisClass::ResetPointer);
        InitializeLocalInput(PC->InputComponent);
    }
}

void UMythicAbilityIntentComponent::ClearInputBindings() {
    if (BoundInput.IsValid()) for (uint32 Handle:BindingHandles) BoundInput->RemoveBindingByHandle(Handle);
    BindingHandles.Reset(); BoundInput.Reset();
    if (BoundCommonInput.IsValid()) BoundCommonInput->OnInputMethodChangedNative.Remove(DeviceHandle);
    DeviceHandle.Reset(); BoundCommonInput.Reset();
    if (BoundRouter.IsValid()) BoundRouter->OnActiveInputConfigChanged().Remove(InputConfigHandle);
    InputConfigHandle.Reset(); BoundRouter.Reset();
    if (BoundSubsystem.IsValid() && InstalledContext.IsValid()) BoundSubsystem->RemoveMappingContext(InstalledContext.Get());
    InstalledContext.Reset(); BoundSubsystem.Reset(); ResetPointer();
}

void UMythicAbilityIntentComponent::EndPlay(const EEndPlayReason::Type Reason) {
    if (auto* PC=Cast<AMythicPlayerController>(GetOwner())) PC->OnLocalGameplayOwnerChanged.Remove(OwnerHandle);
    ClearInputBindings(); Super::EndPlay(Reason);
}

void UMythicAbilityIntentComponent::InitializeLocalInput(UInputComponent* Input) {
    ClearInputBindings();
    auto* PC=Cast<AMythicPlayerController>(GetOwner());
    auto* LP=PC && PC->IsLocalController() ? PC->GetLocalPlayer() : nullptr;
    auto* Enhanced=Cast<UEnhancedInputComponent>(Input);
    if (!LP || !Enhanced) return;
    if (!Definition) Definition=GetDefault<UMythicDeveloperSettings>()->AbilityCameraDefinition.LoadSynchronous();
    if (!Definition) return;
    BoundInput=Enhanced; BoundSubsystem=LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
    BoundCommonInput=LP->GetSubsystem<UCommonInputSubsystem>();
    BoundRouter=LP->GetSubsystem<UCommonUIActionRouterBase>();
    if (BoundRouter.IsValid()) InputConfigHandle=BoundRouter->OnActiveInputConfigChanged().AddWeakLambda(this,[this](const FUIInputConfig&) { RefreshCursorPresentation(); });
    if (BoundCommonInput.IsValid()) DeviceHandle=BoundCommonInput->OnInputMethodChangedNative.AddUObject(this,&ThisClass::OnDeviceChanged);
    if (auto* VirtualReticleAction=Definition->VirtualReticleAction.Get()) {
        BindingHandles.Add(Enhanced->BindAction(VirtualReticleAction,ETriggerEvent::Triggered,this,&ThisClass::OnReticleInput).GetHandle());
        BindingHandles.Add(Enhanced->BindAction(VirtualReticleAction,ETriggerEvent::Completed,this,&ThisClass::ResetHeldInput).GetHandle());
        BindingHandles.Add(Enhanced->BindAction(VirtualReticleAction,ETriggerEvent::Canceled,this,&ThisClass::ResetHeldInput).GetHandle());
    }
    ApplyModeContext();
    RefreshGameplayInputConfig();
}

FMythicCameraMode UMythicAbilityIntentComponent::GetSelectedCameraMode() const {
    return Definition && Definition->CameraModes.IsValidIndex(SelectedMode) ? Definition->CameraModes[SelectedMode] : FMythicCameraMode();
}

bool UMythicAbilityIntentComponent::SelectCameraMode(int32 Index) {
    auto* PC=Cast<AMythicPlayerController>(GetOwner());
    if (!Definition || !PC || !PC->IsLocalController() || PC->IsGameplayAbilityInputBlocked()) return false;
    const auto& CameraModes=Definition->CameraModes;
    if (!CameraModes.IsValidIndex(Index) || !CameraModes[Index].CameraRig || CameraModes.Num()>16) return false;
    SelectedMode=Index; ResetPointer(); ApplyModeContext();
    RefreshGameplayInputConfig();
    return true;
}

bool UMythicAbilityIntentComponent::CycleCameraMode() {
    return Definition && !Definition->CameraModes.IsEmpty() && SelectCameraMode((SelectedMode+1)%Definition->CameraModes.Num());
}

void UMythicAbilityIntentComponent::ApplyModeContext() {
    if (!BoundSubsystem.IsValid() || !Definition) return;
    UInputMappingContext* Next=GetSelectedCameraMode().AbilityAimPolicy==EMythicAbilityAimPolicy::WorldCursor ? Definition->WorldAimContext : Definition->ViewAimContext;
    if (InstalledContext.Get()==Next) return;
    if (InstalledContext.IsValid()) BoundSubsystem->RemoveMappingContext(InstalledContext.Get());
    InstalledContext=Next;
    if (Next) { FModifyContextOptions Options; Options.bIgnoreAllPressedKeysUntilRelease=true; BoundSubsystem->AddMappingContext(Next,1,Options); }
}

void UMythicAbilityIntentComponent::ResetHeldInput() {
    HeldAxes=FVector2D::ZeroVector;
    // Existing owner/mode/device/menu/application reset routes also suspend optional camera assistance.
    if (auto* Owner=GetOwner()) if (auto* Utility=Owner->FindComponentByClass<UMythicCombatUtilityComponent>()) Utility->SuppressLockYawAssist();
}
void UMythicAbilityIntentComponent::ResetPointer() { ResetHeldInput(); bHasVirtualPosition=false; PreviousView=FIntRect(); }
void UMythicAbilityIntentComponent::OnDeviceChanged(ECommonInputType) { ResetPointer(); RefreshGameplayInputConfig(); }

void UMythicAbilityIntentComponent::RefreshGameplayInputConfig() {
    if (!BoundRouter.IsValid()) return;
    auto* HUD=Cast<UMythicHUDLayout>(BoundRouter->GetLeafmostActivatableWidget());
    if (!HUD || !HUD->IsActivated() || !BoundRouter->CanProcessNormalGameInput()) return;
    if (const auto Config=HUD->GetDesiredInputConfig(); Config.IsSet()) BoundRouter->SetActiveUIInputConfig(Config.GetValue(),HUD);
    RefreshCursorPresentation();
}

void UMythicAbilityIntentComponent::RefreshCursorPresentation() {
    if (!BoundRouter.IsValid() || !BoundCommonInput.IsValid()) return;
    auto* PC=Cast<AMythicPlayerController>(GetOwner());
    auto* HUD=Cast<UMythicHUDLayout>(BoundRouter->GetLeafmostActivatableWidget());
    if (!PC || !HUD || !HUD->IsActivated() || PC->IsGameplayAbilityInputBlocked()) return;
    PC->SetShowMouseCursor(GetSelectedCameraMode().AbilityAimPolicy==EMythicAbilityAimPolicy::WorldCursor
        && BoundCommonInput->GetCurrentInputType()!=ECommonInputType::Gamepad);
}

void UMythicAbilityIntentComponent::OnReticleInput(const FInputActionValue& Value) {
    ResetHeldInput();
    const auto* PC=Cast<AMythicPlayerController>(GetOwner());
    if (!PC || PC->IsGameplayAbilityInputBlocked() || GetSelectedCameraMode().AbilityAimPolicy!=EMythicAbilityAimPolicy::WorldCursor
        || !BoundCommonInput.IsValid() || BoundCommonInput->GetCurrentInputType()!=ECommonInputType::Gamepad
        || Value.GetValueType()!=EInputActionValueType::Axis2D) return;
    FVector2D Axis=Value.Get<FVector2D>();
    if (Axis.ContainsNaN()) return;
    const auto* Settings=UMythicUserSettings::Get();
    const float Deadzone=Settings ? FMath::Clamp(Settings->GetGamepadDeadzoneRight(),0.0f,0.9f) : 0.2f;
    const double Size=Axis.Size();
    if (Size<=Deadzone) return;
    Axis*=FMath::Min(1.0,(Size-Deadzone)/(1.0-Deadzone))/Size;
    HeldAxes=FVector2D(Axis.X,-Axis.Y);
}

bool UMythicAbilityIntentComponent::GetLocalView(FIntRect& Rect) const {
    const auto* PC=Cast<AMythicPlayerController>(GetOwner());
    const auto* LP=PC && PC->IsLocalController() ? PC->GetLocalPlayer() : nullptr;
    if (!LP || !LP->ViewportClient || !LP->ViewportClient->Viewport || !PC->GetPawn()
        || PC->IsGameplayAbilityInputBlocked() || !GetSelectedCameraMode().CameraRig) return false;
    FSceneViewProjectionData Data;
    if (!LP->GetProjectionData(LP->ViewportClient->Viewport,Data)) return false;
    Rect=Data.GetConstrainedViewRect();
    return Rect.Width()>=64 && Rect.Height()>=64;
}

bool UMythicAbilityIntentComponent::GetAimScreenPosition(FVector2D& Position,FIntRect& Rect) {
    if (!GetLocalView(Rect)) return false;
    auto* PC=CastChecked<AMythicPlayerController>(GetOwner());
    if (GetSelectedCameraMode().AbilityAimPolicy==EMythicAbilityAimPolicy::ViewCenter) {
        Position=FVector2D(Rect.Min)+FVector2D(Rect.Size())*0.5; return true;
    }
    const auto* Common=PC->GetLocalPlayer()->GetSubsystem<UCommonInputSubsystem>();
    if (!Common) return false;
    if (Common->GetCurrentInputType()!=ECommonInputType::Gamepad) {
        float X=0,Y=0;
        if (!PC->GetMousePosition(X,Y)) return false;
        Position=FVector2D(X,Y);
        return X>=Rect.Min.X && X<Rect.Max.X && Y>=Rect.Min.Y && Y<Rect.Max.Y;
    }
    if (!bHasVirtualPosition || Rect!=PreviousView) {
        const APawn* Pawn=PC->GetPawn();
        FVector Forward=Pawn->GetActorForwardVector(); Forward.Z=0;
        const FVector Ahead=Pawn->GetActorLocation()+Forward.GetSafeNormal()*500;
        if (!PC->ProjectWorldLocationToScreen(Ahead,VirtualPosition,false)) VirtualPosition=FVector2D(Rect.Min)+FVector2D(Rect.Size())*0.5;
        PreviousView=Rect; bHasVirtualPosition=true;
    }
    const float Margin=FMath::Min(Rect.Width(),Rect.Height())*0.04f;
    VirtualPosition.X=FMath::Clamp(VirtualPosition.X,Rect.Min.X+Margin,Rect.Max.X-Margin);
    VirtualPosition.Y=FMath::Clamp(VirtualPosition.Y,Rect.Min.Y+Margin,Rect.Max.Y-Margin);
    Position=VirtualPosition; return true;
}

void UMythicAbilityIntentComponent::AdvanceReticle(float DeltaSeconds) {
    if (HeldAxes.IsNearlyZero()) return;
    FVector2D Position; FIntRect Rect;
    if (!GetAimScreenPosition(Position,Rect) || GetSelectedCameraMode().AbilityAimPolicy!=EMythicAbilityAimPolicy::WorldCursor) { ResetHeldInput(); return; }
    VirtualPosition+=HeldAxes*FMath::Min(Rect.Width(),Rect.Height())*FMath::Clamp(Definition->ReticleViewportSpeed,0.1f,2.0f)*FMath::Clamp(DeltaSeconds,0.0f,0.05f);
    GetAimScreenPosition(Position,Rect);
}

bool UMythicAbilityIntentComponent::GetLocalReticlePosition(FVector2D& Position) {
    Position=FVector2D::ZeroVector; FIntRect Rect; FVector2D Pixel;
    if (!GetAimScreenPosition(Pixel,Rect)) return false;
    Position=(Pixel-FVector2D(Rect.Min))/FVector2D(Rect.Size()); return true;
}

bool UMythicAbilityIntentComponent::GetReticleViewportPixel(FVector2D& Position,bool& bUsesHardwareCursor) {
    FIntRect Rect; bUsesHardwareCursor=false;
    if (!GetAimScreenPosition(Position,Rect)) return false;
    const auto* PC=CastChecked<AMythicPlayerController>(GetOwner());
    const auto* Common=PC->GetLocalPlayer()->GetSubsystem<UCommonInputSubsystem>();
    bUsesHardwareCursor=GetSelectedCameraMode().AbilityAimPolicy==EMythicAbilityAimPolicy::WorldCursor
        && Common && Common->GetCurrentInputType()!=ECommonInputType::Gamepad;
    return true;
}

bool UMythicAbilityIntentComponent::TryCaptureAimDirection(float MaxRange,FVector& OutDirection) {
    OutDirection=FVector::ZeroVector;
    if (!FMath::IsFinite(MaxRange) || MaxRange<=0) return false;
    FVector2D Pixel; FIntRect Rect;
    if (!GetAimScreenPosition(Pixel,Rect)) return false;
    auto* PC=CastChecked<AMythicPlayerController>(GetOwner());
    FVector Camera,Ray;
    if (!PC->DeprojectScreenPositionToWorld(Pixel.X,Pixel.Y,Camera,Ray) || Camera.ContainsNaN() || Ray.ContainsNaN()) return false;
    FVector Eye; FRotator Rotation; PC->GetPawn()->GetActorEyesViewPoint(Eye,Rotation);
    return ResolveCameraAim(GetWorld(),PC->GetPawn(),Eye,Camera,Ray,MaxRange,OutDirection);
}

bool UMythicAbilityIntentComponent::ResolveCameraAim(UWorld* World,AActor* Source,FVector Eye,FVector Camera,FVector Ray,float MaxRange,FVector& OutDirection) {
    OutDirection=FVector::ZeroVector;
    if (!World || !IsValid(Source) || Source->GetWorld()!=World || Camera.ContainsNaN() || Eye.ContainsNaN() || Ray.ContainsNaN()
        || !FMath::IsFinite(MaxRange) || MaxRange<=0) return false;
    Ray=Ray.GetSafeNormal();
    const double Offset=FVector::Distance(Camera,Eye);
    if (Eye.ContainsNaN() || Ray.IsNearlyZero() || Offset>10000) return false;
    const FVector End=Camera+Ray*FMath::Min(13000.0,Offset+FMath::Clamp(MaxRange,1.0f,3000.0f));
    FCollisionQueryParams Params(SCENE_QUERY_STAT(AbilityCameraIntent),false,Source); FHitResult Hit;
    const FVector Point=World->LineTraceSingleByChannel(Hit,Camera,End,ECC_Visibility,Params) ? Hit.ImpactPoint : End;
    const FVector Delta=Point-Eye;
    // Geometry between the camera and the pawn must not reverse the requested direction.
    if (Delta.ContainsNaN() || Delta.IsNearlyZero() || FVector::DotProduct(Delta,Ray)<=0) return false;
    OutDirection=Delta.GetSafeNormal(); return true;
}
