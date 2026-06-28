#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPtr.h"
#include "MythicDeveloperSettings.generated.h"

class UMythicAbilityTagRelationshipMapping;
class UMythicDamageNumberConfig;
class UMythicLivingWorldSettings;
class UMythicLootTable;

/**
 * Settings for Mythic gameplay systems.
 * Configure these in Project Settings > Game > Mythic.
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Mythic"))
class MYTHIC_API UMythicDeveloperSettings : public UDeveloperSettings {
    GENERATED_BODY()

public:
    UMythicDeveloperSettings();

    //~UDeveloperSettings interface
    virtual FName GetCategoryName() const override { return FName("Game"); }
    //~End of UDeveloperSettings interface

    /** 
     * Default ability tag relationship mapping applied to all AbilitySystemComponents.
     * Defines how ability tags interact (block, cancel, require other abilities).
     * Can be overridden per-ASC via SetTagRelationshipMapping().
     * 
     * NOTE: This is loaded asynchronously at startup. Access via GetAbilityTagRelationshipMapping().
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Abilities")
    TSoftObjectPtr<UMythicAbilityTagRelationshipMapping> DefaultAbilityTagRelationshipMapping;

    /** 
     * Default generic ability used for items that have an Input Tag but no specific Gameplay Ability.
     * Often used for Consumables that apply effects or simple actions.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Abilities")
    TSoftClassPtr<class UMythicGameplayAbility> DefaultItemInputAbility;

    /** 
     * Returns the loaded AbilityTagRelationshipMapping, or nullptr if not yet loaded.
     * The asset is preloaded during game startup - this should always be valid during gameplay.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Abilities")
    UMythicAbilityTagRelationshipMapping *GetAbilityTagRelationshipMapping() const;

    /** Get all soft object paths that should be preloaded at startup */
    void GetStartupAssetPaths(TArray<FSoftObjectPath> &OutPaths) const;

    /**
     * Configuration for screen-space damage numbers.
     * Controls font, colors, animation, and formatting.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "UI|Damage Numbers")
    TSoftObjectPtr<UMythicDamageNumberConfig> DamageNumberConfig;

    /**
     * Living World System settings data asset.
     * Controls causal fabric, faction database, territory grid, and simulation parameters.
     * Assign in Project Settings > Game > Mythic > Living World.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Living World")
    TSoftObjectPtr<UMythicLivingWorldSettings> LivingWorldSettings;

    /** Maximum character level, derived from the summed proficiency levels. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Progression")
    int32 MaxLevel = 60;

    /** death penalty: fraction of combat proficiency XP progress a player loses on full death (after bleed-out, or a
     *  solo death). 0 = OFF (no penalty, the default). because proficiency XP is per-level progress (not cumulative),
     *  this is a within-level setback that never de-levels. genre-standard when enabled is ~0.1-0.25 */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Progression", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DeathProficiencyPenaltyFraction = 0.0f;

    /** Global loot table consulted by loot rewards when an item has no more specific table. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Loot")
    TSoftObjectPtr<UMythicLootTable> GlobalLootTable;

    /** Global multiplier applied to loot drop rates. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Loot")
    float DropRateMultiplier = 1.0f;

    /**
     * Co-op down/revive policy. When TRUE, a revivable player taking a lethal blow enters a downed state (bleeds out
     * unless a teammate revives) instead of dying outright. DEFAULT FALSE → lethal blows kill exactly as before, so
     * the downed paths stay dormant until a designer opts in and verifies them in-PIE.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Co-op")
    bool bCoopDownStateEnabled = false;

    /** Seconds a downed player has before bleeding out to a real death if not revived. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Co-op", meta = (ClampMin = "0.0"))
    float DownedBleedOutSeconds = 30.0f;

    /** Fraction of max health a revived player is restored to (0..1). */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Co-op", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ReviveHealthFraction = 0.5f;

    /** Seconds a teammate must stay near a downed ally to revive them (a hold/channel, not instant). 0 = INSTANT revive
     *  on interact (the default → byte-identical to before; co-op tension is opt-in). >0 enables the proximity channel:
     *  the reviver presses Revive then stays within ReviveChannelRange until progress completes; leaving range / going
     *  down themselves interrupts and resets it. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Co-op", meta = (ClampMin = "0.0"))
    float ReviveChannelSeconds = 0.0f;

    /** Max distance (cm) the reviver may be from the downed ally to keep a revive channel progressing. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Co-op", meta = (ClampMin = "1.0"))
    float ReviveChannelRange = 250.0f;

    /** When true (default), the reviver TAKING DAMAGE during a revive channel interrupts it (the combat tension that
     *  makes a channeled revive meaningful — "cover me!"). Only matters while ReviveChannelSeconds > 0, so the default-off
     *  channel path stays byte-identical regardless. Set false for a forgiving revive that ignores incoming damage. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Co-op")
    bool bReviveInterruptOnReviverDamage = true;

    /** Max distance (cm) between giver and recipient for a co-op item gift offer/accept. Re-validated server-side at both
     *  the offer and the accept (so neither player can drift out of range to complete an unfair hand-off). */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Co-op", meta = (ClampMin = "1.0"))
    float GiftRange = 350.0f;

    /** Seconds a pending gift offer waits for the recipient to accept before it auto-expires (clears + the item stays with
     *  the giver). 0 = no timeout (the offer waits indefinitely until accepted, declined, or superseded). */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Co-op", meta = (ClampMin = "0.0"))
    float GiftOfferTimeoutSeconds = 20.0f;

    /**
     * Friendly-fire policy. When FALSE (default), a combat hit from one player onto ANOTHER player is negated in the
     * damage-application execution (no damage/shield/status) — the standard co-op default. When TRUE, players can
     * damage each other. SELF-damage (fall damage, environmental hazards, self-DoT) is never affected — the gate fires
     * only for distinct player-vs-player hits. (A PvP-teams model would need a team check instead of "both players" —
     * deferred design; see backlog.)
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Co-op")
    bool bFriendlyFireEnabled = false;

    /**
     * Encumbrance (carry-weight) master switch. When FALSE (default), carried weight is ignored entirely — no movement
     * penalty, today's behaviour. When TRUE, total carried item weight vs the capacities below drives an encumbrance
     * tier (Unencumbered/Heavy/Overloaded) that scales move speed. Items default to Weight 0 (weightless), so even with
     * this enabled a world that hasn't authored item weights leaves every player Unencumbered — non-breaking.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Encumbrance")
    bool bEncumbranceEnabled = false;

    /** Comfortable carry limit: above this (and at/below the hard cap) the player is Heavy. <=0 disables this tier. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Encumbrance", meta = (ClampMin = "0.0"))
    float EncumbranceSoftCapacity = 100.0f;

    /** Absolute carry limit: above this the player is Overloaded. <=0 disables this tier. Should be >= the soft cap. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Encumbrance", meta = (ClampMin = "0.0"))
    float EncumbranceHardCapacity = 150.0f;

    /** Move-speed multiplier while Heavy (0..1). */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Encumbrance", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float EncumbranceHeavySpeedMultiplier = 0.7f;

    /** Move-speed multiplier while Overloaded (0..1) — low so over-capacity is a real cost (a near-stagger trudge). */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Encumbrance", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float EncumbranceOverloadedSpeedMultiplier = 0.3f;

    /**
     * Stamina-gated sprint. When FALSE (default), sprinting (GAS.State.Sprinting) is free — today's behaviour. When TRUE,
     * sprinting drains CurrentStamina while the entity is actually moving; at 0 stamina the sprint speed bonus is
     * suppressed (GAS.State.Exhausted) until stamina recovers to SprintRecoverStaminaFraction of max. Server-authoritative,
     * folded into the existing stamina-regen tick + the move-speed recompute. Gameplay-affecting → off by default.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Stamina")
    bool bStaminaGatedSprint = false;

    /** Stamina drained per second while actively sprinting (only applied when bStaminaGatedSprint is true). */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = "0.0"))
    float SprintStaminaDrainPerSecond = 15.0f;

    /** Fraction of max stamina an exhausted entity must recover before it can sprint again — hysteresis so the sprint
     *  speed bonus doesn't stutter on/off at 0 stamina. A small positive ClampMin enforces a non-zero recovery band
     *  (a 0 fraction would re-allow sprinting the instant stamina leaves 0, flickering the bonus every regen tick). */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Stamina", meta = (ClampMin = "0.05", ClampMax = "1.0"))
    float SprintRecoverStaminaFraction = 0.2f;

    /**
     * Aggro/threat targeting. When FALSE (default), an NPC targets the geometrically CLOSEST perceived hostile (today's
     * behaviour). When TRUE, each NPC accrues per-attacker threat (damage dealt to it) and targets the HIGHEST-threat
     * perceived hostile instead — letting a tank hold aggro off the squishy players. Falls back to closest when no
     * perceived candidate has accrued threat, so it degrades gracefully.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Combat")
    bool bThreatTargetingEnabled = false;

    /** Threat accrued per point of damage dealt to an NPC (the damage→threat multiplier). */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.0"))
    float ThreatPerDamage = 1.0f;

    /**
     * Status-buildup decay rate (Burn/Bleed/Poison/Slow/Freeze/Stun buildup points shed per second). 0 (default) = no
     * decay — sub-threshold buildup persists indefinitely (today's behaviour). A positive value gives the Souls-like
     * falloff: scattered hits no longer accumulate status across unrelated encounters; you must SUSTAIN hits to cross
     * the threshold. Applied server-side in the existing regen tick to every afflicted entity. Recommend ~5-15 to enable.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.0"))
    float StatusBuildupDecayPerSecond = 0.0f;
};
