
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "CreatureAggressionTypes.generated.h"

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

struct FMythicCreatureAggressionMatrix {
    TMap<uint16, float> Entries;

    static uint16 PackKey(uint8 Attacker, uint8 Target) {
        return static_cast<uint16>((static_cast<uint16>(Attacker) << 8) | static_cast<uint16>(Target));
    }

    float Get(uint8 Attacker, uint8 Target) const {
        if (const float *Found = Entries.Find(PackKey(Attacker, Target))) {
            return *Found;
        }
        return 0.0f;
    }

    bool IsEmpty() const { return Entries.Num() == 0; }
};
