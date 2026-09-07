#include "MythicAppearancePreview.h"
#include "MythicAppearanceCatalog.h"
#include "MythicAppearanceRenderer.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/TextureRenderTarget2D.h"
#include "PreviewScene.h"
#include "GameFramework/WorldSettings.h"
#include "TimerManager.h"

UMythicAppearancePreview::UMythicAppearancePreview() = default;
UMythicAppearancePreview::~UMythicAppearancePreview() = default;
bool UMythicAppearancePreview::Start(USkeletalMeshComponent* CanonicalDriver) {
    Stop();
    if (!CanonicalDriver || !CanonicalDriver->GetSkeletalMeshAsset()) return false;
    Scene=MakeUnique<FPreviewScene>(FPreviewScene::ConstructionValues().SetEditor(false).SetTransactional(false)
        .SetForceMipsResident(false).AllowAudioPlayback(false).ShouldSimulatePhysics(false));
    auto* World=Scene->GetWorld();
    Avatar=World->SpawnActor<AActor>();
    auto* Fill=NewObject<UDirectionalLightComponent>(Avatar,NAME_None,RF_Transient);
    Avatar->AddInstanceComponent(Fill); Fill->SetIntensity(UE_PI*0.65f);
    Fill->SetCastShadows(false); Fill->SetWorldRotation(FRotator(-20,145,0)); Fill->RegisterComponent();
    Driver=NewObject<USkeletalMeshComponent>(Avatar,NAME_None,RF_Transient);
    Avatar->SetRootComponent(Driver); Avatar->AddInstanceComponent(Driver);
    Driver->SetSkeletalMesh(CanonicalDriver->GetSkeletalMeshAsset());
    Driver->SetRelativeRotation(FRotator(0,-90,0));
    Driver->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Driver->SetCanEverAffectNavigation(false); Driver->SetGenerateOverlapEvents(false);
    Driver->bEnableUpdateRateOptimizations=false;
    Driver->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    Driver->SetHiddenInGame(true,false); Driver->SetCastShadow(false); Driver->RegisterComponent();
    Renderer=NewObject<UMythicAppearanceRenderer>(Avatar,NAME_None,RF_Transient);
    Avatar->AddInstanceComponent(Renderer); Renderer->SetIsReplicated(false); Renderer->RegisterComponent();
    Target=NewObject<UTextureRenderTarget2D>(this,NAME_None,RF_Transient);
    Target->ClearColor=FLinearColor(0.035f,0.04f,0.05f,1); Target->InitAutoFormat(768,1024);
    Capture=NewObject<USceneCaptureComponent2D>(Avatar,NAME_None,RF_Transient);
    Avatar->AddInstanceComponent(Capture); Capture->TextureTarget=Target;
    Capture->bCaptureEveryFrame=false; Capture->bCaptureOnMovement=false; Capture->bAlwaysPersistRenderingState=true;
    Capture->CaptureSource=ESceneCaptureSource::SCS_FinalColorLDR; Capture->FOVAngle=38;
    Capture->PostProcessSettings.bOverride_AutoExposureMethod=true;
    Capture->PostProcessSettings.AutoExposureMethod=EAutoExposureMethod::AEM_Manual;
    Capture->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure=true;
    Capture->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure=false;
    Capture->PostProcessSettings.bOverride_AutoExposureBias=true; Capture->PostProcessSettings.AutoExposureBias=0;
    Capture->ShowFlags.SetMotionBlur(false); Capture->RegisterComponent();
    World->BeginPlay();
    if (!World->HasBegunPlay()) World->GetWorldSettings()->NotifyBeginPlay();
    if (!Avatar->HasActorBegunPlay() || !Renderer->HasBegunPlay()) { Stop(); return false; }
    ResetView(); return true;
}
void UMythicAppearancePreview::Apply(const FMythicAppearancePublicState& State) {
    if (!Scene || !Renderer) return;
    PendingState=State;
    if (Frame==State.Frame) { if(bIdleReady)Renderer->ApplyPreview(State,Driver); return; }
    Frame=State.Frame; const uint32 Expected=++Generation;
    bIdleReady=false;bIdleFailed=false;
    Scene->GetWorld()->GetTimerManager().ClearTimer(IdleDeadline);
    if (IdleLoad) { IdleLoad->CancelHandle(); IdleLoad.Reset(); }
    auto* Catalog=UMythicAppearanceCatalog::Get(); const auto* Style=Catalog?Catalog->FindFrame(Frame):nullptr;
    if (!Style || Style->PreviewIdle.IsNull()) { bIdleFailed=true; return; }
    const auto Idle=Style->PreviewIdle;
    auto Ready=[this,Expected,Idle](){
        if(!Scene || !Driver || Generation!=Expected)return;
        Scene->GetWorld()->GetTimerManager().ClearTimer(IdleDeadline);
        if(!Idle.Get()){bIdleFailed=true;return;}
        Driver->PlayAnimation(Idle.Get(),true);bIdleReady=true;Renderer->ApplyPreview(PendingState,Driver);
    };
    if (Idle.Get()) Ready();
    else {
        Scene->GetWorld()->GetTimerManager().SetTimer(IdleDeadline,FTimerDelegate::CreateWeakLambda(this,[this,Expected](){if(Generation==Expected){bIdleFailed=true;if(IdleLoad){IdleLoad->CancelHandle();IdleLoad.Reset();}}}),5.f,false);
        IdleLoad=UAssetManager::GetStreamableManager().RequestAsyncLoad(Idle.ToSoftObjectPath(),FStreamableDelegate::CreateWeakLambda(this,Ready));
    }
}
void UMythicAppearancePreview::Tick(float Delta) {
    if (!Scene || !Capture) return;
    Accumulator+=FMath::Clamp(Delta,0.f,0.1f);
    if (Accumulator<1.f/30.f) return;
    const float Step=Accumulator; Accumulator=0;
    Scene->GetWorld()->Tick(LEVELTICK_All,Step);
    if(IsReady())Capture->CaptureScene();
}
void UMythicAppearancePreview::Orbit(float Degrees) { Yaw=FMath::UnwindDegrees(Yaw+Degrees); PlaceCamera(); }
void UMythicAppearancePreview::Zoom(float Amount) { Distance=FMath::Clamp(Distance+Amount,160.f,390.f); PlaceCamera(); }
void UMythicAppearancePreview::ResetView() { Yaw=25; Distance=300; PlaceCamera(); }
void UMythicAppearancePreview::PlaceCamera() {
    if (!Capture) return;
    const FVector Focus(0,0,90);
    const FVector Location=Focus+FRotator(-3,Yaw,0).Vector()*Distance;
    Capture->SetWorldLocationAndRotation(Location,(Focus-Location).Rotation());
}
bool UMythicAppearancePreview::IsReady() const { return bIdleReady && Renderer && Renderer->IsVisualReady() && !Renderer->IsUsingFallback(); }
bool UMythicAppearancePreview::HasFailed() const { return bIdleFailed || (Renderer && Renderer->IsUsingFallback()); }
void UMythicAppearancePreview::Stop() {
    ++Generation; if (IdleLoad) { IdleLoad->CancelHandle(); IdleLoad.Reset(); }
    if(Scene)Scene->GetWorld()->GetTimerManager().ClearTimer(IdleDeadline);
    if (Capture) { Capture->TextureTarget=nullptr; Capture->SetComponentTickEnabled(false); }
    if (Avatar) Avatar->Destroy();
    Renderer=nullptr; Driver=nullptr; Capture=nullptr; Avatar=nullptr;
    Scene.Reset(); Target=nullptr; Frame=NAME_None; Accumulator=0;bIdleReady=false;bIdleFailed=false;PendingState={};
}
void UMythicAppearancePreview::BeginDestroy() { Stop(); Super::BeginDestroy(); }
