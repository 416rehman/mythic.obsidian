#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "World/Entity/MythicEntityId.h"

#include "MythicEntityViewerKnowledgeTypes.generated.h"

/** Coarse, player-learned relationship language; exact simulation scores remain private. */
UENUM(BlueprintType)
enum class EMythicKnownRelationshipBand : uint8 {
    Unknown UMETA(DisplayName = "Unknown"),
    Hostile UMETA(DisplayName = "Hostile"),
    Wary UMETA(DisplayName = "Wary"),
    Neutral UMETA(DisplayName = "Neutral"),
    Familiar UMETA(DisplayName = "Familiar"),
    Friendly UMETA(DisplayName = "Friendly"),
    Trusted UMETA(DisplayName = "Trusted")
};

/** Coarse, player-learned faction-standing language; exact reputation values remain private. */
UENUM(BlueprintType)
enum class EMythicKnownStandingBand : uint8 {
    Unknown UMETA(DisplayName = "Unknown"),
    Hostile UMETA(DisplayName = "Hostile"),
    Unfriendly UMETA(DisplayName = "Unfriendly"),
    Neutral UMETA(DisplayName = "Neutral"),
    Friendly UMETA(DisplayName = "Friendly"),
    Honored UMETA(DisplayName = "Honored")
};

/**
 * Immutable, viewer-safe copy consumed by nameplates and the player Inspect surface.
 *
 * Every field represents knowledge earned by this player. The structure intentionally contains no canonical ID,
 * true faction, hidden role, intent, exact relationship score, private attributes, or raw LivingWorld state.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicEntityKnowledgeView {
    GENERATED_BODY()

    /** True only when authority has bound the queried presentation embodiment to this player's learned identity. */
    UPROPERTY(BlueprintReadOnly, Category = "Entity Knowledge")
    bool bRecognitionGranted = false;

    /** True when the player has learned a localized personal or authored name for this entity. */
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Entity Knowledge")
    bool bNameKnown = false;

    /** Localized learned name; empty while bNameKnown is false and never reconstructed from a hash or object name. */
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Entity Knowledge")
    FText RecognizedName;

    /** True when KnownFactionTag is knowledge the player has legitimately acquired. */
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Entity Knowledge")
    bool bFactionKnown = false;

    /** Player-learned public faction or affiliation; invalid while it is unknown or only suspected. */
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Entity Knowledge")
    FGameplayTag KnownFactionTag;

    /** True when KnownRoleTag is knowledge the player has legitimately acquired. */
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Entity Knowledge")
    bool bRoleKnown = false;

    /** Player-learned role, profession, species, or social function; never a hidden cover-breaking role. */
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Entity Knowledge")
    FGameplayTag KnownRoleTag;

    /** Coarse relationship band learned from prior contact; Unknown conceals the underlying simulation score. */
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Entity Knowledge")
    EMythicKnownRelationshipBand RelationshipBand = EMythicKnownRelationshipBand::Unknown;

    /** Coarse standing band relevant to this entity; Unknown conceals exact faction-reputation values. */
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Entity Knowledge")
    EMythicKnownStandingBand StandingBand = EMythicKnownStandingBand::Unknown;

    /** Bounded Entity.Knowledge.Trait.* facts the player has discovered through play. */
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Entity Knowledge")
    FGameplayTagContainer DiscoveredTraits;

    /** Bounded Entity.Knowledge.History.* facts the player learned or personally witnessed. */
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Entity Knowledge")
    FGameplayTagContainer DiscoveredHistory;

    /** Bounded Entity.Knowledge.Preference.Likes.* facts the player has learned. */
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Entity Knowledge")
    FGameplayTagContainer KnownLikes;

    /** Bounded Entity.Knowledge.Preference.Dislikes.* facts the player has learned. */
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Entity Knowledge")
    FGameplayTagContainer KnownDislikes;

    /** Bounded Entity.Knowledge.Connection.* ties the player has learned through play rather than raw graph access. */
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Entity Knowledge")
    FGameplayTagContainer DiscoveredConnections;

    /** Clears the copy to an unrecognized, knowledge-free state. */
    void Reset() { *this = FMythicEntityKnowledgeView(); }
};

/** Save-backed authority record keyed only by the canonical typed entity identity. */
USTRUCT()
struct MYTHIC_API FMythicEntityLearnedDossier {
    GENERATED_BODY()

    // Canonical identity is intentionally reflected only for SaveGame and native authority code.
    UPROPERTY(SaveGame)
    FMythicEntityId EntityId;

    // The nested DTO contains only learned, player-safe facts; recognition is embodiment-scoped and is cleared on save.
    UPROPERTY(SaveGame)
    FMythicEntityKnowledgeView LearnedKnowledge;

    // Native-only monotonic revision used to refresh owner projections without exposing a private record key.
    UPROPERTY(SaveGame)
    uint32 KnowledgeRevision = 0;

    bool IsValid() const { return EntityId.IsValid(); }
};

/** Pure validation and merge rules shared by authority ingestion, save restore, and automation tests. */
struct MYTHIC_API FMythicEntityKnowledgeRules {
    static constexpr int32 MaxTagsPerKnowledgeFacet = 16;
    static constexpr int32 MaxRecognizedNameCharacters = 128;

    /** Returns a fail-closed copy containing only bounded, category-correct, player-safe presentation fields. */
    static FMythicEntityKnowledgeView Sanitize(const FMythicEntityKnowledgeView &Source);

    /** Additively merges a learned delta into an existing safe snapshot and returns whether presentation changed. */
    static bool MergeLearnedDelta(FMythicEntityKnowledgeView &InOutKnowledge,
                                  const FMythicEntityKnowledgeView &LearnedDelta);

    /** Returns the typed-ID record index or INDEX_NONE without string, hash, or row-name identity conversion. */
    static int32 FindDossierIndex(TConstArrayView<FMythicEntityLearnedDossier> Dossiers,
                                  const FMythicEntityId &EntityId);
};
