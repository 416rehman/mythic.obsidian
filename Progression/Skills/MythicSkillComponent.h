
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "MythicSkillDefinition.h"
#include "MythicSkillProgressTypes.h"
#include "UObject/SoftObjectPtr.h"
#include "MythicSkillComponent.generated.h"

class UAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMythicOnSkillsChanged);

UCLASS(ClassGroup = (Mythic), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicSkillComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicSkillComponent();

    // How many skills this character can ever have bound at once. Slot one is free; the rest arrive as GrantSkillSlot unlocks.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression|Skills", meta = (ClampMin = "1", ClampMax = "8"))
    int32 MaxSlots = 2;

    // The key each slot answers to, index-matched to the slots. Adding a slot is a row here, not a code change.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression|Skills")
    TArray<FGameplayTag> SlotInputTags;

    /**
     * Safety clamp only. A skill's real ceiling is how many modifiers it authors — see GetMaxSkillLevel — because a
     * level exists to buy one. This exists so a content edit that pastes a hundred modifiers into one asset cannot
     * hand out a hundred points before anyone notices; it is not the number a designer tunes progression with.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression|Skills", meta = (ClampMin = "1", ClampMax = "50"))
    int32 MaxSkillLevel = 10;

    // Modifiers a skill may have active at once before any Unlock.Rule.SkillModifier deed is done.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression|Skills", meta = (ClampMin = "1", ClampMax = "8"))
    int32 BaseModifierCapacity = 1;

    // The ceiling those unlocks climb towards.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression|Skills", meta = (ClampMin = "1", ClampMax = "8"))
    int32 MaxModifierCapacity = 4;

    // SERVER: put Skill in SlotIndex, granting its ability on the owner's ASC under that slot's input tag. Refuses (and
    // says why) for an out-of-range or locked slot, a skill with no ability, a skill whose RequiredTag the player has
    // not earned, or a skill already bound to another slot. Whatever the slot held is cleared first.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Progression|Skills")
    void ServerEquipSkill(int32 SlotIndex, UMythicSkillDefinition *Skill);

    // SERVER: empty SlotIndex and clear the ability it granted.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Progression|Skills")
    void ServerUnequipSkill(int32 SlotIndex);

    // SERVER: switch one authored modifier on or off. Switching on costs its PointCost and takes one of the slots
    // GetModifierCapacity allows; switching off hands both back. Refuses (and says why) for a skill this character
    // has not earned, an index the definition does not have, a modifier that would overrun the capacity or the
    // points, or a modifier authored with no deltas at all.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Progression|Skills")
    void ServerSetModifierActive(UMythicSkillDefinition *Skill, int32 ModifierIndex, bool bActive);

    /**
     * SERVER: one completed use of Skill, recorded by the ability that ran it. Adds a use to the skill's row, mirrors
     * it into the stat ledger as Stat.Skill.Used, then grants every level the new total has crossed on the authored
     * ladder. This is the ONLY path that raises a skill level in play, and no client can reach it: a copy running on
     * a client returns before it touches anything.
     */
    UFUNCTION(BlueprintCallable, Category = "Progression|Skills")
    void RecordSkillUse(UMythicSkillDefinition *Skill);

    // The deed may be recorded in any of the four ledgers that can hold one — achievements, unlocks, story tags, or the
    // owner's ASC — so all four count. A skill with no RequiredTag needs no deed.
    UFUNCTION(BlueprintPure, Category = "Progression|Skills")
    bool IsSkillUnlocked(const UMythicSkillDefinition *Skill) const;

    UFUNCTION(BlueprintPure, Category = "Progression|Skills")
    bool IsSlotUnlocked(int32 SlotIndex) const;

    UFUNCTION(BlueprintPure, Category = "Progression|Skills")
    int32 GetUnlockedSlots() const { return FMath::Clamp(UnlockedSlots, 0, MaxSlots); }

    UFUNCTION(BlueprintPure, Category = "Progression|Skills")
    UMythicSkillDefinition *GetSkillInSlot(int32 SlotIndex) const;

    UFUNCTION(BlueprintPure, Category = "Progression|Skills")
    FGameplayTag GetSlotInputTag(int32 SlotIndex) const;

    // A skill nobody has levelled is level one, so this answers for every skill, stored or not.
    UFUNCTION(BlueprintPure, Category = "Progression|Skills")
    int32 GetSkillLevel(const UMythicSkillDefinition *Skill) const;

    /**
     * How far this skill can be levelled: one level per modifier it authors, since a level exists to pay for one.
     * A skill authoring nothing to buy sits at level one forever, which is the same as saying it has nothing to
     * level toward. MaxSkillLevel clamps the answer, and is only ever a guard against runaway content.
     */
    UFUNCTION(BlueprintPure, Category = "Progression|Skills")
    int32 GetMaxSkillLevel(const UMythicSkillDefinition *Skill) const;

    // Times this character has cast Skill. Never negative, and zero for a skill nobody has used.
    UFUNCTION(BlueprintPure, Category = "Progression|Skills")
    int32 GetSkillUses(const UMythicSkillDefinition *Skill) const;

    // Running total of uses the next level needs, or 0 at the ceiling. What a practice bar divides by.
    UFUNCTION(BlueprintPure, Category = "Progression|Skills")
    int32 GetUsesForNextLevel(const UMythicSkillDefinition *Skill) const;

    // Positions in Skill->Modifiers that are switched on. This is what the ability folds into its resolved shape.
    UFUNCTION(BlueprintPure, Category = "Progression|Skills")
    TArray<int32> GetActiveModifiers(const UMythicSkillDefinition *Skill) const;

    UFUNCTION(BlueprintPure, Category = "Progression|Skills")
    bool IsModifierActive(const UMythicSkillDefinition *Skill, int32 ModifierIndex) const;

    // One per level reached, so a freshly learned skill already has one to spend.
    UFUNCTION(BlueprintPure, Category = "Progression|Skills")
    int32 GetGrantedPoints(const UMythicSkillDefinition *Skill) const;

    UFUNCTION(BlueprintPure, Category = "Progression|Skills")
    int32 GetSpentPoints(const UMythicSkillDefinition *Skill) const;

    UFUNCTION(BlueprintPure, Category = "Progression|Skills")
    int32 GetAvailablePoints(const UMythicSkillDefinition *Skill) const;

    // How many modifiers one skill may hold active at once. Separate from points: points say what you own, this says
    // how much of it you can carry.
    UFUNCTION(BlueprintPure, Category = "Progression|Skills")
    int32 GetModifierCapacity() const;

    const TArray<TSoftObjectPtr<UMythicSkillDefinition>> &GetEquippedSkills() const { return EquippedSkills; }

    const TArray<FMythicSkillProgress> &GetSkillProgress() const { return SkillProgress; }

    // Open the next slot. Called by the GrantSkillSlot unlock effect; clamped to MaxSlots.
    void GrantSlot();

    // Carry one more modifier at once. Called by the GrantSkillModifierSlot unlock effect; clamped to MaxModifierCapacity.
    void GrantModifierCapacity();

    /**
     * Raise Skill by Levels without the practice, clamped to the skill's ceiling. Server-side and plain C++ on
     * purpose: it sits beside GrantSlot and GrantModifierCapacity because it hands out the same kind of thing, and
     * because the moment it is reachable by RPC the point economy mints itself. Cheats and tests call it.
     */
    void GrantSkillLevel(UMythicSkillDefinition *Skill, int32 Levels = 1);

    /**
     * Running total of uses that stands at Level, from an authored ladder. Level one is free. Past the last authored
     * row the total compounds by TailGrowth per level, so a skill that gains a modifier never runs off the end of
     * the table. An empty ladder is unreachable at every level above one — a skill that cannot be practised into.
     */
    static int64 UsesToReachLevel(const TArray<int32> &Thresholds, float TailGrowth, int32 Level);

    // The level Uses has bought against that ladder, never above Ceiling and never below one.
    static int32 LevelFromUses(const TArray<int32> &Thresholds, float TailGrowth, int32 Ceiling, int64 Uses);

    // Authority-only. Restores persisted skill state and re-grants each restored skill's ability.
    void RestoreSkills(const TArray<TSoftObjectPtr<UMythicSkillDefinition>> &SavedSkills, int32 SavedUnlockedSlots);

    // Authority-only. Restores practice, levels and active modifiers, re-running every gate the live verbs run —
    // inert, capacity and cost alike — so a save that predates a content or ceiling change loads the part of itself
    // that still holds up.
    void RestoreSkillProgress(const TArray<FMythicSkillProgress> &SavedProgress, int32 SavedModifierCapacity);

    UPROPERTY(BlueprintAssignable, Category = "Progression|Skills")
    FMythicOnSkillsChanged OnSkillsChanged;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Always MaxSlots long on the server; an empty slot is a null entry, so a slot index is an array index.
    UPROPERTY(ReplicatedUsing = OnRep_Skills, BlueprintReadOnly, SaveGame, Category = "Progression|Skills")
    TArray<TSoftObjectPtr<UMythicSkillDefinition>> EquippedSkills;

    UPROPERTY(ReplicatedUsing = OnRep_Skills, BlueprintReadOnly, SaveGame, Category = "Progression|Skills")
    int32 UnlockedSlots = 1;

    // Sparse: a skill earns an entry the first time it is levelled or a modifier is switched on. Absent means level
    // one with nothing active, which is what a character starts every skill on.
    UPROPERTY(ReplicatedUsing = OnRep_Skills, BlueprintReadOnly, SaveGame, Category = "Progression|Skills")
    TArray<FMythicSkillProgress> SkillProgress;

    UPROPERTY(ReplicatedUsing = OnRep_Skills, BlueprintReadOnly, SaveGame, Category = "Progression|Skills")
    int32 ModifierCapacity = 1;

    UFUNCTION()
    void OnRep_Skills();

private:
    UAbilitySystemComponent *ResolveASC() const;

    void SizeSlots();

    void ClearSlotAbility(int32 SlotIndex);

    FGameplayAbilitySpecHandle GrantSlotAbility(UAbilitySystemComponent *ASC, UMythicSkillDefinition *Skill, int32 SlotIndex);

    FMythicSkillProgress *FindProgress(const FSoftObjectPath &SkillPath);

    const FMythicSkillProgress *FindProgress(const FSoftObjectPath &SkillPath) const;

    FMythicSkillProgress &FindOrAddProgress(UMythicSkillDefinition *Skill);

    // Drops the entry once it says nothing a default would not, so an untouched skill leaves no row behind.
    void PruneProgress(const FSoftObjectPath &SkillPath);

    static int32 PointCostOf(const UMythicSkillDefinition *Skill, int32 ModifierIndex);

    // The authored ladder, read back off the settings object rather than held as a constant anywhere in here.
    static void ResolveUseLadder(TArray<int32> &OutThresholds, float &OutTailGrowth);

    class UMythicStatLedgerComponent *ResolveStatLedger() const;

    // Server-only, index-matched to EquippedSkills.
    TArray<FGameplayAbilitySpecHandle> GrantedHandles;
};
