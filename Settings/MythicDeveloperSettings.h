#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPtr.h"
#include "World/LivingWorld/Bounty/MythicBountyRules.h"
#include "World/LivingWorld/Acquaintance/MythicMourningRules.h"
#include "World/Camping/MythicCampsiteCore.h"
#include "World/Gathering/MythicYieldQuality.h"
#include "World/Farming/MythicFarmingRules.h"
#include "World/Farming/MythicLivestockGenome.h"
#include "World/LivingWorld/Pressure/MythicRegionalPressureRules.h"
#include "World/Gathering/MythicHarvestPressureRules.h"
#include "World/Fishing/MythicFishingMinigameRules.h"
#include "World/Fishing/MythicFishStockRules.h"
#include "World/Hunting/MythicSkinningRules.h"
#include "World/Hunting/MythicSpoorRules.h"
#include "World/LivingWorld/EmergentQuests/MythicApexHuntRules.h"
#include "World/Trading/MythicTradingConfig.h"
#include "GAS/Effects/MythicEnemyScaling.h"
#include "MythicDeveloperSettings.generated.h"

class UMythicAbilityTagRelationshipMapping;
class UMythicDamageNumberConfig;
class UMythicLivingWorldSettings;
class UMythicLootTable;
class AMythicWorldItem;
class UMythicAchievementSet;
class UMythicUnlockRuleSet;
class UMythicCodexLibrary;
class UMythicStatusEffectLibrary;
class UMythicRenownTierTable;
class UMythicTitleRegistry;
class UGameplayEffect;
class AMythicMount;
class UProficiencyDefinition;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Mythic"))
class MYTHIC_API UMythicDeveloperSettings : public UDeveloperSettings {
    GENERATED_BODY()

public:
    UMythicDeveloperSettings();

    virtual FName GetCategoryName() const override { return FName("Game"); }

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

    /**
     * Default achievement set (the authored UMythicAchievementDefinition collection) the per-player
     * UMythicAchievementComponent resolves when it has no component-level override..
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Progression")
    TSoftObjectPtr<UMythicAchievementSet> DefaultAchievementSet;

    /**
     * Default tag-unlock rule set the per-player UMythicUnlockComponent resolves when it has no component-level override.
     * Preloaded at startup — mirrors DefaultAchievementSet.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Progression")
    TSoftObjectPtr<UMythicUnlockRuleSet> DefaultUnlockRuleSet;

    /**
     * The codex CONTENT library (authored UMythicBestiaryEntry + UMythicGlossaryEntry collections) UMythicCodexRegistry
     * indexes for bestiary/glossary UI lookups. Preloaded at startup — mirrors DefaultAchievementSet. Unset = codex
     * progress still records/replicates/persists; content lookups just resolve empty ("???" pages).
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Knowledge")
    TSoftObjectPtr<UMythicCodexLibrary> DefaultCodexLibrary;

    /**
     * Every status effect the game can apply (Burn, Bleed, Stun, ...). UMythicStatusRegistry indexes it by
     * Status.Type tag. A status must appear here to be reachable by buildup, abilities or cheats — unset means no
     * status effect can land.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Combat")
    TSoftObjectPtr<UMythicStatusEffectLibrary> StatusEffectLibrary;

    /** Maximum character level, derived from the summed proficiency levels. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Progression")
    int32 MaxLevel = 60;

    /**
     * Default renown tier table (7 ascending value boundaries -> 8 tiers, vendor discounts, per-tier payloads) the
     * per-player UMythicRenownComponent resolves when it has no component-level override. Unset = built-in code-default
     * curve (renown still fully works; only the authored per-tier payloads need an asset). Preloaded at startup.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Progression|Renown")
    TSoftObjectPtr<UMythicRenownTierTable> DefaultRenownTierTable;

    /**
     * Default title DISPLAY registry (Title.* tag -> nameplate text) UMythicTitleRegistry::GetActiveTitleText resolves.
     * Unset = active titles render their tag leaf as a readable fallback. Preloaded at startup (nameplates read it via
     * the non-loading Get()).
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Progression|Titles")
    TSoftObjectPtr<UMythicTitleRegistry> DefaultTitleRegistry;

    /** death penalty: fraction of combat proficiency XP progress a player loses on full death (after bleed-out, or a
     *  solo death). 0 = OFF (no penalty, the default). because proficiency XP is per-level progress (not cumulative),
     *  this is a within-level setback that never de-levels. genre-standard when enabled is ~0.1-0.25 */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Progression", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DeathProficiencyPenaltyFraction = 0.0f;

