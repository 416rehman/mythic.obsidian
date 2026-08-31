#include "UI/MythicRichTextIconLibrary.h"

FName UMythicRichTextIconLibrary::GetSemanticIconRowName(
    const EMythicSemanticIcon Icon) {
    switch (Icon) {
    case EMythicSemanticIcon::AttentionFocus:
        return TEXT("NP_Attention_Focus");
    case EMythicSemanticIcon::AttentionInteraction:
        return TEXT("NP_Attention_Interaction");
    case EMythicSemanticIcon::AttentionSoftTarget:
        return TEXT("NP_Attention_SoftTarget");
    case EMythicSemanticIcon::AttentionHardTarget:
        return TEXT("NP_Attention_HardTarget");
    case EMythicSemanticIcon::AttentionLockedTarget:
        return TEXT("NP_Attention_LockedTarget");
    case EMythicSemanticIcon::CueTalk:
        return TEXT("NP_Cue_Talk");
    case EMythicSemanticIcon::CueQuestOffer:
        return TEXT("NP_Cue_QuestOffer");
    case EMythicSemanticIcon::CueQuestTurnIn:
        return TEXT("NP_Cue_QuestTurnIn");
    case EMythicSemanticIcon::CueService:
        return TEXT("NP_Cue_Service");
    case EMythicSemanticIcon::CueFaction:
        return TEXT("NP_Cue_Faction");
    case EMythicSemanticIcon::CueRole:
        return TEXT("NP_Cue_Role");
    case EMythicSemanticIcon::StateSurrender:
        return TEXT("NP_State_Surrender");
    case EMythicSemanticIcon::StateFleeing:
        return TEXT("NP_State_Fleeing");
    case EMythicSemanticIcon::StateAttacking:
        return TEXT("NP_State_Attacking");
    case EMythicSemanticIcon::StateDying:
        return TEXT("NP_State_Dying");
    case EMythicSemanticIcon::StateDowned:
        return TEXT("NP_State_Downed");
    case EMythicSemanticIcon::StateDead:
        return TEXT("NP_State_Dead");
    case EMythicSemanticIcon::RankSuperior:
        return TEXT("NP_Rank_Superior");
    case EMythicSemanticIcon::RankElite:
        return TEXT("NP_Rank_Elite");
    case EMythicSemanticIcon::RankChampion:
        return TEXT("NP_Rank_Champion");
    case EMythicSemanticIcon::RankBoss:
        return TEXT("NP_Rank_Boss");
    case EMythicSemanticIcon::RankWorldBoss:
        return TEXT("NP_Rank_WorldBoss");
    case EMythicSemanticIcon::ThreatRisky:
        return TEXT("NP_Threat_Risky");
    case EMythicSemanticIcon::ThreatDeadly:
        return TEXT("NP_Threat_Deadly");
    case EMythicSemanticIcon::ThreatOverwhelming:
        return TEXT("NP_Threat_Overwhelming");
    case EMythicSemanticIcon::RelationHostile:
        return TEXT("NP_Relation_Hostile");
    case EMythicSemanticIcon::None:
    default:
        return NAME_None;
    }
}

FText UMythicRichTextIconLibrary::MakeSemanticIconMarkup(
    const EMythicSemanticIcon Icon) {
    const FName RowName = GetSemanticIconRowName(Icon);
    return RowName.IsNone()
        ? FText::GetEmpty()
        : FText::FromString(FString::Printf(
              TEXT("<img id=\"%s\"/>"), *RowName.ToString()));
}
