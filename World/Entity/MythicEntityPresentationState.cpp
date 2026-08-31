#include "World/Entity/MythicEntityPresentationState.h"

#include "World/Entity/MythicEntityPresentationComponent.h"

void FMythicObservableFactArray::PostReplicatedReceive(
    const FFastArraySerializer::FPostReplicatedReceiveParameters & /*Parameters*/) {
    if (Owner) {
        Owner->HandleReplicatedFactsReceived();
    }
}

void FMythicPublicStatusPresentationArray::PostReplicatedReceive(
    const FFastArraySerializer::FPostReplicatedReceiveParameters & /*Parameters*/) {
    if (Owner) {
        Owner->HandleReplicatedStatusesReceived();
    }
}