    /**
     * Per-tier enemy scaling, and the item level bonus a kill of that tier adds to the world's base.
     *
     * These five rows were a switch of magic numbers in C++, which meant no designer could retune the
     * tier ladder and nothing could grant a tier a better drop. Seeded with the values that switch used,
     * so leaving this untouched behaves exactly as before.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Combat")
    TArray<FMythicEnemyTierScaling> EnemyTierScaling;


    /** Global loot table consulted by loot rewards when an item has no more specific table. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Loot")
    TSoftObjectPtr<UMythicLootTable> GlobalLootTable;

    /**
     * The AMythicWorldItem subclass every dropped/overflowing item spawns as.
     *
     * UMythicLootManagerSubsystem::DefaultWorldItemClass had NO way to be set: its only setter is plain C++ with zero
     * callers and no UFUNCTION, and a GameInstanceSubsystem CDO is not editor-editable. So it was always null, and the
     * null-guards in CreateAndGive / Spawn made EVERY loot grant a silent early-return. This config field is what the
     * subsystem resolves on Initialize, so the class is a project setting like the global loot table beside it.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Loot")
    TSoftClassPtr<AMythicWorldItem> DefaultWorldItemClass;

    /** Global multiplier applied to loot drop rates. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Loot")
    float DropRateMultiplier = 1.0f;

    /** Base currency cost to reroll an item's affixes (the gear-optimization gold sink; scaled up by item level +
     *  rarity — see MythicCurrency::ComputeRerollCost). Non-zero default so the sink is ACTIVE out of the box (reroll
     *  is no longer free/unlimited); set to 0 to restore free reroll. Tune to your currency scale. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0"))
    int32 RerollBaseCost = 50;

    /** Per-item-level growth of the reroll cost (0.10 = +10% per level). Only matters when RerollBaseCost > 0. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0.0"))
    float RerollCostPerLevelFraction = 0.10f;

    /** Per-rarity-tier growth of the reroll cost (0.75 = +75% per tier: Common→Rare→Epic…). Only matters when
     *  RerollBaseCost > 0. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0.0"))
    float RerollCostPerRarityFraction = 0.75f;

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

    /**
     * Item types that count as GATHERED when acquired rather than looted, matched against the item's type tag and
     * its parents. Everything else acquired counts as looted. Authored rather than fixed, because which families
     * are "taken from the world" is a content decision — adding a Fishing type should not need a code change.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Itemization", meta = (Categories = "Itemization.Type"))
    FGameplayTagContainer GatheredItemTypes;


    // ─────────────────────────────────── Survival Needs ───────────────────────────────────
    /**
     * Survival-needs MASTER switch. When FALSE (default), the whole per-player survival tick never starts — Nourishment/
     * Hydration/Warmth/Wetness sit at their full/neutral defaults and nothing decays = today's behaviour. When TRUE, each
     * PlayerState runs ONE ~SurvivalTickInterval timer (server-authoritative, no Tick) that decays food/water, nets
     * warmth/wetness off the EXISTING weather + campfire/shelter tags, and applies the threshold GEs below. Survival-LITE:
     * the default rates are deliberately gentle (not punishing).
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Survival")
    bool bSurvivalNeedsEnabled = false;

    /** Seconds between survival ticks (the single repeating timer cadence). Decay/warmth latency, not a frame budget. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Survival", meta = (ClampMin = "0.05"))
    float SurvivalTickInterval = 2.0f;

    /** Nourishment lost per second (full→empty at 0.05 ≈ 33 min from 100). Low = survival-lite. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Survival", meta = (ClampMin = "0.0"))
    float NourishmentDecayPerSecond = 0.05f;

    /** Hydration lost per second (slightly faster than food — thirst outpaces hunger). */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Survival", meta = (ClampMin = "0.0"))
    float HydrationDecayPerSecond = 0.07f;

    /** Warmth gained per second near a warm source (campfire aura grants Status.Warm). */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Survival", meta = (ClampMin = "0.0"))
    float WarmSourceWarmthPerSecond = 8.0f;

