
#include "World/Placement/MythicPlacedProxySubsystem.h"
#include "World/Placement/MythicProxyStateful.h"
#include "Components/StaticMeshComponent.h"

#include "Mythic/Mythic.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

UMythicPlacedProxySubsystem *UMythicPlacedProxySubsystem::Get(const UObject *WorldContextObject) {
    if (!WorldContextObject) {
        return nullptr;
    }
    const UWorld *World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    return World ? World->GetSubsystem<UMythicPlacedProxySubsystem>() : nullptr;
}

void UMythicPlacedProxySubsystem::Deinitialize() {
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(PassTimer);
    }
    LiveActors.Empty();
    InstanceGroups.Empty();
    Proxies.Empty();
    InstanceHost = nullptr;
    Super::Deinitialize();
}

FMythicPlacedProxy *UMythicPlacedProxySubsystem::FindProxy(const FGuid &Id) {
    return Proxies.FindByPredicate([&Id](const FMythicPlacedProxy &P) { return P.Id == Id; });
}

const FMythicPlacedProxy *UMythicPlacedProxySubsystem::FindProxy(const FGuid &Id) const {
    return Proxies.FindByPredicate([&Id](const FMythicPlacedProxy &P) { return P.Id == Id; });
}

AActor *UMythicPlacedProxySubsystem::GetOrCreateInstanceHost() {
    if (InstanceHost) {
        return InstanceHost;
    }
    UWorld *World = GetWorld();
    if (!World) {
        return nullptr;
    }
    FActorSpawnParameters Params;
    Params.ObjectFlags |= RF_Transient;
    InstanceHost = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
    if (InstanceHost) {
        InstanceHost->SetActorEnableCollision(false);
        InstanceHost->SetReplicates(false);
    }
    return InstanceHost;
}

UInstancedStaticMeshComponent *UMythicPlacedProxySubsystem::GetOrCreateGroup(UStaticMesh *Mesh) {
    if (!Mesh) {
        return nullptr;
    }
    if (TObjectPtr<UInstancedStaticMeshComponent> *Existing = InstanceGroups.Find(Mesh)) {
        return *Existing;
    }
    AActor *Host = GetOrCreateInstanceHost();
    if (!Host) {
        return nullptr;
    }

    UInstancedStaticMeshComponent *Group = NewObject<UInstancedStaticMeshComponent>(Host);
    Group->SetStaticMesh(Mesh);
    Group->SetMobility(EComponentMobility::Static);
    Group->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Group->SetupAttachment(Host->GetRootComponent());
    Group->RegisterComponent();

    InstanceGroups.Add(Mesh, Group);
    return Group;
}

void UMythicPlacedProxySubsystem::AddInstanceFor(FMythicPlacedProxy &Proxy) {
    if (Proxy.InstanceIndex != INDEX_NONE) {
        return;
    }
    if (UInstancedStaticMeshComponent *Group = GetOrCreateGroup(Proxy.DormantMesh)) {
        Proxy.InstanceIndex = Group->AddInstance(Proxy.Transform, true);
    }
}

void UMythicPlacedProxySubsystem::RemoveInstanceFor(FMythicPlacedProxy &Proxy) {
    if (Proxy.InstanceIndex == INDEX_NONE) {
        return;
    }
    UInstancedStaticMeshComponent *Group = InstanceGroups.FindRef(Proxy.DormantMesh);
    if (!Group) {
        Proxy.InstanceIndex = INDEX_NONE;
        return;
    }

    const int32 Removed = Proxy.InstanceIndex;
    UStaticMesh *const Mesh = Proxy.DormantMesh;
    Group->RemoveInstance(Removed);
    Proxy.InstanceIndex = INDEX_NONE;

    const int32 SwappedFrom = Group->GetInstanceCount();
    for (FMythicPlacedProxy &Other : Proxies) {
        if (&Other != &Proxy && Other.DormantMesh == Mesh && Other.InstanceIndex == SwappedFrom) {
            Other.InstanceIndex = Removed;
            break;
        }
    }
}

FGuid UMythicPlacedProxySubsystem::RegisterProxy(FGameplayTag Type, const FTransform &Transform, int32 StateFlags) {
    FMythicPlacedProxy Proxy;
    Proxy.Id = FGuid::NewGuid();
    Proxy.Type = Type;
    Proxy.Transform = Transform;
    Proxy.StateFlags = StateFlags;
    Proxy.State = EMythicProxyState::Dormant;

    const int32 Index = Proxies.Add(Proxy);
    AddInstanceFor(Proxies[Index]);
    UpdateTimer();
    return Proxies[Index].Id;
}

FGuid UMythicPlacedProxySubsystem::AdoptPlacedActor(AActor *Actor, FGameplayTag Type, int32 StateFlags) {
    if (!Actor || !Type.IsValid()) {
        return FGuid();
    }

    int32 CarriedState = StateFlags;
    if (Actor->GetClass()->ImplementsInterface(UMythicProxyStateful::StaticClass())) {
        CarriedState |= IMythicProxyStateful::Execute_GetProxyStateFlags(Actor);
    }

    const FGuid Id = RegisterProxy(Type, Actor->GetActorTransform(), CarriedState);

    if (FMythicPlacedProxy *Proxy = FindProxy(Id)) {
        Proxy->ActorClass = Actor->GetClass();
        if (const UStaticMeshComponent *MeshComp = Actor->FindComponentByClass<UStaticMeshComponent>()) {
            Proxy->DormantMesh = MeshComp->GetStaticMesh();
        }
        AddInstanceFor(*Proxy);
    }
    return Id;
}

