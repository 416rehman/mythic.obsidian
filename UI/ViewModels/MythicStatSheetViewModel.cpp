// Copyright Stellar Games. All Rights Reserved.

#include "MythicStatSheetViewModel.h"
#include "Settings/MythicCombatSettings.h"
#include "GAS/MythicStatContribution.h"
#include "Itemization/InventoryProviderInterface.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/Fragments/Passive/AffixesFragment.h"
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/MythicAttributeSet.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

void UMythicStatSheetViewModel::InitializeForASC(UAbilitySystemComponent *InASC) {
    if (!InASC) {
        return;
    }
    Shutdown();
    ASC = InASC;

    for (UAttributeSet *Set : InASC->GetSpawnedAttributes()) {
        if (UMythicAttributeSet *MythicSet = Cast<UMythicAttributeSet>(Set)) {
            MythicSet->OnAttributeChanged.AddDynamic(this, &UMythicStatSheetViewModel::HandleAttributeChanged);
            BoundSets.Add(MythicSet);
        }
    }

    Rebuild();
}

void UMythicStatSheetViewModel::Shutdown() {
    for (const TWeakObjectPtr<UMythicAttributeSet> &WeakSet : BoundSets) {
        if (UMythicAttributeSet *Set = WeakSet.Get()) {
            Set->OnAttributeChanged.RemoveDynamic(this, &UMythicStatSheetViewModel::HandleAttributeChanged);
        }
    }
    BoundSets.Reset();
    ASC = nullptr;
    bRebuildScheduled = false;
}

void UMythicStatSheetViewModel::BeginDestroy() {
    Shutdown();
    Super::BeginDestroy();
}

void UMythicStatSheetViewModel::HandleAttributeChanged(const FGameplayAttribute &, float, float) {
    ScheduleRebuild();
}

void UMythicStatSheetViewModel::ScheduleRebuild() {
    if (bRebuildScheduled) {
        return;
    }

    UWorld *World = nullptr;
    if (const UAbilitySystemComponent *A = ASC.Get()) {
        if (const AActor *Owner = A->GetOwner()) {
            World = Owner->GetWorld();
        }
    }

    if (!World) {
        Rebuild();
        return;
    }

    bRebuildScheduled = true;
    World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UMythicStatSheetViewModel::Rebuild));
}

void UMythicStatSheetViewModel::Refresh() {
    Rebuild();
}

void UMythicStatSheetViewModel::SetShowUnmodified(bool bShow) {
    if (UE_MVVM_SET_PROPERTY_VALUE(bShowUnmodified, bShow)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bShowUnmodified);
        Rebuild();
    }
}

TArray<FMythicStatContributionLine> UMythicStatSheetViewModel::GetContributionsFor(FGameplayAttribute Stat) const {
    TArray<FMythicStatContributionLine> Out;

    const UAbilitySystemComponent *A = ASC.Get();
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    if (!A || !Settings || !Stat.IsValid()) {
        return Out;
    }

    const float StatValue = A->GetNumericAttribute(Stat);
    for (const FMythicStatContribution &Row : Settings->StatContributions.Contributions) {
        if (Row.SourceStat != Stat || !FMythicStatContributionRules::IsRowLive(Row)) {
            continue;
        }

        FMythicStatContributionLine Line;
        // Named by the same rule table that names it everywhere else on the sheet, so the tooltip and the
        // row below it cannot disagree about what a stat is called.
        Line.Label = FText::FromString(MythicStatDisplay::GetRule(Row.TargetAttribute).Label);
        if (Line.Label.IsEmpty()) {
            Line.Label = FText::FromString(MythicStatDisplay::MakeFriendlyLabel(Row.TargetAttribute.GetName()));
        }
        Line.Fraction = FMythicStatContributionRules::ResolveRow(Row, StatValue);
        Line.Value = MythicStatDisplay::FormatBonus(Line.Fraction, EMythicStatFormat::Percent);

        const float Undiminished = FMath::Max(0.0f, StatValue) * Row.PerPoint;
        Line.bDiminished = Undiminished - Line.Fraction > KINDA_SMALL_NUMBER;

        Out.Add(Line);
    }
    return Out;
}

void UMythicStatSheetViewModel::GatherGearContributions(const UAbilitySystemComponent *InASC,
                                                        TMap<FGameplayAttribute, float> &OutByAttribute) {
    OutByAttribute.Reset();
    if (!InASC) {
        return;
    }

    AActor *Candidates[] = {InASC->GetOwnerActor(), InASC->GetAvatarActor()};

    TSet<const UMythicInventoryComponent *> Seen;
    for (AActor *Owner : Candidates) {
        IInventoryProviderInterface *Provider = Owner ? Cast<IInventoryProviderInterface>(Owner) : nullptr;
        if (!Provider) {
            continue;
        }
        for (UMythicInventoryComponent *Inv : Provider->GetAllInventoryComponents()) {
            if (!Inv || Seen.Contains(Inv)) {
                continue;
            }
            Seen.Add(Inv);

            for (const FMythicInventorySlotEntry &Slot : Inv->GetAllSlots()) {
                if (!Slot.bEquipmentSlot || !Slot.SlottedItemInstance) {
                    continue;
                }
                const UAffixesFragment *Affixes = Slot.SlottedItemInstance->GetFragment<UAffixesFragment>();
                if (!Affixes) {
                    continue;
                }

                const TArray<FRolledAffix> *Groups[] = {&Affixes->AffixesRuntimeReplicatedData.RolledCoreAffixes,
                                                        &Affixes->AffixesRuntimeReplicatedData.RolledAffixes};
                for (const TArray<FRolledAffix> *Group : Groups) {
                    for (const FRolledAffix &Roll : *Group) {
                        if (!Roll.bIsApplied || !Roll.Attribute.IsValid()
                            || Roll.Definition.Modifier != EGameplayModOp::Additive) {
                            continue;
                        }
                        OutByAttribute.FindOrAdd(Roll.Attribute) += Roll.Value;
                    }
                }
            }
        }
    }
}