    /** Warmth lost per second in cold weather (Snow) when neither warm nor sheltered. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Survival", meta = (ClampMin = "0.0"))
    float ColdWeatherWarmthPerSecond = 3.0f;

    /** EXTRA warmth lost per second in cold weather, scaled by current wetness fraction (being wet makes cold worse). */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Survival", meta = (ClampMin = "0.0"))
    float WetChillWarmthPerSecond = 2.0f;

    /** Warmth drift-per-second back toward neutral when neither warmed nor chilled. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Survival", meta = (ClampMin = "0.0"))
    float PassiveWarmthRegenPerSecond = 2.0f;

    /** The neutral-warmth target as a fraction of MaxWarmth (passive drift goal). */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Survival", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float NeutralWarmthFraction = 0.5f;

    /** Wetness gained per second standing in rain/snow (unless sheltered or beside a warm source). */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Survival", meta = (ClampMin = "0.0"))
    float WettingPerSecond = 6.0f;

    /** Wetness lost per second otherwise (sheltered / warm / clear weather). */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Survival", meta = (ClampMin = "0.0"))
    float DryingPerSecond = 4.0f;

    /** Nourishment fraction below which the player is Starving (GE applied). */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Survival", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float StarvingThreshold = 0.15f;

    /** Nourishment fraction above which the player is Well Fed (buff GE applied). */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Survival", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WellFedThreshold = 0.85f;

    /** Hydration fraction below which the player is Dehydrated (GE applied). */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Survival", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DehydratedThreshold = 0.15f;

    /** Warmth fraction below which the player is Cold (GE applied). */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Survival", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ColdThreshold = 0.20f;

    /** Hysteresis band width (fraction) added to each threshold's EXIT edge so a status doesn't flicker at the boundary. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Survival", meta = (ClampMin = "0.0", ClampMax = "0.5"))
    float SurvivalHysteresisBand = 0.05f;

    /** How much a fully-soaked player raises the effective Cold threshold (gets Cold sooner). */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Survival", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WetColdAggravation = 0.10f;

    /** Designer-authored GE applied while Starving. Unset = feedback-only (onset/relief callout, no numeric effect). */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Survival|Effects")
    TSoftClassPtr<UGameplayEffect> SurvivalStarvingEffect;

    /** Designer-authored buff GE applied while Well Fed. Unset = feedback-only. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Survival|Effects")
    TSoftClassPtr<UGameplayEffect> SurvivalWellFedEffect;

    /** Designer-authored GE applied while Dehydrated. Unset = feedback-only. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Survival|Effects")
    TSoftClassPtr<UGameplayEffect> SurvivalDehydratedEffect;

    /** Designer-authored GE applied while Cold. Unset = feedback-only. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Survival|Effects")
    TSoftClassPtr<UGameplayEffect> SurvivalColdEffect;

    // ─────────────────────────────────── Region / Danger Tracker ───────────────────────────────────
    /**
     * Per-player REGION/DANGER TRACKER master switch. When TRUE (default), each PlayerState runs ONE 1s server-only
     * timer (no Tick) that samples the pawn's cell → living-world danger tier + region name and replicates them
     * COND_OwnerOnly for a durable HUD readout (plus an INERT "entering danger" cue on a tier INCREASE). This is pure
     * feedback state — no gameplay effect, no cost until a HUD binds. When FALSE, the timer never arms = zero runtime
     * cost (byte-identical to before). Mirrors bSurvivalNeedsEnabled, but defaults ON (feedback-only, harmless).
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Region Tracker")
    bool bRegionDangerTrackerEnabled = true;

    // ─────────────────────────────────── Camaraderie Synergy (co-op proximity buff) ───────────────────────────────────
    /**
     * CAMARADERIE SYNERGY master switch. When TRUE (default), each PlayerState runs ONE 1s server-only timer (no Tick)
     * that counts allies within CamaraderieRadius and maintains a scaling Buff.Camaraderie GE (magnitude = stacks ×
     * PerAllyBonus) on this player's ASC — the "group up vs spread out" tension. This is ALREADY inert by default because
     * PerAllyBonus ships at 0 (EffectiveBonus is 0 for any stack count ⇒ the GE is never applied ⇒ byte-identical); the
     * switch exists so a designer can globally disable the whole timer (an A/B, a timed event) WITHOUT zeroing the bonus.
     * When FALSE, the timer never arms = zero runtime cost. Defaults ON (harmless until PerAllyBonus is raised). Mirrors
     * bRegionDangerTrackerEnabled / bSecretsEnabled.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Camaraderie")
    bool bCamaraderieEnabled = true;

    /** Max distance (cm) an ally's pawn may be from this player to count toward camaraderie (squared for the compare). */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Camaraderie", meta = (ClampMin = "0.0"))
    float CamaraderieRadius = 1500.0f;

