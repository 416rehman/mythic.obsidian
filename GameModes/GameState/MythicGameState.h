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

    // Flat MaxHealth granted per character level (and healed immediately) when ProcessLevelUps fires. 0 = leveling
    // grants no health. The single source for the per-level health reward.
    UPROPERTY(EditDefaultsOnly, Category = "Mythic")
    float HealthPerLevel = 20.0f;

    // Flat Armor (damage mitigation, consumed every hit by the damage execution) granted per character level when
    // ProcessLevelUps fires. 0 = leveling grants no armor. The single source for the per-level armor reward.
    UPROPERTY(EditDefaultsOnly, Category = "Mythic")
    float ArmorPerLevel = 2.0f;

    // Raw Armor -> incoming-damage reduction fraction [0,1]. Used by the damage Application execution.
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    FCurveTableRowHandle ArmorMitigationCurveRowHandle;

    // Post-mitigation damage floor: high Armor can never reduce a non-zero hit below this (keeps targets killable).
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    float MinChipDamage = 1.0f;

    // Status-tag damage modifiers, consumed by the damage Application execution PRE-mitigation. Source RAGE deals
    // more / WEAKENED deals less; target TERRIFIED takes more / FORTIFY takes less. Designer-tunable fractions.
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    float RageDamageBonus = 0.25f;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    float WeakenedDamagePenalty = 0.25f;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    float TerrifiedDamageBonus = 0.25f;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    float FortifyDamageReduction = 0.25f;

    // Bonus XP fraction while GAS.Buff.Enlighten is active, applied at the canonical XP funnel (AddExperience).
    // Same designer-tunable shape as the four status-damage scalars above. Ex: 0.5 = +50% XP gain.
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    float EnlightenXpBonus = 0.5f;

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
