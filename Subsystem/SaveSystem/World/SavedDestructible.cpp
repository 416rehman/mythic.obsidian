#include "SavedDestructible.h"
#include "Mythic/Resources/MythicResourceManagerComponent.h"
#include "Mythic/Resources/MythicResourceISM.h"

void FSerializedDestructibleHelper::Serialize(UMythicResourceManagerComponent *ResMgr, TArray<FSerializedDestructibleData> &OutData) {
    if (!ResMgr) {
        return;
    }

    OutData.Empty();
    const TArray<FTrackedDestructibleData> &Items = ResMgr->GetDestroyedItems();

    // RespawnTime is an absolute world-time deadline, but world time resets to ~0 on reload. Persist the
    // REMAINING seconds (clock-independent) so respawn timing survives save/reload/world-time reset.
    const UWorld *World = ResMgr->GetWorld();
    const double Now = World ? World->GetTimeSeconds() : 0.0;

    for (const FTrackedDestructibleData &Item : Items) {
        if (!Item.ResourceISM) {
            continue;
        }

        FSerializedDestructibleData Data;
        Data.ComponentPath = FSoftObjectPath(Item.ResourceISM);
        Data.InstanceId = Item.InstanceId;
        Data.OriginalTransform = Item.Transform;
        Data.RespawnTime = FMath::Max(0.0, Item.RespawnTime - Now); // remaining seconds, not an absolute deadline

        OutData.Add(Data);
    }
}

void FSerializedDestructibleHelper::Deserialize(UMythicResourceManagerComponent *ResMgr, const TArray<FSerializedDestructibleData> &InData) {
    if (!ResMgr) {
        return;
    }

    for (const FSerializedDestructibleData &Dat : InData) {
        UMythicResourceISM *ISM = Cast<UMythicResourceISM>(Dat.ComponentPath.TryLoad());
        if (ISM) {
            ResMgr->LoadDestroyedResource(ISM, Dat.InstanceId, Dat.OriginalTransform, Dat.RespawnTime);
        }
    }
}
