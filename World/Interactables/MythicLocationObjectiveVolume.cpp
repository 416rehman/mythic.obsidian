
#include "MythicLocationObjectiveVolume.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GAS/MythicTags_GAS.h"

AMythicLocationObjectiveVolume::AMythicLocationObjectiveVolume() {
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = false;

    Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
    SetRootComponent(Trigger);
    Trigger->SetBoxExtent(FVector(200.0f, 200.0f, 200.0f));
    Trigger->SetCollisionProfileName(TEXT("Trigger"));
    Trigger->SetGenerateOverlapEvents(true);
}

void AMythicLocationObjectiveVolume::BeginPlay() {
    Super::BeginPlay();
    if (GetNetMode() != NM_Client && Trigger) {
        Trigger->OnComponentBeginOverlap.AddDynamic(this, &AMythicLocationObjectiveVolume::OnVolumeBeginOverlap);
    }
}

bool AMythicLocationObjectiveVolume::ShouldEmitReachEvent(bool bHasAuthority, bool bResolvedPlayerASC, bool bAlreadyFiredForPlayer) {
    return bHasAuthority && bResolvedPlayerASC && !bAlreadyFiredForPlayer;
}

void AMythicLocationObjectiveVolume::OnVolumeBeginOverlap(UPrimitiveComponent *, AActor *OtherActor,
                                                          UPrimitiveComponent *, int32,
                                                          bool, const FHitResult &) {
    if (!LocationTag.IsValid()) {
        return;
    }

    const APawn *Pawn = Cast<APawn>(OtherActor);
    AController *Controller = Pawn ? Pawn->GetController() : nullptr;
    UAbilitySystemComponent *ASC = nullptr;
    if (const IAbilitySystemInterface *ASI = Cast<IAbilitySystemInterface>(Controller)) {
        ASC = ASI->GetAbilitySystemComponent();
    }

    const bool bServerAuthoritativeForPlayer = ASC && ASC->IsOwnerActorAuthoritative();
    const bool bAlreadyFired = Controller && FiredControllers.Contains(Controller);
    if (!ShouldEmitReachEvent(bServerAuthoritativeForPlayer, ASC != nullptr, bAlreadyFired)) {
        return;
    }

    FiredControllers.Add(Controller);

    FGameplayEventData Payload;
    Payload.EventTag = GAS_EVENT_REACHED_LOCATION;
    Payload.Instigator = Pawn;
    Payload.Target = ASC->GetAvatarActor();
    Payload.TargetTags.AddTag(LocationTag);
    Payload.EventMagnitude = 1.0f;
    ASC->HandleGameplayEvent(GAS_EVENT_REACHED_LOCATION, &Payload);
}