    /** Max ally-count the buff scales to (the cap — e.g. 3 for the other members of a 4-player party). 0 = no stacks. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Camaraderie", meta = (ClampMin = "0"))
    int32 MaxAllyStacks = 3;

    /** Buff magnitude granted PER nearby ally (fed as SetByCaller "Camaraderie.Bonus" = stacks × this). 0 (DEFAULT) ⇒
     *  the buff is zero-magnitude and never applied ⇒ INERT / byte-identical. Raise it to opt the synergy in (the native
     *  fallback GE reads it as a flat additive Power bonus; an authored CamaraderieEffect can read it however it likes). */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Camaraderie", meta = (ClampMin = "0.0"))
    float PerAllyBonus = 0.0f;

    /** Designer-authored camaraderie buff GE (the CONTENT that decides WHICH attributes the synergy buffs — +damage /
     *  +regen / …). It reads the SetByCaller "Camaraderie.Bonus" magnitude the component stamps. Unset (DEFAULT) ⇒ the
     *  native UMythicGE_Camaraderie fallback (a SetByCaller-driven additive Power bonus) is used instead. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Camaraderie")
    TSoftClassPtr<UGameplayEffect> CamaraderieEffect;

    // ─────────────────────────────────── Reputation-Driven Encounters ───────────────────────────────────
    /**
     * REPUTATION-DRIVEN AMBIENT ENCOUNTERS master switch. When TRUE (default), the EncounterDirector folds ONE
     * multiplicative reputation term into any encounter template that OPTS IN by setting a RequiredReputationBand
     * (Reputation.Band.Feared / Renowned) — so a feared/infamous party draws more bounty-seekers + glory-challengers and
     * a renowned party draws more rivals/help-seekers, and a template banded to the "wrong" reputation is gated out.
     * When FALSE, the term is SKIPPED entirely: every template spawns at its authored BaseProbability/EntityCount exactly
     * as it would with no reputation system — byte-identical. Defaults ON because the feature is ALREADY inert unless a
     * template opts in (an unstamped RequiredReputationBand is ignored regardless of this switch), so this exists to let
     * a designer globally disable ALL reputation gating (an A/B, a timed event) WITHOUT editing every template. Mirrors
     * bSecretsEnabled / bProcAffixesEnabled.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Reputation Encounters")
    bool bReputationEncountersEnabled = true;

    // ─────────────────────────────────── Bounty Hunters ───────────────────────────────────
    /**
     * Notoriety→bounty-hunters MASTER switch (J2). When FALSE (default), UMythicBountySubsystem is never created — no
     * timer, no standing polls, no spawns; the world is byte-identical to before. When TRUE, high notoriety (a player's
     * worst faction standing, fed by the crime pipeline / kills / trespass) telegraphs and then dispatches roaming
     * hunter packs near the player (never during the pacing director's Rest phase). Requires BountyHunters.HunterNPCType
     * to be authored (CONTENT) before any hunt actually fires.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Bounty Hunters")
    bool bEnableBountyHunters = false;

    /** Bounty tuning: tier thresholds, cooldown, pack sizes, chance, telegraph delay, hunter NPC type, spawn ring.
     *  Defaults are deliberately high-threshold/low-chance (near-inert even when the master switch is on). */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Bounty Hunters", meta = (EditCondition = "bEnableBountyHunters"))
    FMythicBountyConfig BountyHunters;

