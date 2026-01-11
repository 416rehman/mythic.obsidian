// ConsumableEffectFragment.cpp

#include "ConsumableEffectFragment.h"
#include "ConsumableEffectFragment.h"
#include "AbilitySystemComponent.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Mythic/Mythic.h"
#include "UObject/UnrealType.h"
#include "GameplayAbilitySpec.h"
#include "GAS/Abilities/MythicGameplayAbility.h"

// Helper to access protected AttributeBasedMagnitude via reflection
static const FAttributeBasedFloat *GetAttributeBasedFloat(const FGameplayEffectModifierMagnitude &Magnitude) {
    static FStructProperty *Prop = nullptr;
    if (!Prop) {
        Prop = CastField<FStructProperty>(FGameplayEffectModifierMagnitude::StaticStruct()->FindPropertyByName(TEXT("AttributeBasedMagnitude")));
    }
    if (Prop) {
        return Prop->ContainerPtrToValuePtr<FAttributeBasedFloat>(&Magnitude);
    }
    return nullptr;
}

// ============================================================================
// FMythicEffectModifierDisplayData
// ============================================================================

FText FMythicEffectModifierDisplayData::GetAttributeDisplayName() const {
    if (!Attribute.IsValid()) {
        return FText::FromString(TEXT("Unknown"));
    }
    return FText::FromString(Attribute.GetName());
}

FText FMythicEffectModifierDisplayData::GetMagnitudeText() const {
    FString Sign = bIsPositive ? TEXT("+") : TEXT("");

    switch (MagnitudeSource) {
    case EMythicMagnitudeSource::AttributeBased: {
        // ARPG style: "+1 per Wisdom" or "+1 per 2 Wisdom"
        if (FMath::IsNearlyEqual(BackingCoefficient, 1.0f)) {
            return FText::Format(NSLOCTEXT("Mythic", "PerAttr", "{0}1 per {1}"),
                                 FText::FromString(Sign), FText::FromString(BackingAttribute.GetName()));
        }
        if (BackingCoefficient > 1.0f) {
            return FText::Format(NSLOCTEXT("Mythic", "PerAttrMult", "{0}{1} per {2}"),
                                 FText::FromString(Sign), FText::AsNumber(static_cast<int32>(BackingCoefficient)),
                                 FText::FromString(BackingAttribute.GetName()));
        }
        // Coefficient < 1: "+1 per 2 Wisdom"
        int32 Divisor = FMath::RoundToInt(1.0f / BackingCoefficient);
        return FText::Format(NSLOCTEXT("Mythic", "PerAttrDiv", "{0}1 per {1} {2}"),
                             FText::FromString(Sign), FText::AsNumber(Divisor),
                             FText::FromString(BackingAttribute.GetName()));
    }
    case EMythicMagnitudeSource::Custom:
        return FText::FromString(TEXT("?"));
    default:
        // Static or SetByCaller - known value
        if (bIsPercentage) {
            float DisplayValue = Magnitude;
            if (ModifierOp == EGameplayModOp::Multiplicitive ||
                ModifierOp == EGameplayModOp::MultiplyAdditive ||
                ModifierOp == EGameplayModOp::MultiplyCompound) {
                DisplayValue = (Magnitude - 1.0f) * 100.0f;
                Sign = DisplayValue >= 0 ? TEXT("+") : TEXT("");
            }
            else {
                DisplayValue = Magnitude * 100.0f;
            }
            return FText::FromString(FString::Printf(TEXT("%s%.0f%%"), *Sign, DisplayValue));
        }
        return FText::FromString(FString::Printf(TEXT("%s%.0f"), *Sign, Magnitude));
    }
}

FText FMythicEffectModifierDisplayData::GetFormattedText() const {
    return FText::Format(NSLOCTEXT("Mythic", "ModFmt", "{0} {1}"),
                         GetMagnitudeText(), GetAttributeDisplayName());
}

FText FMythicEffectModifierDisplayData::GetRichText() const {
    FString Tag = bIsPositive ? TEXT("Pos") : TEXT("Neg");
    return FText::FromString(FString::Printf(TEXT("<%s>%s %s</>"),
                                             *Tag, *GetMagnitudeText().ToString(), *GetAttributeDisplayName().ToString()));
}

