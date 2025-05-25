// 


#include "MythicAIController.h"

#include "Mythic.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GameModes/GameState/MythicGameState.h"
#include "Net/UnrealNetwork.h"
#include "Player/MythicNPCCharacter.h"
#include "Player/MythicPlayerState.h"


// Sets default values
AMythicAIController::AMythicAIController() {
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;
}

void AMythicAIController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(AMythicAIController, Faction);
    DOREPLIFETIME(AMythicAIController, Name);
}

void AMythicAIController::BeginPlay() {
    Super::BeginPlay();

    AMythicGameState *GameState = GetWorld()->GetGameState<AMythicGameState>();
    if (!GameState) {
        UE_LOG(Mythic, Error, TEXT("GameState not found"));
        return;
    }

    UMythicAbilitySystemComponent *MythicASC = GameState->GetMythicAbilitySystemComponent();
    if (!MythicASC) {
        UE_LOG(Mythic, Error, TEXT("MythicASC not found"));
        return;
    }
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
