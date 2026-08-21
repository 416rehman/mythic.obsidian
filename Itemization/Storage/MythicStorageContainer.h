
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
class UMythicLootTable;

UCLASS()
class MYTHIC_API AMythicStorageContainer : public AActor, public IMythicInteractable, public IInventoryProviderInterface, public IMythicSaveableActor {
    GENERATED_BODY()

public:
    AMythicStorageContainer();

    virtual TArray<UMythicInventoryComponent *> GetAllInventoryComponents() const override;
    virtual UAbilitySystemComponent *GetSchematicsASC() const override;

    virtual void OnPrimaryInteract_Implementation(AActor *Interactor) override;
    virtual void OnSecondaryInteract_Implementation(AActor *Interactor) override;
    virtual USceneComponent *GetWidgetAttachmentComponent_Implementation() const override;
    virtual bool GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const override;
    virtual void OnFocused_Implementation(AActor *Interactor) override;
    virtual void OnUnfocused_Implementation(AActor *Interactor) override;

    virtual void SerializeCustomData(TArray<uint8> &OutCustomData) override;
    virtual void DeserializeCustomData(const TArray<uint8> &InCustomData) override;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    UMythicInventoryComponent *GetContainerInventory() const { return ContainerInventory; }

    /** True when no slot holds an item. Used by the restock gate; also useful to a Blueprint for open/empty visuals. */
    UFUNCTION(BlueprintPure, Category = "Storage")
    bool IsEmpty() const;

    /**
     * SERVER: roll StockTables into this container's own inventory. Called once at BeginPlay and again on the
     * restock timer. Safe to call by hand (a quest handing a stash fresh goods). No-op off authority, with no
     * tables, or when the loot subsystem is unavailable. Returns the number of item stacks actually added.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Storage")
    int32 ServerStock();

    bool IsActorInRange(const AActor *Actor) const;

    void Server_AddOpener(AMythicPlayerController *PC);
    bool Server_IsOpener(const AMythicPlayerController *PC) const;
    void Server_RemoveOpener(AMythicPlayerController *PC);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


    /**
     * Loot tables rolled into this container on the server when play begins. Empty = the container starts empty
     * and stays that way (what every placement did before this existed).
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storage|Stock")
    TArray<TObjectPtr<UMythicLootTable>> StockTables;

    /** Item level stocked items roll at. A world container has no player to scale against, so this is flat. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storage|Stock", meta = (ClampMin = "0"))
    int32 StockItemLevel = 1;

    /**
     * Drop chance for a stock entry that sets no OverrideDropChance. The loot-reward path would read a
     * level-scaled per-rarity weight here, but a container stocking itself has no player to read — this flat
     * value replaces it, so a container's contents are a designer decision rather than a function of whoever
     * happens to walk past.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storage|Stock", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float StockDefaultEntryChance = 0.5f;

    /**
     * Seconds between restock passes. <= 0 disables restocking entirely: the container is stocked once and never
     * refills, so looting it is a one-time event. A positive value is what makes robbing a town repeatable.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storage|Stock", meta = (ClampMin = "0.0"))
    float RestockIntervalSeconds = 0.0f;

    /** Restock only when the container is completely empty, rather than topping it up on every tick of the timer. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Storage|Stock")
    bool bRestockOnlyWhenEmpty = true;

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

    static class AController *ResolveController(AActor *Interactor);

private:
    TSet<TWeakObjectPtr<AMythicPlayerController>> Openers;

    FTimerHandle RestockTimer;

    void ServerRestockTick();
};
