#include "World/LivingWorld/Chronicle/MythicChronicleRelayComponent.h"

#include "World/LivingWorld/Chronicle/MythicWorldChronicleSubsystem.h"
#include "GameFramework/Actor.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

UMythicChronicleRelayComponent::UMythicChronicleRelayComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UMythicChronicleRelayComponent::BeginPlay() {
    Super::BeginPlay();

    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }
    UMythicWorldChronicleSubsystem *Chronicle = ResolveChronicle();
    if (!Chronicle) {
        return;
    }
    Chronicle->OnChronicleEntry.AddDynamic(this, &UMythicChronicleRelayComponent::HandleChronicleEntry);
    ReplicatedChronicle = Chronicle->GetRecentChronicle(MaxRelayEntries);
}

void UMythicChronicleRelayComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (GetOwner() && GetOwner()->HasAuthority()) {
        if (UMythicWorldChronicleSubsystem *Chronicle = ResolveChronicle()) {
            Chronicle->OnChronicleEntry.RemoveDynamic(this, &UMythicChronicleRelayComponent::HandleChronicleEntry);
        }
    }
    Super::EndPlay(EndPlayReason);
}

void UMythicChronicleRelayComponent::HandleChronicleEntry(const FMythicChronicleEntry &Entry) {
    ReplicatedChronicle.Add(Entry);
    if (ReplicatedChronicle.Num() > MaxRelayEntries) {
        ReplicatedChronicle.RemoveAt(0, ReplicatedChronicle.Num() - MaxRelayEntries, EAllowShrinking::No);
    }
}

void UMythicChronicleRelayComponent::OnRep_ReplicatedChronicle() {
    UMythicWorldChronicleSubsystem *Chronicle = ResolveChronicle();
    if (!Chronicle) {
        return;
    }
    for (const FMythicChronicleEntry &Entry : ReplicatedChronicle) {
        if (Entry.Sequence > LastIngestedSequence) {
            Chronicle->IngestReplicatedEntry(Entry);
            LastIngestedSequence = Entry.Sequence;
        }
    }
}

UMythicWorldChronicleSubsystem *UMythicChronicleRelayComponent::ResolveChronicle() const {
    if (const UWorld *World = GetWorld()) {
        if (UGameInstance *GI = World->GetGameInstance()) {
            return GI->GetSubsystem<UMythicWorldChronicleSubsystem>();
        }
    }
    return nullptr;
}

void UMythicChronicleRelayComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMythicChronicleRelayComponent, ReplicatedChronicle, COND_OwnerOnly);
}
