// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MythicCharacter.h"
#include "Interaction/IMythicInteractable.h"
#include "Interaction/ContextActions/MythicContextActionProvider.h"
#include "World/Entity/IMythicPresentableEntity.h"
#include "MythicCharacter_Player.generated.h"

class UCommonGenericInputActionDataTable;
class UMythicContextActionDefinition;
class UMythicEntityPresentationComponent;

UCLASS(Blueprintable)
class AMythicCharacter_Player : public AMythicCharacter,
                                public IMythicInteractable,
                                public IMythicPresentableEntity,
                                public IMythicContextActionProvider {
    GENERATED_BODY()

    UPROPERTY(Replicated)
    class UAbilitySystemComponent *ASC_Ref;

    // Consumes death/health events, runs regen, and drives respawn for this pawn (owns no attributes; the
    // player's ASC + attribute sets live on the PlayerState).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mythic", meta = (AllowPrivateAccess = true))
    class UMythicLifeComponent *LifeComponent;

    /** Shared replicated public-presentation adapter; it owns no widgets and never replicates the canonical player ID. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Presentation",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UMythicEntityPresentationComponent> EntityPresentationComponent;

public:
    AMythicCharacter_Player();

    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;
    virtual void InitializeASC() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void PossessedBy(AController *NewController) override;
    virtual void OnRep_PlayerState() override;

    virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> &OutLifetimeProps) const override;

    virtual void OnPrimaryInteract_Implementation(AActor *Interactor) override;
    virtual void OnSecondaryInteract_Implementation(AActor *Interactor) override;
    virtual USceneComponent *GetWidgetAttachmentComponent_Implementation() const override;
    virtual bool GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const override;
    virtual void OnFocused_Implementation(AActor *Interactor) override;
    virtual void OnUnfocused_Implementation(AActor *Interactor) override;

    /** Returns this player's one shared presentation adapter for attention, targeting, actions, and nameplates. */
    virtual UMythicEntityPresentationComponent *
        GetEntityPresentationComponent_Implementation() const override;

    virtual void GatherContextActions_Implementation(
        AController *RequestingController, AActor *Subject,
        TArray<FMythicContextActionOffer> &OutOffers) const override;
    virtual bool CanExecuteContextAction_Implementation(
        AController *RequestingController, AActor *Subject,
        FGameplayTag ActionTag, int64 ObservedOfferRevision,
        FGameplayTag &OutFailureReason) const override;
    virtual bool ExecuteContextAction_Implementation(
        AController *RequestingController, AActor *Subject,
        FGameplayTag ActionTag, int64 ObservedOfferRevision,
        FGameplayTag &OutFailureReason) override;

protected:
    // Interaction prompt data for the "Revive" verb (matches the container/toggleable pattern).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    TObjectPtr<const UCommonGenericInputActionDataTable> InputActionDataTable;

    /** Localized interaction-row key used by the legacy revive prompt; the contextual action uses its definition label. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FName ReviveInteractionName = FName("Revive");

    /** Canonical authored Revive action projected only while this player is downed and another valid player can help. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
              Category = "World Presentation|Context Actions")
    TObjectPtr<UMythicContextActionDefinition> ReviveContextActionDefinition;

private:
    void BindPersistentEntityIdentity();
    void UnbindPersistentEntityIdentity();
    void TryActivateEntityPresentation();
    void AdvanceContextActionRevision();
    bool ValidateReviveContextAction(
        AController *RequestingController, AActor *Subject,
        FGameplayTag ActionTag, int64 ObservedOfferRevision,
        FGameplayTag &OutFailureReason) const;

    UFUNCTION()
    void HandlePlayerDowned(AActor *DownedActor);

    UFUNCTION()
    void HandlePlayerRevived(AActor *RevivedActor);

    UFUNCTION()
    void HandlePlayerDeath(AActor *DeadActor);

    void HandlePersistentEntityIdentityReady(
        const struct FMythicEntityId &EntityId);

    TWeakObjectPtr<class AMythicPlayerState> BoundIdentityPlayerState;
    FDelegateHandle PersistentEntityIdentityReadyHandle;
    uint32 ContextActionRevision = 1;
    bool bBoundLifePresentation = false;
};
