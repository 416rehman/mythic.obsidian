// Mythic Living World — Group spawn templates (data-driven WHO spawns TOGETHER)
// A "group template" describes a small cluster of NPCs that spawn together in a settlement cell with intra-group social
// edges (a noble + his guards, a merchant + his porters, a trio of friends). The GroupSpawnerProcessor (Step 4) draws
// one eligible template per chance-passing settlement cell, rolls each member spec's count, spawns the members as normal
// {Identity,Schedule,Significance,FMythicGroupFragment,FMythicNPCTag,FMythicGroupMemberTag} entities sharing one cell
// (so existing scatter cohesion + the embodiment gate + the shared per-cell cap all apply unchanged), and wires the
// social graph with the template's IntraRelation edges.
//
// This is a strict DATA layer — zero behavior. Members ALWAYS carry the humanoid FMythicNPCTag so they ride the EXISTING
// significance/embodiment/despawn path; the group fragment + member tag are additive KIND markers only.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "World/LivingWorld/Social/SocialGraph.h" // EMythicSocialRelation (intra-group edge type)
// AllowedEconomies is a REFLECTED TArray<EMythicSettlementEconomy> UPROPERTY, so UHT needs the full enum definition (a
// forward decl is insufficient for a reflected enum-typed array element). The enum lives in MythicSettlement.h.
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "GroupTypes.generated.h"

/**
 * Hard cap on members in a single group template. Caps social-edge fan-out: each member adds a directed edge to every
 * OTHER member, so a group of N produces N×(N-1) directed edges and N-1 OUTGOING edges per member. With MaxGroupMembers
 * = 6 that is at most 5 outgoing edges per member — comfortably under UMythicSocialGraph's MaxEdgesPerEntity (default 8),
 * so no group member's own intra-group edges are ever evicted. Enforced in both authoring (RollMemberCount clamps to it)
 * and the processor (member loop stops at it).
 */
static constexpr int32 MaxGroupMembers = 6;

// ─────────────────────────────────────────────────────────────
// Group Member Spec — one role + a count range within a template
// ─────────────────────────────────────────────────────────────

/**
 * One slot in a group template: a role and how many of it to spawn (rolled deterministically in [MinCount,MaxCount]).
 * Exactly one member spec across a template should be the leader (bIsLeader) — the anchor the social edges orient toward
 * (e.g. guards are Subordinate TO the noble). If none is flagged, the first spawned member is treated as the leader.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicGroupMemberSpec {
    GENERATED_BODY()

    /** Role stamped on this member's FMythicIdentityFragment.RoleTag (same role family the archetype catalog uses). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group", meta = (Categories = "NPC.Role"))
    FGameplayTag RoleTag;

    /** Minimum members of this role (inclusive). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group", meta = (ClampMin = "1"))
    int32 MinCount = 1;

    /** Maximum members of this role (inclusive). The rolled count is clamped to the template's remaining headroom. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group", meta = (ClampMin = "1"))
    int32 MaxCount = 1;

    /** If true, this spec is the group's leader (social edges orient toward it; it is spawned first). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group")
    bool bIsLeader = false;
};

// ─────────────────────────────────────────────────────────────
// Group Template — a full clustered spawn recipe
// ─────────────────────────────────────────────────────────────

/**
 * A clustered-spawn recipe: a set of member specs, the social relation that binds them, and the sim gates that decide
 * whether a faction/settlement can field it. The GroupSpawnerProcessor weighted-picks one eligible template per
 * chance-passing settlement cell. Total rolled member count is capped at MaxGroupMembers so the social-edge fan-out
 * stays under the social graph's per-entity edge cap.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicGroupTemplate {
    GENERATED_BODY()

    /** Identifies this group kind (e.g. "NPC.Group.Retinue"). Also stamped on each member's FMythicGroupFragment so the
     *  debugger can tally a group's activity/kind. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group", meta = (Categories = "NPC.Group"))
    FGameplayTag GroupTag;

    /** Human-readable label (debug/UI only). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group")
    FText DisplayName;

    /** The member slots that make up this group (one should be flagged bIsLeader). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group")
    TArray<FMythicGroupMemberSpec> Members;

    /** The social relation written between members at spawn (guard→noble = Subordinate, merchant↔porter = Associate,
     *  friends = Friend). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group")
    EMythicSocialRelation IntraRelation = EMythicSocialRelation::Friend;

    /** Initial strength of each intra-group social edge [0,1]. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float IntraEdgeStrength = 0.6f;

    /** Minimum governing-faction MilitaryStrength [0,1] to field this group (e.g. a retinue needs a strong faction). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group|Requirements", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MinFactionMilitaryStrength = 0.0f;

    /** Minimum governing-faction Population to field this group. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group|Requirements", meta = (ClampMin = "0"))
    int32 MinFactionPopulation = 0;

    /** Minimum governing-faction Reserves.Wealth to field this group (a noble retinue needs a wealthy faction). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group|Requirements")
    float MinReserveWealth = 0.0f;

    /** Economies this group may appear in (resolved settlement economy). Empty = any economy. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group|Requirements")
    TArray<EMythicSettlementEconomy> AllowedEconomies;

    /** Relative weight among eligible templates in the weighted pick. 0 = effectively disabled. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group", meta = (ClampMin = "0.0"))
    float RelativeWeight = 1.0f;
};

// ─────────────────────────────────────────────────────────────
// Group Template Database — designer-authored data asset
// ─────────────────────────────────────────────────────────────

/**
 * Designer-authored catalog of group templates. Referenced from DA_LivingWorldSettings → GroupTemplateDatabase. When
 * unset, the group spawner falls back to MythicGroupDefaults::BuildDefaultTemplates() so the system runs unauthored.
 */
UCLASS(BlueprintType)
class MYTHIC_API UMythicGroupTemplateDatabase : public UDataAsset {
    GENERATED_BODY()

public:
    /** All group templates in this database. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Groups")
    TArray<FMythicGroupTemplate> Templates;

    /** Find a template by its group tag (exact match). Returns nullptr if not present. Mirrors
     *  UMythicArchetypeCatalog::FindByRole. */
    const FMythicGroupTemplate *FindByTag(const FGameplayTag &GroupTag) const {
        for (const FMythicGroupTemplate &T : Templates) {
            if (T.GroupTag.MatchesTagExact(GroupTag)) {
                return &T;
            }
        }
        return nullptr;
    }
};

// ─────────────────────────────────────────────────────────────
// Code-default group templates
// ─────────────────────────────────────────────────────────────

/**
 * Built-in group templates so clustered spawning runs with ZERO authored data: a Noble + 2 Guards retinue (guards
 * Subordinate to the noble), a Merchant + 2-3 Civilian porters barter party (Associate), and a 3-Civilian friend trio
 * (Friend). All <= MaxGroupMembers members. Pure + unit-testable. Used whenever
 * UMythicLivingWorldSettings::GroupTemplateDatabase is unset.
 */
namespace MythicGroupDefaults {
    /** Fill Out with the built-in group templates (clears Out first). Safe to call from any thread (no shared state). */
    MYTHIC_API void BuildDefaultTemplates(TArray<FMythicGroupTemplate> &Out);
}
