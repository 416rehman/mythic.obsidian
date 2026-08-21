

#include "MythicWorldItem.h"

#include "Mythic.h"
#include "NavigationSystem.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/InventoryProviderInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"


class UNavigationSystemV1;
void AMythicWorldItem::BeginPlay() {
    Super::BeginPlay();

    if (HasAuthority() && StaticMesh) {
        StaticMesh->SetGenerateOverlapEvents(true);
        StaticMesh->OnComponentBeginOverlap.AddDynamic(this, &AMythicWorldItem::OnPickupOverlap);
    }
}

void AMythicWorldItem::OnPickupOverlap(UPrimitiveComponent *OverlappedComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp,
                                       int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult) {
    if (!HasAuthority() || !bAutoPickup || !ItemInstance || IsActorBeingDestroyed()) {
        return;
    }
    if (StaticMesh && StaticMesh->IsSimulatingPhysics()) {
        return;
    }

    const UItemDefinition *Def = ItemInstance->GetItemDefinition();
    if (!Def || !ShouldAutoPickup(Def->StackSizeMax)) {
        return;
    }

    const APawn *Pawn = Cast<APawn>(OtherActor);
    if (!Pawn) {
        return;
    }
    AController *PawnController = Pawn->GetController();
    if (!PawnController || (TargetRecipient && TargetRecipient != PawnController)) {
        return;
    }

    IInventoryProviderInterface *Provider = Cast<IInventoryProviderInterface>(PawnController);
    if (!Provider) {
        return;
    }
    if (UMythicInventoryComponent *Inventory = Provider->GetInventoryForWorldItem(this)) {
        Inventory->PickupItem(this);
    }
}

AMythicWorldItem::AMythicWorldItem() {
    this->bReplicates = true;
    this->bReplicateUsingRegisteredSubObjectList = true;

    StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));

    RootComponent = StaticMesh;

    this->SetActorEnableCollision(true);
    StaticMesh->SetSimulatePhysics(false);
    StaticMesh->SetCollisionProfileName(TEXT("OverlapAll"));

    SetNetCullDistanceSquared(FMath::Square(8000.f));
}

void AMythicWorldItem::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    Super::EndPlay(EndPlayReason);

    UE_LOG(Myth, Verbose, TEXT("AMythicWorldItem::EndPlay: %s"), *GetName());
}

void AMythicWorldItem::SetItemInstance(UMythicItemInstance *ItemInst) {
    checkf(HasAuthority(), TEXT("AMythicWorldItem::SetItemInstance: Only call this on the server"));
    if (!ItemInst) {
        UE_LOG(Myth, Warning, TEXT("AMythicWorldItem::SetItemInstance: ItemInstance is null"));
        return;
    }

    ItemInst->SetOwner(this);
    this->ItemInstance = ItemInst;

    OnRep_ItemInstance();
}

void AMythicWorldItem::OnRep_ItemInstance() {
    UE_LOG(Myth, Verbose, TEXT("AMythicWorldItem::OnRep_ItemInstance: %s"), *GetName());
    OnItemInstanceUpdated();
}

void AMythicWorldItem::OnHit(UPrimitiveComponent *HitComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp, FVector NormalImpulse,
                             const FHitResult &Hit) {
    if (Hit.ImpactPoint.Z < GetActorLocation().Z || Hit.Normal.Z > 0.5f) {
        UE_LOG(Myth, Verbose, TEXT("AMythicWorldItem::OnHit: %s"), *GetName());
        StaticMesh->SetSimulatePhysics(false);
        StaticMesh->SetEnableGravity(false);
        StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        StaticMesh->SetCollisionResponseToAllChannels(ECR_Overlap);

        SetNetDormancy(DORM_DormantAll);
    }
}

void AMythicWorldItem::EmulateDropPhysics(const FVector &location, float radius) {
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

    this->StaticMesh->SetCollisionResponseToAllChannels(ECR_Block);
    this->StaticMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    this->StaticMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    this->StaticMesh->SetSimulatePhysics(true);
    this->StaticMesh->SetEnableGravity(true);
    this->StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    this->StaticMesh->SetPhysicsLinearVelocity(SuggestedVelocity);

    this->StaticMesh->SetNotifyRigidBodyCollision(true);
    this->StaticMesh->OnComponentHit.AddDynamic(this, &AMythicWorldItem::OnHit);
}

void AMythicWorldItem::OnRep_TargetRecipient() {
    if (TargetRecipient) {
        APlayerController *OwnerPlayerController = Cast<APlayerController>(GetOwner());
        if (!OwnerPlayerController) {
            UE_LOG(Myth, Warning, TEXT("AMythicWorldItem::OnRep_ForAll: Owner is not a player controller"));
            return;
        }


        auto isLocalPlayer = OwnerPlayerController->IsLocalController();
        SetActorHiddenInGame(!isLocalPlayer);
    }
    else {
        SetActorHiddenInGame(false);
    }
}

void AMythicWorldItem::SetTargetRecipient(AController *NewTargetRecipient) {
    checkf(HasAuthority(), TEXT("AMythicWorldItem::SetIsPrivate: Only call this on the server"));

    this->TargetRecipient = NewTargetRecipient;
    OnRep_TargetRecipient();
}


void AMythicWorldItem::SerializeCustomData(TArray<uint8> &OutCustomData) {
    if (!ItemInstance) {
        return;
    }

    FMemoryWriter MemWriter(OutCustomData);
    FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
    Ar.ArIsSaveGame = true;

    FSoftClassPath ItemClassPath(ItemInstance->GetClass());
    Ar << ItemClassPath;

    ItemInstance->Serialize(Ar);
}

void AMythicWorldItem::DeserializeCustomData(const TArray<uint8> &InCustomData) {
    if (InCustomData.Num() == 0) {
        return;
    }

    FMemoryReader MemReader(InCustomData);
    FObjectAndNameAsStringProxyArchive Ar(MemReader, false);
    Ar.ArIsSaveGame = true;

    FSoftClassPath ItemClassPath;
    Ar << ItemClassPath;

    UClass *ItemClass = ItemClassPath.TryLoadClass<UMythicItemInstance>();
    if (ItemClass) {
        ItemInstance = NewObject<UMythicItemInstance>(this, ItemClass);
        ItemInstance->Serialize(Ar);
        ItemInstance->SetOwner(this);

        OnRep_ItemInstance();
    }
}