FText FMythicEffectModifierDisplayData::GetMagnitudeText(UAbilitySystemComponent *ASC, bool bShowContext) const {
    if (MagnitudeSource != EMythicMagnitudeSource::AttributeBased || !ASC) {
        return GetMagnitudeText();
    }

    // Calculate actual value: coefficient × attribute value
    bool bFound = false;
    float AttrValue = ASC->GetGameplayAttributeValue(BackingAttribute, bFound);
    if (!bFound) {
        return GetMagnitudeText();
    }

    float CalculatedMag = BackingCoefficient * AttrValue;
    FString Sign = CalculatedMag >= 0 ? TEXT("+") : TEXT("");

    FText ContextText = FText::GetEmpty();
    if (bShowContext) {
        // Format: <Context>(Coefficient * Attribute)</>
        // e.g. <Context>(0.5 * Wisdom)</>
        ContextText = FText::Format(NSLOCTEXT("Mythic", "ScalingContext", " <Context>({0} * {1})</>"),
                                    FText::AsNumber(BackingCoefficient),
                                    FText::FromString(BackingAttribute.GetName()));
    }

    if (bIsPercentage) {
        return FText::Format(NSLOCTEXT("Mythic", "CalcPercent", "{0}{1}%{2}"),
                             FText::FromString(Sign),
                             FText::AsNumber(static_cast<int32>(CalculatedMag * 100.0f)),
                             ContextText);
    }
    return FText::Format(NSLOCTEXT("Mythic", "CalcFlat", "{0}{1}{2}"),
                         FText::FromString(Sign),
                         FText::AsNumber(static_cast<int32>(CalculatedMag)),
                         ContextText);
}

FText FMythicEffectModifierDisplayData::GetRichText(UAbilitySystemComponent *ASC, bool bShowContext) const {
    FString Tag = bIsPositive ? TEXT("Pos") : TEXT("Neg");
    return FText::FromString(FString::Printf(TEXT("<%s>%s %s</>"),
                                             *Tag, *GetMagnitudeText(ASC, bShowContext).ToString(), *GetAttributeDisplayName().ToString()));
}

// ============================================================================
// FMythicEffectDurationDisplayData
// ============================================================================

FText FMythicEffectDurationDisplayData::GetFormattedText() const {
    switch (DurationType) {
    case EGameplayEffectDurationType::Instant:
        return FText::GetEmpty();
    case EGameplayEffectDurationType::Infinite:
        return NSLOCTEXT("Mythic", "Perm", "permanent");
    case EGameplayEffectDurationType::HasDuration:
        if (DurationSource == EMythicMagnitudeSource::AttributeBased) {
            return FText::Format(NSLOCTEXT("Mythic", "DurAttr", "duration scales with {0}"),
                                 FText::FromString(DurationBackingAttribute.GetName()));
        }
        if (IsPeriodic()) {
            return FText::Format(NSLOCTEXT("Mythic", "Periodic", "every <Val>{0}s</> for <Val>{1}s</>"),
                                 FText::AsNumber(Period), FText::AsNumber(Duration));
        }
        return FText::Format(NSLOCTEXT("Mythic", "Timed", "for <Val>{0}s</>"), FText::AsNumber(Duration));
    default:
        return FText::GetEmpty();
    }
}

FText FMythicEffectDurationDisplayData::GetRichText() const {
    FText Dur = GetFormattedText();
    if (Dur.IsEmpty()) {
        return Dur;
    }
    return Dur;
}

