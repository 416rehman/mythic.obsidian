#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Mythic/Subsystem/SaveSystem/Character/CharacterData.h"
#include "Mythic/Subsystem/SaveSystem/World/WorldData.h"
#include "Mythic/Subsystem/SaveSystem/MythicSaveGameManifest.h"
#include "Mythic/World/Harvesting/MythicHarvestReceiptTypes.h"
#include "MythicSaveGameSubsystem.generated.h"

class UMythicSaveGame;
class USaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveGameActionStarted, const FString&, SlotName);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSaveGameActionFinished, const FString&, SlotName, bool, bSuccess);

/** Exact immutable character receipt snapshot associated with one completed physical save write. */
struct MYTHIC_API FMythicDurableCharacterSaveResult {
    FGuid OperationId;
    FString RoutingCharacterId;
    bool bSuccess = false;
    FMythicHarvestReceiptLedgerSaveV1 CapturedHarvestReceipts;
    FMythicHarvestItemEscrowSaveV1 CapturedHarvestItemEscrow;
};

DECLARE_DELEGATE_OneParam(FMythicDurableCharacterSaveComplete,
                          const FMythicDurableCharacterSaveResult &);

UCLASS()
class MYTHIC_API UMythicSaveGameSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;

    /** Placed saveable actors whose World Partition cell was not resident at restore; applied as each cell streams in. */
    int32 GetDeferredPlacedActorRestoreCount() const { return DeferredPlacedActorRestores.Num(); }

    /** Broadcast on the game thread before a character/world save or load begins; presentation listeners are read-only. */
    UPROPERTY(BlueprintAssignable, Category = "Save System | Events")
    FOnSaveGameActionStarted OnSaveGameActionStarted;

    /** Broadcast on the game thread when a save/load finishes; false includes validation, authority, and I/O failure. */
    UPROPERTY(BlueprintAssignable, Category = "Save System | Events")
    FOnSaveGameActionFinished OnSaveGameActionFinished;

    /** Development-only world slot selected only by the explicit -MythicUseDebugWorld command-line opt-in. */
    static constexpr const TCHAR *DebugWorldSlot = TEXT("DebugWorld");
    static constexpr const TCHAR *DebugCharacterSlot = TEXT("DebugCharacter");

    static FString ResolvePerPlayerCharacterSlot(const FString &StablePlayerId);

    /**
     * Resolves the authority world's one persistence route from -MythicWorldSlot=<deployment-instance-id>.
     * Non-Shipping builds may explicitly opt into DebugWorld with -MythicUseDebugWorld; Shipping never may.
     */
    static bool TryResolveAuthorityWorldSlot(FString &OutSlotName);

    /** Asynchronously captures SourceActor on the game thread and writes its validated character slot in the background. */
    UFUNCTION(BlueprintCallable, Category = "Save System")
    void SaveCharacter(AActor *SourceActor, const FString &CharacterID);

    /**
     * Native request-scoped durability barrier. Same-slot requests coalesce behind an in-flight physical write; the
     * callback receives the exact receipt snapshot actually written and never infers durability from a global event.
     */
    bool RequestDurableCharacterSave(
        AActor *SourceActor,
        const FString &CharacterID,
        FMythicDurableCharacterSaveComplete Completion,
        FGuid &OutOperationId);

    /** Asynchronously loads CharacterID into TargetActor; invalid/checksum-failed data leaves the actor unchanged. */
    UFUNCTION(BlueprintCallable, Category = "Save System")
    void LoadCharacter(AActor *TargetActor, const FString &CharacterID);

    /** Returns sanitized local save-slot names; this read-only query performs no load or mutation. */
    UFUNCTION(BlueprintCallable, Category = "Save System")
    TArray<FString> GetLocalSaveFiles() const;

    /** Captures the authoritative world, including stable harvest lifecycle/outbox state, then saves asynchronously. */
    UFUNCTION(BlueprintCallable, Category = "Save System")
    void SaveWorld(const FString &SlotName);

    /** Gates harvest transactions, validates the full world snapshot, and asynchronously restores an authority world. */
    UFUNCTION(BlueprintCallable, Category = "Save System")
    void LoadWorld(const FString &SlotName);

    static bool SerializeCharacterToStruct(AActor *SourceActor, FSerializedCharacterData &OutData);
    static bool DeserializeCharacterFromStruct(AActor *TargetActor, const FSerializedCharacterData &InData);
    static bool ValidateCharacterData(const FSerializedCharacterData &InData, FString &OutError);

    /** Returns character metadata from the local manifest without loading a character into the active world. */
    UFUNCTION(BlueprintCallable, Category = "Save System | Manifest")
    TArray<FMythicCharacterMetadata> GetCharacterList();

    /** Creates and persists a character slot; ExplicitCharacterID reuses a sanitized deterministic slot when supplied. */
    UFUNCTION(BlueprintCallable, Category = "Save System | Manifest")
    FString CreateNewCharacter(const FString &DisplayName, const FString &ClassName, bool bHardcore, const FString &ExplicitCharacterID = TEXT(""));

    /** Deletes the sanitized character slot and removes its manifest row; false means no recoverable deletion occurred. */
    UFUNCTION(BlueprintCallable, Category = "Save System | Manifest")
    bool DeleteCharacter(const FString &CharacterID);


    int32 GetInFlightSaveCount() const { return InFlightSaveSlots.Num(); }

    int32 GetPendingLoadCount() const { return PendingLoadTargets.Num(); }

    int32 GetPendingWorldLoadCount() const { return PendingWorldLoadSlots.Num(); }

