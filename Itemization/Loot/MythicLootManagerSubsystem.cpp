#include "MythicLootManagerSubsystem.h"

#include "Player/MythicPlayerController.h"
#include "../Loot/MythicWorldItem.h"
#include "Engine/World.h"
#include "GameFramework/GameState.h"
#include "Itemization/InventoryProviderInterface.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Mythic/Itemization/Inventory/MythicInventoryComponent.h"
#include "Settings/MythicDeveloperSettings.h"
#include "NiagaraComponent.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

UMythicItemInstance *UMythicLootManagerSubsystem::Create(UItemDefinition *item_def, int32 quantity_if_stackable, AController *TargetRecipient, int32 level) {
    if (!item_def) {
        UE_LOG(Myth, Warning, TEXT("Item definition is null"));
        return nullptr;
    }

    UMythicItemInstance *ItemInstance = NewObject<UMythicItemInstance>();
    if (!ItemInstance) {
        UE_LOG(Myth, Warning, TEXT("Failed to create item instance"));
        return nullptr;
    }

    if (TargetRecipient) {
        ItemInstance->SetOwner(TargetRecipient);
    }
    else {
        auto GameMode = GetWorld()->GetAuthGameMode();
        if (!GameMode) {
            UE_LOG(Myth, Warning, TEXT("GameMode is null. GameMode is only available on the server."));
            return nullptr;
        }
        auto GameState = GameMode->GameState;
        ItemInstance->SetOwner(GameState);
    }
    ItemInstance->Initialize(item_def, quantity_if_stackable, level);

    return ItemInstance;
}

AMythicWorldItem *UMythicLootManagerSubsystem::CreateAndSpawn(UItemDefinition *item_def, const FVector &location, AController *TargetRecipient, int32 level = 0,
                                                              int32 quantity_if_stackable = 1, float radius = 100.0f) {
    if (auto item_instance = Create(item_def, quantity_if_stackable, TargetRecipient, level)) {
        return Spawn(item_instance, location, radius, TargetRecipient);
    }

    return nullptr;
}


AMythicWorldItem *UMythicLootManagerSubsystem::CreateAndGive(UItemDefinition *ItemDef, int32 QtyIfStackable, TScriptInterface<IInventoryProviderInterface> InventoryProvider,
                                                             AController *TargetRecipient, int32 Lvl) {
    if (!InventoryProvider) {
        UE_LOG(Myth, Warning, TEXT("InventoryProvider is null"));
        return nullptr;
    }
    if (!ItemDef) {
        UE_LOG(Myth, Warning, TEXT("Item definition is null"));
        return nullptr;
    }
    if (!DefaultWorldItemClass) {
        UE_LOG(Myth, Warning, TEXT("Default world item class is not set - Use SetDefaultWorldItemClass to set it."));
        return nullptr;
    }

    // Level 0 is the documented sentinel every reward path passes: "the recipient's own level". Nothing ever
    // implemented it, so quest, dialogue and achievement rewards rolled level-0 items that no affix tier
    // accepts. Resolved here, at the one entry every giver funnels through.
    if (Lvl <= 0) {
        const AMythicPlayerController *RecipientPC = Cast<AMythicPlayerController>(TargetRecipient);
        Lvl = RecipientPC ? FMath::Max(1, RecipientPC->GetPlayerLevel()) : 1;
    }

    auto Inventory = InventoryProvider.GetInterface()->GetInventoryForItemDefinition(ItemDef);
    if (!Inventory) {
        UE_LOG(Myth, Warning, TEXT("InventoryProvider does not have an inventory for item definition %s"), *ItemDef->GetName());
        return nullptr;
    }

    UMythicItemInstance *ItemInstance = Create(ItemDef, QtyIfStackable, TargetRecipient, Lvl);

    return Inventory->AddItem(ItemInstance, TargetRecipient);
}

AMythicWorldItem *UMythicLootManagerSubsystem::Spawn(UMythicItemInstance *item, const FVector &location, float radius, AController *TargetRecipient) {
    if (!DefaultWorldItemClass) {
        UE_LOG(Myth, Warning, TEXT("Default world item class is not set - Use SetDefaultWorldItemClass to set it."));
        return nullptr;
    }

    if (!item) {
        UE_LOG(Myth, Warning, TEXT("Item instance is null"));
        return nullptr;
    }

    if (location == FVector::ZeroVector) {
        UE_LOG(Myth, Error, TEXT("Location for the drop is not set"));
    }

    FVector start_location = location + FVector(0, 0, 50);
    auto spawn_params = FActorSpawnParameters();
    if (TargetRecipient) {
        spawn_params.Owner = TargetRecipient;
    }
    else {
        auto GameMode = GetWorld()->GetAuthGameMode();
        if (!GameMode) {
            UE_LOG(Myth, Warning, TEXT("GameMode is null. GameMode is only available on the server."));
            return nullptr;
        }
        auto GameState = GameMode->GameState;
        spawn_params.Owner = GameState;
    }

    AMythicWorldItem *WorldItem = GetWorld()->SpawnActor<AMythicWorldItem>(DefaultWorldItemClass, start_location, FRotator::ZeroRotator, spawn_params);

    if (!WorldItem) {
        UE_LOG(Myth, Warning, TEXT("SpawnActor failed"));
        return nullptr;
    }

    WorldItem->bOnlyRelevantToOwner = TargetRecipient != nullptr;
    WorldItem->SetTargetRecipient(TargetRecipient);

    WorldItem->SetItemInstance(item);

    WorldItem->EmulateDropPhysics(start_location, radius);

    LiveWorldItems.Add(WorldItem);
    if (WorldItemLifetimeSeconds > 0.0f) {
        WorldItem->SetLifeSpan(WorldItemLifetimeSeconds);
    }
    EnforceWorldItemBudget();
    UpdateWorldItemFXTimer();

    return WorldItem;
}

