
#pragma once

#include "CoreMinimal.h"
#include "Mass/EntityHandle.h"
#include "GameplayTagContainer.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Morality/MoralSignature.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "ActionEventTypes.generated.h"


USTRUCT(BlueprintType)
struct MYTHIC_API FMythicActionEvent {
    GENERATED_BODY()

    UPROPERTY()
    TWeakObjectPtr<AActor> Perpetrator;

    UPROPERTY()
    TWeakObjectPtr<AActor> Victim;

    FMythicMoralAction MoralVector;

    /** Gameplay tag describing the action (e.g. "Action.Combat.MeleeKill") */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Event", meta=(Categories="World.Action"))
    FGameplayTag ActionTag;

    /** Category for magic/ability classification (maps to moral axes) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Event")
    EMythicActionCategory ActionCategory = EMythicActionCategory::Melee;

    uint16 CategoryFlags = 0;

    /** How significant this event is [0.0, 1.0]. Affects propagation priority and significance scoring. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Event", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Significance = 0.5f;

    FMythicCellCoord OverrideCell = FMythicCellCoord(-1, -1);

    FMythicFactionId PerpFactionOverride;
    FMythicFactionId VictimFactionOverride;

    FString PerpPlayerKey;

    uint8 VisibilityGroup = 0;

    float StealthPerceptionScale = 1.0f;
};


struct FMythicWitnessResult {
    FMassEntityHandle WitnessEntity;

    EMythicMoralSeverity Severity = EMythicMoralSeverity::Ignore;

    FMythicMoralAction ActionMoralVector;

    uint16 EventCategoryFlags = 0;

    float EventSignificance = 0.0f;

    double EventWorldTime = 0.0;

    FMythicCellCoord EventCell;

    FMythicFactionId PerpFaction;
};


struct FMythicPendingActionEvent {
    FMythicWorldEvent WorldEvent;

    int32 WitnessesProcessed = 0;

    bool bFullyProcessed = false;

    FString PerpPlayerKey;

    float StealthPerceptionScale = 1.0f;
};
