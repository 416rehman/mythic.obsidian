#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GAS/Combat/MythicCombatThreatAssessment.h"
#include "World/Entity/MythicEntityPresentationTypes.h"

#include "MythicEntityInspectTypes.generated.h"

class UTexture2D;

/** Fully resolved player-facing row for one viewer-earned entity knowledge fact. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicEntityInspectFactProjection {
    GENERATED_BODY()

    /** Safe learned semantic identity retained for stable row reuse; widgets must render ResolvedLabel, not this tag. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Inspect|Fact",
              meta = (Categories = "Entity.Knowledge"))
    FGameplayTag FactTag;

    /** Localized player-facing fact label resolved from the canonical knowledge-fact definition. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Inspect|Fact")
    FText ResolvedLabel;

    /** Optional localized learned-context explanation; empty produces a compact label-only row. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Inspect|Fact")
    FText ResolvedDescription;

    /** Resident canonical icon; null intentionally uses the section's authored default treatment. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Inspect|Fact")
    TObjectPtr<UTexture2D> Icon;

    /** Native deterministic order copied from the definition; it is not player-facing. */
    int32 PresentationPriority = 0;
};

/**
 * Immutable player-knowledge dossier for one exact current embodiment.
 *
 * Every field is public observation or owner-only learned knowledge. It contains no canonical entity ID, raw social
 * graph, intent, hidden trait, exact relationship score, private faction, AI state, or arbitrary debug payload.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicEntityInspectProjection {
    GENERATED_BODY()

    /** Exact opaque embodiment key; a generation change invalidates this Inspect surface. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Inspect")
    FMythicEntityPresentationInstance Instance;

    /** Viewer-safe learned/public identity. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Inspect|Who")
    FText ResolvedName;

    /** Learned or visibly public role/species; empty means unknown. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Inspect|Who")
    FText ResolvedRole;

    /** Learned or visibly presented faction; empty means unknown. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Inspect|Who")
    FText ResolvedFaction;

    /** Localized coarse learned relationship language; empty means the player has not learned it. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Inspect|Between Us")
    FText ResolvedRelationship;

    /** Localized coarse learned faction-standing language; exact reputation remains private. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Inspect|Between Us")
    FText ResolvedStanding;

    /** Combat-owned viewer-relative warning; Unknown means no current entitlement or assessment. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Inspect|Combat")
    EMythicThreatBand ThreatBand = EMythicThreatBand::Unknown;

    /** Whether authority permits this viewer to know the inspected subject is combat-capable. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Inspect|Combat")
    bool bCombatCapable = false;

    /** Whether authority permits this viewer to know the inspected subject has boss or world-boss rank. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Inspect|Combat")
    bool bBoss = false;

    /** Whether ExactCombatLevel contains a separately permissioned viewer-safe value. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Inspect|Combat")
    bool bShowExactCombatLevel = false;

    /** Permissioned exact combat level; zero while bShowExactCombatLevel is false. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Inspect|Combat", meta = (ClampMin = "0"))
    int32 ExactCombatLevel = 0;

    /** Bounded, definition-resolved traits the player has discovered. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Inspect|Who")
    TArray<FMythicEntityInspectFactProjection> Traits;

    /** Bounded, definition-resolved witnessed or learned history facts. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Inspect|Known History")
    TArray<FMythicEntityInspectFactProjection> History;

    /** Bounded positive preferences the player has learned. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Inspect|Preferences")
    TArray<FMythicEntityInspectFactProjection> Likes;

    /** Bounded negative preferences the player has learned. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Inspect|Preferences")
    TArray<FMythicEntityInspectFactProjection> Dislikes;

    /** Bounded social ties learned through play; this never queries the raw LivingWorld social graph. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Inspect|Known Connections")
    TArray<FMythicEntityInspectFactProjection> Connections;

    /** Returns true only for a current embodiment with a nonempty viewer-safe identity. */
    bool IsValid() const {
        return Instance.IsValid() && !ResolvedName.IsEmpty();
    }
};
