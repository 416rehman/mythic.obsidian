// 


#include "MythicWorldItem.h"

#include "Mythic.h"
#include "NavigationSystem.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Kismet/GameplayStatics.h"


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

    UE_LOG(Mythic, Warning, TEXT("AMythicWorldItem::EndPlay: %s"), *GetName());

    // If the item was not picked up, add it to the player's unclaimed items
    if (ItemInstance) {
        AddToUnclaimed();
    }
}

void AMythicWorldItem::AddToUnclaimed() {
    //TODO
    // Get the DataManagerSubsystem
    // UDataManagerSubsystem* DataManager = GetGameInstance()->GetSubsystem<UDataManagerSubsystem>();
    //
    // // Add this item to the player's unclaimed items in the database
    // DataManager->AddUnclaimedItem(RelevantPlayerController->PlayerState->GetUniqueId(), this);
}

// Initialize. Sets the item instance
void AMythicWorldItem::SetItemInstance(UMythicItemInstance *ItemInst) {
    checkf(HasAuthority(), TEXT("AMythicWorldItem::SetItemInstance: Only call this on the server"));
    if (!ItemInst) {
        UE_LOG(Mythic, Warning, TEXT("AMythicWorldItem::SetItemInstance: ItemInstance is null"));
        return;
    }

    ItemInst->SetOwner(this); // take ownership,this actor will become responsible for replicating it
    this->ItemInstance = ItemInst;

    OnRep_ItemInstance();
}

void AMythicWorldItem::OnRep_ItemInstance() {
    UE_LOG(Mythic, Warning, TEXT("AMythicWorldItem::OnRep_ItemInstance: %s"), *GetName());
    OnItemInstanceUpdated();
}

// If the item hits the ground (the hit is under the item), stop simulating physics
void AMythicWorldItem::OnHit(UPrimitiveComponent *HitComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp, FVector NormalImpulse,
    const FHitResult &Hit) {
    // If the hit is under the item, stop simulating physics, and allow overlaps only
    if (Hit.ImpactPoint.Z < GetActorLocation().Z || Hit.Normal.Z > 0.5f) {
        UE_LOG(Mythic, Warning, TEXT("AMythicWorldItem::OnHit: %s"), *GetName());
        StaticMesh->SetSimulatePhysics(false);
        StaticMesh->SetEnableGravity(false);
        StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        StaticMesh->SetCollisionResponseToAllChannels(ECR_Overlap);
    }
}

void AMythicWorldItem::EmulateDropPhysics(const FVector &location, float radius) {
    // Use the suggest projectile velocity function to get a suggested velocity to emulate a drop effect
    // Randomize the location using navmesh
    FNavLocation RandomizedLocation;
    UNavigationSystemV1 *NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (NavSys && NavSys->GetRandomPointInNavigableRadius(location, radius, RandomizedLocation, nullptr)) {
        UE_LOG(Mythic, Warning, TEXT("Randomized location in %f radius from %s to %s"), radius, *location.ToString(), *RandomizedLocation.Location.ToString());
    }

    FVector SuggestedVelocity;
    UGameplayStatics::SuggestProjectileVelocity_CustomArc(
        GetWorld(),
        SuggestedVelocity,
        location,
        RandomizedLocation.Location);

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
            UE_LOG(Mythic, Warning, TEXT("AMythicWorldItem::OnRep_ForAll: Owner is not a player controller"));
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
