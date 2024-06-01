// 

#pragma once

#include "CoreMinimal.h"
#include "MythicLootManagerSubsystem.h"
#include "MythicLootSource.h"
#include "Components/ActorComponent.h"
#include "AbilitySystemComponent.h"
#include "MythicLootSourceComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYTHIC_API UMythicLootSourceComponent : public UActorComponent {
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UMythicLootSourceComponent();

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

    // LootSourceDA to use for this component
    UPROPERTY(EditDefaultsOnly, Category="Loot")
    class UMythicLootSource* LootSource;

    // The loot manager subsystem
    UPROPERTY()
    UMythicLootManagerSubsystem * LootManager;
    
    // The radius to spawn loot
    UPROPERTY(EditAnywhere, Category="Loot")
    float SpawnRadius;

public:
    // Set the loot source data asset
    UFUNCTION(BlueprintCallable, Category="Loot")
    void SetLootSource(class UMythicLootSource* NewLootSource);
    
    UFUNCTION(BlueprintCallable, Category="Loot")
    void RequestLoot(UAbilitySystemComponent *AbilitySystemComponent, ELootType Type);
};