private:

    struct FQueuedCharacterSave {
        FGuid OperationId;
        TWeakObjectPtr<AActor> SourceActor;
        TArray<FMythicDurableCharacterSaveComplete> Completions;
        bool bBroadcastPresentationEvents = false;
    };

    struct FInFlightCharacterSave {
        FGuid OperationId;
        FMythicHarvestReceiptLedgerSaveV1 CapturedHarvestReceipts;
        FMythicHarvestItemEscrowSaveV1 CapturedHarvestItemEscrow;
        TArray<FMythicDurableCharacterSaveComplete> Completions;
        bool bBroadcastPresentationEvents = false;
    };

    bool QueueCharacterSave(
        AActor *SourceActor,
        const FString &CharacterID,
        FMythicDurableCharacterSaveComplete Completion,
        bool bBroadcastPresentationEvents,
        FGuid *OutOperationId = nullptr);
    bool StartCharacterSave(const FString &SafeSlotName,
                            FQueuedCharacterSave &&Request);
    void StartNextQueuedCharacterSave(const FString &SafeSlotName);

    void HandleLevelAddedToWorld(ULevel *Level, UWorld *World);

    void HandleAsyncSaveFinished(const FString &SlotName, const int32 UserIndex, bool bSuccess);
    void HandleAsyncLoadFinished(const FString &SlotName, const int32 UserIndex, USaveGame *LoadedSaveGame);
    void HandleAsyncWorldLoadFinished(const FString &SlotName, const int32 UserIndex, USaveGame *LoadedSaveGame);


    TMap<FString, TWeakObjectPtr<AActor>> PendingLoadTargets;

    TSet<FString> PendingWorldLoadSlots;

    TSet<FString> InFlightSaveSlots;

    TMap<FString, FQueuedCharacterSave> QueuedCharacterSaves;

    TMap<FString, FInFlightCharacterSave> InFlightCharacterSaves;

    /** Exact outbox snapshots captured by in-flight world writes, used only to publish successful durability bounds. */
    TMap<FString, FMythicHarvestRewardOutboxSaveV1>
        InFlightWorldRewardOutboxSnapshots;

    /** Placed records whose cell was not resident at restore; applied on stream-in and re-emitted by the next save. */
    TArray<FSerializedWorldActorData> DeferredPlacedActorRestores;

    FDelegateHandle LevelAddedToWorldHandle;


    static FString SanitizeSlotName(const FString &Input);

    static FString ComputeChecksum(const TArray<uint8> &Data);

    static bool ValidateChecksum(const TArray<uint8> &Data, const FString &ExpectedChecksum);

    void FindSaveGames(TArray<FString> &OutSaveFiles) const;
    void UpdateManifestInternal(const FMythicCharacterMetadata &Metadata, bool bRemove = false);
};
