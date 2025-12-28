// 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Mythic/Subsystem/SaveSystem/World/MythicSaveableActor.h"
#include "Net/UnrealNetwork.h"
#include "MythicWorldItem.generated.h"


// This is a world representation of an item. Call SetItemInstance after spawning to set the item instance.
UCLASS(Blueprintable, BlueprintType)
class MYTHIC_API AMythicWorldItem : public AActor, public IMythicSaveableActor {
    GENERATED_BODY()

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    // Called when the item is going to be destroyed
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void AddToUnclaimed();

    // For all. If this is set, the item will only be relevant to the owner, and hidden from other players.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", ReplicatedUsing=OnRep_TargetRecipient)
    AController *TargetRecipient;

public:
    AMythicWorldItem();

    // The item instance that this world item represents. Replicated OnRep.
    UPROPERTY(ReplicatedUsing = OnRep_ItemInstance, VisibleAnywhere, BlueprintReadOnly, Category = "Item")
    UMythicItemInstance *ItemInstance;

    // Static mesh for the item. This will be the root component.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
    UStaticMeshComponent *StaticMesh;


    // get lifetime replicated props
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        DOREPLIFETIME(AMythicWorldItem, ItemInstance);
        DOREPLIFETIME(AMythicWorldItem, TargetRecipient);
    }

    UFUNCTION(BlueprintCallable, Category = "Item")
    void SetTargetRecipient(AController *NewTargetRecipient);

    UFUNCTION()
    void OnRep_TargetRecipient();

    UFUNCTION(BlueprintCallable, Category = "Item")
    void SetItemInstance(UMythicItemInstance *ItemInst);

    UFUNCTION()
    void OnRep_ItemInstance();

    AController *GetTargetRecipient() const { return TargetRecipient; }

    /// ----------------
    /// EVENTS
    /// ----------------

    /// this event is called when the item instance is updated
    UFUNCTION(BlueprintImplementableEvent, Category = "Item")
    void OnItemInstanceUpdated();

    UFUNCTION()
    void OnHit(UPrimitiveComponent *HitComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp, FVector NormalImpulse, const FHitResult &Hit);

    UFUNCTION()
    void EmulateDropPhysics(const FVector &location, float radius);

    // --- IMythicSaveableActor ---
    virtual void SerializeCustomData(TArray<uint8> &OutCustomData) override;
    virtual void DeserializeCustomData(const TArray<uint8> &InCustomData) override;
};
