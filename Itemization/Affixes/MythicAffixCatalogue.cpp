#include "MythicAffixCatalogue.h"

#include "Mythic/Mythic.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "Mythic"

namespace {
bool IsRollable(const FMythicAffixCatalogueEntry &Entry) {
    return Entry.Def.Attribute.IsValid() && Entry.Def.Tiers.Num() > 0;
}

bool AppliesTo(const FMythicTieredAffixDef &Def, const FGameplayTagContainer &TypeProbe) {
    return Def.Applicability.IsEmpty() || Def.Applicability.Matches(TypeProbe);
}

int32 AppendDefsById(TConstArrayView<FMythicAffixCatalogueEntry> Entries, TConstArrayView<FName> Ids,
                     const FGameplayTagContainer *TypeProbe, TArray<FMythicTieredAffixDef> &Out,
                     TArray<bool> *OutAppended = nullptr) {
    int32 Appended = 0;
    for (const FName Id : Ids) {
        const int32 Index = FMythicAffixCatalogueMath::FindEntryIndex(Entries, Id);
        if (Index == INDEX_NONE) {
            UE_LOG(Myth, Warning, TEXT("Affix catalogue has no entry '%s'; the rule naming it rolls one affix fewer"),
                   *Id.ToString());
            continue;
        }
        const FMythicTieredAffixDef &Def = Entries[Index].Def;
        if (!IsRollable(Entries[Index])) {
            UE_LOG(Myth, Warning, TEXT("Affix '%s' has no attribute or no tiers, so it cannot roll"),
                   *Id.ToString());
            continue;
        }
        if (TypeProbe && !AppliesTo(Def, *TypeProbe)) {
            continue;
        }
        Out.Add(Def);
        if (OutAppended) {
            (*OutAppended)[Index] = true;
        }
        ++Appended;
    }
    return Appended;
}
}

const FMythicAffixCatalogueEntry *UMythicAffixCatalogue::FindEntry(FName AffixId) const {
    const int32 Index = FMythicAffixCatalogueMath::FindEntryIndex(Entries, AffixId);
    return (Index == INDEX_NONE) ? nullptr : &Entries[Index];
}

const FMythicItemTypeAffixRule *UMythicAffixCatalogue::ResolveRule(const FGameplayTag &ItemType) const {
    const int32 Index = FMythicAffixCatalogueMath::ResolveRuleIndex(RulesByItemType, ItemType);
    return (Index == INDEX_NONE) ? nullptr : &RulesByItemType[Index];
}

int32 UMythicAffixCatalogue::BuildCoreDefs(const FGameplayTag &ItemType, const FGameplayTagContainer &TypeProbe,
                                           TArray<FMythicTieredAffixDef> &Out) const {
    TArray<int32> Chain;
    FMythicAffixCatalogueMath::ResolveRuleChain(RulesByItemType, ItemType, Chain);
    if (Chain.Num() == 0) {
        return 0;
    }

    TArray<FName> CoreIds;
    for (const int32 RuleIdx : Chain) {
        for (const FName Id : RulesByItemType[RuleIdx].CoreAffixIds) {
            CoreIds.AddUnique(Id);
        }
    }
    return AppendDefsById(Entries, CoreIds, nullptr, Out);
}

int32 UMythicAffixCatalogue::BuildRandomDefs(const FGameplayTag &ItemType, const FGameplayTagContainer &TypeProbe,
                                             TArray<FMythicTieredAffixDef> &Out) const {
    TArray<int32> Chain;
    FMythicAffixCatalogueMath::ResolveRuleChain(RulesByItemType, ItemType, Chain);

    // Both meanings are honoured, because taking the explicit list the moment one exists lets a child rule
    // authored to ADD two ids silently delete its parent's "everything applicable".
    TArray<FName> ExplicitIds;
    bool bOpenPool = (Chain.Num() == 0);
    for (const int32 RuleIdx : Chain) {
        const TArray<FName> &Ids = RulesByItemType[RuleIdx].RandomAffixIds;
        bOpenPool |= (Ids.Num() == 0);
        for (const FName Id : Ids) {
            ExplicitIds.AddUnique(Id);
        }
    }

    if (!bOpenPool) {
        return AppendDefsById(Entries, ExplicitIds, &TypeProbe, Out);
    }

    TArray<bool> Taken;
    Taken.Init(false, Entries.Num());
    int32 Appended = AppendDefsById(Entries, ExplicitIds, &TypeProbe, Out, &Taken);
    for (int32 i = 0; i < Entries.Num(); ++i) {
        if (Taken[i] || !IsRollable(Entries[i]) || !AppliesTo(Entries[i].Def, TypeProbe)) {
            continue;
        }
        Out.Add(Entries[i].Def);
        ++Appended;
    }
    return Appended;
}

