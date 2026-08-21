
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "World/LivingWorld/Chronicle/MythicWorldChronicleSubsystem.h"
#include "MythicChronicleRelayComponent.generated.h"

class UMythicWorldChronicleSubsystem;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicChronicleRelayComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicChronicleRelayComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION()
    void HandleChronicleEntry(const FMythicChronicleEntry &Entry);

    UFUNCTION()
    void OnRep_ReplicatedChronicle();

    UPROPERTY(ReplicatedUsing = OnRep_ReplicatedChronicle)
    TArray<FMythicChronicleEntry> ReplicatedChronicle;

private:
    UMythicWorldChronicleSubsystem *ResolveChronicle() const;

    int32 LastIngestedSequence = 0;

    static constexpr int32 MaxRelayEntries = 256;
};
