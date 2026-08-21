
#include "World/POI/MythicLandmarkPOI.h"

#include "World/POI/MythicPOIDiscoverySubsystem.h"
#include "Player/MythicPlayerState.h"
#include "Progression/MythicStatLedgerComponent.h"
#include "Progression/MythicTags_MetaProgression.h"
#include "World/POI/MythicPOIDiscoveryRules.h"
#include "World/POI/MythicTags_POI.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/Feedback/MythicTags_FeedbackCues.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

AMythicLandmarkPOI::AMythicLandmarkPOI() {
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = false;

    DiscoverySphere = CreateDefaultSubobject<USphereComponent>(TEXT("DiscoverySphere"));
    SetRootComponent(DiscoverySphere);
    DiscoverySphere->InitSphereRadius(DiscoveryRadius);
    DiscoverySphere->SetCollisionProfileName(TEXT("Trigger"));
    DiscoverySphere->SetGenerateOverlapEvents(true);

    POITag = TAG_POI_Landmark;
}

#if WITH_EDITOR
void AMythicLandmarkPOI::OnConstruction(const FTransform &Transform) {
    Super::OnConstruction(Transform);
    if (DiscoverySphere) {
        DiscoverySphere->SetSphereRadius(DiscoveryRadius);
    }
}
#endif

void AMythicLandmarkPOI::BeginPlay() {
    Super::BeginPlay();

    if (DiscoverySphere) {
        DiscoverySphere->SetSphereRadius(DiscoveryRadius);
    }

    if (GetNetMode() != NM_Client && DiscoverySphere) {
        DiscoverySphere->OnComponentBeginOverlap.AddDynamic(this, &AMythicLandmarkPOI::OnDiscoverySphereBeginOverlap);
    }
}

void AMythicLandmarkPOI::OnDiscoverySphereBeginOverlap(UPrimitiveComponent *, AActor *OtherActor,
                                                       UPrimitiveComponent *, int32,
                                                       bool, const FHitResult &) {
    if (POIId < 0) {
        return;
    }

    const APawn *Pawn = Cast<APawn>(OtherActor);
    const bool bIsPlayer = Pawn && Pawn->GetController() && Pawn->GetController()->IsPlayerController();

    UGameInstance *GI = GetGameInstance();
    UMythicPOIDiscoverySubsystem *Sub = GI ? GI->GetSubsystem<UMythicPOIDiscoverySubsystem>() : nullptr;
    if (!Sub) {
        return;
    }

    const bool bHasAuthority = GetNetMode() != NM_Client;
    const bool bAlreadyUnlocked = Sub->IsPOIUnlocked(POIId);
    if (!MythicPOIDiscovery::ShouldRegisterDiscovery(bHasAuthority, bIsPlayer, bAlreadyUnlocked)) {
        return;
    }

    Sub->ServerUnlockPOI(POIId, GetActorLocation(), POITag, DisplayName, DiscoveryRadius);

    // Credited to whoever walked in. ShouldRegisterDiscovery already rejects a repeat, so this counts each
    // landmark once.
    if (const AController *Discoverer = Pawn ? Pawn->GetController() : nullptr) {
        if (const AMythicPlayerState *PS = Discoverer->GetPlayerState<AMythicPlayerState>()) {
            if (UMythicStatLedgerComponent *Ledger = PS->GetStatLedgerComponent()) {
                Ledger->RecordStat(STAT_DISCOVERY);
            }
        }
    }

    if (UMythicAbilitySystemComponent *DiscovererASC =
            Cast<UMythicAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))) {
        FGameplayCueParameters CueParams;
        CueParams.Location = GetActorLocation();
        CueParams.Instigator = OtherActor;
        DiscovererASC->ExecuteGameplayCueMulticast(TAG_GameplayCue_World_POIDiscovered, CueParams);
    }
}
