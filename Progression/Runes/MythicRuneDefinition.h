
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
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

    // Drawn by a RichTextBlock, so style markup in the authored string is live.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rune", meta = (MultiLine = true))
    FText Description;

    // Passive. Granted while the rune sits in a slot, cleared when it leaves.
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

    bool HasPayload() const { return Ability != nullptr; }
};
