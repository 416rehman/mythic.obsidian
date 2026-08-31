#include "MythicSaveGameSubsystem.h"
#include "Mythic/Subsystem/SaveSystem/MythicSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"
#include "HAL/FileManager.h"
#include "Misc/SecureHash.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

#include "Mythic/Mythic.h"
#include "Mythic/Player/MythicPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "Mythic/Itemization/Inventory/MythicInventoryComponent.h"
#include "Mythic/Player/Proficiency/ProficiencyComponent.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/POI/MythicPOIDiscoverySubsystem.h"
#include "World/Digging/MythicDiggingSubsystem.h"
#include "World/Harvesting/MythicHarvestRewardOutboxSubsystem.h"
#include "World/Harvesting/MythicHarvestReceiptLedgerComponent.h"
#include "World/Harvesting/MythicHarvestRewardEscrowComponent.h"
#include "World/Harvesting/MythicHarvestSettings.h"
#include "World/Harvesting/MythicHarvestWorldSubsystem.h"
#include "World/LivingWorld/MythicWorldStateSubsystem.h"
#include "AI/Party/PartySubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Serialization/MemoryReader.h"

FString UMythicSaveGameSubsystem::SanitizeSlotName(const FString &Input) {
    FString Safe = Input;

    Safe.ReplaceInline(TEXT(".."), TEXT(""));
    Safe.ReplaceInline(TEXT("/"), TEXT("_"));
    Safe.ReplaceInline(TEXT("\\"), TEXT("_"));
    Safe.ReplaceInline(TEXT(":"), TEXT("_"));
    Safe.ReplaceInline(TEXT("*"), TEXT("_"));
    Safe.ReplaceInline(TEXT("?"), TEXT("_"));
    Safe.ReplaceInline(TEXT("\""), TEXT("_"));
    Safe.ReplaceInline(TEXT("<"), TEXT("_"));
    Safe.ReplaceInline(TEXT(">"), TEXT("_"));
    Safe.ReplaceInline(TEXT("|"), TEXT("_"));

    if (Safe.Len() > 64) {
        Safe = Safe.Left(64);
    }

    if (Safe.IsEmpty()) {
        Safe = TEXT("InvalidSlot");
    }

    return Safe;
}

void UMythicSaveGameSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);

    LevelAddedToWorldHandle = FWorldDelegates::LevelAddedToWorld.AddUObject(
        this, &UMythicSaveGameSubsystem::HandleLevelAddedToWorld);
}

void UMythicSaveGameSubsystem::Deinitialize() {
    FWorldDelegates::LevelAddedToWorld.Remove(LevelAddedToWorldHandle);
    LevelAddedToWorldHandle.Reset();
    DeferredPlacedActorRestores.Empty();

    Super::Deinitialize();
}

void UMythicSaveGameSubsystem::HandleLevelAddedToWorld(ULevel *Level, UWorld *World) {
    if (DeferredPlacedActorRestores.IsEmpty() || !Level || World != GetWorld()) {
        return;
    }

    FSerializedWorldActorHelper::RestoreDeferredPlacedActors(Level, DeferredPlacedActorRestores);
}

FString UMythicSaveGameSubsystem::ComputeChecksum(const TArray<uint8> &Data) {
    FSHAHash Hash;
    FSHA1::HashBuffer(Data.GetData(), Data.Num(), Hash.Hash);
    return Hash.ToString();
}

bool UMythicSaveGameSubsystem::ValidateChecksum(const TArray<uint8> &Data, const FString &ExpectedChecksum) {
    const FString ComputedChecksum = ComputeChecksum(Data);
    return ComputedChecksum.Equals(ExpectedChecksum, ESearchCase::IgnoreCase);
}


void UMythicSaveGameSubsystem::SaveCharacter(AActor *SourceActor, const FString &CharacterID) {
    QueueCharacterSave(SourceActor, CharacterID,
                       FMythicDurableCharacterSaveComplete(), true);
}

bool UMythicSaveGameSubsystem::RequestDurableCharacterSave(
    AActor *SourceActor, const FString &CharacterID,
    FMythicDurableCharacterSaveComplete Completion,
    FGuid &OutOperationId) {
    return QueueCharacterSave(SourceActor, CharacterID, MoveTemp(Completion),
                              false, &OutOperationId);
}

bool UMythicSaveGameSubsystem::QueueCharacterSave(
    AActor *SourceActor, const FString &CharacterID,
    FMythicDurableCharacterSaveComplete Completion,
    const bool bBroadcastPresentationEvents,
    FGuid *OutOperationId) {
    if (OutOperationId) OutOperationId->Invalidate();
    if (!SourceActor || CharacterID.IsEmpty()) {
        UE_LOG(MythSaveLoad, Error,
               TEXT("SaveCharacter: Invalid SourceActor or CharacterID"));
        FMythicDurableCharacterSaveResult Result;
        Result.RoutingCharacterId = CharacterID;
        Completion.ExecuteIfBound(Result);
        if (bBroadcastPresentationEvents) {
            OnSaveGameActionFinished.Broadcast(CharacterID, false);
        }
        return false;
    }

    const FString SafeSlotName = SanitizeSlotName(CharacterID);
    FQueuedCharacterSave &Queued =
        QueuedCharacterSaves.FindOrAdd(SafeSlotName);
    if (!Queued.OperationId.IsValid()) {
        do {
            Queued.OperationId = FGuid::NewGuid();
        } while (!Queued.OperationId.IsValid());
    }
    if (OutOperationId) *OutOperationId = Queued.OperationId;
    Queued.SourceActor = SourceActor;
    if (Completion.IsBound()) {
        Queued.Completions.Add(MoveTemp(Completion));
    }
    Queued.bBroadcastPresentationEvents |= bBroadcastPresentationEvents;
    if (InFlightSaveSlots.Contains(SafeSlotName)) {
        return true;
    }
    StartNextQueuedCharacterSave(SafeSlotName);
    return InFlightSaveSlots.Contains(SafeSlotName);
}

