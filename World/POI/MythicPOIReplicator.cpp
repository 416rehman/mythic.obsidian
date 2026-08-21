
#include "World/POI/MythicPOIReplicator.h"

#include "World/POI/MythicPOIDiscoverySubsystem.h"
#include "Net/UnrealNetwork.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

AMythicPOIReplicator::AMythicPOIReplicator() {
    bReplicates = true;
    bAlwaysRelevant = true;
    PrimaryActorTick.bCanEverTick = false;

    POIProxies.OwnerReplicator = this;
}

void AMythicPOIReplicator::BeginPlay() {
    Super::BeginPlay();

    if (!HasAuthority()) {
        if (const UWorld *World = GetWorld()) {
            if (UGameInstance *GI = World->GetGameInstance()) {
                if (UMythicPOIDiscoverySubsystem *Sub = GI->GetSubsystem<UMythicPOIDiscoverySubsystem>()) {
                    ClientSubsystem = Sub;
                    Sub->RegisterClientReplicator(this);
                }
            }
        }
    }
}

void AMythicPOIReplicator::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (!HasAuthority()) {
        if (UMythicPOIDiscoverySubsystem *Sub = ClientSubsystem.Get()) {
            Sub->RegisterClientReplicator(nullptr);
        }
    }
    ClientSubsystem.Reset();
    Super::EndPlay(EndPlayReason);
}

void AMythicPOIReplicator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AMythicPOIReplicator, POIProxies);
}

void AMythicPOIReplicator::ServerAddPOI(int32 POIId, const FVector &Anchor, const FGameplayTag &POITag, const FText &DisplayName) {
    if (!HasAuthority() || POIId == INDEX_NONE) {
        return;
    }
    for (const FMythicPOIProxyItem &Existing : POIProxies.Items) {
        if (Existing.POIId == POIId) {
            return;
        }
    }
    FMythicPOIProxyItem NewItem;
    NewItem.POIId = POIId;
    NewItem.Anchor = Anchor;
    NewItem.POITag = POITag;
    NewItem.DisplayName = DisplayName;
    const int32 NewIdx = POIProxies.Items.Add(NewItem);
    POIProxies.MarkItemDirty(POIProxies.Items[NewIdx]);
    POIProxies.MarkArrayDirty();
}

void AMythicPOIReplicator::NotifyClientPOIsChanged() {
    if (UMythicPOIDiscoverySubsystem *Sub = ClientSubsystem.Get()) {
        Sub->NotifyPOIsChanged();
    }
}

void FMythicPOIProxyItem::PostReplicatedAdd(const FMythicPOIProxyArray &InArraySerializer) {
    if (InArraySerializer.OwnerReplicator) { InArraySerializer.OwnerReplicator->NotifyClientPOIsChanged(); }
}

void FMythicPOIProxyItem::PostReplicatedChange(const FMythicPOIProxyArray &InArraySerializer) {
    if (InArraySerializer.OwnerReplicator) { InArraySerializer.OwnerReplicator->NotifyClientPOIsChanged(); }
}

void FMythicPOIProxyItem::PreReplicatedRemove(const FMythicPOIProxyArray &InArraySerializer) {
    if (InArraySerializer.OwnerReplicator) { InArraySerializer.OwnerReplicator->NotifyClientPOIsChanged(); }
}