bool UMythicPlacedProxySubsystem::UnregisterProxy(const FGuid &Id) {
    const int32 Index = Proxies.IndexOfByPredicate([&Id](const FMythicPlacedProxy &P) { return P.Id == Id; });
    if (Index == INDEX_NONE) {
        return false;
    }

    if (Proxies[Index].IsPromoted()) {
        Demote(Proxies[Index]);
    }
    RemoveInstanceFor(Proxies[Index]);
    Proxies.RemoveAt(Index);
    UpdateTimer();
    return true;
}

int32 UMythicPlacedProxySubsystem::GetProxyState(const FGuid &Id) const {
    const FMythicPlacedProxy *Proxy = FindProxy(Id);
    return Proxy ? Proxy->StateFlags : 0;
}

bool UMythicPlacedProxySubsystem::SetProxyState(const FGuid &Id, int32 StateFlags) {
    if (FMythicPlacedProxy *Proxy = FindProxy(Id)) {
        Proxy->StateFlags = StateFlags;
        return true;
    }
    return false;
}

int32 UMythicPlacedProxySubsystem::GetPromotedCount() const {
    int32 Count = 0;
    for (const FMythicPlacedProxy &Proxy : Proxies) {
        if (Proxy.IsPromoted()) {
            ++Count;
        }
    }
    return Count;
}

void UMythicPlacedProxySubsystem::Promote(FMythicPlacedProxy &Proxy) {
    UWorld *World = GetWorld();

    TSubclassOf<AActor> SpawnClass = Proxy.ActorClass;
    if (!SpawnClass) {
        if (const FMythicProxyTypeDef *Def = TypeDefs.Find(Proxy.Type)) {
            SpawnClass = Def->ActorClass;
        }
    }
    if (!World || !SpawnClass) {
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    TGuardValue<bool> PromotionGuard(bPromotionInProgress, true);
    AActor *Spawned = World->SpawnActor<AActor>(SpawnClass, Proxy.Transform, Params);
    if (!Spawned) {
        UE_LOG(Myth, Warning, TEXT("PlacedProxy: failed to promote %s."), *Proxy.Type.ToString());
        return;
    }

    if (Spawned->GetClass()->ImplementsInterface(UMythicProxyStateful::StaticClass())) {
        IMythicProxyStateful::Execute_ApplyProxyStateFlags(Spawned, Proxy.StateFlags);
    }

    LiveActors.Add(Proxy.Id, Spawned);
    Proxy.State = EMythicProxyState::Promoted;
    RemoveInstanceFor(Proxy);
}

void UMythicPlacedProxySubsystem::Demote(FMythicPlacedProxy &Proxy) {
    if (TObjectPtr<AActor> *Found = LiveActors.Find(Proxy.Id)) {
        if (AActor *Actor = *Found) {
            Actor->Destroy();
        }
        LiveActors.Remove(Proxy.Id);
    }
    Proxy.State = EMythicProxyState::Dormant;
    AddInstanceFor(Proxy);
}

void UMythicPlacedProxySubsystem::RunPass() {
    UWorld *World = GetWorld();
    if (!World || Proxies.Num() == 0) {
        UpdateTimer();
        return;
    }

    if (World->GetNetMode() == NM_Client) {
        return;
    }

    TArray<FVector, TInlineAllocator<8>> PlayerLocations;
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It) {
        if (const APlayerController *PC = It->Get()) {
            if (const APawn *Pawn = PC->GetPawn()) {
                PlayerLocations.Add(Pawn->GetActorLocation());
            }
        }
    }

    for (FMythicPlacedProxy &Proxy : Proxies) {
        float NearestSq = -1.0f;
        const FVector Location = Proxy.Transform.GetLocation();
        for (const FVector &PlayerLocation : PlayerLocations) {
            const float DistSq = FVector::DistSquared(PlayerLocation, Location);
            if (NearestSq < 0.0f || DistSq < NearestSq) {
                NearestSq = DistSq;
            }
        }

        const bool bWant = FMythicPlacedProxyRules::ShouldBePromoted(NearestSq, PromoteRadius, DemoteRadius,
                                                                    Proxy.IsPromoted());
        if (bWant && !Proxy.IsPromoted()) {
            Promote(Proxy);
        }
        else if (!bWant && Proxy.IsPromoted()) {
            Demote(Proxy);
        }
    }
}

void UMythicPlacedProxySubsystem::UpdateTimer() {
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    const bool bWant = Proxies.Num() > 0;
    const bool bHave = World->GetTimerManager().IsTimerActive(PassTimer);

    if (bWant && !bHave) {
        World->GetTimerManager().SetTimer(PassTimer, this, &UMythicPlacedProxySubsystem::RunPass,
                                          FMath::Max(0.05f, PassInterval), true);
    }
    else if (!bWant && bHave) {
        World->GetTimerManager().ClearTimer(PassTimer);
    }
}
