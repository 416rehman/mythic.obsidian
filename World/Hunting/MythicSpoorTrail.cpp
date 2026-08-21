
#include "World/Hunting/MythicSpoorTrail.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "EngineUtils.h"

#include "World/Hunting/MythicTags_Hunting.h"
#include "Player/MythicPlayerController.h"
#include "Mythic.h"

AMythicSpoorTrail::AMythicSpoorTrail() {
    PrimaryActorTick.bCanEverTick = false;

    bReplicates = true;
    SetNetDormancy(DORM_Initial);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    InteractionBounds = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionBounds"));
    InteractionBounds->SetupAttachment(SceneRoot);
    InteractionBounds->InitSphereRadius(60.0f);
    InteractionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(SceneRoot);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AMythicSpoorTrail::ServerInitTrailNode(const FVector &InAnchorLocation, int32 InStepsRemaining, const FMythicSpoorConfig &InConfig,
                                            bool bRainingAtSpawn) {
    if (!HasAuthority()) {
        return;
    }
    AnchorLocation = InAnchorLocation;
    StepsRemaining = FMath::Max(0, InStepsRemaining);
    Config = InConfig;
    SpawnServerTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

    EffectiveLifetimeSeconds = FMythicSpoorRules::EffectiveLifetime(Config.NodeLifetimeSeconds, bRainingAtSpawn, Config.RainLifetimeMultiplier);
    SetLifeSpan(EffectiveLifetimeSeconds);

    FlushNetDormancy();
}

float AMythicSpoorTrail::GetFreshness() const {
    const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : SpawnServerTime;
    return FMythicSpoorRules::FreshnessAtAge(static_cast<float>(Now - SpawnServerTime), EffectiveLifetimeSeconds);
}

int32 AMythicSpoorTrail::CountNodesNear(const UWorld *World, const FVector &Location, float RadiusCm) {
    if (!World) {
        return 0;
    }
    int32 Count = 0;
    const float RadiusSq = FMath::Square(FMath::Max(1.0f, RadiusCm));
    for (TActorIterator<AMythicSpoorTrail> It(const_cast<UWorld *>(World)); It; ++It) {
        if (IsValid(*It) && FVector::DistSquared(It->GetActorLocation(), Location) <= RadiusSq) {
            ++Count;
        }
    }
    return Count;
}

EMythicSpoorReadResult AMythicSpoorTrail::ServerHandleRead() {
    if (!HasAuthority() || bConsumed) {
        return EMythicSpoorReadResult::Cold;
    }
    bConsumed = true;

    EMythicSpoorReadResult Result = EMythicSpoorReadResult::Cold;
    UWorld *World = GetWorld();

    if (!FMythicSpoorRules::IsStale(GetFreshness(), Config.StaleFreshnessThreshold) && World) {
        if (StepsRemaining <= 0 || FMythicSpoorRules::IsFinalStep(GetActorLocation(), AnchorLocation, Config.StepDistanceCm)) {
            Result = EMythicSpoorReadResult::TrailEnd;
        }
        else if (CountNodesNear(World, GetActorLocation(), 15000.0f) >= FMath::Max(1, Config.MaxNodesPerRegion)) {
            Result = EMythicSpoorReadResult::Capped;
            UE_LOG(Myth, Verbose, TEXT("SpoorTrail: region node cap reached — reveal suppressed at %s"), *GetActorLocation().ToCompactString());
        }
        else {
            const FVector NextLoc = FMythicSpoorRules::NextStepLocation(GetActorLocation(), AnchorLocation, Config.StepDistanceCm,
                                                                        Config.StepJitterDegrees, FMath::FRand());
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            if (AMythicSpoorTrail *Next = World->SpawnActor<AMythicSpoorTrail>(GetClass(), FTransform(NextLoc), Params)) {
                Next->ServerInitTrailNode(AnchorLocation, StepsRemaining - 1, Config, false);
                Result = EMythicSpoorReadResult::Revealed;
            }
        }
    }

    SetLifeSpan(0.5f);
    return Result;
}

void AMythicSpoorTrail::OnPrimaryInteract_Implementation(AActor *Interactor) {
    if (!Interactor) {
        return;
    }
    if (!HasAuthority()) {
        AMythicPlayerController *PC = Cast<AMythicPlayerController>(Interactor);
        if (!PC) {
            if (const APawn *InteractorPawn = Cast<APawn>(Interactor)) {
                PC = Cast<AMythicPlayerController>(InteractorPawn->GetController());
            }
        }
        if (PC && PC->IsLocalController()) {
            PC->ServerInteractPrimary(this);
        }
        return;
    }
}

USceneComponent *AMythicSpoorTrail::GetWidgetAttachmentComponent_Implementation() const {
    return SceneRoot;
}

bool AMythicSpoorTrail::GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const {
    OutInteractionData.InputActionDataTable = InputActionDataTable;
    OutInteractionData.PrimaryInteractionName = PrimaryInteractionName;
    return !bConsumed;
}
