#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Mythic/Subsystem/SaveSystem/Character/CharacterData.h"
#include "Mythic/Subsystem/SaveSystem/World/WorldData.h"
#include "Mythic/Subsystem/SaveSystem/MythicSaveGameManifest.h"
#include "MythicSaveGameSubsystem.generated.h"

class UMythicSaveGame;
class USaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveGameActionStarted, const FString&, SlotName);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSaveGameActionFinished, const FString&, SlotName, bool, bSuccess);

UCLASS()
class MYTHIC_API UMythicSaveGameSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

public:

    UPROPERTY(BlueprintAssignable, Category = "Save System | Events")
    FOnSaveGameActionStarted OnSaveGameActionStarted;

    UPROPERTY(BlueprintAssignable, Category = "Save System | Events")
    FOnSaveGameActionFinished OnSaveGameActionFinished;

    static constexpr const TCHAR *DebugWorldSlot = TEXT("DebugWorld");
    static constexpr const TCHAR *DebugCharacterSlot = TEXT("DebugCharacter");

    static FString ResolvePerPlayerCharacterSlot(const FString &StablePlayerId);


    // Asynchronously saves character data. Validates source actor on GameThread, then writes to disk on background thread.
    UFUNCTION(BlueprintCallable, Category = "Save System")
    void SaveCharacter(AActor *SourceActor, const FString &CharacterID);

    UFUNCTION(BlueprintCallable, Category = "Save System")
    void LoadCharacter(AActor *TargetActor, const FString &CharacterID);

    UFUNCTION(BlueprintCallable, Category = "Save System")
    TArray<FString> GetLocalSaveFiles() const;


    UFUNCTION(BlueprintCallable, Category = "Save System")
    void SaveWorld(const FString &SlotName);

    UFUNCTION(BlueprintCallable, Category = "Save System")
    void LoadWorld(const FString &SlotName);

    static bool SerializeCharacterToStruct(AActor *SourceActor, FSerializedCharacterData &OutData);
    static bool DeserializeCharacterFromStruct(AActor *TargetActor, const FSerializedCharacterData &InData);
    static bool ValidateCharacterData(const FSerializedCharacterData &InData, FString &OutError);


    // Returns list of all available characters from the central manifest.
    UFUNCTION(BlueprintCallable, Category = "Save System | Manifest")
    TArray<FMythicCharacterMetadata> GetCharacterList();

    // Creates a new character slot and updates the manifest. Returns the generated SlotName (CharacterID).
    UFUNCTION(BlueprintCallable, Category = "Save System | Manifest")
    FString CreateNewCharacter(const FString &DisplayName, const FString &ClassName, bool bHardcore);

    // Deletes a character and their save file.
    UFUNCTION(BlueprintCallable, Category = "Save System | Manifest")
    bool DeleteCharacter(const FString &CharacterID);


    int32 GetInFlightSaveCount() const { return InFlightSaveSlots.Num(); }

    int32 GetPendingLoadCount() const { return PendingLoadTargets.Num(); }

private:

    void HandleAsyncSaveFinished(const FString &SlotName, const int32 UserIndex, bool bSuccess);
    void HandleAsyncLoadFinished(const FString &SlotName, const int32 UserIndex, USaveGame *LoadedSaveGame);
    void HandleAsyncWorldLoadFinished(const FString &SlotName, const int32 UserIndex, USaveGame *LoadedSaveGame);


    TMap<FString, TWeakObjectPtr<AActor>> PendingLoadTargets;

    TSet<FString> InFlightSaveSlots;


    static FString SanitizeSlotName(const FString &Input);

    static FString ComputeChecksum(const TArray<uint8> &Data);

    static bool ValidateChecksum(const TArray<uint8> &Data, const FString &ExpectedChecksum);

    void FindSaveGames(TArray<FString> &OutSaveFiles) const;
    void UpdateManifestInternal(const FMythicCharacterMetadata &Metadata, bool bRemove = false);
};
