// Mythic Living World — Group Spawner Processor
// MASS processor that spawns CLUSTERED groups of NPCs (a noble's retinue, a merchant's barter party, a friend trio) into
// settlement cells near players, with intra-group social-graph edges. It is the SETTLEMENT-cell counterpart to the
// territory patrol spawner (which fills open faction country): per chance-passing settlement cell it weighted-picks one
// eligible group template, rolls each member spec's count (total capped at MaxGroupMembers), and spawns the members as
// normal {Identity, Schedule, Significance, FMythicGroupFragment, FMythicNPCTag, FMythicGroupMemberTag} entities sharing
// one Identity.Cell. Because members carry FMythicNPCTag they ride the EXISTING significance/embodiment/despawn path,
// count toward the shared per-cell cap, and are culled by the population despawner (single despawn authority) — this
// processor adds NO embodiment branch and NO despawn logic.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "GroupSpawnerProcessor.generated.h"

class UMythicLivingWorldSubsystem;
class UMythicTerritoryGrid;
class UMythicFactionDatabase;
struct FMythicGroupTemplate;
struct FMythicGroupMemberSpec;
struct FMythicFactionData;
enum class EMythicSettlementEconomy : uint8;

/**
 * MASS processor that maintains a population of clustered NPC GROUPS in settlement cells near players.
 *
 * Each throttled tick:
 * 1. Gather player positions → cell coordinates.
 * 2. Count existing distinct active groups (MaxActiveGroups cap) and per-cell NPC density (shared MaxEntitiesPerCell).
 * 3. For each settlement cell within GroupSpawnRadius of a player (deduped) with a valid, alive, Active governing
 *    faction: a deterministic per-(faction,cell) chance roll decides whether to seed a group. On a pass, weighted-pick
 *    one eligible template (military/pop/reserve-wealth/economy gates), roll member counts, and — if the whole group
 *    fits the per-cell + per-tick budgets — queue all members for one deferred batch-create.
 * 4. Deferred-create the members and wire intra-group social edges (ordered member pairs) INSIDE the create lambda,
 *    where the entity handles exist.
 *
 * Runs PrePhysics, Server|Standalone, game thread, at GroupSpawnIntervalSeconds. ExecuteAfter the population spawner so
 * the per-cell count it reads already reflects this tick's ambient spawns where the radii overlap.
 */
UCLASS()
class MYTHIC_API UMythicGroupSpawnerProcessor : public UMassProcessor {
    GENERATED_BODY()

public:
    UMythicGroupSpawnerProcessor();

    /**
     * Is this template eligible for a settlement cell governed by Faction with the resolved EffEconomy? Checks the
     * faction military/population/reserve-wealth gates and the AllowedEconomies list (empty = any). Pure + static so the
     * eligibility rule is unit-testable. (Faction liveness is the caller's responsibility — by the time we test a
     * template the faction is already known alive+Active.)
     */
    static bool TemplateEligible(const FMythicGroupTemplate &Template, const FMythicFactionData &Faction,
                                 EMythicSettlementEconomy EffEconomy);

    /**
     * Weighted-pick a template index over the ELIGIBLE templates by RelativeWeight, deterministically from Seed. Pure +
     * static + lock-free. Returns false (OutIndex untouched) when no template is eligible or total weight is ~0.
     * @param Templates    All templates (authored DB or code defaults).
     * @param EffEconomy   Resolved settlement economy for the cell.
     * @param Faction      Governing faction snapshot (for the eligibility gates).
     * @param Seed         Deterministic seed (NameHash-style) — the sole RNG source.
     * @param OutIndex     Index into Templates of the chosen eligible template.
     */
    static bool PickTemplateIndex(const TArray<FMythicGroupTemplate> &Templates, EMythicSettlementEconomy EffEconomy,
                                  const FMythicFactionData &Faction, uint32 Seed, int32 &OutIndex);

    /**
     * Roll a member spec's spawn count in [MinCount, MaxCount] deterministically from Seed (salted by the spec's role so
     * two specs in one template roll independently). Pure + static. The result is NOT yet clamped to the group/tick
     * headroom — the caller applies that.
     */
    static int32 RollMemberCount(const FMythicGroupMemberSpec &Spec, uint32 Seed);

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    /** Existing-ambient-NPC count query — SAME SHAPE as the population spawner's so the per-cell density count is
     *  consistent with the shared MaxEntitiesPerCell cap (group members carry FMythicNPCTag, so they self-count on
     *  subsequent ticks). Read-only. */
    FMassEntityQuery ExistingNPCQuery;

    /** Existing-group-member query (entities carrying FMythicGroupFragment) — used to count distinct active GroupIds for
     *  the MaxActiveGroups cap. Read-only. */
    FMassEntityQuery ExistingGroupQuery;

    /** Timer accumulator — processor runs at GroupSpawnIntervalSeconds, not every frame. */
    float TimeSinceLastTick = 0.0f;
};
