#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/NetSerialization.h"
#include "World/Entity/MythicEntityPresentationTypes.h"
#include "MythicCombatUtilityComponent.generated.h"

class AMythicPlayerController;
class UCameraRigAsset;
class UMythicEntityPresentationComponent;
class UMythicEntityPresentationRegistry;
class UMythicHarvestClaimMembershipSubsystem;

UENUM(BlueprintType)
enum class EMythicPingIntent : uint8 { GoHere, Danger, NeedHelp, Loot, Wait, DefendHere, Regroup, Acknowledge };

/** A permitted location/intent, never a private player key, faction, inventory or canonical Mass identity. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicPartyPing {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FGuid SenderToken;
    UPROPERTY(BlueprintReadOnly) FGuid PingId;
    UPROPERTY(BlueprintReadOnly) FMythicEntityPresentationInstance Subject;
    UPROPERTY(BlueprintReadOnly) FVector_NetQuantize Location=FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) EMythicPingIntent Intent=EMythicPingIntent::GoHere;
    UPROPERTY(BlueprintReadOnly) double ExpiryServerTime=0;
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMythicCombatUtilityProjectionChanged);

/** Controller-owned input endpoints and owner-only lock/ping transport. No authored skill or position authority. */
UCLASS(BlueprintType,ClassGroup=(Input),meta=(BlueprintSpawnableComponent))
class MYTHIC_API UMythicCombatUtilityComponent : public UActorComponent {
    GENERATED_BODY()
public:
    UMythicCombatUtilityComponent();
    UFUNCTION(BlueprintCallable,Category="Combat Utility") bool ToggleTargetLock();
    UFUNCTION(BlueprintCallable,Category="Combat Utility") void ClearTargetLock();
    UFUNCTION(BlueprintCallable,Category="Combat Utility") bool TryCapturePingIntent(FMythicEntityPresentationInstance& Subject,FVector& Point);
    UFUNCTION(BlueprintCallable,Category="Combat Utility") bool SubmitPing(EMythicPingIntent Intent,FMythicEntityPresentationInstance Subject,FVector Point);
    UFUNCTION(BlueprintCallable,Category="Combat Utility") void ResetLocalUtilityState();
    UFUNCTION(BlueprintPure,Category="Combat Utility") FMythicEntityPresentationInstance GetLockedInstance() const { return LocalLock; }
    UFUNCTION(BlueprintPure,Category="Combat Utility") FMythicEntityPresentationInstance GetVisibleLockedInstance() const;
    UFUNCTION(BlueprintPure,Category="Combat Utility") TArray<FMythicPartyPing> GetActivePings() const;
    /** Hostile-only nomination. Current camera aim policy and LOS are required again on every call. */
    bool TryGetLockedAimDirection(float Range,FVector& Direction);
    /** Runs after ordinary local input/ability sampling. No nomination, RPC or pawn movement is performed. */
    void AdvanceLockYawAssist(float DeltaSeconds);
    /** Called only for real filtered look values and local input lifecycle resets. */
    void NotifyManualLook(FVector2D Value);
    void SuppressLockYawAssist();
    /** Called before logout/unpossess; removes this sender's exact accepted work and authority lock. */
    void AuthorityEndUtilitySession();
    UPROPERTY(BlueprintAssignable,Category="Combat Utility") FMythicCombatUtilityProjectionChanged OnUtilityProjectionChanged;

    // Read-only runtime copy of MythicControlsSettings; there is one authored policy source.
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Combat Utility|Lock") float LockAcquireRange=2000;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Combat Utility|Lock") float LockAcquireDegrees=20;
    /** Local WorldCursor acquisition/assist radius in raw viewport pixels; never world distance. */
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Combat Utility|Lock") float LockCursorRadius=250;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Combat Utility|Lock") float LockMaintainRange=2400;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Combat Utility|Lock") float LockLineOfSightGrace=0.75f;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Combat Utility|Ping") float PingRange=10000;
    UPROPERTY(VisibleInstanceOnly,BlueprintReadOnly,Category="Combat Utility|Ping") float PingLifetime=8;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction) override;

