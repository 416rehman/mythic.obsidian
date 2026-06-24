// 

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/MythicAttributeSet.h"
#include "MythicAttributeSet_Life.generated.h"

/**
 * What a would-be-lethal hit resolves to. Foundation for the co-op down/revive verb: with the down-state policy
 * enabled, a revivable pawn that isn't already downed enters a downed state instead of dying; otherwise the hit is
 * lethal as today. The downed STATE itself (replicated tag, bleed-out timer, revive interaction) is a follow-up
 * layer — this is the pure decision that gates it.
 */
UENUM(BlueprintType)
enum class EMythicLethalOutcome : uint8 {
    Survive,        // the hit was not lethal — normal damage, no death/down
    EnterDownState, // lethal, but co-op down is enabled + pawn is revivable + not already downed → go downed, not dead
    Die             // lethal → death (also the path for a non-revivable pawn, or finishing off an already-downed one)
};

/**
 * When Health gets changed by an effect, the following flow occurs:
 * (SERVER: PostGameplayEffectExecute) -> HealthChangeDelegate_Internal/MaxHealthChangeDelegate_Internal -> HealthComponent -> Broadcast to owner actor & Trigger(OnHit/OnDeath)GameplayEvent
 */
UCLASS()
class MYTHIC_API UMythicAttributeSet_Life : public UMythicAttributeSet {
    GENERATED_BODY()

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxHealth)
    FGameplayAttributeData MaxHealth;
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Health)
    FGameplayAttributeData Health;

    // Meta Attribute - NOT replicated, used for damage/healing calculations
    // Damage is applied to this, then converted to -Health in PostGameplayEffectExecute
    UPROPERTY(BlueprintReadOnly, Category = "Attributes")
    FGameplayAttributeData Damage;
    // Meta Attribute - NOT replicated, used for damage/healing calculations
    // Healing is applied to this, then converted to +Health in PostGameplayEffectExecute
    UPROPERTY(BlueprintReadOnly, Category = "Attributes")
    FGameplayAttributeData Healing;

    // Store the health before any changes are made by a gameplay effect
    float HealthBeforeAttributeChange;
    float MaxHealthBeforeAttributeChange;

    // Track if we're already dead to prevent re-triggering death
    bool bOutOfHealth = false;

public:
    UMythicAttributeSet_Life();

    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Life, MaxHealth);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Life, Health);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Life, Damage);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Life, Healing);

    UFUNCTION()
    virtual void OnRep_MaxHealth(const FGameplayAttributeData &OldMaxHealth);
    UFUNCTION()
    virtual void OnRep_Health(const FGameplayAttributeData &OldHealth);

    //~ Delegate when health changes due to damage/healing, some information may be missing on the client
    // Effect -> Delegate -> HealthComponent -> Broadcast to owner actor & Trigger(OnHit/OnDeath)GameplayEvent
    mutable FMythicAttributeEvent OnHealthChanged;
    mutable FMythicAttributeEvent OnMaxHealthChanged;
    //~
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    // PreAttributeChange is called just before any modification to attributes takes place.
    virtual void PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue) override;

    // PreGameplayEffectExecute is called just before a GE is executed. This can be used to modify the GE or cancel it.
    virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData &Data) override;
    void SendEventToInstigator(const FGameplayEffectModCallbackData &Data, AActor *Instigator, UAbilitySystemComponent *InstigatorASC,
                               UAbilitySystemComponent *OwnerASC,
                               FGameplayTag EventTag, float Magnitude);
    void SendEventToOwner(const FGameplayEffectModCallbackData &Data, UAbilitySystemComponent *OwnerASC, AActor *Instigator, FGameplayTag EventTag,
                          float Magnitude);
    // PostGameplayEffectExecute is called after a GE is executed. This can be used to show damage numbers, play sounds, etc.
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData &Data) override;

    // Clamp attributes to valid values
    virtual void ClampAttributes(const FGameplayAttribute &Attribute, float &NewValue);

    // Pure death-latch rule shared by every health-mutating path in PostGameplayEffectExecute: an entity is "out of
    // health" exactly when its health is at or below zero. The latch is SET when a hit/drain brings health to 0 and
    // CLEARED when any later change (heal or direct set) restores it above 0 — so the Damage, Healing AND direct-Health
    // paths must all derive it from the same rule. Static + pure so the transition (and the fix that a restoring heal
    // clears the latch) is unit-testable without a live ASC.
    static bool ComputeOutOfHealthLatch(float NewHealth);

    // Pure decision for what a would-be-lethal hit resolves to — the foundation of the co-op down/revive verb.
    // Parameterized over the down-state POLICY (a design/GameState flag, default OFF so behavior is UNCHANGED until
    // a designer enables it): when enabled, a revivable pawn (a player, not an NPC) that isn't already downed enters
    // the downed state instead of dying; a non-revivable pawn, or one already downed (finished off), dies. A
    // non-lethal hit always Survives. Static + pure so the gate is unit-testable without a live ASC. (The downed
    // state machine, bleed-out timer, and revive interaction are follow-up layers that consume this verdict.)
    static EMythicLethalOutcome ResolveLethalOutcome(bool bWouldBeLethal, bool bCoopDownStateEnabled, bool bAlreadyDowned, bool bRevivablePawn);

    // True once Health has hit zero (the server-authoritative death latch).
    UFUNCTION(BlueprintPure, Category = "Attributes")
    bool IsDead() const { return bOutOfHealth; }

    // SERVER: restore to full Health and clear the death latch. Call on respawn and on NPC pool reuse so a
    // previously-dead actor does not spawn already dead (bOutOfHealth otherwise sticks forever).
    void ResetForRespawn();
};