FText FMythicEffectDurationDisplayData::GetFormattedText(UAbilitySystemComponent *ASC, bool bShowContext) const {
    if (DurationType != EGameplayEffectDurationType::HasDuration) {
        return GetFormattedText();
    }
    if (DurationSource != EMythicMagnitudeSource::AttributeBased || !ASC) {
        return GetFormattedText();
    }

    // Calculate actual duration: coefficient × attribute value
    bool bFound = false;
    float AttrValue = ASC->GetGameplayAttributeValue(DurationBackingAttribute, bFound);
    if (!bFound) {
        return GetFormattedText();
    }

    float CalcDuration = DurationBackingCoefficient * AttrValue;

    FText ContextText = FText::GetEmpty();
    if (bShowContext) {
        // Format: <Context>(Coefficient * Attribute)</>
        ContextText = FText::Format(NSLOCTEXT("Mythic", "ScalingContext", " <Context>({0} * {1})</>"),
                                    FText::AsNumber(DurationBackingCoefficient),
                                    FText::FromString(DurationBackingAttribute.GetName()));
    }

    if (IsPeriodic()) {
        return FText::Format(NSLOCTEXT("Mythic", "PeriodicCalc", "every <Val>{0}s</> for <Val>{1}s</>{2}"),
                             FText::AsNumber(Period), FText::AsNumber(static_cast<int32>(CalcDuration)),
                             ContextText);
    }
    return FText::Format(NSLOCTEXT("Mythic", "TimedCalc", "for <Val>{0}s</>{1}"),
                         FText::AsNumber(static_cast<int32>(CalcDuration)),
                         ContextText);
}

FText FMythicEffectDurationDisplayData::GetRichText(UAbilitySystemComponent *ASC, bool bShowContext) const {
    FText Dur = GetFormattedText(ASC, bShowContext);
    if (Dur.IsEmpty()) {
        return Dur;
    }
    return Dur;
}

// ============================================================================
// FMythicEffectStackingDisplayData
// ============================================================================

FText FMythicEffectStackingDisplayData::GetFormattedText() const {
    if (!CanStack()) {
        return FText::GetEmpty();
    }
    if (StackLimitCount <= 0) {
        return NSLOCTEXT("Mythic", "StacksUnlim", "stacks infinitely");
    }
    return FText::Format(NSLOCTEXT("Mythic", "Stacks", "stacks {0}x"), FText::AsNumber(StackLimitCount));
}

FText FMythicEffectStackingDisplayData::GetRichText() const {
    FText St = GetFormattedText();
    if (St.IsEmpty()) {
        return St;
    }
    return FText::FromString(FString::Printf(TEXT("<img id=\"Stack\"/> <Stack>%s</>"), *St.ToString()));
}

// ============================================================================
// FMythicEffectDisplayData
// ============================================================================

FText FMythicEffectDisplayData::GetRichText() const {
    TArray<FString> Parts;

    // Modifiers
    for (const auto &Mod : Modifiers) {
        Parts.Add(Mod.GetRichText().ToString());
    }

    FString Result = FString::Join(Parts, TEXT(", "));

    // Duration
    FText Dur = Duration.GetRichText();
    if (!Dur.IsEmpty()) {
        Result += TEXT(" ") + Dur.ToString();
    }

    // Stacking
    FText St = Stacking.GetRichText();
    if (!St.IsEmpty()) {
        Result += TEXT(" ") + St.ToString();
    }

    return FText::FromString(Result);
}

FText FMythicEffectDisplayData::GetRichText(UAbilitySystemComponent *ASC, bool bShowContext) const {
    TArray<FString> Parts;

    // Modifiers with calculated values
    for (const auto &Mod : Modifiers) {
        Parts.Add(Mod.GetRichText(ASC, bShowContext).ToString());
    }

    FString Result = FString::Join(Parts, TEXT(", "));

    // Duration with calculated values
    FText Dur = Duration.GetRichText(ASC, bShowContext);
    if (!Dur.IsEmpty()) {
        Result += TEXT(" ") + Dur.ToString();
    }

    // Stacking
    FText St = Stacking.GetRichText();
    if (!St.IsEmpty()) {
        Result += TEXT(" ") + St.ToString();
    }

    return FText::FromString(Result);
}

// ============================================================================
// UConsumableEffectFragment - Lifecycle
// ============================================================================

