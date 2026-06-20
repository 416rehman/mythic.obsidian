// Mythic Living World System — Faction Database
// Cached faction data snapshot. Background thread computes, game thread reads lock-free.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include <atomic>
#include "FactionDatabase.generated.h"

// ─────────────────────────────────────────────────────────────
// Faction Relationship — Pairwise faction stance
// ─────────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class EMythicFactionRelation : uint8 {
    Allied UMETA(DisplayName = "Allied"),
    Friendly UMETA(DisplayName = "Friendly"),
    Neutral UMETA(DisplayName = "Neutral"),
    Unfriendly UMETA(DisplayName = "Unfriendly"),
    Hostile UMETA(DisplayName = "Hostile")
};

// ─────────────────────────────────────────────────────────────
// Faction Status — Lifecycle state (REQ-FAC-003)
// ─────────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class EMythicFactionStatus : uint8 {
    /** Faction is active and participating in the simulation */
    Active UMETA(DisplayName = "Active"),

    /** Faction has been completely destroyed (no population, no territory) */
    Annihilated UMETA(DisplayName = "Annihilated"),

    /** Faction was destroyed but retained enough population to form a resistance */
    Resistance UMETA(DisplayName = "Resistance"),

    /** Faction exists in data but has zero presence (e.g. an ancient empire waiting for an event) */
    Dormant UMETA(DisplayName = "Dormant")
};

// ─────────────────────────────────────────────────────────────
// Ideology Profile — Per-axis moral stance (designer-facing)
// ─────────────────────────────────────────────────────────────

/**
 * How strongly a faction feels about each moral axis. Range: -1.0 to +1.0 per axis.
 *   +1.0 = faction fully endorses this behavior (members do it, won't punish it).
 *   -1.0 = faction utterly condemns it (severe punishment, hostility toward offenders).
 *    0.0 = indifferent (no opinion, won't react).
 *
 * Simulation uses:
 *   - Dot-product against player/NPC moral signatures → checked against faction thresholds
 *     to decide reaction severity (disapproval, criminal charge, hostile response).
 *   - Diplomacy: ideology distance between two factions affects relationship score.
 *     Similar ideologies → trend toward allies. Opposed → trend toward hostile.
 *   - Economy: specific axes gate income sources (Theft → raid income, Authority → tax income).
 *   - Faction Evolution: ideology drift + schism detection compare axis values.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicIdeologyProfile {
    GENERATED_BODY()

    /**
     * +1.0 = glorifies combat (won't punish murder, NPCs cheer killing).
     * -1.0 = total pacifism (any kill near territory triggers hostility, guards attack killers).
     *  0.0 = pragmatic (self-defense OK, unprovoked murder still condemned).
     * Sim effect: when a kill event occurs near this faction's territory, the severity
     * is (event.Violence × this.Violence). Negative result = faction disapproves.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float Violence = 0.0f;

    /**
     * +1.0 = theft is honorable (faction members steal openly, won't report theft).
     * -1.0 = theft is unforgivable (pickpocket → criminal charge, looting → hostile response).
     *  0.0 = indifferent.
     * Sim effect: factions with Theft > RaidIdeologyThreshold (default 0.3) earn raid income
     * by stealing from nearby trade routes. Higher Theft → more profitable raiding.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float Theft = 0.0f;

    /**
     * +1.0 = cunning and manipulation are virtues (fraud goes unpunished).
     * -1.0 = absolute intolerance for lies (forged documents → death sentence).
     *  0.0 = neutral.
     * Sim effect: primarily affects NPC reaction severity to deception-tagged events.
     * Two factions with opposing Deception values will have high ideology distance → diplomatic friction.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float Deception = 0.0f;

    /**
     * +1.0 = always spare the fallen (executing prisoners → max severity reaction).
     * -1.0 = no quarter given (showing mercy is weakness, faction kills prisoners on sight).
     *  0.0 = practical (won't punish either choice).
     * Sim effect: affects reaction to death/combat events. A Mercy=-1.0 faction won't penalize
     * you for killing surrendered enemies; a Mercy=+1.0 faction treats it as a grave offense.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float Mercy = 0.0f;

    /**
     * +1.0 = oaths are absolute (betrayal = death, desertion = hunt-on-sight).
     * -1.0 = every person for themselves (betrayal is just business).
     *  0.0 = moderate expectations.
     * Sim effect: high Loyalty increases faction cohesion — SchismIdeologyThreshold effectively
     * needs more divergence to trigger a split. Low Loyalty → factions fracture more easily.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float Loyalty = 0.0f;

    /**
     * +1.0 = zealously protects sacred sites (defiling a temple → instant war).
     * -1.0 = nothing is sacred (grave robbing, blood rituals all acceptable).
     *  0.0 = secular, no strong opinion.
     * Sim effect: affects reaction severity to sanctity-tagged events (shrine desecration,
     * cannibalism, necromancy). High Sanctity factions will condemn necromancer factions.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float Sanctity = 0.0f;

    /**
     * +1.0 = absolute rule, strict hierarchy (rebellion → instant hostile response).
     * -1.0 = anarchist (no laws, anti-government, won't enforce order).
     *  0.0 = moderate governance.
     * Sim effect: factions with Authority > TaxAuthorityThreshold (default -0.5) earn tax
     * income from their population. Also affects reaction to authority-tagged events
     * (assassinating leaders, freeing prisoners, inciting rebellion).
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float Authority = 0.0f;

    /**
     * +1.0 = embraces all magic (mages welcomed, enchanting encouraged).
     * -1.0 = magic is abomination (mages hunted, enchanted items destroyed on sight).
     *  0.0 = neutral on magic.
     * Sim effect: affects reaction to arcane-tagged events (spellcasting, enchanting, potions).
     * Two factions at opposite Arcane extremes will have large ideology distance → diplomatic hostility.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float Arcane = 0.0f;

    /** Indexed access for dot-product evaluation loops. */
    float GetAxis(EMythicMoralAxis Axis) const {
        switch (Axis) {
        case EMythicMoralAxis::Violence:
            return Violence;
        case EMythicMoralAxis::Theft:
            return Theft;
        case EMythicMoralAxis::Deception:
            return Deception;
        case EMythicMoralAxis::Mercy:
            return Mercy;
        case EMythicMoralAxis::Loyalty:
            return Loyalty;
        case EMythicMoralAxis::Sanctity:
            return Sanctity;
        case EMythicMoralAxis::Authority:
            return Authority;
        case EMythicMoralAxis::Arcane:
            return Arcane;
        default:
            return 0.0f;
        }
    }

    /** Indexed access for simulation writes. */
    float &GetAxisMutable(EMythicMoralAxis Axis) {
        switch (Axis) {
        case EMythicMoralAxis::Violence:
            return Violence;
        case EMythicMoralAxis::Theft:
            return Theft;
        case EMythicMoralAxis::Deception:
            return Deception;
        case EMythicMoralAxis::Mercy:
            return Mercy;
        case EMythicMoralAxis::Loyalty:
            return Loyalty;
        case EMythicMoralAxis::Sanctity:
            return Sanctity;
        case EMythicMoralAxis::Authority:
            return Authority;
        case EMythicMoralAxis::Arcane:
            return Arcane;
        default:
            static float Dummy = 0.0f;
            return Dummy;
        }
    }
};

