#include "SavedDestructible.h"
#include "Mythic/Resources/MythicResourceManagerComponent.h"
#include "Mythic/Resources/MythicResourceISM.h"

void FSerializedDestructibleHelper::Serialize(UMythicResourceManagerComponent *ResMgr, TArray<FSerializedDestructibleData> &OutData) {
    if (!ResMgr) {
        return;
    }

    OutData.Empty();
    const TArray<FTrackedDestructibleData> &Items = ResMgr->GetDestroyedItems();

    for (const FTrackedDestructibleData &Item : Items) {
        if (!Item.ResourceISM) {
            continue;
        }

        FSerializedDestructibleData Data;
        Data.ComponentPath = FSoftObjectPath(Item.ResourceISM);
        Data.InstanceId = Item.InstanceId;
        Data.OriginalTransform = Item.Transform;
        Data.RespawnTime = Item.RespawnTime;

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
