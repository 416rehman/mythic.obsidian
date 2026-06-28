// Mythic — toggleable world interactable (door / gate / lever / switch)
// A server-authoritative on/off world object: interacting flips its state (server), which replicates (dormant until
// it changes) and persists (SaveGame). A lever can drive remote targets via LinkedToggleables so a switch here opens
// a gate there. The visual (mesh swap / open animation / sound) is a cosmetic Blueprint reaction — never replicated.

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

/** Pure outcome of a toggle interaction (what the state should become, and whether it changed). */
struct FMythicToggleOutcome {
    bool bChanged = false; // did the on/off state actually change → replicate + persist + propagate
    bool bNewIsOn = false; // the resulting state
};

/** Pure outcome of a keyed-unlock attempt (should the lock open, and should the key be consumed). */
struct FMythicUnlockOutcome {
    bool bUnlock = false;     // the lock should open
    bool bConsumeKey = false; // the matching key should be consumed (single-use key)
};

UCLASS()
class MYTHIC_API AMythicToggleable : public AActor, public IMythicInteractable, public IMythicSaveableActor {
    GENERATED_BODY()

public:
    AMythicToggleable();

    //~ IMythicInteractable
    virtual void OnPrimaryInteract_Implementation(AActor *Interactor) override;
    virtual void OnSecondaryInteract_Implementation(AActor *Interactor) override;
    virtual USceneComponent *GetWidgetAttachmentComponent_Implementation() const override;
    virtual bool GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const override;
    virtual void OnFocused_Implementation(AActor *Interactor) override;
    virtual void OnUnfocused_Implementation(AActor *Interactor) override;

    UFUNCTION(BlueprintPure, Category = "Toggleable")
    bool IsOn() const { return bIsOn; }

    // SERVER: apply a toggle interaction (respects lock / one-shot), then mirror the new state to LinkedToggleables.
    // Public so a lever-link or a debug command can drive it; no-op off authority.
    void ServerToggle(AActor *Interactor);

    // Pure toggle decision — locked → no change; one-shot already fired → no change; one-shot fresh → turn ON;
    // otherwise flip. Static + no engine state so the rule is unit-testable without a live actor.
    static FMythicToggleOutcome ResolveToggle(bool bCurrentlyOn, bool bLocked, bool bOneShot, bool bHasActivated);

    // Pure: a key opens a lock iff the lock requires a VALID key tag and the key item's effective type-probe ({def
    // ItemType} ∪ ItemTags) contains it. An empty RequiredKeyTag never matches (a quest-gate lock no key opens).
    static bool DoesKeyOpenLock(const FGameplayTagContainer &KeyTypeProbe, const FGameplayTag &RequiredKeyTag);

    // Pure unlock decision: a LOCKED object with a matching key unlocks (and consumes the key iff bConsumeKey). An
    // already-unlocked object, or a locked one without a matching key, → no unlock. Static + unit-testable.
    static FMythicUnlockOutcome PlanKeyedUnlock(bool bLocked, bool bHasMatchingKey, bool bConsumeKey);

    //~ IMythicSaveableActor — no nested UObjects, but this guaranteed post-restore hook (called after the SaveGame
    //  properties are deserialized) reconciles the replicated + cosmetic state: the raw restore of bIsOn/bLocked neither
    //  wakes this DORM_DormantAll actor nor re-fires the BP visuals, so do that here.
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

    // Whether a one-shot has already fired (server logic; persisted so a fired one-shot stays fired across save/load).
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
    // SERVER: set the replicated state, fire the visual, wake dormancy so the change replicates. Used by the self
    // toggle and by link propagation.
    void ApplyState(bool bNewIsOn);

    // SERVER: if locked, attempt to unlock using a matching key the Interactor holds (clears bLocked, persists +
    // replicates, consumes the key iff bConsumeKeyOnUnlock). Returns true if the object is unlocked afterwards (either
    // it just unlocked, or it was never locked). No-op / false off authority or with no matching key.
    bool ServerTryUnlockWithKey(AActor *Interactor);

    // Scan the Interactor's inventories for the first item whose type-probe opens this lock (RequiredKeyTag). nullptr
    // if none / no inventory provider. O(slots), run once per unlock interaction (not a hot path).
    UMythicItemInstance *FindMatchingKey(AActor *Interactor) const;
};