// ─────────────────────────────────────────────────────────────
// Faction Data — Runtime state for one faction
// ─────────────────────────────────────────────────────────────

/**
 * Runtime data for a single faction. Maintained by the background simulation thread.
 * Game thread reads a snapshot copy.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicFactionData {
    GENERATED_BODY()

    /** Faction display name */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText DisplayName;

    /** Gameplay tag identifying this faction (e.g. "Faction.Imperials") */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTag FactionTag;

    /** Is this faction still alive? False = annihilated */
    UPROPERTY(BlueprintReadOnly)
    bool bAlive = true;

    /** Current lifecycle status (Active, Annihilated, Resistance, Dormant) */
    UPROPERTY(BlueprintReadOnly)
    EMythicFactionStatus Status = EMythicFactionStatus::Active;

    /**
     * This faction's moral stance on each axis.
     * Compared against player/NPC moral signatures to determine reaction severity.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ideology")
    FMythicIdeologyProfile Ideology;

    /**
     * How severe must an action be before the faction disapproves?
     * Actions scoring above this threshold trigger mild reputation loss, suspicious NPC dialogue, etc.
     * Lower values = faction is more easily offended. Range: 0.0 - 1.0.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ideology", meta = (ClampMin = "0.0", ClampMax = "1.0",
        Tooltip = "Severity threshold for mild disapproval (suspicious dialogue, minor rep loss). Lower = more easily offended."))
    float DisapproveThreshold = 0.2f;

    /**
     * Actions scoring above this threshold are considered criminal by this faction.
     * Triggers bounty, guard alerts, faction reputation penalties.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ideology", meta = (ClampMin = "0.0", ClampMax = "1.0",
        Tooltip = "Severity threshold for criminal condemnation (bounties, guard alerts, major rep loss)."))
    float CondemnThreshold = 0.5f;

    /**
     * Actions scoring above this threshold provoke an immediate hostile response.
     * Faction members will attack on sight, declare war, or escalate to lethal force.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ideology", meta = (ClampMin = "0.0", ClampMax = "1.0",
        Tooltip = "Severity threshold for hostile response (attack on sight, war declaration, lethal force)."))
    float HostileThreshold = 0.8f;

    // ─── Behavior Flags (runtime-mutable, drive sim formula selection) ───
    // These flags determine which simulation formulas apply to a faction.
    // The sim can flip these at runtime based on faction evolution (e.g., a warband
    // capturing enough territory gains bControlsTerritory + bHasCivilianPopulation).

    /**
     * Faction produces resources from territory cells and expands/defends via influence.
     * TRUE: Kingdom, Empire, Religious order. FALSE: Bandit roamers, merchant guilds.
     * Controls: territory-based production scaling, influence propagation, governance evolution.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
    bool bControlsTerritory = true;

    /**
     * Faction participates in resource production, consumption, and trade.
     * TRUE: Any faction with an economy. FALSE: Mindless creatures, undead hordes.
     * When false: pop grows only via spawning (SpawnRatePerCell), no resource tracking.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
    bool bHasEconomy = true;

    /**
     * Population grows organically via births/deaths/migration (vs spawning or recruitment).
     * TRUE: Kingdoms, settlements, villages. FALSE: Mercenary bands (recruitment), creature packs (spawning).
     * Affects: birth/death/migration formulas, food sufficiency impact.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
    bool bHasCivilianPopulation = true;

    /**
     * Faction can buy/sell resources with other factions via natural trade.
     * TRUE: Kingdoms, merchants, mercenaries. FALSE: Bandits, creatures.
     * Trade only occurs with Neutral-or-better relationships.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
    bool bParticipatesInTrade = true;

    /**
     * Faction relationships can shift via diplomacy scoring each tick.
     * TRUE: All sentient factions. FALSE: Creatures, undead (locked to Hostile forever).
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
    bool bCanNegotiate = true;

    // ─── Economy ──────────────────────────────────────────

    /**
     * Designer-set production profile. Defines this faction's economic identity.
     * Each resource is produced per tick scaled by (ControlledCells / ReferenceCells).
     * Ex: Farming kingdom: Food=1.0, Mat=0.3, Arms=0.0, Wealth=0.1
     * Ex: Mining hold:     Food=0.2, Mat=1.0, Arms=0.0, Wealth=0.2
     * Ex: Bandits:         All 0.0 (they steal, not produce)
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy")
    FMythicResourceStock BaseProduction;

    /** Computed supply per tick. Territory production + income sources (tax/raid/contract). Sim-written. */
    UPROPERTY(BlueprintReadOnly, Category = "Economy")
    FMythicResourceStock Supply;

    /** Computed demand per tick. Population food + military upkeep. Sim-written. */
    UPROPERTY(BlueprintReadOnly, Category = "Economy")
    FMythicResourceStock Demand;

    /**
     * Resource stockpile. Accumulated Supply - Demand each tick. Can go negative (deficit).
     * Negative reserves = faction is struggling. Affects food sufficiency, military strength, trade.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Economy")
    FMythicResourceStock Reserves;

    /** Market prices = Demand / max(Supply, 0.01). High demand + low supply = expensive. Sim-written. */
    UPROPERTY(BlueprintReadOnly, Category = "Economy")
    FMythicResourceStock Prices;

    /**
     * Aggregate military strength 0.0-1.0. Derived: (Arms*0.6 + Wealth*0.4) / MaxReserve, clamped.
     * 0.0 = defenseless. 0.5 = moderate. 1.0 = fully armed. Affects contract income, diplo weight.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Economy")
    float MilitaryStrength = 0.5f;

    // ─── Population ───────────────────────────────────────

    /**
     * Total population count. Set initial value here in data asset.
     * Ex: Large kingdom=500, Small guild=40, Creature pack=0 (grows via spawning).
     * Sim modifies this each tick via births/deaths/migration/spawning/recruitment.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Population")
    int32 Population = 0;

    /**
     * The faction's Population at the instant it was annihilated, captured by AnnihilateFaction BEFORE Population is
     * zeroed, so the TickFactionEvolution refugee-absorption pass can grant a fraction of it to the nearest ally.
     * Transient + within-tick: set on annihilation, consumed (cleared to 0) by the same tick's absorption pass, so it
     * is always 0 between ticks and is NOT serialized. Without it the absorption pass was dead (the collector's
     * `!bAlive && Population>0` predicate is mutually exclusive with AnnihilateFaction zeroing Population in one call).
     */
    UPROPERTY(Transient)
    int32 LastAlivePopulation = 0;

    /**
     * Sticky latch set by the sim when Population first goes above 0 (and seeded from Population>0 at registration).
     * Prevents annihilation of newly registered factions that haven't been populated yet (e.g., territory grid hasn't
     * assigned cells). It is a one-way latch (never cleared), so it IS persisted via the custom Serialize() — a faction
     * that lost its population but still holds territory must stay annihilation-eligible across reload. UPROPERTY(Transient)
     * only suppresses UE's reflection-based serialization; it does NOT affect this class's explicit FArchive override.
     */
    UPROPERTY(Transient)
    bool bHasBeenPopulated = false;

    /**
     * Dirty flag: set when ideology drifts during TickIdeologyMetabolism.
     * Consumed by NPC generation pipeline to indicate spawned NPC templates need
     * regeneration (personality, visual archetype, etc.). Reset after consumption.
     * Phase 6: crystallization also reads this flag.
     */
    UPROPERTY(Transient)
    bool bIdeologyDirty = false;

    /**
     * Edge-trigger latches for the distress chronicle beats (famine / military weakness). Set when the faction ENTERS
     * the distress condition so TickHistoryAppend emits the chronicle event exactly ONCE per episode (not every tick),
     * cleared when the condition resolves. Transient (not serialized) — a still-distressed faction re-logs once on
     * reload, which is acceptable.
     */
    UPROPERTY(Transient)
    bool bFamineActive = false;

    UPROPERTY(Transient)
    bool bWeaknessActive = false;

    /** Index of the current leader NPC entity (0 = no leader). Set by crystallization (Phase 6). */
    UPROPERTY(BlueprintReadOnly, Category = "Population")
    int32 LeaderEntityId = 0;

    /** Significance score of the current leader. Used for leadership succession on leader death. */
    UPROPERTY(BlueprintReadOnly, Category = "Population")
    float LeaderSignificanceScore = 0.0f;

    // ─── Territory ────────────────────────────────────────

    /** Number of territory grid cells this faction currently dominates. Sim-written from territory propagation. */
    UPROPERTY(BlueprintReadOnly, Category = "Territory")
    int32 ControlledCellCount = 0;
};