#if WITH_EDITOR
EDataValidationResult UMythicAffixCatalogue::IsDataValid(FDataValidationContext &Context) const {
    EDataValidationResult Result = Super::IsDataValid(Context);

    for (int32 i = 0; i < Entries.Num(); ++i) {
        const FMythicAffixCatalogueEntry &Entry = Entries[i];
        const FText IdText = FText::FromName(Entry.AffixId);

        if (Entry.AffixId.IsNone()) {
            Context.AddError(LOCTEXT("AffixNoId", "An entry has no AffixId, so no rule can name it."));
            Result = EDataValidationResult::Invalid;
        }
        // Asked through the lookup itself, so validation and runtime resolution can never disagree.
        else if (FMythicAffixCatalogueMath::FindEntryIndex(Entries, Entry.AffixId) != i) {
            Context.AddError(FText::Format(
                LOCTEXT("AffixDupId", "'{0}' is catalogued twice - ids ignore case - so every rule naming it "
                        "gets the first row."),
                IdText));
            Result = EDataValidationResult::Invalid;
        }

        if (!Entry.Def.Attribute.IsValid()) {
            Context.AddError(FText::Format(LOCTEXT("AffixNoAttribute", "'{0}' names no attribute to modify."),
                                           IdText));
            Result = EDataValidationResult::Invalid;
        }

        if (Entry.Def.Tiers.Num() == 0) {
            Context.AddError(FText::Format(LOCTEXT("AffixNoTiers", "'{0}' has an empty tier ladder, so it has "
                                                   "nothing to roll."), IdText));
            Result = EDataValidationResult::Invalid;
        }

        // The random roller takes Min/Max straight off the tier, so a 0..0 band rolls 0 - and a multiplying
        // affix then scales the attribute by KINDA_SMALL_NUMBER, wiping the stat the moment the item is worn.
        for (int32 t = 0; t < Entry.Def.Tiers.Num(); ++t) {
            const FMythicAffixTier &Tier = Entry.Def.Tiers[t];
            // A 0/0 band is not empty by itself: MythicCombat::ResolveCoreAffixBand reads it as "the central band
            // decides", and a level-scaled tier rolls ItemLevel * LevelScaling from it. Only all three at zero
            // genuinely rolls nothing.
            if (Tier.Min == 0.0f && Tier.Max == 0.0f && Tier.LevelScaling == 0.0f) {
                Context.AddError(FText::Format(
                    LOCTEXT("AffixTierNoBand", "'{0}' tier {1} has no band and no level scaling, so it always rolls "
                            "zero - and a multiplying or dividing affix wipes the attribute instead."),
                    IdText, FText::AsNumber(t)));
                Result = EDataValidationResult::Invalid;
            }
            else if (Tier.Max < Tier.Min) {
                Context.AddError(FText::Format(
                    LOCTEXT("AffixTierInverted", "'{0}' tier {1} has Max {2} below Min {3}, so its roll band is "
                            "inverted."),
                    IdText, FText::AsNumber(t), FText::AsNumber(Tier.Max), FText::AsNumber(Tier.Min)));
                Result = EDataValidationResult::Invalid;
            }
        }
    }

    int32 ClaimedIds = 0;
    int32 Unresolved = 0;
    for (const FMythicItemTypeAffixRule &Rule : RulesByItemType) {
        if (!Rule.ItemType.IsValid()) {
            Context.AddError(LOCTEXT("AffixRuleNoType", "A rule has no ItemType tag, so no item matches it."));
            Result = EDataValidationResult::Invalid;
            continue;
        }

        const FText RuleText = FText::FromString(Rule.ItemType.ToString());
        for (const TArray<FName> *List : {&Rule.CoreAffixIds, &Rule.RandomAffixIds}) {
            for (const FName Id : *List) {
                ++ClaimedIds;
                if (FMythicAffixCatalogueMath::FindEntryIndex(Entries, Id) == INDEX_NONE) {
                    ++Unresolved;
                    Context.AddError(FText::Format(
                        LOCTEXT("AffixRuleUnknownId", "Rule '{0}' names '{1}', which is not in the catalogue."),
                        RuleText, FText::FromName(Id)));
                    Result = EDataValidationResult::Invalid;
                }
            }
        }

        // A guaranteed affix whose ladder starts above level 1 has no eligible tier on a low-level item. The
        // roller falls back to the lowest tier rather than dropping it, but the ladder is still misauthored.
        for (const FName Id : Rule.CoreAffixIds) {
            const int32 Index = FMythicAffixCatalogueMath::FindEntryIndex(Entries, Id);
            if (Index == INDEX_NONE || Entries[Index].Def.Tiers.Num() == 0) {
                continue;
            }
            int32 Floor = MAX_int32;
            for (const FMythicAffixTier &Tier : Entries[Index].Def.Tiers) {
                Floor = FMath::Min(Floor, Tier.MinItemLevel);
            }
            if (Floor > 1) {
                Context.AddError(FText::Format(
                    LOCTEXT("AffixCoreLadderFloor", "Rule '{0}' guarantees '{1}', but its lowest tier needs item "
                            "level {2}, so an item below that level has no tier to roll."),
                    RuleText, FText::FromName(Id), FText::AsNumber(Floor)));
                Result = EDataValidationResult::Invalid;
            }
        }
    }

    // No reachability claim is made. Whether an entry can ever roll depends on an ITEM's probe, and validation
    // has no item: an item type matching no rule falls into the applicability fallback over every entry, so an
    // entry nothing names is still rollable and condemning it would point the designer at live content.

    // Say how much was examined. Without the denominator a catalogue wired to nothing reports the same clean
    // bill of health as one wired to everything.
    Context.AddMessage(EMessageSeverity::Info,
                       FText::Format(LOCTEXT("AffixCatalogueSummary",
                                             "Affix catalogue: {0} entries, {1} rules claiming {2} ids, {3} "
                                             "unresolved."),
                                     FText::AsNumber(Entries.Num()), FText::AsNumber(RulesByItemType.Num()),
                                     FText::AsNumber(ClaimedIds), FText::AsNumber(Unresolved)));
    return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
