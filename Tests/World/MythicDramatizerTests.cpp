
#include "Misc/AutomationTest.h"
#include "World/LivingWorld/Chronicle/MythicDramatizerRules.h"
#include "World/LivingWorld/Chronicle/MythicDossierComponent.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDramatizerComposeTest,
    "Mythic.World.Dramatizer.ComposeBeatText",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicDramatizerComposeTest::RunTest(const FString &Parameters) {
    FMythicDramatizerConfig Config;

    const FGameplayTag FamineTag = TAG_LIVINGWORLD_EVENT_FACTION_FAMINE.GetTag();
    const FGameplayTag SchismTag = TAG_LIVINGWORLD_EVENT_FACTION_SCHISM.GetTag();

    TestEqual(TEXT("leaf of Faction.Famine"), FMythicDramatizerRules::EventLeaf(FamineTag), FString(TEXT("Famine")));
    TestEqual(TEXT("invalid tag leaf"), FMythicDramatizerRules::EventLeaf(FGameplayTag()), FString(TEXT("World Event")));

    {
        const FText Line = FMythicDramatizerRules::ComposeBeatText(FamineTag, FText::FromString(TEXT("The Ashfang")),
                                                                   FText(), 0.8f, Config);
        TestTrue(TEXT("default line carries the actor"), Line.ToString().Contains(TEXT("The Ashfang")));
        TestTrue(TEXT("default line carries the event leaf"), Line.ToString().Contains(TEXT("Famine")));
        TestFalse(TEXT("no unfilled {actor} token"), Line.ToString().Contains(TEXT("{actor}")));
    }

    {
        FMythicDramaTemplate Generic;
        Generic.TagPrefix = TEXT("LivingWorld.Event.Faction");
        Generic.Format = FText::FromString(TEXT("Trouble stirs in {actor}"));
        Config.Templates.Add(Generic);

        FMythicDramaTemplate Famine;
        Famine.TagPrefix = TEXT("LivingWorld.Event.Faction.Famine");
        Famine.Format = FText::FromString(TEXT("{actor} burned the granary at {target} — famine looms"));
        Config.Templates.Add(Famine);

        const FText Line = FMythicDramatizerRules::ComposeBeatText(FamineTag, FText::FromString(TEXT("The Ashfang")),
                                                                   FText::FromString(TEXT("Thornwick")), 0.9f, Config);
        TestEqual(TEXT("longest prefix wins + placeholders fill"),
                  Line.ToString(), FString(TEXT("The Ashfang burned the granary at Thornwick — famine looms")));

        const FText Sibling = FMythicDramatizerRules::ComposeBeatText(SchismTag, FText::FromString(TEXT("The Ashfang")),
                                                                      FText(), 0.9f, Config);
        TestEqual(TEXT("sibling event falls to the generic template"),
                  Sibling.ToString(), FString(TEXT("Trouble stirs in The Ashfang")));
    }

    {
        const FText Line = FMythicDramatizerRules::ComposeBeatText(FamineTag, FText(), FText(), 0.5f, Config);
        TestTrue(TEXT("empty actor fills with neutral text"), Line.ToString().Contains(TEXT("Unknown powers")));
    }

    return true;
}