// ─────────────────────────────────────────────────────────────
// Faction Database Settings — Data asset
// ─────────────────────────────────────────────────────────────

UCLASS(BlueprintType, Const)
class MYTHIC_API UMythicFactionDatabaseSettings : public UDataAsset {
    GENERATED_BODY()

public:
    /** Maximum number of factions the system supports. Drives array sizing. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "2", ClampMax = "64"))
    int32 MaxFactions = 20;

    /** Initial faction definitions. Loaded at startup. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TArray<FMythicFactionData> InitialFactions;
};

// ─────────────────────────────────────────────────────────────
// Faction Database — Double-buffered faction state
// ─────────────────────────────────────────────────────────────

/**
 * Central faction database. Background thread maintains the write copy,
 * game thread reads from the committed snapshot.
 *
 * Faction relationships are stored as a flat array (NxN matrix, packed upper triangle).
 */
UCLASS()
class MYTHIC_API UMythicFactionDatabase : public UObject {
    GENERATED_BODY()

public:
    /** Initialize from settings. Must be called once before use. */
    void Initialize(const UMythicFactionDatabaseSettings *Settings);

    /** Cleanup arrays before destruction */
    virtual void BeginDestroy() override;

    // ─── Write Interface (Background Thread Only) ─────────

    /** Get mutable reference to a faction's data in the write buffer */
    FMythicFactionData *GetFactionMutable(FMythicFactionId Id);

