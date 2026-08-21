
#include "World/LivingWorld/MythicWorldStateSubsystem.h"

#include "Mythic.h"
#include "Engine/World.h"

bool UMythicWorldStateSubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    const UWorld *World = Cast<UWorld>(Outer);
    if (!World || !World->IsGameWorld()) {
        return false;
    }
    return World->GetNetMode() != NM_Client;
}

void UMythicWorldStateSubsystem::ServerSetFlag(FGameplayTag Flag) {
    const UWorld *World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client || !Flag.IsValid()) {
        return;
    }
    if (WorldFlags.HasTagExact(Flag)) {
        return;
    }
    WorldFlags.AddTag(Flag);
    UE_LOG(Myth, Log, TEXT("WorldState: raised world flag %s"), *Flag.ToString());
}

void UMythicWorldStateSubsystem::ServerClearFlag(FGameplayTag Flag) {
    const UWorld *World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client || !Flag.IsValid()) {
        return;
    }
    if (WorldFlags.RemoveTag(Flag)) {
        UE_LOG(Myth, Log, TEXT("WorldState: cleared world flag %s"), *Flag.ToString());
    }
}
