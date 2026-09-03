// Copyright Stellar Games. All Rights Reserved.

#include "MythicPlayerStatusViewModel.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GAS/Effects/MythicStatusEffectDefinition.h"
#include "GAS/Effects/MythicStatusRegistry.h"
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/MythicTags_GAS.h"
#include "GameFramework/Actor.h"
#include "Progression/Runes/MythicRuneDefinition.h"

void UMythicPlayerStatusViewModel::InitializeForASC(UAbilitySystemComponent *InASC) {
    if (!InASC) {
        return;
    }
    Unbind();
    ASC = InASC;

    CurHealth = InASC->GetNumericAttribute(UMythicAttributeSet_Life::GetHealthAttribute());
    MaxHealthV = InASC->GetNumericAttribute(UMythicAttributeSet_Life::GetMaxHealthAttribute());
    CurStamina = InASC->GetNumericAttribute(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute());
    MaxStaminaV = InASC->GetNumericAttribute(UMythicAttributeSet_Utility::GetMaxStaminaAttribute());
    CurShield = InASC->GetNumericAttribute(UMythicAttributeSet_Defense::GetShieldAttribute());
    MaxShieldV = InASC->GetNumericAttribute(UMythicAttributeSet_Defense::GetMaxShieldAttribute());
    SetHealthPercent(MaxHealthV > 0.0f ? FMath::Clamp(CurHealth / MaxHealthV, 0.0f, 1.0f) : 0.0f);
    SetStaminaPercent(MaxStaminaV > 0.0f ? FMath::Clamp(CurStamina / MaxStaminaV, 0.0f, 1.0f) : 0.0f);
    SetShieldPercent(MaxShieldV > 0.0f ? FMath::Clamp(CurShield / MaxShieldV, 0.0f, 1.0f) : 0.0f);

    SetInCombat(InASC->HasMatchingGameplayTag(GAS_STATE_INCOMBAT));
    SetExhausted(InASC->HasMatchingGameplayTag(GAS_STATE_EXHAUSTED));

    InASC->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Life::GetHealthAttribute()).AddUObject(this, &UMythicPlayerStatusViewModel::HandleAttributeChanged);
    InASC->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Life::GetMaxHealthAttribute()).AddUObject(this, &UMythicPlayerStatusViewModel::HandleAttributeChanged);
    InASC->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute()).AddUObject(this, &UMythicPlayerStatusViewModel::HandleAttributeChanged);
    InASC->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Utility::GetMaxStaminaAttribute()).AddUObject(this, &UMythicPlayerStatusViewModel::HandleAttributeChanged);
    InASC->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Defense::GetShieldAttribute()).AddUObject(this, &UMythicPlayerStatusViewModel::HandleAttributeChanged);
    InASC->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Defense::GetMaxShieldAttribute()).AddUObject(this, &UMythicPlayerStatusViewModel::HandleAttributeChanged);

    const FGameplayTag StatusTags[] = {GAS_STATE_INCOMBAT, GAS_STATE_EXHAUSTED};
    for (const FGameplayTag &Tag : StatusTags) {
        if (Tag.IsValid()) {
            InASC->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &UMythicPlayerStatusViewModel::HandleTagChanged);
        }
    }

    BuildStatusBadges(InASC);

    const AActor *OwnerActor = InASC->GetOwnerActor();
    BindToRunes(OwnerActor ? OwnerActor->FindComponentByClass<UMythicRuneComponent>() : nullptr);
}

void UMythicPlayerStatusViewModel::BindToRunes(UMythicRuneComponent *InRunes) {
    UnbindRunes();
    Runes = InRunes;
    if (InRunes) {
        InRunes->OnRunesChanged.AddUniqueDynamic(this, &UMythicPlayerStatusViewModel::HandleRuneBadgesDirty);
        InRunes->OnRuneHudStateChanged.AddUniqueDynamic(this, &UMythicPlayerStatusViewModel::HandleRuneBadgesDirty);
    }
    RebuildRuneBadges();
}

