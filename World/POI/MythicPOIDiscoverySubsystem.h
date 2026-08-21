#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "Containers/Map.h"
#include "Containers/Set.h"
#include "MythicPOIDiscoverySubsystem.generated.h"

struct FMythicPOIProxyItem;
class AMythicPOIReplicator;
class APawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMythicOnPOIsChanged);

struct FMythicPOIRegistryEntry {
    FVector Anchor = FVector::ZeroVector;
    FGameplayTag Tag;
    FText Name;
    float Radius = 0.0f;
};

UCLASS()
class MYTHIC_API UMythicPOIDiscoverySubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void Deinitialize() override;

    void ServerUnlockPOI(int32 Id, FVector Anchor, FGameplayTag Tag, FText Name, float Radius = 0.0f);

    bool IsPOIUnlocked(int32 Id) const { return UnlockedPOIs.Contains(Id); }

    TArray<int32> GetUnlockedIds() const { return UnlockedPOIs.Array(); }

    bool ResolvePOIAnchor(int32 Id, FVector &OutAnchor) const;

    int32 ResolveCurrentPOI(const FVector &Location) const;

    void ServerFastTravelToPOI(APawn *TravelingPawn, int32 DestPOIId);

    // ── CLIENT ──
    /** Fired when the replicated POI set changes (client). UMG compass / war-map bind this. */
    UPROPERTY(BlueprintAssignable, Category = "POI")
    FMythicOnPOIsChanged OnPOIsChanged;

    void RegisterClientReplicator(AMythicPOIReplicator *InReplicator);

    void NotifyPOIsChanged() { OnPOIsChanged.Broadcast(); }

    void GetReplicatedPOIs(TArray<FMythicPOIProxyItem> &Out) const;

    void GetUnlockedPOIsForSave(TArray<TPair<int32, FMythicPOIRegistryEntry>> &Out) const;

private:
    void EnsureReplicator();

    TSet<int32> UnlockedPOIs;

    TMap<int32, FMythicPOIRegistryEntry> Registry;

    UPROPERTY()
    TObjectPtr<AMythicPOIReplicator> Replicator;
};
