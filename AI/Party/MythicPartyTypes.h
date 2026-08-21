
#pragma once

#include "CoreMinimal.h"
#include "Mass/EntityHandle.h"
#include "AI/Cognition/CognitiveTypes.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "MythicPartyTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMythParty, Log, All);

class AMythicNPCCharacter;

UENUM()
enum class EMythicCompanionRebuildState : uint8 {
    Bound UMETA(DisplayName = "Bound"),
    NotCreated UMETA(DisplayName = "NotCreated"),
    EntityCreated UMETA(DisplayName = "EntityCreated")
};


USTRUCT()
struct FMythicPartyMember {
    GENERATED_BODY()

    TWeakObjectPtr<AMythicNPCCharacter> NPCActor;

    FMassEntityHandle SourceEntity;

    FMythicFactionId OriginalFaction;

    float LoyaltyScore = 0.5f;

    float BetrayalPressure = 0.0f;

    TArray<FMythicBelief> SharedBeliefs;

    double JoinTime = 0.0;

    bool bInRestPhase = false;

    FText CachedDisplayName;

    uint32 PersistedNameHash = 0;
    FMythicFactionId PersistedTrueFaction;
    FMythicCellCoord PersistedSpawnCell;
    FGameplayTag PersistedRoleTag;

    EMythicCompanionRebuildState RebuildState = EMythicCompanionRebuildState::Bound;
};


UENUM(BlueprintType)
enum class EMythicCompanionOrder : uint8 {
    Follow UMETA(DisplayName = "Follow"),

    Hold UMETA(DisplayName = "Hold"),

    MoveTo UMETA(DisplayName = "Move To"),

    AttackTarget UMETA(DisplayName = "Attack Target")
};


UENUM(BlueprintType)
enum class EMythicPartyEventType : uint8 {
    PlayerActionWitnessed UMETA(DisplayName = "PlayerActionWitnessed"),

    RestPhaseStarted UMETA(DisplayName = "RestPhaseStarted"),

    SharedCombat UMETA(DisplayName = "SharedCombat"),

    ForcedCompliance UMETA(DisplayName = "ForcedCompliance"),

    FactionEventWitnessed UMETA(DisplayName = "FactionEventWitnessed")
};
