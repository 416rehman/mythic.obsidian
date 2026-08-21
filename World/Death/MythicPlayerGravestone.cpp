
#include "MythicPlayerGravestone.h"

#include "MythicDeathStakeSettings.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Mythic.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerState.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Loot/MythicLootManagerSubsystem.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

AMythicPlayerGravestone::AMythicPlayerGravestone() {
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetNetCullDistanceSquared(FMath::Square(8000.f));

    NetDormancy = DORM_Initial;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    InteractionBounds = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionBounds"));
    InteractionBounds->SetupAttachment(SceneRoot);
    InteractionBounds->InitSphereRadius(80.0f);
    InteractionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionBounds->SetCollisionObjectType(ECC_WorldDynamic);
    InteractionBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    GravestoneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GravestoneMesh"));
    GravestoneMesh->SetupAttachment(SceneRoot);
    GravestoneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AMythicPlayerGravestone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AMythicPlayerGravestone, StakedGold);
    DOREPLIFETIME(AMythicPlayerGravestone, OwnerPlayerKey);
    DOREPLIFETIME(AMythicPlayerGravestone, OwnerPlayerId);
    DOREPLIFETIME(AMythicPlayerGravestone, OwnerDisplayName);
    DOREPLIFETIME(AMythicPlayerGravestone, DeathTime);
}

void AMythicPlayerGravestone::BeginPlay() {
    Super::BeginPlay();
}

void AMythicPlayerGravestone::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (UWorld *W = GetWorld()) {
        W->GetTimerManager().ClearTimer(ExpiryTimerHandle);
    }
    Super::EndPlay(EndPlayReason);
}


void AMythicPlayerGravestone::ServerInitializeStake(AMythicPlayerState *OwnerPS, int32 Amount, UItemDefinition *InCurrencyDef,
                                                    const FTransform &DeathTransform) {
    if (!HasAuthority()) {
        return;
    }
    SetActorTransform(DeathTransform);

    StakedGold = FMath::Max(0, Amount);
    CurrencyDef = InCurrencyDef;
    if (OwnerPS) {
        OwnerPlayerKey = OwnerPS->GetCanonicalPlayerKey();
        OwnerPlayerId = OwnerPS->GetPlayerId();
        OwnerDisplayName = FText::FromString(OwnerPS->GetPlayerName());
    }
    if (const UWorld *W = GetWorld()) {
        DeathTime = W->GetTimeSeconds();
    }

    OnGravestoneInitialized();
    StartOrResumeExpiry();
    FlushNetDormancy();
}

void AMythicPlayerGravestone::OnRep_Init() {
    OnGravestoneInitialized();
}


float AMythicPlayerGravestone::GetAgeSeconds() const {
    const UWorld *W = GetWorld();
    const float Now = W ? W->GetTimeSeconds() : 0.0f;
    return FMath::Max(0.0f, Now - DeathTime);
}

void AMythicPlayerGravestone::StartOrResumeExpiry() {
    if (!HasAuthority() || bExpiryStarted) {
        return;
    }
    const UMythicDeathStakeSettings *Settings = GetDefault<UMythicDeathStakeSettings>();
    LifetimeSeconds = Settings ? Settings->Config.GravestoneLifetimeSeconds : 1200.0f;
    if (LifetimeSeconds <= 0.0f) {
        return;
    }
    bExpiryStarted = true;
    CheckExpiry();
}

void AMythicPlayerGravestone::CheckExpiry() {
    if (!HasAuthority()) {
        return;
    }
    UWorld *W = GetWorld();
    if (!W) {
        return;
    }
    const float Age = GetAgeSeconds();
    if (Age >= LifetimeSeconds) {
        Destroy();
        return;
    }
    const float Delay = FMath::Max(0.5f, LifetimeSeconds - Age);
    W->GetTimerManager().SetTimer(ExpiryTimerHandle, this, &AMythicPlayerGravestone::CheckExpiry, Delay,false);
}


void AMythicPlayerGravestone::ResolveRecoveryEligibility(AMythicPlayerController *PC, bool &bOutIsOwner, bool &bOutIsPartyMember) const {
    bOutIsOwner = false;
    bOutIsPartyMember = false;
    if (!PC) {
        return;
    }
    const AMythicPlayerState *PS = PC->GetPlayerState<AMythicPlayerState>();
    if (!PS) {
        return;
    }
    const FString Key = PS->GetCanonicalPlayerKey();
    if (!OwnerPlayerKey.IsEmpty() && !Key.IsEmpty()) {
        bOutIsOwner = (Key == OwnerPlayerKey);
    }
    else if (OwnerPlayerId >= 0) {
        bOutIsOwner = (PS->GetPlayerId() == OwnerPlayerId);
    }
    bOutIsPartyMember = !bOutIsOwner;
}

