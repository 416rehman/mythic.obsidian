
#include "World/LivingWorld/Crime/MythicCrimeConsequenceSubsystem.h"
#include "World/LivingWorld/Crime/CrimeTypes.h"
#include "World/LivingWorld/Events/ActionEventSubsystem.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "Player/MythicPlayerRegistrySubsystem.h"
#include "Player/MythicPlayerState.h"
#include "Player/MythicFactionStandingComponent.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "AI/NPCs/MythicAIController.h"
#include "AI/Cognition/CognitiveBrainComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"

bool UMythicCrimeConsequenceSubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    const UWorld *World = Cast<UWorld>(Outer);
    if (!World || !World->IsGameWorld()) {
        return false;
    }
    return World->GetNetMode() != NM_Client;
}

void UMythicCrimeConsequenceSubsystem::OnWorldBeginPlay(UWorld &InWorld) {
    Super::OnWorldBeginPlay(InWorld);

    if (InWorld.GetNetMode() == NM_Client) {
        return;
    }

    InWorld.GetTimerManager().SetTimer(
        DrainTimerHandle,
        this,
        &UMythicCrimeConsequenceSubsystem::DrainCrimeQueue,
        DrainIntervalSeconds,
true,
DrainIntervalSeconds);

    UE_LOG(LogMythLivingWorld, Log, TEXT("CrimeConsequenceSubsystem: crime→notoriety drain armed (interval=%.1fs, cap=%d/drain)"),
           DrainIntervalSeconds, MaxCrimesPerDrain);
}

void UMythicCrimeConsequenceSubsystem::Deinitialize() {
    if (const UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(DrainTimerHandle);
    }
    LastGuardDispatchTime.Empty();
    Super::Deinitialize();
}


float UMythicCrimeConsequenceSubsystem::ComputeNotorietyDelta(EMythicMoralSeverity Severity, float BaseDelta) {
    switch (Severity) {
    case EMythicMoralSeverity::Hostile:
        return 2.0f * BaseDelta;
    case EMythicMoralSeverity::Condemn:
        return 1.0f * BaseDelta;
    case EMythicMoralSeverity::Disapprove:
        return 0.5f * BaseDelta;
    case EMythicMoralSeverity::Ignore:
    default:
        return 0.0f;
    }
}

bool UMythicCrimeConsequenceSubsystem::ShouldDispatchGuards(float Notoriety, float Threshold, bool bAlreadyDispatched) {
    return !bAlreadyDispatched && Notoriety >= Threshold;
}

FString UMythicCrimeConsequenceSubsystem::MakeDispatchKey(const FString &PlayerKey, FMythicFactionId Faction) {
    return FString::Printf(TEXT("%s|%d"), *PlayerKey, static_cast<int32>(Faction.Index));
}


