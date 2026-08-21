#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/IMythicInteractable.h"
#include "World/Secrets/MythicSecretTypes.h"
#include "World/Placement/MythicProxyStateful.h"
#include "MythicSecretInteractable.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UCommonGenericInputActionDataTable;
class AController;

UCLASS()
class MYTHIC_API AMythicSecretInteractable : public AActor, public IMythicInteractable, public IMythicProxyStateful {
    GENERATED_BODY()

public:
    AMythicSecretInteractable();

    // ── Placed-proxy support ──
    // Secrets are the safest thing in the world to proxy: a mesh, a prompt, and one bit of world-shared state. They are
    // also the most numerous, so keeping thousands of them as live actors is the clearest waste in an open world.
    // This component (inert unless enabled) hands the actor to UMythicPlacedProxySubsystem on load and lets the
    // registry spawn it back only when a player is close enough to examine it.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Proxy")
    TObjectPtr<class UMythicProxyRegistrationComponent> ProxyRegistration;

    virtual int32 GetProxyStateFlags_Implementation() const override;
    virtual void ApplyProxyStateFlags_Implementation(int32 StateFlags) override;

    virtual void OnPrimaryInteract_Implementation(AActor *Interactor) override;
    virtual void OnSecondaryInteract_Implementation(AActor *Interactor) override;
    virtual USceneComponent *GetWidgetAttachmentComponent_Implementation() const override;
    virtual bool GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const override;
    virtual void OnFocused_Implementation(AActor *Interactor) override;
    virtual void OnUnfocused_Implementation(AActor *Interactor) override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Secret")
    USceneComponent *SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Secret")
    UStaticMeshComponent *Mesh;

    // The authored secret delivered on interact.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Secret")
    FMythicSecretDef Def;

    // When TRUE, this interactable fires at most ONCE total (the first finder only) — a world-shared one-shot. DEFAULT
    // FALSE: co-op-friendly — every player can find it (dedup is the per-player FoundTag latch + the session guard below).
    // Mirrors AMythicSecretVolume::bGlobalOneShot exactly.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secret")
    bool bGlobalOneShot = false;

    // Interaction prompt data (matches the container/station/toggleable pattern).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    TObjectPtr<const UCommonGenericInputActionDataTable> InputActionDataTable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FName PrimaryInteractionName = FName("Examine");

private:
    static AController *ResolveInteractorController(AActor *Interactor);

    TSet<TWeakObjectPtr<AController>> RevealedControllers;

    bool bGlobalConsumed = false;
};
