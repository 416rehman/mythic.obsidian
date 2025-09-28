// 


#include "CraftingStation.h"

#include "AbilitySystemComponent.h"
#include "CraftingComponent.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Loot/MythicLootManagerSubsystem.h"


TArray<UMythicInventoryComponent *> ACraftingStation::GetAllInventoryComponents() const {
    return {InventoryComponent};
}

UAbilitySystemComponent *ACraftingStation::GetSchematicsASC() const {
    return nullptr;
}

// Sets default values
ACraftingStation::ACraftingStation() {
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    // Create the Crafting Component
    CraftingComponent = CreateDefaultSubobject<UCraftingComponent>(TEXT("CraftingComponent"));
    CraftingComponent->SetIsReplicated(true);

    // Create the Inventory Component
    InventoryComponent = CreateDefaultSubobject<UMythicInventoryComponent>(TEXT("InventoryComponent"));
    InventoryComponent->SetIsReplicated(true);
}

void ACraftingStation::OnItemCrafted(FCraftingQueueEntry Item) {
    // When all the items are removed, use the loot subsystem to give the player the item
    UMythicLootManagerSubsystem *LootSubsystem = GetGameInstance()->GetSubsystem<UMythicLootManagerSubsystem>();
    checkf(LootSubsystem, TEXT("Loot subsystem is null."));

    // Create the item and put it in the backpack. Targetted to player.
    LootSubsystem->CreateAndGive(Item.ItemDefinition, Item.Quantity, this, nullptr);
}

// Called when the game starts or when spawned
void ACraftingStation::BeginPlay() {
    Super::BeginPlay();

    CraftingComponent->OnItemCrafted.AddDynamic(this, &ACraftingStation::OnItemCrafted);
}

void ACraftingStation::ServerCraftItem_Implementation(UItemDefinition *Item, int32 Amount, const AActor *RequestingActor) {


    CraftingComponent->ServerAddToCraftingQueue(Item, Amount, RequestingActor);
}
