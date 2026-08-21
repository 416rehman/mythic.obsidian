
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Mass/EntityHandle.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "CognitiveTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMythCognition, Log, All);


UENUM(BlueprintType)
enum class EMythicDesireType : uint8 {
    Survive UMETA(DisplayName = "Survive"),

    Defend UMETA(DisplayName = "Defend"),

    Avenge UMETA(DisplayName = "Avenge"),

    Patrol UMETA(DisplayName = "Patrol"),

    Trade UMETA(DisplayName = "Trade"),

    Socialize UMETA(DisplayName = "Socialize"),

    JoinPlayer UMETA(DisplayName = "JoinPlayer"),

    Flee UMETA(DisplayName = "Flee"),

    Rest UMETA(DisplayName = "Rest"),

    Exploit UMETA(DisplayName = "Exploit"),

    Rally UMETA(DisplayName = "Rally"),

    Report UMETA(DisplayName = "Report"),

    FollowSchedule UMETA(DisplayName = "FollowSchedule"),

    COUNT UMETA(Hidden)
};

static constexpr int32 DesireTypeCount = static_cast<int32>(EMythicDesireType::COUNT);


USTRUCT()
struct FMythicBelief {
    GENERATED_BODY()

    FGameplayTag EventTag;

    FMythicCellCoord Cell;

    FMythicFactionId InvolvedFaction;

    float Confidence = 1.0f;

    double FormationTime = 0.0;

    double LastDecayTime = 0.0;

    uint8 PropagationHops = 0;

    uint32 SourceEventId = 0;
};

static constexpr int32 MaxBeliefsPerNPC = 16;


USTRUCT()
struct FMythicDesire {
    GENERATED_BODY()

    EMythicDesireType Type = EMythicDesireType::FollowSchedule;

    float Utility = 0.0f;

    FMassEntityHandle TargetEntity;

    FMythicCellCoord TargetCell;

    uint32 SourceEventId = 0;
};


USTRUCT()
struct FMythicIntention {
    GENERATED_BODY()

    FMythicDesire Desire;

    double CommitTime = 0.0;

    float TimeoutSeconds = 30.0f;

    bool bStarted = false;

    bool bValid = false;

    void Reset() {
        bValid = false;
        bStarted = false;
        Desire = FMythicDesire();
    }
};
