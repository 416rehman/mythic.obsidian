
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MythicNarrativeImportSubsystem.generated.h"

struct IConsoleCommand;
class UObjectiveDefinition;
class UMythicQuestDefinition;
class UMythicStorylineDefinition;
class UMythicDialogueGraph;
struct FMythicDialogueGraphSpec;
struct FMythicStorylineSpec;
struct FMythicQuestSpec;
struct FMythicTaskSpec;
struct FMythicStoryConditionSpec;
struct FMythicRewardsSpec;
struct FGameplayTag;
struct FGameplayTagContainer;
struct FRewardsToGive;
struct FMythicStoryCondition;

UCLASS()
class MYTHIC_API UMythicNarrativeImportSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;

    /** Clear all built definitions and re-scan + rebuild from the story content dir. Console: "Mythic.ReloadNarrative". */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Import")
    void ReloadNarrative();

    /** Resolve an authored TASK by its json id. Returns null if unknown. */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Import")
    UObjectiveDefinition *GetTaskById(const FString &Id) const;

    /** Resolve an authored QUEST by its json id. Returns null if unknown. */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Import")
    UMythicQuestDefinition *GetQuestById(const FString &Id) const;

    /** Resolve an authored STORYLINE by its json id. Returns null if unknown. */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Import")
    UMythicStorylineDefinition *GetStorylineById(const FString &Id) const;

    /** Every built storyline (arc), for a quest-giver / tracker to enumerate available content. */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Import")
    TArray<UMythicStorylineDefinition *> GetAllStorylines() const;

    /** Resolve an authored DIALOGUE GRAPH by its json id. Returns null if unknown. */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Import")
    UMythicDialogueGraph *GetDialogueGraphById(const FString &Id) const;

    /**
     * Resolve which dialogue graph an NPC speaks with, by identity-key priority:
     *   NAMED (graph NpcTag == the NPC's QuestNpcTag) → ROLE+FACTION exact pair → ROLE-only → FACTION-only → null.
     * A graph authored with a valid NpcTag binds to that named NPC ONLY (it never leaks into the generic tiers).
     * Ties within a tier resolve to the first graph in file-scan order (deterministic). Invalid keys skip their tiers.
     */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Import")
    UMythicDialogueGraph *ResolveGraphForNpc(FGameplayTag NpcTag, FGameplayTag Role, FGameplayTag Faction) const;

    bool ImportDocument(const FString &JsonText, const FString &SourceForLog, TArray<FMythicStorylineSpec> &OutStorylines);

    UMythicDialogueGraph *BuildDialogueGraph(const FMythicDialogueGraphSpec &Spec);

    static FString GetStoryContentDir();

    static FGameplayTag ToTag(const FString &TagString);
    static FGameplayTagContainer ToTagContainer(const TArray<FString> &TagStrings);
    static FMythicStoryCondition ToCondition(const FMythicStoryConditionSpec &Spec);

    void BuildFromSpecs(const TArray<FMythicStorylineSpec> &Storylines);

protected:
    FRewardsToGive BuildRewards(const FMythicRewardsSpec &Spec, UObject *Owner);

    void ClearBuiltDefinitions();

private:
    UPROPERTY(Transient)
    TMap<FString, TObjectPtr<UObjectiveDefinition>> TasksById;

    UPROPERTY(Transient)
    TMap<FString, TObjectPtr<UMythicQuestDefinition>> QuestsById;

    UPROPERTY(Transient)
    TMap<FString, TObjectPtr<UMythicStorylineDefinition>> StorylinesById;

    UPROPERTY(Transient)
    TMap<FString, TObjectPtr<UMythicDialogueGraph>> GraphsById;

    TMap<FGameplayTag, FString> GraphIdByNpcTag;

    TArray<FString> OrderedGraphIds;

    IConsoleCommand *ReloadCommand = nullptr;
};
