#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/Secrets/MythicSecretTypes.h"
#include "MythicSecretVolume.generated.h"

class USphereComponent;
class AController;
struct FHitResult;

UCLASS()
class MYTHIC_API AMythicSecretVolume : public AActor {
    GENERATED_BODY()

public:
    AMythicSecretVolume();

protected:
    virtual void BeginPlay() override;
#if WITH_EDITOR
    virtual void OnConstruction(const FTransform &Transform) override;
#endif

    UFUNCTION()
    void OnSecretSphereBeginOverlap(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp,
                                    int32 OtherBodyIndex, bool bFromSweep, const FHitResult &Sweep);

    // The trigger sphere; radius kept in sync with TriggerRadius.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Secret")
    USphereComponent *SecretSphere;

    // The authored secret delivered on entry.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Secret")
    FMythicSecretDef Def;

    // Trigger radius (cm) — the "hidden spot" size. Drives the sphere's radius.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secret", meta = (ClampMin = "0.0"))
    float TriggerRadius = 200.0f;

    // When TRUE, the whole volume fires at most ONCE total (the first finder only) — a world-shared one-shot like a
    // landmark. DEFAULT FALSE: co-op-friendly — every player can find it (dedup is the per-player FoundTag latch + the
    // session guard below).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secret")
    bool bGlobalOneShot = false;

private:
    TSet<TWeakObjectPtr<AController>> RevealedControllers;

    bool bGlobalConsumed = false;
};
