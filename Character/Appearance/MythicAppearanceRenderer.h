#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/StreamableManager.h"
#include "MythicAppearanceTypes.h"
#include "MythicAppearanceRenderer.generated.h"

class AMythicCharacter;
class UMythicAppearanceCatalog;
class USkeletalMeshComponent;
class UAnimInstance;
class UAnimMontage;

/** Relevant-avatar projection and local rendering. Animation ticks; outfit resolution never polls inventory. */
UCLASS(ClassGroup=(Mythic), meta=(BlueprintSpawnableComponent))
class MYTHIC_API UMythicAppearanceRenderer : public UActorComponent {
    GENERATED_BODY()
public:
    UMythicAppearanceRenderer();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    virtual void TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Function) override;
    void Publish(const FMythicAppearancePublicState& State);
    void RefreshBinding();
    const FMythicAppearancePublicState& GetPublicState() const { return PublicState; }
    void GetWorldParts(TArray<USkeletalMeshComponent*>& Out) const;
    /** Preview uses the same catalog/renderer without writing accepted or replicated state. */
    void ApplyPreview(const FMythicAppearancePublicState& State, USkeletalMeshComponent* Driver);
    UFUNCTION(BlueprintPure) bool IsVisualReady() const { return !bLoading && !CurrentParts.IsEmpty(); }
    UFUNCTION(BlueprintPure) bool IsUsingFallback() const { return bFallback; }
private:
    UFUNCTION() void OnRep_PublicState();
    void Apply(const FMythicAppearancePublicState& State, bool bAsFallback=false);
    void BuildCandidate(uint32 ExpectedGeneration,FMythicAppearancePublicState State);
    void ApplyColors(const FMythicAppearancePublicState& State,const TMap<FName,TObjectPtr<USkeletalMeshComponent>>& Parts);
    void ShowFallback();
    void QueueGameplayAnimation(FName Frame);
    void TryGameplayAnimation();
    UFUNCTION() void OnGameplayMontageEnded(UAnimMontage* Montage,bool bInterrupted);
    void DestroyParts(TMap<FName,TObjectPtr<USkeletalMeshComponent>>& Parts);
    USkeletalMeshComponent* Driver() const;
    UPROPERTY(ReplicatedUsing=OnRep_PublicState) FMythicAppearancePublicState PublicState;
    UPROPERTY(Transient) TObjectPtr<UMythicAppearanceCatalog> Catalog;
    UPROPERTY(Transient) TMap<FName,TObjectPtr<USkeletalMeshComponent>> CurrentParts;
    UPROPERTY(Transient) TMap<FName,TObjectPtr<USkeletalMeshComponent>> CandidateParts;
    UPROPERTY(Transient) FMythicAppearancePublicState RenderedState;
    UPROPERTY(Transient) FMythicAppearancePublicState CandidateState;
    UPROPERTY(Transient) FMythicAppearancePublicState RequestedState;
    UPROPERTY(Transient) TArray<TObjectPtr<USkeletalMeshComponent>> AuthoredBaseline;
    UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> PreviewDriver;
    TSharedPtr<FStreamableHandle> Load;
    TSharedPtr<FStreamableHandle> GameplayLoad;
    TWeakObjectPtr<UAnimInstance> WaitingAnimation;
    FName PendingGameplayFrame;
    FTimerHandle GameplayRetry;
    FTimerHandle LoadDeadline;
    uint32 Generation = 0;
    TMap<FName,uint32> CandidatePoseRevisions;
    bool bLoading = false;
    bool bFallback = false;
};
