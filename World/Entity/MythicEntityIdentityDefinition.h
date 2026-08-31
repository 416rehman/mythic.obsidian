#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "MythicEntityIdentityDefinition.generated.h"

class UTexture2D;

/**
 * Authored public presentation identity for a role/species/archetype, not a persistent record for one simulated
 * person. Procedural people keep their private identity in LivingWorld and use player knowledge for learned names.
 */
UCLASS(BlueprintType, Const)
class MYTHIC_API UMythicEntityIdentityDefinition : public UPrimaryDataAsset {
    GENERATED_BODY()

public:
    /** Canonical Entity.Identity.* presentation identity; it must be unique and reveals no hidden simulation truth. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity", meta = (Categories = "Entity.Identity"))
    FGameplayTag IdentityTag;

    /** Localized public label used only when this archetype's identity is visibly knowable without player knowledge. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FText PublicDisplayName;

    /** Safe Entity.Kind.* classification used for fallback copy and presentation policy. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity", meta = (Categories = "Entity.Kind"))
    FGameplayTag PublicKindTag;

    /** Safe visible role/species/cover tag; leave empty when the current embodiment should read as unknown. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FGameplayTag PublicArchetypeTag;

    /**
     * Faction visibly presented by this cover through uniform, heraldry, or public allegiance. This is presentation
     * truth only; a spy or disguise must reference a cover definition rather than its private true faction.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity",
              meta = (Categories = "Faction"))
    FGameplayTag PresentedFactionTag;

    /** Soft icon used by Focus/Inspect after policy permits it; overhead Whisper never renders this icon. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    TSoftObjectPtr<UTexture2D> Icon;

    /** Whether PublicDisplayName is legible on sight; false requires an owner-only learned-name fact. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    bool bNameVisibleOnSight = false;

    /** Whether an entitled viewer may open a knowledge-backed Inspect page for this type of subject. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    bool bSupportsInspection = true;

    /** Returns the Asset Manager key used to discover all authored public identity definitions. */
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

    /** Stable Primary Asset Type registered in Asset Manager configuration for runtime discovery and validation. */
    static const FPrimaryAssetType PrimaryAssetType;

    /**
     * Converts an authored soft reference into its Asset Manager key without synchronously loading it. Invalid or
     * unassigned references fail closed to an invalid key.
     */
    static FPrimaryAssetId ResolvePrimaryAssetId(
        const TSoftObjectPtr<UMythicEntityIdentityDefinition> &Definition);

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(
        FDataValidationContext &Context) const override;
#endif
};
