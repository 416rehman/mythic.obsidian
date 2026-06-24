// Mythic Living World — Dialogue Selector
// Selects the best-matching dialogue template for an NPC based on
// brain state, pressure, relationship, and recent events.

#pragma once

#include "CoreMinimal.h"
#include "World/LivingWorld/Dialogue/MythicDialogueTypes.h"
#include "World/LivingWorld/LivingWorldTypes.h"

class UMythicDialogueDatabase;
struct FMythicPersonalityFragment;
struct FMythicPsychodynamicFragment;

/**
 * Input context for dialogue template selection.
 * Populated by the cognitive brain or party system at the moment of dialogue.
 */
struct FMythicDialogueContext {
    /** NPC's role tag */
    FGameplayTag RoleTag;

    /** NPC's faction tag */
    FGameplayTag FactionTag;

    /** Current situation tags (e.g., greeting, trade, combat, crime) */
    FGameplayTagContainer SituationTags;

    /** NPC's pressure channels (indexed by EMythicPressureChannel) */
    const float* PressureChannels = nullptr;
    int32 PressureChannelCount = 0;

    /** Dominant pressure channel index (-1 = no pressure) */
    int32 DominantPressureChannel = -1;

    /** Moral severity of the most recent event involving this NPC */
    uint8 RecentEventSeverity = 0;

    /** Is this a companion commenting on a player action? */
    bool bIsCompanionCommentary = false;
    float PlayerActionMoralScore = 0.0f;
};

/**
 * Result of dialogue template selection.
 */
struct FMythicDialogueResult {
    /** The selected template (nullptr if no match) */
    const FMythicDialogueTemplate* Template = nullptr;

    /** Resolved text with variables filled from live data */
    FText ResolvedText;

    /** Was a valid template found? */
    bool IsValid() const { return Template != nullptr; }
};

/**
 * Variable resolution context — live data used to fill template placeholders.
 */
struct FMythicDialogueVariables {
    FString FactionName;
    FString RecentEvent;
    FString PlayerReputationDescriptor;
    FString NPCName;
    FString SettlementName;
    FString SpeakerMood;
    FString TargetName;
};

/**
 * Static utility class for dialogue template selection.
 * No UObject backing needed — pure function calls for performance.
 *
 * Performance: O(N) scan of templates in the database per selection.
 * Database is typically <500 templates, so this is negligible.
 * Cache-friendly: templates are contiguous TArray.
 */
struct MYTHIC_API FMythicDialogueSelector {

    /**
     * Select the best-matching dialogue template from the database.
     * Scoring: exact role match (+20), exact faction (+15), situation tag overlap (+10 per),
     * severity range match (+5), priority value. Highest total score wins.
     *
     * @param Database    Loaded dialogue template database
     * @param Context     Current NPC brain state / situation
     * @return Selected template (may be invalid if no match)
     */
    static FMythicDialogueResult SelectTemplate(
        const UMythicDialogueDatabase* Database,
        const FMythicDialogueContext& Context);

    /**
     * Fill template variables with live data.
     * Replaces {faction_name}, {recent_event}, {player_reputation_descriptor},
     * {npc_name}, {settlement_name}, {speaker_mood}, {target_name}.
     *
     * @param TemplateText  Raw template text with {variables}
     * @param Variables     Live data to fill
     * @return Resolved text with all variables replaced
     */
    static FText ResolveVariables(
        const FText& TemplateText,
        const FMythicDialogueVariables& Variables);

    /**
     * True if a template constrains the severity of events it applies to — i.e. it tightened EITHER bound from the
     * defaults (MinSeverity 0 / MaxSeverity 0xFF = "any severity"). SelectTemplate hard-filters an out-of-range event
     * only when this is true. (Fix: the filter previously keyed only on MinSeverity > 0, so a template that lowered
     * MaxSeverity but left MinSeverity at its default 0 had its upper bound silently ignored and would match
     * higher-severity events.) Pure + static so the rule is unit-testable.
     */
    static bool TemplateConstrainsSeverity(uint8 MinSeverity, uint8 MaxSeverity);
};
