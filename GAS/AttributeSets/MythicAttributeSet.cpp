#include "MythicAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UnrealType.h"

namespace {

FGameplayAttributeData *ResolveAttributeData(
    const FGameplayAttribute &Attribute, UAttributeSet &AttributeSet) {
    const FStructProperty *StructProperty =
        CastField<FStructProperty>(Attribute.GetUProperty());
    return StructProperty
        && StructProperty->Struct == FGameplayAttributeData::StaticStruct()
        ? StructProperty->ContainerPtrToValuePtr<FGameplayAttributeData>(
              &AttributeSet)
        : nullptr;
}

} // namespace

void UMythicAttributeSet::GetAttributes(TArray<FGameplayAttribute> &OutAttributes) const {
    this->GetAttributesFromSetClass(this->GetClass(), OutAttributes);
}

void UMythicAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    // Every DOREPLIFETIME in a derived set registers by FProperty::RepIndex, which the engine assigns lazily and
    // only from the replication path. A caller that reads lifetime props without going near that path - the
    // Gameplay Debugger does exactly this - finds every index still at 0, so the whole set collapses into one
    // entry, silently when the conditions agree and as an assert when they do not (#148).
    GetClass()->SetUpRuntimeReplicationData();

    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

TConstArrayView<FMythicBoundedAttributePair>
UMythicAttributeSet::GetBoundedAttributePairs() const {
    return {};
}

const FMythicBoundedAttributePair *
UMythicAttributeSet::FindBoundedPairByCurrent(
    const FGameplayAttribute &Attribute) const {
    for (const FMythicBoundedAttributePair &Pair :
         GetBoundedAttributePairs()) {
        if (Pair.CurrentAttribute == Attribute) {
            return &Pair;
        }
    }
    return nullptr;
}

const FMythicBoundedAttributePair *
UMythicAttributeSet::FindBoundedPairByMaximum(
    const FGameplayAttribute &Attribute) const {
    for (const FMythicBoundedAttributePair &Pair :
         GetBoundedAttributePairs()) {
        if (Pair.MaximumAttribute == Attribute) {
            return &Pair;
        }
    }
    return nullptr;
}

float UMythicAttributeSet::SanitizeFloor(const float Floor) {
    return FMath::IsFinite(Floor) ? Floor : 0.0f;
}

float UMythicAttributeSet::SanitizeMaximum(
    const FMythicBoundedAttributePair &Pair, const float Value) {
    const float MaximumFloor = SanitizeFloor(Pair.MaximumFloor);
    return FMath::IsFinite(Value)
        ? FMath::Max(Value, MaximumFloor)
        : MaximumFloor;
}

float UMythicAttributeSet::ReadEffectiveMaximum(
    const FMythicBoundedAttributePair &Pair) const {
    return SanitizeMaximum(
        Pair, Pair.MaximumAttribute.GetNumericValue(this));
}

void UMythicAttributeSet::PreAttributeChange(
    const FGameplayAttribute &Attribute, float &NewValue) {
    Super::PreAttributeChange(Attribute, NewValue);

    if (const FMythicBoundedAttributePair *Pair =
            FindBoundedPairByMaximum(Attribute)) {
        NewValue = SanitizeMaximum(*Pair, NewValue);
        return;
    }
    if (const FMythicBoundedAttributePair *Pair =
            FindBoundedPairByCurrent(Attribute)) {
        const float CurrentFloor = SanitizeFloor(Pair->CurrentFloor);
        const float Maximum =
            FMath::Max(ReadEffectiveMaximum(*Pair), CurrentFloor);
        NewValue = FMath::IsFinite(NewValue)
            ? FMath::Clamp(NewValue, CurrentFloor, Maximum)
            : CurrentFloor;
    }
}

void UMythicAttributeSet::PreAttributeBaseChange(
    const FGameplayAttribute &Attribute, float &NewValue) const {
    Super::PreAttributeBaseChange(Attribute, NewValue);

    if (const FMythicBoundedAttributePair *Pair =
            FindBoundedPairByMaximum(Attribute)) {
        NewValue = SanitizeMaximum(*Pair, NewValue);
        return;
    }
    if (const FMythicBoundedAttributePair *Pair =
            FindBoundedPairByCurrent(Attribute)) {
        const float CurrentFloor = SanitizeFloor(Pair->CurrentFloor);
        if (!FMath::IsFinite(NewValue)) {
            NewValue = CurrentFloor;
        }
        else if (Pair->BaseOverflowPolicy
                 == EMythicAttributeBaseOverflowPolicy::Discard) {
            const float Maximum =
                FMath::Max(ReadEffectiveMaximum(*Pair), CurrentFloor);
            NewValue = FMath::Clamp(NewValue, CurrentFloor, Maximum);
        }
        else {
            NewValue = FMath::Max(NewValue, CurrentFloor);
        }
    }
}