private:
    UFUNCTION(Server,Reliable) void ServerRequestLock(uint32 RequestId,bool bAcquire,FVector_NetQuantizeNormal Direction,bool bWorldCursor,FVector_NetQuantize Point,const TArray<FMythicEntityPresentationInstance>& VisibleCandidates);
    UFUNCTION(Client,Reliable) void ClientSetLock(uint32 RequestId,FMythicEntityPresentationInstance Instance);
    UFUNCTION(Server,Reliable) void ServerSubmitPing(EMythicPingIntent Intent,FMythicEntityPresentationInstance Subject,FVector_NetQuantize Point);
    UFUNCTION(Client,Reliable) void ClientReceivePing(FMythicPartyPing Ping);
    UFUNCTION(Client,Reliable) void ClientRemovePing(FGuid Sender,FGuid Id);
    AMythicPlayerController* Controller() const;
    bool CaptureAim(float Range,FVector& Eye,FVector& Direction,bool& bWorldCursor,FVector& Point) const;
    bool HasSight(const AMythicPlayerController* Viewer,AActor* Subject,const FVector& Point,bool bRequireLocalView=true) const;
    UMythicEntityPresentationComponent* ResolvePublic(const FMythicEntityPresentationInstance& Instance) const;
    bool ResolveAuthoritySubject(const FMythicEntityPresentationInstance& Instance,AActor*& Actor) const;
    bool IsAuthorityHostile(AActor* Target) const;
    bool HasLiveOwner() const;
    bool FitsAim(AActor* Actor,const FVector& Eye,const FVector& Direction,bool bWorldCursor,const FVector& Point,double& Score) const;
    bool IsRecipientAllowed(AMythicPlayerController* Recipient) const;
    void RevokeAuthorityPing();
    void RefreshAuthorityPing(bool bForce);
    void PruneLocalPings();
    void UpdateLocalLock();
    void ApplyLocalLock(FMythicEntityPresentationInstance Instance);
    void HandleUnregistered(const FMythicEntityPresentationInstance& Instance,UMythicEntityPresentationComponent* Component);
    void HandleMembershipChanged();
    void RefreshTick();
    double ServerTime() const;
    FMythicEntityPresentationInstance LocalLock;
    FMythicEntityPresentationInstance PublishedAttentionLock;
    FMythicEntityPresentationInstance AuthorityLock;
    TWeakObjectPtr<AActor> AuthorityAvatar;
    FMythicEntityPresentationInstance AuthorityAvatarInstance;
    TArray<FMythicPartyPing> LocalPings;
    FMythicPartyPing AuthorityPing;
    struct FRecipient { TWeakObjectPtr<UMythicCombatUtilityComponent> Component; FMythicPartyPing Last; };
    TArray<FRecipient> Recipients;
    FGuid SenderToken;
    uint32 LocalLockRequest=0;
    uint32 AuthorityLockRequest=0;
    bool bLockRequestPending=false;
    bool bLocalLockVisible=false;
    bool bStopping=false;
    bool bSubmittingPing=false;
    double LockLastVisible=0;
    double LastLockRequestTime=-DBL_MAX;
    double LastPingTokenTime=0;
    double NextPingRefresh=0;
    double PingTokens=2;
    TWeakObjectPtr<UMythicEntityPresentationRegistry> BoundRegistry;
    TWeakObjectPtr<UMythicHarvestClaimMembershipSubsystem> BoundMembership;
    FDelegateHandle RegistryHandle;
    FDelegateHandle MembershipHandle;
    FDelegateHandle OwnerHandle;
    double LockYawSuppressedUntil=0;
    uint64 LastLockYawFrame=MAX_uint64;
    TWeakObjectPtr<UCameraRigAsset> LastLockYawRig;
};
