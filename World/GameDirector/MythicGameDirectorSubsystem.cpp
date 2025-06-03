// 


#include "MythicGameDirectorSubsystem.h"
#include "Mythic.h"

bool UMythicGameDirectorSubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    // Should only create the subsystem on the server
    UWorld *World = Outer->GetWorld();
    if (World->WorldType != EWorldType::None && World->GetNetMode() < NM_Client) {
        UE_LOG(Mythic, Warning, TEXT("GameDirector created on server"));
        return true;
    }

    UE_LOG(Mythic, Warning, TEXT("GameDirector will not be created on client"));
    return false;
}

void UMythicGameDirectorSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);
}

void UMythicGameDirectorSubsystem::Deinitialize() {
    Super::Deinitialize();
}