
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Components/ActorComponent.h"
#include "MythicRuneDefinition.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "UObject/SoftObjectPtr.h"
#include "MythicRuneComponent.generated.h"

class UAbilitySystemComponent;
class UMythicRuneComponent;

// What the owner's HUD draws for a worn rune. Hidden draws nothing and is the state of every socket by default.
UENUM(BlueprintType)
enum class EMythicRuneHudState : uint8 {
    Hidden,
    Ready,
    Active,
    Cooldown
};

USTRUCT()
struct MYTHIC_API FMythicRuneHudStateItem : public FFastArraySerializerItem {
    GENERATED_BODY()

    UPROPERTY()
    int32 SlotIndex = INDEX_NONE;

    UPROPERTY()
    EMythicRuneHudState State = EMythicRuneHudState::Hidden;

    // Server world seconds. End stays 0 when the state has no timed window.
    UPROPERTY()
    double ServerStartTimeSeconds = 0.0;

    UPROPERTY()
    double ServerEndTimeSeconds = 0.0;

    UPROPERTY()
    uint8 Stacks = 0;
};

// One row per socket with something to show; a socket with no row is Hidden.
USTRUCT()
struct MYTHIC_API FMythicRuneHudStateArray : public FFastArraySerializer {
    GENERATED_BODY()

    void SetOwner(UMythicRuneComponent *InOwner) { Owner = InOwner; }

    const TArray<FMythicRuneHudStateItem> &GetItems() const { return Items; }

    bool NetDeltaSerialize(FNetDeltaSerializeInfo &DeltaParms) {
        return FastArrayDeltaSerialize<FMythicRuneHudStateItem, FMythicRuneHudStateArray>(Items, DeltaParms, *this);
    }

    void PostReplicatedReceive(const FFastArraySerializer::FPostReplicatedReceiveParameters &Parameters);

private:
    friend class UMythicRuneComponent;

    UPROPERTY()
    TArray<FMythicRuneHudStateItem> Items;

    UMythicRuneComponent *Owner = nullptr;
};

template <>
struct TStructOpsTypeTraits<FMythicRuneHudStateArray> : TStructOpsTypeTraitsBase2<FMythicRuneHudStateArray> {
    enum { WithNetDeltaSerializer = true };
};

// Why a request was turned down. None means the verb may run.
UENUM(BlueprintType)
enum class EMythicRuneRefusal : uint8 {
    None,
    SlotOutOfRange,
    SlotLocked,
    NoRune,
    NoPayload,
    DeedMissing,
    AlreadyWornHere,
    WornElsewhere,
    NoAbilitySystem,
    GrantFailed
};

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicRuneRollValue {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Progression|Runes")
    FGameplayTag Parameter;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Progression|Runes")
    float Value = 0.0f;
};

