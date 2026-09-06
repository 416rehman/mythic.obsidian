#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MythicSessionPartySubsystem.generated.h"

class AMythicPlayerController;
class AMythicPlayerState;

/** World-lifetime publisher for the explicitly cooperative gameplay mode. No saved/social identity is invented. */
UCLASS()
class MYTHIC_API UMythicSessionPartySubsystem : public UWorldSubsystem {
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    void RegisterController(AMythicPlayerController* Controller);
    void UnregisterController(AMythicPlayerController* Controller);
    void NotifyPlayerReady(AMythicPlayerState* Player);
    void Reconcile();
    bool IsReadyController(const AMythicPlayerController* Controller) const;
    void GetReadyControllers(TArray<AMythicPlayerController*>& Controllers) const;
private:
    struct FJoinedPlayer {
        TWeakObjectPtr<AMythicPlayerController> Controller;
        TWeakObjectPtr<AMythicPlayerState> Player;
    };
    TArray<FJoinedPlayer> Joined;
    TArray<FString> PublishedKeys;
    FGuid SessionParty;
    bool bShuttingDown=false;
    bool bReconciling=false;
    bool bReconcileQueued=false;
    FTimerHandle ReconcileTimer;
};
