#pragma once

#include "CoreMinimal.h"
#include "AI/Creatures/MythicCreatureCharacter.h"
#include "GAS/Mounts/MythicMountTypes.h"
#include "Engine/TimerHandle.h"
#include "MythicMount.generated.h"

class AMythicPlayerState;

UENUM(BlueprintType)
enum class EMythicMountGait : uint8 {
    Walk = 0,
    Trot = 1,
    Gallop = 2
};

UCLASS(Blueprintable)
class MYTHIC_API AMythicMount : public AMythicCreatureCharacter {
    GENERATED_BODY()

public:
    AMythicMount();

    // ── Config (EditDefaultsOnly on the BP/CDO) ─────────────────────────────────────────────────────────────────
    // Skeletal-mesh socket the rider pawn snaps to while riding (authored on the mount BP's mesh).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mount|Seat")
    FName SeatSocketName = FName("SeatSocket");

    // Gait speeds (cm/s). Gallop is the BASE for MythicMountStatics::ComputeGallopSpeed (bond raises it, stamina gates it).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mount|Movement", meta = (ClampMin = "0.0"))
    float WalkSpeed = 250.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mount|Movement", meta = (ClampMin = "0.0"))
    float TrotSpeed = 600.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mount|Movement", meta = (ClampMin = "0.0"))
    float GallopBaseSpeed = 1100.0f;

    // Stamina pool + drain (while galloping) / regen (otherwise) rates, per second. One repeating timer, no Tick.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mount|Stamina", meta = (ClampMin = "1.0"))
    float MaxStamina = 100.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mount|Stamina", meta = (ClampMin = "0.0"))
    float StaminaDrainPerSecond = 12.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mount|Stamina", meta = (ClampMin = "0.0"))
    float StaminaRegenPerSecond = 18.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mount|Stamina", meta = (ClampMin = "0.05"))
    float StaminaTickInterval = 0.25f;

    // Max distance (cm) the rider may be from the mount to mount up. <= 0 disables the range gate.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mount", meta = (ClampMin = "0.0"))
    float MountRange = 350.0f;

    // Sideways offset (cm) for the nav-projected dismount spot (actor-right; nav projection may move it).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mount", meta = (ClampMin = "0.0"))
    float DismountSideOffset = 180.0f;

    // ── Bond (EARNED BY RIDING — banked to the roster record on dismount) ───────────────────────────────────────────
    // Bond XP earned per second ridden. Banked to the owning player's roster record when the ride ends (→ BondLevel →
    // gallop speed via MythicMountStatics::ComputeGallopSpeed). 0 disables bond-by-riding. Per-BP so a species can be
    // tuned to bond slower/faster. This is the write-side that makes BondXP (and thus mount improvement) actually grow.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mount|Bond", meta = (ClampMin = "0.0"))
    float BondXpPerSecondRidden = 1.0f;

    // Anti-farm clamp on the bond XP a SINGLE ride can bank (a long AFK ride can't dump unbounded XP). <= 0 disables.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mount|Bond", meta = (ClampMin = "0"))
    int32 MaxBondXpPerRide = 500;

    // ── Replicated identity/state ───────────────────────────────────────────────────────────────────────────────
    // Which roster record this live actor embodies (set at summon; invalid for a hand-placed test mount).
    // COND_OwnerOnly (J5): read only server-side (roster bond sync) — other clients never need it.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Mount")
    FGuid OwnerMountId;

    // The owning player's canonical key (AMythicPlayerState::GetCanonicalPlayerKey) — the co-op ownership gate.
    // Empty = unowned (a hand-placed mount anyone may ride, useful in test maps). Replicated to ALL on purpose (J5
    // audit): every client's GetInteractionData reads it to hide the Ride prompt on someone else's horse.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Mount")
    FString OwnerPlayerKey;