void UConsumableEffectFragment::OnInstanced(UMythicItemInstance *ItemInstance) {
    Super::OnInstanced(ItemInstance);
    ConsumableEffectRuntimeReplicatedData.ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ItemInstance->GetInventoryOwner());

    // Grant Generic Ability to handle Input if InputTag is set
    // Using nullptr for AbilityClass triggers the fallback to DefaultItemInputAbility in helper
    if (InputTag.IsValid()) {
        if (UMythicAbilitySystemComponent *MythicASC = Cast<UMythicAbilitySystemComponent>(ConsumableEffectRuntimeReplicatedData.ASC)) {
            ConsumableEffectRuntimeReplicatedData.AbilityHandle = GrantItemAbility(MythicASC, ItemInstance, nullptr);
            if (!ConsumableEffectRuntimeReplicatedData.AbilityHandle.IsValid()) {
                UE_LOG(Myth, Error, TEXT("UConsumableEffectFragment::OnInstanced: Failed to grant generic ability for input handling."));
            }
        }
    }
}

void UConsumableEffectFragment::ExecuteGenericAction(UMythicItemInstance *ItemInstance) {
    ServerApplyEffects(ItemInstance);
}

void UConsumableEffectFragment::OnInventorySlotChanged(UMythicInventoryComponent *Inventory, int32 SlotIndex) {
    Super::OnInventorySlotChanged(Inventory, SlotIndex);
    ConsumableEffectRuntimeReplicatedData.ASC = Inventory
        ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Inventory->GetOwner())
        : nullptr;
}

void UConsumableEffectFragment::OnItemDeactivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemDeactivated(ItemInstance);
    if (ConsumableEffectRuntimeReplicatedData.ASC && ConsumableEffectRuntimeReplicatedData.AbilityHandle.IsValid()) {
        ConsumableEffectRuntimeReplicatedData.ASC->ClearAbility(ConsumableEffectRuntimeReplicatedData.AbilityHandle);
    }
}

void UConsumableEffectFragment::OnClientActionEnd(UMythicItemInstance *ItemInstance) {
    ServerApplyEffects(ItemInstance);
    if (auto Inventory = GetOwningInventoryComponent()) {
        Inventory->ServerRemoveItem(ParentItemInstance, 1);
    }
}

void UConsumableEffectFragment::ServerApplyEffects_Implementation(UMythicItemInstance *ItemInstance) {
    auto ASC = ConsumableEffectRuntimeReplicatedData.ASC;
    if (!ASC) {
        ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ItemInstance->GetInventoryOwner());
    }
    if (!ASC) {
        UE_LOG(Myth, Error, TEXT("UConsumableEffectFragment: No ASC"));
        return;
    }

    for (const FConsumableEffectEntry &Entry : ConsumableEffectConfig.Effects) {
        if (!Entry.GameplayEffectClass) {
            UE_LOG(Myth, Warning, TEXT("UConsumableEffectFragment: Invalid GameplayEffectClass in config"));
            continue;
        }

        FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
        Context.AddSourceObject(this);

        FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Entry.GameplayEffectClass, Entry.EffectLevel, Context);
        if (Spec.IsValid()) {
            // Apply SetByCaller magnitudes
            for (const auto &Pair : Entry.SetByCallerMagnitudes) {
                Spec.Data->SetSetByCallerMagnitude(Pair.Key, Pair.Value);
            }

            ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
            UE_LOG(Myth, Log, TEXT("Applied effect: %s"), *Entry.GameplayEffectClass->GetName());
        }
        else {
            UE_LOG(Myth, Error, TEXT("UConsumableEffectFragment: Failed to make outgoing spec for effect: %s"), *Entry.GameplayEffectClass->GetName());
        }
    }
}

bool UConsumableEffectFragment::CanBeStackedWith(const UItemFragment *Other) const {
    auto OtherFrag = Cast<UConsumableEffectFragment>(Other);
    if (!OtherFrag) {
        return false;
    }

    if (ConsumableEffectConfig.Effects.Num() != OtherFrag->ConsumableEffectConfig.Effects.Num()) {
        return false;
    }

    for (int32 i = 0; i < ConsumableEffectConfig.Effects.Num(); ++i) {
        const auto &A = ConsumableEffectConfig.Effects[i];
        const auto &B = OtherFrag->ConsumableEffectConfig.Effects[i];

        if (A.GameplayEffectClass != B.GameplayEffectClass || A.EffectLevel != B.EffectLevel) {
            return false;
        }
        if (A.SetByCallerMagnitudes.Num() != B.SetByCallerMagnitudes.Num()) {
            return false;
        }
        for (const auto &Pair : A.SetByCallerMagnitudes) {
            const float *OtherVal = B.SetByCallerMagnitudes.Find(Pair.Key);
            if (!OtherVal || *OtherVal != Pair.Value) {
                return false;
            }
        }
    }
    return Super::CanBeStackedWith(Other);
}

