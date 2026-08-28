

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Mythic/Subsystem/SaveSystem/World/MythicSaveableActor.h"
#include "Net/UnrealNetwork.h"
#include "MythicWorldItem.generated.h"

class UMythicItemInstance;

UCLASS(Blueprintable, BlueprintType)
class MYTHIC_API AMythicWorldItem : public AActor, public IMythicSaveableActor {
    GENERATED_BODY()

protected:
    virtual void PostInitializeComponents() override;

    virtual void BeginPlay() override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Optional controller that exclusively sees and may collect this private drop. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", ReplicatedUsing=OnRep_TargetRecipient)
    AController *TargetRecipient;

    /** Enables walk-over pickup for stackable loot; disable for deliberate interactions such as quest hand-ins. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
    bool bAutoPickup = true;

    /**
     * Persistent identity for a runtime-spawned world drop. Actor object names are allocation-order dependent, so
     * they are not a safe save identity across reconnects. Placed actors retain their package path identity.
     */
    UPROPERTY(SaveGame)
    FGuid WorldItemSaveGuid;

public:
    AMythicWorldItem();

    /** Authoritative item instance represented by this replicated world drop. */
    UPROPERTY(ReplicatedUsing = OnRep_ItemInstance, VisibleAnywhere, BlueprintReadOnly, Category = "Item")
    UMythicItemInstance *ItemInstance;

    /** Root mesh used for world presentation, collision, and drop physics. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
    UStaticMeshComponent *StaticMesh;


    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        DOREPLIFETIME(AMythicWorldItem, ItemInstance);
        DOREPLIFETIME(AMythicWorldItem, TargetRecipient);
    }

    /** Assigns or clears the controller that exclusively sees and may collect this drop. */
    UFUNCTION(BlueprintCallable, Category = "Item")
    void SetTargetRecipient(AController *NewTargetRecipient);

    /** Applies client visibility after the replicated private-recipient changes. */
    UFUNCTION()
    void OnRep_TargetRecipient();

    /** Publishes an authoritative item instance into this world drop. */
    UFUNCTION(BlueprintCallable, Category = "Item")
    void SetItemInstance(UMythicItemInstance *ItemInst);

    /** Refreshes world-item presentation after the represented item instance changes. */
    UFUNCTION()
    void OnRep_ItemInstance();

    AController *GetTargetRecipient() const { return TargetRecipient; }

    virtual FString GetSaveableActorId() const override;

    /**
     * Transactional structural restore seam. The existing item remains published if the frame, nested current item
     * payload, loaded-class dependency, or stable identity fails. Affix semantics close asynchronously afterward and
     * remain quarantined from stat application until their live definitions validate.
     */
    bool TryDeserializeCustomData(const TArray<uint8> &InCustomData);


    /** Blueprint presentation hook invoked whenever the represented item instance changes. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Item")
    void OnItemInstanceUpdated();

    /** Stops drop physics after the item lands on an upward-facing surface. */
    UFUNCTION()
    void OnHit(UPrimitiveComponent *HitComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp, FVector NormalImpulse, const FHitResult &Hit);

    /** Launches this drop toward a nearby navigable point using physics. */
    UFUNCTION()
    void EmulateDropPhysics(const FVector &location, float radius);

    static bool ShouldAutoPickup(int32 StackSizeMax) { return StackSizeMax > 1; }

    /** Attempts authority-only walk-over pickup for an eligible stackable item. */
    UFUNCTION()
    void OnPickupOverlap(UPrimitiveComponent *OverlappedComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex,
                         bool bFromSweep, const FHitResult &SweepResult);

    virtual void SerializeCustomData(TArray<uint8> &OutCustomData) override;
    virtual void DeserializeCustomData(const TArray<uint8> &InCustomData) override;

private:
    bool CommitStagedRestore(UMythicItemInstance *StagedItem, const FGuid &RestoredSaveGuid);
};
