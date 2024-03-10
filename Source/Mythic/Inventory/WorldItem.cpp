// 


#include "WorldItem.h"

#include "NavigationSystem.h"


class UNavigationSystemV1;
// Called when the game starts or when spawned
void AWorldItem::BeginPlay() {
    Super::BeginPlay();
}

AWorldItem::AWorldItem() {
    // should replicate to all clients
    this->bReplicates = true;
    this->bReplicateUsingRegisteredSubObjectList = true;

    // Create the static mesh component
    StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));

    // Set the static mesh component as the root component
    RootComponent = StaticMesh;

    this->SetActorEnableCollision(true);
}

void AWorldItem::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    Super::EndPlay(EndPlayReason);

    UE_LOG(Mythic, Warning, TEXT("AWorldItem::EndPlay: %s"), *GetName());

    // If the item was not picked up, add it to the player's unclaimed items
    if (ItemInstances.Num() > 0) {
        AddToUnclaimed();
    }
}

void AWorldItem::AddToUnclaimed() {
    //TODO
    // Get the DataManagerSubsystem
    // UDataManagerSubsystem* DataManager = GetGameInstance()->GetSubsystem<UDataManagerSubsystem>();
    //
    // // Add this item to the player's unclaimed items in the database
    // DataManager->AddUnclaimedItem(RelevantPlayerController->PlayerState->GetUniqueId(), this);
}

// Initialize. Sets the item instance
void AWorldItem::AddItemInstance(UItemInstance *ItemInst) {
    checkf(HasAuthority(), TEXT("AWorldItem::SetItemInstance: Only call this on the server"));
    if (!ItemInst) {
        UE_LOG(Mythic, Warning, TEXT("AWorldItem::SetItemInstance: ItemInstance is null"));
        return;
    }

    ItemInst->SetOwner(this); // take ownership,this actor will become responsible for replicating it
    this->ItemInstances.Add(ItemInst);

    OnRep_ItemInstances();
}

void AWorldItem::OnRep_ItemInstances() {
    UE_LOG(Mythic, Warning, TEXT("AWorldItem::OnRep_ItemInstance: %s"), *GetName());
    OnItemInstanceUpdated();
}

// Cosmetic only.
// Handles making the item visible to only the owner (bOnlyRelevantToOwner should be set before this is called).
// Runs on clients AND, if called via SetIsPrivate, on the server
void AWorldItem::OnRep_IsPrivate() {
    // If private, onlyRelavantToOwner should be true which means only the owner will run this logic
    if (IsPrivateDrop) {
        // Get owner player controller
        APlayerController *OwnerPlayerController = Cast<APlayerController>(GetOwner());
        if (!OwnerPlayerController) {
            UE_LOG(Mythic, Warning, TEXT("AWorldItem::OnRep_ForAll: Owner is not a player controller"));
            return;
        }

        auto is_server = HasAuthority();
        auto test = &is_server;
        UE_LOG(Mythic, Warning, TEXT("AWorldItem::OwnerPlayerController is: %s"), *OwnerPlayerController->GetName());
        UE_LOG(Mythic, Warning, TEXT("AWorldItem::IsServer: %d"), *test);

        // If this owner client is either a connected client or the host client, show the actor to them by setting it to not hidden
        // bool is_client_on_the_listen_server = OwnerPlayerController->IsLocalController() && HasAuthority();
        // bool is_client_on_a_connected_client = OwnerPlayerController->IsLocalController() && !HasAuthority();
        // bool is_any_client = OwnerPlayerController->IsLocalController();

        // The host is the server (and also a client), so the item will always be visible to the host, regardless of the OnlyRelevantToOwner setting.
        // Therefore on all clients (including the host), we check if the owner is their local player controller and show the actor if it is, otherwise hide it.
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
void AWorldItem::SetIsPrivate(bool is_private) {
    checkf(HasAuthority(), TEXT("AWorldItem::SetIsPrivate: Only call this on the server"));

    this->IsPrivateDrop = is_private;
    OnRep_IsPrivate();
}
