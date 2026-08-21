
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Interaction/IMythicInteractable.h"
#include "Itemization/InventoryProviderInterface.h"
#include "Subsystem/SaveSystem/World/MythicSaveableActor.h"
#include "MythicCorpseTypes.h"
#include "World/Hunting/MythicSkinningRules.h"
#include "MythicCorpse.generated.h"

class UMythicInventoryComponent;
class UStaticMeshComponent;
class USceneComponent;
class USphereComponent;
class UInventoryProfile;
class UMythicCorpseConfig;
class UCommonGenericInputActionDataTable;
class AMythicPlayerController;

USTRUCT(BlueprintType)
struct FMythicCorpseIdentity {
    GENERATED_BODY()

    uint32 SourceNameHash = 0;

    // Affiliation/faction the deceased belonged to (AI.Affiliation.*), sourced from the dying ASC's owned tags.
    UPROPERTY(BlueprintReadWrite, Category = "Corpse")
    FGameplayTag Faction;

    // Living-world role of the deceased (from the cognitive brain), for role-specific raise results / flavour.
    UPROPERTY(BlueprintReadWrite, Category = "Corpse")
    FGameplayTag RoleTag;

    // Coarse kind of the deceased (AI.Kind.*), sourced from the dying ASC's owned tag. Drives skinnability:
    // AI.Kind.Creature ⇒ a skinnable body; absent/AI.Kind.Humanoid ⇒ not skinnable (the default → byte-identical
    // to the pre-hunting behaviour). Hunting F2.
    UPROPERTY(BlueprintReadWrite, Category = "Corpse")
    FGameplayTag SourceKind;

    // Combat tier of the source enemy (GetAITierInt: Normal=1..Boss=5; 0 = unknown/Normal). Scales decay lifetime.
    UPROPERTY(BlueprintReadWrite, Category = "Corpse")
    int32 SourceTier = 0;

    // Wave P (P4i): HOW the creature died — crit/burn/bleed/poison off the lethal blow's FMythicGameplayEffectContext,
    // overkill + hits-taken off the life component's damage bookkeeping. Skinning folds this + decomp stage into the
    // pelt/meat quality tier (FMythicSkinningRules::ResolveQuality). Defaults = "no information" (an un-stamped death
    // resolves like an ordinary kill — byte-identical yields until per-tier defs are authored).
    UPROPERTY(BlueprintReadWrite, Category = "Corpse")
    FMythicKillContext KillContext;
};

UCLASS()
class MYTHIC_API AMythicCorpse : public AActor, public IMythicInteractable, public IInventoryProviderInterface, public IMythicSaveableActor {
    GENERATED_BODY()

public:
    AMythicCorpse();

