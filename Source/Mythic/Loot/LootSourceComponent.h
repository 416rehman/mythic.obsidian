// 

#pragma once

#include "CoreMinimal.h"
#include "LootManagerSubsystem.h"
#include "LootSourceDA.h"
#include "Components/ActorComponent.h"
#include "AbilitySystemComponent.h"
#include "LootSourceComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYTHIC_API ULootSourceComponent : public UActorComponent {
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    ULootSourceComponent();

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

    // LootSourceDA to use for this component
    UPROPERTY(EditDefaultsOnly, Category="Loot")
    class ULootSourceDA* LootSource;

    // The loot manager subsystem
    UPROPERTY()
    ULootManagerSubsystem * LootManager;
    
    // The radius to spawn loot
    UPROPERTY(EditAnywhere, Category="Loot")
    float SpawnRadius;

public:
    // Set the loot source data asset
    UFUNCTION(BlueprintCallable, Category="Loot")
    void SetLootSource(class ULootSourceDA* NewLootSource);
    
    // UFUNCTION(BlueprintCallable, Category="Loot")
    // void RequestLoot(UAbilitySystemComponent *AbilitySystemComponent, ELootType Type);
};
