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

    // Find all actors implementing the saveable interface
    for (TActorIterator<AActor> It(World); It; ++It) {
        AActor *Actor = *It;
        if (!Actor) {
            continue;
        }

        // Check if implements our interface
        IMythicSaveableActor *Saveable = Cast<IMythicSaveableActor>(Actor);
        if (!Saveable) {
            continue;
        }

        FSerializedWorldActorData ActorData;
        ActorData.ActorId = Saveable->GetSaveableActorId();
        ActorData.ActorClass = FSoftClassPath(Actor->GetClass());
        ActorData.Transform = Actor->GetTransform();

        // Determine if runtime spawned: actors placed in editor have RF_WasLoaded flag
        // Runtime-spawned actors don't have this flag
        ActorData.bWasRuntimeSpawned = !Actor->HasAnyFlags(RF_WasLoaded);

        // Serialize actor state (SaveGame properties)
        FMemoryWriter MemWriter(ActorData.ByteData);
        FObjectAndNameAsStringProxyArchive Ar(MemWriter, true);
        Ar.ArIsSaveGame = true;
        Actor->Serialize(Ar);

        // Serialize custom data (nested UObjects, complex types)
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

    // Actors spawned by THIS load (vs found-existing / pre-existing). Exempt from the reconciliation destroy pass
    // below: a cross-session respawn gets a fresh path-name id that won't be in SavedIds, and we must not delete the
    // very actors we just restored.
    TSet<AActor *> SpawnedThisLoad;

    for (const FSerializedWorldActorData &Data : InActors) {
        AActor *TargetActor = nullptr;

        // First, try to find an existing actor with this ID
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

        // If not found and was runtime spawned, spawn a new one
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

        // Restore state if we have a target
        if (TargetActor) {
            // Restore SaveGame properties
            FMemoryReader MemReader(Data.ByteData);
            FObjectAndNameAsStringProxyArchive Ar(MemReader, true);
            Ar.ArIsSaveGame = true;
            TargetActor->Serialize(Ar);

            // Restore custom data (nested UObjects)
            if (IMythicSaveableActor *Saveable = Cast<IMythicSaveableActor>(TargetActor)) {
                Saveable->DeserializeCustomData(Data.CustomData);
                UE_LOG(MythSaveLoad, Log, TEXT("DeserializeAll: Restored %s (CustomData: %d bytes)"),
                       *Data.ActorId, Data.CustomData.Num());
            }
        }
    }

    // Subtractive reconciliation: destroy runtime-spawned saveable actors present in the live world but ABSENT from the
    // save, so the loaded world exactly matches what was serialized. The load is in-place (no map reset), so without
    // this an in-place load over a populated world leaves orphan runtime actors (a non-deterministic superset of the
    // save). Level-placed (RF_WasLoaded) actors are never removed; actors just spawned FROM this save are in SavedIds.
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
    // Level-placed actors are part of the map and are never reconciled away.
    if (!bIsRuntimeSpawned) {
        return false;
    }
    // An actor we just spawned FROM this save is the restored state, not a stale orphan. (Cross-session its fresh
    // path-name id won't match the saved id, so without this exemption the id-set check below would destroy it.)
    if (bSpawnedThisLoad) {
        return false;
    }
    // A pre-existing runtime actor stays iff the save knows about it; otherwise it is a stale orphan to remove.
    return !bPresentInSave;
}
