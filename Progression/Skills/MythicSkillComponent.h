
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "MythicSkillDefinition.h"
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

    // SERVER: put Skill in SlotIndex, granting its ability on the owner's ASC under that slot's input tag. Refuses (and
    // says why) for an out-of-range or locked slot, a skill with no ability, a skill whose RequiredTag the player has
    // not earned, or a skill already bound to another slot. Whatever the slot held is cleared first.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Progression|Skills")
    void ServerEquipSkill(int32 SlotIndex, UMythicSkillDefinition *Skill);

    // SERVER: empty SlotIndex and clear the ability it granted.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Progression|Skills")
    void ServerUnequipSkill(int32 SlotIndex);

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

    const TArray<TSoftObjectPtr<UMythicSkillDefinition>> &GetEquippedSkills() const { return EquippedSkills; }

    // Open the next slot. Called by the GrantSkillSlot unlock effect; clamped to MaxSlots.
    void GrantSlot();

    // Authority-only. Restores persisted skill state and re-grants each restored skill's ability.
    void RestoreSkills(const TArray<TSoftObjectPtr<UMythicSkillDefinition>> &SavedSkills, int32 SavedUnlockedSlots);

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

    UFUNCTION()
    void OnRep_Skills();

private:
    UAbilitySystemComponent *ResolveASC() const;

    void SizeSlots();

    void ClearSlotAbility(int32 SlotIndex);

    FGameplayAbilitySpecHandle GrantSlotAbility(UAbilitySystemComponent *ASC, UMythicSkillDefinition *Skill, int32 SlotIndex);

    // Server-only, index-matched to EquippedSkills.
    TArray<FGameplayAbilitySpecHandle> GrantedHandles;
};
