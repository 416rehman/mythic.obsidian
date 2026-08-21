#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/IMythicInteractable.h"
#include "World/Hunting/MythicSpoorRules.h"
#include "MythicSpoorTrail.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class USphereComponent;
class UCommonGenericInputActionDataTable;

UENUM(BlueprintType)
enum class EMythicSpoorReadResult : uint8 {
    Cold,
    Revealed,
    Capped,
    TrailEnd
};

UCLASS()
class MYTHIC_API AMythicSpoorTrail : public AActor, public IMythicInteractable {
    GENERATED_BODY()

public:
    AMythicSpoorTrail();

    void ServerInitTrailNode(const FVector &InAnchorLocation, int32 InStepsRemaining, const FMythicSpoorConfig &InConfig,
                             bool bRainingAtSpawn);

    EMythicSpoorReadResult ServerHandleRead();

    virtual void OnPrimaryInteract_Implementation(AActor *Interactor) override;
    virtual void OnSecondaryInteract_Implementation(AActor *Interactor) override {}
    virtual USceneComponent *GetWidgetAttachmentComponent_Implementation() const override;
    virtual bool GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const override;
    virtual void OnFocused_Implementation(AActor *Interactor) override {}
    virtual void OnUnfocused_Implementation(AActor *Interactor) override {}

    float GetFreshness() const;

    static int32 CountNodesNear(const UWorld *World, const FVector &Location, float RadiusCm);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spoor")
    USceneComponent *SceneRoot;

    // Query-only collision proxy so the node is detected by the interaction sweep even before a BP assigns a visual
    // (mirrors AMythicCorpse::InteractionBounds).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spoor")
    USphereComponent *InteractionBounds;

    // Placeholder visual (a BP subclass supplies paw-print decals etc.).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spoor")
    UStaticMeshComponent *Mesh;

    // Interaction prompt data (matches the toggleable/spot pattern).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    TObjectPtr<const UCommonGenericInputActionDataTable> InputActionDataTable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FName PrimaryInteractionName = FName("ReadTracks");

private:
    FVector AnchorLocation = FVector::ZeroVector;
    int32 StepsRemaining = 0;
    FMythicSpoorConfig Config;
    double SpawnServerTime = 0.0;
    float EffectiveLifetimeSeconds = 600.0f;
    bool bConsumed = false;
};
