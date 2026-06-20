//

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "MythicFactionStandingComponent.generated.h"

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