// ============================================================================
// GE Parsing
// ============================================================================

FMythicEffectModifierDisplayData UConsumableEffectFragment::ParseModifier(
    const FGameplayModifierInfo &Modifier,
    const TMap<FGameplayTag, float> &SetByCallerValues) {

    FMythicEffectModifierDisplayData Data;
    Data.Attribute = Modifier.Attribute;
    Data.ModifierOp = Modifier.ModifierOp;

    Data.bIsPercentage = (Modifier.ModifierOp == EGameplayModOp::Multiplicitive ||
        Modifier.ModifierOp == EGameplayModOp::MultiplyAdditive ||
        Modifier.ModifierOp == EGameplayModOp::MultiplyCompound ||
        Modifier.ModifierOp == EGameplayModOp::Division);

    // Determine magnitude source
    EGameplayEffectMagnitudeCalculation CalcType = Modifier.ModifierMagnitude.GetMagnitudeCalculationType();

    switch (CalcType) {
    case EGameplayEffectMagnitudeCalculation::ScalableFloat:
        Data.MagnitudeSource = EMythicMagnitudeSource::Static;
        Modifier.ModifierMagnitude.GetStaticMagnitudeIfPossible(1.0f, Data.Magnitude);
        break;

    case EGameplayEffectMagnitudeCalculation::SetByCaller: {
        Data.MagnitudeSource = EMythicMagnitudeSource::SetByCaller;
        const FSetByCallerFloat &SBC = Modifier.ModifierMagnitude.GetSetByCallerFloat();
        Data.SetByCallerTag = SBC.DataTag;
        const float *Val = SetByCallerValues.Find(SBC.DataTag);
        Data.Magnitude = Val ? *Val : 0.0f;
        break;
    }
    case EGameplayEffectMagnitudeCalculation::AttributeBased: {
        Data.MagnitudeSource = EMythicMagnitudeSource::AttributeBased;
        if (const FAttributeBasedFloat *ABF = GetAttributeBasedFloat(Modifier.ModifierMagnitude)) {
            Data.BackingAttribute = ABF->BackingAttribute.AttributeToCapture;
            Data.BackingCoefficient = ABF->Coefficient.GetValueAtLevel(1.0f);
        }
        break;
    }
    case EGameplayEffectMagnitudeCalculation::CustomCalculationClass:
        Data.MagnitudeSource = EMythicMagnitudeSource::Custom;
        break;
    }

    // Determine positive
    Data.bIsPositive = Data.bIsPercentage ? (Data.Magnitude >= 1.0f) : (Data.Magnitude >= 0.0f);
    if (Data.MagnitudeSource == EMythicMagnitudeSource::AttributeBased) {
        Data.bIsPositive = Data.BackingCoefficient >= 0.0f;
    }

    return Data;
}