    /** Set the relationship between two factions */
    void SetRelationship(FMythicFactionId A, FMythicFactionId B, EMythicFactionRelation Relation);

    /**
     * Register a new faction at runtime (faction creation from schism/conquest).
     * Returns the assigned FactionId, or invalid if the cap is reached.
     */
    FMythicFactionId RegisterFaction(const FMythicFactionData &Data);

    /**
     * Create a resistance faction from the remnants of an annihilated faction (REQ-FAC-003).
     * Copies ideology, assigns a 'Resistance' tag suffix, and sets status to Resistance.
     * Returns the new FactionId, or invalid if creation fails.
     */
    FMythicFactionId CreateFactionFromConquest(FMythicFactionId OriginalFaction, int32 SurvivorCount);

    /** Mark a faction as annihilated */
    void AnnihilateFaction(FMythicFactionId Id);

    /** Get number of registered factions (for sim iteration bounds) */
    int32 GetRegisteredCount() const { return RegisteredCount; }

    /** Get mutable faction data by raw index (for sim loops that iterate all factions) */
    FMythicFactionData *GetFactionMutableByIndex(int32 Index);

    /** Read relationship from write buffer (for background thread reads during sim) */
    EMythicFactionRelation GetWriteRelationship(FMythicFactionId A, FMythicFactionId B) const;

