#include "Itemization/Affixes/MythicPermanentStatLedger.h"

#include "AbilitySystemComponent.h"

namespace {
bool GuidLexicalLess(const FGuid &Left, const FGuid &Right) {
    if (Left.A != Right.A) return Left.A < Right.A;
    if (Left.B != Right.B) return Left.B < Right.B;
    if (Left.C != Right.C) return Left.C < Right.C;
    return Left.D < Right.D;
}

bool ContributionLess(const FMythicPermanentStatContribution &Left,
                      const FMythicPermanentStatContribution &Right) {
    if (Left.Layer != Right.Layer) {
        return Left.Layer < Right.Layer;
    }
    if (Left.SourceGuid != Right.SourceGuid) {
        return GuidLexicalLess(Left.SourceGuid, Right.SourceGuid);
    }
    return Left.ModifierOp.GetValue() < Right.ModifierOp.GetValue();
}

FString AttributeSortKey(const FGameplayAttribute &Attribute) {
    const UClass *AttributeSetClass = Attribute.GetAttributeSetClass();
    return FString::Printf(TEXT("%s|%s"),
                           AttributeSetClass ? *AttributeSetClass->GetPathName() : TEXT("<invalid>"),
                           *Attribute.GetName());
}

bool BaseValuesEqual(const float Left, const float Right) {
    const float Scale = FMath::Max3(1.0f, FMath::Abs(Left), FMath::Abs(Right));
    return FMath::IsNearlyEqual(Left, Right, UE_KINDA_SMALL_NUMBER * Scale);
}

struct FStagedAttributeWrite {
    FGameplayAttribute Attribute;
    float ActualBefore = 0.0f;
    float Target = 0.0f;
};
}

bool FMythicPermanentStatLedger::Compose(
    const float ExternalBase,
    const TConstArrayView<FMythicPermanentStatContribution> Contributions,
    float &OutComposedBase) {
    if (!FMath::IsFinite(ExternalBase)) {
        return false;
    }

    TArray<FMythicPermanentStatContribution> Ordered;
    Ordered.Append(Contributions.GetData(), Contributions.Num());
    Ordered.Sort(ContributionLess);

    // GAS chooses the first qualifying Override in its deterministic modifier order. Our permanent layer has no tag
    // qualification, so stable source identity order supplies the equivalent deterministic winner.
    for (const FMythicPermanentStatContribution &Contribution : Ordered) {
        if (!FMath::IsFinite(Contribution.Magnitude)) {
            return false;
        }
        if (Contribution.ModifierOp == EGameplayModOp::Override) {
            OutComposedBase = Contribution.Magnitude;
            return FMath::IsFinite(OutComposedBase);
        }
    }

    float AddBase = 0.0f;
    float MultiplyAdditive = 1.0f;
    float DivideAdditive = 1.0f;
    float MultiplyCompound = 1.0f;
    float AddFinal = 0.0f;

    for (const FMythicPermanentStatContribution &Contribution : Ordered) {
        const float Magnitude = Contribution.Magnitude;
        if (!FMath::IsFinite(Magnitude)) {
            return false;
        }
        switch (Contribution.ModifierOp.GetValue()) {
        case EGameplayModOp::AddBase:
            AddBase += Magnitude;
            break;
        case EGameplayModOp::MultiplyAdditive:
            // GameplayEffectUtilities::GetModifierBiasByModifierOp supplies a bias of one: N multipliers aggregate
            // as 1 + Sum(Magnitude - 1), not as a product.
            MultiplyAdditive += Magnitude - 1.0f;
            break;
        case EGameplayModOp::DivideAdditive:
            DivideAdditive += Magnitude - 1.0f;
            break;
        case EGameplayModOp::MultiplyCompound:
            MultiplyCompound *= Magnitude;
            break;
        case EGameplayModOp::AddFinal:
            AddFinal += Magnitude;
            break;
        case EGameplayModOp::Override:
            checkNoEntry(); // handled by the deterministic winner pass above
            break;
        default:
            return false;
        }
        if (!FMath::IsFinite(AddBase) || !FMath::IsFinite(MultiplyAdditive)
            || !FMath::IsFinite(DivideAdditive) || !FMath::IsFinite(MultiplyCompound)
            || !FMath::IsFinite(AddFinal)) {
            return false;
        }
    }

    // Match FAggregatorModChannel::EvaluateWithBase: a near-zero divisor is replaced with one.
    if (FMath::IsNearlyZero(DivideAdditive)) {
        DivideAdditive = 1.0f;
    }
    OutComposedBase = ((ExternalBase + AddBase) * MultiplyAdditive / DivideAdditive
                       * MultiplyCompound) + AddFinal;
    return FMath::IsFinite(OutComposedBase);
}

