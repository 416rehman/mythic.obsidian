//

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "MythicFactionStandingComponent.generated.h"

// Opaque fwd-decl (full definition in World/LivingWorld/Factions/FactionDatabase.h) — used by value below; the .cpp
// that defines FactorForRelation includes the real enum. uint8-backed so it's a complete type for by-value params.
enum class EMythicFactionRelation : uint8;

/**
 * Data-driven faction-politics propagation for a player kill. Killing a faction member doesn't only anger THAT faction
 * — its allies share the grievance and its enemies are quietly pleased. These are fractions of the base kill penalty,
 * applied across the faction-relationship graph so reputation is emergent from politics, not a per-faction island.
 * Allied/Friendly factions LOSE standing (positive factor = same direction as the penalty); Hostile/Unfriendly factions
 * GAIN it (the enemy of my enemy). Neutral factions don't react. All zero = propagation off (pure per-faction behavior).
 */
USTRUCT(BlueprintType)
struct FMythicKillStandingPropagation {
    GENERATED_BODY()

    // Fraction of the kill penalty also lost with factions ALLIED to the victim's faction.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float AlliedFactor = 0.5f;

    // Fraction of the kill penalty also lost with factions FRIENDLY to the victim's faction.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float FriendlyFactor = 0.25f;

    // Fraction of the kill penalty GAINED with factions HOSTILE to the victim's faction (enemy of my enemy).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float HostileFactor = 0.5f;

    // Fraction of the kill penalty GAINED with factions UNFRIENDLY to the victim's faction.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float UnfriendlyFactor = 0.25f;

    // Signed multiplier applied to the (positive) kill penalty: +ve => also lose standing, -ve => gain, 0 => no change.
    float FactorForRelation(EMythicFactionRelation Relation) const;

    // True when every factor is zero — lets the caller skip the whole faction-graph pass.
    bool IsDisabled() const {
        return AlliedFactor == 0.0f && FriendlyFactor == 0.0f && HostileFactor == 0.0f && UnfriendlyFactor == 0.0f;
    }
};

/** Coarse standing tier — drives NPC attitude bands and the player-facing "now Hostile/Friendly" callout. */
UENUM(BlueprintType)
enum class EMythicStandingTier : uint8 {
    Hostile,
    Neutral,
    Friendly,
};

/** One player's standing toward a single faction. Kept in a replicated array (UE replication has no TMap). */
USTRUCT(BlueprintType)
struct FMythicFactionStandingEntry {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FMythicFactionId Faction;

    UPROPERTY(BlueprintReadOnly)
    float Value = 0.0f;
};

/**
 * Per-player, server-authoritative faction standing. Lives on AMythicPlayerState. Standing toward a faction
 * starts at 0 (neutral); negative = disliked, positive = liked. NPC attitude (AMythicAIController) reads the
 * asking player's standing toward the NPC's faction to decide Hostile / Friendly, layered on the global
 * faction relations. The standing array replicates to clients for UI.
 */
UCLASS(ClassGroup = (Mythic), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicFactionStandingComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicFactionStandingComponent();

    // Current standing toward Faction (0 if none recorded yet).
    UFUNCTION(BlueprintPure, Category = "Faction Standing")
    float GetStanding(FMythicFactionId Faction) const;

    // SERVER: add Delta to standing toward Faction (clamped to [MinStanding, MaxStanding]).
    UFUNCTION(BlueprintCallable, Category = "Faction Standing")
    void ServerAdjustStanding(FMythicFactionId Faction, float Delta);

    // SERVER: apply the full reputation consequence of killing a member of VictimFaction — the direct penalty to that
    // faction PLUS data-driven propagation across the faction-relationship graph (allies resent it, enemies approve).
    // Single entry point for a kill's standing impact so callers needn't know about faction politics or the faction DB.
    UFUNCTION(BlueprintCallable, Category = "Faction Standing")
    void ServerApplyKillStanding(FMythicFactionId VictimFaction);

    // SERVER: set standing toward Faction to NewValue (clamped).
    UFUNCTION(BlueprintCallable, Category = "Faction Standing")
    void SetStanding(FMythicFactionId Faction, float NewValue);

    UFUNCTION(BlueprintPure, Category = "Faction Standing")
    float GetHostileThreshold() const { return HostileStandingThreshold; }

    UFUNCTION(BlueprintPure, Category = "Faction Standing")
    float GetFriendlyThreshold() const { return FriendlyStandingThreshold; }

    // The standing tier (Hostile/Neutral/Friendly) for a raw standing value, per the configured thresholds.
    UFUNCTION(BlueprintPure, Category = "Faction Standing")
    EMythicStandingTier TierForStanding(float Value) const;

    UFUNCTION(BlueprintPure, Category = "Faction Standing")
    float GetKillStandingPenalty() const { return KillStandingPenalty; }

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    // Per-faction standing values (replicated to the owning client for UI). Capped, small set -> linear scan.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Faction Standing")
    TArray<FMythicFactionStandingEntry> Standings;

    // Standing at or below this => NPCs of that faction treat the player as Hostile.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Faction Standing")
    float HostileStandingThreshold = -50.0f;

    // Standing at or above this => NPCs of that faction treat the player as Friendly.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Faction Standing")
    float FriendlyStandingThreshold = 50.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Faction Standing")
    float MinStanding = -100.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Faction Standing")
    float MaxStanding = 100.0f;

    // How much standing a player loses with a faction for killing one of its members.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Faction Standing")
    float KillStandingPenalty = 25.0f;

    // How that kill penalty ripples across the faction-relationship graph (allies of the victim's faction also resent
    // it; its enemies approve). Designer-tunable; all-zero disables propagation. See ServerApplyKillStanding.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Faction Standing")
    FMythicKillStandingPropagation KillStandingPropagation;

private:
    // Find the entry index for Faction, or INDEX_NONE.
    int32 FindEntryIndex(FMythicFactionId Faction) const;

    // SERVER: if a standing mutation crossed a tier boundary (Hostile/Neutral/Friendly), resolve the faction's display
    // name and notify the owning client so the player SEES their reputation shift. No-op when the tier is unchanged.
    void NotifyStandingTierChange(FMythicFactionId Faction, float OldValue, float NewValue);

    // CLIENT (owning): float a "<Faction> now Hostile/Friendly/Neutral" callout over the local player's pawn.
    UFUNCTION(Client, Reliable)
    void ClientNotifyStandingTier(const FString &FactionName, EMythicStandingTier NewTier);
};