void UMythicSaveGameSubsystem::StartNextQueuedCharacterSave(
    const FString &SafeSlotName) {
    FQueuedCharacterSave Request;
    if (!QueuedCharacterSaves.RemoveAndCopyValue(SafeSlotName, Request)) {
        return;
    }
    StartCharacterSave(SafeSlotName, MoveTemp(Request));
}

bool UMythicSaveGameSubsystem::StartCharacterSave(
    const FString &SafeSlotName, FQueuedCharacterSave &&Request) {
    const FGuid OperationId = Request.OperationId;
    if (!OperationId.IsValid()) {
        return false;
    }
    auto FailRequest = [this, &SafeSlotName, OperationId](
                           FQueuedCharacterSave &FailedRequest) {
        FMythicDurableCharacterSaveResult Result;
        Result.OperationId = OperationId;
        Result.RoutingCharacterId = SafeSlotName;
        for (FMythicDurableCharacterSaveComplete &Completion :
             FailedRequest.Completions) {
            Completion.ExecuteIfBound(Result);
        }
        if (FailedRequest.bBroadcastPresentationEvents) {
            OnSaveGameActionFinished.Broadcast(SafeSlotName, false);
        }
    };

    AActor *SourceActor = Request.SourceActor.Get();
    if (!SourceActor) {
        FailRequest(Request);
        return false;
    }
    if (Request.bBroadcastPresentationEvents) {
        OnSaveGameActionStarted.Broadcast(SafeSlotName);
    }

    FSerializedCharacterData CharacterData;
    if (!FSerializedCharacterData::Serialize(SourceActor, CharacterData)) {
        UE_LOG(MythSaveLoad, Error,
               TEXT("SaveCharacter: Failed to serialize character data"));
        FailRequest(Request);
        return false;
    }
    CharacterData.CharacterID = SafeSlotName;

    UMythicSaveGame *SaveObj = Cast<UMythicSaveGame>(
        UGameplayStatics::CreateSaveGameObject(
            UMythicSaveGame::StaticClass()));
    if (!SaveObj) {
        FailRequest(Request);
        return false;
    }
    SaveObj->CharacterData = CharacterData;
    SaveObj->SaveSlotName = SafeSlotName;
    SaveObj->CreationTime = FDateTime::Now();

    TArray<uint8> TempBuffer;
    FMemoryWriter MemWriter(TempBuffer);
    FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
    SaveObj->Serialize(Ar);
    SaveObj->DataChecksum = ComputeChecksum(TempBuffer);

    FInFlightCharacterSave InFlight;
    InFlight.OperationId = OperationId;
    InFlight.CapturedHarvestReceipts =
        CharacterData.HarvestReceiptLedger;
    InFlight.CapturedHarvestItemEscrow =
        CharacterData.HarvestItemEscrow;
    InFlight.Completions = MoveTemp(Request.Completions);
    InFlight.bBroadcastPresentationEvents =
        Request.bBroadcastPresentationEvents;
    InFlightCharacterSaves.Add(SafeSlotName, MoveTemp(InFlight));

    FAsyncSaveGameToSlotDelegate SavedDelegate;
    SavedDelegate.BindUObject(
        this, &UMythicSaveGameSubsystem::HandleAsyncSaveFinished);
    InFlightSaveSlots.Add(SafeSlotName);
    UGameplayStatics::AsyncSaveGameToSlot(
        SaveObj, SafeSlotName, 0, SavedDelegate);
    return true;
}

void UMythicSaveGameSubsystem::LoadCharacter(AActor *TargetActor, const FString &CharacterID) {
    if (!TargetActor || CharacterID.IsEmpty()) {
        UE_LOG(MythSaveLoad, Error, TEXT("LoadCharacter: Invalid TargetActor or CharacterID"));
        OnSaveGameActionFinished.Broadcast(CharacterID, false);
        return;
    }

    const FString SafeSlotName = SanitizeSlotName(CharacterID);

    OnSaveGameActionStarted.Broadcast(SafeSlotName);

    if (!UGameplayStatics::DoesSaveGameExist(SafeSlotName, 0)) {
        UE_LOG(MythSaveLoad, Log, TEXT("LoadCharacter: Save slot %s does not exist (first run for this character)"), *SafeSlotName);
        OnSaveGameActionFinished.Broadcast(SafeSlotName, false);
        return;
    }

    PendingLoadTargets.Add(SafeSlotName, TargetActor);

    FAsyncLoadGameFromSlotDelegate LoadedDelegate;
    LoadedDelegate.BindUObject(this, &UMythicSaveGameSubsystem::HandleAsyncLoadFinished);

    UGameplayStatics::AsyncLoadGameFromSlot(SafeSlotName, 0, LoadedDelegate);
}

