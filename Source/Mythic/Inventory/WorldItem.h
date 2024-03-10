// 

#pragma once

#include "CoreMinimal.h"
#include "ItemInstance.h"
#include "GameFramework/Actor.h"
#include "WorldItem.generated.h"


// This is a world representation of an item. Call SetItemInstance after spawning to set the item instance.
UCLASS(Blueprintable, BlueprintType)

class MYTHIC_API AWorldItem : public AActor {
private:
    GENERATED_BODY()

protected:

    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    // Called when the item is going to be destroyed
    void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void AddToUnclaimed();

    // For all. If true, the item will be visible to all players. If false, it will only be visible to owner.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", ReplicatedUsing=OnRep_IsPrivate)
    bool IsPrivateDrop;

public:    
    // The item instances that this world item represents. Replicated OnRep.
    UPROPERTY(ReplicatedUsing = OnRep_ItemInstances, VisibleAnywhere, BlueprintReadOnly, Category = "Item")
    TArray<UItemInstance*> ItemInstances;
    
    // Static mesh for the item. This will be the root component.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
    UStaticMeshComponent *StaticMesh;
    
    AWorldItem();

    // get lifetime replicated props
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        DOREPLIFETIME(AWorldItem, ItemInstances);
        DOREPLIFETIME(AWorldItem, IsPrivateDrop);
    }

    UFUNCTION(BlueprintCallable, Category = "Item")
    void SetIsPrivate(bool is_private);
    
    UFUNCTION()
    void OnRep_IsPrivate();

    UFUNCTION(BlueprintCallable, Category = "Item")
    void AddItemInstance(UItemInstance *ItemInst);

    UFUNCTION()
    void OnRep_ItemInstances();

    bool IsPrivate() const { return IsPrivateDrop; }

    /// ----------------
    /// EVENTS
    /// ----------------
    
    /// this event is called when the item instance is updated
    UFUNCTION(BlueprintImplementableEvent, Category = "Item")
    void OnItemInstanceUpdated();
};
