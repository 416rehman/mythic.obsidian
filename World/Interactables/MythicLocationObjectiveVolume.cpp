// Mythic — location-objective trigger volume implementation. See the header for the design contract.

#include "MythicLocationObjectiveVolume.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GAS/MythicTags_GAS.h"

AMythicLocationObjectiveVolume::AMythicLocationObjectiveVolume() {
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = false; // server-only trigger — it emits a GAS event; it owns no replicated state of its own

    Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
    SetRootComponent(Trigger);
    Trigger->SetBoxExtent(FVector(200.0f, 200.0f, 200.0f));
    Trigger->SetCollisionProfileName(TEXT("Trigger")); // query-only, overlap pawns
    Trigger->SetGenerateOverlapEvents(true);
}

void AMythicLocationObjectiveVolume::BeginPlay() {
    Super::BeginPlay();
    // Overlap detection + the event emit are server-only. NOTE: a bReplicates=false actor reports HasAuthority()==true on
    // clients too (the engine never demotes a non-replicated actor's role), so gate the bind on the NET MODE — only the
    // server (listen/dedicated/standalone) binds; a remote client never does, and never fires the event.
    if (GetNetMode() != NM_Client && Trigger) {
        Trigger->OnComponentBeginOverlap.AddDynamic(this, &AMythicLocationObjectiveVolume::OnVolumeBeginOverlap);
    }
}

bool AMythicLocationObjectiveVolume::ShouldEmitReachEvent(bool bHasAuthority, bool bResolvedPlayerASC, bool bAlreadyFiredForPlayer) {
    return bHasAuthority && bResolvedPlayerASC && !bAlreadyFiredForPlayer;
}

void AMythicLocationObjectiveVolume::OnVolumeBeginOverlap(UPrimitiveComponent * /*OverlappedComp*/, AActor *OtherActor,
                                                          UPrimitiveComponent * /*OtherComp*/, int32 /*OtherBodyIndex*/,
                                                          bool /*bFromSweep*/, const FHitResult & /*Sweep*/) {
    if (!LocationTag.IsValid()) {
        return; // an unconfigured volume reports nothing
    }

    // Resolve the overlapping player's controller + the ASC the ObjectiveTracker listens on (the controller's ASC,
    // forwarded to the PlayerState). A non-player overlapper (creature / projectile) resolves no ASC and is ignored.
    const APawn *Pawn = Cast<APawn>(OtherActor);
    AController *Controller = Pawn ? Pawn->GetController() : nullptr;
    UAbilitySystemComponent *ASC = nullptr;
    if (const IAbilitySystemInterface *ASI = Cast<IAbilitySystemInterface>(Controller)) {
        ASC = ASI->GetAbilitySystemComponent();
    }

    // Authority that is meaningful for a non-replicated actor: the player's ASC owner must be authoritative (mirrors
    // NotifyItemAcquired, not the actor's HasAuthority which is true on clients here). Belt-and-braces with the
    // server-only bind above.
    const bool bServerAuthoritativeForPlayer = ASC && ASC->IsOwnerActorAuthoritative();
    const bool bAlreadyFired = Controller && FiredControllers.Contains(Controller);
    if (!ShouldEmitReachEvent(bServerAuthoritativeForPlayer, ASC != nullptr, bAlreadyFired)) {
        return;
    }

    FiredControllers.Add(Controller); // per-player one-shot for this volume

    // Push the reach through the same GAS event bus the ObjectiveTracker already listens on (mirrors NotifyItemAcquired).
    FGameplayEventData Payload;
    Payload.EventTag = GAS_EVENT_REACHED_LOCATION;
    Payload.Instigator = Pawn;
    Payload.Target = ASC->GetAvatarActor();
    Payload.TargetTags.AddTag(LocationTag); // the objective's RequiredPayloadTag filters on this
    Payload.EventMagnitude = 1.0f;
    ASC->HandleGameplayEvent(GAS_EVENT_REACHED_LOCATION, &Payload);
}
