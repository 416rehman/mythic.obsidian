#include "SavedWorldActor.h"
#include "MythicSaveableActor.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "EngineUtils.h"
#include "Engine/Level.h"
#include "Mythic/Mythic.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace SavedWorldActorPrivate {
/** FString keys match case-insensitively by default; a saved actor identity is exact. */
struct FSavedActorIdMapKeyFuncs : TDefaultMapKeyFuncs<FString, AActor *, false> {
    static bool Matches(KeyInitType Left, KeyInitType Right) {
        return Left.Equals(Right, ESearchCase::CaseSensitive);
    }

    static uint32 GetKeyHash(KeyInitType Key) {
        return FCrc::StrCrc32<TCHAR>(*Key);
    }
};

struct FSavedActorIdSetKeyFuncs : DefaultKeyFuncs<FString, false> {
    static bool Matches(KeyInitType Left, KeyInitType Right) {
        return Left.Equals(Right, ESearchCase::CaseSensitive);
    }

    static uint32 GetKeyHash(KeyInitType Key) {
        return FCrc::StrCrc32<TCHAR>(*Key);
    }
};

using FSavedActorIdMap = TMap<FString, AActor *, FDefaultSetAllocator, FSavedActorIdMapKeyFuncs>;
using FSavedActorIdSet = TSet<FString, FSavedActorIdSetKeyFuncs>;

void BuildLiveSaveableActorIndex(UWorld *World, FSavedActorIdMap &OutActorsById, FSavedActorIdSet &OutAmbiguousIds) {
    for (TActorIterator<AActor> It(World); It; ++It) {
        AActor *Actor = *It;
        if (!Actor) {
            continue;
        }

        IMythicSaveableActor *Saveable = Cast<IMythicSaveableActor>(Actor);
        if (!Saveable) {
            continue;
        }

        const FString ActorId = Saveable->GetSaveableActorId();
        if (OutActorsById.Contains(ActorId)) {
            OutAmbiguousIds.Add(ActorId);
            continue;
        }
        OutActorsById.Add(ActorId, Actor);
    }
}

void ApplyRecordToActor(AActor &Actor, IMythicSaveableActor &Saveable, const FSerializedWorldActorData &Record) {
    FMemoryReader MemReader(Record.ByteData);
    FObjectAndNameAsStringProxyArchive Ar(MemReader, true);
    Ar.ArIsSaveGame = true;
    Actor.Serialize(Ar);

    Saveable.DeserializeCustomData(Record.CustomData);
}

bool ValidateRecordBounds(const FSerializedWorldActorData &Record, const FString &ClassPath,
                          int64 &InOutTotalPayloadBytes, FName &OutDiagnosticCode) {
    if (Record.ActorId.IsEmpty()
        || Record.ActorId.Len() > FSerializedWorldActorHelper::AbsoluteMaximumStableIdCharacters) {
        OutDiagnosticCode = TEXT("InvalidSavedWorldActorId");
        return false;
    }
    if (ClassPath.IsEmpty()
        || ClassPath.Len() > FSerializedWorldActorHelper::AbsoluteMaximumClassPathCharacters) {
        OutDiagnosticCode = TEXT("InvalidSavedWorldActorClassPath");
        return false;
    }
    if (Record.ByteData.Num() > FSerializedWorldActorHelper::AbsoluteMaximumActorArchiveBytes) {
        OutDiagnosticCode = TEXT("SavedWorldActorArchiveCapacityExceeded");
        return false;
    }
    if (Record.CustomData.Num() > FSerializedWorldActorHelper::AbsoluteMaximumActorCustomDataBytes) {
        OutDiagnosticCode = TEXT("SavedWorldActorCustomDataCapacityExceeded");
        return false;
    }

    const int64 RecordPayloadBytes = static_cast<int64>(Record.ByteData.Num())
        + static_cast<int64>(Record.CustomData.Num());
    if (RecordPayloadBytes
        > FSerializedWorldActorHelper::AbsoluteMaximumTotalPayloadBytes - InOutTotalPayloadBytes) {
        OutDiagnosticCode = TEXT("SavedWorldActorPayloadCapacityExceeded");
        return false;
    }
    InOutTotalPayloadBytes += RecordPayloadBytes;

    OutDiagnosticCode = NAME_None;
    return true;
}

