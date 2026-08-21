
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Interaction/IMythicInteractable.h"
#include "Subsystem/SaveSystem/World/MythicSaveableActor.h"
#include "MythicGrave.generated.h"

class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UCommonGenericInputActionDataTable;
class AMythicPlayerController;

USTRUCT(BlueprintType)
struct FMythicGraveIdentity {
    GENERATED_BODY()

    uint32 SourceNameHash = 0;

    // The deceased's display name, inscribed on the marker + shown when a player reads the grave.
    UPROPERTY(BlueprintReadWrite, Category = "Grave")
    FText DisplayName;

    // The composed epitaph text (already filled from a template at death — see FMythicEpitaph::Compose).
    UPROPERTY(BlueprintReadWrite, Category = "Grave")
    FText Epitaph;

    // Affiliation/faction the deceased belonged to (AI.Affiliation.*).
    UPROPERTY(BlueprintReadWrite, Category = "Grave")
    FGameplayTag Faction;

    // Living-world role of the deceased (from the cognitive brain), for role-specific raise results / flavour.
    UPROPERTY(BlueprintReadWrite, Category = "Grave")
    FGameplayTag RoleTag;

    // Combat tier of the deceased (GetAITierInt: Normal=1..Boss=5; 0 = unknown). Necromancy scales a raised power off it.
    UPROPERTY(BlueprintReadWrite, Category = "Grave")
    int32 SourceTier = 0;

    // Server world-seconds at time of death (chronological ordering / flavour).
    UPROPERTY(BlueprintReadWrite, Category = "Grave")
    float DeathTime = 0.0f;

    uint32 KillerNameHash = 0;

    // Stable key of the cemetery this grave belongs to (settlement tag name, or a synthesized wilderness key). Lets the
    // subsystem rebuild per-cemetery grave counts from the saved grave actors after a reload.
    UPROPERTY(BlueprintReadWrite, Category = "Grave")
    FName CemeteryKey;
};

UCLASS()
class MYTHIC_API AMythicGrave : public AActor, public IMythicInteractable, public IMythicSaveableActor {
    GENERATED_BODY()

public:
    AMythicGrave();

    void ServerInitializeFromDeath(const FMythicGraveIdentity &Identity, const FTransform &GraveTransform);

    UFUNCTION(BlueprintPure, Category = "Grave")
    FText GetEpitaph() const { return Epitaph; }

    UFUNCTION(BlueprintPure, Category = "Grave")
    FText GetDeceasedName() const { return DisplayName; }

    UFUNCTION(BlueprintPure, Category = "Grave")
    FGameplayTag GetGraveFaction() const { return Faction; }

    UFUNCTION(BlueprintPure, Category = "Grave")
    FGameplayTag GetGraveRole() const { return RoleTag; }

    UFUNCTION(BlueprintPure, Category = "Grave")
    int32 GetSourceTier() const { return SourceTier; }

    FName GetCemeteryKey() const { return CemeteryKey; }

    uint32 GetKillerNameHash() const { return KillerNameHash; }

    // Blueprint convenience for the mourning read: was this NPC killed by ANYONE the world can name?
    UFUNCTION(BlueprintPure, Category = "Grave")
    bool HasKnownKiller() const { return KillerNameHash != 0; }

    // True if the grave can currently be raised by necromancy: config allows it and it has not already been raised.
    // The raisable substrate mirrors the corpse so the necromancy ability can consume a grave identically.
    UFUNCTION(BlueprintPure, Category = "Grave")
    bool CanBeRaised() const;

    // SERVER: consume the raise substrate (necromancy) — latches bAlreadyRaised so a grave can't be raised twice.
    UFUNCTION(BlueprintCallable, Category = "Grave")
    void ServerMarkRaised();

    UFUNCTION(BlueprintPure, Category = "Grave")
    bool IsAlreadyRaised() const { return bAlreadyRaised; }

    virtual void OnPrimaryInteract_Implementation(AActor *Interactor) override;
    virtual void OnSecondaryInteract_Implementation(AActor *Interactor) override;
    virtual USceneComponent *GetWidgetAttachmentComponent_Implementation() const override;
    virtual bool GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const override;
    virtual void OnFocused_Implementation(AActor *Interactor) override;
    virtual void OnUnfocused_Implementation(AActor *Interactor) override;

    bool IsActorInRange(const AActor *Actor) const;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grave")
    USceneComponent *SceneRoot;

    // Query-only collision proxy so the grave is detected by the interaction sweep (ECC_Visibility) even before a BP
    // assigns a visual mesh — keeps the raw C++ grave interactable unauthored. Mirrors the corpse.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grave")
    USphereComponent *InteractionBounds;

    // Placeholder headstone visual. A BP grave swaps this per faction/role for authored art.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grave")
    UStaticMeshComponent *GraveMesh;

    // Interaction prompt data (same shape as the corpse / storage container).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    TObjectPtr<const UCommonGenericInputActionDataTable> InputActionDataTable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FName PrimaryInteractionName = FName("Read");

    // Squared distance within which a player may read the grave. <= 0 disables the range gate. Mirrors the corpse.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grave")
    float ServerUseRangeSq = 250000.0f; // 500cm

    // Fired on the local interacting client so the BP can present the epitaph (bind GetEpitaph()/GetDeceasedName()).
    // Editor handoff, same shape as the corpse's OnCorpseOpened.
    UFUNCTION(BlueprintImplementableEvent, Category = "Grave")
    void OnEpitaphRead(APlayerController *Interactor);

    // Cosmetic hook: identity was (re)stamped — a BP updates the headstone visual/inscription here (client + server).
    UFUNCTION(BlueprintImplementableEvent, Category = "Grave")
    void OnGraveIdentityChanged();


    UPROPERTY(ReplicatedUsing = OnRep_Identity, VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Grave", meta = (AllowPrivateAccess = "true"))
    FText Epitaph;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Grave", meta = (AllowPrivateAccess = "true"))
    FText DisplayName;

    UPROPERTY(Replicated, SaveGame)
    uint32 SourceNameHash = 0;

    UPROPERTY(Replicated, SaveGame, BlueprintReadOnly, Category = "Grave", meta = (AllowPrivateAccess = "true"))
    FGameplayTag Faction;

    UPROPERTY(Replicated, SaveGame, BlueprintReadOnly, Category = "Grave", meta = (AllowPrivateAccess = "true"))
    FGameplayTag RoleTag;

    UPROPERTY(Replicated, SaveGame, BlueprintReadOnly, Category = "Grave", meta = (AllowPrivateAccess = "true"))
    int32 SourceTier = 0;

    UPROPERTY(Replicated, SaveGame)
    float DeathTime = 0.0f;

    UPROPERTY(Replicated, SaveGame)
    uint32 KillerNameHash = 0;

    UPROPERTY(Replicated, SaveGame)
    FName CemeteryKey;

    UPROPERTY(Replicated, SaveGame, BlueprintReadOnly, Category = "Grave", meta = (AllowPrivateAccess = "true"))
    bool bRaisable = true;

    UPROPERTY(Replicated, SaveGame, BlueprintReadOnly, Category = "Grave", meta = (AllowPrivateAccess = "true"))
    bool bAlreadyRaised = false;

    UFUNCTION()
    void OnRep_Identity();

    static class AController *ResolveController(AActor *Interactor);
};
