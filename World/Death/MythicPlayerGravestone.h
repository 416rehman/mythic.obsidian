
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/IMythicInteractable.h"
#include "Subsystem/SaveSystem/World/MythicSaveableActor.h"
#include "MythicPlayerGravestone.generated.h"

class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UItemDefinition;
class UCommonGenericInputActionDataTable;
class AMythicPlayerController;
class AMythicPlayerState;

UCLASS()
class MYTHIC_API AMythicPlayerGravestone : public AActor, public IMythicInteractable, public IMythicSaveableActor {
    GENERATED_BODY()

public:
    AMythicPlayerGravestone();

    void ServerInitializeStake(AMythicPlayerState *OwnerPS, int32 Amount, UItemDefinition *InCurrencyDef, const FTransform &DeathTransform);

    UFUNCTION(BlueprintPure, Category = "Gravestone")
    int32 GetStakedGold() const { return StakedGold; }

    // The canonical cross-session key of the player who died here (empty for a transient/unsaved player — the session
    // PlayerId fallback still gates ownership). Exposed for UI attribution ("Your grave" / "<Name>'s grave").
    UFUNCTION(BlueprintPure, Category = "Gravestone")
    FString GetOwnerPlayerKey() const { return OwnerPlayerKey; }

    UFUNCTION(BlueprintPure, Category = "Gravestone")
    FText GetOwnerDisplayName() const { return OwnerDisplayName; }

    virtual void OnPrimaryInteract_Implementation(AActor *Interactor) override;
    virtual void OnSecondaryInteract_Implementation(AActor *Interactor) override;
    virtual USceneComponent *GetWidgetAttachmentComponent_Implementation() const override;
    virtual bool GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const override;
    virtual void OnFocused_Implementation(AActor *Interactor) override;
    virtual void OnUnfocused_Implementation(AActor *Interactor) override;

    virtual void DeserializeCustomData(const TArray<uint8> &InCustomData) override;

    bool IsActorInRange(const AActor *Actor) const;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravestone")
    USceneComponent *SceneRoot;

    // Query-only collision proxy so the interaction sweep (ECC_Visibility) detects the stone even before a BP assigns a
    // visual mesh — keeps the raw C++ gravestone interactable unauthored. Mirrors the grave.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravestone")
    USphereComponent *InteractionBounds;

    // Placeholder headstone visual. A BP gravestone swaps this for authored art.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravestone")
    UStaticMeshComponent *GravestoneMesh;

    // Interaction prompt data (same shape as the grave / storage container).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    TObjectPtr<const UCommonGenericInputActionDataTable> InputActionDataTable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FName PrimaryInteractionName = FName("Recover");

    // Squared distance within which a player may recover the stake. <= 0 disables the range gate. Mirrors the grave.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravestone")
    float ServerUseRangeSq = 250000.0f; // 500cm

    // Fired on ALL clients (multicast) when the stake is recovered, right before the stone is destroyed, so a BP can
    // play a "stake reclaimed" VFX/SFX cue at the death site. Editor handoff, same shape as the grave's OnEpitaphRead.
    UFUNCTION(BlueprintImplementableEvent, Category = "Gravestone")
    void OnStakeRecovered(APlayerController *Recoverer);

    // Cosmetic hook: identity/stake was (re)stamped — a BP updates the headstone inscription/visual here (client+server).
    UFUNCTION(BlueprintImplementableEvent, Category = "Gravestone")
    void OnGravestoneInitialized();


    // Gold cached in the stone. ReplicatedUsing so clients fire the cosmetic hook when it (re)stamps.
    UPROPERTY(ReplicatedUsing = OnRep_Init, VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Gravestone", meta = (AllowPrivateAccess = "true"))
    int32 StakedGold = 0;

    // Owner identity — canonical cross-session key (wins when set) + the session PlayerId fallback + display name.
    UPROPERTY(Replicated, SaveGame, BlueprintReadOnly, Category = "Gravestone", meta = (AllowPrivateAccess = "true"))
    FString OwnerPlayerKey;

    UPROPERTY(Replicated, SaveGame)
    int32 OwnerPlayerId = -1;

    UPROPERTY(Replicated, SaveGame, BlueprintReadOnly, Category = "Gravestone", meta = (AllowPrivateAccess = "true"))
    FText OwnerDisplayName;

    UPROPERTY(SaveGame)
    TObjectPtr<UItemDefinition> CurrencyDef;

    UPROPERTY(Replicated, SaveGame)
    float DeathTime = 0.0f;

    UFUNCTION()
    void OnRep_Init();

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_OnRecovered(APlayerController *Recoverer);

    static class AController *ResolveController(AActor *Interactor);

private:
    void ServerTryRecover(AMythicPlayerController *PC);

    void ResolveRecoveryEligibility(AMythicPlayerController *PC, bool &bOutIsOwner, bool &bOutIsPartyMember) const;

    void ServerGrantGoldTo(AMythicPlayerController *PC, int32 Amount);

    float GetAgeSeconds() const;
    void StartOrResumeExpiry();
    void CheckExpiry();
    FTimerHandle ExpiryTimerHandle;
    bool bExpiryStarted = false;
    float LifetimeSeconds = 1200.0f;
};
