
#include "World/Placement/MythicProxyRegistrationComponent.h"

#include "World/Placement/MythicPlacedProxySubsystem.h"
#include "Mythic/Mythic.h"
#include "GameFramework/Actor.h"

UMythicProxyRegistrationComponent::UMythicProxyRegistrationComponent() {
    PrimaryComponentTick.bCanEverTick = false;
}

void UMythicProxyRegistrationComponent::BeginPlay() {
    Super::BeginPlay();

    AActor *Owner = GetOwner();
    if (!bEnabled || !ProxyType.IsValid() || !Owner) {
        return;
    }
    if (!Owner->HasAuthority()) {
        return;
    }

    UMythicPlacedProxySubsystem *Registry = UMythicPlacedProxySubsystem::Get(this);
    if (!Registry) {
        return;
    }

    if (Registry->IsPromotionInProgress()) {
        return;
    }

    const FGuid Id = Registry->AdoptPlacedActor(Owner, ProxyType, InitialStateFlags);
    if (!Id.IsValid()) {
        UE_LOG(Myth, Warning, TEXT("ProxyRegistration: '%s' could not be adopted; leaving it as a live actor."),
               *Owner->GetName());
        return;
    }

    Owner->Destroy();
}
