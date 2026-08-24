
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
    // raw armor -> incoming-damage reduction fraction [0,1], used by the damage application execution
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    FCurveTableRowHandle ArmorMitigationCurveRowHandle;

    /**
     * The one authored armor curve, readable by UI and summary calculations so a displayed mitigation can
     * never drift from what the damage execution applies. Returns the same clamped fraction it uses.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Combat", meta = (WorldContext = "WorldContextObject"))
    static float EvaluateArmorMitigation(const UObject *WorldContextObject, float Armor);

    // post-mitigation damage floor: high armor can never reduce a non-zero hit below this (keeps targets killable)
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    float MinChipDamage = 1.0f;

    // status-tag damage modifiers, consumed by the damage application execution pre-mitigation
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    float RageDamageBonus = 0.25f;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    float WeakenedDamagePenalty = 0.25f;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    float TerrifiedDamageBonus = 0.25f;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    float FortifyDamageReduction = 0.25f;

    // bonus proficiency XP fraction while GAS.Buff.Enlighten is active, applied at the proficiency XP grant path
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    float EnlightenProficiencyBonus = 0.5f;

    // Bounds on the attack montage play rate once AttackSpeed is applied. The floor keeps a stacked slow from
    // stalling a montage, which never finishes and leaves the ability running.
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    float MinAttackSpeedPlayRate = 0.8f;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    float MaxAttackSpeedPlayRate = 1.4f;

    /**
     * Buildup a single landed proc contributes, before the source's StatusBuildupMultiplier. Against the ~100
     * threshold this decides how many procs an unmodified attacker needs to land a status.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline", meta = (ClampMin = "0.0"))
    float StatusBuildupPerProc = 25.0f;

    /**
     * Ceiling on dodge chance, however much an entity stacks. At 1.0 a build reaching 100% dodge is literally
     * invulnerable, so this must stay below 1 for stacked dodge to remain a trade rather than an exploit.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MaxDodgeChance = 0.75f;

    /**
     * Where on-hit chances stop being worth their face value. Below this a chance is exactly what it says; above it
     * each further point buys less than the last, approaching certainty without reaching it. Raise it to let gear
     * carry more before the curve bites; lower it to make specialising bite sooner.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ProbabilitySoftCap = 0.5f;

    // Upper bound on UMythicAttributeSet_Utility::CooldownReduction when it scales ability cooldown durations
    // (effective cooldown = base * (1 - clamp(CDR, 0, MaxCooldownReduction))). A safety cap, NOT a balance lever:
    // it keeps a sliver of cooldown so stacked CDR gear can't reach a degenerate zero/instant cooldown. 0.8 = at
    // most 80% faster. Applied in UMythicGameplayAbility::ApplyCooldown.
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MaxCooldownReduction = 0.8f;

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

    // Highest World Tier ever reached (monotonic — never decreases). Replicated for UI. Advanced via AdvanceWorldTier;
    // seeded from the starting/loaded WorldTier in BeginPlay so it is never below the current tier.
    UPROPERTY(BlueprintReadOnly, Replicated, Category = "Mythic")
    uint8 HighestWorldTier = 0;

    void SetWorldTier(uint8 NewWorldTier);

    static uint8 ComputeAdvancedWorldTier(uint8 CurrentTier, uint8 MaxTier) {
        return FMath::Min<uint8>(static_cast<uint8>(CurrentTier + 1), MaxTier);
    }

    static uint8 ComputeHighestTier(uint8 PrevHighest, uint8 NewTier) { return FMath::Max(PrevHighest, NewTier); }

    // SERVER: advance the world tier by one (clamped at MaxWorldTier), update the monotonic HighestWorldTier, and
    // reapply the tier attributes effect (so ExperienceGainMultiplier etc. refresh) via SetWorldTier.
    // NOTE: the design TRIGGER (what content raises the tier — a capstone/boss-clear reward) is NOT wired yet; this is
    // currently only reachable via the MythAdvanceWorldTier dev console command.
    UFUNCTION(BlueprintCallable, Category = "Mythic")
    void AdvanceWorldTier();

    AMythicGameState(const FObjectInitializer &ObjectInitializer = FObjectInitializer::Get());

    virtual void PostInitializeComponents() override;
    virtual void BeginPlay() override;

    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;

    // Gets the ability system component used for game wide things
    UFUNCTION(BlueprintCallable, Category = "Mythic")
    UMythicAbilitySystemComponent *GetMythicAbilitySystemComponent() const { return AbilitySystemComponent; }

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    TArray<FTrackedDestructibleData> GetTrackedDestructibles() const;
};
