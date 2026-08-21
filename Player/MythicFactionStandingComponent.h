
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "MythicFactionStandingComponent.generated.h"

enum class EMythicFactionRelation : uint8;

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

    float FactorForRelation(EMythicFactionRelation Relation) const;

    bool IsDisabled() const {
        return AlliedFactor == 0.0f && FriendlyFactor == 0.0f && HostileFactor == 0.0f && UnfriendlyFactor == 0.0f;
    }
};

UENUM(BlueprintType)
enum class EMythicStandingTier : uint8 {
    Hostile,
    Neutral,
    Friendly,
};

USTRUCT(BlueprintType)
struct FMythicFactionStandingEntry {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FMythicFactionId Faction;

    UPROPERTY(BlueprintReadOnly)
    float Value = 0.0f;
};

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

    const TArray<FMythicFactionStandingEntry> &GetStandings() const { return Standings; }

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
    int32 FindEntryIndex(FMythicFactionId Faction) const;

    void NotifyStandingTierChange(FMythicFactionId Faction, float OldValue, float NewValue);

    UFUNCTION(Client, Reliable)
    void ClientNotifyStandingTier(const FString &FactionName, EMythicStandingTier NewTier);
};