void UMythicAttributeSet::ReconcileBoundedAttribute(
    const FMythicBoundedAttributePair &Pair,
    const bool bMaximumChanged) {
    if (!Pair.CurrentAttribute.IsValid()
        || !Pair.MaximumAttribute.IsValid()
        || Pair.CurrentAttribute == Pair.MaximumAttribute) {
        ensureMsgf(false,
                   TEXT("%s has an invalid bounded attribute pair."),
                   *GetClass()->GetPathName());
        return;
    }

    const float CurrentFloor = SanitizeFloor(Pair.CurrentFloor);
    const float Maximum =
        FMath::Max(ReadEffectiveMaximum(Pair), CurrentFloor);
    const float Current = Pair.CurrentAttribute.GetNumericValue(this);
    const bool bCurrentInvalid = !FMath::IsFinite(Current)
        || Current < CurrentFloor - KINDA_SMALL_NUMBER
        || Current > Maximum + KINDA_SMALL_NUMBER;
    const bool bMustReevaluateDerivedCap = bMaximumChanged
        && Pair.BaseOverflowPolicy
               == EMythicAttributeBaseOverflowPolicy::Preserve;
    if (!bCurrentInvalid && !bMustReevaluateDerivedCap) {
        return;
    }

    UAbilitySystemComponent *AbilitySystem =
        GetOwningAbilitySystemComponent();
    if (AbilitySystem && AbilitySystem->IsOwnerActorAuthoritative()) {
        float BaseValue = AbilitySystem->GetNumericAttributeBase(
            Pair.CurrentAttribute);
        if (!FMath::IsFinite(BaseValue)) {
            BaseValue = CurrentFloor;
        }
        else if (Pair.BaseOverflowPolicy
                 == EMythicAttributeBaseOverflowPolicy::Discard) {
            BaseValue = FMath::Clamp(BaseValue, CurrentFloor, Maximum);
        }
        else {
            BaseValue = FMath::Max(BaseValue, CurrentFloor);
        }

        // Setting the authoritative base re-evaluates all active modifiers;
        // PreAttributeChange then clamps the effective value exactly once.
        AbilitySystem->SetNumericAttributeBase(
            Pair.CurrentAttribute, BaseValue);
    }
    else if (AbilitySystem) {
        // Replication/prediction may repair the evaluated client value, but
        // must never author or destroy a client-side base contribution.
        float LocalValue = FMath::IsFinite(Current)
            ? FMath::Clamp(Current, CurrentFloor, Maximum)
            : CurrentFloor;
        Pair.CurrentAttribute.SetNumericValueChecked(LocalValue, this);
    }
    else if (FGameplayAttributeData *Data =
                 ResolveAttributeData(Pair.CurrentAttribute, *this)) {
        if (Pair.BaseOverflowPolicy
            == EMythicAttributeBaseOverflowPolicy::Discard) {
            const float BaseValue = FMath::IsFinite(Data->GetBaseValue())
                ? FMath::Clamp(Data->GetBaseValue(), CurrentFloor,
                               Maximum)
                : CurrentFloor;
            Data->SetBaseValue(BaseValue);
        }
        const float LocalValue = FMath::IsFinite(Current)
            ? FMath::Clamp(Current, CurrentFloor, Maximum)
            : CurrentFloor;
        Data->SetCurrentValue(LocalValue);
    }

    ensureMsgf(
        Pair.CurrentAttribute.GetNumericValue(this)
            <= Maximum + KINDA_SMALL_NUMBER,
        TEXT("%s failed to cap %s to %s (%g > %g)."),
        *GetClass()->GetPathName(),
        *Pair.CurrentAttribute.GetName(),
        *Pair.MaximumAttribute.GetName(),
        Pair.CurrentAttribute.GetNumericValue(this), Maximum);
}

void UMythicAttributeSet::ReconcileAllBoundedAttributesInternal() {
    if (bReconcilingBoundedAttributes) {
        return;
    }

    TGuardValue<bool> ReentryGuard(bReconcilingBoundedAttributes, true);
    TSet<FGameplayAttribute> SeenCurrentAttributes;
    TSet<FGameplayAttribute> SeenMaximumAttributes;
    for (const FMythicBoundedAttributePair &Pair :
         GetBoundedAttributePairs()) {
        const bool bUnique =
            !SeenCurrentAttributes.Contains(Pair.CurrentAttribute)
            && !SeenMaximumAttributes.Contains(Pair.MaximumAttribute);
        if (!ensureMsgf(bUnique,
                        TEXT("%s declares a duplicate bounded attribute pair."),
                        *GetClass()->GetPathName())) {
            continue;
        }
        SeenCurrentAttributes.Add(Pair.CurrentAttribute);
        SeenMaximumAttributes.Add(Pair.MaximumAttribute);
        ReconcileBoundedAttribute(Pair, false);
    }
}

void UMythicAttributeSet::ReconcileAllBoundedAttributes() {
    ReconcileAllBoundedAttributesInternal();
}

void UMythicAttributeSet::InitFromMetaDataTable(
    const UDataTable *DataTable) {
    Super::InitFromMetaDataTable(DataTable);
    ReconcileAllBoundedAttributesInternal();
}

void UMythicAttributeSet::PostNetReceive() {
    Super::PostNetReceive();
    ReconcileAllBoundedAttributesInternal();
}

void UMythicAttributeSet::PostRepNotifies() {
    Super::PostRepNotifies();
    ReconcileAllBoundedAttributesInternal();
}

void UMythicAttributeSet::PostAttributeChange(const FGameplayAttribute &Attribute, float OldValue, float NewValue) {
    Super::PostAttributeChange(Attribute, OldValue, NewValue);

    if (!bReconcilingBoundedAttributes) {
        if (const FMythicBoundedAttributePair *Pair =
                FindBoundedPairByMaximum(Attribute)) {
            TGuardValue<bool> ReentryGuard(
                bReconcilingBoundedAttributes, true);
            ReconcileBoundedAttribute(*Pair, true);
        }
    }

    this->OnAttributeChanged.Broadcast(Attribute, OldValue, NewValue);
}
