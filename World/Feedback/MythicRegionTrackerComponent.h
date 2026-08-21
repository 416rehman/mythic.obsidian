
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Internationalization/Text.h"
#include "World/LivingWorld/Territory/MythicDanger.h"
#include "World/LivingWorld/Territory/MythicBiome.h"
#include "MythicRegionTrackerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMythicOnRegionDangerChanged, FText, Region, EMythicDangerTier, Tier);

UCLASS(ClassGroup = (Mythic), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicRegionTrackerComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicRegionTrackerComponent();


    static bool ShouldAnnounce(EMythicDangerTier NewTier, int32 NewSettlementId,
                               EMythicDangerTier LastTier, int32 LastSettlementId);

    static bool IsDangerIncrease(EMythicDangerTier NewTier, EMythicDangerTier LastTier);

    static FText ResolveRegionName(bool bInSettlement, const FText &SettlementName, EMythicBiome Biome);

    // ── Reads (server + owning client) ──
    UFUNCTION(BlueprintPure, Category = "World|Region")
    EMythicDangerTier GetCurrentDangerTier() const { return CurrentDangerTier; }

    UFUNCTION(BlueprintPure, Category = "World|Region")
    FText GetCurrentRegionName() const { return CurrentRegionName; }

    // ── Events ──
    UPROPERTY(BlueprintAssignable, Category = "World|Region")
    FMythicOnRegionDangerChanged OnRegionDangerChanged;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Per-player danger tier of the region the pawn currently stands in. COND_OwnerOnly: private per-player state on a
    // net-everyone PlayerState (mirrors the codex/faction-standing siblings). ReplicatedUsing so the owning client
    // re-broadcasts the change delegate.
    UPROPERTY(ReplicatedUsing = OnRep_RegionDanger, BlueprintReadOnly, Category = "World|Region")
    EMythicDangerTier CurrentDangerTier = EMythicDangerTier::Safe;

    // Per-player region name (settlement DisplayName or wilderness/biome label). Same COND_OwnerOnly rationale; same
    // OnRep (the two fields change together — the OnRep dedupes the paired notify).
    UPROPERTY(ReplicatedUsing = OnRep_RegionDanger, BlueprintReadOnly, Category = "World|Region")
    FText CurrentRegionName;

    UFUNCTION()
    void OnRep_RegionDanger();

private:
    void Sample();

    void BroadcastIfChanged(bool bIsInitialSeed);

    EMythicDangerTier LastDangerTier = EMythicDangerTier::COUNT;
    int32 LastSettlementId = INDEX_NONE;

    FTimerHandle SampleTimerHandle;
    bool bServerSeeded = false;

    bool bClientSeeded = false;
    EMythicDangerTier LastBroadcastTier = EMythicDangerTier::COUNT;
    FText LastBroadcastRegion;
};