void UMythicLootManagerSubsystem::EnforceWorldItemBudget() {
    LiveWorldItems.RemoveAll([](const TWeakObjectPtr<AMythicWorldItem> &Item) {
        return !Item.IsValid();
    });

    if (MaxLiveWorldItems <= 0 || LiveWorldItems.Num() <= MaxLiveWorldItems) {
        return;
    }

    const int32 ToEvict = LiveWorldItems.Num() - MaxLiveWorldItems;
    int32 Evicted = 0;
    while (Evicted < ToEvict && LiveWorldItems.Num() > 0) {
        if (AMythicWorldItem *Oldest = LiveWorldItems[0].Get()) {
            Oldest->Destroy();
        }
        LiveWorldItems.RemoveAt(0);
        ++Evicted;
    }

    UE_LOG(Myth, Log, TEXT("Loot budget: despawned %d oldest dropped item(s); %d live (cap %d)."),
           Evicted, LiveWorldItems.Num(), MaxLiveWorldItems);
}

void UMythicLootManagerSubsystem::UpdateWorldItemFXTimer() {
    UWorld *World = GetWorld();
    if (!World || WorldItemFXDistance <= 0.0f) {
        return;
    }

    const bool bWantTimer = LiveWorldItems.Num() > 0;
    const bool bHaveTimer = World->GetTimerManager().IsTimerActive(WorldItemFXTimer);

    if (bWantTimer && !bHaveTimer) {
        World->GetTimerManager().SetTimer(WorldItemFXTimer, this, &UMythicLootManagerSubsystem::RunWorldItemFXPass,
                                          FMath::Max(0.05f, WorldItemFXInterval), true);
    }
    else if (!bWantTimer && bHaveTimer) {
        World->GetTimerManager().ClearTimer(WorldItemFXTimer);
    }
}

void UMythicLootManagerSubsystem::RunWorldItemFXPass() {
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }

    LiveWorldItems.RemoveAll([](const TWeakObjectPtr<AMythicWorldItem> &Item) {
        return !Item.IsValid();
    });
    if (LiveWorldItems.Num() == 0) {
        UpdateWorldItemFXTimer();
        return;
    }

    TArray<FVector, TInlineAllocator<4>> ViewPoints;
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It) {
        const APlayerController *PC = It->Get();
        if (!PC || !PC->IsLocalController()) {
            continue;
        }
        FVector ViewLocation;
        FRotator ViewRotation;
        PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
        ViewPoints.Add(ViewLocation);
    }

    const float CullSq = WorldItemFXDistance * WorldItemFXDistance;
    for (const TWeakObjectPtr<AMythicWorldItem> &WeakItem : LiveWorldItems) {
        AMythicWorldItem *Item = WeakItem.Get();
        if (!Item) {
            continue;
        }
        UNiagaraComponent *FX = Item->FindComponentByClass<UNiagaraComponent>();
        if (!FX) {
            continue;
        }

        const FVector ItemLocation = Item->GetActorLocation();
        bool bNear = false;
        for (const FVector &View : ViewPoints) {
            if (FVector::DistSquared(View, ItemLocation) <= CullSq) {
                bNear = true;
                break;
            }
        }

        if (bNear && !FX->IsActive()) {
            FX->Activate();
        }
        else if (!bNear && FX->IsActive()) {
            FX->Deactivate();
        }
    }
}

void UMythicLootManagerSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);

    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    if (!Settings || Settings->DefaultWorldItemClass.IsNull()) {
        UE_LOG(Myth, Warning,
               TEXT("LootManager: DefaultWorldItemClass is not set in project settings (Mythic > Loot). Loot will "
                    "only work in worlds whose game mode assigns it."));
        return;
    }

    DefaultWorldItemClass = Settings->DefaultWorldItemClass.LoadSynchronous();
    if (!DefaultWorldItemClass) {
        UE_LOG(Myth, Error, TEXT("LootManager: failed to load DefaultWorldItemClass '%s'."),
               *Settings->DefaultWorldItemClass.ToString());
        return;
    }
    UE_LOG(Myth, Log, TEXT("LootManager: world-item class resolved to %s."), *DefaultWorldItemClass->GetName());
}

void UMythicLootManagerSubsystem::SetDefaultWorldItemClass(const TSubclassOf<AMythicWorldItem> &NewDefaultWorldItemClass) {
    DefaultWorldItemClass = NewDefaultWorldItemClass;
}

void UMythicLootManagerSubsystem::DestroyWorldItem(AMythicWorldItem *WorldItem, AController *Controller) {
    if (!WorldItem) {
        UE_LOG(Myth, Warning, TEXT("WorldItem is null"));
        return;
    }

    if (!Controller) {
        UE_LOG(Myth, Warning, TEXT("Controller is null"));
        return;
    }


    if (!WorldItem->GetTargetRecipient() || WorldItem->GetTargetRecipient() == Controller) {
        WorldItem->Destroy();
    }
    else {
        UE_LOG(Myth, Warning, TEXT("Player %s does not have permission to destroy WorldItem"), *Controller->GetName());
    }
}

bool UMythicLootManagerSubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    UWorld *World = Outer->GetWorld();
    if (World->WorldType != EWorldType::None && World->GetNetMode() < NM_Client) {
        UE_LOG(Myth, Log, TEXT("LootManager created on server"));
        return true;
    }

    UE_LOG(Myth, Warning, TEXT("LootManager will not be created on client"));
    return false;
}
