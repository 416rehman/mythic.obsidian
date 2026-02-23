// Mythic Living World System — Settings data asset
// Centralizes all configurable parameters for the living world system.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "LivingWorldSettings.generated.h"

class UMythicSettlementSettings;

/**
 * Master settings for the Living World System.
 * Create an instance of this data asset in the editor and assign it to the
 * Living World Subsystem (via project settings or direct reference).
 *
 * All configurable parameters live here — no hardcoded values in code.
 */
UCLASS(BlueprintType, Const)
class MYTHIC_API UMythicLivingWorldSettings : public UDataAsset {
    GENERATED_BODY()

public:
    // ─── Shared Data ──────────────────────────────────────

    /** Causal Fabric ring buffer capacity. Determines how many events are in active memory. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Causal Fabric", meta = (ClampMin = "256", ClampMax = "65536"))
    int32 FabricCapacity = 4096;

    /** Faction database settings (initial factions, max count) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Factions")
    TSoftObjectPtr<UMythicFactionDatabaseSettings> FactionSettings;

    /** Territory grid settings (dimensions, cell size, bleed rate) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Territory")
    TSoftObjectPtr<UMythicTerritoryGridSettings> TerritorySettings;

    // ─── Background Thread ────────────────────────────────

    /**
     * Real-time interval between background simulation ticks (seconds).
     * Lower = more responsive world. Higher = less CPU.
     * At 1.0s real time, the sim runs at reasonable game-time intervals.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Simulation", meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float SimTickIntervalSeconds = 1.0f;

    // ─── Budget Caps ──────────────────────────────────────

    /** Max significance promotions per game frame */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Budgets", meta = (ClampMin = "1", ClampMax = "20"))
    int32 MaxPromotionsPerFrame = 4;

    /** Max significance rescores per game frame */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Budgets", meta = (ClampMin = "1", ClampMax = "100"))
    int32 MaxRescoresPerFrame = 20;

    /** Max pressure evaluations per game frame */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Budgets", meta = (ClampMin = "1", ClampMax = "200"))
    int32 MaxPressureEvalsPerFrame = 50;

    /** Max witness evaluations per event per game frame */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Budgets", meta = (ClampMin = "1", ClampMax = "500"))
    int32 MaxWitnessEvalsPerFrame = 100;

    /** Max causal event propagations per game frame */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Budgets", meta = (ClampMin = "1", ClampMax = "50"))
    int32 MaxPropagationsPerFrame = 10;

    /** Max cognitive NPC actors alive at any time */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Budgets", meta = (ClampMin = "1", ClampMax = "100"))
    int32 MaxCognitiveActors = 30;

    // ─── Significance ─────────────────────────────────────

    /** Score threshold to promote an NPC to a higher tier */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Significance", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float PromotionThreshold = 0.7f;

