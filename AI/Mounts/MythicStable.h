#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/IMythicInteractable.h"
#include "MythicStable.generated.h"

class UStaticMeshComponent;
class UMythicMountRosterComponent;
class AMythicPlayerController;

UCLASS()
class MYTHIC_API AMythicStable : public AActor, public IMythicInteractable {
    GENERATED_BODY()

public:
    AMythicStable();

    virtual void OnPrimaryInteract_Implementation(AActor *Interactor) override;
    virtual USceneComponent *GetWidgetAttachmentComponent_Implementation() const override;
    virtual bool GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const override;
    virtual void OnFocused_Implementation(AActor *Interactor) override;
    virtual void OnUnfocused_Implementation(AActor *Interactor) override;

    bool IsActorInRange(const AActor *Actor) const;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stable")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stable")
    TObjectPtr<UStaticMeshComponent> Mesh;

    // Interaction prompt data (same mechanism as the storage container; author a "Stable" row on the BP).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    TObjectPtr<const UCommonGenericInputActionDataTable> InputActionDataTable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FName PrimaryInteractionName = FName("Stable");

    // Squared distance within which a player may use the stable. <= 0 disables the range gate.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stable")
    float InteractRangeSq = 250000.0f; // 500cm

    // SERVER BP hook after a successful use (bStabled: true = mount stashed, false = mount retrieved) so the BP can
    // play doors/hay/VFX or drive a stable UI. Editor handoff — no stable widget invented in C++.
    UFUNCTION(BlueprintImplementableEvent, Category = "Stable")
    void OnStableUsed(APlayerController *Interactor, bool bStabled);

    static AMythicPlayerController *ResolveMythicPC(AActor *Interactor);

    static UMythicMountRosterComponent *ResolveRoster(const AMythicPlayerController *PC);
};
