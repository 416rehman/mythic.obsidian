#pragma once

#include "CoreMinimal.h"
#include "MythicTradingConfig.generated.h"

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicTradingConfig {
    GENERATED_BODY()


    /** Max ABSOLUTE Reserve delta applied per (faction, axis) per SIM TICK from the player queue (deliveries, stall
     *  sales). The over-clamp remainder carries to later ticks — a burst of deliveries trickles in at this ceiling.
     *  Sized against the sim's MaxReserve=100 band: 5.0 ⇒ a famine (Food < −50) needs sustained deliveries over many
     *  ticks to clear, never one truck. Non-positive ⇒ nothing injects (deltas pool until raised). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "P9 Injection", meta = (ClampMin = "0.0"))
    float MaxReserveInjectionPerAxisPerTick = 5.0f;

    /** Reserve units injected per delivered GOODS unit (vendor deliveries). 1 unit of grain = 1.0 Reserves.Food
     *  before the per-tick clamp meters it in. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "P9 Injection", meta = (ClampMin = "0.0"))
    float DeliveryUnitsToReservePerUnit = 1.0f;

    /** Reserve units injected per player-stall unit SOLD (the K-surplus valve; C3 — the stall is the one
     *  passive-sale surface). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "P9 Injection", meta = (ClampMin = "0.0"))
    float StallUnitsToReservePerUnit = 1.0f;


    /** Half-life (seconds) of a stale (fog-of-war) ledger entry's confidence: staleness = 1 − 0.5^(age/halflife).
     *  Stale prices are additionally quantized into coarser bands as they age (rumors get vague, never wrong-signed). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ledger", meta = (ClampMin = "1.0"))
    float LedgerStalenessHalfLifeSeconds = 600.0f;

    /** How often (seconds) the world-shared RUMOR snapshot of each settlement's prices refreshes. Fog-of-war readers
     *  see this lagged snapshot; live readers (POI-unlocked or non-hostile standing) see the current commit. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ledger", meta = (ClampMin = "1.0"))
    float RumorSnapshotRefreshSeconds = 300.0f;

    /** Minimum cross-settlement price DIFFERENTIAL (per axis, absolute sim-price units) before a trade-rumor
     *  chronicle beat is worth surfacing. Sim prices hover ~1.0 (Demand/Supply), so 0.4 = a real spread. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ledger", meta = (ClampMin = "0.0"))
    float RumorMinDifferential = 0.4f;

    /** Cooldown (seconds) between trade-rumor beats per axis (anti-spam on the chronicle). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ledger", meta = (ClampMin = "0.0"))
    float RumorCooldownSeconds = 600.0f;


    /** A faction axis Reserve at/below this emits an edge-triggered Trade.Deficit.<Axis> world event (the delivery
     *  contract trigger for Materials/Arms; Food keeps the sim's own famine beat at −50). Latched per (faction, axis). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contracts")
    float DeficitReserveThreshold = -25.0f;

    /** The deficit latch re-arms once the axis Reserve recovers ABOVE this (hysteresis so a reserve oscillating at
     *  the threshold doesn't strobe beats). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contracts")
    float DeficitRearmThreshold = 0.0f;

    /** How long (seconds) a posted delivery-contract offer stays open on the board before it is retired. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contracts", meta = (ClampMin = "1.0"))
    float ContractOfferLifetimeSeconds = 900.0f;


    /** Seconds between stall sell-through drain passes while the stall is stocked (ONE re-armed timer; P7). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stall", meta = (ClampMin = "5.0"))
    float StallDrainIntervalSeconds = 300.0f;

    /** Per-stack chance a listed stack sells ONE unit per drain pass when listed exactly AT the fair scarcity price.
     *  Under-pricing raises it (capped ×2 at half price), over-pricing kills it at StallPriceCeilingRatio. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stall", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float StallBaseSaleChancePerDrain = 0.25f;

    /** Listed/fair price ratio at (or above) which nothing ever sells (2.0 = double the fair price finds no buyer). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stall", meta = (ClampMin = "1.01"))
    float StallPriceCeilingRatio = 2.0f;

    /** Cap on how many drain passes AWAY-TIME can accrue while the stall was unloaded (clock-safe P7 accrual:
     *  48 × 300s = 4h of catch-up commerce max — generous, never an exploit farm). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stall", meta = (ClampMin = "0"))
    int32 StallMaxAccruedDrains = 48;


    /** Cargo value at which heat saturates (heat = min(value/reference, 1) × danger scale). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Risk", meta = (ClampMin = "1.0"))
    float CargoHeatValueReference = 2000.0f;

    /** Danger tier (EMythicDangerTier ordinal) below which cargo heat is ZERO (design: heat needs danger ≥ 2 —
     *  Moderate — AND valuable cargo; safe roads never manufacture ambushes). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Risk", meta = (ClampMin = "0", ClampMax = "4"))
    int32 CargoHeatMinDangerTier = 2;

    /** Significance of the contraband-sale action submitted to the crime-witness pipeline (trespass is 0.3; kills
     *  are 1.0 — smuggling sits between: seen fencing stolen goods is worse than wandering in). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Risk", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ContrabandCrimeSignificance = 0.4f;
};
