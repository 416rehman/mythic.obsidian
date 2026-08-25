// Copyright Stellar Games. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameplayTagContainer.h"
// ItemDefinition.h, reached through the fragment header, names USkeletalMesh without declaring it.
#include "Engine/SkeletalMesh.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "Itemization/Affixes/MythicAffixCatalogue.h"
#include "Itemization/Inventory/Fragments/Passive/AffixesFragment.h"
#include "Itemization/MythicLootSettings.h"
#include "Itemization/MythicTags_Inventory.h"
#include "Settings/MythicCombatSettings.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

namespace {
FMythicAffixCatalogueEntry MakeEntry(const FName Id, const FGameplayAttribute &Attribute) {
    FMythicAffixCatalogueEntry Entry;
    Entry.AffixId = Id;
    Entry.Def.Attribute = Attribute;
    FMythicAffixTier Tier;
    Tier.MinItemLevel = 1;
    Tier.Weight = 1.0f;
    Tier.Min = 1.0f;
    Tier.Max = 5.0f;
    Entry.Def.Tiers.Add(Tier);
    return Entry;
}

FMythicItemTypeAffixRule MakeRule(const FGameplayTag &ItemType) {
    FMythicItemTypeAffixRule Rule;
    Rule.ItemType = ItemType;
    return Rule;
}

FGameplayTagContainer MakeProbe(const FGameplayTag &A, const FGameplayTag &B = FGameplayTag()) {
    FGameplayTagContainer Probe;
    Probe.AddTag(A);
    Probe.AddTag(B);
    return Probe;
}

int32 CountDefsFor(const TArray<FMythicTieredAffixDef> &Defs, const FGameplayAttribute &Attribute) {
    int32 Count = 0;
    for (const FMythicTieredAffixDef &Def : Defs) {
        if (Def.Attribute == Attribute) {
            ++Count;
        }
    }
    return Count;
}

FMythicTieredAffixDef MakeDef(const FGameplayAttribute &Attribute, EMythicAffixGroup Group) {
    FMythicTieredAffixDef Def;
    Def.Attribute = Attribute;
    Def.Group = Group;
    FMythicAffixTier Tier;
    Tier.MinItemLevel = 1;
    Tier.Weight = 1.0f;
    Tier.Min = 1.0f;
    Tier.Max = 5.0f;
    Def.Tiers.Add(Tier);
    return Def;
}

FRollDefinition MakeRoll() {
    FRollDefinition Roll;
    Roll.Min = 1.0f;
    Roll.Max = 5.0f;
    return Roll;
}

int32 CountRolled(const TArray<FRolledAffix> &Rolled, const FGameplayAttribute &Attribute) {
    int32 Count = 0;
    for (const FRolledAffix &Affix : Rolled) {
        if (Affix.Attribute == Attribute) {
            ++Count;
        }
    }
    return Count;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAffixCatalogueRuleResolutionTest,
    "Mythic.Itemization.Affixes.CatalogueRuleResolution",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAffixCatalogueRuleResolutionTest::RunTest(const FString &Parameters) {
    const FGameplayTag Weapon = ITEMIZATION_TYPE_EQUIPMENT_WEAPON.GetTag();
    const FGameplayTag Sword = ITEMIZATION_TYPE_EQUIPMENT_WEAPON_SWORD.GetTag();
    const FGameplayTag Axe = ITEMIZATION_TYPE_EQUIPMENT_WEAPON_AXE.GetTag();
    const FGameplayTag Potion = ITEMIZATION_TYPE_CONSUMABLE_POTION.GetTag();

    {
        const TArray<FMythicItemTypeAffixRule> Rules = {MakeRule(Sword)};
        TestEqual(TEXT("an item type finds the rule authored for it"),
                  FMythicAffixCatalogueMath::ResolveRuleIndex(Rules, Sword), 0);
    }

    {
        const TArray<FMythicItemTypeAffixRule> Rules = {MakeRule(Weapon)};
        TestEqual(TEXT("a sword falls back to the broad weapon rule"),
                  FMythicAffixCatalogueMath::ResolveRuleIndex(Rules, Sword), 0);
        TestEqual(TEXT("the same broad rule covers an axe"),
                  FMythicAffixCatalogueMath::ResolveRuleIndex(Rules, Axe), 0);
    }

    {
        const TArray<FMythicItemTypeAffixRule> ParentFirst = {MakeRule(Weapon), MakeRule(Sword)};
        TestEqual(TEXT("the deeper sword rule beats a weapon rule that precedes it"),
                  FMythicAffixCatalogueMath::ResolveRuleIndex(ParentFirst, Sword), 1);

        const TArray<FMythicItemTypeAffixRule> ChildFirst = {MakeRule(Sword), MakeRule(Weapon)};
        TestEqual(TEXT("and beats one that follows it, so array order never decides"),
                  FMythicAffixCatalogueMath::ResolveRuleIndex(ChildFirst, Sword), 0);
        TestEqual(TEXT("an axe still takes the weapon rule beside a sword rule"),
                  FMythicAffixCatalogueMath::ResolveRuleIndex(ChildFirst, Axe), 1);
    }

    {
        const TArray<FMythicItemTypeAffixRule> Duplicate = {MakeRule(Sword), MakeRule(Sword)};
        TestEqual(TEXT("equal depth resolves to the earlier rule"),
                  FMythicAffixCatalogueMath::ResolveRuleIndex(Duplicate, Sword), 0);
    }

    {
        const TArray<FMythicItemTypeAffixRule> Rules = {MakeRule(Weapon), MakeRule(Sword)};
        TestEqual(TEXT("a potion matches no weapon rule"),
                  FMythicAffixCatalogueMath::ResolveRuleIndex(Rules, Potion), INDEX_NONE);
        TestEqual(TEXT("an unset item type matches nothing"),
                  FMythicAffixCatalogueMath::ResolveRuleIndex(Rules, FGameplayTag()), INDEX_NONE);

        const TArray<FMythicItemTypeAffixRule> SwordOnly = {MakeRule(Sword)};
        TestEqual(TEXT("matching runs one way only - a weapon does not inherit its sword rule"),
                  FMythicAffixCatalogueMath::ResolveRuleIndex(SwordOnly, Weapon), INDEX_NONE);

        TestEqual(TEXT("an empty rule list resolves to nothing"),
                  FMythicAffixCatalogueMath::ResolveRuleIndex(TArray<FMythicItemTypeAffixRule>(), Sword), INDEX_NONE);
    }

    {
        const TArray<FMythicItemTypeAffixRule> Rules = {MakeRule(Weapon), MakeRule(Sword), MakeRule(Axe)};
        TArray<int32> Chain;
        FMythicAffixCatalogueMath::ResolveRuleChain(Rules, Sword, Chain);
        if (TestEqual(TEXT("a sword collects both the rules it matches, not just the deepest"), Chain.Num(), 2)) {
            TestEqual(TEXT("deepest first, so a merge keeps the specific rule's order"), Chain[0], 1);
            TestEqual(TEXT("then the broader rule it inherits from"), Chain[1], 0);
        }

        Chain.Add(99);
        FMythicAffixCatalogueMath::ResolveRuleChain(Rules, Potion, Chain);
        TestEqual(TEXT("a chain for an unmatched type is emptied, never left stale"), Chain.Num(), 0);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAffixCatalogueRuleInheritanceTest,
    "Mythic.Itemization.Affixes.CatalogueRuleInheritance",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAffixCatalogueRuleInheritanceTest::RunTest(const FString &Parameters) {
    const FGameplayAttribute Power = UMythicAttributeSet_Offense::GetPowerAttribute();
    const FGameplayAttribute Crit = UMythicAttributeSet_Offense::GetCriticalHitChanceAttribute();
    const FGameplayAttribute CritDamage = UMythicAttributeSet_Offense::GetCriticalHitDamageAttribute();
    const FGameplayAttribute Armor = UMythicAttributeSet_Defense::GetArmorAttribute();

    const FGameplayTag Weapon = ITEMIZATION_TYPE_EQUIPMENT_WEAPON.GetTag();
    const FGameplayTag Sword = ITEMIZATION_TYPE_EQUIPMENT_WEAPON_SWORD.GetTag();
    const FGameplayTag Axe = ITEMIZATION_TYPE_EQUIPMENT_WEAPON_AXE.GetTag();

    FMythicItemTypeAffixRule WeaponRule = MakeRule(Weapon);
    WeaponRule.CoreAffixIds = {FName("Power")};
    WeaponRule.RandomAffixIds = {FName("Crit")};

    FMythicItemTypeAffixRule SwordRule = MakeRule(Sword);
    SwordRule.CoreAffixIds = {FName("Armor")};
    SwordRule.RandomAffixIds = {FName("Crit"), FName("CritDamage")};

    // Both orders, because a chain that merged by array position rather than depth would pass one and fail
    // the other.
    for (int32 Order = 0; Order < 2; ++Order) {
        UMythicAffixCatalogue *Catalogue = NewObject<UMythicAffixCatalogue>();
        Catalogue->Entries.Add(MakeEntry(FName("Power"), Power));
        Catalogue->Entries.Add(MakeEntry(FName("Crit"), Crit));
        Catalogue->Entries.Add(MakeEntry(FName("CritDamage"), CritDamage));
        Catalogue->Entries.Add(MakeEntry(FName("Armor"), Armor));
        if (Order == 0) {
            Catalogue->RulesByItemType = {WeaponRule, SwordRule};
        }
        else {
            Catalogue->RulesByItemType = {SwordRule, WeaponRule};
        }

        const FGameplayTagContainer SwordProbe = MakeProbe(Sword);

        TArray<FMythicTieredAffixDef> Core;
        TestEqual(TEXT("a sword rolls its own core stat AND the weapon rule's, merged"),
                  Catalogue->BuildCoreDefs(Sword, SwordProbe, Core), 2);
        TestEqual(TEXT("the parent's guaranteed stat survives the child rule"), CountDefsFor(Core, Power), 1);
        TestEqual(TEXT("beside the child's own"), CountDefsFor(Core, Armor), 1);

        TArray<FMythicTieredAffixDef> Random;
        TestEqual(TEXT("the random pools merge too"), Catalogue->BuildRandomDefs(Sword, SwordProbe, Random), 2);
        TestEqual(TEXT("an id both rules name appears once, not twice"), CountDefsFor(Random, Crit), 1);
        TestEqual(TEXT("and the child's own id is there"), CountDefsFor(Random, CritDamage), 1);

        TArray<FMythicTieredAffixDef> AxeCore;
        TestEqual(TEXT("an axe takes the weapon rule alone"),
                  Catalogue->BuildCoreDefs(Axe, MakeProbe(Axe), AxeCore), 1);
        TestEqual(TEXT("inheritance runs one way - the sword rule never leaks onto an axe"),
                  CountDefsFor(AxeCore, Armor), 0);
    }

    // The trap the merge exists to kill: a child authored only to narrow the random pool must not wipe the
    // core stats the broad rule guarantees.
    {
        FMythicItemTypeAffixRule NarrowingChild = MakeRule(Sword);
        NarrowingChild.RandomAffixIds = {FName("CritDamage")};

        UMythicAffixCatalogue *Catalogue = NewObject<UMythicAffixCatalogue>();
        Catalogue->Entries.Add(MakeEntry(FName("Power"), Power));
        Catalogue->Entries.Add(MakeEntry(FName("Crit"), Crit));
        Catalogue->Entries.Add(MakeEntry(FName("CritDamage"), CritDamage));
        Catalogue->RulesByItemType = {WeaponRule, NarrowingChild};

        TArray<FMythicTieredAffixDef> Core;
        TestEqual(TEXT("a child rule with no core ids still inherits the parent's"),
                  Catalogue->BuildCoreDefs(Sword, MakeProbe(Sword), Core), 1);
        TestEqual(TEXT("and it is the parent's stat"), CountDefsFor(Core, Power), 1);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAffixCatalogueEntryLookupTest,
    "Mythic.Itemization.Affixes.CatalogueEntryLookup",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAffixCatalogueEntryLookupTest::RunTest(const FString &Parameters) {
    const FGameplayAttribute Power = UMythicAttributeSet_Offense::GetPowerAttribute();
    const FGameplayAttribute Crit = UMythicAttributeSet_Offense::GetCriticalHitChanceAttribute();
    const FGameplayAttribute Armor = UMythicAttributeSet_Defense::GetArmorAttribute();

    TArray<FMythicAffixCatalogueEntry> Entries;
    Entries.Add(MakeEntry(FName("Affix_Power"), Power));
    Entries.Add(MakeEntry(FName("Affix_Crit"), Crit));

    TestEqual(TEXT("a catalogued id finds its row"),
              FMythicAffixCatalogueMath::FindEntryIndex(Entries, FName("Affix_Crit")), 1);
    TestEqual(TEXT("an uncatalogued id finds nothing"),
              FMythicAffixCatalogueMath::FindEntryIndex(Entries, FName("Affix_Armor")), INDEX_NONE);
    TestEqual(TEXT("an empty catalogue finds nothing"),
              FMythicAffixCatalogueMath::FindEntryIndex(TArray<FMythicAffixCatalogueEntry>(), FName("Affix_Power")),
              INDEX_NONE);

    // Ids are plain FName ==, so casing behaves identically in the editor and in a cooked build. A guarded
    // case-sensitive assertion would only hold where the build preserves case - which is where the bug wasn't.
    TestEqual(TEXT("a miscased id resolves to the same row in every build"),
              FMythicAffixCatalogueMath::FindEntryIndex(Entries, FName("affix_crit")), 1);

    // A blank id in a rule row must resolve to nothing even when the catalogue holds a blank-id entry, or the
    // rule silently binds to it.
    Entries.Add(MakeEntry(NAME_None, Armor));
    TestEqual(TEXT("an unset id finds nothing even beside a blank-id entry"),
              FMythicAffixCatalogueMath::FindEntryIndex(Entries, NAME_None), INDEX_NONE);
    TestEqual(TEXT("and the named rows still resolve"),
              FMythicAffixCatalogueMath::FindEntryIndex(Entries, FName("Affix_Power")), 0);

    UMythicAffixCatalogue *Catalogue = NewObject<UMythicAffixCatalogue>();
    Catalogue->Entries = Entries;

    const FMythicAffixCatalogueEntry *Found = Catalogue->FindEntry(FName("Affix_Power"));
    if (TestNotNull(TEXT("the asset finds a catalogued entry"), Found)) {
        TestTrue(TEXT("and returns the row that id names"), Found->Def.Attribute == Power);
    }
    TestNull(TEXT("the asset returns nothing for an unknown id"), Catalogue->FindEntry(FName("Affix_Nope")));
    TestNull(TEXT("and nothing for a blank id"), Catalogue->FindEntry(NAME_None));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAffixCatalogueBuildDefsTest,
    "Mythic.Itemization.Affixes.CatalogueBuildDefs",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAffixCatalogueBuildDefsTest::RunTest(const FString &Parameters) {
    const FGameplayAttribute Power = UMythicAttributeSet_Offense::GetPowerAttribute();
    const FGameplayAttribute Crit = UMythicAttributeSet_Offense::GetCriticalHitChanceAttribute();
    const FGameplayAttribute Armor = UMythicAttributeSet_Defense::GetArmorAttribute();

    const FGameplayTag Weapon = ITEMIZATION_TYPE_EQUIPMENT_WEAPON.GetTag();
    const FGameplayTag Sword = ITEMIZATION_TYPE_EQUIPMENT_WEAPON_SWORD.GetTag();
    const FGameplayTag Axe = ITEMIZATION_TYPE_EQUIPMENT_WEAPON_AXE.GetTag();
    const FGameplayTag Potion = ITEMIZATION_TYPE_CONSUMABLE_POTION.GetTag();

    FMythicTieredAffixDef Sentinel;
    Sentinel.Attribute = Armor;
    Sentinel.Tiers.Add(FMythicAffixTier());

    {
        UMythicAffixCatalogue *Catalogue = NewObject<UMythicAffixCatalogue>();
        Catalogue->Entries.Add(MakeEntry(FName("Power"), Power));

        FMythicAffixCatalogueEntry SwordGated = MakeEntry(FName("Crit"), Crit);
        SwordGated.Def.Applicability = FGameplayTagQuery::MakeQuery_MatchAnyTags(FGameplayTagContainer(Sword));
        Catalogue->Entries.Add(SwordGated);

        FMythicAffixCatalogueEntry Ladderless = MakeEntry(FName("Ladderless"), Armor);
        Ladderless.Def.Tiers.Empty();
        Catalogue->Entries.Add(Ladderless);

        Catalogue->Entries.Add(MakeEntry(FName("Attributeless"), FGameplayAttribute()));

        FMythicItemTypeAffixRule SwordRule = MakeRule(Sword);
        SwordRule.CoreAffixIds = {FName("Power"), FName("Missing"), FName("Ladderless"), FName("Attributeless")};
        SwordRule.RandomAffixIds = {FName("Crit")};
        Catalogue->RulesByItemType.Add(SwordRule);

        const FGameplayTagContainer SwordProbe = MakeProbe(Sword);

        TArray<FMythicTieredAffixDef> Out;
        Out.Add(Sentinel);

        const int32 CoreAdded = Catalogue->BuildCoreDefs(Sword, SwordProbe, Out);
        TestEqual(TEXT("an id naming no entry, an empty tier ladder and a missing attribute are all skipped"),
                  CoreAdded, 1);
        if (TestEqual(TEXT("the caller's existing defs survive - Out is appended to, never cleared"), Out.Num(), 2)) {
            TestTrue(TEXT("and they keep the front of the array"), Out[0].Attribute == Armor);
        }
        TestEqual(TEXT("the one appended def is the catalogued core affix"), CountDefsFor(Out, Power), 1);

        TArray<FMythicTieredAffixDef> Random;
        Random.Add(Sentinel);
        const int32 RandomAdded = Catalogue->BuildRandomDefs(Sword, SwordProbe, Random);
        TestEqual(TEXT("an authored random list is taken verbatim"), RandomAdded, 1);
        if (TestEqual(TEXT("BuildRandomDefs appends to Out as well"), Random.Num(), 2)) {
            TestTrue(TEXT("and leaves the caller's def at the front"), Random[0].Attribute == Armor);
        }
        TestEqual(TEXT("and appends the named entry"), CountDefsFor(Random, Crit), 1);
    }

    {
        UMythicAffixCatalogue *Open = NewObject<UMythicAffixCatalogue>();
        Open->Entries.Add(MakeEntry(FName("Power"), Power));

        FMythicAffixCatalogueEntry SwordGated = MakeEntry(FName("Crit"), Crit);
        SwordGated.Def.Applicability = FGameplayTagQuery::MakeQuery_MatchAnyTags(FGameplayTagContainer(Sword));
        Open->Entries.Add(SwordGated);

        // A rule that names no ids at all - the "everything applicable" case.
        Open->RulesByItemType.Add(MakeRule(Weapon));

        TArray<FMythicTieredAffixDef> ForSword;
        ForSword.Add(Sentinel);
        TestEqual(TEXT("an empty random list rolls every applicable entry"),
                  Open->BuildRandomDefs(Sword, MakeProbe(Sword), ForSword), 2);
        if (TestEqual(TEXT("the fallback appends too, it never clears"), ForSword.Num(), 3)) {
            TestTrue(TEXT("the caller's def keeps the front"), ForSword[0].Attribute == Armor);
        }
        TestEqual(TEXT("including the sword-gated one"), CountDefsFor(ForSword, Crit), 1);

        TArray<FMythicTieredAffixDef> ForAxe;
        ForAxe.Add(Sentinel);
        TestEqual(TEXT("an axe gets only the entries that apply to it"),
                  Open->BuildRandomDefs(Axe, MakeProbe(Axe), ForAxe), 1);
        TestEqual(TEXT("the sword-gated entry stays off the axe"), CountDefsFor(ForAxe, Crit), 0);
        TestEqual(TEXT("the universal entry applies to every weapon"), CountDefsFor(ForAxe, Power), 1);

        TArray<FMythicTieredAffixDef> ForPotion;
        ForPotion.Add(Sentinel);
        TestEqual(TEXT("an item type with no rule still rolls the applicable pool"),
                  Open->BuildRandomDefs(Potion, MakeProbe(Potion), ForPotion), 1);
        TestEqual(TEXT("and rolls only what applies to it"), CountDefsFor(ForPotion, Crit), 0);

        TArray<FMythicTieredAffixDef> Core;
        Core.Add(Sentinel);
        TestEqual(TEXT("but an item type with no rule has no guaranteed rolls"),
                  Open->BuildCoreDefs(Potion, MakeProbe(Potion), Core), 0);
        TestEqual(TEXT("so nothing is appended"), Core.Num(), 1);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAffixCatalogueGuaranteedCoreTest,
    "Mythic.Itemization.Affixes.CatalogueGuaranteedCore",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAffixCatalogueGuaranteedCoreTest::RunTest(const FString &Parameters) {
    const FGameplayAttribute Power = UMythicAttributeSet_Offense::GetPowerAttribute();
    const FGameplayAttribute Crit = UMythicAttributeSet_Offense::GetCriticalHitChanceAttribute();

    const FGameplayTag Sword = ITEMIZATION_TYPE_EQUIPMENT_WEAPON_SWORD.GetTag();
    const FGameplayTag Axe = ITEMIZATION_TYPE_EQUIPMENT_WEAPON_AXE.GetTag();
    const FGameplayTagContainer SwordProbe = MakeProbe(Sword);

    {
        UMythicAffixCatalogue *Catalogue = NewObject<UMythicAffixCatalogue>();

        FMythicAffixCatalogueEntry AxeGated = MakeEntry(FName("Power"), Power);
        AxeGated.Def.Applicability = FGameplayTagQuery::MakeQuery_MatchAnyTags(FGameplayTagContainer(Axe));
        Catalogue->Entries.Add(AxeGated);

        FMythicItemTypeAffixRule SwordRule = MakeRule(Sword);
        SwordRule.CoreAffixIds = {FName("Power")};
        Catalogue->RulesByItemType.Add(SwordRule);

        TArray<FMythicTieredAffixDef> Core;
        TestEqual(TEXT("naming an id in CoreAffixIds rolls it, whatever its Applicability says"),
                  Catalogue->BuildCoreDefs(Sword, SwordProbe, Core), 1);
        TestEqual(TEXT("and it is the guaranteed stat"), CountDefsFor(Core, Power), 1);
    }

    // The catalogue does NOT subtract the core half's attributes from the random half. A fragment's own
    // CoreAffixes beat the catalogue's, so the catalogue cannot know which core list actually won - subtracting
    // here drops the attribute from BOTH halves and the item rolls fewer affixes than its rarity promises.
    // De-duplication is the roll site's job, against what actually rolled: see Affixes.RollSiteDedupe.
    for (int32 Path = 0; Path < 2; ++Path) {
        UMythicAffixCatalogue *Catalogue = NewObject<UMythicAffixCatalogue>();
        Catalogue->Entries.Add(MakeEntry(FName("Power"), Power));
        Catalogue->Entries.Add(MakeEntry(FName("Crit"), Crit));

        FMythicItemTypeAffixRule SwordRule = MakeRule(Sword);
        SwordRule.CoreAffixIds = {FName("Power")};
        if (Path == 1) {
            SwordRule.RandomAffixIds = {FName("Power"), FName("Crit")};
        }
        Catalogue->RulesByItemType.Add(SwordRule);

        TArray<FMythicTieredAffixDef> Core;
        TestEqual(TEXT("the core half rolls its guaranteed stat"),
                  Catalogue->BuildCoreDefs(Sword, SwordProbe, Core), 1);

        TArray<FMythicTieredAffixDef> Random;
        TestEqual(TEXT("the random half offers the whole pool, the core attribute included"),
                  Catalogue->BuildRandomDefs(Sword, SwordProbe, Random), 2);
        TestEqual(TEXT("the catalogue never subtracts what a core id named"), CountDefsFor(Random, Power), 1);
        TestEqual(TEXT("and the rest of the pool is offered beside it"), CountDefsFor(Random, Crit), 1);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAffixCatalogueRandomBranchTest,
    "Mythic.Itemization.Affixes.CatalogueRandomBranch",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAffixCatalogueRandomBranchTest::RunTest(const FString &Parameters) {
    const FGameplayAttribute Power = UMythicAttributeSet_Offense::GetPowerAttribute();
    const FGameplayAttribute Crit = UMythicAttributeSet_Offense::GetCriticalHitChanceAttribute();
    const FGameplayAttribute Armor = UMythicAttributeSet_Defense::GetArmorAttribute();

    const FGameplayTag Sword = ITEMIZATION_TYPE_EQUIPMENT_WEAPON_SWORD.GetTag();
    const FGameplayTag Axe = ITEMIZATION_TYPE_EQUIPMENT_WEAPON_AXE.GetTag();
    const FGameplayTagContainer SwordProbe = MakeProbe(Sword);

    // "Unnamed" is applicable, rollable and reached by the same probe as "Named". The ONLY thing that decides
    // whether it rolls is whether the rule authored a random list, so this pair is what separates "the explicit
    // branch is honoured" from "the explicit branch is dead code and everything falls back".
    auto MakeCatalogue = [&](const TArray<FName> &RandomIds) -> UMythicAffixCatalogue * {
        UMythicAffixCatalogue *Catalogue = NewObject<UMythicAffixCatalogue>();
        Catalogue->Entries.Add(MakeEntry(FName("Named"), Power));
        Catalogue->Entries.Add(MakeEntry(FName("Unnamed"), Crit));
        FMythicItemTypeAffixRule Rule = MakeRule(Sword);
        Rule.RandomAffixIds = RandomIds;
        Catalogue->RulesByItemType.Add(Rule);
        return Catalogue;
    };

    TArray<FMythicTieredAffixDef> Explicit;
    TestEqual(TEXT("an authored random list rolls what it names and nothing else"),
              MakeCatalogue({FName("Named")})->BuildRandomDefs(Sword, SwordProbe, Explicit), 1);
    TestEqual(TEXT("the named entry is the one appended"), CountDefsFor(Explicit, Power), 1);
    TestEqual(TEXT("an equally applicable entry the list omits never rolls"), CountDefsFor(Explicit, Crit), 0);

    TArray<FMythicTieredAffixDef> Fallback;
    TestEqual(TEXT("the same catalogue and the same probe roll both entries once the rule names none"),
              MakeCatalogue(TArray<FName>())->BuildRandomDefs(Sword, SwordProbe, Fallback), 2);
    TestEqual(TEXT("so the omitted entry was omitted by the list, not by applicability"),
              CountDefsFor(Fallback, Crit), 1);

    // The explicit branch still owes the item its probe: naming an id does not force a mismatched entry onto it.
    {
        UMythicAffixCatalogue *Catalogue = NewObject<UMythicAffixCatalogue>();
        Catalogue->Entries.Add(MakeEntry(FName("Named"), Power));
        FMythicAffixCatalogueEntry AxeOnly = MakeEntry(FName("AxeOnly"), Armor);
        AxeOnly.Def.Applicability = FGameplayTagQuery::MakeQuery_MatchAnyTags(FGameplayTagContainer(Axe));
        Catalogue->Entries.Add(AxeOnly);

        FMythicItemTypeAffixRule Rule = MakeRule(Sword);
        Rule.RandomAffixIds = {FName("Named"), FName("AxeOnly")};
        Catalogue->RulesByItemType.Add(Rule);

        TArray<FMythicTieredAffixDef> Out;
        TestEqual(TEXT("a named entry whose Applicability the item fails is still filtered out"),
                  Catalogue->BuildRandomDefs(Sword, SwordProbe, Out), 1);
        TestEqual(TEXT("and it is the off-type one that went"), CountDefsFor(Out, Armor), 0);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAffixCatalogueOpenPoolUnionTest,
    "Mythic.Itemization.Affixes.CatalogueOpenPoolUnion",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAffixCatalogueOpenPoolUnionTest::RunTest(const FString &Parameters) {
    const FGameplayAttribute Power = UMythicAttributeSet_Offense::GetPowerAttribute();
    const FGameplayAttribute Crit = UMythicAttributeSet_Offense::GetCriticalHitChanceAttribute();
    const FGameplayAttribute CritDamage = UMythicAttributeSet_Offense::GetCriticalHitDamageAttribute();
    const FGameplayAttribute Armor = UMythicAttributeSet_Defense::GetArmorAttribute();

    const FGameplayTag Weapon = ITEMIZATION_TYPE_EQUIPMENT_WEAPON.GetTag();
    const FGameplayTag Sword = ITEMIZATION_TYPE_EQUIPMENT_WEAPON_SWORD.GetTag();
    const FGameplayTag Axe = ITEMIZATION_TYPE_EQUIPMENT_WEAPON_AXE.GetTag();
    const FGameplayTag Potion = ITEMIZATION_TYPE_CONSUMABLE_POTION.GetTag();
    const FGameplayTagContainer SwordProbe = MakeProbe(Sword);

    /**
     * The inverse of the core half's merge trap. A Weapon rule with an EMPTY random list means "everything
     * applicable"; a Sword rule naming two ids is authored to ADD them. Taking the explicit list the moment one
     * exists deletes the parent's meaning, and the sword silently rolls two affixes instead of the whole pool.
     * Both array orders, because a union decided by array position rather than by the chain would pass one.
     */
    for (int32 Order = 0; Order < 2; ++Order) {
        FMythicItemTypeAffixRule OpenParent = MakeRule(Weapon);
        FMythicItemTypeAffixRule NamedChild = MakeRule(Sword);
        NamedChild.RandomAffixIds = {FName("Crit"), FName("CritDamage")};

        UMythicAffixCatalogue *Catalogue = NewObject<UMythicAffixCatalogue>();
        Catalogue->Entries.Add(MakeEntry(FName("Power"), Power));
        Catalogue->Entries.Add(MakeEntry(FName("Crit"), Crit));
        Catalogue->Entries.Add(MakeEntry(FName("CritDamage"), CritDamage));
        Catalogue->Entries.Add(MakeEntry(FName("Armor"), Armor));
        if (Order == 0) {
            Catalogue->RulesByItemType = {OpenParent, NamedChild};
        }
        else {
            Catalogue->RulesByItemType = {NamedChild, OpenParent};
        }

        TArray<FMythicTieredAffixDef> Random;
        TestEqual(TEXT("the child's ids AND the pool its parent left open both roll"),
                  Catalogue->BuildRandomDefs(Sword, SwordProbe, Random), 4);
        TestEqual(TEXT("an id the child names and the open pass would also reach appears once, not twice"),
                  CountDefsFor(Random, Crit), 1);
        TestEqual(TEXT("and so does the child's other id"), CountDefsFor(Random, CritDamage), 1);
        TestEqual(TEXT("an entry only the open parent reaches still rolls"), CountDefsFor(Random, Power), 1);
        TestEqual(TEXT("all of them, each exactly once"), CountDefsFor(Random, Armor), 1);
    }

    // The explicit half still owes the item its probe, on the open path too: naming an off-type id neither forces
    // it on nor lets the open pass put it back.
    {
        UMythicAffixCatalogue *Catalogue = NewObject<UMythicAffixCatalogue>();
        Catalogue->Entries.Add(MakeEntry(FName("Power"), Power));
        FMythicAffixCatalogueEntry AxeOnly = MakeEntry(FName("AxeOnly"), Armor);
        AxeOnly.Def.Applicability = FGameplayTagQuery::MakeQuery_MatchAnyTags(FGameplayTagContainer(Axe));
        Catalogue->Entries.Add(AxeOnly);

        FMythicItemTypeAffixRule NamedChild = MakeRule(Sword);
        NamedChild.RandomAffixIds = {FName("AxeOnly")};
        Catalogue->RulesByItemType.Add(MakeRule(Weapon));
        Catalogue->RulesByItemType.Add(NamedChild);

        TArray<FMythicTieredAffixDef> Random;
        TestEqual(TEXT("an id the item's probe rejects is appended by neither half"),
                  Catalogue->BuildRandomDefs(Sword, SwordProbe, Random), 1);
        TestEqual(TEXT("so the off-type entry stays off the sword"), CountDefsFor(Random, Armor), 0);
        TestEqual(TEXT("and the open pool's own entry rolls once"), CountDefsFor(Random, Power), 1);
    }

    // The other side of the union: with every rule in the chain authoring a list, the explicit path still owns the
    // result. Widening this to "always union the fallback" would make an authored list unable to narrow anything.
    {
        FMythicItemTypeAffixRule WeaponRule = MakeRule(Weapon);
        WeaponRule.RandomAffixIds = {FName("Power")};
        FMythicItemTypeAffixRule SwordRule = MakeRule(Sword);
        SwordRule.RandomAffixIds = {FName("Crit")};

        UMythicAffixCatalogue *Catalogue = NewObject<UMythicAffixCatalogue>();
        Catalogue->Entries.Add(MakeEntry(FName("Power"), Power));
        Catalogue->Entries.Add(MakeEntry(FName("Crit"), Crit));
        Catalogue->Entries.Add(MakeEntry(FName("Armor"), Armor));
        Catalogue->RulesByItemType = {WeaponRule, SwordRule};

        TArray<FMythicTieredAffixDef> Random;
        TestEqual(TEXT("a chain where every rule names ids rolls their union and nothing else"),
                  Catalogue->BuildRandomDefs(Sword, SwordProbe, Random), 2);
        TestEqual(TEXT("an equally applicable entry no rule names stays out"), CountDefsFor(Random, Armor), 0);

        TArray<FMythicTieredAffixDef> ForPotion;
        TestEqual(TEXT("but a type matching no rule at all still takes the open pool"),
                  Catalogue->BuildRandomDefs(Potion, MakeProbe(Potion), ForPotion), 3);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAffixCatalogueInstanceTagProbeTest,
    "Mythic.Itemization.Affixes.CatalogueInstanceTagProbe",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAffixCatalogueInstanceTagProbeTest::RunTest(const FString &Parameters) {
    // GetTypeProbe is the item's type tag PLUS its ItemTags, so an entry gated on a tag that lives in ItemTags
    // is reachable only through that item's own probe. Itemization.ActionType.InInventory is such a tag -
    // UMythicItemInstance::HasTag reads it straight off ItemTags. Rarity is not: it is the enum
    // UItemDefinition::Rarity and never reaches the probe.
    const FGameplayTag InInventory = ITEMIZATION_ACTIONTYPE_ININVENTORY.GetTag();

    const FGameplayAttribute Power = UMythicAttributeSet_Offense::GetPowerAttribute();
    const FGameplayAttribute Crit = UMythicAttributeSet_Offense::GetCriticalHitChanceAttribute();
    const FGameplayTag Sword = ITEMIZATION_TYPE_EQUIPMENT_WEAPON_SWORD.GetTag();

    FMythicAffixCatalogueEntry StowedOnly = MakeEntry(FName("StowedOnly"), Crit);
    StowedOnly.Def.Applicability = FGameplayTagQuery::MakeQuery_MatchAnyTags(FGameplayTagContainer(InInventory));

    for (int32 Path = 0; Path < 2; ++Path) {
        UMythicAffixCatalogue *Catalogue = NewObject<UMythicAffixCatalogue>();
        Catalogue->Entries.Add(MakeEntry(FName("Power"), Power));
        Catalogue->Entries.Add(StowedOnly);

        FMythicItemTypeAffixRule SwordRule = MakeRule(Sword);
        if (Path == 1) {
            SwordRule.RandomAffixIds = {FName("Power"), FName("StowedOnly")};
        }
        Catalogue->RulesByItemType.Add(SwordRule);

        TArray<FMythicTieredAffixDef> WithTag;
        TestEqual(TEXT("an entry gated on an instance tag is reachable when the item carries it"),
                  Catalogue->BuildRandomDefs(Sword, MakeProbe(Sword, InInventory), WithTag), 2);
        TestEqual(TEXT("and it is the gated entry"), CountDefsFor(WithTag, Crit), 1);

        TArray<FMythicTieredAffixDef> WithoutTag;
        TestEqual(TEXT("the same entry is absent from an item of the same type without the tag"),
                  Catalogue->BuildRandomDefs(Sword, MakeProbe(Sword), WithoutTag), 1);
        TestEqual(TEXT("only the ungated entry survives"), CountDefsFor(WithoutTag, Crit), 0);
        TestEqual(TEXT("which still rolls"), CountDefsFor(WithoutTag, Power), 1);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAffixCoreTierRollTest,
    "Mythic.Itemization.Affixes.CoreTierRoll",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAffixCoreTierRollTest::RunTest(const FString &Parameters) {
    const FGameplayTagContainer Probe = MakeProbe(ITEMIZATION_TYPE_EQUIPMENT_WEAPON_SWORD.GetTag());

    {
        // Every rung is gated above the item's level. A guaranteed affix still rolls, off the lowest rung -
        // and "lowest" means lowest MinItemLevel, not index 0, so the ladder is authored out of order here.
        TArray<FMythicTieredAffixDef> Defs;
        FMythicTieredAffixDef Def;
        Def.Attribute = UMythicAttributeSet_Offense::GetCriticalHitDamageAttribute();

        FMythicAffixTier High;
        High.MinItemLevel = 80;
        High.Weight = 1.0f;
        High.Min = 200.0f;
        High.Max = 200.0f;
        Def.Tiers.Add(High);

        FMythicAffixTier Low;
        Low.MinItemLevel = 50;
        Low.Weight = 1.0f;
        Low.Min = 100.0f;
        Low.Max = 100.0f;
        Def.Tiers.Add(Low);
        Defs.Add(Def);

        UAffixesFragment *Fragment = NewObject<UAffixesFragment>();
        Fragment->RollCoreAffixesTiered(5, Probe, Defs);
        if (!TestEqual(TEXT("a core affix below every tier's level still rolls"),
                       Fragment->AffixesRuntimeReplicatedData.RolledCoreAffixes.Num(), 1)) {
            return false;
        }
        const FRolledAffix &Affix = Fragment->AffixesRuntimeReplicatedData.RolledCoreAffixes[0];
        TestEqual(TEXT("off the rung with the lowest MinItemLevel"), Affix.TierIndex, 1);
        TestTrue(TEXT("and it rolled a value, not nothing"), Affix.Value > 0.0f);
        TestTrue(TEXT("core affixes roll locked so a Refine cannot take them"), Affix.bIsLocked);
    }

    {
        // A centrally scaled family draws its band from combat settings, so retuning CoreAffixLevelCurve moves
        // the tiered core path and the legacy one together.
        const FGameplayAttribute SwordDamage = UMythicAttributeSet_Offense::GetBonusSwordDamageAttribute();
        float CentralMin = 0.0f;
        float CentralMax = 0.0f;
        if (!MythicCombat::ResolveCoreAffixBand(SwordDamage, 0.0f, 0.0f, 12.0f, CentralMin, CentralMax)) {
            AddInfo(TEXT("BonusSwordDamage has no central row; the tiered core band is unverified."));
            return true;
        }

        TArray<FMythicTieredAffixDef> Defs;
        FMythicTieredAffixDef Def;
        Def.Attribute = SwordDamage;
        FMythicAffixTier Tier;
        Tier.MinItemLevel = 1;
        Tier.Weight = 1.0f;
        Def.Tiers.Add(Tier);
        Defs.Add(Def);

        UAffixesFragment *Fragment = NewObject<UAffixesFragment>();
        Fragment->RollCoreAffixesTiered(12, Probe, Defs);
        if (!TestEqual(TEXT("the unauthored band rolls"),
                       Fragment->AffixesRuntimeReplicatedData.RolledCoreAffixes.Num(), 1)) {
            return false;
        }
        const FRolledAffix &Affix = Fragment->AffixesRuntimeReplicatedData.RolledCoreAffixes[0];
        TestTrue(TEXT("the central band is what a zero-authored tier rolls from"), CentralMin > 0.0f);
        TestTrue(FString::Printf(TEXT("the tiered core roll sits inside the central band (got %f)"), Affix.Value),
                 Affix.Value >= CentralMin - KINDA_SMALL_NUMBER && Affix.Value <= CentralMax + KINDA_SMALL_NUMBER);
        TestTrue(TEXT("the stored definition carries the central band so a reroll matches"),
                 FMath::IsNearlyEqual(Affix.Definition.Min, CentralMin, 0.001f)
                 && FMath::IsNearlyEqual(Affix.Definition.Max, CentralMax, 0.001f));
        TestTrue(TEXT("and its private level scaling is zeroed so the level never applies twice"),
                 FMath::IsNearlyZero(Affix.Definition.LevelScaling));
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAffixRollSiteDedupeTest,
    "Mythic.Itemization.Affixes.RollSiteDedupe",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAffixRollSiteDedupeTest::RunTest(const FString &Parameters) {
    /**
     * The catalogue hands both halves the same attribute on purpose, so the roll site is the only thing
     * standing between an item and a stat applied twice, printed twice, and refined only half away. Both
     * random rollers are exercised because both branches ship: the tiered one for catalogue and tiered-pool
     * content, the flat one for hand-authored AffixPoolMap content.
     */
    const FGameplayAttribute Power = UMythicAttributeSet_Offense::GetPowerAttribute();
    const FGameplayAttribute Crit = UMythicAttributeSet_Offense::GetCriticalHitChanceAttribute();
    const FGameplayTagContainer Probe = MakeProbe(ITEMIZATION_TYPE_EQUIPMENT_WEAPON_SWORD.GetTag());
    const int32 ItemLevel = 10;

    {
        TArray<FMythicTieredAffixDef> CoreDefs;
        CoreDefs.Add(MakeDef(Power, EMythicAffixGroup::Prefix));

        // Prefix and Suffix so the two-affix budget can seat both, and only the de-dupe keeps Power out.
        TArray<FMythicTieredAffixDef> RandomDefs;
        RandomDefs.Add(MakeDef(Power, EMythicAffixGroup::Prefix));
        RandomDefs.Add(MakeDef(Crit, EMythicAffixGroup::Suffix));

        UAffixesFragment *Fragment = NewObject<UAffixesFragment>();
        Fragment->RollCoreAffixesTiered(ItemLevel, Probe, CoreDefs);
        Fragment->RollAffixesTiered(ItemLevel, 2, Probe, RandomDefs);

        const TArray<FRolledAffix> &Core = Fragment->AffixesRuntimeReplicatedData.RolledCoreAffixes;
        const TArray<FRolledAffix> &Random = Fragment->AffixesRuntimeReplicatedData.RolledAffixes;
        TestEqual(TEXT("the guaranteed stat rolled on the core half"), CountRolled(Core, Power), 1);
        TestEqual(TEXT("the tiered random roller never rolls an attribute the core half owns"),
                  CountRolled(Random, Power), 0);
        TestEqual(TEXT("so it lands on the item exactly once"),
                  CountRolled(Core, Power) + CountRolled(Random, Power), 1);
        TestEqual(TEXT("and the rest of the tiered pool still rolls"), CountRolled(Random, Crit), 1);
    }

    {
        UAffixesFragment *Fragment = NewObject<UAffixesFragment>();
        Fragment->AffixesBuildData.CoreAffixes.Add(Power, MakeRoll());
        Fragment->AffixesBuildData.AffixPoolMap.Add(Power, MakeRoll());
        Fragment->AffixesBuildData.AffixPoolMap.Add(Crit, MakeRoll());

        Fragment->RollCoreAffixes(ItemLevel);
        Fragment->RollAffixes(ItemLevel, 2);

        const TArray<FRolledAffix> &Core = Fragment->AffixesRuntimeReplicatedData.RolledCoreAffixes;
        const TArray<FRolledAffix> &Random = Fragment->AffixesRuntimeReplicatedData.RolledAffixes;
        TestEqual(TEXT("the guaranteed stat rolled on the core half"), CountRolled(Core, Power), 1);
        TestEqual(TEXT("the flat random roller never rolls an attribute the core half owns"),
                  CountRolled(Random, Power), 0);
        TestEqual(TEXT("so it lands on the item exactly once"),
                  CountRolled(Core, Power) + CountRolled(Random, Power), 1);
        TestEqual(TEXT("and the rest of the flat pool still rolls"), CountRolled(Random, Crit), 1);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAffixCoreRollDedupeTest,
    "Mythic.Itemization.Affixes.CoreRollDedupe",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAffixCoreRollDedupeTest::RunTest(const FString &Parameters) {
    /**
     * Both core rollers are public and ungated, so the "one attribute, one item" invariant cannot rest on
     * OnInstanced happening to roll core first. Each is called DIRECTLY with the attribute already sitting on the
     * random list - the case the call order was masking - and a second core def proves the roller still runs.
     */
    const FGameplayAttribute Power = UMythicAttributeSet_Offense::GetPowerAttribute();
    const FGameplayAttribute Crit = UMythicAttributeSet_Offense::GetCriticalHitChanceAttribute();
    const FGameplayTagContainer Probe = MakeProbe(ITEMIZATION_TYPE_EQUIPMENT_WEAPON_SWORD.GetTag());
    const int32 ItemLevel = 10;

    {
        UAffixesFragment *Fragment = NewObject<UAffixesFragment>();
        FRollDefinition Roll = MakeRoll();
        Fragment->AffixesRuntimeReplicatedData.RolledAffixes.Add(FRolledAffix(Power, ItemLevel, Roll, false));

        TArray<FMythicTieredAffixDef> CoreDefs;
        CoreDefs.Add(MakeDef(Power, EMythicAffixGroup::Prefix));
        CoreDefs.Add(MakeDef(Crit, EMythicAffixGroup::Suffix));
        Fragment->RollCoreAffixesTiered(ItemLevel, Probe, CoreDefs);

        const TArray<FRolledAffix> &Core = Fragment->AffixesRuntimeReplicatedData.RolledCoreAffixes;
        const TArray<FRolledAffix> &Random = Fragment->AffixesRuntimeReplicatedData.RolledAffixes;
        TestEqual(TEXT("the tiered core roller skips an attribute the random half already holds"),
                  CountRolled(Core, Power), 0);
        TestEqual(TEXT("so it lands on the item exactly once"),
                  CountRolled(Core, Power) + CountRolled(Random, Power), 1);
        TestEqual(TEXT("and the core def nothing else owns still rolls"), CountRolled(Core, Crit), 1);
    }

    {
        UAffixesFragment *Fragment = NewObject<UAffixesFragment>();
        FRollDefinition Roll = MakeRoll();
        Fragment->AffixesRuntimeReplicatedData.RolledAffixes.Add(FRolledAffix(Power, ItemLevel, Roll, false));

        Fragment->AffixesBuildData.CoreAffixes.Add(Power, MakeRoll());
        Fragment->AffixesBuildData.CoreAffixes.Add(Crit, MakeRoll());
        Fragment->RollCoreAffixes(ItemLevel);

        const TArray<FRolledAffix> &Core = Fragment->AffixesRuntimeReplicatedData.RolledCoreAffixes;
        const TArray<FRolledAffix> &Random = Fragment->AffixesRuntimeReplicatedData.RolledAffixes;
        TestEqual(TEXT("the flat core roller skips an attribute the random half already holds"),
                  CountRolled(Core, Power), 0);
        TestEqual(TEXT("so it lands on the item exactly once"),
                  CountRolled(Core, Power) + CountRolled(Random, Power), 1);
        TestEqual(TEXT("and the core stat nothing else owns still rolls"), CountRolled(Core, Crit), 1);
    }

    return true;
}

#if WITH_EDITOR
namespace {
// AddError/AddWarning fill FIssue::Message; AddMessage builds a tokenized message and leaves Message empty.
FString IssueText(const FDataValidationContext::FIssue &Issue) {
    return Issue.TokenizedMessage.IsValid() ? Issue.TokenizedMessage->ToText().ToString() : Issue.Message.ToString();
}

bool ReportMentions(const FDataValidationContext &Context, EMessageSeverity::Type Severity, const TCHAR *Needle) {
    for (const FDataValidationContext::FIssue &Issue : Context.GetIssues()) {
        if (Issue.Severity == Severity && IssueText(Issue).Contains(Needle)) {
            return true;
        }
    }
    return false;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAffixCatalogueValidationTest,
    "Mythic.Itemization.Affixes.CatalogueValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMythicAffixCatalogueValidationTest::RunTest(const FString &Parameters) {
    const FGameplayAttribute Power = UMythicAttributeSet_Offense::GetPowerAttribute();
    const FGameplayAttribute Crit = UMythicAttributeSet_Offense::GetCriticalHitChanceAttribute();
    const FGameplayAttribute Armor = UMythicAttributeSet_Defense::GetArmorAttribute();
    const FGameplayTag Weapon = ITEMIZATION_TYPE_EQUIPMENT_WEAPON.GetTag();
    const FGameplayTag Sword = ITEMIZATION_TYPE_EQUIPMENT_WEAPON_SWORD.GetTag();
    const FGameplayTag Axe = ITEMIZATION_TYPE_EQUIPMENT_WEAPON_AXE.GetTag();
    const FGameplayTag Spear = ITEMIZATION_TYPE_EQUIPMENT_WEAPON_SPEAR.GetTag();
    const FGameplayTag Potion = ITEMIZATION_TYPE_CONSUMABLE_POTION.GetTag();

    auto MakeClean = [&]() -> UMythicAffixCatalogue * {
        UMythicAffixCatalogue *Catalogue = NewObject<UMythicAffixCatalogue>();
        Catalogue->Entries.Add(MakeEntry(FName("Affix_Power"), Power));
        Catalogue->Entries.Add(MakeEntry(FName("Affix_Crit"), Crit));
        FMythicItemTypeAffixRule Rule = MakeRule(Sword);
        Rule.RandomAffixIds = {FName("Affix_Power"), FName("Affix_Crit")};
        Catalogue->RulesByItemType.Add(Rule);
        return Catalogue;
    };

    // The denominator: a wired catalogue reports nothing, so every assertion below is a real failure and not
    // a validator that reports on everything.
    {
        FDataValidationContext Context;
        const EDataValidationResult Result = MakeClean()->IsDataValid(Context);
        TestEqual(TEXT("a wired catalogue raises no error"), static_cast<int32>(Context.GetNumErrors()), 0);
        TestEqual(TEXT("and no warning"), static_cast<int32>(Context.GetNumWarnings()), 0);
        TestTrue(TEXT("so it is not reported invalid"), Result != EDataValidationResult::Invalid);
    }

    {
        UMythicAffixCatalogue *Catalogue = MakeClean();
        Catalogue->Entries.Add(MakeEntry(NAME_None, Armor));

        FDataValidationContext Context;
        const EDataValidationResult Result = Catalogue->IsDataValid(Context);
        TestEqual(TEXT("a blank AffixId is an error, not a warning - a blank rule id would bind to it"),
                  static_cast<int32>(Context.GetNumErrors()), 1);
        TestTrue(TEXT("and fails the asset"), Result == EDataValidationResult::Invalid);
    }

    {
        UMythicAffixCatalogue *Catalogue = MakeClean();
        Catalogue->Entries.Add(MakeEntry(FName("affix_power"), Armor));

        FDataValidationContext Context;
        const EDataValidationResult Result = Catalogue->IsDataValid(Context);
        TestEqual(TEXT("two ids differing only by case are the same FName, so they are duplicates"),
                  static_cast<int32>(Context.GetNumErrors()), 1);
        TestTrue(TEXT("and fail the asset"), Result == EDataValidationResult::Invalid);
    }

    // Reachability is undecidable without an item, so an entry no rule names must NOT be reported: an item type
    // matching no rule falls into the applicability fallback over every entry, and condemning it would point the
    // designer at live content.
    {
        UMythicAffixCatalogue *Catalogue = MakeClean();
        Catalogue->Entries.Add(MakeEntry(FName("Affix_Unnamed"), Armor));

        FDataValidationContext Context;
        const EDataValidationResult Result = Catalogue->IsDataValid(Context);
        TestEqual(TEXT("an entry no rule names raises no warning"),
                  static_cast<int32>(Context.GetNumWarnings()), 0);
        TestEqual(TEXT("and no error"), static_cast<int32>(Context.GetNumErrors()), 0);
        TestTrue(TEXT("so the asset passes"), Result != EDataValidationResult::Invalid);

        // The silence is not vacuous: the entry the validator said nothing about really does roll.
        TArray<FMythicTieredAffixDef> ForPotion;
        TestEqual(TEXT("a type matching no rule rolls every applicable entry"),
                  Catalogue->BuildRandomDefs(Potion, MakeProbe(Potion), ForPotion), 3);
        TestEqual(TEXT("the unnamed one included"), CountDefsFor(ForPotion, Armor), 1);
    }

    // The shape that makes reachability undecidable: a rule's tag is an ANCESTOR of the item's, so only a real
    // item's probe can say whether an entry gated deeper applies, and validation has no item.
    {
        UMythicAffixCatalogue *Catalogue = NewObject<UMythicAffixCatalogue>();
        FMythicAffixCatalogueEntry SwordGated = MakeEntry(FName("Affix_SwordOnly"), Power);
        SwordGated.Def.Applicability = FGameplayTagQuery::MakeQuery_MatchAnyTags(FGameplayTagContainer(Sword));
        Catalogue->Entries.Add(SwordGated);
        Catalogue->RulesByItemType.Add(MakeRule(Weapon));

        FDataValidationContext Context;
        const EDataValidationResult Result = Catalogue->IsDataValid(Context);
        TestEqual(TEXT("an entry gated below the rule that covers it is not condemned"),
                  static_cast<int32>(Context.GetNumWarnings()), 0);
        TestEqual(TEXT("and raises no error either"), static_cast<int32>(Context.GetNumErrors()), 0);
        TestTrue(TEXT("so a correctly wired catalogue passes review"), Result != EDataValidationResult::Invalid);

        // The silence is not vacuous: a real sword's probe does reach the entry the validator said nothing about.
        TArray<FMythicTieredAffixDef> ForSword;
        TestEqual(TEXT("and the entry really does roll on a sword"),
                  Catalogue->BuildRandomDefs(Sword, MakeProbe(Sword), ForSword), 1);
    }

    // The other direction: a rule naming an id the catalogue lacks.
    {
        UMythicAffixCatalogue *Catalogue = MakeClean();
        FMythicItemTypeAffixRule Ghosts = MakeRule(Axe);
        Ghosts.CoreAffixIds = {FName("Affix_GhostCore")};
        Ghosts.RandomAffixIds = {FName("Affix_GhostRandom")};
        Catalogue->RulesByItemType.Add(Ghosts);

        FDataValidationContext Context;
        const EDataValidationResult Result = Catalogue->IsDataValid(Context);
        TestEqual(TEXT("validation runs both ways: every unresolved id a rule names is an error"),
                  static_cast<int32>(Context.GetNumErrors()), 2);
        TestTrue(TEXT("the guaranteed list is checked"),
                 ReportMentions(Context, EMessageSeverity::Error, TEXT("Affix_GhostCore")));
        TestTrue(TEXT("and so is the random list"),
                 ReportMentions(Context, EMessageSeverity::Error, TEXT("Affix_GhostRandom")));
        TestTrue(TEXT("and it fails the asset"), Result == EDataValidationResult::Invalid);
    }

    // The denominator itself. Every count is a different number, so a swapped argument cannot pass.
    {
        UMythicAffixCatalogue *Catalogue = NewObject<UMythicAffixCatalogue>();
        Catalogue->Entries.Add(MakeEntry(FName("Affix_Power"), Power));
        Catalogue->Entries.Add(MakeEntry(FName("Affix_Crit"), Crit));
        Catalogue->Entries.Add(MakeEntry(FName("Affix_Armor"), Armor));
        Catalogue->Entries.Add(MakeEntry(FName("Affix_Spare1"), Power));
        Catalogue->Entries.Add(MakeEntry(FName("Affix_Spare2"), Crit));

        FMythicItemTypeAffixRule Explicit = MakeRule(Sword);
        Explicit.CoreAffixIds = {FName("Affix_Power")};
        Explicit.RandomAffixIds = {FName("Affix_Crit"), FName("Affix_Ghost")};
        Catalogue->RulesByItemType.Add(Explicit);

        FMythicItemTypeAffixRule CoreOnly = MakeRule(Axe);
        CoreOnly.CoreAffixIds = {FName("Affix_Armor")};
        Catalogue->RulesByItemType.Add(CoreOnly);

        Catalogue->RulesByItemType.Add(MakeRule(Spear));

        FDataValidationContext Context;
        Catalogue->IsDataValid(Context);

        FString Summary;
        for (const FDataValidationContext::FIssue &Issue : Context.GetIssues()) {
            if (Issue.Severity == EMessageSeverity::Info) {
                Summary = IssueText(Issue);
            }
        }
        if (TestFalse(TEXT("validation says how much it examined"), Summary.IsEmpty())) {
            TestTrue(TEXT("every catalogued entry is counted"), Summary.Contains(TEXT("5 entries")));
            TestTrue(TEXT("every rule, and every id those rules claim"),
                     Summary.Contains(TEXT("3 rules claiming 4 ids")));
            TestTrue(TEXT("the ids that resolved to nothing"), Summary.Contains(TEXT("1 unresolved")));
        }
        TestEqual(TEXT("and it makes no reachability claim, so the two entries nothing names are not reported"),
                  static_cast<int32>(Context.GetNumWarnings()), 0);
        TestEqual(TEXT("and the only error is the unresolved id"), static_cast<int32>(Context.GetNumErrors()), 1);
    }

    {
        UMythicAffixCatalogue *Catalogue = MakeClean();
        FMythicItemTypeAffixRule Tagless = MakeRule(FGameplayTag());
        Tagless.CoreAffixIds = {FName("Affix_GhostCore")};
        Catalogue->RulesByItemType.Add(Tagless);

        FDataValidationContext Context;
        const EDataValidationResult Result = Catalogue->IsDataValid(Context);
        TestEqual(TEXT("a rule with no ItemType tag can never match an item, so it is an error"),
                  static_cast<int32>(Context.GetNumErrors()), 1);
        TestFalse(TEXT("and it is the only one - a rule no item reaches claims nothing"),
                  ReportMentions(Context, EMessageSeverity::Error, TEXT("Affix_GhostCore")));
        TestTrue(TEXT("and fails the asset"), Result == EDataValidationResult::Invalid);
    }

    {
        UMythicAffixCatalogue *Catalogue = NewObject<UMythicAffixCatalogue>();
        FMythicAffixCatalogueEntry HighFloor = MakeEntry(FName("Affix_HighFloor"), Power);
        HighFloor.Def.Tiers[0].MinItemLevel = 10;
        Catalogue->Entries.Add(HighFloor);
        FMythicItemTypeAffixRule Rule = MakeRule(Sword);
        Rule.CoreAffixIds = {FName("Affix_HighFloor")};
        Catalogue->RulesByItemType.Add(Rule);

        FDataValidationContext Context;
        const EDataValidationResult Result = Catalogue->IsDataValid(Context);
        TestEqual(TEXT("a guaranteed ladder that starts above level 1 is an error - the roller masks it, the "
                       "designer should still hear about it"),
                  static_cast<int32>(Context.GetNumErrors()), 1);
        TestTrue(TEXT("and fails the asset"), Result == EDataValidationResult::Invalid);
    }

    // The random roller takes a catalogue tier's band verbatim, so an unauthored one rolls 0 and a Multiplicitive
    // affix then scales the attribute by KINDA_SMALL_NUMBER - equipping the item wipes the stat it advertises.
    {
        UMythicAffixCatalogue *Catalogue = MakeClean();
        FMythicAffixCatalogueEntry NoBand = MakeEntry(FName("Affix_NoBand"), Armor);
        NoBand.Def.Tiers.Add(FMythicAffixTier());
        Catalogue->Entries.Add(NoBand);

        FDataValidationContext Context;
        const EDataValidationResult Result = Catalogue->IsDataValid(Context);
        TestEqual(TEXT("a tier whose Min and Max are both zero is an error"),
                  static_cast<int32>(Context.GetNumErrors()), 1);
        TestTrue(TEXT("naming the affix and the rung it sits on"),
                 ReportMentions(Context, EMessageSeverity::Error, TEXT("'Affix_NoBand' tier 1")));
        TestTrue(TEXT("and fails the asset"), Result == EDataValidationResult::Invalid);
    }

    {
        UMythicAffixCatalogue *Catalogue = MakeClean();
        FMythicAffixCatalogueEntry Inverted = MakeEntry(FName("Affix_Inverted"), Armor);
        Inverted.Def.Tiers[0].Min = 5.0f;
        Inverted.Def.Tiers[0].Max = 1.0f;
        Catalogue->Entries.Add(Inverted);

        FDataValidationContext Context;
        const EDataValidationResult Result = Catalogue->IsDataValid(Context);
        TestEqual(TEXT("so is a tier whose Max sits below its Min"),
                  static_cast<int32>(Context.GetNumErrors()), 1);
        TestTrue(TEXT("and that report names its rung too"),
                 ReportMentions(Context, EMessageSeverity::Error, TEXT("'Affix_Inverted' tier 0")));
        TestTrue(TEXT("and fails the asset"), Result == EDataValidationResult::Invalid);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAffixFragmentValidationTest,
    "Mythic.Itemization.Affixes.FragmentValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMythicAffixFragmentValidationTest::RunTest(const FString &Parameters) {
    const FGameplayAttribute Power = UMythicAttributeSet_Offense::GetPowerAttribute();

    auto MakePoolFragment = [&](const TArray<FMythicAffixTier> &Tiers) -> UAffixesFragment * {
        FMythicTieredAffixDef Def = MakeDef(Power, EMythicAffixGroup::Prefix);
        Def.Tiers = Tiers;
        UMythicAffixPoolDataAsset *Pool = NewObject<UMythicAffixPoolDataAsset>();
        Pool->Defs.Add(Def);
        UAffixesFragment *Fragment = NewObject<UAffixesFragment>();
        Fragment->AffixesBuildData.TieredAffixPool = Pool;
        return Fragment;
    };

    FMythicAffixTier Authored;
    Authored.MinItemLevel = 1;
    Authored.Weight = 1.0f;
    Authored.Min = 1.0f;
    Authored.Max = 5.0f;

    // The denominator: an authored pool passes, so the two failures below are the band and nothing else.
    {
        FText Error;
        TestTrue(TEXT("a tiered pool with an authored band validates"),
                 MakePoolFragment({Authored})->IsValidFragment(Error));
    }

    {
        FText Error;
        TestFalse(TEXT("a tiered pool tier with Min and Max both zero fails the fragment"),
                  MakePoolFragment({Authored, FMythicAffixTier()})->IsValidFragment(Error));
        TestTrue(TEXT("naming the rung it sits on"), Error.ToString().Contains(TEXT("tier 1")));
    }

    {
        FMythicAffixTier Inverted = Authored;
        Inverted.Min = 5.0f;
        Inverted.Max = 1.0f;

        FText Error;
        TestFalse(TEXT("so does one whose Max sits below its Min"),
                  MakePoolFragment({Inverted})->IsValidFragment(Error));
        TestTrue(TEXT("and it names that rung too"), Error.ToString().Contains(TEXT("tier 0")));
    }

    // Authoring nothing is legal ONLY because the shared catalogue fills both halves. The precondition lives on
    // the loot settings CDO, so the test sets it both ways and puts the project's own value back.
    {
        UMythicLootSettings *Settings = GetMutableDefault<UMythicLootSettings>();
        const TSoftObjectPtr<UMythicAffixCatalogue> Configured = Settings->AffixCatalogue;
        Settings->AffixCatalogue.Reset();

        UAffixesFragment *Empty = NewObject<UAffixesFragment>();
        FText Error;
        TestFalse(TEXT("a fragment authoring no pool and no core stats fails when no catalogue can fill it"),
                  Empty->IsValidFragment(Error));
        TestFalse(TEXT("and says so"), Error.IsEmpty());

        UAffixesFragment *SelfAuthored = NewObject<UAffixesFragment>();
        SelfAuthored->AffixesBuildData.AffixPoolMap.Add(Power, MakeRoll());
        FText SelfAuthoredError;
        TestTrue(TEXT("while a fragment that authors its own pool needs no catalogue"),
                 SelfAuthored->IsValidFragment(SelfAuthoredError));

        // Only IsNull() is read: a configured path that would fail to LOAD is a different defect, and validation
        // must not force a synchronous package load to find out.
        Settings->AffixCatalogue = FSoftObjectPath(TEXT("/Game/Mythic/Itemization/Affixes/DA_AffixCatalogue.DA_AffixCatalogue"));
        FText WithCatalogue;
        TestTrue(TEXT("and the same empty fragment passes once a catalogue is configured"),
                 Empty->IsValidFragment(WithCatalogue));

        Settings->AffixCatalogue = Configured;
    }

    return true;
}
#endif

#endif
