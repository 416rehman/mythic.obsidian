// Mythic Living World — Creature species×species aggression matrix
// Schema for UMythicLivingWorldSettings::CreatureAggressionMatrix. One row per ordered (Attacker, Target) species pair
// giving the attacker's aggression toward the target [0,1]. Sparse: any unlisted pair defaults to "indifferent" (0).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CreatureAggressionTypes.generated.h"

/**
 * One directed entry of the creature aggression matrix: how aggressive species `AttackerSpeciesId` is toward species
 * `TargetSpeciesId`, in [0,1]. Directed (predator->prey is high, prey->predator is low/0), so list each ordered pair
 * you care about. Unlisted pairs are treated as 0 (no inter-species aggression). SpeciesIds match
 * FMythicCreatureSpeciesRow.SpeciesId. Pure data — the lookup + behavioral effect live in CreatureEcologyProcessor.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicCreatureAggressionRow : public FTableRowBase {
    GENERATED_BODY()

    /** The aggressor species (FMythicCreatureSpeciesRow.SpeciesId). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature")
    uint8 AttackerSpeciesId = 0;

    /** The target species this row scores aggression toward. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature")
    uint8 TargetSpeciesId = 0;

    /** Aggression of attacker toward target [0,1]. 0 = ignores, 1 = always hostile. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Aggression = 0.0f;
};

/**
 * Flattened, lookup-friendly view of the aggression matrix built ONCE from the DataTable (or left empty when no table
 * is authored). Keyed by a packed (Attacker<<8 | Target) uint16 so a per-tick lookup is a single TMap::Find with no
 * string/row-name hashing. Built by the consumer (CreatureEcologyProcessor) and cached for the matrix's lifetime.
 */
struct FMythicCreatureAggressionMatrix {
    /** Packed key -> aggression. Empty => every cross-species lookup returns the default (0). */
    TMap<uint16, float> Entries;

    static uint16 PackKey(uint8 Attacker, uint8 Target) {
        return static_cast<uint16>((static_cast<uint16>(Attacker) << 8) | static_cast<uint16>(Target));
    }

    /** Aggression of Attacker toward Target, or 0 if the pair isn't listed. */
    float Get(uint8 Attacker, uint8 Target) const {
        if (const float *Found = Entries.Find(PackKey(Attacker, Target))) {
            return *Found;
        }
        return 0.0f;
    }

    bool IsEmpty() const { return Entries.Num() == 0; }
};
