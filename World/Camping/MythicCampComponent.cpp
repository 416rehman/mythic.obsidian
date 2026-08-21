
#include "World/Camping/MythicCampComponent.h"

#include "World/Camping/MythicCampsiteSubsystem.h"
#include "World/Camping/MythicCampfireComponent.h"
#include "Player/MythicPlayerState.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

UMythicCampComponent::UMythicCampComponent() {
    PrimaryComponentTick.bCanEverTick = false;
}

void UMythicCampComponent::BeginPlay() {
    Super::BeginPlay();

    AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }

    if (!bCampAnchor && Owner->FindComponentByClass<UMythicCampfireComponent>()) {
        bCampAnchor = true;
    }

    ResolveOwnerPlayerKey();

    if (UWorld *World = GetWorld()) {
        if (UMythicCampsiteSubsystem *Camps = World->GetSubsystem<UMythicCampsiteSubsystem>()) {
            Camps->RegisterPiece(this);
        }
        if (!OwnerPlayerKey.IsEmpty() && Owner) {
            {
            }
        }
    }
}

void UMythicCampComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (GetOwner() && GetOwner()->HasAuthority()) {
        if (UWorld *World = GetWorld()) {
            if (UMythicCampsiteSubsystem *Camps = World->GetSubsystem<UMythicCampsiteSubsystem>()) {
                Camps->UnregisterPiece(this);
            }
        }
    }
    Super::EndPlay(EndPlayReason);
}

void UMythicCampComponent::ResolveOwnerPlayerKey() {
    OwnerPlayerKey.Reset();
    const AActor *Owner = GetOwner();
    const APawn *InstigatorPawn = Owner ? Owner->GetInstigator() : nullptr;
    if (!InstigatorPawn) {
        return;
    }
    if (const AMythicPlayerState *PS = InstigatorPawn->GetPlayerState<AMythicPlayerState>()) {
        OwnerPlayerKey = PS->GetCanonicalPlayerKey();
    }
}
