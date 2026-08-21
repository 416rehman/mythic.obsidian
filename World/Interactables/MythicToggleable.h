
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Interaction/IMythicInteractable.h"
#include "Subsystem/SaveSystem/World/MythicSaveableActor.h"
#include "MythicToggleable.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UCommonGenericInputActionDataTable;
class UMythicItemInstance;
class UProficiencyDefinition;

struct FMythicToggleOutcome {
    bool bChanged = false;
    bool bNewIsOn = false;
};

struct FMythicUnlockOutcome {
    bool bUnlock = false;
    bool bConsumeKey = false;
};

UCLASS()
class MYTHIC_API AMythicToggleable : public AActor, public IMythicInteractable, public IMythicSaveableActor {
    GENERATED_BODY()

public:
    AMythicToggleable();

    virtual void OnPrimaryInteract_Implementation(AActor *Interactor) override;
    virtual void OnSecondaryInteract_Implementation(AActor *Interactor) override;
    virtual USceneComponent *GetWidgetAttachmentComponent_Implementation() const override;
    virtual bool GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const override;
    virtual void OnFocused_Implementation(AActor *Interactor) override;
    virtual void OnUnfocused_Implementation(AActor *Interactor) override;

    UFUNCTION(BlueprintPure, Category = "Toggleable")
    bool IsOn() const { return bIsOn; }

    void ServerToggle(AActor *Interactor);

    static FMythicToggleOutcome ResolveToggle(bool bCurrentlyOn, bool bLocked, bool bOneShot, bool bHasActivated);

    static bool DoesKeyOpenLock(const FGameplayTagContainer &KeyTypeProbe, const FGameplayTag &RequiredKeyTag);

    static FMythicUnlockOutcome PlanKeyedUnlock(bool bLocked, bool bHasMatchingKey, bool bConsumeKey);

    static float ComputePickSuccessChance(int32 SkillLevel, int32 LockDifficulty);

    static bool ResolvePickLock(int32 SkillLevel, int32 LockDifficulty, float Roll01);

    virtual void DeserializeCustomData(const TArray<uint8> &InCustomData) override;

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    UFUNCTION()
    void OnRep_IsOn();

    UFUNCTION()
    void OnRep_Locked();

    // Cosmetic lock-state reaction (padlock mesh show/hide, locked-rattle vs unlock click). Fires on the acting server
    // AND on clients via OnRep. LOCAL/cosmetic — gameplay reads the authoritative bLocked.
    UFUNCTION(BlueprintImplementableEvent, Category = "Toggleable")
    void OnLockStateChanged(bool bNewLocked);

    // Cosmetic open/close reaction (mesh swap / animation / sound). Fires on the acting server AND on clients via
    // OnRep. LOCAL/cosmetic — it never gates gameplay (the authoritative state is bIsOn).
    UFUNCTION(BlueprintImplementableEvent, Category = "Toggleable")
    void OnToggleVisualChanged(bool bNewIsOn);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Toggleable")
    USceneComponent *SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Toggleable")
    UStaticMeshComponent *Mesh;

    // Replicated + persisted on/off state (door open, lever thrown, gate raised).
    UPROPERTY(ReplicatedUsing = OnRep_IsOn, SaveGame, BlueprintReadOnly, Category = "Toggleable")
    bool bIsOn = false;

    UPROPERTY(SaveGame)
    bool bHasActivated = false;

    // Initial state when first placed (before any save restore).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toggleable")
    bool bStartsOn = false;

    // Locked: direct interaction does nothing until unlocked — by a held key (RequiredKeyTag) or another system (quest
    // gate). Replicated (clients show the locked/unlocked prompt + visual) + SaveGame (an unlock persists). EditAnywhere
    // seeds the placed/initial state.
    UPROPERTY(ReplicatedUsing = OnRep_Locked, SaveGame, EditAnywhere, BlueprintReadOnly, Category = "Toggleable")
    bool bLocked = false;

    // The key item tag that unlocks this when a player interacts while holding a matching key. EMPTY = no key opens it
    // (a quest-gate lock only another system clears — the prior behaviour). A "key" is any inventory item whose effective
    // type-probe ({def ItemType} ∪ runtime ItemTags) contains this tag — data-driven, no dedicated key class needed.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toggleable")
    FGameplayTag RequiredKeyTag;

    // If true, the matching key is consumed (one stack) when it unlocks this — a single-use key. Default false: a
    // reusable key that stays in the player's inventory.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toggleable")
    bool bConsumeKeyOnUnlock = false;

    // One-shot: can only be switched ON once (a one-time portcullis lever); further interacts no-op.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toggleable")
    bool bOneShot = false;

    // ── Lockpicking ──
    // If true, a LOCKED object without a matching key can be PICKED (a skill roll) on interact. Picking an OWNED lock (a
    // stamped UMythicOwnershipComponent) is a WITNESSED theft crime — submitted for the ATTEMPT, success or not. DEFAULT
    // FALSE → a lock stays key-only and no pick/crime path ever runs (byte-identical to before).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toggleable|Lock")
    bool bLockPickable = false;

    // Difficulty subtracted from the picker's lockpick level in the success roll. Higher = harder. Only read when
    // bLockPickable is true.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toggleable|Lock", meta = (ClampMin = "0"))
    int32 LockpickDifficulty = 0;

    // Optional lockpick proficiency track whose level feeds the pick roll. Unset = skill level 0 (base chance only).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toggleable|Lock")
    TObjectPtr<UProficiencyDefinition> LockpickProficiency;

    // Remote toggleables this one drives (a lever → its gates). On a state change each is mirrored to THIS object's
    // new state (one hop, no cascade — a linked target's own lock is bypassed; that's the lever's authority).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Toggleable")
    TArray<TObjectPtr<AMythicToggleable>> LinkedToggleables;

    // Interaction prompt data (matches the container/station pattern).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    TObjectPtr<const UCommonGenericInputActionDataTable> InputActionDataTable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FName PrimaryInteractionName = FName("Use");

private:
    void ApplyState(bool bNewIsOn);

    bool ServerTryUnlockWithKey(AActor *Interactor);

    UMythicItemInstance *FindMatchingKey(AActor *Interactor) const;

    bool ServerTryPickLock(AActor *Interactor);

    int32 ResolveLockpickLevel(AActor *Interactor) const;
};
