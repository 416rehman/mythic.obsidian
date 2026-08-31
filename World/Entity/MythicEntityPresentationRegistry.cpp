#include "World/Entity/MythicEntityPresentationRegistry.h"

#include "Engine/World.h"
#include "World/Entity/MythicEntityPresentationComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogMythicEntityPresentationRegistry, Log, All);

bool UMythicEntityPresentationRegistry::ShouldCreateSubsystem(
    UObject *Outer) const {
    const UWorld *World = Cast<UWorld>(Outer);
    return World && World->IsGameWorld();
}

void UMythicEntityPresentationRegistry::Initialize(
    FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);

    RegisteredComponents.Reset();
    AuthorityEntityByInstance.Reset();
    AuthorityInstanceByEntity.Reset();
    IssuedHandles.Reset();
    NextEmbodimentGeneration = 1;
    PresentationEpochRevision = 1;
    RegistryRevision = 0;
}

void UMythicEntityPresentationRegistry::Deinitialize() {
    TArray<TPair<FMythicEntityPresentationInstance,
                 TWeakObjectPtr<UMythicEntityPresentationComponent>>>
        RemovedRegistrations;
    RemovedRegistrations.Reserve(RegisteredComponents.Num());
    for (const TPair<FMythicEntityPresentationInstance,
                     TWeakObjectPtr<UMythicEntityPresentationComponent>>
             &Pair : RegisteredComponents) {
        RemovedRegistrations.Emplace(Pair.Key, Pair.Value);
    }

    RegisteredComponents.Reset();
    AuthorityEntityByInstance.Reset();
    AuthorityInstanceByEntity.Reset();
    IssuedHandles.Reset();
    ++RegistryRevision;

    for (const TPair<FMythicEntityPresentationInstance,
                     TWeakObjectPtr<UMythicEntityPresentationComponent>>
             &Pair : RemovedRegistrations) {
        OnPresentationUnregistered.Broadcast(Pair.Key, Pair.Value.Get());
    }

    OnPresentationRegistered.Clear();
    OnPresentationUnregistered.Clear();

    Super::Deinitialize();
}

FMythicEntityPresentationInstance
UMythicEntityPresentationRegistry::AllocateAuthorityInstance(
    const FMythicEntityId &EntityId) {
    if (!IsInGameThread() || !IsAuthorityContext() || !EntityId.IsValid()) {
        return FMythicEntityPresentationInstance();
    }

    if (const FMythicEntityPresentationInstance *Existing =
            AuthorityInstanceByEntity.Find(EntityId)) {
        const FMythicEntityPresentationInstance Previous = *Existing;
        ReleaseAuthorityInstance(Previous);
    }

    const FMythicPresentationHandle Handle = AllocatePresentationHandle();
    const uint32 Generation = AllocateEmbodimentGeneration();
    if (!Handle.IsValid() || Generation == 0) {
        return FMythicEntityPresentationInstance();
    }

    const FMythicEntityPresentationInstance Instance(Handle, Generation);
    AuthorityEntityByInstance.Add(Instance, EntityId);
    AuthorityInstanceByEntity.Add(EntityId, Instance);
    return Instance;
}

bool UMythicEntityPresentationRegistry::RegisterPresentationComponent(
    const FMythicEntityPresentationInstance &Instance,
    UMythicEntityPresentationComponent *Component) {
    if (!IsInGameThread() || !Instance.IsValid() || !IsValid(Component)
        || !Component->RepresentsInstance(Instance)) {
        return false;
    }

    if (IsAuthorityContext()
        && !AuthorityEntityByInstance.Contains(Instance)) {
        UE_LOG(LogMythicEntityPresentationRegistry, Warning,
               TEXT("Rejected authority registration without a canonical allocation: %s"),
               *Instance.ToDebugString());
        return false;
    }

    PruneStaleRegistrations();

    if (TWeakObjectPtr<UMythicEntityPresentationComponent> *Existing =
            RegisteredComponents.Find(Instance)) {
        if (Existing->Get() == Component) {
            return true;
        }

        if (Existing->IsValid()) {
            UE_LOG(LogMythicEntityPresentationRegistry, Error,
                   TEXT("Rejected live presentation-handle collision for %s"),
                   *Instance.ToDebugString());
            return false;
        }

        RegisteredComponents.Remove(Instance);
        OnPresentationUnregistered.Broadcast(Instance, nullptr);
    }

    RegisteredComponents.Add(Instance, Component);
    ++RegistryRevision;
    OnPresentationRegistered.Broadcast(Instance, Component);
    return true;
}

