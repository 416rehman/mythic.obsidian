
#pragma once

#include "CoreMinimal.h"
#include "World/LivingWorld/Dialogue/MythicDialogueTypes.h"
#include "World/LivingWorld/LivingWorldTypes.h"

class UMythicDialogueDatabase;
struct FMythicPersonalityFragment;
struct FMythicPsychodynamicFragment;

struct FMythicDialogueContext {
    FGameplayTag RoleTag;

    FGameplayTag FactionTag;

    FGameplayTagContainer SituationTags;

    const float* PressureChannels = nullptr;
    int32 PressureChannelCount = 0;

    int32 DominantPressureChannel = -1;

    uint8 RecentEventSeverity = 0;

    bool bIsCompanionCommentary = false;
    float PlayerActionMoralScore = 0.0f;
};

struct FMythicDialogueResult {
    const FMythicDialogueTemplate* Template = nullptr;

    FText ResolvedText;

    bool IsValid() const { return Template != nullptr; }
};

struct FMythicDialogueVariables {
    FString FactionName;
    FString RecentEvent;
    FString PlayerReputationDescriptor;
    FString NPCName;
    FString SettlementName;
    FString SpeakerMood;
    FString TargetName;
};

struct MYTHIC_API FMythicDialogueSelector {
    static FMythicDialogueResult SelectTemplate(
        const UMythicDialogueDatabase* Database,
        const FMythicDialogueContext& Context);

    static FText ResolveVariables(
        const FText& TemplateText,
        const FMythicDialogueVariables& Variables);

    static bool TemplateConstrainsSeverity(uint8 MinSeverity, uint8 MaxSeverity);
};
