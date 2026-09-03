
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Itemization/Inventory/Fragments/FragmentTypes.h"
#include "UObject/SoftObjectPtr.h"
#include "MythicRuneDefinition.generated.h"

class UTexture2D;

UCLASS(BlueprintType)
class MYTHIC_API UMythicRuneDefinition : public UPrimaryDataAsset {
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rune")
    FText Name;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rune")
    TSoftObjectPtr<UTexture2D> Icon;

    // Drawn on the HUD badge. Icon stands in while this is unset.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rune")
    TSoftObjectPtr<UTexture2D> HudIcon;

    // Drawn by a RichTextBlock, so style markup in the authored string is live. "<#Rune.Param.X>" draws the owner's
    // roll for that parameter against its range.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rune", meta = (MultiLine = true))
    FText Description;

    // The numbers this rune rolls, keyed Rune.Param.<Rune>.<Name>. Each owner rolls them once, at the rune's first
    // socket, and keeps them; the ability reads them through UMythicRuneComponent::GetRolledRuneValue.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rune", meta = (Categories = "Rune.Param"))
    TMap<FGameplayTag, FRollDefinition> Parameters;

    // Passive. Granted while the rune sits in a slot, cleared when it leaves. Must be a UMythicGA_Rune.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rune")
    TSubclassOf<UGameplayAbility> Ability;

    // The deed that earns this rune. INVALID = no deed; the rune is available from the first hour.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rune")
    FGameplayTag RequiredTag;

    // Shown while the rune is locked: how RequiredTag is earned.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rune", meta = (MultiLine = true))
    FText Hint;

    // Rune.Category.* — the picker colour-codes and groups from these.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rune")
    FGameplayTagContainer CategoryTags;

    // The middle of Parameter's range, rounded for a whole-number roll; Fallback for a tag this rune does not roll.
    UFUNCTION(BlueprintPure, Category = "Rune")
    float GetParameterMidpoint(FGameplayTag Parameter, float Fallback) const;

    const TSoftObjectPtr<UTexture2D> &GetHudIconOrIcon() const { return HudIcon.IsNull() ? Icon : HudIcon; }

    // Only a UMythicGA_Rune carries the seams a socketed rune listens on, so anything else is no payload at all.
    bool HasPayload() const;
};
