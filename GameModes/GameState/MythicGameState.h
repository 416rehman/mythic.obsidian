// 

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "ModularGameState.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "Resources/MythicResourceManagerComponent.h"
#include "MythicGameState.generated.h"

struct FTrackedDestructibleDataArray;
class UMythicResourceManagerComponent;
class URewardManager;
class UWorldTierAttributes;
/**
 * 
 */
UCLASS(Config = Game, Abstract, Blueprintable, BlueprintType)
class MYTHIC_API AMythicGameState : public AGameStateBase, public IAbilitySystemInterface {
    GENERATED_BODY()

    UPROPERTY()
    bool IsSessionJoinTimeInitialized = false;

    virtual void OnRep_ReplicatedWorldTimeSecondsDouble() override;

protected:
    // The ability system component subobject for game-wide things (primarily gameplay cues)
    UPROPERTY(Replicated, VisibleAnywhere, Category = "Mythic")
    TObjectPtr<UMythicAbilitySystemComponent> AbilitySystemComponent;

    // Resource Manager Component - Used to track instanced static mesh based resources
    UPROPERTY(Blueprintable, BlueprintReadOnly, Replicated, VisibleAnywhere, Category = "Mythic")
    TObjectPtr<UMythicResourceManagerComponent> ResourceManagerComponent;

    // Array of World Attributes Initialization Effects - Index 0 is the lowest tier, index 1 is the next tier, etc.
    UPROPERTY(EditDefaultsOnly, Category = "Mythic")
    TSubclassOf<UGameplayEffect> WorldTierAttributesInitializationEffect;

    // Handle to the currently active World Tier Attributes Initialization Effect
    UPROPERTY(BlueprintReadOnly)
    FActiveGameplayEffectHandle ActiveWorldTierInitEffectHandle;

public:
    // Curve Table Row Handle for XP Levels
    UPROPERTY(EditDefaultsOnly, Category = "Mythic")
    FCurveTableRowHandle XPLevelsCurveRowHandle;

    // Minimum Health Curve Row Handle - Used for initializing the health of the NPC's at their level
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    FCurveTableRowHandle HealthMinCurveRowHandle;

    // Maximum Health Curve Row Handle - Used for initializing the health of the NPC's at their level
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    FCurveTableRowHandle HealthMaxCurveRowHandle;

    // Minimum Damage Curve Row Handle - Used for initializing the damage of the NPC's at their level
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    FCurveTableRowHandle DamageMinCurveRowHandle;

    // Maximum Damage Curve Row Handle - Used for initializing the damage of the NPC's at their level
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    FCurveTableRowHandle DamageMaxCurveRowHandle;

    // Common Loot Chance Curve Row Handle - Used for determining the chance of common loot drops at a given level
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Loot")
    FCurveTableRowHandle CommonLootChanceCurveRowHandle;

    // Rare Loot Chance Curve Row Handle - Used for determining the chance of uncommon loot drops at a given level
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Loot")
    FCurveTableRowHandle RareLootChanceCurveRowHandle;

    // Epic Loot Chance Curve Row Handle - Used for determining the chance of rare loot drops at a given level
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Loot")
    FCurveTableRowHandle EpicLootChanceCurveRowHandle;

    // Legendary Loot Chance Curve Row Handle - Used for determining the chance of legendary loot drops at a given level
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Loot")
    FCurveTableRowHandle LegendaryLootChanceCurveRowHandle;

    // Mythic Loot Chance Curve Row Handle - Used for determining the chance of mythic loot drops at a given level
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Loot")
    FCurveTableRowHandle MythicLootChanceCurveRowHandle;

    // World tier attributes
    UPROPERTY(BlueprintReadOnly)
    UWorldTierAttributes *WorldTierAttributes;

    // Default World Tier
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic")
    uint8 WorldTier;

    // Max World Tier - Used to clamp the World Tier to a maximum value
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic")
    uint8 MaxWorldTier = 4;

    void SetWorldTier(uint8 NewWorldTier);

    AMythicGameState(const FObjectInitializer &ObjectInitializer = FObjectInitializer::Get());

    //~AActor interface
    virtual void PostInitializeComponents() override;
    virtual void BeginPlay() override;

    //~IAbilitySystemInterface
    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;
    //~End of IAbilitySystemInterface

    // Gets the ability system component used for game wide things
    UFUNCTION(BlueprintCallable, Category = "Mythic")
    UMythicAbilitySystemComponent *GetMythicAbilitySystemComponent() const { return AbilitySystemComponent; }

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    // Returns array of all the tracked destructibles
    TArray<FTrackedDestructibleData> GetTrackedDestructibles() const;
};
