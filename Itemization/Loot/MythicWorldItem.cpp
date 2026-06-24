// 

#include "MythicWorldItem.h"

#include "Mythic.h"
#include "NavigationSystem.h"
#include "Components/StaticMeshComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"


class UNavigationSystemV1;
// Called when the game starts or when spawned
void AMythicWorldItem::BeginPlay() {
    Super::BeginPlay();
}

AMythicWorldItem::AMythicWorldItem() {
    // should replicate to all clients
    this->bReplicates = true;
    this->bReplicateUsingRegisteredSubObjectList = true;

    // Create the static mesh component
    StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));

    // Set the static mesh component as the root component
    RootComponent = StaticMesh;

    // Set collision to overlaps only
    this->SetActorEnableCollision(true);
    StaticMesh->SetSimulatePhysics(false);
    StaticMesh->SetCollisionProfileName(TEXT("OverlapAll"));
}

void AMythicWorldItem::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    Super::EndPlay(EndPlayReason);

    UE_LOG(Myth, Warning, TEXT("AMythicWorldItem::EndPlay: %s"), *GetName());

    // NOTE: a despawned/streamed-out world item that was never picked up is currently NOT preserved. The former
    // AddToUnclaimed() here was a TODO stub referencing a non-existent UDataManagerSubsystem; the "unclaimed items"
    // feature (persisting un-picked-up drops per player) needs a real storage subsystem and is tracked as deferred.
}

// Initialize. Sets the item instance
void AMythicWorldItem::SetItemInstance(UMythicItemInstance *ItemInst) {
    checkf(HasAuthority(), TEXT("AMythicWorldItem::SetItemInstance: Only call this on the server"));
    if (!ItemInst) {
        UE_LOG(Myth, Warning, TEXT("AMythicWorldItem::SetItemInstance: ItemInstance is null"));
        return;
    }

    ItemInst->SetOwner(this); // take ownership,this actor will become responsible for replicating it
    this->ItemInstance = ItemInst;

    OnRep_ItemInstance();
}

void AMythicWorldItem::OnRep_ItemInstance() {
    UE_LOG(Myth, Warning, TEXT("AMythicWorldItem::OnRep_ItemInstance: %s"), *GetName());
    OnItemInstanceUpdated();
}

// If the item hits the ground (the hit is under the item), stop simulating physics
void AMythicWorldItem::OnHit(UPrimitiveComponent *HitComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp, FVector NormalImpulse,
                             const FHitResult &Hit) {
    // If the hit is under the item, stop simulating physics, and allow overlaps only
    if (Hit.ImpactPoint.Z < GetActorLocation().Z || Hit.Normal.Z > 0.5f) {
        UE_LOG(Myth, Warning, TEXT("AMythicWorldItem::OnHit: %s"), *GetName());
        StaticMesh->SetSimulatePhysics(false);
        StaticMesh->SetEnableGravity(false);
        StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        StaticMesh->SetCollisionResponseToAllChannels(ECR_Overlap);
    }
}

void AMythicWorldItem::EmulateDropPhysics(const FVector &location, float radius) {
    // Use the suggest projectile velocity function to get a suggested velocity to emulate a drop effect.
    // Randomize the target using navmesh. Default the arc target to the drop origin: on nav failure (no nav system, or
    // the drop point is off-navmesh — rooftops/ledges/water/caves) GetRandomPointInNavigableRadius leaves
    // RandomizedLocation at ZeroVector, which previously got passed as the arc End and flung the loot across the map
    // toward world origin (unrecoverable). With Start==End the arc is degenerate and the item simply drops in place.
    FVector TargetLoc = location;
    FNavLocation RandomizedLocation;
    UNavigationSystemV1 *NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (NavSys && NavSys->GetRandomPointInNavigableRadius(location, radius, RandomizedLocation, nullptr)) {
        TargetLoc = RandomizedLocation.Location;
        UE_LOG(Myth, Warning, TEXT("Randomized location in %f radius from %s to %s"), radius, *location.ToString(), *RandomizedLocation.Location.ToString());
    }

    FVector SuggestedVelocity;
    UGameplayStatics::SuggestProjectileVelocity_CustomArc(
        GetWorld(),
        SuggestedVelocity,
        location,
        TargetLoc);

    // Allow physics emulation on the world item
    this->StaticMesh->SetCollisionResponseToAllChannels(ECR_Block);
    this->StaticMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    this->StaticMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    this->StaticMesh->SetSimulatePhysics(true);
    this->StaticMesh->SetEnableGravity(true);
    this->StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    // Apply the suggested velocity to the world item
    this->StaticMesh->SetPhysicsLinearVelocity(SuggestedVelocity);

    // generate hit events, If the item hits the ground, stop simulating physics
    this->StaticMesh->SetNotifyRigidBodyCollision(true);
    this->StaticMesh->OnComponentHit.AddDynamic(this, &AMythicWorldItem::OnHit);
}