    void ServerInitializeFromDeath(const FMythicCorpseIdentity &Identity, int32 Tier, const FTransform &DeathTransform,
                                   UMythicInventoryComponent *ContentsToAbsorb = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Corpse")
    UMythicInventoryComponent *GetContainerInventory() const { return ContainerInventory; }

    UFUNCTION(BlueprintPure, Category = "Corpse")
    EMythicDecompStage GetDecompStage() const { return DecompStage; }

    // Combat tier of the source enemy (GetAITierInt: Normal=1..Boss=5; 0 = unknown/Normal). Necromancy scales the
    // raised minion's power off this — a public C++ getter for the otherwise-protected raise-substrate field.
    UFUNCTION(BlueprintPure, Category = "Corpse")
    int32 GetSourceTier() const { return SourceTier; }

    // Affiliation the deceased belonged to (AI.Affiliation.*). Exposed for necromancy flavour / faction handling.
    UFUNCTION(BlueprintPure, Category = "Corpse")
    FGameplayTag GetCorpseFaction() const { return Faction; }

    // True if the corpse can currently be raised by necromancy: config allows it, the stage is within
    // MaxRaisableStage, and it has not already been raised. The unit-tested substrate for the necromancy pillar.
    UFUNCTION(BlueprintPure, Category = "Corpse")
    bool CanBeRaised() const;

    // SERVER: consume the raise substrate (necromancy) — latches bAlreadyRaised so the body can't be raised twice.
    // Also clears the corpse's decomposition-hazard signal (a raised body no longer rots in place).
    UFUNCTION(BlueprintCallable, Category = "Corpse")
    void ServerMarkRaised();

    // SERVER: burn the corpse — clears its decomposition-hazard signal and destroys the body (fire cleanses the rot).
    // A concrete entry point for the "burn" arm of the emergent triangle; a BP pyre/torch ability calls this.
    UFUNCTION(BlueprintCallable, Category = "Corpse")
    void ServerBurnCorpse();

    // SERVER: mark the corpse looted — clears its decomposition-hazard signal (a picked-over body stops drawing the
    // sim's attention) while leaving the emptied husk in the world. The loot-completion flow calls this.
    UFUNCTION(BlueprintCallable, Category = "Corpse")
    void ServerNotifyLooted();

    // True once this corpse's raise substrate has been consumed (raised or consumed-for-heal). Lets necromancy skip a
    // spent body — a public C++ getter for the otherwise-protected latch.
    UFUNCTION(BlueprintPure, Category = "Corpse")
    bool IsAlreadyRaised() const { return bAlreadyRaised; }

    // ── Hunting F2: skinning substrate ──────────────────────────────────────────────────────────────────────────
    // Coarse kind of the source (AI.Kind.*, e.g. AI.Kind.Creature). Exposed for hunting/skinning + flavour.
    UFUNCTION(BlueprintPure, Category = "Corpse")
    FGameplayTag GetSourceKind() const { return SourceKind; }

    // True if this is a skinnable body (a creature corpse). INTRINSIC — does NOT account for the already-skinned latch;
    // the skin gate is IsSkinnable() && !IsSkinned(). Stamped once at ServerInitializeFromDeath from SourceKind.
    UFUNCTION(BlueprintPure, Category = "Corpse")
    bool IsSkinnable() const { return bSkinnable; }

    // True once this corpse's skin/meat has been harvested (latch — a body is skinned exactly once; mirrors bAlreadyRaised).
    UFUNCTION(BlueprintPure, Category = "Corpse")
    bool IsSkinned() const { return bSkinned; }

    // SERVER: consume the skinning yield — latches bSkinned so the body can't be skinned twice. Idempotent, no-op off
    // authority. Called by UMythicGA_SkinCorpse on channel completion (mirrors ServerMarkRaised).
    UFUNCTION(BlueprintCallable, Category = "Corpse")
    void ServerMarkSkinned();

    // Wave P (P4i): the death-stamped kill context (how this creature died) — the skinning quality resolve's input.
    UFUNCTION(BlueprintPure, Category = "Corpse")
    const FMythicKillContext &GetKillContext() const { return KillContext; }

    virtual TArray<UMythicInventoryComponent *> GetAllInventoryComponents() const override;
    virtual UAbilitySystemComponent *GetSchematicsASC() const override;

    virtual void OnPrimaryInteract_Implementation(AActor *Interactor) override;
    virtual void OnSecondaryInteract_Implementation(AActor *Interactor) override;
    virtual USceneComponent *GetWidgetAttachmentComponent_Implementation() const override;
    virtual bool GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const override;
    virtual void OnFocused_Implementation(AActor *Interactor) override;
    virtual void OnUnfocused_Implementation(AActor *Interactor) override;

    virtual void SerializeCustomData(TArray<uint8> &OutCustomData) override;
    virtual void DeserializeCustomData(const TArray<uint8> &InCustomData) override;

    bool IsActorInRange(const AActor *Actor) const;

    void Server_AddOpener(AMythicPlayerController *PC);
    bool Server_IsOpener(const AMythicPlayerController *PC) const;
    void Server_RemoveOpener(AMythicPlayerController *PC);

    bool Server_HasOpeners() const { return Openers.Num() > 0; }

    void Server_BeginChannelLock();
    void Server_EndChannelLock();
    bool Server_IsChannelLocked() const { return ActiveChannelRefs > 0; }

    bool AreHazardSignalsMuted() const { return bHazardSignalsMuted; }

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Corpse")
    USceneComponent *SceneRoot;

    // Query-only collision proxy so the corpse is detected by the interaction sweep (ECC_Visibility) even before a
    // BP assigns a visual mesh — keeps the raw C++ corpse interactable/lootable unauthored.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Corpse")
    USphereComponent *InteractionBounds;

    // Placeholder visual. A BP corpse swaps this (or a skeletal mesh) per DecompStage in OnRep_DecompStage.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Corpse")
    UStaticMeshComponent *CorpseMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    UMythicInventoryComponent *ContainerInventory;

    // Optional data-driven tuning; a null config uses the inline defaults below (runs unauthored).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Corpse")
    TObjectPtr<UMythicCorpseConfig> Config;

    // Interaction prompt data (same shape as the storage container).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    TObjectPtr<const UCommonGenericInputActionDataTable> InputActionDataTable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FName PrimaryInteractionName = FName("Search");

    // Offered as the corpse's SECONDARY interaction ONLY while it is skinnable (a creature body, not yet skinned). A
    // DataTable row of this name in InputActionDataTable supplies the prompt's icon/label. Hunting F2.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FName SkinInteractionName = FName("Skin");

    // Squared distance within which a player may open / loot. <= 0 disables the range gate. Mirrors the container.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Corpse")
    float ServerUseRangeSq = 250000.0f; // 500cm

    // Fired on the local interacting client so the BP can push the loot/container WBP (bind the player inventory VM
    // + GetContainerInventory()->GetViewModel()). Editor handoff, same as the storage container.
    UFUNCTION(BlueprintImplementableEvent, Category = "Corpse")
    void OnCorpseOpened(APlayerController *Interactor);

    // Cosmetic hook: the replicated DecompStage changed — a BP swaps the ragdoll/skeletal visual here (client + server).
    UFUNCTION(BlueprintImplementableEvent, Category = "Corpse")
    void OnDecompStageChanged(EMythicDecompStage NewStage);


    // Current decomposition stage (BP swaps visuals off it). ReplicatedUsing so clients fire the cosmetic hook.
    UPROPERTY(ReplicatedUsing = OnRep_DecompStage, VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Corpse")
    EMythicDecompStage DecompStage = EMythicDecompStage::Fresh;

    UFUNCTION()
    void OnRep_DecompStage();

    UPROPERTY(Replicated, SaveGame)
    float DeathTime = 0.0f;

    UPROPERTY(Replicated, SaveGame)
    uint32 SourceNameHash = 0;

    UPROPERTY(Replicated, SaveGame, BlueprintReadOnly, Category = "Corpse", meta = (AllowPrivateAccess = "true"))
    FGameplayTag Faction;

    UPROPERTY(Replicated, SaveGame, BlueprintReadOnly, Category = "Corpse", meta = (AllowPrivateAccess = "true"))
    FGameplayTag RoleTag;

    UPROPERTY(Replicated, SaveGame, BlueprintReadOnly, Category = "Corpse", meta = (AllowPrivateAccess = "true"))
    int32 SourceTier = 0;

    UPROPERTY(Replicated, SaveGame, BlueprintReadOnly, Category = "Corpse", meta = (AllowPrivateAccess = "true"))
    bool bRaisable = true;

    UPROPERTY(Replicated, SaveGame, BlueprintReadOnly, Category = "Corpse", meta = (AllowPrivateAccess = "true"))
    bool bAlreadyRaised = false;

    // Hunting F2: coarse kind of the source (AI.Kind.*), the skinnable discriminator (replicated for the client prompt,
    // SaveGame so a restored creature body stays skinnable).
    UPROPERTY(Replicated, SaveGame, BlueprintReadOnly, Category = "Corpse", meta = (AllowPrivateAccess = "true"))
    FGameplayTag SourceKind;

    // True if this is a creature corpse (SourceKind matches AI.Kind.Creature) → skinnable. Stamped once at init.
    UPROPERTY(Replicated, SaveGame, BlueprintReadOnly, Category = "Corpse", meta = (AllowPrivateAccess = "true"))
    bool bSkinnable = false;

    // Latch: this body's skin/meat has been harvested (skinned exactly once; mirrors bAlreadyRaised).
    UPROPERTY(Replicated, SaveGame, BlueprintReadOnly, Category = "Corpse", meta = (AllowPrivateAccess = "true"))
    bool bSkinned = false;

    // Wave P (P4i): the death-stamped kill context. Replicated (a client codex/inspect UI may read it) + SaveGame so a
    // restored body still resolves its earned quality. Corpses are SPAWNED fresh per death (never pooled), so no
    // pooled-reuse reset is needed HERE — the LifeComponent's per-pawn counters are what reset on pool reuse.
    UPROPERTY(Replicated, SaveGame, BlueprintReadOnly, Category = "Corpse", meta = (AllowPrivateAccess = "true"))
    FMythicKillContext KillContext;

    UPROPERTY(SaveGame)
    bool bHazardSignalsMuted = false;

    static class AController *ResolveController(AActor *Interactor);

private:
    float EffectiveDecayLifetime = 300.0f;
    TArray<float> EffectiveThresholds;
    EMythicDecompStage EffectiveMaxRaisableStage = EMythicDecompStage::Decayed;

    void ResolveEffectiveConfig();

    float GetAgeSeconds() const;

    void AdvanceDecay();
    FTimerHandle DecayTimerHandle;
    bool bDecayStarted = false;

    void SetDecompStage(EMythicDecompStage NewStage);

    void RegisterCorpseHazard();
    void UnregisterCorpseHazard();
    void MuteHazardSignals();
    bool bHazardRegistered = false;

    TSet<TWeakObjectPtr<AMythicPlayerController>> Openers;

    int32 ActiveChannelRefs = 0;

    void Server_SweepOpeners();
    FTimerHandle OpenerSweepHandle;
};