// Every number one owner rolled for one rune. Rolled at the rune's first socket and kept for good: taking the rune
// off, moving it, or wearing it again never rerolls, so a good roll is worth keeping and a bad one is a real cost.
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicRuneRollSet {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Progression|Runes")
    TSoftObjectPtr<UMythicRuneDefinition> Rune;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Progression|Runes")
    TArray<FMythicRuneRollValue> Values;

    const FMythicRuneRollValue *Find(FGameplayTag Parameter) const {
        return Values.FindByPredicate([Parameter](const FMythicRuneRollValue &Value) { return Value.Parameter == Parameter; });
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMythicOnRunesChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMythicOnRuneRefused, int32, SlotIndex, EMythicRuneRefusal, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMythicRuneHudStateChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMythicRuneRollsChanged);

UCLASS(ClassGroup = (Mythic), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicRuneComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicRuneComponent();

    // How many sockets this character can ever have. Slot one is free; the rest arrive as GrantPerkSlot unlocks.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression|Runes", meta = (ClampMin = "1", ClampMax = "8"))
    int32 MaxSlots = 4;

    // SERVER: put Rune in SlotIndex, granting its passive ability on the owner's ASC. Every refusal reaches the
    // owning client through ClientRuneRequestRefused. Whatever the slot held is cleared once the new grant lands.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Progression|Runes")
    void ServerEquipRune(int32 SlotIndex, UMythicRuneDefinition *Rune);

    // SERVER: empty SlotIndex and clear the ability it granted. An already-empty slot stays silent.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Progression|Runes")
    void ServerUnequipRune(int32 SlotIndex);

    // SERVER: carry the rune worn in FromSlot to ToSlot without re-granting its ability. Whatever ToSlot held is
    // cleared. One player intent, one validated verb, one broadcast.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Progression|Runes")
    void ServerMoveRune(int32 FromSlot, int32 ToSlot);

    // The same gate the server verbs run, so the UI can refuse locally before a click is honoured. OutOtherSlot is
    // the socket already wearing Rune when the answer is WornElsewhere, else INDEX_NONE.
    UFUNCTION(BlueprintPure, Category = "Progression|Runes")
    EMythicRuneRefusal CanEquipRune(int32 SlotIndex, const UMythicRuneDefinition *Rune, int32 &OutOtherSlot) const;

    // Sent from every refusal branch of the server verbs; broadcasts OnRuneRefused on the owning client. Unreliable
    // because a lost refusal costs one missed shake, never state.
    UFUNCTION(Client, Unreliable)
    void ClientRuneRequestRefused(int32 SlotIndex, EMythicRuneRefusal Reason);

    // Player-facing text for a refusal; OtherSlot is the socket named by WornElsewhere.
    static FText DescribeRefusal(EMythicRuneRefusal Reason, int32 OtherSlot);

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

    // SERVER: what the owner's HUD draws for the rune in SlotIndex. Hidden drops the row. DurationSeconds > 0 opens
    // a timed window the owning client counts down on its own clock; the server still decides the state after it.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Progression|Runes")
    void SetRuneHudState(int32 SlotIndex, EMythicRuneHudState State, float DurationSeconds = 0.f, int32 Stacks = 0);

    // SERVER: the same, addressed by the rune. A rune that is not worn is ignored.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Progression|Runes")
    void SetRuneHudStateForRune(const UMythicRuneDefinition *Rune, EMythicRuneHudState State, float DurationSeconds = 0.f,
                                int32 Stacks = 0);

    // INDEX_NONE when the rune is not worn.
    UFUNCTION(BlueprintPure, Category = "Progression|Runes")
    int32 FindSlotOfRune(const UMythicRuneDefinition *Rune) const;

    UFUNCTION(BlueprintPure, Category = "Progression|Runes")
    EMythicRuneHudState GetRuneHudState(int32 SlotIndex) const;

    // 0 when the state has no end time or the end has passed.
    UFUNCTION(BlueprintPure, Category = "Progression|Runes")
    float GetRuneHudRemainingSeconds(int32 SlotIndex) const;

    const TArray<FMythicRuneHudStateItem> &GetRuneHudStates() const { return HudStates.GetItems(); }

    // The clock every HUD state time is written against: the server's world seconds as this machine knows them.
    double GetServerWorldTimeSeconds() const;

    // Fires on the server after a set and on the owning client after each replicated receive.
    UPROPERTY(BlueprintAssignable, Category = "Progression|Runes")
    FOnMythicRuneHudStateChanged OnRuneHudStateChanged;

    // The number this owner rolled for Parameter on Rune. False for a rune this owner has never socketed and for a
    // parameter the rune does not roll; callers fall back to UMythicRuneDefinition::GetParameterMidpoint.
    UFUNCTION(BlueprintPure, Category = "Progression|Runes")
    bool GetRolledRuneValue(const UMythicRuneDefinition *Rune, FGameplayTag Parameter, float &OutValue) const;

    // One set per rune this owner has ever socketed, in first-socket order.
    const TArray<FMythicRuneRollSet> &GetRuneRolls() const { return RuneRolls; }

    // Fires on the server after a roll lands and on the owning client after each replicated receive.
    UPROPERTY(BlueprintAssignable, Category = "Progression|Runes")
    FOnMythicRuneRollsChanged OnRuneRollsChanged;

    // Open the next socket. Called by the GrantPerkSlot unlock effect; clamped to MaxSlots.
    void GrantSlot();

    // Close the highest socket, taking out whatever it held first. The first socket never closes.
    void RevokeSlot();

    // Authority-only. Restores persisted rune state and re-grants each restored rune's ability. A restored rune with
    // no roll set rolls one first, so a save older than rune rolls reads as a first socket.
    void RestoreRunes(const TArray<TSoftObjectPtr<UMythicRuneDefinition>> &SavedRunes, int32 SavedUnlockedSlots);

    // Authority-only. Replaces every roll set with the saved ones. Runs before RestoreRunes so a re-granted ability
    // finds its numbers the moment it activates.
    void RestoreRuneRolls(const TArray<FMythicRuneRollSet> &SavedRolls);

    UPROPERTY(BlueprintAssignable, Category = "Progression|Runes")
    FMythicOnRunesChanged OnRunesChanged;

    UPROPERTY(BlueprintAssignable, Category = "Progression|Runes")
    FMythicOnRuneRefused OnRuneRefused;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Always MaxSlots long on the server; an empty socket is a null entry, so a slot index is an array index.
    UPROPERTY(ReplicatedUsing = OnRep_Runes, BlueprintReadOnly, SaveGame, Category = "Progression|Runes")
    TArray<TSoftObjectPtr<UMythicRuneDefinition>> EquippedRunes;

    UPROPERTY(ReplicatedUsing = OnRep_Runes, BlueprintReadOnly, SaveGame, Category = "Progression|Runes")
    int32 UnlockedSlots = 1;

    UPROPERTY(Replicated)
    FMythicRuneHudStateArray HudStates;

    // Owner-only like the sockets: a partner never needs another player's numbers.
    UPROPERTY(ReplicatedUsing = OnRep_RuneRolls, BlueprintReadOnly, SaveGame, Category = "Progression|Runes")
    TArray<FMythicRuneRollSet> RuneRolls;

    UFUNCTION()
    void OnRep_Runes();

    UFUNCTION()
    void OnRep_RuneRolls();

private:
    friend struct FMythicRuneHudStateArray;

    UAbilitySystemComponent *ResolveASC() const;

    void SizeSlots();

    void ClearSlotAbility(int32 SlotIndex);

    const FMythicRuneRollSet *FindRollSet(const UMythicRuneDefinition *Rune) const;

    // True when a set was rolled now. A rune that already has one, or has nothing to roll, leaves the array alone.
    // Never broadcasts: the verb that rolled it does, once its own work has landed.
    bool EnsureRuneRolled(const UMythicRuneDefinition *Rune);

    FMythicRuneHudStateItem *FindHudItem(int32 SlotIndex);
    const FMythicRuneHudStateItem *FindHudItem(int32 SlotIndex) const;

    // True when a row was dropped. Never broadcasts: the verb that dropped it does, once.
    bool ClearHudItem(int32 SlotIndex);

    void HandleHudStatesReceived();

    void Refuse(const TCHAR *Verb, int32 SlotIndex, EMythicRuneRefusal Reason, const UMythicRuneDefinition *Rune,
                int32 OtherSlot);

    // Server-only, index-matched to EquippedRunes.
    TArray<FGameplayAbilitySpecHandle> GrantedHandles;
};