    /** Score threshold to demote an NPC to a lower tier (hysteresis gap prevents oscillation) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Significance", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DemotionThreshold = 0.3f;

    // ─── Weights for significance scoring ─────────────────

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Significance")
    float ProximityWeight = 0.4f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Significance")
    float EventCountWeight = 0.3f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Significance")
    float EmotionalIntensityWeight = 0.3f;

    // ─── Economy Simulation ──────────────────────────────

    /**
     * Baseline territory for production scaling.
     * Supply = BaseProduction × (ControlledCells / ReferenceCells).
     * Ex: ReferenceCells=20, faction has 10 cells → 50% of BaseProduction. 40 cells → 200%.
     * Higher = factions need more territory before reaching full production capacity.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "1"))
    int32 ReferenceCellCount = 20;

    /**
     * Food consumed per person per sim tick.
     * Ex: Pop=500, rate=0.001 → 0.5 Food consumed/tick. Higher = factions starve faster.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0.0"))
    float FoodPerCapita = 0.001f;

    /**
     * Arms consumed per unit of MilitaryStrength per tick (maintenance cost of armies).
     * Ex: MilStrength=0.8, rate=0.01 → 0.008 Arms/tick. Higher = armies expensive to maintain.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0.0"))
    float ArmsUpkeepRate = 0.01f;

    /**
     * Wealth consumed per unit of MilitaryStrength per tick (soldier wages).
     * Ex: MilStrength=0.8, rate=0.005 → 0.004 Wealth/tick. Combined with ArmsUpkeepRate = total military cost.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0.0"))
    float WealthUpkeepRate = 0.005f;

    /**
     * Materials needed to produce 1 unit of Arms.
     * Ex: rate=2.0 → costs 2 Materials per Arms. Higher = factions must be material-rich to militarize.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0.0"))
    float MaterialsPerArm = 2.0f;

    /**
     * Max Arms a faction can produce per tick (capped by available Materials).
     * Ex: rate=0.05, has 1.0 Materials → produces min(0.05, 1.0/MaterialsPerArm) = 0.05 Arms.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0.0"))
    float ArmsProductionRate = 0.05f;

    /**
     * Wealth generated per person per tick (taxation). Only for factions with Authority > TaxAuthorityThreshold.
     * Ex: Pop=500, rate=0.0005 → 0.25 Wealth/tick. Kingdoms thrive on this, bandits don't qualify.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0.0"))
    float TaxRate = 0.0005f;

    /**
     * Hard cap on each resource reserve (prevents infinite accumulation).
     * Reserves clamped to [-MaxReserve, +MaxReserve] each tick. Also normalizes MilitaryStrength.
     * Ex: 100.0 → max stockpile 100 of each resource. MilStrength = (Arms*0.6 + Wealth*0.4) / 100.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "1.0"))
    float MaxReserve = 100.0f;

    /**
     * Reserve surplus needed before a faction exports resources in natural trade.
     * Ex: threshold=5.0, faction has 8.0 Food → exports up to 3.0. Higher = factions hoard before trading.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0.0"))
    float TradeSurplusThreshold = 5.0f;

    /**
     * Reserve level below which a faction seeks imports. Trade only with Neutral-or-better factions.
     * Ex: threshold=-2.0, faction at -3.0 Food → tries to import from neighbors with surplus.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0.0"))
    float TradeDeficitThreshold = -2.0f;

    /**
     * Theft ideology threshold for raid income. Factions with Ideology.Theft > this steal from trade routes.
     * Low (0.1) = common banditry. High (0.8) = only the most criminal factions raid.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float RaidIdeologyThreshold = 0.3f;

    /**
     * Fraction of nearby trade volume stolen per tick by raiding factions.
     * Ex: nearby trade=10.0, fraction=0.1 → steals 1.0 (70% as Wealth, 30% as Materials).
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float RaidFraction = 0.1f;

    /**
     * Authority ideology threshold for taxation. Factions with Ideology.Authority > this earn tax income.
     * -0.5 = most organized factions qualify. 0.3 = only strongly authoritarian. 1.0 = nobody.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float TaxAuthorityThreshold = -0.5f;

    /**
     * Military strength needed for mercenary contract income. Must also be under PopulationCeiling.
     * Ex: 0.6 = only battle-hardened factions qualify. Prevents peaceful farmers from earning contracts.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ContractMilitaryThreshold = 0.6f;

    /**
     * Max population for contract income. Prevents empires from also earning merc income.
     * Ex: ceiling=200 → 500-pop kingdom can't earn contracts, but 50-pop warband can.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0"))
    int32 ContractPopulationCeiling = 200;

    // ─── Population Simulation ───────────────────────────

    /**
     * Max population supported per controlled territory cell.
     * Total capacity = ControlledCellCount × PopulationPerCell.
     * Ex: 10 cells × 100 = 1000 capacity. Births slow as pop approaches capacity.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Population", meta = (ClampMin = "1"))
    int32 PopulationPerCell = 100;

    /**
     * Growth rate: fraction of the capacity gap filled by births per tick.
     * Births = (Capacity - Population) × BaseBirthRate × FoodSufficiency.
     * Ex: 200 gap, rate=0.01, food=1.0 → 2 births/tick. Higher = faster recovery from losses.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Population", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BaseBirthRate = 0.01f;

    /**
     * Baseline death rate: fraction of population that dies per tick.
     * Amplified 3× during starvation (FoodSufficiency < 0.3).
     * Ex: Pop=500, rate=0.005 → 2 deaths/tick normally, 7 deaths/tick starving.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Population", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BaseDeathRate = 0.005f;

    /**
     * Emigration rate when food is scarce. Scales with starvation severity.
     * Ex: Pop=500, rate=0.01, food at 25% of threshold → ~3 emigrants/tick leaving for better conditions.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Population", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MigrationRate = 0.01f;

    /**
     * Food sufficiency ratio below which emigration begins.
     * FoodSufficiency = FoodReserves / (FoodDemand × 10). <threshold = people flee.
     * Ex: 0.5 → emigration starts when food covers less than ~5 ticks of demand.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Population", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float MigrationFoodThreshold = 0.5f;

    /**
     * Population gained per territory cell per tick for non-civilian factions (creatures, undead).
     * Used when bHasCivilianPopulation=false AND bHasEconomy=false.
     * Ex: 3 cells × 5/cell = 15 new creatures per tick.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Population", meta = (ClampMin = "0"))
    int32 SpawnRatePerCell = 5;

    /**
     * Pop gained per unit of Wealth reserves for recruitment-based factions (mercenaries).
     * Used when bHasEconomy=true AND bHasCivilianPopulation=false.
     * Ex: Wealth=10.0, rate=0.5 → 5 recruits/tick.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Population", meta = (ClampMin = "0.0"))
    float RecruitmentPerWealth = 0.5f;

    // ─── Diplomacy ───────────────────────────────────────
    // RelationshipScore = -(IdeologyDist × IdeologyWeight) + (EconDep × DepWeight) + (EventScore × EventWeight)
    // Score is compared against tier thresholds to determine relationship.

    /**
     * Weight of ideology distance in relationship scoring. Higher = ideology matters more.
     * Ideology distance = sum of absolute differences across all moral axes.
     * Ex: 1.0 → two factions with opposed ideology (dist ~8.0) → -8.0 ideology contribution to score.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Diplomacy", meta = (ClampMin = "0.0"))
    float IdeologyWeight = 1.0f;

    /**
     * Weight of economic dependency in relationship scoring. Trade partners like each other more.
     * Trade volume between two factions → dependency score. Higher weight = trade matters more.
     * Ex: 0.5 → heavy trading partners get +trade_volume * 0.5 to their relationship.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Diplomacy", meta = (ClampMin = "0.0"))
    float DependencyWeight = 0.5f;

    /**
     * Weight of recent Causal Fabric events in relationship scoring.
     * Events near both factions' territory are scored (positive events = boost, negative = penalty).
     * Ex: 0.3 → a war event (significance 1.0) adds -0.3 to the relationship.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Diplomacy", meta = (ClampMin = "0.0"))
    float EventWeight = 0.3f;

    /**
     * Relationship tier thresholds. Score above threshold = that tier.
     * Score > 1.5 = Allied, > 0.5 = Friendly, > -0.5 = Neutral, > -1.5 = Unfriendly, else Hostile.
     * Adjust to make alliances rarer (raise AllyThreshold) or hostility easier (raise HostileThreshold).
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Diplomacy")
    float AllyThreshold = 1.5f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Diplomacy")
    float FriendlyThreshold = 0.5f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Diplomacy")
    float UnfriendlyThreshold = -0.5f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Diplomacy")
    float HostileThreshold = -1.5f;

    /**
     * Prevents rapid oscillation between relationship tiers.
     * A faction must cross threshold±margin to change tier.
     * Ex: 0.2 → to go from Neutral→Friendly (threshold 0.5), score must reach 0.7.
     * To drop back, must fall below 0.3. Prevents jitter at boundaries.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Diplomacy", meta = (ClampMin = "0.0"))
    float HysteresisMargin = 0.2f;

    /**
     * Max events from Causal Fabric scanned per faction pair during diplomacy.
     * Higher = more accurate but slightly more CPU. 10 is a good balance for ~20 factions.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Diplomacy", meta = (ClampMin = "1", ClampMax = "50"))
    int32 DiplomacyEventScanCap = 10;

    // ─── Ideology Metabolism ─────────────────────────────
    // Each tick, faction ideology drifts toward the moral vectors of events
    // occurring in/near their territory. This creates emergent worldview shifts.

    /**
     * Rate at which faction ideology drifts toward lived experience.
     * Per-axis drift = DriftRate × event_significance × event_moral_weight.
     * Ex: 0.001 → very slow drift. A faction surrounded by violence for 100+ ticks
     * gradually becomes more violence-tolerant. Set near 0 for stable factions.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ideology", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float IdeologyDriftRate = 0.001f;

    /**
     * Minimum event significance (0.0-1.0) to affect ideology drift.
     * Filters out trivial events. 0.3 = only notable events shift ideology.
     * Lower = more responsive to minor events. Higher = only major events matter.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ideology", meta = (ClampMin = "0.0"))
    float DriftMinSignificance = 0.3f;

    /**
     * Rate at which non-territorial factions bleed ideology into co-located territorial factions.
     * Simulates cultural influence (e.g., a thieves guild operating in a kingdom's territory
     * slowly shifts the kingdom's Theft tolerance upward).
     * Ex: 0.0005 → very slow cultural contamination. 0.01 → rapid influence.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ideology", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float IdeologyBleedRate = 0.0005f;

    /** Max events scanned per faction for ideology drift. Higher = more accurate, slightly more CPU. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ideology", meta = (ClampMin = "1", ClampMax = "50"))
    int32 IdeologyEventScanCap = 10;

    // ─── Faction Evolution ───────────────────────────────
    // The sim can mutate faction behavior flags at runtime based on these thresholds.
    // This enables emergent transformation: a warband becomes a kingdom, a kingdom shatters.

    /**
     * Cell count above which a non-territorial faction evolves territory control.
     * When a nomadic/roaming faction occupies enough cells, it gains bControlsTerritory + bHasCivilianPopulation.
     * Ex: 5 → a mercenary band holding 5+ cells starts governing.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Evolution", meta = (ClampMin = "1"))
    int32 TerritorialCellThreshold = 5;

    /**
     * Minimum population required alongside cell count for territorial evolution.
     * Prevents empty land claims from triggering governance.
     * Ex: 50 → need at least 50 people + TerritorialCellThreshold cells.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Evolution", meta = (ClampMin = "1"))
    int32 MinTerritorialPopulation = 50;

    /**
     * Cell count below which a territorial faction devolves (loses governance).
     * Faction loses bControlsTerritory + bHasCivilianPopulation, becoming nomadic.
     * Ex: 1 → a kingdom reduced to a single cell reverts to a roaming remnant.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Evolution", meta = (ClampMin = "0"))
    int32 DevolveCellThreshold = 1;

    /**
     * Minimum cells required for schism detection to activate.
     * Schism = a geographic cluster of cells diverges ideologically from the parent faction.
     * Ex: 8 → only factions controlling 8+ cells can split. Small factions are too cohesive.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Evolution", meta = (ClampMin = "2"))
    int32 MinSchismSize = 8;

    /**
     * Ideology distance threshold for a geographic cluster to trigger schism.
     * Distance = sum of absolute ideology axis differences between cluster avg and parent faction.
     * Ex: 1.5 → a cluster drifting 1.5+ total ideology distance from parent triggers a split.
     * Lower = schisms happen easily. Higher = only extreme divergence splits factions.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Evolution", meta = (ClampMin = "0.0"))
    float SchismIdeologyThreshold = 1.5f;

    /**
     * Minimum population in a geographic cluster for schism to fire.
     * Prevents tiny pockets from splitting off.
     * Ex: 30 → a cluster of 10 people won't trigger schism even if ideology diverges.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Evolution", meta = (ClampMin = "1"))
    int32 MinSchismPopulation = 30;

    /**
     * Fraction of dead faction's population absorbed as refugees by allied factions.
     * Split evenly among alive allies. Remainder is lost.
     * Ex: 0.3 → when a faction dies, 30% of its pop joins allies. 70% perishes.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Evolution", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float AbsorptionFraction = 0.3f;

    /**
     * Random ideology mutation range (±) when creating a splinter faction from schism.
     * Applied on top of the cluster's already-diverged ideology.
     * Ex: 0.1 → each axis mutated by ±0.1 from cluster avg. Higher = more chaotic splinters.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Evolution", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SchismIdeologyMutation = 0.1f;

    // ─── Settlements ─────────────────────────────────────

    /** Settlement settings (defaults, capital boost) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settlements")
    TSoftObjectPtr<UMythicSettlementSettings> SettlementSettings;

    // ─── Population Spawner ──────────────────────────────

    /**
     * Cells around each player to populate with MASS entities.
     * Beyond this radius, cells are not spawned. Lower = fewer entities, better performance.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Population Spawner", meta = (ClampMin = "1.0", ClampMax = "20.0"))
    float PopulationSpawnRadius = 3.0f;

    /**
     * Hard cap on MASS entities per territory cell (overrides settlement MaxPopulationDensity if lower).
     * Safety valve against performance issues from misconfigured settlements.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Population Spawner", meta = (ClampMin = "1", ClampMax = "200"))
    int32 MaxEntitiesPerCell = 20;

    /** Cells beyond any player at which MASS entities are despawned */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Population Spawner", meta = (ClampMin = "1.0", ClampMax = "30.0"))
    float PopulationDespawnDistance = 5.0f;

