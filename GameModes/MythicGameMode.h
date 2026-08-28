
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/GameModeBase.h"
#include "MythicGameMode.generated.h"

class UMythicAbilitySystemComponent;
class UMythicSaveGameSubsystem;
UCLASS()
class MYTHIC_API AMythicGameMode : public AGameModeBase {
    GENERATED_BODY()

public:
    // SERVER: respawn the controller's pawn after Delay seconds (0 = immediate). Safe to call on death.
    // A second request for the same controller cancels the first.
    UFUNCTION(BlueprintCallable, Category = "Mythic")
    void RequestRespawn(AController *Controller, float Delay = 5.0f);

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Logout(AController *Exiting) override;
    virtual void OnPostLogin(AController *NewPlayer) override;


    float GetAutosaveTimeRemaining() const;

    int32 GetPendingRespawnCount() const { return RespawnTimers.Num(); }

protected:
    void HandleRespawnTimer(AController *Controller);

    TMap<TWeakObjectPtr<AController>, FTimerHandle> RespawnTimers;


    void HandleAutosaveTimer();

    UMythicSaveGameSubsystem *GetSaveSubsystem() const;

    FString GetCharacterSlotForPlayer(const APlayerState *PS) const;

    FTimerHandle AutosaveTimerHandle;

    /** Authority persistence route resolved once from deployment command line; empty disables automatic world I/O. */
    FString AuthorityWorldSaveSlot;

    static constexpr float AutosaveIntervalSeconds = 300.0f;
};
