#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "MythicEntityKnowledgeFactDefinition.generated.h"

class UTexture2D;

/** Player-facing Inspect section that owns one learned semantic fact. */
UENUM(BlueprintType)
enum class EMythicEntityKnowledgeFactSection : uint8 {
    /** Discovered personality, reputation, or behavioral trait. */
    Trait,

    /** Witnessed or learned history beat. */
    History,

    /** Learned positive preference. */
    Like,

    /** Learned negative preference. */
    Dislike,

    /** Learned social tie or connection. */
    Connection,
};

/**
 * Canonical localized presentation for one viewer-earned entity knowledge fact.
 *
 * The learned dossier stores only FactTag. Inspect resolves this Primary Data Asset locally, preventing replicated
 * strings and tag-name parsing while keeping simulation truth separate from what the player is entitled to know.
 */
UCLASS(BlueprintType, Const)
class MYTHIC_API UMythicEntityKnowledgeFactDefinition : public UPrimaryDataAsset {
    GENERATED_BODY()

public:
    /** Stable Primary Asset type registered with Asset Manager for global validation and asynchronous lookup. */
    static const FPrimaryAssetType PrimaryAssetType;

    /** Canonical Entity.Knowledge.* identity stored in learned dossiers; the section must match its tag category. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Knowledge Fact|Identity",
              meta = (Categories = "Entity.Knowledge"))
    FGameplayTag FactTag;

    /** Inspect section used for deterministic grouping and density limits. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Knowledge Fact|Presentation")
    EMythicEntityKnowledgeFactSection Section =
        EMythicEntityKnowledgeFactSection::Trait;

    /** Localized short label rendered in Inspect; it must never be reconstructed from FactTag. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Knowledge Fact|Presentation")
    FText DisplayName;

    /** Optional localized learned-context explanation; empty produces a compact label-only row. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Knowledge Fact|Presentation")
    FText Description;

    /** Optional soft icon loaded by the Inspect surface; null uses the section's authored visual treatment. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Knowledge Fact|Presentation")
    TSoftObjectPtr<UTexture2D> Icon;

    /** Relative order inside one Inspect section; larger values appear first, then FactTag provides a stable tie-break. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Knowledge Fact|Presentation",
              meta = (ClampMin = "-1000", ClampMax = "1000"))
    int32 PresentationPriority = 0;

    /** Returns the tag-keyed Asset Manager identity; invalid/root tags make the definition undiscoverable. */
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext &Context) const override;
#endif
};