void UMythicPlayerStatusViewModel::UnbindRunes() {
    if (UMythicRuneComponent *R = Runes.Get()) {
        R->OnRunesChanged.RemoveDynamic(this, &UMythicPlayerStatusViewModel::HandleRuneBadgesDirty);
        R->OnRuneHudStateChanged.RemoveDynamic(this, &UMythicPlayerStatusViewModel::HandleRuneBadgesDirty);
    }
    Runes = nullptr;
}

void UMythicPlayerStatusViewModel::HandleRuneBadgesDirty() {
    RebuildRuneBadges();
}

void UMythicPlayerStatusViewModel::RebuildRuneBadges() {
    RuneBadges.Reset();
    if (const UMythicRuneComponent *R = Runes.Get()) {
        const UWorld *World = R->GetWorld();
        // Server clocks land as local world seconds so the view compares against the clock it already ticks with.
        const double Offset = World ? World->GetTimeSeconds() - R->GetServerWorldTimeSeconds() : 0.0;
        for (const FMythicRuneHudStateItem &Item : R->GetRuneHudStates()) {
            if (Item.State == EMythicRuneHudState::Hidden) {
                continue;
            }
            const UMythicRuneDefinition *Rune = R->GetRuneInSlot(Item.SlotIndex);
            if (!Rune) {
                continue;
            }
            FMythicRuneBadgeEntry &Entry = RuneBadges.AddDefaulted_GetRef();
            Entry.SlotIndex = Item.SlotIndex;
            Entry.Icon = Rune->GetHudIconOrIcon();
            Entry.Name = Rune->Name;
            Entry.State = Item.State;
            Entry.StartTimeSeconds = Item.ServerStartTimeSeconds + Offset;
            Entry.EndTimeSeconds = Item.ServerEndTimeSeconds > 0.0 ? Item.ServerEndTimeSeconds + Offset : 0.0;
            Entry.Stacks = Item.Stacks;
        }
        RuneBadges.Sort([](const FMythicRuneBadgeEntry &A, const FMythicRuneBadgeEntry &B) { return A.SlotIndex < B.SlotIndex; });
    }
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(RuneBadges);
}

void UMythicPlayerStatusViewModel::BuildStatusBadges(UAbilitySystemComponent *InASC) {
    StatusBadges.Reset();
    BadgeBoundTags.Reset();
    BadgeBoundAttributes.Reset();

    const UWorld *World = InASC ? InASC->GetWorld() : nullptr;
    const UGameInstance *GameInstance = World ? World->GetGameInstance() : nullptr;
    const UMythicStatusRegistry *Registry = GameInstance ? GameInstance->GetSubsystem<UMythicStatusRegistry>() : nullptr;
    if (!Registry) {
        return;
    }

    TArray<UMythicStatusEffectDefinition *> Definitions = Registry->GetAllStatuses();
    // The registry returns map order, which is not stable between runs, and a badge row must not reshuffle.
    Definitions.Sort([](const UMythicStatusEffectDefinition &A, const UMythicStatusEffectDefinition &B) {
        return A.StatusType.ToString() < B.StatusType.ToString();
    });

    for (const UMythicStatusEffectDefinition *Definition : Definitions) {
        if (!Definition || !Definition->StatusType.IsValid()) {
            continue;
        }
        FMythicStatusBadgeEntry Entry;
        Entry.StatusType = Definition->StatusType;
        Entry.GrantedStateTag = Definition->GrantedStateTag;
        Entry.DisplayName = Definition->DisplayName;
        Entry.Description = Definition->Description;
        Entry.Icon = Definition->Icon;
        Entry.DisplayColor = Definition->DisplayColor;
        Entry.bActive = Definition->GrantedStateTag.IsValid() && InASC->HasMatchingGameplayTag(Definition->GrantedStateTag);
        Entry.BuildupFraction = ComputeBuildupFraction(InASC, Definition);
        StatusBadges.Add(Entry);

        if (Definition->GrantedStateTag.IsValid()) {
            BadgeBoundTags.Add(Definition->GrantedStateTag);
            InASC->RegisterGameplayTagEvent(Definition->GrantedStateTag, EGameplayTagEventType::NewOrRemoved)
                 .AddUObject(this, &UMythicPlayerStatusViewModel::HandleBadgeTagChanged);
        }
        if (Definition->BuildupAttribute.IsValid()) {
            BadgeBoundAttributes.Add(Definition->BuildupAttribute);
            InASC->GetGameplayAttributeValueChangeDelegate(Definition->BuildupAttribute)
                 .AddUObject(this, &UMythicPlayerStatusViewModel::HandleBadgeAttributeChanged);
        }
    }

    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(StatusBadges);
}

