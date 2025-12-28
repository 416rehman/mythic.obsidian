#pragma once

#include "CoreMinimal.h"
#include "SavedDestructible.generated.h"

class UMythicResourceManagerComponent;

USTRUCT(BlueprintType)
struct FSerializedDestructibleData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FSoftObjectPath ComponentPath;

    UPROPERTY(BlueprintReadWrite)
    int32 InstanceId = -1;

    UPROPERTY(BlueprintReadWrite)
    double RespawnTime = 0.0;

    UPROPERTY(BlueprintReadWrite)
    FTransform OriginalTransform;
};

struct FSerializedDestructibleHelper {
    static void Serialize(UMythicResourceManagerComponent *ResMgr, TArray<FSerializedDestructibleData> &OutData);
    static void Deserialize(UMythicResourceManagerComponent *ResMgr, const TArray<FSerializedDestructibleData> &InData);
};
