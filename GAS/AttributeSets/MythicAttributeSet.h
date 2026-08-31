#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "MythicAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

struct FGameplayEffectSpec;
class UDataTable;

/** Determines whether an over-cap base value remains stored behind an evaluated maximum. */
enum class EMythicAttributeBaseOverflowPolicy : uint8 {
    /** Consumable, gauge, and progression overflow is discarded when its maximum shrinks. */
    Discard,

    /** Derived-stat investment remains stored while only the evaluated value is capped. */
    Preserve
};

/**
 * Native, strongly typed Current/Maximum contract used by GAS before any presentation or data registry is ready.
 * The attributes are explicit property references: runtime enforcement never infers a pair from a name, tag, or ID.
 */
struct MYTHIC_API FMythicBoundedAttributePair {
    FGameplayAttribute CurrentAttribute;
    FGameplayAttribute MaximumAttribute;
    float CurrentFloor = 0.0f;
    float MaximumFloor = 0.0f;
    EMythicAttributeBaseOverflowPolicy BaseOverflowPolicy =
        EMythicAttributeBaseOverflowPolicy::Discard;
};

DECLARE_MULTICAST_DELEGATE_SixParams(FMythicAttributeEvent, AActor*, AActor*, const FGameplayEffectSpec*,
                                     float, float, float);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMythicAttributeUpdateEvent, const FGameplayAttribute&, Attribute, float, OldValue, float, NewValue);

UCLASS(Abstract)
class MYTHIC_API UMythicAttributeSet : public UAttributeSet {
    GENERATED_BODY()

public:
    /** Broadcast after native invariants have produced a valid final attribute value. */
    UPROPERTY(BlueprintAssignable, Category = "AttributeSet")
    FMythicAttributeUpdateEvent OnAttributeChanged;

    /** Returns every Gameplay Attribute declared by this Attribute Set class. */
    UFUNCTION(BlueprintCallable, Category = "AttributeSet")
    void GetAttributes(TArray<FGameplayAttribute> &OutAttributes) const;

    /** Reconciles every explicit Current/Maximum pair after initialization, restore, or runtime set registration. */
    void ReconcileAllBoundedAttributes();

    virtual void InitFromMetaDataTable(const UDataTable *DataTable) override;

    virtual void PostNetReceive() override;

    virtual void PostRepNotifies() override;

    virtual void PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue) override;

    virtual void PreAttributeBaseChange(const FGameplayAttribute &Attribute, float &NewValue) const override;

    virtual void PostAttributeChange(const FGameplayAttribute &Attribute, float OldValue, float NewValue) override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    /** Returns this concrete set's bounded pairs. Derived sets must use generated typed attribute accessors. */
    virtual TConstArrayView<FMythicBoundedAttributePair> GetBoundedAttributePairs() const;

    const FMythicBoundedAttributePair *FindBoundedPairByCurrent(
        const FGameplayAttribute &Attribute) const;

    const FMythicBoundedAttributePair *FindBoundedPairByMaximum(
        const FGameplayAttribute &Attribute) const;

private:
    static float SanitizeFloor(float Floor);
    static float SanitizeMaximum(
        const FMythicBoundedAttributePair &Pair, float Value);
    float ReadEffectiveMaximum(
        const FMythicBoundedAttributePair &Pair) const;
    void ReconcileBoundedAttribute(
        const FMythicBoundedAttributePair &Pair,
        bool bMaximumChanged);
    void ReconcileAllBoundedAttributesInternal();

    bool bReconcilingBoundedAttributes = false;
};
