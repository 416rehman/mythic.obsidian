
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

        if (Context.bIsCompanionCommentary != Template.bIsCompanionCommentary) {
            continue;
        }
        if (Template.bIsCompanionCommentary) {
            if (Context.PlayerActionMoralScore < Template.CommentaryMoralThreshold) {
                continue;
            }
            Score += 10;
        }

        if (Template.RequiredRole.IsValid()) {
            if (Context.RoleTag.MatchesTag(Template.RequiredRole)) {
                Score += 20;
            } else {
                continue;
            }
        }

        if (Template.RequiredFaction.IsValid()) {
            if (Context.FactionTag.MatchesTag(Template.RequiredFaction)) {
                Score += 15;
            } else {
                continue;
            }
        }

        if (Template.SituationTags.Num() > 0) {
            int32 Overlap = 0;
            for (const FGameplayTag& Tag : Template.SituationTags) {
                if (Context.SituationTags.HasTag(Tag)) {
                    ++Overlap;
                }
            }
            if (Overlap == 0) {
                continue;
            }
            Score += Overlap * 10;
        }

        if (Context.RecentEventSeverity >= Template.MinSeverity
            && Context.RecentEventSeverity <= Template.MaxSeverity) {
            Score += 5;
        } else if (TemplateConstrainsSeverity(Template.MinSeverity, Template.MaxSeverity)) {
            continue;
        }

        if (Template.MoralAxisFilter != 0xFF && Context.DominantPressureChannel >= 0) {
            if ((Template.MoralAxisFilter & (1 << Context.DominantPressureChannel)) != 0) {
                Score += 8;
            }
        }

        Score += Template.Priority;

        if (Score > BestScore) {
            BestScore = Score;
            BestTemplate = &Template;
        }
    }

    if (BestTemplate) {
        Result.Template = BestTemplate;
        Result.ResolvedText = BestTemplate->DialogueText;
    }

    return Result;
}

bool FMythicDialogueSelector::TemplateConstrainsSeverity(uint8 MinSeverity, uint8 MaxSeverity) {
    return MinSeverity > 0 || MaxSeverity < 0xFF;
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
