


#include "DestinationComponent.h"


UDestinationComponent::UDestinationComponent() {
	PrimaryComponentTick.bCanEverTick = false;
}


void UDestinationComponent::BeginPlay() {
	Super::BeginPlay();
}


void UDestinationComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                          FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