void UMythicStatSheetViewModel::Rebuild() {
    bRebuildScheduled = false;

    UAbilitySystemComponent *A = ASC.Get();
    if (!A) {
        SetSections({});
        SetModifiedStatCount(0);
        return;
    }

    const UMythicStatDisplaySettings *Settings = GetDefault<UMythicStatDisplaySettings>();
    const bool bHideInert = Settings ? Settings->bHideUnmodifiedZeroStats : true;

    TArray<FGameplayAttribute> AllAttributes;
    TMap<FString, FGameplayAttribute> ByName;
    for (UAttributeSet *Set : A->GetSpawnedAttributes()) {
        if (const UMythicAttributeSet *MythicSet = Cast<UMythicAttributeSet>(Set)) {
            TArray<FGameplayAttribute> SetAttributes;
            MythicSet->GetAttributes(SetAttributes);
            for (const FGameplayAttribute &Attr : SetAttributes) {
                if (Attr.IsValid()) {
                    AllAttributes.Add(Attr);
                    ByName.Add(Attr.GetName(), Attr);
                }
            }
        }
    }

    TMap<FGameplayAttribute, float> GearBonus;
    GatherGearContributions(A, GearBonus);

    TMap<EMythicStatCategory, TArray<FMythicStatLine>> Buckets;
    int32 ModifiedCount = 0;

    for (const FGameplayAttribute &Attr : AllAttributes) {
        const FMythicStatRule Rule = MythicStatDisplay::GetRule(Attr);
        if (Rule.bHidden || Rule.Category == EMythicStatCategory::Hidden) {
            continue;
        }

        FMythicStatLine Line;
        Line.Label = FText::FromString(Rule.Label);
        Line.Category = Rule.Category;
        Line.Format = Rule.Format;
        Line.SortOrder = Rule.SortOrder;
        Line.CurrentValue = A->GetNumericAttribute(Attr);
        Line.BaseValue = A->GetNumericAttributeBase(Attr);
        Line.BonusValue = (Line.CurrentValue - Line.BaseValue) + GearBonus.FindRef(Attr);
        Line.bHasBonus = !FMath::IsNearlyZero(Line.BonusValue, 0.001f);

        bool bIsPair = false;
        if (!Rule.MaxAttribute.IsEmpty()) {
            if (const FGameplayAttribute *MaxAttr = ByName.Find(Rule.MaxAttribute)) {
                const float MaxValue = A->GetNumericAttribute(*MaxAttr);
                if (MaxValue > 0.0f) {
                    bIsPair = true;
                    Line.BarPercent = FMath::Clamp(Line.CurrentValue / MaxValue, 0.0f, 1.0f);
                    Line.Value = FText::FromString(FString::Printf(
                        TEXT("%s / %s"),
                        *MythicStatDisplay::FormatValue(Line.CurrentValue, Rule.Format).ToString(),
                        *MythicStatDisplay::FormatValue(MaxValue, Rule.Format).ToString()));
                }
            }
        }

        if (!bIsPair) {
            Line.Value = MythicStatDisplay::FormatValue(Line.CurrentValue, Rule.Format);
        }

        Line.BonusText = MythicStatDisplay::FormatBonus(Line.BonusValue, Rule.Format);

        if (!bShowUnmodified && bHideInert && !bIsPair && !Line.bHasBonus) {
            const bool bInert = (Rule.Format == EMythicStatFormat::Multiplier)
                                    ? FMath::IsNearlyEqual(Line.CurrentValue, 1.0f, 0.001f)
                                    : FMath::IsNearlyZero(Line.CurrentValue, 0.001f);
            if (bInert) {
                continue;
            }
        }

        if (Line.bHasBonus) {
            ++ModifiedCount;
        }
        Buckets.FindOrAdd(Rule.Category).Add(MoveTemp(Line));
    }

    static const EMythicStatCategory Order[] = {
        EMythicStatCategory::Primary,
        EMythicStatCategory::Vitality,
        EMythicStatCategory::Offense,
        EMythicStatCategory::Defense,
        EMythicStatCategory::Utility,
        EMythicStatCategory::Survival,
        EMythicStatCategory::Proficiency};

    TArray<FMythicStatSection> NewSections;
    for (const EMythicStatCategory Category : Order) {
        TArray<FMythicStatLine> *Lines = Buckets.Find(Category);
        if (!Lines || Lines->Num() == 0) {
            continue;
        }
        Lines->Sort([](const FMythicStatLine &L, const FMythicStatLine &R) {
            return L.SortOrder != R.SortOrder ? L.SortOrder < R.SortOrder : L.Label.ToString() < R.Label.ToString();
        });

        FMythicStatSection Section;
        Section.Category = Category;
        Section.Heading = MythicStatDisplay::GetCategoryLabel(Category);
        Section.Lines = MoveTemp(*Lines);
        NewSections.Add(MoveTemp(Section));
    }

    SetSections(MoveTemp(NewSections));
    SetModifiedStatCount(ModifiedCount);
}

void UMythicStatSheetViewModel::SetSections(TArray<FMythicStatSection> &&In) {
    Sections = MoveTemp(In);
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Sections);
}

void UMythicStatSheetViewModel::SetModifiedStatCount(int32 In) {
    if (UE_MVVM_SET_PROPERTY_VALUE(ModifiedStatCount, In)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ModifiedStatCount);
    }
}
