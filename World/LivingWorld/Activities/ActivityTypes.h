// Mythic Living World — Context-driven Activity Catalog (data-driven WHAT an idle NPC does)
// An "activity" is a contextual ambient action an embodied, non-combat NPC performs when its committed routine intention
// leaves it free (a fisher casting a line by the water, a merchant bartering, a guard patrolling, a townsperson
// socialising). The catalog is a strict DATA + PURE-SELECTION layer: it answers "given this NPC's role/biome/time/phase/
// surroundings, which contextual activity should it pick?" — it owns NO movement, NO replication, NO behavior. The AI
// controller (AMythicAIController::TickActivityBehavior) consumes the chosen FMythicActivityDef to steer the NPC and
// fires the cosmetic OnPerformActivity Blueprint hook; the selection itself is a pure, deterministic function so it is
// unit-testable without a live world (mirrors UMythicGroupSpawnerProcessor's PickTemplateIndex + GroupTypes data layer).

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "World/LivingWorld/Territory/MythicBiome.h" // EMythicBiome (eligible-biome gate)
#include "ActivityTypes.generated.h"

enum class EMythicSchedulePhase : uint8; // FMythicScheduleFragment phase (forward-decl — only used by-value in the plain context struct)

// ─────────────────────────────────────────────────────────────
// Activity gating enums
// ─────────────────────────────────────────────────────────────

/** When (relative to day/night) an activity may run. Any = no time gate. */
UENUM(BlueprintType)
enum class EMythicActivityTimeWindow : uint8 {
    Any = 0 UMETA(DisplayName = "Any"),
    Day UMETA(DisplayName = "Day Only"),
    Night UMETA(DisplayName = "Night Only")
};

/**
 * Which committed SCHEDULE phase an activity is appropriate for. Any = no phase gate. Maps onto the NPC's cached schedule
 * phase (FMythicScheduleFragment.Phase) via a coarse grouping (Work/Rest/Social), so an activity tagged Work only surfaces
 * while the NPC is in its working part of the day. Kept SEPARATE from EMythicSchedulePhase (which has Travel/Idle the
 * activity catalog never gates on) so authoring stays a small, intentional set.
 */
UENUM(BlueprintType)
enum class EMythicActivitySchedulePhase : uint8 {
    Any = 0 UMETA(DisplayName = "Any"),
    Work UMETA(DisplayName = "Work"),
    Rest UMETA(DisplayName = "Rest"),
    Social UMETA(DisplayName = "Social")
};

/**
 * Where the chosen activity steers the NPC. The AI controller resolves each kind to a concrete world location at
 * dispatch time (HomeCell/WorkCell from the brain, CurrentCell = stand-and-emote in place, SettlementCenter via the
 * settlement reverse-index, NearbyMerchant via a bounded actor scan, BiomeWander = a short scatter in the current cell).
 */
UENUM(BlueprintType)
enum class EMythicActivityTargetKind : uint8 {
    HomeCell = 0 UMETA(DisplayName = "Home Cell"),
    WorkCell UMETA(DisplayName = "Work Cell"),
    CurrentCell UMETA(DisplayName = "Current Cell (in place)"),
    SettlementCenter UMETA(DisplayName = "Settlement Center"),
    NearbyMerchant UMETA(DisplayName = "Nearby Merchant"),
    BiomeWander UMETA(DisplayName = "Biome Wander")
};

// ─────────────────────────────────────────────────────────────
// Activity Definition — one authored/code-default contextual action
// ─────────────────────────────────────────────────────────────

/**
 * One contextual activity recipe: an identifying tag, the contexts it is eligible in (role/biome/time/phase/merchant/
 * water gates), where it steers the NPC, and a relative weight for the eligible-weighted pick. Pure data — the gating
 * predicate (MythicActivityDefaults::ActivityEligible) and the weighted picker (PickActivityIndex) live free + static
 * so they are unit-testable. Mirrors FMythicGroupTemplate's data+gate shape.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicActivityDef {
    GENERATED_BODY()

    /** Identifies this activity (e.g. "NPC.Activity.Fish"). Also handed to the cosmetic OnPerformActivity BP hook. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity", meta = (Categories = "NPC.Activity"))
    FGameplayTag ActivityTag;

    /** Roles this activity is appropriate for (exact-or-child match). Empty = any role. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity", meta = (Categories = "NPC.Role"))
    FGameplayTagContainer EligibleRoles;

    /** Biomes this activity is appropriate for. Empty = any biome. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity")
    TArray<EMythicBiome> EligibleBiomes;

    /** Require the NPC's current biome to be water-adjacent (v1 honest stub: Biome == Wetland). For fishing. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity")
    uint8 bRequiresWaterAdjacent : 1;

    /** Day/night gate. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity")
    EMythicActivityTimeWindow TimeWindow = EMythicActivityTimeWindow::Any;

    /** Committed-schedule-phase gate. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity")
    EMythicActivitySchedulePhase RequiredPhase = EMythicActivitySchedulePhase::Any;

    /** Where this activity steers the NPC. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity")
    EMythicActivityTargetKind TargetKind = EMythicActivityTargetKind::CurrentCell;

    /** Require a merchant NPC nearby (the controller's bounded ScanNearbyMerchant must have found one). For bartering. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity")
    uint8 bRequiresNearbyMerchant : 1;

    /** Relative weight among eligible activities in the weighted pick. 0 = effectively disabled. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity", meta = (ClampMin = "0.0"))
    float RelativeWeight = 1.0f;

    /** Human-readable label (debug/UI only). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity")
    FText DisplayName;

    // Bitfield members require an explicit ctor to default-initialize (mirrors FMythicGroupFragment::bIsLeader).
    FMythicActivityDef() : bRequiresWaterAdjacent(0), bRequiresNearbyMerchant(0) {}
};

// ─────────────────────────────────────────────────────────────
// Activity Context — live, lock-free inputs gathered at dispatch
// ─────────────────────────────────────────────────────────────

/**
 * The live context the eligibility/selection helpers read. PLAIN struct (NOT reflected) — it is built transiently on the
 * game thread inside the AI controller's idle dispatch from already-lock-free sources (brain role/cached phase,
 * GetBiomeAtCell, the resolved game hour, a bounded merchant scan, the identity NameHash) and never stored/replicated.
 */
