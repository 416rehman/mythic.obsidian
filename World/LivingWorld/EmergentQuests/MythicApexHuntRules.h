
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/SoftObjectPtr.h"
#include "MythicApexHuntRules.generated.h"

class UItemDefinition;

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicApexHuntSpecies {
    GENERATED_BODY()

    /** The species' bestiary key (Codex.Bestiary.Creature.X) — the FULL-tier knowledge gate reads this page, and the
     *  offer objective's payload filter matches the apex's kill against it (the creature publishes the same stamp). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apex", meta = (Categories = "Codex.Bestiary"))
    FGameplayTag BestiaryKey;

    /** NPC-type tag handed to UMythicNPCManager::SpawnRandomNPC — author the APEX-TIER variant here (the tier boost
     *  IS the NPC type's authored tier; the proven public spawn path does the rest). Unset = the row never offers. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apex")
    FGameplayTag ApexNPCType;

    /** Over-hunting channel queried for the population gate. Unset = the native Pressure.Hunt root (coarse but truthful). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apex", meta = (Categories = "Pressure.Hunt"))
    FGameplayTag HuntPressureChannel;

    /** Trophy item DEF minted through the offer objective's reward on completion (C13 forward contract: inert-but-
     *  tradable now; K's wall consumes Trophy.* later). Unset = loot-only reward. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apex")
    TSoftObjectPtr<UItemDefinition> TrophyItem;

    /** Display name for the offer headline ("The Great {Species}"). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apex")
    FText SpeciesName;
};

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicApexHuntConfig {
    GENERATED_BODY()

    /** Seconds between offer-gate checks (ONE repeating server timer, armed only while the master switch is on). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apex Hunts", meta = (ClampMin = "10.0"))
    float CheckIntervalSeconds = 90.0f;

    /** Pressure.Hunt at/above which the population is UNHEALTHY → no apex offer (conservation gate). <= 0 disables the gate. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apex Hunts", meta = (ClampMin = "0.0"))
    float PopulationPressureThreshold = 5.0f;

    /** Minimum seconds between offers of the SAME species (armed when an offer retires — kill, expiry, or despawn). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apex Hunts", meta = (ClampMin = "0.0"))
    float OfferCooldownSeconds = 1800.0f;

    /** How long an offer (and its live apex) stays current before it retires. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apex Hunts", meta = (ClampMin = "60.0"))
    float OfferLifetimeSeconds = 900.0f;

    /** Apex spawn ring around the offered player (min/max cm) — far enough to make the trail matter. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apex Hunts", meta = (ClampMin = "1000.0"))
    float SpawnMinDistance = 6000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apex Hunts", meta = (ClampMin = "1000.0"))
    float SpawnMaxDistance = 12000.0f;

    /** Spoor trail nodes chained from the hunter toward the apex site (0 = no trail, marker only). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apex Hunts", meta = (ClampMin = "0"))
    int32 TrailNodeCount = 3;

    /** Bestiary kills for the FULL knowledge tier (0 = the codex component's default of 10). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apex Hunts", meta = (ClampMin = "0"))
    int32 KillThresholdFullOverride = 0;

    /** P6i: Pressure.Hunt pushed at the kill site per player creature kill (the over-hunting feed the population gate
     *  + the content-noted spawn-weight decay / predator-raid escalation read). Read even while apex hunts are OFF. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apex Hunts", meta = (ClampMin = "0.0"))
    float HuntPressurePerKill = 1.0f;

    /** The huntable apex species rows (CONTENT — empty ⇒ the subsystem never arms its timer). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apex Hunts")
    TArray<FMythicApexHuntSpecies> Species;
};

struct FMythicApexHuntRules {
    static bool ShouldOfferApexHunt(bool bMasterEnabled, bool bKnowledgeFull, float HuntPressure, float PopulationThreshold,
                                    bool bOfferActiveForSpecies, double SecondsSinceLastOffer, float CooldownSeconds) {
        if (!bMasterEnabled || !bKnowledgeFull || bOfferActiveForSpecies) {
            return false;
        }
        if (PopulationThreshold > 0.0f && HuntPressure >= PopulationThreshold) {
            return false;
        }
        if (SecondsSinceLastOffer >= 0.0 && SecondsSinceLastOffer < FMath::Max(0.0f, CooldownSeconds)) {
            return false;
        }
        return true;
    }
};
