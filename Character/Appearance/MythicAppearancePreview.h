#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/StreamableManager.h"
#include "MythicAppearanceTypes.h"
#include "MythicAppearancePreview.generated.h"

class FPreviewScene;
class UTextureRenderTarget2D;
class USkeletalMeshComponent;
class USceneCaptureComponent2D;
class UMythicAppearanceRenderer;

/** One isolated scene per open editor. Contains no pawn, inventory, ASC or network driver. */
UCLASS()
class MYTHIC_API UMythicAppearancePreview : public UObject {
    GENERATED_BODY()
public:
    UMythicAppearancePreview();
    virtual ~UMythicAppearancePreview() override;
    virtual void BeginDestroy() override;
    bool Start(USkeletalMeshComponent* CanonicalDriver);
    void Stop();
    void Apply(const FMythicAppearancePublicState& State);
    void Tick(float Delta);
    void Orbit(float Degrees);
    void Zoom(float Amount);
    void ResetView();
    UTextureRenderTarget2D* GetTexture() const { return Target; }
    bool IsReady() const;
    bool HasFailed() const;
private:
    void PlaceCamera();
    TUniquePtr<FPreviewScene> Scene;
    TSharedPtr<FStreamableHandle> IdleLoad;
    UPROPERTY(Transient) TObjectPtr<AActor> Avatar;
    UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> Driver;
    UPROPERTY(Transient) TObjectPtr<UMythicAppearanceRenderer> Renderer;
    UPROPERTY(Transient) TObjectPtr<USceneCaptureComponent2D> Capture;
    UPROPERTY(Transient) TObjectPtr<UTextureRenderTarget2D> Target;
    FName Frame;
    FMythicAppearancePublicState PendingState;
    FTimerHandle IdleDeadline;
    bool bIdleReady=false,bIdleFailed=false;
    uint32 Generation=0;
    float Accumulator=0, Yaw=25, Distance=300;
};