void UMythicEntityPresentationRegistry::UnregisterPresentationComponent(
    const FMythicEntityPresentationInstance &Instance,
    UMythicEntityPresentationComponent *ExpectedComponent) {
    if (!IsInGameThread() || !Instance.IsValid()
        || ExpectedComponent == nullptr) {
        return;
    }

    RemovePublicRegistration(Instance, ExpectedComponent, true);
}

void UMythicEntityPresentationRegistry::ReleaseAuthorityInstance(
    const FMythicEntityPresentationInstance &Instance) {
    if (!IsInGameThread() || !IsAuthorityContext() || !Instance.IsValid()) {
        return;
    }

    RemovePublicRegistration(Instance, nullptr, false);

    FMythicEntityId EntityId;
    if (!AuthorityEntityByInstance.RemoveAndCopyValue(Instance, EntityId)) {
        return;
    }

    if (const FMythicEntityPresentationInstance *MappedInstance =
            AuthorityInstanceByEntity.Find(EntityId);
        MappedInstance && *MappedInstance == Instance) {
        AuthorityInstanceByEntity.Remove(EntityId);
    }
}

UMythicEntityPresentationComponent *
UMythicEntityPresentationRegistry::ResolvePresentationComponent(
    const FMythicEntityPresentationInstance &Instance) const {
    if (!IsInGameThread() || !Instance.IsValid()) {
        return nullptr;
    }

    const TWeakObjectPtr<UMythicEntityPresentationComponent> *Found =
        RegisteredComponents.Find(Instance);
    UMythicEntityPresentationComponent *Component =
        Found ? Found->Get() : nullptr;
    return Component && Component->RepresentsInstance(Instance) ? Component
                                                                : nullptr;
}

void UMythicEntityPresentationRegistry::GetRegisteredComponents(
    TArray<UMythicEntityPresentationComponent *> &OutComponents) const {
    OutComponents.Reset();
    OutComponents.Reserve(RegisteredComponents.Num());
    for (const TPair<FMythicEntityPresentationInstance,
                     TWeakObjectPtr<UMythicEntityPresentationComponent>>
             &Pair : RegisteredComponents) {
        if (UMythicEntityPresentationComponent *Component = Pair.Value.Get();
            Component && Component->RepresentsInstance(Pair.Key)) {
            OutComponents.Add(Component);
        }
    }
}

bool UMythicEntityPresentationRegistry::ResolveAuthorityEntity(
    const FMythicEntityPresentationInstance &Instance,
    FMythicEntityId &OutEntityId) const {
    OutEntityId.Reset();
    if (!IsInGameThread() || !IsAuthorityContext() || !Instance.IsValid()) {
        return false;
    }

    const FMythicEntityId *Found = AuthorityEntityByInstance.Find(Instance);
    if (!Found || !Found->IsValid()) {
        return false;
    }

    OutEntityId = *Found;
    return true;
}

bool UMythicEntityPresentationRegistry::FindAuthorityInstance(
    const FMythicEntityId &EntityId,
    FMythicEntityPresentationInstance &OutInstance) const {
    OutInstance.Reset();
    if (!IsInGameThread() || !IsAuthorityContext() || !EntityId.IsValid()) {
        return false;
    }

    const FMythicEntityPresentationInstance *Found =
        AuthorityInstanceByEntity.Find(EntityId);
    if (!Found || !Found->IsValid()) {
        return false;
    }

    OutInstance = *Found;
    return true;
}

void UMythicEntityPresentationRegistry::ResetAuthorityPresentationEpoch() {
    if (!IsInGameThread() || !IsAuthorityContext()) {
        return;
    }

    TArray<TPair<FMythicEntityPresentationInstance,
                 TWeakObjectPtr<UMythicEntityPresentationComponent>>>
        RemovedRegistrations;
    RemovedRegistrations.Reserve(RegisteredComponents.Num());
    for (const TPair<FMythicEntityPresentationInstance,
                     TWeakObjectPtr<UMythicEntityPresentationComponent>>
             &Pair : RegisteredComponents) {
        RemovedRegistrations.Emplace(Pair.Key, Pair.Value);
    }

    RegisteredComponents.Reset();
    AuthorityEntityByInstance.Reset();
    AuthorityInstanceByEntity.Reset();
    ++RegistryRevision;

    ++PresentationEpochRevision;
    if (PresentationEpochRevision == 0) {
        PresentationEpochRevision = 1;
    }

    for (const TPair<FMythicEntityPresentationInstance,
                     TWeakObjectPtr<UMythicEntityPresentationComponent>>
             &Pair : RemovedRegistrations) {
        OnPresentationUnregistered.Broadcast(Pair.Key, Pair.Value.Get());
    }
}