    /** Max entity spawns per processor tick (budget cap to prevent frame spikes) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Population Spawner", meta = (ClampMin = "1", ClampMax = "200"))
    int32 MaxSpawnsPerTick = 50;

    /** Max entity despawns per processor tick */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Population Spawner", meta = (ClampMin = "1", ClampMax = "200"))
    int32 MaxDespawnsPerTick = 50;

    /** Interval in seconds between population spawner ticks (not per-frame) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Population Spawner", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float PopulationSpawnIntervalSeconds = 0.5f;

    // ─── Event Pipeline ──────────────────────────────────

    /**
     * Exponential decay rate per second for pressure channels.
     * Each channel decays: P *= exp(-DecayRate × elapsed). Lazy-evaluated on touch.
     * Lower = long-lasting emotional impact. Higher = entities recover quickly.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event Pipeline", meta = (ClampMin = "0.001", ClampMax = "0.1"))
    float PressureDecayRate = 0.01f;

    /**
     * Pressure level that triggers venting behavior.
     * When any pressure channel exceeds this, the entity routes through personality → vent action.
     * Lower = more reactive NPCs. Higher = only extreme events cause behavioral response.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event Pipeline", meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float VentThreshold = 1.0f;

    /**
     * Cell radius within which entities can "hear" action events and become witnesses.
     * 0 = same cell only, 2 = 2-cell Manhattan distance. Larger = more entities react per event.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event Pipeline", meta = (ClampMin = "0", ClampMax = "5"))
    int32 WitnessHearingRadius = 2;

    /**
     * Interval between witness perception processor ticks (seconds).
     * 0 = every frame (event-driven, zero cost when no events pending).
     * >0 = throttled. Recommend 0.0 since the processor is already event-gated.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event Pipeline", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WitnessIntervalSeconds = 0.0f;

    /**
     * Interval between significance rescore/promotion ticks (seconds).
     * Lower = faster tier transitions. Higher = less CPU.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event Pipeline", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float SignificanceIntervalSeconds = 0.5f;

    /**
     * Prevents oscillation between significance tiers.
     * Entity must cross threshold±margin to change tier.
     * Ex: 0.1 with PromotionThreshold=0.7 → must reach 0.8 to promote, 0.2 to demote.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event Pipeline", meta = (ClampMin = "0.0", ClampMax = "0.5"))
    float SignificanceHysteresisMargin = 0.1f;

    // ─── Creature Ecology ────────────────────────────────

    /** Interval in seconds between creature ecology processor ticks */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Creature Ecology", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float CreatureEcologyIntervalSeconds = 0.5f;

    /** Cells within which pack members share pressure state */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Creature Ecology", meta = (ClampMin = "0.0", ClampMax = "10.0"))
    float PackPressureShareRadius = 2.0f;

    /** Aggression multiplier applied to creatures within their territorial radius of their den */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Creature Ecology", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float TerritorialAggressionBoost = 0.3f;

    /** Max herd-flee contagion propagations per ecology tick (budget cap) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Creature Ecology", meta = (ClampMin = "1", ClampMax = "100"))
    int32 MaxHerdContagionPerTick = 10;
};