namespace {
    FMythicChronicleEntry MakeEntry(const FGameplayTag &Tag, float Significance, int32 Sequence) {
        FMythicChronicleEntry Entry;
        Entry.EventTag = Tag;
        Entry.Significance = Significance;
        Entry.Sequence = Sequence;
        Entry.Text = FText::FromString(FString::Printf(TEXT("entry %d"), Sequence));
        return Entry;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDramatizerSelectTest,
    "Mythic.World.Dramatizer.SelectDramaticEntries",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicDramatizerSelectTest::RunTest(const FString &Parameters) {
    FMythicDramatizerConfig Config;

    const FGameplayTag WarTag = TAG_LIVINGWORLD_EVENT_TERRITORY_SETTLEMENT_TRANSFER.GetTag();
    const FGameplayTag FamineTag = TAG_LIVINGWORLD_EVENT_FACTION_FAMINE.GetTag();
    const FGameplayTag SchismTag = TAG_LIVINGWORLD_EVENT_FACTION_SCHISM.GetTag();

    TestEqual(TEXT("territory → war weight"), FMythicDramatizerRules::CategoryWeight(WarTag, Config), Config.WarWeight);
    TestEqual(TEXT("famine → economy weight"), FMythicDramatizerRules::CategoryWeight(FamineTag, Config), Config.EconomyWeight);
    TestEqual(TEXT("schism → default weight"), FMythicDramatizerRules::CategoryWeight(SchismTag, Config), Config.DefaultWeight);

    TArray<FMythicChronicleEntry> Feed;
    Feed.Add(MakeEntry(SchismTag, 1.0f, 1));
    Feed.Add(MakeEntry(FamineTag, 0.9f, 2));
    Feed.Add(MakeEntry(WarTag, 0.6f, 3));

    {
        const TArray<FMythicChronicleEntry> Picked = FMythicDramatizerRules::SelectDramaticEntries(Feed, 10, Config);
        TestEqual(TEXT("all pass through under the cap"), Picked.Num(), 3);
        TestEqual(TEXT("war beat first"), Picked[0].Sequence, 3);
        TestEqual(TEXT("economy beat second"), Picked[1].Sequence, 2);
        TestEqual(TEXT("minor beat last"), Picked[2].Sequence, 1);
    }

    {
        const TArray<FMythicChronicleEntry> Picked = FMythicDramatizerRules::SelectDramaticEntries(Feed, 2, Config);
        TestEqual(TEXT("capped to 2"), Picked.Num(), 2);
        TestEqual(TEXT("capped keeps the top score"), Picked[0].Sequence, 3);
        TestEqual(TEXT("capped keeps the runner-up"), Picked[1].Sequence, 2);
    }

    {
        TArray<FMythicChronicleEntry> Tie;
        Tie.Add(MakeEntry(SchismTag, 0.7f, 10));
        Tie.Add(MakeEntry(SchismTag, 0.7f, 20));
        const TArray<FMythicChronicleEntry> Picked = FMythicDramatizerRules::SelectDramaticEntries(Tie, 2, Config);
        TestEqual(TEXT("tie → newest first"), Picked[0].Sequence, 20);
        TestEqual(TEXT("tie → older second"), Picked[1].Sequence, 10);
    }

    TestEqual(TEXT("MaxCount 0 → empty"), FMythicDramatizerRules::SelectDramaticEntries(Feed, 0, Config).Num(), 0);
    TestEqual(TEXT("negative MaxCount → empty"), FMythicDramatizerRules::SelectDramaticEntries(Feed, -5, Config).Num(), 0);
    TestEqual(TEXT("empty feed → empty"),
              FMythicDramatizerRules::SelectDramaticEntries(TArray<FMythicChronicleEntry>(), 5, Config).Num(), 0);
    TestEqual(TEXT("input feed untouched"), Feed[0].Sequence, 1);

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDossierRulesTest,
    "Mythic.World.Dramatizer.Dossier",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicDossierRulesTest::RunTest(const FString &Parameters) {
    TArray<FMythicNpcDossier> Dossiers;

    TestNull(TEXT("zero hash → refused"), FMythicDossierRules::Upsert(Dossiers, 0, 1.0, 4));
    TestEqual(TEXT("zero hash files nothing"), Dossiers.Num(), 0);

    FMythicNpcDossier *Row = FMythicDossierRules::Upsert(Dossiers, 42, 1.0, 4);
    TestNotNull(TEXT("row created"), Row);
    TestEqual(TEXT("one row"), Dossiers.Num(), 1);
    Row = FMythicDossierRules::Upsert(Dossiers, 42, 2.0, 4);
    TestEqual(TEXT("upsert reuses the row"), Dossiers.Num(), 1);
    TestEqual(TEXT("touch time stamped"), Dossiers[0].LastUpdateTime, 2.0);

    {
        FMythicNpcRelation Relation;
        Relation.NpcNameHash = 42;
        Relation.Warmth = 17.0f;
        FMythicDossierRules::ApplyRelationEvent(*Row, Relation, EMythicNpcInteraction::Met);
        FMythicDossierRules::ApplyRelationEvent(*Row, Relation, EMythicNpcInteraction::Met);
        Relation.Warmth = 22.0f;
        FMythicDossierRules::ApplyRelationEvent(*Row, Relation, EMythicNpcInteraction::Traded);
        TestEqual(TEXT("two meetings"), Row->TimesMet, 2);
        TestEqual(TEXT("one deed"), Row->DeedsWitnessed, 1);
        TestEqual(TEXT("warmth snapshot follows the last event"), Row->LastKnownWarmth, 22.0f);
    }

    {
        FMythicDossierRules::Upsert(Dossiers, 43, 3.0, 3);
        FMythicDossierRules::Upsert(Dossiers, 44, 4.0, 3);
        TestEqual(TEXT("at cap"), Dossiers.Num(), 3);
        FMythicDossierRules::Upsert(Dossiers, 42, 5.0, 3);
        FMythicDossierRules::Upsert(Dossiers, 45, 6.0, 3);
        TestEqual(TEXT("still at cap"), Dossiers.Num(), 3);
        TestNull(TEXT("LRU row (43) evicted"), FMythicDossierRules::Find(Dossiers, 43));
        TestNotNull(TEXT("refreshed row (42) kept"), FMythicDossierRules::Find(Dossiers, 42));
        TestNotNull(TEXT("new row (45) added"), FMythicDossierRules::Find(Dossiers, 45));

        TestEqual(TEXT("survivor keeps TimesMet"), FMythicDossierRules::Find(Dossiers, 42)->TimesMet, 2);
    }

    return true;
}