void UMythicEntityPresentationRegistry::ResetAuthorityDomain(
    const EMythicEntityDomain Domain) {
    if (!IsInGameThread() || !IsAuthorityContext()
        || Domain == EMythicEntityDomain::Invalid) {
        return;
    }

    TArray<FMythicEntityPresentationInstance> DomainInstances;
    for (const TPair<FMythicEntityPresentationInstance, FMythicEntityId> &Pair :
         AuthorityEntityByInstance) {
        if (Pair.Value.GetDomain() == Domain) {
            DomainInstances.Add(Pair.Key);
        }
    }
    for (const FMythicEntityPresentationInstance &Instance : DomainInstances) {
        ReleaseAuthorityInstance(Instance);
    }

    ++RegistryRevision;
    ++PresentationEpochRevision;
    if (PresentationEpochRevision == 0) {
        PresentationEpochRevision = 1;
    }
}

void UMythicEntityPresentationRegistry::PruneStaleRegistrations() {
    if (!IsInGameThread()) {
        return;
    }

    TArray<FMythicEntityPresentationInstance> StaleInstances;
    for (const TPair<FMythicEntityPresentationInstance,
             TWeakObjectPtr<UMythicEntityPresentationComponent>>
             &Pair : RegisteredComponents) {
        UMythicEntityPresentationComponent *Component = Pair.Value.Get();
        if (!Component || !Component->RepresentsInstance(Pair.Key)) {
            StaleInstances.Add(Pair.Key);
        }
    }

    for (const FMythicEntityPresentationInstance &Instance :
         StaleInstances) {
        RemovePublicRegistration(Instance, nullptr, false);
        if (IsAuthorityContext()) {
            ReleaseAuthorityInstance(Instance);
        }
    }
}

bool UMythicEntityPresentationRegistry::IsAuthorityContext() const {
    const UWorld *World = GetWorld();
    return World && World->GetNetMode() != NM_Client;
}

uint32 UMythicEntityPresentationRegistry::AllocateEmbodimentGeneration() {
    if (NextEmbodimentGeneration == 0) {
        NextEmbodimentGeneration = 1;
        ++PresentationEpochRevision;
        if (PresentationEpochRevision == 0) {
            PresentationEpochRevision = 1;
        }
    }

    return NextEmbodimentGeneration++;
}

FMythicPresentationHandle
UMythicEntityPresentationRegistry::AllocatePresentationHandle() {
    constexpr int32 MaxCollisionRetries = 64;
    for (int32 Attempt = 0; Attempt < MaxCollisionRetries; ++Attempt) {
        const FMythicPresentationHandle Candidate =
            FMythicPresentationHandle::FromAuthorityNonce(FGuid::NewGuid());
        if (Candidate.IsValid() && !IssuedHandles.Contains(Candidate)) {
            IssuedHandles.Add(Candidate);
            return Candidate;
        }
    }

    UE_LOG(LogMythicEntityPresentationRegistry, Error,
           TEXT("Could not allocate a unique presentation nonce after %d attempts"),
           MaxCollisionRetries);
    return FMythicPresentationHandle();
}

void UMythicEntityPresentationRegistry::RemovePublicRegistration(
    const FMythicEntityPresentationInstance &Instance,
    UMythicEntityPresentationComponent *ExpectedComponent,
    const bool bRequireExpectedComponent) {
    TWeakObjectPtr<UMythicEntityPresentationComponent> *Found =
        RegisteredComponents.Find(Instance);
    if (!Found) {
        return;
    }

    if (bRequireExpectedComponent) {
        const TWeakObjectPtr<UMythicEntityPresentationComponent> ExpectedWeak(
            ExpectedComponent);
        if (!Found->HasSameIndexAndSerialNumber(ExpectedWeak)) {
            return;
        }
    }

    UMythicEntityPresentationComponent *RemovedComponent = Found->Get();
    RegisteredComponents.Remove(Instance);
    ++RegistryRevision;
    OnPresentationUnregistered.Broadcast(Instance, RemovedComponent);
}
