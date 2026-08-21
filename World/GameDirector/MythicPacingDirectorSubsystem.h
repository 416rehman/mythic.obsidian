
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/DataAsset.h"
#include "World/GameDirector/MythicDirectorPacing.h"
#include "MythicPacingDirectorSubsystem.generated.h"

class UAbilitySystemComponent;
class UMythicLifeComponent;
struct FGameplayEventData;

UCLASS(BlueprintType)
class MYTHIC_API UMythicDirectorConfigAsset : public UDataAsset {
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director")
    FMythicDirectorConfig Config;
};

UCLASS()
class MYTHIC_API UMythicPacingDirectorSubsystem : public UWorldSubsystem {
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;


    /** The current spawn-intensity multiplier. Always within [Config.Min, Config.Max]. Multiply spawn probability / budget by this. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Director")
    float GetSpawnIntensityMultiplier() const { return State.SpawnIntensityMultiplier; }

    /** The current pacing phase (BuildUp / Peak / SustainPeak / Relax / Rest). */
    UFUNCTION(BlueprintPure, Category = "Mythic|Director")
    EMythicDirectorPhase GetPhase() const { return State.Phase; }

    const FMythicDirectorState& GetState() const { return State; }

    const FMythicDirectorInputs& GetLastInputs() const { return LastInputs; }


    /** Push an authored config asset (null → revert to code defaults). Safe to call at runtime. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Director")
    void SetConfigAsset(const UMythicDirectorConfigAsset* InAsset);

    void SetConfig(const FMythicDirectorConfig& InConfig) { Config = InConfig; }

    const FMythicDirectorConfig& GetConfig() const { return Config; }

protected:
    void SampleAndStep();

    FMythicDirectorInputs GatherInputs(float DeltaSeconds);

    void RefreshPartyBindings();

    void UnbindAll();

    UFUNCTION()
    void HandlePlayerDowned(AActor* DownedActor);

    void HandleKillEvent(const FGameplayEventData* Payload);

private:
    FMythicDirectorState State;

    FMythicDirectorConfig Config;

    FMythicDirectorInputs LastInputs;

    FTimerHandle SampleTimerHandle;

    float SampleInterval = 2.5f;

    float WindowSeconds = 10.0f;

    float DamageAccumNorm = 0.0f;

    double LastThreatTime = -1.0e9;

    TArray<double> DownTimestamps;
    TArray<double> KillTimestamps;

    TMap<TWeakObjectPtr<APawn>, float> PrevHealthByPawn;

    TMap<TWeakObjectPtr<UMythicLifeComponent>, bool> BoundLifeComps;

    TMap<TWeakObjectPtr<UAbilitySystemComponent>, FDelegateHandle> BoundKillASCs;
};
