
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Itemization/MythicDataAsset.h"
#include "MythicGlossaryEntry.generated.h"

class UTexture2D;

UCLASS(BlueprintType)
class MYTHIC_API UMythicGlossaryEntry : public UMythicDataAsset {
    GENERATED_BODY()

public:
    // The term's stable identity — the tag ServerDiscoverTerm stamps and the registry indexes by.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity", meta = (Categories = "Codex.Term"))
    FGameplayTag TermKey;

    // Player-facing term name (e.g. "Starving").
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FText Term;

    // The definition/explanation shown on the glossary page (and the discovery toast body).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity", meta = (MultiLine = true))
    FText Definition;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Art")
    TSoftObjectPtr<UTexture2D> Icon;

    // Grouping tag for glossary UI sections (e.g. Codex.Term.Status vs Codex.Term.Activity).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity", meta = (Categories = "Codex.Term"))
    FGameplayTag Category;
};