bool FMythicPermanentStatLedger::ReconcileTransactional(
    UAbilitySystemComponent &AbilitySystem,
    const TConstArrayView<FMythicPermanentStatContribution> DesiredContributions,
    FMythicPermanentStatReconcileResult &OutResult) {
    OutResult = FMythicPermanentStatReconcileResult();
    if (!AbilitySystem.IsOwnerActorAuthoritative()) {
        OutResult.Error = TEXT("Permanent affix stats can only be reconciled by an authoritative ASC.");
        return false;
    }
    if (BoundAbilitySystem.IsValid() && BoundAbilitySystem.Get() != &AbilitySystem && !Attributes.IsEmpty()) {
        OutResult.Error = TEXT("Permanent stat ledger is still bound to a different ASC.");
        return false;
    }

    TMap<FGameplayAttribute, TArray<FMythicPermanentStatContribution>> DesiredByAttribute;
    TSet<FGuid> ContributionIdentities;
    for (const FMythicPermanentStatContribution &Contribution : DesiredContributions) {
        if (!Contribution.SourceGuid.IsValid() || !Contribution.Attribute.IsValid()
            || !FMath::IsFinite(Contribution.Magnitude)
            || Contribution.ModifierOp.GetValue() < EGameplayModOp::AddBase
            || Contribution.ModifierOp.GetValue() >= EGameplayModOp::Max
            || (Contribution.Layer != EMythicPermanentStatContributionLayer::Progression
                && Contribution.Layer != EMythicPermanentStatContributionLayer::Equipment)
            || !AbilitySystem.HasAttributeSetForAttribute(Contribution.Attribute)) {
            OutResult.Error = TEXT("Desired permanent contribution has an invalid identity, attribute, operation, or magnitude.");
            return false;
        }

        if (ContributionIdentities.Contains(Contribution.SourceGuid)) {
            OutResult.Error = FString::Printf(TEXT("Duplicate permanent contribution identity %s."),
                                              *Contribution.SourceGuid.ToString());
            return false;
        }
        ContributionIdentities.Add(Contribution.SourceGuid);
        DesiredByAttribute.FindOrAdd(Contribution.Attribute).Add(Contribution);
    }
    for (TPair<FGameplayAttribute, TArray<FMythicPermanentStatContribution>> &Pair : DesiredByAttribute) {
        Pair.Value.Sort(ContributionLess);
    }

    TSet<FGameplayAttribute> TouchedAttributes;
    for (const TPair<FGameplayAttribute, FAttributeState> &Pair : Attributes) {
        TouchedAttributes.Add(Pair.Key);
    }
    for (const TPair<FGameplayAttribute, TArray<FMythicPermanentStatContribution>> &Pair : DesiredByAttribute) {
        TouchedAttributes.Add(Pair.Key);
    }

    TArray<FGameplayAttribute> OrderedAttributes = TouchedAttributes.Array();
    OrderedAttributes.Sort([](const FGameplayAttribute &Left, const FGameplayAttribute &Right) {
        return AttributeSortKey(Left) < AttributeSortKey(Right);
    });

    TMap<FGameplayAttribute, FAttributeState> StagedAttributes;
    TArray<FStagedAttributeWrite> Writes;
    Writes.Reserve(OrderedAttributes.Num());
    for (const FGameplayAttribute &Attribute : OrderedAttributes) {
        if (!Attribute.IsValid() || !AbilitySystem.HasAttributeSetForAttribute(Attribute)) {
            OutResult.Error = FString::Printf(TEXT("ASC no longer owns permanent affix attribute %s."),
                                              *Attribute.GetName());
            return false;
        }

        const float ActualBase = AbilitySystem.GetNumericAttributeBase(Attribute);
        if (!FMath::IsFinite(ActualBase)) {
            OutResult.Error = FString::Printf(TEXT("Attribute %s has a non-finite GAS base."),
                                              *Attribute.GetName());
            return false;
        }

        float BaselineBase = ActualBase;
        if (const FAttributeState *Existing = Attributes.Find(Attribute)) {
            if (!BaseValuesEqual(ActualBase, Existing->LastComposedBase)) {
                OutResult.Error = FString::Printf(
                    TEXT("Tracked permanent stat %s was changed outside its typed source ledger."),
                    *Attribute.GetName());
                return false;
            }
            BaselineBase = Existing->BaselineBase;
        }
        if (!FMath::IsFinite(BaselineBase)) {
            OutResult.Error = FString::Printf(TEXT("Permanent baseline tracking overflowed for attribute %s."),
                                              *Attribute.GetName());
            return false;
        }

        const TArray<FMythicPermanentStatContribution> *Desired = DesiredByAttribute.Find(Attribute);
        if (Desired && !Desired->IsEmpty()) {
            FAttributeState &Staged = StagedAttributes.Add(Attribute);
            Staged.BaselineBase = BaselineBase;
            Staged.Contributions = *Desired;
            TArray<FMythicPermanentStatContribution> ProgressionContributions;
            TArray<FMythicPermanentStatContribution> EquipmentContributions;
            for (const FMythicPermanentStatContribution &Contribution : Staged.Contributions) {
                (Contribution.Layer == EMythicPermanentStatContributionLayer::Progression
                     ? ProgressionContributions : EquipmentContributions).Add(Contribution);
            }
            if (!Compose(BaselineBase, ProgressionContributions, Staged.NonEquipmentBase)
                || !Compose(Staged.NonEquipmentBase, EquipmentContributions,
                            Staged.LastComposedBase)) {
                OutResult.Error = FString::Printf(TEXT("Permanent contribution composition failed for attribute %s."),
                                                  *Attribute.GetName());
                return false;
            }
            Writes.Add(FStagedAttributeWrite{Attribute, ActualBase, Staged.LastComposedBase});
        }
        else {
            // Removing the last typed source restores the separately tracked pre-source baseline.
            Writes.Add(FStagedAttributeWrite{Attribute, ActualBase, BaselineBase});
        }
    }

    bool bWriteSucceeded = true;
    for (int32 Index = 0; Index < Writes.Num(); ++Index) {
        const FStagedAttributeWrite &Write = Writes[Index];
        if (BaseValuesEqual(Write.ActualBefore, Write.Target)) {
            continue;
        }
        AbilitySystem.SetNumericAttributeBase(Write.Attribute, Write.Target);
        if (!BaseValuesEqual(AbilitySystem.GetNumericAttributeBase(Write.Attribute), Write.Target)) {
            OutResult.Error = FString::Printf(TEXT("GAS rejected/clamped permanent base write for attribute %s."),
                                              *Write.Attribute.GetName());
            bWriteSucceeded = false;
            break;
        }
    }

    // Attribute callbacks may change an attribute written earlier in the transaction. Verify the whole write set,
    // not only each immediate SetNumericAttributeBase result.
    if (bWriteSucceeded) {
        for (const FStagedAttributeWrite &Write : Writes) {
            if (!BaseValuesEqual(AbilitySystem.GetNumericAttributeBase(Write.Attribute), Write.Target)) {
                OutResult.Error = FString::Printf(
                    TEXT("A GAS callback overwrote permanent base attribute %s during reconciliation."),
                    *Write.Attribute.GetName());
                bWriteSucceeded = false;
                break;
            }
        }
    }

    if (!bWriteSucceeded) {
        // A callback from one attempted write may have modified any other touched attribute, including one whose
        // target originally matched its base and therefore did not need a direct write. Restore the whole closure.
        for (int32 WriteIndex = Writes.Num() - 1; WriteIndex >= 0; --WriteIndex) {
            const FStagedAttributeWrite &Write = Writes[WriteIndex];
            AbilitySystem.SetNumericAttributeBase(Write.Attribute, Write.ActualBefore);
        }
        for (const FStagedAttributeWrite &Write : Writes) {
            if (!BaseValuesEqual(AbilitySystem.GetNumericAttributeBase(Write.Attribute), Write.ActualBefore)) {
                OutResult.bRollbackSucceeded = false;
            }
        }
        return false;
    }

    Attributes = MoveTemp(StagedAttributes);
    BoundAbilitySystem = Attributes.IsEmpty() ? nullptr : &AbilitySystem;
    OutResult.bSucceeded = true;
    return true;
}

