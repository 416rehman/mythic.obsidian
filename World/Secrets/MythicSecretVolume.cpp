
#include "World/Secrets/MythicSecretVolume.h"

#include "World/Secrets/MythicSecretReveal.h"
#include "Settings/MythicDeveloperSettings.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"

AMythicSecretVolume::AMythicSecretVolume() {
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = false;

    SecretSphere = CreateDefaultSubobject<USphereComponent>(TEXT("SecretSphere"));
    SetRootComponent(SecretSphere);
    SecretSphere->InitSphereRadius(TriggerRadius);
    SecretSphere->SetCollisionProfileName(TEXT("Trigger"));
    SecretSphere->SetGenerateOverlapEvents(true);
}

#if WITH_EDITOR
void AMythicSecretVolume::OnConstruction(const FTransform &Transform) {
    Super::OnConstruction(Transform);
    if (SecretSphere) {
        SecretSphere->SetSphereRadius(TriggerRadius);
    }
}
#endif

void AMythicSecretVolume::BeginPlay() {
    Super::BeginPlay();

    if (SecretSphere) {
        SecretSphere->SetSphereRadius(TriggerRadius);
    }

    if (GetNetMode() != NM_Client && SecretSphere) {
        SecretSphere->OnComponentBeginOverlap.AddDynamic(this, &AMythicSecretVolume::OnSecretSphereBeginOverlap);
    }
}

void AMythicSecretVolume::OnSecretSphereBeginOverlap(UPrimitiveComponent *, AActor *OtherActor,
                                                     UPrimitiveComponent *, int32,
                                                     bool, const FHitResult &) {
    if (!GetDefault<UMythicDeveloperSettings>()->bSecretsEnabled) {
        return;
    }
    if (bGlobalOneShot && bGlobalConsumed) {
        return;
    }

    const APawn *Pawn = Cast<APawn>(OtherActor);
    if (!Pawn) {
        return;
    }
    AController *Controller = Pawn->GetController();
    APlayerController *PC = Cast<APlayerController>(Controller);
    if (!PC) {
        return;
    }

    const TWeakObjectPtr<AController> WeakController(Controller);
    if (RevealedControllers.Contains(WeakController)) {
        return;
    }

    const bool bRevealed = FMythicSecretReveal::TryRevealSecret(PC, Def, GetActorLocation());
    if (bRevealed) {
        RevealedControllers.Add(WeakController);
        if (bGlobalOneShot) {
            bGlobalConsumed = true;
        }
    }
}
