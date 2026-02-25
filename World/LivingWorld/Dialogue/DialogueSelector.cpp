// Mythic Living World — Dialogue Selector Implementation

#include "World/LivingWorld/Dialogue/DialogueSelector.h"
#include "World/LivingWorld/Dialogue/MythicDialogueTypes.h"

FMythicDialogueResult FMythicDialogueSelector::SelectTemplate(
    const UMythicDialogueDatabase* Database,
    const FMythicDialogueContext& Context)
{
    FMythicDialogueResult Result;
    if (!Database || Database->Templates.Num() == 0) {
        return Result;
    }

    int32 BestScore = -1;
    const FMythicDialogueTemplate* BestTemplate = nullptr;

    for (const FMythicDialogueTemplate& Template : Database->Templates) {
        int32 Score = 0;

        // ─── Companion commentary filter ───
        if (Context.bIsCompanionCommentary != Template.bIsCompanionCommentary) {
            continue;
        }
        if (Template.bIsCompanionCommentary) {
            // Companion commentary: only fire when player action exceeds moral threshold
            if (Context.PlayerActionMoralScore < Template.CommentaryMoralThreshold) {
                continue;
            }
            Score += 10; // Bonus for commentary match
        }

        // ─── Role match ───
        if (Template.RequiredRole.IsValid()) {
            if (Context.RoleTag.MatchesTag(Template.RequiredRole)) {
                Score += 20; // Exact role match bonus
            } else {
                continue; // Hard filter: role mismatch = skip
            }
        }

        // ─── Faction match ───
        if (Template.RequiredFaction.IsValid()) {
            if (Context.FactionTag.MatchesTag(Template.RequiredFaction)) {
                Score += 15; // Faction match bonus
            } else {
                continue; // Hard filter: faction mismatch = skip
            }
        }

        // ─── Situation tag overlap ───
        if (Template.SituationTags.Num() > 0) {
            int32 Overlap = 0;
            for (const FGameplayTag& Tag : Template.SituationTags) {
                if (Context.SituationTags.HasTag(Tag)) {
                    ++Overlap;
                }
            }
            if (Overlap == 0) {
                continue; // No situation match at all = skip
            }
            Score += Overlap * 10; // +10 per matching situation tag
        }

        // ─── Severity range check ───
        if (Context.RecentEventSeverity >= Template.MinSeverity
            && Context.RecentEventSeverity <= Template.MaxSeverity) {
            Score += 5;
        } else if (Template.MinSeverity > 0) {
            continue; // Severity out of range and template requires specific range
        }

        // ─── Moral axis filter ───
        if (Template.MoralAxisFilter != 0xFF && Context.DominantPressureChannel >= 0) {
            if ((Template.MoralAxisFilter & (1 << Context.DominantPressureChannel)) != 0) {
                Score += 8; // Pressure channel matches axis filter
            }
        }

        // ─── Priority scoring ───
        Score += Template.Priority;

        // ─── Best candidate ───
        if (Score > BestScore) {
            BestScore = Score;
            BestTemplate = &Template;
        }
    }

    if (BestTemplate) {
        Result.Template = BestTemplate;
        Result.ResolvedText = BestTemplate->DialogueText; // Variables resolved in caller
    }

    return Result;
}

FText FMythicDialogueSelector::ResolveVariables(
    const FText& TemplateText,
    const FMythicDialogueVariables& Variables)
{
    FString Resolved = TemplateText.ToString();

    Resolved.ReplaceInline(TEXT("{faction_name}"), *Variables.FactionName);
    Resolved.ReplaceInline(TEXT("{recent_event}"), *Variables.RecentEvent);
    Resolved.ReplaceInline(TEXT("{player_reputation_descriptor}"), *Variables.PlayerReputationDescriptor);
    Resolved.ReplaceInline(TEXT("{npc_name}"), *Variables.NPCName);
    Resolved.ReplaceInline(TEXT("{settlement_name}"), *Variables.SettlementName);
    Resolved.ReplaceInline(TEXT("{speaker_mood}"), *Variables.SpeakerMood);
    Resolved.ReplaceInline(TEXT("{target_name}"), *Variables.TargetName);

    return FText::FromString(Resolved);
}