void UMythicSaveGameSubsystem::HandleAsyncSaveFinished(const FString &SlotName, const int32 UserIndex, bool bSuccess) {
    InFlightSaveSlots.Remove(SlotName);
    UE_LOG(MythSaveLoad, Log, TEXT("Async Save Finished for %s: %s"), *SlotName, bSuccess ? TEXT("Success") : TEXT("Failed"));
    FInFlightCharacterSave CharacterSave;
    if (InFlightCharacterSaves.RemoveAndCopyValue(SlotName,
                                                   CharacterSave)) {
        FMythicDurableCharacterSaveResult Result;
        Result.OperationId = CharacterSave.OperationId;
        Result.RoutingCharacterId = SlotName;
        Result.bSuccess = bSuccess;
        if (bSuccess) {
            Result.CapturedHarvestReceipts =
                CharacterSave.CapturedHarvestReceipts;
            Result.CapturedHarvestItemEscrow =
                CharacterSave.CapturedHarvestItemEscrow;
        }
        for (FMythicDurableCharacterSaveComplete &Completion :
             CharacterSave.Completions) {
            Completion.ExecuteIfBound(Result);
        }
        if (CharacterSave.bBroadcastPresentationEvents) {
            OnSaveGameActionFinished.Broadcast(SlotName, bSuccess);
        }
        StartNextQueuedCharacterSave(SlotName);
        return;
    }
    FMythicHarvestRewardOutboxSaveV1 DurableWorldOutbox;
    if (InFlightWorldRewardOutboxSnapshots.RemoveAndCopyValue(
            SlotName, DurableWorldOutbox) && bSuccess) {
        UWorld *World = GetWorld();
        UMythicHarvestRewardOutboxSubsystem *Outbox = World
            ? World->GetSubsystem<
                UMythicHarvestRewardOutboxSubsystem>() : nullptr;
        FName Diagnostic;
        if (!Outbox
            || !Outbox->MarkWorldSnapshotDurable(
                DurableWorldOutbox, Diagnostic)) {
            UE_LOG(MythSaveLoad, Error,
                   TEXT("Durable world harvest-outbox boundary rejected for %s: %s"),
                   *SlotName, *Diagnostic.ToString());
        }
    }
    OnSaveGameActionFinished.Broadcast(SlotName, bSuccess);
    StartNextQueuedCharacterSave(SlotName);
}

void UMythicSaveGameSubsystem::HandleAsyncLoadFinished(const FString &SlotName, const int32 UserIndex, USaveGame *LoadedSaveGame) {
    TWeakObjectPtr<AActor> *TargetPtr = PendingLoadTargets.Find(SlotName);
    AActor *TargetActor = TargetPtr ? TargetPtr->Get() : nullptr;
    PendingLoadTargets.Remove(SlotName);

    if (!TargetActor) {
        UE_LOG(MythSaveLoad, Warning, TEXT("AsyncLoadFinished: Target Actor for %s is no longer valid or was cancelled"), *SlotName);
        OnSaveGameActionFinished.Broadcast(SlotName, false);
        return;
    }

    UMythicSaveGame *SaveObj = Cast<UMythicSaveGame>(LoadedSaveGame);
    if (!SaveObj) {
        UE_LOG(MythSaveLoad, Error, TEXT("AsyncLoadFinished: Failed to cast loaded object"));
        OnSaveGameActionFinished.Broadcast(SlotName, false);
        return;
    }

    if (SaveObj->DataChecksum.IsEmpty()) {
        UE_LOG(MythSaveLoad, Error, TEXT("AsyncLoadFinished: No checksum"));
        OnSaveGameActionFinished.Broadcast(SlotName, false);
        return;
    }

    TArray<uint8> TempBuffer;
    FString StoredChecksum = SaveObj->DataChecksum;
    SaveObj->DataChecksum = TEXT("");

    FMemoryWriter MemWriter(TempBuffer);
    FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
    SaveObj->Serialize(Ar);

    if (!ValidateChecksum(TempBuffer, StoredChecksum)) {
        UE_LOG(MythSaveLoad, Error, TEXT("AsyncLoadFinished: Checksum failed for %s"), *SlotName);
        OnSaveGameActionFinished.Broadcast(SlotName, false);
        return;
    }

    SaveObj->DataChecksum = StoredChecksum;
    SaveObj->FixupData();

    FString ValidationError;
    if (!ValidateCharacterData(SaveObj->CharacterData, ValidationError)) {
        UE_LOG(MythSaveLoad, Error, TEXT("AsyncLoadFinished: Invalid character data for %s: %s"), *SlotName, *ValidationError);
        OnSaveGameActionFinished.Broadcast(SlotName, false);
        return;
    }

    UWorld *ActiveWorld = TargetActor->GetWorld();
    UMythicHarvestRewardOutboxSubsystem *ActiveHarvestOutbox =
        ActiveWorld
        ? ActiveWorld->GetSubsystem<
            UMythicHarvestRewardOutboxSubsystem>() : nullptr;
    FName CharacterFenceDiagnostic;
    if (ActiveHarvestOutbox
        && !ActiveHarvestOutbox->ValidateCharacterReceiptSnapshot(
            SlotName, SaveObj->CharacterData.HarvestReceiptLedger,
            SaveObj->CharacterData.HarvestItemEscrow,
            CharacterFenceDiagnostic)) {
        UE_LOG(MythSaveLoad, Error,
               TEXT("AsyncLoadFinished: character/world harvest fence rejected %s (%s)"),
               *SlotName, *CharacterFenceDiagnostic.ToString());
        OnSaveGameActionFinished.Broadcast(SlotName, false);
        return;
    }

    if (!FSerializedCharacterData::Deserialize(TargetActor, SaveObj->CharacterData)) {
        UE_LOG(MythSaveLoad, Error, TEXT("AsyncLoadFinished: Failed to deserialize"));
        OnSaveGameActionFinished.Broadcast(SlotName, false);
        return;
    }

    AMythicPlayerState *MythPS = nullptr;
    if (const APawn *Pawn = Cast<APawn>(TargetActor)) {
        MythPS = Pawn->GetPlayerState<AMythicPlayerState>();
    }
    else if (const APlayerController *PC = Cast<APlayerController>(TargetActor)) {
        MythPS = PC->GetPlayerState<AMythicPlayerState>();
    }
    else {
        MythPS = Cast<AMythicPlayerState>(TargetActor);
    }
    if (MythPS) {
        MythPS->SetPersistentCharacterId(SlotName);
    }
    if (ActiveHarvestOutbox
        && !ActiveHarvestOutbox->ObserveDurableCharacterReceiptSnapshot(
            SlotName, SaveObj->CharacterData.HarvestReceiptLedger,
            SaveObj->CharacterData.HarvestItemEscrow,
            CharacterFenceDiagnostic)) {
        UE_LOG(MythSaveLoad, Fatal,
               TEXT("AsyncLoadFinished: a preflighted character harvest fence failed installation for %s (%s)"),
               *SlotName, *CharacterFenceDiagnostic.ToString());
    }

    UE_LOG(MythSaveLoad, Log, TEXT("AsyncLoadFinished: Successfully loaded %s"), *SlotName);
    OnSaveGameActionFinished.Broadcast(SlotName, true);
}


