
#include "World/Camping/MythicCampfireComponent.h"

#include "World/Camping/MythicCampfireFuel.h"
#include "World/Camping/MythicCampsiteSubsystem.h"
#include "World/Survival/MythicWarmthAuraComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Mythic.h"

UMythicCampfireComponent::UMythicCampfireComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UMythicCampfireComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UMythicCampfireComponent, bLit);
}

void UMythicCampfireComponent::BeginPlay() {
    Super::BeginPlay();
    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }
    ApplyDeadline(MythicCampfireFuel::AddFuelSeconds(0.0, WorldNow(), InitialFuelSeconds, MaxFuelSeconds));
}

void UMythicCampfireComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (const UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(BurnOutTimer);
    }
    Super::EndPlay(EndPlayReason);
}

double UMythicCampfireComponent::WorldNow() const {
    const UWorld *World = GetWorld();
    return World ? World->GetTimeSeconds() : 0.0;
}

float UMythicCampfireComponent::GetRemainingBurnSeconds() const {
    return static_cast<float>(MythicCampfireFuel::RemainingBurnSeconds(BurnDeadline, WorldNow()));
}

void UMythicCampfireComponent::ServerAddFuel(float Seconds) {
    if (!GetOwner() || !GetOwner()->HasAuthority() || Seconds <= 0.0f) {
        return;
    }
    ApplyDeadline(MythicCampfireFuel::AddFuelSeconds(BurnDeadline, WorldNow(), Seconds, MaxFuelSeconds));
    UE_LOG(Myth, Log, TEXT("Campfire %s: fuel added (+%.0fs → %.0fs remaining)"), *GetNameSafe(GetOwner()), Seconds,
           GetRemainingBurnSeconds());
}

void UMythicCampfireComponent::ApplyDeadline(double NewDeadline) {
    BurnDeadline = NewDeadline;
    const double Now = WorldNow();
    const double Remaining = MythicCampfireFuel::RemainingBurnSeconds(BurnDeadline, Now);

    SetLit(Remaining > 0.0);

    if (UWorld *World = GetWorld()) {
        if (Remaining > 0.0) {
            World->GetTimerManager().SetTimer(BurnOutTimer, this, &UMythicCampfireComponent::HandleBurnOut,
                                              FMath::Max(0.05f, static_cast<float>(Remaining)), false);
        }
        else {
            World->GetTimerManager().ClearTimer(BurnOutTimer);
        }
    }
}

void UMythicCampfireComponent::HandleBurnOut() {
    const double Remaining = MythicCampfireFuel::RemainingBurnSeconds(BurnDeadline, WorldNow());
    if (Remaining > 0.0) {
        ApplyDeadline(BurnDeadline);
        return;
    }
    SetLit(false);
    UE_LOG(Myth, Log, TEXT("Campfire %s: burned out"), *GetNameSafe(GetOwner()));
}

void UMythicCampfireComponent::SetLit(bool bNewLit) {
    if (bLit == bNewLit) {
        SyncWarmthAura();
        return;
    }
    bLit = bNewLit;
    SyncWarmthAura();
    OnLitChanged.Broadcast(bLit);

    if (const UWorld *World = GetWorld()) {
        if (UMythicCampsiteSubsystem *Camps = World->GetSubsystem<UMythicCampsiteSubsystem>()) {
            Camps->NotifyCampStateChanged();
        }
    }
}

void UMythicCampfireComponent::SyncWarmthAura() {
    AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    if (UMythicWarmthAuraComponent *Aura = Owner->FindComponentByClass<UMythicWarmthAuraComponent>()) {
        Aura->SetCollisionEnabled(bLit ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
    }
}

void UMythicCampfireComponent::OnRep_Lit() {
    OnLitChanged.Broadcast(bLit);
}

void UMythicCampfireComponent::SerializeFuelState(TArray<uint8> &OutData) const {
    MythicCampfireFuel::SerializeFuel(OutData, MythicCampfireFuel::RemainingBurnSeconds(BurnDeadline, WorldNow()));
}

void UMythicCampfireComponent::RestoreFuelState(const TArray<uint8> &InData) {
    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }
    const double Remaining = MythicCampfireFuel::DeserializeFuel(InData, InitialFuelSeconds);
    ApplyDeadline(MythicCampfireFuel::AddFuelSeconds(0.0, WorldNow(), Remaining, MaxFuelSeconds));
}
