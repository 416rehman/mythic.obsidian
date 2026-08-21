#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GAS/Mounts/MythicMountTypes.h"
#include "MythicMountRosterComponent.generated.h"

class AMythicMount;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMythicOnMountRosterChanged);

UCLASS(ClassGroup = (Mythic), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicMountRosterComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicMountRosterComponent();

    // ── Config ──
    // Mount actor class the whistle spawns. Unset ⇒ UMythicDeveloperSettings::DefaultMountClass (soft, preloaded).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mounts")
    TSubclassOf<AMythicMount> MountClassOverride;

    // Soft roster cap (a stable only holds so many horses). ServerAddMount refuses past this. <= 0 = unbounded.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mounts", meta = (ClampMin = "0"))
    int32 MaxRosterSize = 8;

    FGuid ServerAddMount(uint8 SpeciesId, FName CustomName);

    void ServerGrantBondXP(const FGuid &MountId, int32 XP);

    void RestoreRoster(const TArray<FMythicMountRecord> &InRoster, const FGuid &InActiveId);

    // ── Player verbs (client → server RPCs on this owned component; validated server-side) ──
    // Select which roster record the whistle summons.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Mounts")
    void ServerSetActiveMount(FGuid MountId);

    // Rename a roster record (cosmetic).
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Mounts")
    void ServerRenameMount(FGuid MountId, FName NewName);

    // WHISTLE: summon the active mount near the owner's pawn (nav-projected). Gated by MythicMountStatics::CanSummon
    // (active record + not InCombat + cooldown). Destroys any previously-summoned live mount first (one live mount
    // per player). Records Stat.Mount.Summoned. Lives HERE because the PlayerController is do-not-edit.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Mounts")
    void ServerSummonMount();

    // Stash (despawn) the live summoned mount; the roster record (and bActive) persists — "sent to the stable".
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Mounts")
    void ServerStashMount();

    // ── Reads (server + owning client) ──
    UFUNCTION(BlueprintPure, Category = "Mounts")
    const TArray<FMythicMountRecord> &GetRoster() const { return Roster; }

    UFUNCTION(BlueprintPure, Category = "Mounts")
    FGuid GetActiveMountId() const { return ActiveMountId; }

    const FMythicMountRecord *GetActiveRecord() const;

    UFUNCTION(BlueprintPure, Category = "Mounts")
    AMythicMount *GetSummonedMount() const { return SummonedMount.Get(); }

    UFUNCTION(BlueprintPure, Category = "Mounts")
    bool IsMountSummoned() const { return SummonedMount.IsValid(); }

    UPROPERTY(BlueprintAssignable, Category = "Mounts")
    FMythicOnMountRosterChanged OnRosterChanged;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(ReplicatedUsing = OnRep_Roster, SaveGame)
    TArray<FMythicMountRecord> Roster;

    UPROPERTY(Replicated, SaveGame)
    FGuid ActiveMountId;

    UFUNCTION()
    void OnRep_Roster();

    UFUNCTION()
    void HandleOwnerPawnDeath(AActor *DeadActor);

    void SummonMountNear(const FVector &NearLocation);

    void DespawnSummonedMount();

    void SyncActiveFlags();

    UClass *ResolveMountClass() const;

    double ServerNow() const;

    TWeakObjectPtr<AMythicMount> SummonedMount;

    double LastSummonServerTime = -1.0e9;
};