TArray<FString> UMythicSaveGameSubsystem::GetLocalSaveFiles() const {
    TArray<FString> Result;
    FindSaveGames(Result);
    return Result;
}

FString UMythicSaveGameSubsystem::ResolvePerPlayerCharacterSlot(const FString &StablePlayerId) {
    return StablePlayerId.IsEmpty() ? FString(DebugCharacterSlot) : FString::Printf(TEXT("Character_%s"), *StablePlayerId);
}

bool UMythicSaveGameSubsystem::TryResolveAuthorityWorldSlot(
    FString &OutSlotName) {
    OutSlotName.Reset();
    FString ExplicitSlot;
    if (FParse::Value(FCommandLine::Get(), TEXT("MythicWorldSlot="),
                      ExplicitSlot)) {
        ExplicitSlot.TrimStartAndEndInline();
        if (ExplicitSlot.IsEmpty()) {
            return false;
        }
        OutSlotName = SanitizeSlotName(ExplicitSlot);
#if UE_BUILD_SHIPPING
        if (OutSlotName.Equals(DebugWorldSlot,
                               ESearchCase::IgnoreCase)) {
            OutSlotName.Reset();
            return false;
        }
#endif
        return true;
    }

#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("MythicUseDebugWorld"))) {
        OutSlotName = DebugWorldSlot;
        return true;
    }
#endif
    return false;
}


bool UMythicSaveGameSubsystem::SerializeCharacterToStruct(AActor *SourceActor, FSerializedCharacterData &OutData) {
    return FSerializedCharacterData::Serialize(SourceActor, OutData);
}

bool UMythicSaveGameSubsystem::DeserializeCharacterFromStruct(AActor *TargetActor, const FSerializedCharacterData &InData) {
    return FSerializedCharacterData::Deserialize(TargetActor, InData);
}

bool UMythicSaveGameSubsystem::ValidateCharacterData(const FSerializedCharacterData &InData, FString &OutError) {
    if (InData.DataVersion != static_cast<int32>(CurrentCharacterSaveVersion)) {
        OutError = FString::Printf(TEXT("Data version %d does not match current unreleased schema %d"),
                                   InData.DataVersion, static_cast<int32>(CurrentCharacterSaveVersion));
        return false;
    }
    if (!InData.PlayerEntityId.IsValid()
        || InData.PlayerEntityId.GetDomain()
               != EMythicEntityDomain::PlayerCharacter) {
        OutError = TEXT("Missing or invalid canonical PlayerCharacter identity");
        return false;
    }

    FName ReceiptDiagnostic;
    const UMythicHarvestSettings *HarvestSettings =
        GetDefault<UMythicHarvestSettings>();
    const int32 MaximumReceiptRows = HarvestSettings
        ? HarvestSettings->RewardReceiptMaximumRows
        : FMythicHarvestReceiptLedgerSaveV1::AbsoluteMaximumRows;
    if (!FMythicHarvestReceiptLedgerSaveV1::Validate(
            InData.HarvestReceiptLedger, ReceiptDiagnostic,
            MaximumReceiptRows)) {
        OutError = FString::Printf(
            TEXT("Invalid harvest receipt ledger: %s"),
            *ReceiptDiagnostic.ToString());
        return false;
    }

    FName EscrowDiagnostic;
    const int32 MaximumEscrowRows = HarvestSettings
        ? HarvestSettings->RewardItemEscrowMaximumRows
        : FMythicHarvestItemEscrowSaveV1::AbsoluteMaximumRows;
    if (!FMythicHarvestItemEscrowSaveV1::Validate(
            InData.HarvestItemEscrow, EscrowDiagnostic,
            MaximumEscrowRows)
        || !FMythicHarvestItemEscrowSaveV1::ValidateReceiptBinding(
            InData.HarvestItemEscrow, InData.HarvestReceiptLedger,
            EscrowDiagnostic)) {
        OutError = FString::Printf(
            TEXT("Invalid harvest item escrow: %s"),
            *EscrowDiagnostic.ToString());
        return false;
    }

    if (InData.CharacterName.Len() > 64) {
        OutError = TEXT("Character name too long (max 64 chars)");
        return false;
    }

    int32 TotalSlots = 0;
    for (const FSerializedInventoryData &InvData : InData.Inventories) {
        TotalSlots += InvData.Slots.Num();
    }
    if (TotalSlots > 1000) {
        OutError = TEXT("Too many total inventory slots (max 1000)");
        return false;
    }

    return true;
}


