// 


#include "MythicAIController.h"
#include "MythicNPCCharacter.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Player/MythicPlayerState.h"

AMythicAIController::AMythicAIController() {
    // AI Controllers don't replicate in multiplayer
    bReplicates = false;
}

void AMythicAIController::BeginPlay() {
    Super::BeginPlay();
}

UAbilitySystemComponent *AMythicAIController::GetAbilitySystemComponent() const {
    // If controlled pawn is MythicNPCCharacter, return their AbilitySystemComponent
    if (const AMythicNPCCharacter *MythicNPCCharacter = Cast<AMythicNPCCharacter>(GetPawn())) {
        return MythicNPCCharacter->GetAbilitySystemComponent();
    }

    return nullptr;
}

// TODO: Implement this function
ETeamAttitude::Type AMythicAIController::GetTeamAttitudeTowards(const AActor &Other) const {
    // Against other NPCs, do a quick faction check.
    if (const AMythicAIController *OtherAIController = Cast<AMythicAIController>(&Other)) {}

    if (const AMythicPlayerState *playerState = Cast<AMythicPlayerState>(&Other)) {}


    return ETeamAttitude::Neutral;
}
