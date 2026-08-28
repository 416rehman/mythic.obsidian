
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "ModularGameState.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "World/Harvesting/MythicHarvestReplicationTypes.h"
#include "MythicGameState.generated.h"

class URewardManager;
class UMythicHarvestWorldSubsystem;
class UWorldTierAttributes;
UCLASS(Config = Game, Abstract, Blueprintable, BlueprintType)
class MYTHIC_API AMythicGameState : public AGameStateBase, public IAbilitySystemInterface {
    GENERATED_BODY()

    friend class UMythicHarvestWorldSubsystem;
#if WITH_DEV_AUTOMATION_TESTS
    friend class FMythicHarvestPresentationStreamRestoreTest;
#endif

    /**
     * Always-relevant coordinator for one opaque client harvest-presentation stream.
     * It contains no destroyed-node state and is deliberately unrelated to the authority-only durable WorldEpoch.
     */
    UPROPERTY(ReplicatedUsing = OnRep_HarvestPresentationStreamToken)
    FMythicHarvestPresentationStreamToken HarvestPresentationStreamToken;

    /** Activates a received presentation stream only through the client harvest subsystem's reset/replay barrier. */
    UFUNCTION()
    void OnRep_HarvestPresentationStreamToken();

    /** Returns whether authority can publish this same or strictly newer token without stream regression. */
    bool CanSetHarvestPresentationStreamToken(
        const FMythicHarvestPresentationStreamToken &Token) const;

    /** Publishes an opaque stream token from the sole authority harvest subsystem and forces a GameState update. */
    bool SetHarvestPresentationStreamToken(
        const FMythicHarvestPresentationStreamToken &Token);

    UPROPERTY()
    bool IsSessionJoinTimeInitialized = false;

    virtual void OnRep_ReplicatedWorldTimeSecondsDouble() override;

protected:
    /** GameState-owned replicated ASC for world-wide effects and cues; null is invalid after initialization. */
    UPROPERTY(Replicated, VisibleAnywhere, Category = "Mythic")
    TObjectPtr<UMythicAbilitySystemComponent> AbilitySystemComponent;

    /** Authored world-tier initialization effect; an unset class disables tier attribute initialization. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic")
    TSubclassOf<UGameplayEffect> WorldTierAttributesInitializationEffect;

    /**
     * Runtime handle for the authority-owned world-tier initialization effect; Blueprint may inspect it read-only,
     * an invalid handle means no tier effect is active, and the value has no units.
     */
    UPROPERTY(BlueprintReadOnly)
    FActiveGameplayEffectHandle ActiveWorldTierInitEffectHandle;
public:
    /** Authored raw-armor mitigation curve used by both damage execution and UI; output units are a [0,1] fraction. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    FCurveTableRowHandle ArmorMitigationCurveRowHandle;

    /**
     * The one authored armor curve, readable by UI and summary calculations so a displayed mitigation can
     * never drift from what the damage execution applies. Returns the same clamped fraction it uses.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Combat", meta = (WorldContext = "WorldContextObject"))
    static float EvaluateArmorMitigation(const UObject *WorldContextObject, float Armor);

    /** Authored post-mitigation floor for nonzero hits; negative values are invalid and units are damage points. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    float MinChipDamage = 1.0f;

    /** Authored pre-mitigation Rage damage bonus; units are an additive fractional multiplier. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    float RageDamageBonus = 0.25f;

    /** Authored pre-mitigation Weakened damage penalty; units are an additive fractional multiplier. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    float WeakenedDamagePenalty = 0.25f;

    /** Authored pre-mitigation Terrified damage bonus; units are an additive fractional multiplier. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    float TerrifiedDamageBonus = 0.25f;

    /** Authored pre-mitigation Fortify damage reduction; units are an additive fractional multiplier. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    float FortifyDamageReduction = 0.25f;

    /** Authored Enlighten proficiency-XP bonus; units are an additive fractional multiplier. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    float EnlightenProficiencyBonus = 0.5f;

    /** Authored lower bound for attack montage play rate; positive values are unitless rate multipliers. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    float MinAttackSpeedPlayRate = 0.8f;

    /** Authored upper bound for attack montage play rate; values must not be below the minimum and are unitless. */
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

