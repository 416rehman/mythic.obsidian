// Mythic Living World — Dialogue Template Types
// Template selection system for NPC dialogue. Zero procedural text generation.
// Designers author templates per role × faction × situation archetype.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "MythicDialogueTypes.generated.h"

/**
 * A single dialogue template authored by designers.
 * The cognitive brain selects the best-matching template based on
 * pressure state, relationship, recent events, and role.
 *
 * Template variables are filled from live data at runtime:
 * {faction_name}, {recent_event}, {player_reputation_descriptor},
 * {npc_name}, {settlement_name}, {scheme_name}
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicDialogueTemplate {
    GENERATED_BODY()

    /** Unique tag identifying this template for cross-referencing */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "Dialogue"))
    FGameplayTag TemplateTag;

    /** NPC role required to use this template (empty = any role) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "NPC.Role"))
    FGameplayTag RequiredRole;

    /** Faction tag required (empty = universal for all factions) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "Faction"))
    FGameplayTag RequiredFaction;

    /** Situational context tags (combat, greeting, trade, crime witness, etc.) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "Dialogue.Situation"))
    FGameplayTagContainer SituationTags;

    /** Moral axis filter — template only valid when the relevant pressure channel is active */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    uint8 MoralAxisFilter = 0xFF; // 0xFF = any axis

    /** Minimum severity required for this template to be eligible */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    uint8 MinSeverity = 0;

    /** Maximum severity allowed (0xFF = no cap) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    uint8 MaxSeverity = 0xFF;

    /**
     * Template text with variable placeholders.
     * Variables: {faction_name}, {recent_event}, {player_reputation_descriptor},
     * {npc_name}, {settlement_name}, {speaker_mood}, {target_name}
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FText DialogueText;

    /** Priority when multiple templates match — higher wins */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "100"))
    int32 Priority = 50;

    /** Is this a companion commentary template? (triggered by player actions) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    bool bIsCompanionCommentary = false;

    /**
     * For companion commentary: moral severity threshold above which this comment fires.
     * The companion comments when the player's action scores above this on their moral surface.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CommentaryMoralThreshold = 0.5f;
};

/**
 * Designer-authored database of dialogue templates.
 * Loaded from the DA_LivingWorldSettings → DialogueDatabase reference.
 * Templates are indexed at load time for fast lookup.
 */
UCLASS(BlueprintType)
class MYTHIC_API UMythicDialogueDatabase : public UDataAsset {
    GENERATED_BODY()

public:
    /** All dialogue templates in this database */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
    TArray<FMythicDialogueTemplate> Templates;
};
