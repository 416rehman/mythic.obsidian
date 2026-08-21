
#include "MythicGrave.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Player/MythicPlayerController.h"
#include "Net/UnrealNetwork.h"

AMythicGrave::AMythicGrave() {
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetNetCullDistanceSquared(FMath::Square(8000.f));

    NetDormancy = DORM_DormantAll;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    InteractionBounds = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionBounds"));
    InteractionBounds->SetupAttachment(SceneRoot);
    InteractionBounds->InitSphereRadius(80.0f);
    InteractionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionBounds->SetCollisionObjectType(ECC_WorldDynamic);
    InteractionBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    GraveMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GraveMesh"));
    GraveMesh->SetupAttachment(SceneRoot);
    GraveMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AMythicGrave::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AMythicGrave, Epitaph);
    DOREPLIFETIME(AMythicGrave, DisplayName);
    DOREPLIFETIME(AMythicGrave, SourceNameHash);
    DOREPLIFETIME(AMythicGrave, Faction);
    DOREPLIFETIME(AMythicGrave, RoleTag);
    DOREPLIFETIME(AMythicGrave, SourceTier);
    DOREPLIFETIME(AMythicGrave, DeathTime);
    DOREPLIFETIME(AMythicGrave, KillerNameHash);
    DOREPLIFETIME(AMythicGrave, CemeteryKey);
    DOREPLIFETIME(AMythicGrave, bRaisable);
    DOREPLIFETIME(AMythicGrave, bAlreadyRaised);
}

void AMythicGrave::ServerInitializeFromDeath(const FMythicGraveIdentity &Identity, const FTransform &GraveTransform) {
    if (!HasAuthority()) {
        return;
    }
    SetActorTransform(GraveTransform);

    SourceNameHash = Identity.SourceNameHash;
    DisplayName = Identity.DisplayName;
    Epitaph = Identity.Epitaph;
    Faction = Identity.Faction;
    RoleTag = Identity.RoleTag;
    SourceTier = Identity.SourceTier;
    DeathTime = Identity.DeathTime;
    KillerNameHash = Identity.KillerNameHash;
    CemeteryKey = Identity.CemeteryKey;
    bAlreadyRaised = false;
    bRaisable = true;

    OnGraveIdentityChanged();
    FlushNetDormancy();
}

void AMythicGrave::OnRep_Identity() {
    OnGraveIdentityChanged();
}

AController *AMythicGrave::ResolveController(AActor *Interactor) {
    if (AController *C = Cast<AController>(Interactor)) {
        return C;
    }
    if (const APawn *P = Cast<APawn>(Interactor)) {
        return P->GetController();
    }
    return nullptr;
}

bool AMythicGrave::CanBeRaised() const {
    return bRaisable && !bAlreadyRaised;
}

void AMythicGrave::ServerMarkRaised() {
    if (!HasAuthority()) {
        return;
    }
    bAlreadyRaised = true;
    FlushNetDormancy();
}


void AMythicGrave::OnPrimaryInteract_Implementation(AActor *Interactor) {
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(ResolveController(Interactor));
    if (!PC) {
        return;
    }

    if (HasAuthority()) {
        if (!IsActorInRange(PC->GetPawn())) {
            return;
        }
    }
    else {
        PC->ServerInteractPrimary(this);
    }

    if (PC->IsLocalController()) {
        OnEpitaphRead(PC);
    }
}

void AMythicGrave::OnSecondaryInteract_Implementation(AActor *Interactor) {
}

USceneComponent *AMythicGrave::GetWidgetAttachmentComponent_Implementation() const {
    return SceneRoot;
}

bool AMythicGrave::GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const {
    OutInteractionData.InputActionDataTable = InputActionDataTable;
    OutInteractionData.PrimaryInteractionName = PrimaryInteractionName;
    return true;
}

void AMythicGrave::OnFocused_Implementation(AActor *Interactor) {
}

void AMythicGrave::OnUnfocused_Implementation(AActor *Interactor) {
}

bool AMythicGrave::IsActorInRange(const AActor *Actor) const {
    if (ServerUseRangeSq <= 0.0f) {
        return true;
    }
    if (!Actor) {
        return false;
    }
    return FVector::DistSquared(Actor->GetActorLocation(), GetActorLocation()) <= ServerUseRangeSq;
}
