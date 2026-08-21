#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "UObject/SoftObjectPtr.h"
#include "Narrative/MythicStoryCondition.h"
#include "MythicUnlockComponent.generated.h"

class UMythicUnlockRuleSet;
class UMythicUnlockRule;
class UMythicNarrativeStateComponent;
class UMythicAchievementComponent;
class APlayerController;

UCLASS(ClassGroup = (Mythic), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicUnlockComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicUnlockComponent();

    // Pure: is this title/cosmetic tag granted (hierarchical)?
    UFUNCTION(BlueprintPure, Category = "Progression|Unlocks")
    bool HasUnlockTag(FGameplayTag Tag) const { return GrantedUnlockTags.HasTag(Tag); }

    const FGameplayTagContainer &GetGrantedUnlockTags() const { return GrantedUnlockTags; }

    const TArray<FGameplayTag> &GetAppliedUnlockRules() const { return AppliedUnlockRules; }

    // SERVER: select the active title (must already be in GrantedUnlockTags). An invalid tag CLEARS it. Replicated to ALL
    // peers (co-op nameplates), unlike the owner-only granted set. No-op off authority / for an unowned title.
    UFUNCTION(BlueprintCallable, Category = "Progression|Unlocks")
    void ServerSetActiveTitle(FGameplayTag TitleTag);

    // SERVER (Wave M4 — the crafting known-set): learn a recipe by granting its Itemization.Schematic.* tag. The tag is
    //   (a) recorded in GrantedUnlockTags (persisted with the character save; owner-replicated for the codex UI), and
    //   (b) asserted as a REPLICATED loose gameplay tag on the owner's schematics ASC, which is exactly what every
    //       conversion recipe's InstigatorTagQuery gate (server + client advisory eligibility) matches against.
    // IDEMPOTENT: returns true only on a genuine first learn (re-learns re-assert the ASC tag and return false — so
    // discovery side-effects like codex pages / Stat.Cooking.RecipesDiscovered fire exactly once). Callers: the
    // UnlockRecipe rule effect (schematic items / achievements) and cooking's experiment discovery.
    UFUNCTION(BlueprintCallable, Category = "Progression|Unlocks")
    bool ServerLearnRecipe(FGameplayTag SchematicTag);

    static bool ShouldGrantLearn(const FGameplayTagContainer &AlreadyGranted, FGameplayTag SchematicTag) {
        return SchematicTag.IsValid() && !AlreadyGranted.HasTagExact(SchematicTag);
    }

    UFUNCTION(BlueprintPure, Category = "Progression|Unlocks")
    FGameplayTag GetActiveTitleTag() const { return ActiveTitle; }

    void RestoreUnlockState(const FGameplayTagContainer &SavedGranted, const TArray<FGameplayTag> &SavedAppliedRules,
                            FGameplayTag SavedActiveTitle);

    void SetRestoring(bool bInRestoring) { bIsRestoring = bInRestoring; }

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    // Component-level override for the rule set; if null, resolved from DeveloperSettings in BeginPlay.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression|Unlocks")
    TSoftObjectPtr<UMythicUnlockRuleSet> UnlockRuleSet;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(Replicated)
    FGameplayTagContainer GrantedUnlockTags;

    UPROPERTY(Replicated)
    FGameplayTag ActiveTitle;

    UPROPERTY()
    TArray<FGameplayTag> AppliedUnlockRules;

    bool bIsRestoring = false;

private:
    UMythicNarrativeStateComponent *ResolveNarrative() const;
    UMythicAchievementComponent *ResolveAchievements() const;
    APlayerController *ResolvePC() const;
    UMythicUnlockRuleSet *ResolveRuleSet();

    void BuildRuleIndex();

    FGameplayTagContainer GatherOwnedTags() const;

    void EnsureSchematicTagOnASC(const FGameplayTag &SchematicTag) const;

    void EnqueueAndDrain(const FGameplayTag &Tag);
    bool EvaluatePass();
    void ApplyRule(const UMythicUnlockRule &Rule);

    UFUNCTION()
    void HandleStoryTagEarned(FGameplayTag Tag);
    UFUNCTION()
    void HandleAchievementUnlocked(FGameplayTag AchievementTag);

    UPROPERTY(Transient)
    TObjectPtr<UMythicUnlockRuleSet> ResolvedRuleSet = nullptr;

    TArray<FMythicStoryCondition> RulePreconditions;
    TMap<FGameplayTag, int32> RuleIndexById;
    TSet<int32> AppliedRuleIndices;
    bool bIndexBuilt = false;

    TArray<FGameplayTag> PendingTriggers;
    bool bEvaluating = false;
};
