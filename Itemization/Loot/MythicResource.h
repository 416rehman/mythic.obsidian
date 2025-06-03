// 

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MythicResource.generated.h"

class UMythicAttributeSet_Life;
class UMythicAttributeSet;

UCLASS()
class MYTHIC_API AMythicResource : public AActor, public IAbilitySystemInterface {
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    AMythicResource();
    bool TakePlaceOfISM();

protected:
    // Ability System Component for this actor
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "MythicResource")
    UMythicAbilitySystemComponent *AbilitySystemComponent;

    // Default Ability Set
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MythicResource")
    TArray<TSubclassOf<UMythicGameplayAbility>> DefaultAbilities;

    // Default Gameplay Effects - Can also be used to initialize attributes.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MythicResource")
    TArray<TSubclassOf<UGameplayEffect>> DefaultGameplayEffects;

    // Life attributes
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "MythicResource")
    UMythicAttributeSet_Life *LifeAttributes;

    // Should the ISM detection trace be complex
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MythicResource")
    bool bTraceComplex = false;

    // ISM Trace Radius
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MythicResource")
    int Radius = 10;

    // ISM Trace Object Types
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MythicResource")
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes = {UEngineTypes::ConvertToObjectType(ECC_WorldStatic)};

    // ISM Trace Debug Type
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "MythicResource")
    TEnumAsByte<EDrawDebugTrace::Type> ISMDetectionTraceDebug = EDrawDebugTrace::None;

    // Static Mesh component
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MythicResource")
    UStaticMeshComponent *ReplacementMeshComponent;

    // The ISM that this resource was spawned from
    UPROPERTY(BlueprintReadOnly, Category = "MythicResource")
    UInstancedStaticMeshComponent *Source_ISM;

public:
    void OnHit(AActor *Actor, AActor *Actor1, const FGameplayEffectSpec *GameplayEffectSpec, float X, float Arg, float X1);
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    // Implement IAbilitySystemInterface
    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override {
        return AbilitySystemComponent;
    }

    // The number of hits till this resource is destroyed, based on the scale of the ISM
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "MythicResource")
    int32 HitsTillDestruction = 1;

    // Event for when the hits till destruction changes - Only called on the server
    UFUNCTION(BlueprintImplementableEvent, Category = "MythicResource")
    void OnResourceHit(AActor *Actor, int32 RemainingHits);

    // Event for when the resource is destroyed - Only called on the server
    UFUNCTION(BlueprintImplementableEvent, Category = "MythicResource")
    void OnResourceDestroyed(AActor *Actor);

    // Algorithm to determine the number of hits till destruction - MUST OVERRIDE IN BLUEPRINT
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MythicResource")
    int32 CalculateHitsTillDestruction();
};
