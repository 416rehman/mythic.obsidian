
#include "World/POI/MythicPOIDiscoverySubsystem.h"

#include "World/POI/MythicPOIReplicator.h"
#include "Player/FastTravel/MythicFastTravelRules.h"
#include "World/POI/MythicPOIDiscoveryRules.h"
#include "GAS/MythicTags_GAS.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/Feedback/MythicTags_FeedbackCues.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Mythic.h"

bool UMythicPOIDiscoverySubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    return true;
}

void UMythicPOIDiscoverySubsystem::Deinitialize() {
    if (IsValid(Replicator)) {
        Replicator->Destroy();
    }
    Replicator = nullptr;
    Super::Deinitialize();
}

void UMythicPOIDiscoverySubsystem::EnsureReplicator() {
    if (Replicator) {
        return;
    }
    UWorld *World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client) {
        return;
    }
    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = FName("MythicPOIReplicator");
    Replicator = World->SpawnActor<AMythicPOIReplicator>(SpawnParams);
}

void UMythicPOIDiscoverySubsystem::ServerUnlockPOI(int32 Id, FVector Anchor, FGameplayTag Tag, FText Name, float Radius) {
    const UWorld *World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client || Id == INDEX_NONE) {
        return;
    }

    FMythicPOIRegistryEntry Entry;
    Entry.Anchor = Anchor;
    Entry.Tag = Tag;
    Entry.Name = Name;
    Entry.Radius = Radius;
    Registry.Add(Id, Entry);

    if (UnlockedPOIs.Contains(Id)) {
        return;
    }
    UnlockedPOIs.Add(Id);

    EnsureReplicator();
    if (Replicator) {
        Replicator->ServerAddPOI(Id, Anchor, Tag, Name);
    }
    UE_LOG(Myth, Log, TEXT("POI: unlocked %d '%s' (world-shared across the party)"), Id, *Name.ToString());
}

bool UMythicPOIDiscoverySubsystem::ResolvePOIAnchor(int32 Id, FVector &OutAnchor) const {
    if (const FMythicPOIRegistryEntry *Entry = Registry.Find(Id)) {
        OutAnchor = Entry->Anchor;
        return true;
    }
    return false;
}

int32 UMythicPOIDiscoverySubsystem::ResolveCurrentPOI(const FVector &Location) const {
    constexpr float DefaultZone = 600.0f;
    int32 Best = INDEX_NONE;
    float BestDistSq = TNumericLimits<float>::Max();
    for (const int32 Id : UnlockedPOIs) {
        const FMythicPOIRegistryEntry *Entry = Registry.Find(Id);
        if (!Entry) {
            continue;
        }
        const float Zone = Entry->Radius > 0.0f ? Entry->Radius : DefaultZone;
        const float DistSq = FVector::DistSquared(Location, Entry->Anchor);
        if (MythicPOIDiscovery::IsWithinDiscoveryRadius(DistSq, Zone * Zone) && DistSq < BestDistSq) {
            BestDistSq = DistSq;
            Best = Id;
        }
    }
    return Best;
}

void UMythicPOIDiscoverySubsystem::ServerFastTravelToPOI(APawn *TravelingPawn, int32 DestPOIId) {
    const UWorld *World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client || !TravelingPawn) {
        return;
    }

    bool bBlocked = false;
    if (const IAbilitySystemInterface *ASI = Cast<IAbilitySystemInterface>(TravelingPawn->GetController())) {
        if (const UAbilitySystemComponent *ASC = ASI->GetAbilitySystemComponent()) {
            bBlocked = ASC->HasMatchingGameplayTag(GAS_STATE_INCOMBAT);
        }
    }

    const int32 SourcePOI = ResolveCurrentPOI(TravelingPawn->GetActorLocation());

    if (!MythicFastTravel::CanFastTravelBetween(UnlockedPOIs, SourcePOI, DestPOIId, bBlocked)) {
        return;
    }

    FVector Anchor = FVector::ZeroVector;
    if (!ResolvePOIAnchor(DestPOIId, Anchor)) {
        return;
    }

    UMythicAbilitySystemComponent *TravelerASC = nullptr;
    if (IAbilitySystemInterface *ASI = Cast<IAbilitySystemInterface>(TravelingPawn->GetController())) {
        TravelerASC = Cast<UMythicAbilitySystemComponent>(ASI->GetAbilitySystemComponent());
    }
    if (TravelerASC) {
        FGameplayCueParameters DepartParams;
        DepartParams.Location = TravelingPawn->GetActorLocation();
        DepartParams.Instigator = TravelingPawn;
        TravelerASC->ExecuteGameplayCueMulticast(TAG_GameplayCue_World_FastTravel_Depart, DepartParams);
    }

    Anchor.Z += 100.0f;
    TravelingPawn->TeleportTo(Anchor, TravelingPawn->GetActorRotation());

    if (TravelerASC) {
        FGameplayCueParameters ArriveParams;
        ArriveParams.Location = Anchor;
        ArriveParams.Instigator = TravelingPawn;
        TravelerASC->ExecuteGameplayCueMulticast(TAG_GameplayCue_World_FastTravel_Arrive, ArriveParams);
    }
}

void UMythicPOIDiscoverySubsystem::RegisterClientReplicator(AMythicPOIReplicator *InReplicator) {
    Replicator = InReplicator;
    OnPOIsChanged.Broadcast();
}

void UMythicPOIDiscoverySubsystem::GetReplicatedPOIs(TArray<FMythicPOIProxyItem> &Out) const {
    Out.Reset();
    if (Replicator) {
        Out = Replicator->GetAllPOIs();
    }
}

void UMythicPOIDiscoverySubsystem::GetUnlockedPOIsForSave(TArray<TPair<int32, FMythicPOIRegistryEntry>> &Out) const {
    Out.Reset();
    Out.Reserve(Registry.Num());
    for (const TPair<int32, FMythicPOIRegistryEntry> &Pair : Registry) {
        Out.Add(Pair);
    }
}