void UMythicSaveGameSubsystem::SaveWorld(const FString &SlotName) {
    if (SlotName.IsEmpty()) {
        UE_LOG(MythSaveLoad, Error, TEXT("SaveWorld: SlotName is empty"));
        OnSaveGameActionFinished.Broadcast(SlotName, false);
        return;
    }

    const FString SafeSlotName = SanitizeSlotName(SlotName);

    if (InFlightSaveSlots.Contains(SafeSlotName)) {
        UE_LOG(MythSaveLoad, Warning, TEXT("SaveWorld: a save to slot '%s' is already in flight; skipping concurrent write."), *SafeSlotName);
        OnSaveGameActionFinished.Broadcast(SafeSlotName, false);
        return;
    }

    OnSaveGameActionStarted.Broadcast(SafeSlotName);

    UWorld *World = GetWorld();
    if (!World) {
        OnSaveGameActionFinished.Broadcast(SafeSlotName, false);
        return;
    }

    FSerializedWorldData WorldData;

    UMythicHarvestWorldSubsystem *HarvestWorld =
        World->GetSubsystem<UMythicHarvestWorldSubsystem>();
    UMythicHarvestRewardOutboxSubsystem *HarvestOutbox =
        World->GetSubsystem<UMythicHarvestRewardOutboxSubsystem>();
    FName HarvestDiagnostic;
    if (!HarvestWorld || !HarvestOutbox
        || !HarvestWorld->BeginSaveCapture(HarvestDiagnostic)) {
        UE_LOG(MythSaveLoad, Error,
               TEXT("SaveWorld: could not gate a consistent harvest/world snapshot (%s)"),
               *HarvestDiagnostic.ToString());
        OnSaveGameActionFinished.Broadcast(SafeSlotName, false);
        return;
    }
    ON_SCOPE_EXIT {
        HarvestWorld->EndSaveCapture();
    };

    if (!HarvestWorld->BuildSaveSnapshot(WorldData.HarvestWorld,
                                          HarvestDiagnostic)
        || !HarvestOutbox->BuildSaveSnapshot(
            WorldData.HarvestWorld.WorldEpoch,
            WorldData.HarvestRewardOutbox, HarvestDiagnostic)
        || WorldData.HarvestRewardOutbox.WorldEpoch
            != WorldData.HarvestWorld.WorldEpoch) {
        UE_LOG(MythSaveLoad, Error,
               TEXT("SaveWorld: harvest snapshot failed (%s)"),
               *HarvestDiagnostic.ToString());
        OnSaveGameActionFinished.Broadcast(SafeSlotName, false);
        return;
    }

    if (!FSerializedWorldActorHelper::SerializeAll(World,
                                                   DeferredPlacedActorRestores,
                                                   WorldData.SavedActors)) {
        UE_LOG(MythSaveLoad, Error,
               TEXT("SaveWorld: saved world actor capture failed"));
        OnSaveGameActionFinished.Broadcast(SafeSlotName, false);
        return;
    }

    if (UMythicLivingWorldSubsystem *LWS = GetGameInstance()->GetSubsystem<UMythicLivingWorldSubsystem>()) {
        FMemoryWriter LWWriter(WorldData.LivingWorldBlob);
        LWS->SaveLivingWorld(LWWriter);
    }

    if (const UMythicPOIDiscoverySubsystem *POI = GetGameInstance()->GetSubsystem<UMythicPOIDiscoverySubsystem>()) {
        TArray<TPair<int32, FMythicPOIRegistryEntry>> Entries;
        POI->GetUnlockedPOIsForSave(Entries);
        WorldData.UnlockedPOIs.Reset();
        WorldData.UnlockedPOIs.Reserve(Entries.Num());
        for (const TPair<int32, FMythicPOIRegistryEntry> &Pair : Entries) {
            FSerializedPOIUnlock Row;
            Row.POIId = Pair.Key;
            Row.Anchor = Pair.Value.Anchor;
            Row.POITag = Pair.Value.Tag;
            Row.DisplayName = Pair.Value.Name;
            Row.Radius = Pair.Value.Radius;
            WorldData.UnlockedPOIs.Add(MoveTemp(Row));
        }
    }
    if (const UMythicDiggingSubsystem *Digging = GetGameInstance()->GetSubsystem<UMythicDiggingSubsystem>()) {
        Digging->GetConsumedSiteIds(WorldData.ConsumedDigSiteIds);
    }
    if (const UWorld *FlagWorld = GetWorld()) {
        if (const UMythicWorldStateSubsystem *WorldState = FlagWorld->GetSubsystem<UMythicWorldStateSubsystem>()) {
            WorldData.WorldFlags = WorldState->GetWorldFlags();
        }
    }

    UMythicSaveGame *SaveObj = Cast<UMythicSaveGame>(UGameplayStatics::CreateSaveGameObject(UMythicSaveGame::StaticClass()));
    if (!SaveObj) {
        OnSaveGameActionFinished.Broadcast(SafeSlotName, false);
        return;
    }

    SaveObj->WorldData = WorldData;
    SaveObj->SaveSlotName = SafeSlotName;
    SaveObj->CreationTime = FDateTime::Now();

    TArray<uint8> TempBuffer;
    FMemoryWriter MemWriter(TempBuffer);
    FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
    SaveObj->Serialize(Ar);
    SaveObj->DataChecksum = ComputeChecksum(TempBuffer);

    FAsyncSaveGameToSlotDelegate SavedDelegate;
    SavedDelegate.BindUObject(this, &UMythicSaveGameSubsystem::HandleAsyncSaveFinished);

    InFlightWorldRewardOutboxSnapshots.Add(
        SafeSlotName, WorldData.HarvestRewardOutbox);
    InFlightSaveSlots.Add(SafeSlotName);
    UGameplayStatics::AsyncSaveGameToSlot(SaveObj, SafeSlotName, 0, SavedDelegate);
}

