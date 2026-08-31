
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/MythicAttributeSet.h"
#include "MythicAttributeSet_Life.generated.h"

class UMythicStatusEffectDefinition;

UENUM(BlueprintType)
enum class EMythicLethalOutcome : uint8 {
    Survive,
    EnterDownState,
    Die
};

UCLASS()
class MYTHIC_API UMythicAttributeSet_Life : public UMythicAttributeSet {
    GENERATED_BODY()

protected:
    /** Maximum health replicated to clients and available to Blueprint UI bindings. */
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxHealth)
    FGameplayAttributeData MaxHealth;

    /** Current health replicated to clients and available to Blueprint UI bindings. */
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Health)
    FGameplayAttributeData Health;

    /** Non-replicated damage meta attribute converted into a negative Health delta after combat resolution. */
    UPROPERTY(BlueprintReadOnly, Category = "Attributes")
    FGameplayAttributeData Damage;

    /** Non-replicated healing meta attribute converted into a positive Health delta after resolution. */
    UPROPERTY(BlueprintReadOnly, Category = "Attributes")
    FGameplayAttributeData Healing;

    float HealthBeforeAttributeChange;
    float MaxHealthBeforeAttributeChange;

    bool bOutOfHealth = false;

    bool bInDeathPreHook = false;

    virtual TConstArrayView<FMythicBoundedAttributePair> GetBoundedAttributePairs() const override;

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

    mutable FMythicAttributeEvent OnHealthChanged;
    mutable FMythicAttributeEvent OnMaxHealthChanged;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData &Data) override;
    // Tags describing the hit itself, beyond the source's own. Read from the Mythic effect context, which is
    // where the damage execution records what the hit turned out to be.
    static void AppendHitTags(const FGameplayEffectContextHandle &Context, FGameplayTagContainer &OutTags);

    void SendEventToInstigator(const FGameplayEffectModCallbackData &Data, AActor *Instigator, UAbilitySystemComponent *InstigatorASC,
                               UAbilitySystemComponent *OwnerASC,
                               FGameplayTag EventTag, float Magnitude);
    void SendEventToOwner(const FGameplayEffectModCallbackData &Data, UAbilitySystemComponent *OwnerASC, AActor *Instigator, FGameplayTag EventTag,
                          float Magnitude);
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData &Data) override;

    static bool ComputeOutOfHealthLatch(float NewHealth);

    /** Returns the finite, positive health amount actually removed after clamping. */
    static float ResolveAppliedHealthDamage(float OldHealth, float NewHealth);

    /** Returns the finite, positive shield amount actually absorbed without rounding it for transport. */
    static float ResolveAppliedShieldDamage(float RawShieldAbsorbed);

    /** True only when an authoritative resolution represents a finite, positive renderable magnitude. */
    static bool ShouldEmitResolvedCombatText(float ResolvedMagnitude, bool bAuthoritative);

    /** Routes an outgoing copy only when the source has a distinct owning viewer. */
    static bool ShouldRouteResolvedCombatTextToSource(bool bHasSourceViewer, bool bSourceIsTargetViewer);

    /** Routes an incoming copy whenever the damaged target has an owning viewer. */
    static bool ShouldRouteResolvedCombatTextToTarget(bool bHasTargetViewer);

    /**
     * Resolves the exact canonical status Data Asset carried by a periodic effect context. Returns null for direct
     * damage, arbitrary periodic effects, and contexts that do not carry a typed status definition.
     */
    static const UMythicStatusEffectDefinition *ResolvePeriodicStatusDefinition(
        float Period, const FGameplayEffectContextHandle &Context);

    static EMythicLethalOutcome ResolveLethalOutcome(bool bWouldBeLethal, bool bCoopDownStateEnabled, bool bAlreadyDowned, bool bRevivablePawn);

    /** Returns true once Health has reached zero and the server-authoritative death latch is set. */
    UFUNCTION(BlueprintPure, Category = "Attributes")
    bool IsDead() const { return bOutOfHealth; }

    void ResetForRespawn();

    /**
     * Re-derives the out-of-health latch from current health. Anything that restores health with
     * SetNumericAttributeBase must call this: that path skips PostGameplayEffectExecute, so the latch set on
     * death stays set and every later hit is zeroed.
     */
    void RefreshOutOfHealthLatch();
};