bool ValidateRuntimeSpawnClass(const FSoftClassPath &ActorClassPath, FName &OutDiagnosticCode) {
    UClass *ActorClass = ActorClassPath.TryLoadClass<AActor>();
    if (!ActorClass) {
        OutDiagnosticCode = TEXT("UnresolvedSavedWorldActorClass");
        return false;
    }
    if (ActorClass->HasAnyClassFlags(CLASS_Abstract)) {
        OutDiagnosticCode = TEXT("AbstractSavedWorldActorClass");
        return false;
    }
    if (ActorClass->HasAnyClassFlags(CLASS_Transient)) {
        OutDiagnosticCode = TEXT("TransientSavedWorldActorClass");
        return false;
    }
    // Restore casts the spawned actor to the native interface; a Blueprint-only implementation satisfies
    // ImplementsInterface but yields a null cast, so accept exactly what restore will accept.
    if (!Cast<IMythicSaveableActor>(ActorClass->GetDefaultObject())) {
        OutDiagnosticCode = TEXT("UnsaveableSavedWorldActorClass");
        return false;
    }

    OutDiagnosticCode = NAME_None;
    return true;
}

bool PreflightSavedActorDomain(const TArray<FSerializedWorldActorData> &InActors,
                               const FSavedActorIdMap &LiveActorsById, const FSavedActorIdSet &AmbiguousLiveIds,
                               FName &OutDiagnosticCode) {
    if (InActors.Num() > FSerializedWorldActorHelper::AbsoluteMaximumActorRecords) {
        OutDiagnosticCode = TEXT("SavedWorldActorCapacityExceeded");
        return false;
    }

    FSavedActorIdSet RecordIds;
    RecordIds.Reserve(InActors.Num());
    FSavedActorIdSet ValidatedRuntimeClassPaths;
    int64 TotalPayloadBytes = 0;

    for (const FSerializedWorldActorData &Record : InActors) {
        const FString ClassPath = Record.ActorClass.ToString();
        if (!ValidateRecordBounds(Record, ClassPath, TotalPayloadBytes, OutDiagnosticCode)) {
            return false;
        }
        if (RecordIds.Contains(Record.ActorId)) {
            OutDiagnosticCode = TEXT("DuplicateSavedWorldActorId");
            return false;
        }
        RecordIds.Add(Record.ActorId);
        if (AmbiguousLiveIds.Contains(Record.ActorId)) {
            OutDiagnosticCode = TEXT("AmbiguousLiveSavedWorldActorId");
            return false;
        }

        if (!Record.bWasRuntimeSpawned) {
            continue;
        }

        if (Record.Transform.ContainsNaN()) {
            OutDiagnosticCode = TEXT("InvalidSavedWorldActorTransform");
            return false;
        }
        if (LiveActorsById.Contains(Record.ActorId)) {
            continue;
        }
        if (ValidatedRuntimeClassPaths.Contains(ClassPath)) {
            continue;
        }
        if (!ValidateRuntimeSpawnClass(Record.ActorClass, OutDiagnosticCode)) {
            return false;
        }
        ValidatedRuntimeClassPaths.Add(ClassPath);
    }

    OutDiagnosticCode = NAME_None;
    return true;
}
} // namespace SavedWorldActorPrivate