void UMythicSaveGameSubsystem::LoadWorld(const FString &SlotName) {
    if (SlotName.IsEmpty()) {
        UE_LOG(MythSaveLoad, Error, TEXT("LoadWorld: SlotName is empty"));
        OnSaveGameActionFinished.Broadcast(SlotName, false);
        return;
    }

    const FString SafeSlotName = SanitizeSlotName(SlotName);
    OnSaveGameActionStarted.Broadcast(SafeSlotName);

    if (!UGameplayStatics::DoesSaveGameExist(SafeSlotName, 0)) {
        UE_LOG(MythSaveLoad, Warning, TEXT("LoadWorld: Slot %s does not exist"), *SafeSlotName);
        OnSaveGameActionFinished.Broadcast(SafeSlotName, false);
        return;
    }

    if (!PendingWorldLoadSlots.IsEmpty()) {
        UE_LOG(MythSaveLoad, Warning,
               TEXT("LoadWorld: another world restore is already pending"));
        OnSaveGameActionFinished.Broadcast(SafeSlotName, false);
        return;
    }

    DeferredPlacedActorRestores.Empty();

    UWorld *World = GetWorld();
    UMythicHarvestWorldSubsystem *HarvestWorld = World
        ? World->GetSubsystem<UMythicHarvestWorldSubsystem>() : nullptr;
    FName HarvestDiagnostic;
    if (!HarvestWorld
        || !HarvestWorld->BeginSaveRestore(HarvestDiagnostic)) {
        UE_LOG(MythSaveLoad, Error,
               TEXT("LoadWorld: could not gate harvest state (%s)"),
               *HarvestDiagnostic.ToString());
        OnSaveGameActionFinished.Broadcast(SafeSlotName, false);
        return;
    }

    FAsyncLoadGameFromSlotDelegate LoadedDelegate;
    LoadedDelegate.BindUObject(this, &UMythicSaveGameSubsystem::HandleAsyncWorldLoadFinished);

    PendingWorldLoadSlots.Add(SafeSlotName);
    UGameplayStatics::AsyncLoadGameFromSlot(SafeSlotName, 0, LoadedDelegate);
}