FMythicEffectDurationDisplayData UConsumableEffectFragment::ParseDuration(const UGameplayEffect *GE, const TMap<FGameplayTag, float> &SetByCallerValues) {

    FMythicEffectDurationDisplayData Data;
    Data.DurationType = GE->DurationPolicy;
    Data.Period = GE->Period.GetValueAtLevel(1.0f);

    if (GE->DurationPolicy == EGameplayEffectDurationType::HasDuration) {
        EGameplayEffectMagnitudeCalculation CalcType = GE->DurationMagnitude.GetMagnitudeCalculationType();

        switch (CalcType) {
        case EGameplayEffectMagnitudeCalculation::ScalableFloat:
            Data.DurationSource = EMythicMagnitudeSource::Static;
            GE->DurationMagnitude.GetStaticMagnitudeIfPossible(1.0f, Data.Duration);
            break;
        case EGameplayEffectMagnitudeCalculation::SetByCaller: {
            Data.DurationSource = EMythicMagnitudeSource::SetByCaller;
            // Lookup duration from the entered SetByCaller values
            const FSetByCallerFloat &SBC = GE->DurationMagnitude.GetSetByCallerFloat();
            const float *Val = SetByCallerValues.Find(SBC.DataTag);
            Data.Duration = Val ? *Val : 0.0f;
            break;
        }
        case EGameplayEffectMagnitudeCalculation::AttributeBased: {
            Data.DurationSource = EMythicMagnitudeSource::AttributeBased;
            TArray<FGameplayEffectAttributeCaptureDefinition> CaptureDefs;
            GE->DurationMagnitude.GetAttributeCaptureDefinitions(CaptureDefs);
            if (CaptureDefs.Num() > 0) {
                Data.DurationBackingAttribute = CaptureDefs[0].AttributeToCapture;
            }
            break;
        }
        default:
            Data.DurationSource = EMythicMagnitudeSource::Custom;
            break;
        }
    }
    else if (GE->DurationPolicy == EGameplayEffectDurationType::Infinite) {
        Data.Duration = -1.0f;
    }

    return Data;
}

FMythicEffectStackingDisplayData UConsumableEffectFragment::ParseStacking(const UGameplayEffect *GE) {
    FMythicEffectStackingDisplayData Data;
    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    Data.StackingType = GE->StackingType;
    PRAGMA_ENABLE_DEPRECATION_WARNINGS
    Data.StackLimitCount = GE->StackLimitCount;
    return Data;
}

FMythicEffectDisplayData UConsumableEffectFragment::GetEffectDisplayDataFromEntry(const FConsumableEffectEntry &Entry) {
    FMythicEffectDisplayData Result;
    if (!Entry.GameplayEffectClass) {
        return Result;
    }

    const UGameplayEffect *GE = Entry.GameplayEffectClass.GetDefaultObject();

    // Check for display override
    if (!Entry.DisplayTextOverride.IsEmpty()) {
        // Just return empty with the override accessible via the entry
        return Result;
    }

    for (const FGameplayModifierInfo &Mod : GE->Modifiers) {
        Result.Modifiers.Add(ParseModifier(Mod, Entry.SetByCallerMagnitudes));
    }

    Result.Duration = ParseDuration(GE, Entry.SetByCallerMagnitudes);
    Result.Stacking = ParseStacking(GE);

    return Result;
}

FMythicEffectDisplayData UConsumableEffectFragment::GetEffectDisplayData(TSubclassOf<UGameplayEffect> EffectClass) {
    FConsumableEffectEntry Entry;
    Entry.GameplayEffectClass = EffectClass;
    return GetEffectDisplayDataFromEntry(Entry);
}

FMythicEffectDisplayData UConsumableEffectFragment::GetPrimaryEffectDisplayData() const {
    if (ConsumableEffectConfig.Effects.Num() > 0) {
        return GetEffectDisplayDataFromEntry(ConsumableEffectConfig.Effects[0]);
    }
    return FMythicEffectDisplayData();
}

TArray<FMythicEffectDisplayData> UConsumableEffectFragment::GetAllEffectDisplayData() const {
    TArray<FMythicEffectDisplayData> Result;
    for (const FConsumableEffectEntry &Entry : ConsumableEffectConfig.Effects) {
        Result.Add(GetEffectDisplayDataFromEntry(Entry));
    }
    return Result;
}

FText UConsumableEffectFragment::GetCombinedRichText() const {
    TArray<FString> Lines;
    for (const FConsumableEffectEntry &Entry : ConsumableEffectConfig.Effects) {
        if (!Entry.DisplayTextOverride.IsEmpty()) {
            Lines.Add(Entry.DisplayTextOverride.ToString());
        }
        else {
            auto Data = GetEffectDisplayDataFromEntry(Entry);
            if (ConsumableEffectRuntimeReplicatedData.ASC) {
                Lines.Add(Data.GetRichText(ConsumableEffectRuntimeReplicatedData.ASC).ToString());
            }
            else {
                Lines.Add(Data.GetRichText().ToString());
            }
        }
    }
    return FText::FromString(FString::Join(Lines, TEXT("\n")));
}