void UMythicCrimeConsequenceSubsystem::DrainCrimeQueue() {
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }

    if (!CachedActionSubsystem.IsValid()) {
        CachedActionSubsystem = World->GetSubsystem<UMythicActionEventSubsystem>();
    }
    UMythicActionEventSubsystem *ActionSub = CachedActionSubsystem.Get();
    if (!ActionSub) {
        return;
    }

    FMythicCrimeReportQueue &CrimeQueue = ActionSub->GetCrimeReportQueue();
    if (CrimeQueue.IsEmpty()) {
        return;
    }

    if (!CachedPlayerRegistry.IsValid()) {
        CachedPlayerRegistry = World->GetSubsystem<UMythicPlayerRegistrySubsystem>();
    }
    UMythicPlayerRegistrySubsystem *Registry = CachedPlayerRegistry.Get();

    const double Now = World->GetTimeSeconds();
    int32 Processed = 0;

    for (FMythicCrimeRecord &Crime : CrimeQueue.PendingReports) {
        if (Crime.bPropagated) {
            continue;
        }
        if (Processed >= MaxCrimesPerDrain) {
            break;
        }
        ++Processed;
        Crime.bPropagated = true;

        if (Crime.PerpPlayerKey.IsEmpty()) {
            continue;
        }
        if (!Crime.ViolatedFaction.IsValid()) {
            continue;
        }
        AMythicPlayerState *PerpPS = Registry ? Registry->GetPlayerStateForKey(Crime.PerpPlayerKey) : nullptr;
        if (!PerpPS) {
            continue;
        }
        UMythicFactionStandingComponent *Standing = PerpPS->GetFactionStanding();
        if (!Standing) {
            continue;
        }

        const float Delta = ComputeNotorietyDelta(Crime.Severity, NotorietyBaseDelta);
        if (Delta <= 0.0f) {
            continue;
        }
        Standing->ServerAdjustStanding(Crime.ViolatedFaction, -Delta);

        const float NotorietyNow = -Standing->GetStanding(Crime.ViolatedFaction);
        const float DispatchThreshold = -Standing->GetHostileThreshold();
        const FString DispatchKey = MakeDispatchKey(Crime.PerpPlayerKey, Crime.ViolatedFaction);
        const double *LastDispatch = LastGuardDispatchTime.Find(DispatchKey);
        const bool bAlreadyDispatched = LastDispatch && (Now - *LastDispatch) < GuardDispatchCooldownSeconds;

        if (ShouldDispatchGuards(NotorietyNow, DispatchThreshold, bAlreadyDispatched)) {
            APawn *PerpPawn = Registry ? Registry->GetPawnForKey(Crime.PerpPlayerKey) : nullptr;
            const int32 Roused = DispatchGuardResponse(PerpPawn, Crime.ViolatedFaction);
            LastGuardDispatchTime.Add(DispatchKey, Now);
            UE_LOG(LogMythLivingWorld, Log,
                   TEXT("CrimeConsequence: player '%s' crossed notoriety %.0f/%.0f with faction %d → roused %d guard(s)"),
                   *Crime.PerpPlayerKey, NotorietyNow, DispatchThreshold, static_cast<int32>(Crime.ViolatedFaction.Index), Roused);
        }
    }

    CrimeQueue.FlushPropagated();

    if (Processed >= MaxCrimesPerDrain && !CrimeQueue.IsEmpty()) {
        UE_LOG(LogMythLivingWorld, Warning,
               TEXT("CrimeConsequence: drain capped at %d crimes; %d still queued (will drain next tick)"),
               MaxCrimesPerDrain, CrimeQueue.Num());
    }
}

int32 UMythicCrimeConsequenceSubsystem::DispatchGuardResponse(APawn *PerpPawn, FMythicFactionId OffendedFaction) {
    UWorld *World = GetWorld();
    if (!World || !IsValid(PerpPawn) || !OffendedFaction.IsValid()) {
        return 0;
    }

    const float RadiusSq = GuardAlertRadius * GuardAlertRadius;
    const FVector PerpLoc = PerpPawn->GetActorLocation();
    int32 Roused = 0;

    for (TActorIterator<AMythicNPCCharacter> It(World); It; ++It) {
        if (Roused >= GuardAlertMaxResponders) {
            break;
        }
        AMythicNPCCharacter *Responder = *It;
        if (!IsValid(Responder)) {
            continue;
        }
        if (GuardAlertRadius > 0.0f && FVector::DistSquared(PerpLoc, Responder->GetActorLocation()) > RadiusSq) {
            continue;
        }
        UMythicCognitiveBrainComponent *Brain = Responder->FindComponentByClass<UMythicCognitiveBrainComponent>();
        if (!Brain || Brain->GetFaction() != OffendedFaction) {
            continue;
        }
        AMythicAIController *RespAI = Cast<AMythicAIController>(Responder->GetController());
        if (!RespAI) {
            continue;
        }
        Brain->OnSignificantEvent(TAG_LIVINGWORLD_ACTION_VIOLENCE_ATTACK, Brain->GetHomeCell());
        RespAI->ForceEngageTarget(PerpPawn);
        ++Roused;
    }

    return Roused;
}
