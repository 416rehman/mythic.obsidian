#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "Tickable.h"
#include "GildedRenderSubsystem.generated.h"

/**
 * Enforces the "Gilded Lithograph" strict rendering configuration.
 * Disables Lumen, standard TAA, and overrides post-processing globally.
 */
UCLASS()
class MYTHIC_API UGildedRenderSubsystem : public UEngineSubsystem, public FTickableGameObject {
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;

    // FTickableGameObject interface
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override { return true; }
    virtual bool IsTickableInEditor() const override { return true; }
    virtual TStatId GetStatId() const override;

private:
    // applies console variables aggressively
    void EnforceRenderInvariants();
};