// Cosmetic only.
// Handles making the item visible to only the owner (bOnlyRelevantToOwner should be set before this is called).
// Runs on clients AND, if called via SetIsPrivate, on the server
void AMythicWorldItem::OnRep_TargetRecipient() {
    // If private, onlyRelavantToOwner should be true which means only the owner will run this logic
    if (TargetRecipient) {
        // Get owner player controller
        APlayerController *OwnerPlayerController = Cast<APlayerController>(GetOwner());
        if (!OwnerPlayerController) {
            UE_LOG(Myth, Warning, TEXT("AMythicWorldItem::OnRep_ForAll: Owner is not a player controller"));
            return;
        }

        // If this owner client is either a connected client or the host client, show the actor to them by setting it to not hidden
        // bool is_client_on_the_listen_server = OwnerPlayerController->IsLocalController() && HasAuthority();
        // bool is_client_on_a_connected_client = OwnerPlayerController->IsLocalController() && !HasAuthority();
        // bool is_any_client = OwnerPlayerController->IsLocalController();

        // The host is the server (and also a client), so the item will always be visible to the host, regardless of the OnlyRelevantToOwner setting.
        // Therefore on all clients (including the host), we check if the owner is their local player controller and show the actor if it is, otherwise hide it.
        // Basically, only the owner client will see the item if it is private.
        auto isLocalPlayer = OwnerPlayerController->IsLocalController();
        SetActorHiddenInGame(!isLocalPlayer);
    }
    else {
        // If not private, show the actor to all clients
        SetActorHiddenInGame(false);
    }
}

// Sets the IsPrivate property and explicitly calls the OnRep_IsPrivate function for caller
// If caller is server, OnRep_IsPrivate will also be called on clients since IsPrivate is replicated
// Runs on server
void AMythicWorldItem::SetTargetRecipient(AController *NewTargetRecipient) {
    checkf(HasAuthority(), TEXT("AMythicWorldItem::SetIsPrivate: Only call this on the server"));

    this->TargetRecipient = NewTargetRecipient;
    OnRep_TargetRecipient();
}

// --- IMythicSaveableActor Implementation ---

void AMythicWorldItem::SerializeCustomData(TArray<uint8> &OutCustomData) {
    if (!ItemInstance) {
        return; // Nothing to serialize
    }

    // Serialize the item instance class and data
    FMemoryWriter MemWriter(OutCustomData);
    FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
    Ar.ArIsSaveGame = true;

    // Save the class path so we can recreate it
    FSoftClassPath ItemClassPath(ItemInstance->GetClass());
    Ar << ItemClassPath;

    // Save the item's internal data
    ItemInstance->Serialize(Ar);
}

void AMythicWorldItem::DeserializeCustomData(const TArray<uint8> &InCustomData) {
    if (InCustomData.Num() == 0) {
        return; // No data to deserialize
    }

    FMemoryReader MemReader(InCustomData);
    FObjectAndNameAsStringProxyArchive Ar(MemReader, false);
    Ar.ArIsSaveGame = true;

    // Load the class path
    FSoftClassPath ItemClassPath;
    Ar << ItemClassPath;

    // Create the item instance
    UClass *ItemClass = ItemClassPath.TryLoadClass<UMythicItemInstance>();
    if (ItemClass) {
        ItemInstance = NewObject<UMythicItemInstance>(this, ItemClass);
        ItemInstance->Serialize(Ar);
        ItemInstance->SetOwner(this);

        OnRep_ItemInstance();
    }
}
