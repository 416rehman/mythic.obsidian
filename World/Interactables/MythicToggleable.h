// Mythic — toggleable world interactable (door / gate / lever / switch)
// A server-authoritative on/off world object: interacting flips its state (server), which replicates (dormant until
// it changes) and persists (SaveGame). A lever can drive remote targets via LinkedToggleables so a switch here opens
// a gate there. The visual (mesh swap / open animation / sound) is a cosmetic Blueprint reaction — never replicated.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/IMythicInteractable.h"
#include "Subsystem/SaveSystem/World/MythicSaveableActor.h"
#include "MythicToggleable.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UCommonGenericInputActionDataTable;

/** Pure outcome of a toggle interaction (what the state should become, and whether it changed). */
struct FMythicToggleOutcome {
    bool bChanged = false; // did the on/off state actually change → replicate + persist + propagate
    bool bNewIsOn = false; // the resulting state
};

UCLASS()
class MYTHIC_API AMythicToggleable : public AActor, public IMythicInteractable, public IMythicSaveableActor {
    GENERATED_BODY()

public:
    AMythicToggleable();

    //~ IMythicInteractable
    virtual void OnPrimaryInteract_Implementation(AActor *Interactor) override;
    virtual void OnSecondaryInteract_Implementation(AActor *Interactor) override;
    virtual USceneComponent *GetWidgetAttachmentComponent_Implementation() const override;
    virtual bool GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const override;
    virtual void OnFocused_Implementation(AActor *Interactor) override;
    virtual void OnUnfocused_Implementation(AActor *Interactor) override;

    UFUNCTION(BlueprintPure, Category = "Toggleable")
    bool IsOn() const { return bIsOn; }

    // SERVER: apply a toggle interaction (respects lock / one-shot), then mirror the new state to LinkedToggleables.
    // Public so a lever-link or a debug command can drive it; no-op off authority.
    void ServerToggle(AActor *Interactor);

    // Pure toggle decision — locked → no change; one-shot already fired → no change; one-shot fresh → turn ON;
    // otherwise flip. Static + no engine state so the rule is unit-testable without a live actor.
    static FMythicToggleOutcome ResolveToggle(bool bCurrentlyOn, bool bLocked, bool bOneShot, bool bHasActivated);

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    UFUNCTION()
    void OnRep_IsOn();

    // Cosmetic open/close reaction (mesh swap / animation / sound). Fires on the acting server AND on clients via
    // OnRep. LOCAL/cosmetic — it never gates gameplay (the authoritative state is bIsOn).
    UFUNCTION(BlueprintImplementableEvent, Category = "Toggleable")
    void OnToggleVisualChanged(bool bNewIsOn);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Toggleable")
    USceneComponent *SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Toggleable")
    UStaticMeshComponent *Mesh;

    // Replicated + persisted on/off state (door open, lever thrown, gate raised).
    UPROPERTY(ReplicatedUsing = OnRep_IsOn, SaveGame, BlueprintReadOnly, Category = "Toggleable")
    bool bIsOn = false;

    // Whether a one-shot has already fired (server logic; persisted so a fired one-shot stays fired across save/load).
    UPROPERTY(SaveGame)
    bool bHasActivated = false;

    // Initial state when first placed (before any save restore).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toggleable")
    bool bStartsOn = false;

    // Locked: direct interaction does nothing until another system unlocks it (keyed door, quest gate).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toggleable")
    bool bLocked = false;

    // One-shot: can only be switched ON once (a one-time portcullis lever); further interacts no-op.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toggleable")
    bool bOneShot = false;

    // Remote toggleables this one drives (a lever → its gates). On a state change each is mirrored to THIS object's
    // new state (one hop, no cascade — a linked target's own lock is bypassed; that's the lever's authority).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toggleable")
    TArray<TObjectPtr<AMythicToggleable>> LinkedToggleables;

    // Interaction prompt data (matches the container/station pattern).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    TObjectPtr<const UCommonGenericInputActionDataTable> InputActionDataTable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FName PrimaryInteractionName = FName("Use");

private:
    // SERVER: set the replicated state, fire the visual, wake dormancy so the change replicates. Used by the self
    // toggle and by link propagation.
    void ApplyState(bool bNewIsOn);
};
