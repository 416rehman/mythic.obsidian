
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "DesignerSpawnerTypes.generated.h"


UENUM(BlueprintType)
enum class EMythicDesignerFactionStatePredicate : uint8 {
    Any = 0     UMETA(DisplayName = "Any (ignore faction state)"),
    Alive       UMETA(DisplayName = "Alive (status == Active)"),
    Annihilated UMETA(DisplayName = "Annihilated"),
    Resistance  UMETA(DisplayName = "Resistance"),
    Dormant     UMETA(DisplayName = "Dormant")
};

UENUM(BlueprintType)
enum class EMythicDesignerRelationPredicate : uint8 {
    Ignore = 0 UMETA(DisplayName = "Ignore (no relation gate)"),
    AtWar      UMETA(DisplayName = "At War (Hostile)"),
    AtPeace    UMETA(DisplayName = "At Peace (not Hostile)")
};


USTRUCT(BlueprintType)
struct FMythicTimeWindow {
    GENERATED_BODY()

    /** When false, the time window imposes no gate (always satisfied). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Spawner")
    bool bEnabled = false;

    /** Inclusive window start, game hours [0,24]. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Spawner", meta = (ClampMin = "0.0", ClampMax = "24.0", EditCondition = "bEnabled"))
    float StartHour = 0.0f;

    /** Exclusive window end, game hours [0,24]. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Spawner", meta = (ClampMin = "0.0", ClampMax = "24.0", EditCondition = "bEnabled"))
    float EndHour = 24.0f;
};


USTRUCT(BlueprintType)
struct FMythicDesignerConditionSet {
    GENERATED_BODY()

    /** Time-of-day gate. Disabled by default. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Spawner|Conditions")
    FMythicTimeWindow TimeWindow;

    /** A player must own ALL of these gameplay tags. Empty => no player-tag gate. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Spawner|Conditions")
    FGameplayTagContainer RequiredPlayerTags;

    /** When true, at least one player must be within PlayerRangeCm of the spawner (in addition to any tag gate). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Spawner|Conditions")
    bool bRequireAnyPlayerInRange = false;

    /** Proximity radius (cm) for bRequireAnyPlayerInRange and as the search radius for the player-tag gate. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Spawner|Conditions", meta = (ClampMin = "0.0"))
    float PlayerRangeCm = 10000.0f;

    /** The faction whose lifecycle state gates the spawn. Empty => no faction-state gate. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Spawner|Conditions")
    FGameplayTag GatingFactionTag;

    /** Required lifecycle state of GatingFactionTag. `Any` ignores the gate. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Spawner|Conditions")
    EMythicDesignerFactionStatePredicate FactionState = EMythicDesignerFactionStatePredicate::Any;

    /** First faction of the relation gate. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Spawner|Conditions")
    FGameplayTag RelationFactionA;

    /** Second faction of the relation gate. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Spawner|Conditions")
    FGameplayTag RelationFactionB;

    /** Required relation between A and B. `Ignore` disables the gate. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Spawner|Conditions")
    EMythicDesignerRelationPredicate Relation = EMythicDesignerRelationPredicate::Ignore;
};


struct FMythicDesignerConditionInputs {
    float GameHour = 0.0f;

    bool bAnyPlayerSatisfiesTags = false;

    EMythicFactionStatus GatingFactionStatus = EMythicFactionStatus::Dormant;

    bool bGatingFactionResolved = false;

    EMythicFactionRelation RelationAB = EMythicFactionRelation::Neutral;

    bool bRelationResolved = false;
};


USTRUCT()
struct FMythicDesignerSpawnerState {
    GENERATED_BODY()

    UPROPERTY()
    int32 SpawnsEver = 0;

    UPROPERTY()
    bool bPermaDead = false;

    UPROPERTY()
    double LastDeathTime = 0.0;
};


namespace MythicDesignerSpawner {
inline bool IsHourInWindow(const FMythicTimeWindow& W, float GameHour) {
    if (!W.bEnabled) {
        return true;
    }
    if (W.StartHour <= W.EndHour) {
        return GameHour >= W.StartHour && GameHour < W.EndHour;
    }
    return GameHour >= W.StartHour || GameHour < W.EndHour;
}

inline EMythicFactionStatus DesignerStateToStatus(EMythicDesignerFactionStatePredicate P) {
    switch (P) {
    case EMythicDesignerFactionStatePredicate::Alive:       return EMythicFactionStatus::Active;
    case EMythicDesignerFactionStatePredicate::Annihilated: return EMythicFactionStatus::Annihilated;
    case EMythicDesignerFactionStatePredicate::Resistance:  return EMythicFactionStatus::Resistance;
    case EMythicDesignerFactionStatePredicate::Dormant:     return EMythicFactionStatus::Dormant;
    default:                                                 return EMythicFactionStatus::Active;
    }
}

inline bool EvaluateConditions(const FMythicDesignerConditionSet& C, const FMythicDesignerConditionInputs& In) {
    if (!IsHourInWindow(C.TimeWindow, In.GameHour)) {
        return false;
    }

    const bool bPlayerGateActive = !C.RequiredPlayerTags.IsEmpty() || C.bRequireAnyPlayerInRange;
    if (bPlayerGateActive && !In.bAnyPlayerSatisfiesTags) {
        return false;
    }

    if (C.FactionState != EMythicDesignerFactionStatePredicate::Any && C.GatingFactionTag.IsValid()) {
        if (!In.bGatingFactionResolved) {
            return false;
        }
        if (In.GatingFactionStatus != DesignerStateToStatus(C.FactionState)) {
            return false;
        }
    }

    if (C.Relation != EMythicDesignerRelationPredicate::Ignore &&
        C.RelationFactionA.IsValid() && C.RelationFactionB.IsValid()) {
        if (!In.bRelationResolved) {
            return false;
        }
        const bool bHostile = (In.RelationAB == EMythicFactionRelation::Hostile);
        if (C.Relation == EMythicDesignerRelationPredicate::AtWar && !bHostile) {
            return false;
        }
        if (C.Relation == EMythicDesignerRelationPredicate::AtPeace && bHostile) {
            return false;
        }
    }

    return true;
}
}
