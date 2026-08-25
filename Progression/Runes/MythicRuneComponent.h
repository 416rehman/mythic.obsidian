
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Components/ActorComponent.h"
#include "MythicRuneDefinition.h"
#include "UObject/SoftObjectPtr.h"
#include "MythicRuneComponent.generated.h"

class UAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMythicOnRunesChanged);

UCLASS(ClassGroup = (Mythic), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicRuneComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicRuneComponent();

    // How many sockets this character can ever have. Slot one is free; the rest arrive as GrantPerkSlot unlocks.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression|Runes", meta = (ClampMin = "1", ClampMax = "8"))
    int32 MaxSlots = 4;

    // SERVER: put Rune in SlotIndex, granting its passive ability on the owner's ASC. Refuses (and says why) for an
    // out-of-range or locked slot, a rune with no ability, a rune whose RequiredTag the player has not earned, or a
    // rune already worn in another slot. Whatever the slot held is cleared first.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Progression|Runes")
    void ServerEquipRune(int32 SlotIndex, UMythicRuneDefinition *Rune);

    // SERVER: empty SlotIndex and clear the ability it granted.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Progression|Runes")
    void ServerUnequipRune(int32 SlotIndex);

    // The deed may be recorded in any of the four ledgers that can hold one — achievements, unlocks, story tags, or the
    // owner's ASC — so all four count. A rune with no RequiredTag needs no deed.
    UFUNCTION(BlueprintPure, Category = "Progression|Runes")
    bool IsRuneUnlocked(const UMythicRuneDefinition *Rune) const;

    UFUNCTION(BlueprintPure, Category = "Progression|Runes")
    bool IsSlotUnlocked(int32 SlotIndex) const;

    UFUNCTION(BlueprintPure, Category = "Progression|Runes")
    int32 GetUnlockedSlots() const { return FMath::Clamp(UnlockedSlots, 0, MaxSlots); }

    UFUNCTION(BlueprintPure, Category = "Progression|Runes")
    UMythicRuneDefinition *GetRuneInSlot(int32 SlotIndex) const;

    const TArray<TSoftObjectPtr<UMythicRuneDefinition>> &GetEquippedRunes() const { return EquippedRunes; }

    // Open the next socket. Called by the GrantPerkSlot unlock effect; clamped to MaxSlots.
    void GrantSlot();

    // Authority-only. Restores persisted rune state and re-grants each restored rune's ability.
    void RestoreRunes(const TArray<TSoftObjectPtr<UMythicRuneDefinition>> &SavedRunes, int32 SavedUnlockedSlots);

    UPROPERTY(BlueprintAssignable, Category = "Progression|Runes")
    FMythicOnRunesChanged OnRunesChanged;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Always MaxSlots long on the server; an empty socket is a null entry, so a slot index is an array index.
    UPROPERTY(ReplicatedUsing = OnRep_Runes, BlueprintReadOnly, SaveGame, Category = "Progression|Runes")
    TArray<TSoftObjectPtr<UMythicRuneDefinition>> EquippedRunes;

    UPROPERTY(ReplicatedUsing = OnRep_Runes, BlueprintReadOnly, SaveGame, Category = "Progression|Runes")
    int32 UnlockedSlots = 1;

    UFUNCTION()
    void OnRep_Runes();

private:
    UAbilitySystemComponent *ResolveASC() const;

    void SizeSlots();

    void ClearSlotAbility(int32 SlotIndex);

    // Server-only, index-matched to EquippedRunes.
    TArray<FGameplayAbilitySpecHandle> GrantedHandles;
};