    /**
     * Safety ceiling used by cooldown-duration scaling; authored values are clamped to [0,1] and units are a
     * reduction fraction. It prevents stacked cooldown reduction from reaching a degenerate zero duration.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MaxCooldownReduction = 0.8f;

    /** Authored minimum-health-by-level curve for NPC initialization; curve outputs use health points. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    FCurveTableRowHandle HealthMinCurveRowHandle;

    /** Authored maximum-health-by-level curve for NPC initialization; curve outputs use health points. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    FCurveTableRowHandle HealthMaxCurveRowHandle;

    /** Authored minimum-damage-by-level curve for NPC initialization; curve outputs use damage points. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    FCurveTableRowHandle DamageMinCurveRowHandle;

    /** Authored maximum-damage-by-level curve for NPC initialization; curve outputs use damage points. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Baseline")
    FCurveTableRowHandle DamageMaxCurveRowHandle;

    /** Authored common-loot chance-by-level curve; curve outputs use probability fractions. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Loot")
    FCurveTableRowHandle CommonLootChanceCurveRowHandle;

    /** Authored rare-loot chance-by-level curve; curve outputs use probability fractions. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Loot")
    FCurveTableRowHandle RareLootChanceCurveRowHandle;

    /** Authored epic-loot chance-by-level curve; curve outputs use probability fractions. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Loot")
    FCurveTableRowHandle EpicLootChanceCurveRowHandle;

    /** Authored legendary-loot chance-by-level curve; curve outputs use probability fractions. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Loot")
    FCurveTableRowHandle LegendaryLootChanceCurveRowHandle;

    /** Authored mythic-loot chance-by-level curve; curve outputs use probability fractions. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic | Loot")
    FCurveTableRowHandle MythicLootChanceCurveRowHandle;

    /**
     * GameState-owned world attribute set; Blueprint may inspect its replicated/read-only values, null is invalid
     * after component initialization, and individual attributes define their own units.
     */
    UPROPERTY(BlueprintReadOnly)
    UWorldTierAttributes *WorldTierAttributes;

    /**
     * Current authority-owned world tier; Blueprint may inspect but not mutate it directly, values are clamped to
     * [1, MaxWorldTier], and units are discrete progression tiers.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic")
    uint8 WorldTier;

    /**
     * Authored upper bound for WorldTier; Blueprint may inspect it read-only, values below one are invalid content,
     * and units are discrete progression tiers.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic")
    uint8 MaxWorldTier = 4;

    /**
     * Highest authority tier ever reached, replicated for UI; Blueprint may inspect it read-only, it never decreases
     * or falls below WorldTier, and units are discrete progression tiers.
     */
    UPROPERTY(BlueprintReadOnly, Replicated, Category = "Mythic")
    uint8 HighestWorldTier = 0;

    void SetWorldTier(uint8 NewWorldTier);

    static uint8 ComputeAdvancedWorldTier(uint8 CurrentTier, uint8 MaxTier) {
        return FMath::Min<uint8>(static_cast<uint8>(CurrentTier + 1), MaxTier);
    }

    static uint8 ComputeHighestTier(uint8 PrevHighest, uint8 NewTier) { return FMath::Max(PrevHighest, NewTier); }

    /**
     * Advances the authority world tier once, clamped at MaxWorldTier, updates HighestWorldTier, and reapplies the
     * tier attribute effect. Non-authority calls are rejected; the function has no return value or units.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic")
    void AdvanceWorldTier();

    AMythicGameState(const FObjectInitializer &ObjectInitializer = FObjectInitializer::Get());

    virtual void PostInitializeComponents() override;
    virtual void BeginPlay() override;

    // Party size feeds NPC combat scaling; joins and leaves re-stamp every active NPC so a 4-player fight
    // never lingers on a lone survivor.
    virtual void AddPlayerState(APlayerState *PlayerState) override;
    virtual void RemovePlayerState(APlayerState *PlayerState) override;

    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;

    /**
     * Returns the GameState-owned ability system used for world-wide effects; Blueprint receives read-only access,
     * null is invalid after component initialization, and the result has no units.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic")
    UMythicAbilitySystemComponent *GetMythicAbilitySystemComponent() const { return AbilitySystemComponent; }

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

};
