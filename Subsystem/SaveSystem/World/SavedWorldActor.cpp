#include "SavedWorldActor.h"
#include "MythicSaveableActor.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "EngineUtils.h"
#include "Mythic/Mythic.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

void FSerializedWorldActorHelper::SerializeAll(UWorld *World, TArray<FSerializedWorldActorData> &OutActors) {
    if (!World) {
        return;
    }

    OutActors.Empty();

    for (TActorIterator<AActor> It(World); It; ++It) {
        AActor *Actor = *It;
        if (!Actor) {
            continue;
        }

        IMythicSaveableActor *Saveable = Cast<IMythicSaveableActor>(Actor);
        if (!Saveable) {
            continue;
        }

        FSerializedWorldActorData ActorData;
        ActorData.ActorId = Saveable->GetSaveableActorId();
        ActorData.ActorClass = FSoftClassPath(Actor->GetClass());
        ActorData.Transform = Actor->GetTransform();

        ActorData.bWasRuntimeSpawned = !Actor->HasAnyFlags(RF_WasLoaded);

        FMemoryWriter MemWriter(ActorData.ByteData);
        FObjectAndNameAsStringProxyArchive Ar(MemWriter, true);
        Ar.ArIsSaveGame = true;
        Actor->Serialize(Ar);

        Saveable->SerializeCustomData(ActorData.CustomData);

        UE_LOG(MythSaveLoad, Log, TEXT("SerializeAll: Saved %s (Class: %s, RuntimeSpawned: %s, ByteData: %d, CustomData: %d)"),
               *ActorData.ActorId, *ActorData.ActorClass.ToString(),
               ActorData.bWasRuntimeSpawned ? TEXT("true") : TEXT("false"),
               ActorData.ByteData.Num(), ActorData.CustomData.Num());

        OutActors.Add(ActorData);
    }

    UE_LOG(MythSaveLoad, Log, TEXT("SerializeAll: Total saved actors: %d"), OutActors.Num());
}

void FSerializedWorldActorHelper::DeserializeAll(UWorld *World, const TArray<FSerializedWorldActorData> &InActors) {
    if (!World) {
        return;
    }

    UE_LOG(MythSaveLoad, Log, TEXT("DeserializeAll: Processing %d actors"), InActors.Num());

    TSet<AActor *> SpawnedThisLoad;

    for (const FSerializedWorldActorData &Data : InActors) {
        AActor *TargetActor = nullptr;

        for (TActorIterator<AActor> It(World); It; ++It) {
            AActor *Actor = *It;
            if (!Actor) {
                continue;
            }

            IMythicSaveableActor *Saveable = Cast<IMythicSaveableActor>(Actor);
            if (Saveable && Saveable->GetSaveableActorId() == Data.ActorId) {
                TargetActor = Actor;
                UE_LOG(MythSaveLoad, Log, TEXT("DeserializeAll: Found existing actor for ID %s"), *Data.ActorId);
                break;
            }
        }

        if (!TargetActor && Data.bWasRuntimeSpawned) {
            UClass *ActorClass = Data.ActorClass.TryLoadClass<AActor>();
            if (ActorClass) {
                FActorSpawnParameters Params;
                Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

                TargetActor = World->SpawnActor<AActor>(ActorClass, Data.Transform, Params);
                if (TargetActor) {
                    SpawnedThisLoad.Add(TargetActor);
                }
                UE_LOG(MythSaveLoad, Log, TEXT("DeserializeAll: Spawned runtime actor %s at %s"),
                       *Data.ActorClass.ToString(), *Data.Transform.GetLocation().ToString());
            }
            else {
                UE_LOG(MythSaveLoad, Error, TEXT("DeserializeAll: Failed to load class %s"), *Data.ActorClass.ToString());
            }
        }
        else if (!TargetActor) {
            UE_LOG(MythSaveLoad, Warning, TEXT("DeserializeAll: No target actor for %s (RuntimeSpawned: %s)"),
                   *Data.ActorId, Data.bWasRuntimeSpawned ? TEXT("true") : TEXT("false"));
        }

        if (TargetActor) {
            FMemoryReader MemReader(Data.ByteData);
            FObjectAndNameAsStringProxyArchive Ar(MemReader, true);
            Ar.ArIsSaveGame = true;
            TargetActor->Serialize(Ar);

            if (IMythicSaveableActor *Saveable = Cast<IMythicSaveableActor>(TargetActor)) {
                Saveable->DeserializeCustomData(Data.CustomData);
                UE_LOG(MythSaveLoad, Log, TEXT("DeserializeAll: Restored %s (CustomData: %d bytes)"),
                       *Data.ActorId, Data.CustomData.Num());
            }
        }
    }

    TSet<FString> SavedIds;
    SavedIds.Reserve(InActors.Num());
    for (const FSerializedWorldActorData &Data : InActors) {
        SavedIds.Add(Data.ActorId);
    }

    TArray<AActor *> ToDestroy;
    for (TActorIterator<AActor> It(World); It; ++It) {
        AActor *Actor = *It;
        if (!Actor) {
            continue;
        }
        IMythicSaveableActor *Saveable = Cast<IMythicSaveableActor>(Actor);
        if (!Saveable) {
            continue;
        }
        const bool bIsRuntimeSpawned = !Actor->HasAnyFlags(RF_WasLoaded);
        const bool bSpawnedThisLoad = SpawnedThisLoad.Contains(Actor);
        const bool bPresentInSave = SavedIds.Contains(Saveable->GetSaveableActorId());
        if (ShouldDestroyOnReconcile(bIsRuntimeSpawned, bSpawnedThisLoad, bPresentInSave)) {
            ToDestroy.Add(Actor);
        }
    }
    for (AActor *Actor : ToDestroy) {
        UE_LOG(MythSaveLoad, Log, TEXT("DeserializeAll: Destroying stale runtime actor %s (absent from save)"), *Actor->GetName());
        Actor->Destroy();
    }
}

bool FSerializedWorldActorHelper::ShouldDestroyOnReconcile(const bool bIsRuntimeSpawned, const bool bSpawnedThisLoad,
                                                           const bool bPresentInSave) {
    if (!bIsRuntimeSpawned) {
        return false;
    }
    if (bSpawnedThisLoad) {
        return false;
    }
    return !bPresentInSave;
}
