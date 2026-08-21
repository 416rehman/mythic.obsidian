
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "MythicTitleTypes.generated.h"

class APlayerState;

USTRUCT(BlueprintType)
struct FMythicTitleDef {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title")
    FGameplayTag TitleTag;

    // The rendered title text (e.g. "the Exalted").
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title")
    FText Display;

    // Optional tooltip/codex flavor line.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Title")
    FText Flavor;
};

UCLASS(BlueprintType)
class MYTHIC_API UMythicTitleRegistry : public UDataAsset {
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Titles")
    TArray<FMythicTitleDef> Titles;

    static const FMythicTitleDef *Find(TConstArrayView<FMythicTitleDef> Defs, const FGameplayTag &TitleTag) {
        if (!TitleTag.IsValid()) {
            return nullptr;
        }
        for (const FMythicTitleDef &Def : Defs) {
            if (Def.TitleTag == TitleTag) {
                return &Def;
            }
        }
        return nullptr;
    }

    const FMythicTitleDef *Find(const FGameplayTag &TitleTag) const { return Find(Titles, TitleTag); }

    // Display text for a title tag: the registry entry's Display, else the tag's leaf name as a readable fallback
    // (a granted-but-unregistered title still shows SOMETHING on the nameplate), else empty for an invalid tag.
    UFUNCTION(BlueprintPure, Category = "Progression|Titles")
    static FText GetTitleDisplayText(const UMythicTitleRegistry *Registry, FGameplayTag TitleTag);

    // THE nameplate binding: the resolved display text of PlayerState's ACTIVE title (UMythicUnlockComponent::ActiveTitle
    // replicates to ALL peers, so this works for any player's nameplate on any client). Registry resolves from
    // DeveloperSettings. Empty text when no title is active (hide the nameplate line).
    UFUNCTION(BlueprintPure, Category = "Progression|Titles")
    static FText GetActiveTitleText(const APlayerState *PlayerState);
};