void UMythicSaveGameSubsystem::HandleAsyncWorldLoadFinished(const FString &SlotName, const int32 UserIndex, USaveGame *LoadedSaveGame) {
    PendingWorldLoadSlots.Remove(SlotName);
    UWorld *World = GetWorld();
    UMythicHarvestWorldSubsystem *HarvestWorld = World
        ? World->GetSubsystem<UMythicHarvestWorldSubsystem>() : nullptr;
    auto AbortHarvestRestore = [HarvestWorld]() {
        if (HarvestWorld) {
            HarvestWorld->AbortSaveRestore();
        }
    };

    UMythicSaveGame *SaveObj = Cast<UMythicSaveGame>(LoadedSaveGame);
    if (!SaveObj) {
        UE_LOG(MythSaveLoad, Error, TEXT("AsyncWorldLoadFinished: Failed to cast"));
        AbortHarvestRestore();
        OnSaveGameActionFinished.Broadcast(SlotName, false);
        return;
    }

    if (SaveObj->DataChecksum.IsEmpty()) {
        AbortHarvestRestore();
        OnSaveGameActionFinished.Broadcast(SlotName, false);
        return;
    }

    TArray<uint8> TempBuffer;
    FString StoredChecksum = SaveObj->DataChecksum;
    SaveObj->DataChecksum = TEXT("");
    FMemoryWriter MemWriter(TempBuffer);
    FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
    SaveObj->Serialize(Ar);

    if (!ValidateChecksum(TempBuffer, StoredChecksum)) {
        UE_LOG(MythSaveLoad, Error, TEXT("AsyncWorldLoadFinished: Checksum failed"));
        AbortHarvestRestore();
        OnSaveGameActionFinished.Broadcast(SlotName, false);
        return;
    }
    SaveObj->DataChecksum = StoredChecksum;
    SaveObj->FixupData();

    if (World) {
        const FSerializedWorldData &Data = SaveObj->WorldData;

        UMythicHarvestRewardOutboxSubsystem *HarvestOutbox =
            World->GetSubsystem<UMythicHarvestRewardOutboxSubsystem>();
        FName HarvestDiagnostic;
        if (!HarvestWorld || !HarvestOutbox
            || !FMythicHarvestWorldSaveV1::Validate(
                Data.HarvestWorld, HarvestDiagnostic)
            || !UMythicHarvestRewardOutboxSubsystem::ValidateSaveSnapshot(
                Data.HarvestRewardOutbox, HarvestDiagnostic)
            || Data.HarvestRewardOutbox.WorldEpoch
                != Data.HarvestWorld.WorldEpoch) {
            UE_LOG(MythSaveLoad, Error,
                   TEXT("AsyncWorldLoadFinished: invalid harvest snapshot (%s)"),
                   *HarvestDiagnostic.ToString());
            AbortHarvestRestore();
            OnSaveGameActionFinished.Broadcast(SlotName, false);
            return;
        }

        if (const AGameStateBase *GameState = World->GetGameState()) {
            for (APlayerState *PlayerState : GameState->PlayerArray) {
                const AMythicPlayerState *MythicPlayerState =
                    Cast<AMythicPlayerState>(PlayerState);
                const UMythicHarvestReceiptLedgerComponent *Ledger =
                    MythicPlayerState
                    ? MythicPlayerState->GetHarvestReceiptLedger() : nullptr;
                if (Ledger
                    && !Ledger->ValidateWorldSnapshotMinimum(
                        Data.HarvestRewardOutbox.WorldEpoch,
                        Data.HarvestRewardOutbox.SnapshotSequence,
                        HarvestDiagnostic)) {
                    UE_LOG(MythSaveLoad, Error,
                           TEXT("AsyncWorldLoadFinished: world snapshot predates compacted player receipts (%s)"),
                           *HarvestDiagnostic.ToString());
                    AbortHarvestRestore();
                    OnSaveGameActionFinished.Broadcast(SlotName, false);
                    return;
                }
                if (Ledger && MythicPlayerState
                    && !MythicPlayerState->GetPersistentCharacterId().IsEmpty()) {
                    const UMythicHarvestRewardEscrowComponent *Escrow =
                        MythicPlayerState->GetHarvestRewardEscrow();
                    FMythicHarvestReceiptLedgerSaveV1 CharacterSnapshot;
                    FMythicHarvestItemEscrowSaveV1 EscrowSnapshot;
                    if (!Escrow) {
                        HarvestDiagnostic = TEXT("MissingHarvestRewardEscrowComponent");
                    }
                    if (!Escrow
                        || !Ledger->BuildSaveSnapshot(
                            CharacterSnapshot, HarvestDiagnostic)
                        || !Escrow->BuildSaveSnapshot(
                            EscrowSnapshot, HarvestDiagnostic)
                        || !UMythicHarvestRewardOutboxSubsystem::
                            ValidateCharacterReceiptSnapshotAgainstWorld(
                                Data.HarvestRewardOutbox,
                                MythicPlayerState->
                                    GetPersistentCharacterId(),
                                CharacterSnapshot,
                                EscrowSnapshot,
                                HarvestDiagnostic)) {
                        UE_LOG(MythSaveLoad, Error,
                               TEXT("AsyncWorldLoadFinished: candidate world/character harvest fence rejected (%s)"),
                               *HarvestDiagnostic.ToString());
                        AbortHarvestRestore();
                        OnSaveGameActionFinished.Broadcast(
                            SlotName, false);
                        return;
                    }
                }
            }
        }

        if (!HarvestWorld->PreflightSaveRestore(
                Data.HarvestWorld, Data.HarvestRewardOutbox,
                HarvestDiagnostic)) {
            UE_LOG(MythSaveLoad, Error,
                   TEXT("AsyncWorldLoadFinished: harvest transaction preflight failed (%s)"),
                   *HarvestDiagnostic.ToString());
            AbortHarvestRestore();
            OnSaveGameActionFinished.Broadcast(SlotName, false);
            return;
        }
        if (!FSerializedWorldActorHelper::PreflightDeserialize(
                World, Data.SavedActors, HarvestDiagnostic)) {
            UE_LOG(MythSaveLoad, Error,
                   TEXT("AsyncWorldLoadFinished: saved world actor preflight failed (%s)"),
                   *HarvestDiagnostic.ToString());
            AbortHarvestRestore();
            OnSaveGameActionFinished.Broadcast(SlotName, false);
            return;
        }
        if (!HarvestOutbox->RestoreSaveSnapshot(
                Data.HarvestRewardOutbox, HarvestDiagnostic)) {
            UE_LOG(MythSaveLoad, Fatal,
                   TEXT("AsyncWorldLoadFinished: a validated/preflighted durable harvest outbox failed installation (%s)"),
                   *HarvestDiagnostic.ToString());
        }
        if (!HarvestWorld->RestoreSaveSnapshot(
                Data.HarvestWorld, HarvestDiagnostic)) {
            UE_LOG(MythSaveLoad, Fatal,
                   TEXT("AsyncWorldLoadFinished: a preflighted harvest world restore failed after its durable outbox was installed (%s)"),
                   *HarvestDiagnostic.ToString());
        }

        if (!FSerializedWorldActorHelper::DeserializeAll(World,
                                                        Data.SavedActors,
                                                        DeferredPlacedActorRestores)) {
            UE_LOG(MythSaveLoad, Fatal,
                   TEXT("AsyncWorldLoadFinished: a preflighted saved world actor domain failed installation"));
        }

        if (Data.LivingWorldBlob.Num() > 0) {
            if (UMythicLivingWorldSubsystem *LWS = GetGameInstance()->GetSubsystem<UMythicLivingWorldSubsystem>()) {
                FMemoryReader LWReader(Data.LivingWorldBlob, true);
                LWS->LoadLivingWorld(LWReader);
            }
        }

        if (Data.UnlockedPOIs.Num() > 0) {
            if (UMythicPOIDiscoverySubsystem *POI = GetGameInstance()->GetSubsystem<UMythicPOIDiscoverySubsystem>()) {
                for (const FSerializedPOIUnlock &Row : Data.UnlockedPOIs) {
                    POI->ServerUnlockPOI(Row.POIId, Row.Anchor, Row.POITag, Row.DisplayName, Row.Radius);
                }
            }
        }
        if (Data.ConsumedDigSiteIds.Num() > 0) {
            if (UMythicDiggingSubsystem *Digging = GetGameInstance()->GetSubsystem<UMythicDiggingSubsystem>()) {
                Digging->LoadConsumedSiteIds(Data.ConsumedDigSiteIds);
            }
        }
        if (!Data.WorldFlags.IsEmpty()) {
            if (UMythicWorldStateSubsystem *WorldState = World->GetSubsystem<UMythicWorldStateSubsystem>()) {
                for (const FGameplayTag &Flag : Data.WorldFlags) {
                    WorldState->ServerSetFlag(Flag);
                }
            }
        }

        if (!HarvestWorld->CompleteSaveRestore(HarvestDiagnostic)) {
            UE_LOG(MythSaveLoad, Fatal,
                   TEXT("AsyncWorldLoadFinished: a fully applied world could not release the harvest restore barrier (%s)"),
                   *HarvestDiagnostic.ToString());
        }
    }
    else {
        AbortHarvestRestore();
        OnSaveGameActionFinished.Broadcast(SlotName, false);
        return;
    }

    UE_LOG(MythSaveLoad, Log, TEXT("AsyncWorldLoadFinished: Success for %s"), *SlotName);
    OnSaveGameActionFinished.Broadcast(SlotName, true);
}


