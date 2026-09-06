#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/Entity/MythicEntityPresentationTypes.h"
#include "MythicAbilityPoseRemnant.generated.h"

class UMaterialInterface;
class UNiagaraSystem;
class USceneComponent;
class UMythicEntityPresentationComponent;

/** Authored cosmetics only. Lifetime and successful status-removal identity come from authority. */
UCLASS(BlueprintType)
class MYTHIC_API UMythicAbilityPoseRemnantStyle : public UDataAsset {
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TObjectPtr<UMaterialInterface> PoseMaterial;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TObjectPtr<UNiagaraSystem> AccentSystem;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FLinearColor Tint=FLinearColor::White;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(ClampMin="0",ClampMax="1")) float Opacity=0.7f;
};

/** Public cosmetic receipt: no removed effect context, status applier, damage or gameplay authority. */
USTRUCT()
struct FMythicAbilityPoseRemnantEvent {
    GENERATED_BODY()
    UPROPERTY() FGuid ActivationId;
    UPROPERTY() FGuid ReceiptId;
    UPROPERTY() FMythicEntityPresentationInstance Instance;
    UPROPERTY() TObjectPtr<AActor> Avatar;
    UPROPERTY() TObjectPtr<UMythicAbilityPoseRemnantStyle> Style;
    UPROPERTY() FTransform Departure;
    UPROPERTY() double ServerTime=0;
    UPROPERTY() float Duration=0;
    UPROPERTY() float CapsuleHalfHeight=0;
    bool HasValidKey() const { return ActivationId.IsValid() && ReceiptId.IsValid() && Instance.IsValid(); }
    bool SameKey(const FMythicAbilityPoseRemnantEvent& Other) const {
        return ReceiptId==Other.ReceiptId && ActivationId==Other.ActivationId && Instance==Other.Instance;
    }
};

USTRUCT()
struct FMythicAbilityPoseRemnantLease {
    GENERATED_BODY()
    UPROPERTY() FMythicAbilityPoseRemnantEvent Event;
    UPROPERTY() TArray<TObjectPtr<USceneComponent>> Components;
    TWeakObjectPtr<UMythicEntityPresentationComponent> Presentation;
    FDelegateHandle Deactivated;
    double LocalExpiry=0;
    int32 BoneCount=0;
};

/** Finite local component leases. No actor, collision, gameplay effect or component tick per remnant. */
UCLASS()
class MYTHIC_API UMythicAbilityPoseRemnantSubsystem : public UWorldSubsystem {
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Deinitialize() override;
    void Receive(const FMythicAbilityPoseRemnantEvent& Event,bool bCancel);
private:
    UPROPERTY() TMap<FGuid,FMythicAbilityPoseRemnantLease> Leases;
    struct FRetired { FMythicAbilityPoseRemnantEvent Key; double LocalExpiry=0; };
    TMap<FGuid,FRetired> Retired;
    FTimerHandle ExpiryTimer;
    int32 TotalBones=0;
    void Remove(FGuid ReceiptId);
    void Expire();
    void ScheduleExpiry();
};