    // ─────────────────────────────────── Avengers ───────────────────────────────────
    /**
     * Mourning→avenger MASTER switch (J3). When FALSE (default), UMythicAvengerSubsystem is never created — no timer,
     * no state; the world is byte-identical to before. When TRUE, a NOTABLE NPC killed by a player (the cemetery's
     * notability gate) has a LOW, notoriety-scaled chance to raise an avenger: a telegraphed "kin swear vengeance"
     * beat (objective + chronicle), then one faction-mate spawned near the killer who hunts them. Requires
     * Avengers.AvengerNPCType to be authored (CONTENT) before any vengeance actually fires.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Avengers")
    bool bEnableAvengers = false;

    /** Avenger tuning: chance curve, cooldown, telegraph delay, avenger NPC type, spawn ring. Defaults are
     *  deliberately low-chance (near-inert even when the master switch is on). */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Avengers", meta = (EditCondition = "bEnableAvengers"))
    FMythicAvengerConfig Avengers;

    // ─────────────────────────────────── Camping ───────────────────────────────────
    /**
     * CAMP EVENTS master switch (Wave N, Raid Gates d). When FALSE (default), the campsite subsystem's event timer is
     * never armed — no rolls, no telegraphs, no spawns; camps/comfort/rest still work fully (they are core loop, not
     * events). When TRUE, a camp with players nearby can draw a telegraphed NIGHT AMBUSH (chance from the cell's
     * danger tier — Safe cells never roll) and an occasional FRIENDLY traveling merchant. Requires
     * Camping.Events.AmbushNPCType / MerchantNPCType to be authored (CONTENT) before anything actually spawns.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Camping")
    bool bEnableCampEvents = false;

    /** Camping tuning: camp cluster radius, per-player piece cap (oldest-collapses), the comfort→Rested payoff ladder,
     *  and camp-event knobs (only read while bEnableCampEvents is TRUE). Defaults are gentle/near-inert. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Camping")
    FMythicCampingConfig Camping;

    // ─────────────────────────────────── Yield Quality (P1, Wave M) ───────────────────────────────────
    /** THE shared yield-quality rule block (P1): tier→potency/price multipliers, injected-roll chances, mastery
     *  floors. Consumed by cooking potency (Wave M) and, as they land, farming produce (L), pelts/meat/fish (P),
     *  pricing (O) and trophy grading (K). Defaults are Common-neutral (1.0× everywhere) and floors are disabled —
     *  an untuned world behaves byte-identically to today. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Yield Quality")
    FMythicYieldQualityRules YieldQuality;

    // ─────────────────────────────────── Farming (Wave L) ───────────────────────────────────
    /** FARMING DEPTH tuning (moisture decay, dry-growth slowdown, wither, quality bonus chances, Gravebloom radius,
     *  scarecrow habituation). GENTLE DEFAULTS: growth speed stays 1.0 regardless of moisture and crops never wither
     *  until authored otherwise — an unauthored world farms byte-identically to before. Quality tiers roll but only
     *  MANIFEST when a crop def authors per-tier reward blocks. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Farming")
    FMythicFarmingConfig Farming;

    // ─────────────────────────────────── Livestock Breeding (genetics) ───────────────────────────────────
    /**
     * LIVESTOCK BREEDING master switch (mirrors bSecretsEnabled — defaults ON because the feature is ALREADY inert
     * unless bred into). When TRUE, an animal pen's ServerTryBreed can pair two fed same-species parents into an
     * offspring with a heritable (blended + mutated) genome, and the produce calc folds each animal's genome into its
     * yield RATE (Yield gene) and produce TIER (Quality gene). Every animal starts with a NEUTRAL (all-zero) genome that
     * adds 0 tiers and 1.0x rate, so an un-bred world produces byte-identically to today; only a deliberate breed ever
     * introduces a non-neutral genome. When FALSE, ServerTryBreed no-ops AND the produce fold ignores the genome
     * entirely -> baseline behaviour regardless of any bred stock. Gameplay-additive + opt-in, hence safe to default ON.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Livestock Breeding")
    bool bLivestockBreedingEnabled = true;

    /** Breeding tuning: per-breed mutation magnitude + trait clamp range, Quality->tier-step + cap, Yield->rate bonus.
     *  Gentle defaults keep single generations incremental (selection over many generations is the payoff) and the
     *  neutral genome exactly inert. Only read while bLivestockBreedingEnabled is TRUE. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Livestock Breeding")
    FMythicBreedingParams Breeding;

    // ─────────────────────────────────── Regional Pressure (P5, Wave L) ───────────────────────────────────
    /**
     * FARM RAIDS master switch (P6 Raid Gates d). When FALSE (default), no raid ever rolls — pressure still accrues
     * (a few float ops per minute, only while mature plots exist) so QueryPressure stays truthful for Wave P, but the
     * world never spawns a raider. When TRUE, a farm cell whose Pressure.Farm crosses the (scarecrow-raised) threshold
     * draws a TELEGRAPHED creature raid — online/proximity-gated, never during the pacing Rest phase, and crop damage
     * is stage regression ONLY (never seed loss). Requires RegionalPressure.RaidNPCType (CONTENT) before anything spawns.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Regional Pressure")
    bool bEnableFarmRaids = false;

    /** P5 regional-pressure tuning: check cadence, decay, farm-channel emission/threshold/raid knobs. The
     *  accumulate/decay half serves every future channel (hunt/fish in Wave P); the raid half is farm content. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Regional Pressure")
    FMythicRegionalPressureConfig RegionalPressure;

    // ─────────────────────────────────── Harvest-Pressure Ecology (commons depletion / fallow regrowth) ──────────────
    /**
     * HARVEST-PRESSURE ECOLOGY master switch (READ-side gate). When TRUE (default), the pressure subsystem's live
     * harvest read helpers fold Pressure.Harvest into the yield multiplier / produced quality tier / respawn
     * delay+gate — so hammering one grove/vein drops its yield and lengthens (or gates) regrowth, while a fallow cell
     * recovers via the shared channel decay. When FALSE, every read helper returns BASELINE (byte-identical to today).
     * Mirrors bSecretsEnabled: defaults ON because the mechanic is ALREADY inert unless a designer raises the depletion
     * WEIGHTS below (all default 0 → pressure accrues but changes nothing) — this switch lets a designer globally
     * disable the READ side (an A/B, a timed event) without zeroing every weight.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Harvest Pressure")
    bool bHarvestPressureEnabled = true;

    /** Harvest-pressure tuning: the PUSH amount per completed gather, the yield/quality/respawn depletion WEIGHTS (all
     *  default 0 = inert), the respawn gate threshold, and the fallow recovery curve. An untuned world gathers
     *  byte-identically to before; a designer raises the weights to opt the commons-depletion ecology in. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Harvest Pressure")
    FMythicHarvestPressureConfig HarvestPressure;

    // ─────────────────────────────────── Fishing (Wave P) ───────────────────────────────────
    /**
     * FISHING MINIGAME master switch (P1i). When FALSE (default), every cast is the classic quiet channel — byte-
     * identical to today (the beats never arm; Hook/Pull intents are inert). When TRUE, casts play WAIT → bite-window →
     * FIGHT surge beats on the field-activity substrate's existing 0.1s re-validate timer, with cue-only feedback
     * (GameplayCue.Fishing.*) and the trash-at-mastery auto-resolve valve.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Fishing")
    bool bFishingMinigameEnabled = false;

    /** Minigame tuning (only read while bFishingMinigameEnabled): wait/bite/fight windows, surge cadence, pulls to
     *  land, the auto-resolve mastery valve. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Fishing")
    FMythicFishingMinigameConfig FishingMinigame;

    /**
     * FISH STOCKS master switch (P3i). When FALSE (default), every spot has bottomless stock — byte-identical to
     * today (no state, no pressure). When TRUE, spots hold clock-regenerating stock units; real catches draw them
     * down (+ Pressure.Fish), and an EXHAUSTED spot degrades its catch table to trash-only until it refills — teeth
     * are prices/degraded tables via the pressure channel, never hard locks.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Fishing")
    bool bFishStocksEnabled = false;

    /** Stock tuning (only read while bFishStocksEnabled): default max stock, regen cadence, Pressure.Fish amounts. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Fishing")
    FMythicFishStockConfig FishStocks;

    // ─────────────────────────────────── Hunting (Wave P) ───────────────────────────────────
    /** PELT/MEAT QUALITY tuning (P4i): what makes a kill BOTCHED (Ragged) vs CLEAN (Fine base). Always-on math with an
     *  inert manifestation — the tier only changes what is minted once a skin-yield entry authors per-tier item defs. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Hunting")
    FMythicPeltQualityConfig PeltQuality;

    /** SPOOR TRAIL tuning (P5i): node lifetime (rain washes faster), staleness, stride, per-region anti-litter cap.
     *  Nodes only spawn through the (master-gated, default-OFF) apex-hunt subsystem. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Hunting")
    FMythicSpoorConfig Spoor;

    // ─────────────────────────────────── Apex Hunts (Wave P) ───────────────────────────────────
    /**
     * APEX HUNTS master switch (P5i/P6i; P6 Raid-Gates posture — default OFF ⇒ the subsystem never arms its timer:
     * no offers, no spawns, no trails; byte-identical world). When TRUE, a player who masters a species' bestiary
     * page to FULL tier can be offered a tier-boosted apex hunt — gated on a HEALTHY local population
     * (Pressure.Hunt.<Species> below the threshold: over-hunting eats your own apex content). Requires authored
     * ApexHunts.Species rows (CONTENT) before anything actually offers.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Apex Hunts")
    bool bEnableApexHunts = false;

    /** Apex-hunt tuning + species rows. HuntPressurePerKill inside is ALWAYS read (the P6i over-hunting feed at the
     *  kill site) — the rest only while bEnableApexHunts is TRUE. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Apex Hunts")
    FMythicApexHuntConfig ApexHunts;

    // ─────────────────────────────────── Homestead (Wave K) ───────────────────────────────────
    /**
     * HOMESTEAD RAIDS master switch (K5; P6 Raid Gates d — default OFF ⇒ vendetta RetaliationRaids never target the
     * homestead: no telegraphs, no spawns, no structure damage; byte-identical world). When TRUE, a faction whose
     * grudge crosses RaidAt while the hunted player's party owns a claimed homestead — AND a member is online + near —
     * mounts a TELEGRAPHED raid at the totem: chronicle beat, warning delay, pacing-Rest re-check at fire time, then
     * a spawned pack and a Damaged (repairable, NEVER deleted/downgraded) shell. Requires Homestead.Raids.RaidNPCType
     * (CONTENT) before anything actually spawns.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Homestead")
    bool bEnableHomesteadRaids = false;

    /**
     * The CONSTRUCTION proficiency track (a DATA asset on the generic proficiency system) fed by homestead building
     * (shell large / repair medium / deploy small — every feed anti-grind capped). The ConstructionProficiency ASC
     * attribute has existed as a ghost track; this asset finally connects it. Unset = building grants no proficiency
     * XP (the transactions still work; warned once). Mirrors Mounts.RidingProficiency.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Homestead")
    TSoftObjectPtr<UProficiencyDefinition> ConstructionProficiency;

    // ─────────────────────────────────── Trading (Wave O) ───────────────────────────────────
    /**
     * TRADING master switch (Wave O). When FALSE (default), the trade-ledger subsystem is never created (no commit
     * sampling, no rumor/deficit beats, no contract board), the emergent-quest pool adds NO delivery rows, and a
     * deployed player stall behaves as a plain storage chest (its drain timer never arms) — the world is
     * byte-identical to before. When TRUE, the ledger rides the existing world-sim commit signal, famine/deficit
     * beats post delivery-contract offers, vendors flagged bAcceptsDeliveries redeem them (paying the scarcity
     * price and injecting Reserves through the P9 queue), and stalls sell through against the fair scarcity price.
     *
     * NOT gated by this switch: the P9 EnqueuePlayerResourceDelta drain and its per-tick clamp
     * (Trading.MaxReserveInjectionPerAxisPerTick) — shared infrastructure, always clamped (R6).
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Trading")
    bool bEnableTrading = false;

    /** Trading tuning: the P9 injection rail (always read), ledger staleness/rumor knobs, deficit→contract
     *  thresholds, stall sell-through/away-accrual knobs, cargo-heat + contraband knobs. Gentle defaults. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Trading")
    FMythicTradingConfig Trading;

    // ─────────────────────────────────── Secrets / Easter Eggs ───────────────────────────────────
    /**
     * SECRETS master gate. When TRUE (default), placed secret triggers (AMythicSecretVolume / AMythicSecretInteractable)
     * can reveal their authored payoff. The system is ALREADY inert-by-default (an empty world has no secrets and does
     * nothing), so this is not required to keep the world quiet — it exists so a designer can globally disable ALL
     * secrets (e.g. during a timed event, or to A/B a build) WITHOUT deleting the placed actors. When FALSE, every
     * trigger's reveal is skipped = the placed secrets go dormant. Defaults ON because a placed secret is already opt-in.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Secrets")
    bool bSecretsEnabled = true;

    // ─────────────────────────────────── Ownership / Theft Crime ───────────────────────────────────
    /**
     * OWNERSHIP-THEFT master gate. When TRUE (default), taking from / picking an OWNED object (a container / stall /
     * locked door stamped with a UMythicOwnershipComponent) submits a theft crime into the EXISTING witness→moral→
     * crime-record→notoriety→guard-dispatch pipeline. The system is ALREADY inert-by-default (an unstamped object is
     * unowned → zero theft events → byte-identical), so this is not required to keep the world quiet — it exists so a
     * designer can globally disable ALL ownership crime (a timed event, an A/B) WITHOUT unstamping every actor. When
     * FALSE, MythicTheftCrime::TrySubmitTheft no-ops → fully inert. Mirrors bSecretsEnabled; defaults ON (a stamp is
     * already opt-in, authored per actor).
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Ownership")
    bool bOwnershipCrimeEnabled = true;


    // ─────────────────────────────────── Item Corruption (Vaal-style gamble) ───────────────────────────────────
    /**
     * ITEM CORRUPTION master switch (mirrors bSecretsEnabled). When TRUE (default), the Corrupt craft op ("Vaal") rolls a
     * weighted OUTCOME from ItemCorruptionOutcomes and applies it (reroll / add-socket / upgrade-affix-tier / …) BEFORE
     * permanently sealing the item — a high-risk/high-reward gamble. The system is ALREADY inert-by-default: an EMPTY
     * ItemCorruptionOutcomes table makes the roll return Seal, so corruption reduces to the legacy "just seal" behavior —
     * byte-identical. This switch is NOT required to keep the world quiet; it exists so a designer can globally disable
     * the outcome roll (an A/B, a timed event) WITHOUT clearing the table. When FALSE, Corrupt does ONLY the legacy seal.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Item Corruption")
    bool bItemCorruptionEnabled = true;


    // ─────────────────────────────────── Mounts ───────────────────────────────────
    /**
     * Mount actor class UMythicMountRosterComponent spawns on whistle-summon when the component has no
     * MountClassOverride. Point this at the mesh-bearing mount BP (BP_MythicMount). Unset = summon refuses (logged) —
     * the roster/taming records still work; only the live spawn needs a class. Preloaded at startup.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Mounts")
    TSoftClassPtr<AMythicMount> DefaultMountClass;

    /** Seconds between whistle summons (per player; boundary inclusive). <= 0 disables the cooldown. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Mounts", meta = (ClampMin = "0.0"))
    float MountSummonCooldown = 30.0f;

    /**
     * The RIDING proficiency track (a DATA asset on the generic proficiency system) granted XP by taming (and future
     * riding payoffs) when UMythicGA_Tame has no CDO-level override. Unset = taming grants no proficiency XP (the
     * roster mint still happens). Preloaded at startup.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Mounts")
    TSoftObjectPtr<UProficiencyDefinition> RidingProficiency;

    // ─────────────────────────────── Batched world FX ───────────────────────────────
    /**
     * Niagara Data Channel that UMythicFXChannelSubsystem writes every queued world effect into, so ONE global Niagara
     * system can draw them all as particles instead of spawning one system instance per hit, per status onset and per
     * dropped item. UNSET (default) = batching is off and every caller keeps its existing per-effect cue behaviour, so
     * the game is byte-identical until the channel and its reader system are authored.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "FX")
    TSoftObjectPtr<class UNiagaraDataChannelAsset> WorldFXDataChannel;

    /** Effects further than this (cm) from every LOCAL viewer are dropped before reaching Niagara. 0 = no cull. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "FX", meta = (ClampMin = "0.0"))
    float WorldFXCullDistance = 8000.0f;

    /** Ceiling on batched effects written per frame. A pathological frame drops the excess instead of stalling. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "FX", meta = (ClampMin = "1"))
    int32 WorldFXMaxEventsPerFrame = 256;
};