float UMythicPlayerStatusViewModel::ComputeBuildupFraction(const UAbilitySystemComponent *InASC, const UMythicStatusEffectDefinition *Definition) {
    if (!InASC || !Definition || !Definition->BuildupAttribute.IsValid()) {
        return 0.0f;
    }
    // No attacker in a self-readout, so nothing reduces the threshold. Resistance does not belong here: it gates
    // whether a proc lands at all, one layer down, and never moved this number.
    const float Threshold = UMythicAttributeSet_Defense::ComputeBuildupThreshold(0.0f);
    if (Threshold <= 0.0f) {
        return 0.0f;
    }
    return FMath::Clamp(InASC->GetNumericAttribute(Definition->BuildupAttribute) / Threshold, 0.0f, 1.0f);
}

void UMythicPlayerStatusViewModel::HandleBadgeTagChanged(const FGameplayTag Tag, int32 NewCount) {
    bool bChanged = false;
    for (FMythicStatusBadgeEntry &Entry : StatusBadges) {
        if (Entry.GrantedStateTag == Tag && Entry.bActive != (NewCount > 0)) {
            Entry.bActive = NewCount > 0;
            bChanged = true;
        }
    }
    if (bChanged) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(StatusBadges);
    }
}

void UMythicPlayerStatusViewModel::HandleBadgeAttributeChanged(const FOnAttributeChangeData &Data) {
    const UAbilitySystemComponent *A = ASC.Get();
    const UWorld *World = A ? A->GetWorld() : nullptr;
    const UGameInstance *GameInstance = World ? World->GetGameInstance() : nullptr;
    const UMythicStatusRegistry *Registry = GameInstance ? GameInstance->GetSubsystem<UMythicStatusRegistry>() : nullptr;
    if (!Registry) {
        return;
    }
    const UMythicStatusEffectDefinition *Definition = Registry->FindStatusByBuildupAttribute(Data.Attribute);
    if (!Definition) {
        return;
    }
    const float NewFraction = ComputeBuildupFraction(A, Definition);
    bool bChanged = false;
    for (FMythicStatusBadgeEntry &Entry : StatusBadges) {
        if (Entry.StatusType == Definition->StatusType && !FMath::IsNearlyEqual(Entry.BuildupFraction, NewFraction)) {
            Entry.BuildupFraction = NewFraction;
            bChanged = true;
        }
    }
    if (bChanged) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(StatusBadges);
    }
}

