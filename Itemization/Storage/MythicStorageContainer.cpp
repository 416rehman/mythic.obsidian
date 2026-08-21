
#include "MythicStorageContainer.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Mythic.h"
#include "MythicContainerStock.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Loot/MythicLootManagerSubsystem.h"
#include "Player/MythicPlayerController.h"
#include "Rewards/LootReward.h"
#include "TimerManager.h"
#include "Subsystem/SaveSystem/Character/SavedInventory.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

AMythicStorageContainer::AMythicStorageContainer() {
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    bReplicateUsingRegisteredSubObjectList = true;
    SetNetCullDistanceSquared(FMath::Square(4000.f));

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(SceneRoot);

    ContainerInventory = CreateDefaultSubobject<UMythicInventoryComponent>(TEXT("ContainerInventory"));
    ContainerInventory->SetIsReplicated(true);
}

void AMythicStorageContainer::BeginPlay() {
    Super::BeginPlay();

    if (!HasAuthority()) {
        return;
    }

    ServerStock();

    if (RestockIntervalSeconds > 0.0f) {
        GetWorldTimerManager().SetTimer(RestockTimer, this, &AMythicStorageContainer::ServerRestockTick,
                                        RestockIntervalSeconds, true);
    }
}

void AMythicStorageContainer::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    Openers.Empty();
    GetWorldTimerManager().ClearTimer(RestockTimer);
    Super::EndPlay(EndPlayReason);
}

bool AMythicStorageContainer::IsEmpty() const {
    if (!ContainerInventory) {
        return true;
    }
    for (const FMythicInventorySlotEntry &Slot : ContainerInventory->GetAllSlots()) {
        if (Slot.SlottedItemInstance) {
            return false;
        }
    }
    return true;
}

void AMythicStorageContainer::ServerRestockTick() {
    if (bRestockOnlyWhenEmpty && !IsEmpty()) {
        return;
    }
    ServerStock();
}

int32 AMythicStorageContainer::ServerStock() {
    if (!HasAuthority() || StockTables.Num() == 0 || !ContainerInventory) {
        return 0;
    }

    const UGameInstance *GI = GetGameInstance();
    UMythicLootManagerSubsystem *LootManager = GI ? GI->GetSubsystem<UMythicLootManagerSubsystem>() : nullptr;
    if (!LootManager) {
        UE_LOG(Myth, Warning, TEXT("%s: cannot stock — no loot manager subsystem (server-only)."), *GetName());
        return 0;
    }

    FRandomStream Stream(FMath::Rand());

    int32 Added = 0;
    TArray<MythicContainerStock::FStockEntry> Flat;
    TArray<MythicContainerStock::FStockRoll> Rolls;

    for (const UMythicLootTable *Table : StockTables) {
        if (!Table) {
            continue;
        }

        Flat.Reset(Table->Entries.Num());
        for (const FLootTableEntry &Entry : Table->Entries) {
            MythicContainerStock::FStockEntry Out;
            Out.OverrideDropChance = Entry.OverrideDropChance;
            Out.StackMin = Entry.StackRange.Min;
            Out.StackMax = Entry.StackRange.Max;
            Out.bStackable = Entry.Item && Entry.Item->StackSizeMax > 1;
            Flat.Add(Out);
        }

        MythicContainerStock::RollStock(Flat, Table->DropChance, Table->MaxItems, StockDefaultEntryChance, Stream, Rolls);

        for (const MythicContainerStock::FStockRoll &Roll : Rolls) {
            if (!Table->Entries.IsValidIndex(Roll.EntryIndex)) {
                continue;
            }
            UItemDefinition *Def = Table->Entries[Roll.EntryIndex].Item;
            if (!Def) {
                continue;
            }
            AMythicWorldItem *Overflow = LootManager->CreateAndGive(Def, Roll.Quantity, this, nullptr, StockItemLevel);
            if (Overflow) {
                Overflow->Destroy();
                UE_LOG(Myth, Warning,
                       TEXT("%s: stock overflowed while adding %s — the container is full. Stopping this pass; give it "
                            "more slots or lower the table's MaxItems."),
                       *GetName(), *Def->GetName());
                return Added;
            }
            ++Added;
        }
    }

    UE_LOG(Myth, Verbose, TEXT("%s: stocked %d item stack(s) from %d table(s)."), *GetName(), Added, StockTables.Num());
    return Added;
}