bool FSerializedWorldActorHelper::SerializeAll(UWorld *World,
                                               const TArray<FSerializedWorldActorData> &CarryForwardPlaced,
                                               TArray<FSerializedWorldActorData> &OutActors) {
    OutActors.Empty();

    if (!World) {
        UE_LOG(MythSaveLoad, Error, TEXT("SerializeAll: rejected because the world is unavailable"));
        return false;
    }

    TArray<FSerializedWorldActorData> CapturedActors;
    SavedWorldActorPrivate::FSavedActorIdSet CapturedIds;
    int64 TotalPayloadBytes = 0;

    for (TActorIterator<AActor> It(World); It; ++It) {
        AActor *Actor = *It;
        if (!Actor) {
            continue;
        }

        IMythicSaveableActor *Saveable = Cast<IMythicSaveableActor>(Actor);
        if (!Saveable) {
            continue;
        }

        if (CapturedActors.Num() >= AbsoluteMaximumActorRecords) {
            UE_LOG(MythSaveLoad, Error, TEXT("SerializeAll: rejected at %s because the world exceeds the %d saveable actor bound"),
                   *Actor->GetName(), AbsoluteMaximumActorRecords);
            return false;
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

        const FString ClassPath = ActorData.ActorClass.ToString();
        FName DiagnosticCode = NAME_None;
        if (!SavedWorldActorPrivate::ValidateRecordBounds(ActorData, ClassPath, TotalPayloadBytes, DiagnosticCode)) {
            UE_LOG(MythSaveLoad, Error, TEXT("SerializeAll: rejected %s [%s] (%s)"),
                   *Actor->GetName(), *ClassPath, *DiagnosticCode.ToString());
            return false;
        }
        if (CapturedIds.Contains(ActorData.ActorId)) {
            UE_LOG(MythSaveLoad, Error, TEXT("SerializeAll: rejected %s because another live saveable actor already claims id %s"),
                   *Actor->GetName(), *ActorData.ActorId);
            return false;
        }
        CapturedIds.Add(ActorData.ActorId);

        UE_LOG(MythSaveLoad, Verbose, TEXT("SerializeAll: captured %s (class %s, runtime %s, archive %d, custom %d)"),
               *ActorData.ActorId, *ClassPath,
               ActorData.bWasRuntimeSpawned ? TEXT("true") : TEXT("false"),
               ActorData.ByteData.Num(), ActorData.CustomData.Num());

        CapturedActors.Add(MoveTemp(ActorData));
    }

    int32 CarriedForward = 0;
    for (const FSerializedWorldActorData &Deferred : CarryForwardPlaced) {
        if (Deferred.bWasRuntimeSpawned || CapturedIds.Contains(Deferred.ActorId)) {
            continue;
        }
        if (CapturedActors.Num() >= AbsoluteMaximumActorRecords) {
            UE_LOG(MythSaveLoad, Error, TEXT("SerializeAll: rejected because carrying %s forward exceeds the %d saveable actor bound"),
                   *Deferred.ActorId, AbsoluteMaximumActorRecords);
            return false;
        }

        const FString ClassPath = Deferred.ActorClass.ToString();
        FName DiagnosticCode = NAME_None;
        if (!SavedWorldActorPrivate::ValidateRecordBounds(Deferred, ClassPath, TotalPayloadBytes, DiagnosticCode)) {
            UE_LOG(MythSaveLoad, Error, TEXT("SerializeAll: rejected carried-forward %s [%s] (%s)"),
                   *Deferred.ActorId, *ClassPath, *DiagnosticCode.ToString());
            return false;
        }
        CapturedIds.Add(Deferred.ActorId);
        CapturedActors.Add(Deferred);
        ++CarriedForward;
    }

    OutActors = MoveTemp(CapturedActors);

    UE_LOG(MythSaveLoad, Log, TEXT("SerializeAll: captured %d saveable actors (%d carried forward, %lld payload bytes)"),
           OutActors.Num(), CarriedForward, TotalPayloadBytes);
    return true;
}

bool FSerializedWorldActorHelper::PreflightDeserialize(UWorld *World, const TArray<FSerializedWorldActorData> &InActors,
                                                       FName &OutDiagnosticCode) {
    if (!World || World->bIsTearingDown) {
        OutDiagnosticCode = TEXT("SavedWorldActorWorldUnavailable");
        return false;
    }

    SavedWorldActorPrivate::FSavedActorIdMap LiveActorsById;
    SavedWorldActorPrivate::FSavedActorIdSet AmbiguousLiveIds;
    SavedWorldActorPrivate::BuildLiveSaveableActorIndex(World, LiveActorsById, AmbiguousLiveIds);

    return SavedWorldActorPrivate::PreflightSavedActorDomain(InActors, LiveActorsById, AmbiguousLiveIds, OutDiagnosticCode);
}

bool FSerializedWorldActorHelper::DeserializeAll(UWorld *World, const TArray<FSerializedWorldActorData> &InActors,
                                                 TArray<FSerializedWorldActorData> &OutDeferredPlaced) {
    OutDeferredPlaced.Reset();

    if (!World || World->bIsTearingDown) {
        UE_LOG(MythSaveLoad, Error, TEXT("DeserializeAll: rejected because the world is unavailable"));
        return false;
    }

    SavedWorldActorPrivate::FSavedActorIdMap LiveActorsById;
    SavedWorldActorPrivate::FSavedActorIdSet AmbiguousLiveIds;
    SavedWorldActorPrivate::BuildLiveSaveableActorIndex(World, LiveActorsById, AmbiguousLiveIds);

    FName DiagnosticCode = NAME_None;
    if (!SavedWorldActorPrivate::PreflightSavedActorDomain(InActors, LiveActorsById, AmbiguousLiveIds, DiagnosticCode)) {
        UE_LOG(MythSaveLoad, Error, TEXT("DeserializeAll: rejected %d saved actor records before any mutation (%s)"),
               InActors.Num(), *DiagnosticCode.ToString());
        return false;
    }

    TSet<AActor *> SpawnedThisLoad;
    SavedWorldActorPrivate::FSavedActorIdSet SavedIds;
    SavedIds.Reserve(InActors.Num());

    for (const FSerializedWorldActorData &Data : InActors) {
        SavedIds.Add(Data.ActorId);

        AActor *TargetActor = LiveActorsById.FindRef(Data.ActorId);
        if (!TargetActor && !Data.bWasRuntimeSpawned) {
            OutDeferredPlaced.Add(Data);
            UE_LOG(MythSaveLoad, Verbose, TEXT("DeserializeAll: deferred placed actor %s until its cell streams in"),
                   *Data.ActorId);
            continue;
        }

        if (!TargetActor) {
            UClass *ActorClass = Data.ActorClass.TryLoadClass<AActor>();
            if (!ActorClass) {
                UE_LOG(MythSaveLoad, Fatal, TEXT("DeserializeAll: preflighted class %s for %s stopped resolving after restore began"),
                       *Data.ActorClass.ToString(), *Data.ActorId);
                return false;
            }

            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

            TargetActor = World->SpawnActor<AActor>(ActorClass, Data.Transform, Params);
            if (!TargetActor) {
                UE_LOG(MythSaveLoad, Fatal, TEXT("DeserializeAll: preflighted runtime actor %s [%s] failed to spawn"),
                       *Data.ActorId, *Data.ActorClass.ToString());
                return false;
            }
            SpawnedThisLoad.Add(TargetActor);

            UE_LOG(MythSaveLoad, Verbose, TEXT("DeserializeAll: spawned runtime actor %s at %s"),
                   *Data.ActorClass.ToString(), *Data.Transform.GetLocation().ToString());
        }

        IMythicSaveableActor *Saveable = Cast<IMythicSaveableActor>(TargetActor);
        if (!Saveable) {
            UE_LOG(MythSaveLoad, Fatal, TEXT("DeserializeAll: restore target for %s does not implement the saveable interface"),
                   *Data.ActorId);
            return false;
        }

        SavedWorldActorPrivate::ApplyRecordToActor(*TargetActor, *Saveable, Data);

        UE_LOG(MythSaveLoad, Verbose, TEXT("DeserializeAll: restored %s (custom %d bytes)"),
               *Data.ActorId, Data.CustomData.Num());
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
        UE_LOG(MythSaveLoad, Verbose, TEXT("DeserializeAll: destroying stale runtime actor %s (absent from save)"), *Actor->GetName());
        Actor->Destroy();
    }

    UE_LOG(MythSaveLoad, Log, TEXT("DeserializeAll: restored %d saved actors, spawned %d, deferred %d placed, destroyed %d stale runtime actors"),
           InActors.Num() - OutDeferredPlaced.Num(), SpawnedThisLoad.Num(), OutDeferredPlaced.Num(), ToDestroy.Num());
    return true;
}

bool FSerializedWorldActorHelper::RestoreDeferredPlacedActors(ULevel *Level,
                                                              TArray<FSerializedWorldActorData> &InOutDeferred) {
    if (!Level || InOutDeferred.IsEmpty()) {
        return false;
    }

    int32 Applied = 0;
    for (AActor *Actor : Level->Actors) {
        if (!Actor) {
            continue;
        }
        IMythicSaveableActor *Saveable = Cast<IMythicSaveableActor>(Actor);
        if (!Saveable) {
            continue;
        }

        const FString ActorId = Saveable->GetSaveableActorId();
        const int32 Index = InOutDeferred.IndexOfByPredicate(
            [&ActorId](const FSerializedWorldActorData &Record) {
                return Record.ActorId.Equals(ActorId, ESearchCase::CaseSensitive);
            });
        if (Index == INDEX_NONE) {
            continue;
        }

        SavedWorldActorPrivate::ApplyRecordToActor(*Actor, *Saveable, InOutDeferred[Index]);
        InOutDeferred.RemoveAtSwap(Index, EAllowShrinking::No);
        ++Applied;

        UE_LOG(MythSaveLoad, Verbose, TEXT("RestoreDeferredPlacedActors: applied %s from streamed level %s"),
               *ActorId, *Level->GetOutermost()->GetName());
    }

    if (Applied > 0) {
        UE_LOG(MythSaveLoad, Log, TEXT("RestoreDeferredPlacedActors: applied %d deferred placed actors, %d still pending"),
               Applied, InOutDeferred.Num());
    }
    return Applied > 0;
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