struct FMythicActivityContext {
    /** The NPC's role tag (UMythicCognitiveBrainComponent::GetRole). */
    FGameplayTag Role;

    /** The NPC's CURRENT biome (UMythicTerritoryGrid::GetBiomeAtCell of its live cell). */
    EMythicBiome Biome = EMythicBiome::Plains;

    /** True when the resolved game hour is daytime (AMythicAIController::IsDayHour). */
    bool bIsDay = true;

    /** The NPC's committed schedule phase (brain cached, copied from FMythicScheduleFragment). */
    EMythicSchedulePhase Phase{};

    /** True when the controller's bounded merchant scan found a merchant within range. */
    bool bHasNearbyMerchant = false;

    /** Stable per-NPC hash (FMythicIdentityFragment.NameHash) — drives the deterministic weighted pick tie-break. */
    uint32 NameHash = 0;
};

/** Result of a selection pass. PLAIN struct. ChosenIndex indexes into the activity array (INDEX_NONE = none eligible). */
struct FMythicActivityResult {
    int32 ChosenIndex = INDEX_NONE;
};

// ─────────────────────────────────────────────────────────────
// Activity Catalog — designer-authored data asset
// ─────────────────────────────────────────────────────────────

/**
 * Designer-authored catalog of activity defs. Referenced from DA_LivingWorldSettings → ActivityCatalog. When unset, the
 * AI controller falls back to MythicActivityDefaults::BuildDefaultActivities() so the system runs unauthored. Mirrors
 * UMythicGroupTemplateDatabase / UMythicArchetypeCatalog.
 */
UCLASS(BlueprintType)
class MYTHIC_API UMythicActivityCatalog : public UDataAsset {
    GENERATED_BODY()

public:
    /** All activity defs in this catalog. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Activities")
    TArray<FMythicActivityDef> Activities;

    /** Find an activity by its tag (exact match). Returns nullptr if not present. Mirrors UMythicGroupTemplateDatabase::FindByTag. */
    const FMythicActivityDef *FindByTag(const FGameplayTag &ActivityTag) const {
        for (const FMythicActivityDef &A : Activities) {
            if (A.ActivityTag.MatchesTagExact(ActivityTag)) {
                return &A;
            }
        }
        return nullptr;
    }
};

// ─────────────────────────────────────────────────────────────
// Code defaults + pure selection helpers
// ─────────────────────────────────────────────────────────────

/**
 * Built-in activities so context-driven idle behavior runs with ZERO authored data: Fish (Fisher, wetland, day),
 * Barter (any townsperson near a merchant), Work (Work phase, work cell), Rest (Rest phase, home), Socialize (Social
 * phase, settlement center), Patrol (Guard/Soldier), and an always-eligible Wander fallback. All pure + unit-testable;
 * used whenever UMythicLivingWorldSettings::ActivityCatalog is unset.
 */
namespace MythicActivityDefaults {
    /** Fill Out with the built-in activity defs (clears Out first). Safe to call from any thread (no shared state). */
    MYTHIC_API void BuildDefaultActivities(TArray<FMythicActivityDef> &Out);

    /**
     * Pure gate: is Def appropriate in Ctx? Checks role (empty or matching), biome (empty or contains Ctx.Biome),
     * water-adjacency (Ctx.Biome == Wetland), day/night, committed phase, and the nearby-merchant requirement. NO engine
     * state — call-site-agnostic + unit-testable.
     */
    MYTHIC_API bool ActivityEligible(const FMythicActivityDef &Def, const FMythicActivityContext &Ctx);

    /**
     * Pure weighted pick over the ELIGIBLE subset, deterministic from Ctx.NameHash (large-prime modulus tie-break,
     * mirroring ScheduleTransitionProcessor::ComputeStaggeredHour + GroupSpawnerProcessor::PickTemplateIndex). Returns
     * the chosen index into Activities, or INDEX_NONE when nothing is eligible / total weight is ~0. Allocation-free.
     */
    MYTHIC_API int32 PickActivityIndex(TConstArrayView<FMythicActivityDef> Activities, const FMythicActivityContext &Ctx);
}
