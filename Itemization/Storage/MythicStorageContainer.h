//

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/IMythicInteractable.h"
#include "Itemization/InventoryProviderInterface.h"
#include "Subsystem/SaveSystem/World/MythicSaveableActor.h"
#include "MythicStorageContainer.generated.h"

class UMythicInventoryComponent;
class UStaticMeshComponent;
class USceneComponent;
class UCommonGenericInputActionDataTable;
class AMythicPlayerController;

/**
 * A designer-placed storage container (chest / stash / locker). Hosts a single replicated inventory the player
 * can open and move items into/out of. A simpler sibling of AMythicConversionStation: same interaction + open
 * pattern and inventory provider, no recipes. Persists its contents through the world-save path via
 * IMythicSaveableActor. Slot count / accepted item types / player put-take rules come entirely from the
 * inventory's designer-assigned UInventoryProfile (data-driven, no hardcoding).
 */
UCLASS()
class MYTHIC_API AMythicStorageContainer : public AActor, public IMythicInteractable, public IInventoryProviderInterface, public IMythicSaveableActor {
    GENERATED_BODY()

public:
    AMythicStorageContainer();

    //~ IInventoryProviderInterface
    virtual TArray<UMythicInventoryComponent *> GetAllInventoryComponents() const override;
    virtual UAbilitySystemComponent *GetSchematicsASC() const override;

    //~ IMythicInteractable
    virtual void OnPrimaryInteract_Implementation(AActor *Interactor) override;
    virtual void OnSecondaryInteract_Implementation(AActor *Interactor) override;
    virtual USceneComponent *GetWidgetAttachmentComponent_Implementation() const override;
    virtual bool GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const override;
    virtual void OnFocused_Implementation(AActor *Interactor) override;
    virtual void OnUnfocused_Implementation(AActor *Interactor) override;

    //~ IMythicSaveableActor
    virtual void SerializeCustomData(TArray<uint8> &OutCustomData) override;
    virtual void DeserializeCustomData(const TArray<uint8> &InCustomData) override;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    UMythicInventoryComponent *GetContainerInventory() const { return ContainerInventory; }

    // True if Actor is within the configured use range (always true when ServerUseRangeSq <= 0).
    bool IsActorInRange(const AActor *Actor) const;

    // SERVER: opener registration so the move RPC can verify the player actually has THIS container open.
    void Server_AddOpener(AMythicPlayerController *PC);
    bool Server_IsOpener(const AMythicPlayerController *PC) const;
    void Server_RemoveOpener(AMythicPlayerController *PC);

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Storage")
    USceneComponent *SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Storage")
    UStaticMeshComponent *Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    UMythicInventoryComponent *ContainerInventory;

    // Interaction prompt data (same as the conversion station).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    TObjectPtr<const UCommonGenericInputActionDataTable> InputActionDataTable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FName PrimaryInteractionName = FName("Use");

    // Squared distance within which a player may open / move items. <= 0 disables the range gate.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storage")
    float ServerUseRangeSq = 250000.0f; // 500cm

    // Fired on the local interacting client so the Blueprint can push the dual-pane container WBP, binding the
    // player inventory VM + GetContainerInventory()->GetViewModel(). (Editor handoff, same as the station.)
    UFUNCTION(BlueprintImplementableEvent, Category = "Storage")
    void OnContainerOpened(APlayerController *Interactor);

    // Resolves the owning controller from an interactor that may be a pawn or a controller.
    static class AController *ResolveController(AActor *Interactor);

private:
    // Server-only set of players who currently have this container open. Pruned on EndPlay; the per-move range
    // check is the real gate, so a stale entry is harmless.
    TSet<TWeakObjectPtr<AMythicPlayerController>> Openers;
};