AController *AMythicStorageContainer::ResolveController(AActor *Interactor) {
    if (AController *C = Cast<AController>(Interactor)) {
        return C;
    }
    if (const APawn *P = Cast<APawn>(Interactor)) {
        return P->GetController();
    }
    return nullptr;
}

TArray<UMythicInventoryComponent *> AMythicStorageContainer::GetAllInventoryComponents() const {
    return {ContainerInventory};
}

UAbilitySystemComponent *AMythicStorageContainer::GetSchematicsASC() const {
    return nullptr;
}

void AMythicStorageContainer::OnPrimaryInteract_Implementation(AActor *Interactor) {
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(ResolveController(Interactor));
    if (!PC) {
        return;
    }

    if (HasAuthority()) {
        if (IsActorInRange(PC->GetPawn())) {
            Server_AddOpener(PC);
        }
    }
    else {
        PC->ServerInteractPrimary(this);
    }

    if (PC->IsLocalController()) {
        PC->ActiveContainer = this;
        OnContainerOpened(PC);
    }
}

void AMythicStorageContainer::OnSecondaryInteract_Implementation(AActor *Interactor) {
}

USceneComponent *AMythicStorageContainer::GetWidgetAttachmentComponent_Implementation() const {
    return SceneRoot;
}

bool AMythicStorageContainer::GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const {
    OutInteractionData.InputActionDataTable = InputActionDataTable;
    OutInteractionData.PrimaryInteractionName = PrimaryInteractionName;
    return true;
}

void AMythicStorageContainer::OnFocused_Implementation(AActor *Interactor) {
}

void AMythicStorageContainer::OnUnfocused_Implementation(AActor *Interactor) {
}

bool AMythicStorageContainer::IsActorInRange(const AActor *Actor) const {
    if (ServerUseRangeSq <= 0.0f) {
        return true;
    }
    if (!Actor) {
        return false;
    }
    return FVector::DistSquared(Actor->GetActorLocation(), GetActorLocation()) <= ServerUseRangeSq;
}

void AMythicStorageContainer::Server_AddOpener(AMythicPlayerController *PC) {
    if (PC && HasAuthority()) {
        Openers.Add(PC);
    }
}

bool AMythicStorageContainer::Server_IsOpener(const AMythicPlayerController *PC) const {
    return PC && Openers.Contains(const_cast<AMythicPlayerController *>(PC));
}

void AMythicStorageContainer::Server_RemoveOpener(AMythicPlayerController *PC) {
    if (PC) {
        Openers.Remove(PC);
    }
}

void AMythicStorageContainer::SerializeCustomData(TArray<uint8> &OutCustomData) {
    if (!ContainerInventory) {
        return;
    }

    FSerializedInventoryData Data;
    FSerializedInventoryData::Serialize(ContainerInventory, Data);

    FMemoryWriter MemWriter(OutCustomData);
    FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
    FSerializedInventoryData::StaticStruct()->SerializeItem(Ar, &Data, nullptr);
}

void AMythicStorageContainer::DeserializeCustomData(const TArray<uint8> &InCustomData) {
    if (InCustomData.Num() == 0 || !ContainerInventory) {
        return;
    }

    FMemoryReader MemReader(InCustomData);
    FObjectAndNameAsStringProxyArchive Ar(MemReader, false);

    FSerializedInventoryData Data;
    FSerializedInventoryData::StaticStruct()->SerializeItem(Ar, &Data, nullptr);

    FSerializedInventoryData::Deserialize(ContainerInventory, Data);
}