bool FMythicPermanentStatLedger::ClearTransactional(
    UAbilitySystemComponent &AbilitySystem,
    FMythicPermanentStatReconcileResult &OutResult) {
    return ReconcileTransactional(AbilitySystem, {}, OutResult);
}

void FMythicPermanentStatLedger::Abandon() {
    BoundAbilitySystem.Reset();
    Attributes.Reset();
}

void FMythicPermanentStatLedger::GetLayerSnapshots(
    TArray<FMythicPermanentStatLayerSnapshot> &OutSnapshots) const {
    OutSnapshots.Reset(Attributes.Num());
    for (const TPair<FGameplayAttribute, FAttributeState> &Pair : Attributes) {
        FMythicPermanentStatLayerSnapshot &Snapshot = OutSnapshots.AddDefaulted_GetRef();
        Snapshot.Attribute = Pair.Key;
        Snapshot.NonEquipmentBase = Pair.Value.NonEquipmentBase;
        Snapshot.EquipmentBase = Pair.Value.LastComposedBase;
    }
    OutSnapshots.Sort([](const FMythicPermanentStatLayerSnapshot &Left,
                         const FMythicPermanentStatLayerSnapshot &Right) {
        return AttributeSortKey(Left.Attribute) < AttributeSortKey(Right.Attribute);
    });
}