void UMythicPlayerStatusViewModel::HandleAttributeChanged(const FOnAttributeChangeData &Data) {
    const FGameplayAttribute &A = Data.Attribute;
    if (A == UMythicAttributeSet_Life::GetHealthAttribute()) {
        const float Old = CurHealth;
        CurHealth = Data.NewValue;
        SetHealthPercent(MaxHealthV > 0.0f ? FMath::Clamp(CurHealth / MaxHealthV, 0.0f, 1.0f) : 0.0f);
        if (Data.NewValue < Old) {
            OnHealthDamaged.Broadcast((Old - Data.NewValue) / FMath::Max(1.0f, MaxHealthV), HealthPercent);
        }
    }
    else if (A == UMythicAttributeSet_Life::GetMaxHealthAttribute()) {
        MaxHealthV = Data.NewValue;
        SetHealthPercent(MaxHealthV > 0.0f ? FMath::Clamp(CurHealth / MaxHealthV, 0.0f, 1.0f) : 0.0f);
    }
    else if (A == UMythicAttributeSet_Utility::GetCurrentStaminaAttribute()) {
        CurStamina = Data.NewValue;
        SetStaminaPercent(MaxStaminaV > 0.0f ? FMath::Clamp(CurStamina / MaxStaminaV, 0.0f, 1.0f) : 0.0f);
    }
    else if (A == UMythicAttributeSet_Utility::GetMaxStaminaAttribute()) {
        MaxStaminaV = Data.NewValue;
        SetStaminaPercent(MaxStaminaV > 0.0f ? FMath::Clamp(CurStamina / MaxStaminaV, 0.0f, 1.0f) : 0.0f);
    }
    else if (A == UMythicAttributeSet_Defense::GetShieldAttribute()) {
        CurShield = Data.NewValue;
        SetShieldPercent(MaxShieldV > 0.0f ? FMath::Clamp(CurShield / MaxShieldV, 0.0f, 1.0f) : 0.0f);
    }
    else if (A == UMythicAttributeSet_Defense::GetMaxShieldAttribute()) {
        MaxShieldV = Data.NewValue;
        SetShieldPercent(MaxShieldV > 0.0f ? FMath::Clamp(CurShield / MaxShieldV, 0.0f, 1.0f) : 0.0f);
    }
}

void UMythicPlayerStatusViewModel::HandleTagChanged(const FGameplayTag Tag, int32 NewCount) {
    const bool bOn = NewCount > 0;
    if (Tag == GAS_STATE_INCOMBAT) {
        SetInCombat(bOn);
    }
    else if (Tag == GAS_STATE_EXHAUSTED) {
        SetExhausted(bOn);
    }
}

void UMythicPlayerStatusViewModel::Unbind() {
    UnbindRunes();
    UAbilitySystemComponent *A = ASC.Get();
    if (!A) {
        return;
    }
    A->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Life::GetHealthAttribute()).RemoveAll(this);
    A->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Life::GetMaxHealthAttribute()).RemoveAll(this);
    A->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute()).RemoveAll(this);
    A->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Utility::GetMaxStaminaAttribute()).RemoveAll(this);
    A->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Defense::GetShieldAttribute()).RemoveAll(this);
    A->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Defense::GetMaxShieldAttribute()).RemoveAll(this);

    const FGameplayTag StatusTags[] = {GAS_STATE_INCOMBAT, GAS_STATE_EXHAUSTED};
    for (const FGameplayTag &Tag : StatusTags) {
        if (Tag.IsValid()) {
            A->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
        }
    }
    for (const FGameplayTag &Tag : BadgeBoundTags) {
        A->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
    }
    for (const FGameplayAttribute &Attribute : BadgeBoundAttributes) {
        A->GetGameplayAttributeValueChangeDelegate(Attribute).RemoveAll(this);
    }
    BadgeBoundTags.Reset();
    BadgeBoundAttributes.Reset();

    ASC = nullptr;
}

void UMythicPlayerStatusViewModel::BeginDestroy() {
    Unbind();
    Super::BeginDestroy();
}

void UMythicPlayerStatusViewModel::SetHealthPercent(float V) {
    if (UE_MVVM_SET_PROPERTY_VALUE(HealthPercent, V)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(HealthPercent);
    }
}
void UMythicPlayerStatusViewModel::SetStaminaPercent(float V) {
    if (UE_MVVM_SET_PROPERTY_VALUE(StaminaPercent, V)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(StaminaPercent);
    }
}
void UMythicPlayerStatusViewModel::SetShieldPercent(float V) {
    if (UE_MVVM_SET_PROPERTY_VALUE(ShieldPercent, V)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ShieldPercent);
    }
}
void UMythicPlayerStatusViewModel::SetInCombat(bool V) {
    if (UE_MVVM_SET_PROPERTY_VALUE(bInCombat, V)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bInCombat);
    }
}
void UMythicPlayerStatusViewModel::SetExhausted(bool V) {
    if (UE_MVVM_SET_PROPERTY_VALUE(bExhausted, V)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bExhausted);
    }
}
