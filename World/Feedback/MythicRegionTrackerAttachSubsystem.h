
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MythicRegionTrackerAttachSubsystem.generated.h"

class AGameModeBase;
class APlayerController;

UCLASS()
class MYTHIC_API UMythicRegionTrackerAttachSubsystem : public UWorldSubsystem {
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;

private:
    void HandlePostLogin(AGameModeBase *GameMode, APlayerController *NewPlayer);

    bool IsAuthority() const;

    FDelegateHandle PostLoginHandle;
};
