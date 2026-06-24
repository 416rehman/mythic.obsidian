// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MythicCharacter.h"
#include "Interaction/IMythicInteractable.h"
#include "MythicCharacter_Player.generated.h"

class UCommonGenericInputActionDataTable;

// A player pawn is interactable ONLY while downed (co-op): a teammate focuses it ("Revive") and interacts to revive
// it. The interactable methods no-op / return false when the pawn is alive, so a healthy player offers no prompt.
UCLASS(Blueprintable)
class AMythicCharacter_Player : public AMythicCharacter, public IMythicInteractable {
    GENERATED_BODY()

    UPROPERTY(Replicated)
    class UAbilitySystemComponent *ASC_Ref;

    // Consumes death/health events, runs regen, and drives respawn for this pawn (owns no attributes; the
    // player's ASC + attribute sets live on the PlayerState).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mythic", meta = (AllowPrivateAccess = true))
    class UMythicLifeComponent *LifeComponent;

public:
    AMythicCharacter_Player();

    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;
    virtual void InitializeASC() override;

    virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> &OutLifetimeProps) const override;

    //~ IMythicInteractable — a downed player can be revived by a teammate (the only interaction it offers).
    virtual void OnPrimaryInteract_Implementation(AActor *Interactor) override;
    virtual void OnSecondaryInteract_Implementation(AActor *Interactor) override;
    virtual USceneComponent *GetWidgetAttachmentComponent_Implementation() const override;
    virtual bool GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const override;
    virtual void OnFocused_Implementation(AActor *Interactor) override;
    virtual void OnUnfocused_Implementation(AActor *Interactor) override;

protected:
    // Interaction prompt data for the "Revive" verb (matches the container/toggleable pattern).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    TObjectPtr<const UCommonGenericInputActionDataTable> InputActionDataTable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FName ReviveInteractionName = FName("Revive");
};
