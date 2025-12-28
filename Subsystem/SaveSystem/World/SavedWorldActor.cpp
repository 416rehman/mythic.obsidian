#include "SavedWorldActor.h"
#include "MythicSaveableActor.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "EngineUtils.h"
#include "Mythic/Mythic.h"

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
}