void UMythicSaveGameSubsystem::FindSaveGames(TArray<FString> &OutSaveFiles) const {
    const FString SavesFolder = FPaths::ProjectSavedDir() / TEXT("SaveGames");
    const FString Extension = TEXT("*.sav");

    IFileManager::Get().FindFiles(OutSaveFiles, *(SavesFolder / Extension), true, false);

    for (FString &File : OutSaveFiles) {
        File = FPaths::GetBaseFilename(File);
    }
}


TArray<FMythicCharacterMetadata> UMythicSaveGameSubsystem::GetCharacterList() {
    TArray<FMythicCharacterMetadata> Result;

    if (UMythicSaveGameManifest *Manifest = Cast<UMythicSaveGameManifest>(UGameplayStatics::LoadGameFromSlot(TEXT("MythicCharacterManifest"), 0))) {
        Manifest->CharacterSlots.GenerateValueArray(Result);
    }

    Result.Sort([](const FMythicCharacterMetadata &A, const FMythicCharacterMetadata &B) {
        return A.LastPlayed > B.LastPlayed;
    });

    return Result;
}

FString UMythicSaveGameSubsystem::CreateNewCharacter(const FString &DisplayName, const FString &ClassName, bool bHardcore, const FString &ExplicitCharacterID) {
    const FString NewSlotName = ExplicitCharacterID.IsEmpty() ? FGuid::NewGuid().ToString() : SanitizeSlotName(ExplicitCharacterID);

    FMythicCharacterMetadata NewChar;
    NewChar.CharacterID = NewSlotName;
    NewChar.DisplayName = DisplayName;
    NewChar.ClassName = ClassName;
    NewChar.bIsHardcore = bHardcore;
    NewChar.Level = 1;
    NewChar.LastPlayed = FDateTime::Now();

    UMythicSaveGame *NewSave = Cast<UMythicSaveGame>(UGameplayStatics::CreateSaveGameObject(UMythicSaveGame::StaticClass()));
    if (NewSave) {
        NewSave->SaveSlotName = NewSlotName;
        NewSave->CreationTime = NewChar.LastPlayed;
        NewSave->CharacterData.CharacterID = NewSlotName;
        NewSave->CharacterData.CharacterName = DisplayName;
        NewSave->CharacterData.DataVersion = static_cast<int32>(CurrentCharacterSaveVersion);
        do {
            NewSave->CharacterData.PlayerEntityId =
                FMythicEntityId::FromAuthorityGuid(
                    EMythicEntityDomain::PlayerCharacter,
                    FGuid::NewGuid());
        } while (!NewSave->CharacterData.PlayerEntityId.IsValid());
        do {
            NewSave->CharacterData.HarvestReceiptLedger.LedgerEpoch =
                FGuid::NewGuid();
        } while (!NewSave->CharacterData.HarvestReceiptLedger.LedgerEpoch.IsValid());
        do {
            NewSave->CharacterData.HarvestItemEscrow.EscrowEpoch =
                FGuid::NewGuid();
        } while (!NewSave->CharacterData.HarvestItemEscrow.EscrowEpoch.IsValid());
        NewSave->CharacterData.HarvestItemEscrow.EscrowRevision = 1;

        TArray<uint8> TempBuffer;
        FMemoryWriter MemWriter(TempBuffer);
        FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
        NewSave->Serialize(Ar);
        NewSave->DataChecksum = ComputeChecksum(TempBuffer);

        UGameplayStatics::SaveGameToSlot(NewSave, NewSlotName, 0);
    }

    UpdateManifestInternal(NewChar);

    return NewSlotName;
}

bool UMythicSaveGameSubsystem::DeleteCharacter(const FString &CharacterID) {
    if (CharacterID.IsEmpty()) {
        return false;
    }

    FMythicCharacterMetadata Dummy;
    Dummy.CharacterID = CharacterID;
    UpdateManifestInternal(Dummy, true);

    const FString SafeSlot = SanitizeSlotName(CharacterID);
    if (UGameplayStatics::DoesSaveGameExist(SafeSlot, 0)) {
        return UGameplayStatics::DeleteGameInSlot(SafeSlot, 0);
    }

    return true;
}

void UMythicSaveGameSubsystem::UpdateManifestInternal(const FMythicCharacterMetadata &Metadata, bool bRemove) {
    UMythicSaveGameManifest *Manifest = Cast<UMythicSaveGameManifest>(UGameplayStatics::LoadGameFromSlot(TEXT("MythicCharacterManifest"), 0));

    if (!Manifest) {
        Manifest = Cast<UMythicSaveGameManifest>(UGameplayStatics::CreateSaveGameObject(UMythicSaveGameManifest::StaticClass()));
    }

    if (!Manifest) {
        return;
    }

    if (bRemove) {
        Manifest->CharacterSlots.Remove(Metadata.CharacterID);
    }
    else {
        Manifest->CharacterSlots.Add(Metadata.CharacterID, Metadata);
    }

    UGameplayStatics::SaveGameToSlot(Manifest, TEXT("MythicCharacterManifest"), 0);
}