    /** Iterate all alive factions in write buffer (background thread only) */
    void ForEachAliveFactionMutable(TFunctionRef<void(FMythicFactionId, FMythicFactionData &)> Callback);

    /** Commit the write buffer — game thread snapshot swap */
    void CommitWrites();

    // ─── Read Interface (Game Thread — Lock-Free) ─────────

    /** Get faction data by ID (read snapshot). Returns false if ID is invalid. Copies thread-safe snapshot. */
    bool GetFaction(FMythicFactionId Id, FMythicFactionData &OutData) const;

    /** Get faction by gameplay tag. Linear scan — use sparingly. Copies thread-safe snapshot. Optionally returns ID. */
    bool FindFactionByTag(const FGameplayTag &Tag, FMythicFactionData &OutData, FMythicFactionId *OutId = nullptr) const;

    /** Find faction ID by tag. Lightweight linear scan. Thread-safe. */
    FMythicFactionId FindFactionId(const FGameplayTag &Tag) const;

    /** Get the relationship between two factions */
    EMythicFactionRelation GetRelationship(FMythicFactionId A, FMythicFactionId B) const;

    /** Get the number of active (alive) factions */
    int32 GetActiveFactionCount() const;

    /** Get the maximum faction capacity */
    int32 GetMaxFactions() const { return MaxFactions; }

    /** Iterate all alive factions */
    void ForEachAliveFaction(TFunctionRef<void(FMythicFactionId, const FMythicFactionData &)> Callback) const;

    /**
     * Report a leadership candidate for a faction (game thread only).
     * Called by the SignificanceProcessor when it encounters a Tier 2+ entity
     * that belongs to a faction. If the entity's significance score exceeds
     * the current leader's score, the entity becomes the new leader.
     *
     * @param FactionId     Faction to nominate a leader for
     * @param EntityId      MASS entity ID of the candidate
     * @param Score         Significance score of the candidate
     */
    void ReportLeaderCandidate(FMythicFactionId FactionId, uint32 EntityId, float Score);

    // ─── Serialization ───────────────────────────────────

    /**
     * Serialize all faction data and relationships for save/load.
     * Includes ideology profiles, resources, population, relationships,
     * leader tracking, and dirty flags.
     */
    virtual void Serialize(FArchive &Ar) override;

private:
    int32 MaxFactions = 0;

    /** Write buffer — background thread only */
    UPROPERTY(Transient)
    TArray<FMythicFactionData> WriteFactions;

    /** Read buffer — game thread snapshot */
    UPROPERTY(Transient)
    TArray<FMythicFactionData> ReadFactions;

    /** Relationship matrix — flat array, indexed by RelationIndex(A, B) */
    UPROPERTY(Transient)
    TArray<EMythicFactionRelation> WriteRelationships;

    UPROPERTY(Transient)
    TArray<EMythicFactionRelation> ReadRelationships;

    /** Number of factions currently registered. Atomic: the sim thread increments it under SimulationLock (RegisterFaction,
     *  SpawnSplinterFaction), while game-thread accessors (GetFaction/GetActiveFactionCount/ForEachAliveFaction) read it
     *  lock-free — a plain int32 there is a data race (benign-bounded, but real UB). Atomic load/store removes it. */
    std::atomic<int32> RegisteredCount = 0;

    /** Lock protecting the Read snapshots from concurrent modification by CommitWrites */
    mutable FCriticalSection SnapshotLock;

    /** Flatten two faction IDs into a relationship matrix index (upper triangle) */
    int32 RelationIndex(FMythicFactionId A, FMythicFactionId B) const {
        const int32 Low = FMath::Min(A.Index, B.Index);
        const int32 High = FMath::Max(A.Index, B.Index);
        return Low * MaxFactions + High;
    }
};