    // The owning PlayerState (for despawn-on-logout checks + UI attribution). Weak-ish via replication; may be null.
    // COND_OwnerOnly (J5): no non-owner client reads exist.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Mount")
    TObjectPtr<AMythicPlayerState> OwnerPlayerState;

    // Bond level snapshotted from the record at summon (drives gallop speed). COND_OwnerOnly (J5): the riding
    // connection IS the net owner (Possess), so its prediction/HUD get bond; a simulated proxy's CMC consumes
    // replicated velocity, making its locally-computed MaxWalkSpeed inert.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Mount")
    int32 BondLevel = 0;

    // Current stamina. Only the riding connection needs it (its HUD bar) — COND_OwnerOnly; while ridden the mount's
    // owner IS the rider's PlayerController (Possess sets it), so this reaches exactly the right client.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Mount")
    float Stamina = 100.0f;

    // Current gait (rider-requested, server-validated). Replicated to ALL (anim + speed apply on every client).
    UPROPERTY(ReplicatedUsing = OnRep_Gait, BlueprintReadOnly, Category = "Mount")
    EMythicMountGait CurrentGait = EMythicMountGait::Walk;

    // The cosmetic passenger pawn while ridden (the rider's real player pawn). Null when riderless.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Mount")
    TObjectPtr<APawn> RiderPawn;

    UFUNCTION(BlueprintPure, Category = "Mount")
    bool IsRidden() const { return RiderPawn != nullptr; }

    UFUNCTION(BlueprintPure, Category = "Mount")
    float GetStamina01() const { return MaxStamina > 0.0f ? Stamina / MaxStamina : 0.0f; }

    void ConfigureFromRecord(const FMythicMountRecord &Record, const FString &InOwnerKey, AMythicPlayerState *InOwnerPS);

    bool ServerMount(APawn *InRiderPawn);

    // SERVER (owning-client callable): reverse the swap — detach the rider to a nav-projected side offset, restore
    // CMC + collision, re-possess the rider pawn, restore/park the mount's AI controller, clear GAS.State.Mounted.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Mount")
    void ServerDismount();

    // SERVER (owning-client callable): rider gait toggle (walk/trot/gallop). Gallop requires stamina > 0; hitting 0
    // stamina server-drops the gait to trot. Applies CMC MaxWalkSpeed via ComputeGallopSpeed for gallop.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Mount")
    void ServerSetGait(EMythicMountGait NewGait);

    EMountGateResult EvaluateMountGate(const APawn *InRiderPawn) const;

    virtual void OnPrimaryInteract_Implementation(AActor *Interactor) override;
    virtual bool GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Fired on the server when a rider mounts / dismounts, so the mount BP can play saddle-up cosmetics, and the
    // riding camera/input BP layer can react (editor handoff — no camera invented in C++).
    UFUNCTION(BlueprintImplementableEvent, Category = "Mount")
    void OnMounted(APawn *InRiderPawn);
    UFUNCTION(BlueprintImplementableEvent, Category = "Mount")
    void OnDismounted(APawn *InRiderPawn);

    // Interaction prompt data (same mechanism as the corpse/container; assign a table with a "Ride" row on the BP).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mount|Interaction")
    TObjectPtr<const UCommonGenericInputActionDataTable> MountInputActionDataTable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mount|Interaction")
    FName RideInteractionName = FName("Ride");

    UFUNCTION()
    void OnRep_Gait();

    UFUNCTION()
    void HandleMountDeath(AActor *DeadActor);

    void StaminaTick();

    void StartStaminaTimer();
    void StopStaminaTimer();

    void ApplyGaitSpeed();
    float SpeedForGait(EMythicMountGait Gait) const;

    FVector FindDismountLocation() const;

    static class UAbilitySystemComponent *ResolveRiderASC(const APawn *Pawn);

    TWeakObjectPtr<AController> ParkedAIController;

    double RideStartTime = 0.0;

    FTimerHandle StaminaTimerHandle;
};
