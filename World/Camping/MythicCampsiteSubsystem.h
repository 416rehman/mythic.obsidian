#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/TimerHandle.h"
#include "UObject/ObjectKey.h"
#include "World/Camping/MythicCampsiteCore.h"
#include "MythicCampsiteSubsystem.generated.h"

class UMythicCampComponent;
class UMythicLivingWorldSubsystem;
class AMythicNPCCharacter;
class APlayerState;

struct FMythicResolvedCamp {
    TWeakObjectPtr<UMythicCampComponent> Anchor;
    TArray<TWeakObjectPtr<UMythicCampComponent>> Pieces;
    FVector Center = FVector::ZeroVector;
    int32 ComfortTier = 0;
};

UCLASS()
class MYTHIC_API UMythicCampsiteSubsystem : public UWorldSubsystem {
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void OnWorldBeginPlay(UWorld &InWorld) override;
    virtual void Deinitialize() override;

    void RegisterPiece(UMythicCampComponent *Piece);
    void UnregisterPiece(UMythicCampComponent *Piece);

    void NotifyCampStateChanged() { bClustersDirty = true; }

    bool GetCampAt(const FVector &Location, FMythicResolvedCamp &OutCamp);

    int32 GetComfortTierAt(const FVector &Location);

    void CollectComfortSources(const FVector &Center, float Radius, TArray<FMythicComfortSource> &OutSources);

    const FMythicCampingConfig &GetConfig() const { return Config; }

    bool WouldExceedPieceCap(const APlayerState *PlayerState) const;

    int32 GetLivePieceCount() const;
    int32 GetResolvedCampCount();

private:
    struct FPendingCampEvent {
        TWeakObjectPtr<UMythicCampComponent> Anchor;
        double DueTime = 0.0;
        int32 DangerTier = 0;
        bool bHostile = true;
    };

    void MarkClustersDirty() { bClustersDirty = true; }
    void ResolveClustersIfDirty();
    void PrunePieces();

    void EnforcePieceCap(const FString &PlayerKey);
    void CollapsePiece(UMythicCampComponent *Piece);

    void UpdateEventTimer();
    void HandleEventCheck();
    void EvaluateCampForEvents(const FMythicResolvedCamp &Camp, double Now);
    void ProcessDueEvents(double Now);
    void TelegraphEvent(const FMythicResolvedCamp &Camp, int32 DangerTier, bool bHostile, double Now);
    void DispatchAmbush(const FMythicResolvedCamp &Camp, int32 DangerTier);
    void DispatchMerchant(const FMythicResolvedCamp &Camp);
    void SubmitCampChronicle(const FVector &NearLocation, bool bDispatched) const;

    bool IsAuthority() const;
    double Now() const;
    bool IsPacingRestPhase() const;
    bool IsNight() const;
    int32 DangerTierAt(const FVector &Location) const;
    APawn *NearestPlayerPawn(const FVector &Location, float MaxRadius) const;
    bool AnyPlayerNear(const FVector &Location, float Radius) const;

    TArray<TWeakObjectPtr<UMythicCampComponent>> Pieces;
    TArray<FMythicResolvedCamp> ResolvedCamps;
    bool bClustersDirty = true;

    TArray<FPendingCampEvent> PendingEvents;
    TMap<FObjectKey, double> LastEventTimeByAnchor;
    FTimerHandle EventTimerHandle;

    FMythicCampingConfig Config;
    bool bEventsEnabled = false;
    bool bWarnedMissingAmbushContent = false;
    bool bWarnedMissingMerchantContent = false;
};
