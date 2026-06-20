// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MythicCharacter.h"
#include "MythicCharacter_Player.generated.h"

UCLASS(Blueprintable)
class AMythicCharacter_Player : public AMythicCharacter {
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
};
