#include "World/Entity/MythicEntityPresentationTypes.h"

#include "World/Entity/MythicEntityPresentationComponent.h"

FMythicEntityInstanceHandle::FMythicEntityInstanceHandle(
    const FMythicEntityPresentationInstance &InInstance,
    UMythicEntityPresentationComponent *InComponent)
    : Instance(InInstance), Component(InComponent) {}

bool FMythicEntityInstanceHandle::IsValid() const {
    const UMythicEntityPresentationComponent *ResolvedComponent =
        Component.Get();
    return Instance.IsValid() && ResolvedComponent
           && ResolvedComponent->RepresentsInstance(Instance);
}