void AMythicPlayerGravestone::ServerTryRecover(AMythicPlayerController *PC) {
    if (!HasAuthority() || !PC) {
        return;
    }
    bool bIsOwner = false;
    bool bIsPartyMember = false;
    ResolveRecoveryEligibility(PC, bIsOwner, bIsPartyMember);
    const bool bInRange = IsActorInRange(PC->GetPawn());
    if (!FMythicDeathStakeRules::CanRecover(bIsOwner, bIsPartyMember, bInRange)) {
        return;
    }

    if (StakedGold > 0) {
        ServerGrantGoldTo(PC, StakedGold);
    }

    FlushNetDormancy();
    Multicast_OnRecovered(PC);
    Destroy();
}

void AMythicPlayerGravestone::ServerGrantGoldTo(AMythicPlayerController *PC, int32 Amount) {
    if (!HasAuthority() || !PC || Amount <= 0 || !CurrencyDef) {
        return;
    }
    UGameInstance *GI = GetGameInstance();
    UMythicLootManagerSubsystem *Loot = GI ? GI->GetSubsystem<UMythicLootManagerSubsystem>() : nullptr;
    if (!Loot) {
        return;
    }
    UMythicInventoryComponent *Inv = nullptr;
    for (UMythicInventoryComponent *I : PC->GetAllInventoryComponents()) {
        if (I) {
            Inv = I;
            break;
        }
    }
    if (!Inv) {
        return;
    }
    const int32 Cap = FMath::Max(1, CurrencyDef->StackSizeMax);
    const int64 MaxChunks = (static_cast<int64>(Amount) + Cap - 1) / Cap + 4;
    int32 Remaining = Amount;
    int64 Guard = 0;
    while (Remaining > 0 && Guard++ < MaxChunks) {
        const int32 Chunk = FMath::Min(Remaining, Cap);
        UMythicItemInstance *Coins = Loot->Create(CurrencyDef, Chunk, PC, 0);
        if (!Coins) {
            break;
        }
        Inv->AddItem(Coins, PC);
        Remaining -= Chunk;
    }
    if (Remaining > 0) {
        UE_LOG(Myth, Warning, TEXT("Gravestone: could not mint %d of %d staked currency on recovery"), Remaining, Amount);
    }
}

void AMythicPlayerGravestone::Multicast_OnRecovered_Implementation(APlayerController *Recoverer) {
    OnStakeRecovered(Recoverer);
}


void AMythicPlayerGravestone::OnPrimaryInteract_Implementation(AActor *Interactor) {
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(ResolveController(Interactor));
    if (!PC) {
        return;
    }

    if (HasAuthority()) {
        ServerTryRecover(PC);
    }
    else {
        PC->ServerInteractPrimary(this);
    }
}

void AMythicPlayerGravestone::OnSecondaryInteract_Implementation(AActor *Interactor) {
}

USceneComponent *AMythicPlayerGravestone::GetWidgetAttachmentComponent_Implementation() const {
    return SceneRoot;
}

bool AMythicPlayerGravestone::GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const {
    OutInteractionData.InputActionDataTable = InputActionDataTable;
    OutInteractionData.PrimaryInteractionName = PrimaryInteractionName;
    return true;
}

void AMythicPlayerGravestone::OnFocused_Implementation(AActor *Interactor) {
}

void AMythicPlayerGravestone::OnUnfocused_Implementation(AActor *Interactor) {
}

bool AMythicPlayerGravestone::IsActorInRange(const AActor *Actor) const {
    if (ServerUseRangeSq <= 0.0f) {
        return true;
    }
    if (!Actor) {
        return false;
    }
    return FVector::DistSquared(Actor->GetActorLocation(), GetActorLocation()) <= ServerUseRangeSq;
}


void AMythicPlayerGravestone::DeserializeCustomData(const TArray<uint8> &InCustomData) {
    if (HasAuthority() && !bExpiryStarted) {
        if (const UWorld *W = GetWorld()) {
            const float Now = W->GetTimeSeconds();
            if (DeathTime <= 0.0f || DeathTime > Now) {
                DeathTime = Now;
            }
        }
        StartOrResumeExpiry();
    }
}


AController *AMythicPlayerGravestone::ResolveController(AActor *Interactor) {
    if (AController *C = Cast<AController>(Interactor)) {
        return C;
    }
    if (const APawn *P = Cast<APawn>(Interactor)) {
        return P->GetController();
    }
    return nullptr;
}
