
#include "Misc/AutomationTest.h"
#include "World/Fishing/MythicFishingTypes.h"
#include "World/Fishing/MythicTags_Fishing.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicFishingCatchTest,
    "Mythic.Fishing.Catch",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

static FMythicCatchTableEntry MakeEntry(float Weight, int32 MinProf = 0, float Override = 0.0f) {
    FMythicCatchTableEntry E;
    E.Weight = Weight;
    E.MinProficiency = MinProf;
    E.OverrideChance = Override;
    return E;
}

bool FMythicFishingCatchTest::RunTest(const FString &Parameters) {
    using namespace MythicFishing;

    const FGameplayTag TagTool = TAG_FieldActivity_Fishing;
    const FGameplayTag TagBait = TAG_FieldActivity_Drink;
    const FGameplayTag TagLoc = TAG_Surface_Water;

    {
        FMythicCatchContext Ctx;
        TArray<FMythicCatchTableEntry> Empty;
        TestEqual(TEXT("empty table → -1"), PickCatchIndex(Empty, Ctx, 0.5f), -1);

        TArray<FMythicCatchTableEntry> Gated = {MakeEntry(1.0f, 5)};
        Ctx.ProficiencyLevel = 0;
        TestEqual(TEXT("all below min-proficiency → -1"), PickCatchIndex(Gated, Ctx, 0.5f), -1);

        TArray<FMythicCatchTableEntry> ZeroW = {MakeEntry(0.0f)};
        TestEqual(TEXT("all eligible but zero weight → -1"), PickCatchIndex(ZeroW, Ctx, 0.5f), -1);
    }

    {
        TArray<FMythicCatchTableEntry> T = {MakeEntry(1.0f, 5)};
        FMythicCatchContext Ctx;
        Ctx.ProficiencyLevel = 4;
        TestEqual(TEXT("level 4 < min 5 → -1"), PickCatchIndex(T, Ctx, 0.5f), -1);
        Ctx.ProficiencyLevel = 5;
        TestEqual(TEXT("level 5 >= min 5 → eligible (index 0)"), PickCatchIndex(T, Ctx, 0.5f), 0);
    }

    {
        FMythicCatchTableEntry NeedTool = MakeEntry(1.0f);
        NeedTool.RequiredTool.AddTag(TagTool);
        FMythicCatchTableEntry NeedBait = MakeEntry(1.0f);
        NeedBait.RequiredBait.AddTag(TagBait);

        TArray<FMythicCatchTableEntry> Tool = {NeedTool};
        TArray<FMythicCatchTableEntry> Bait = {NeedBait};

        FMythicCatchContext Empty;
        TestEqual(TEXT("tool-gated, no tool owned → -1"), PickCatchIndex(Tool, Empty, 0.5f), -1);
        TestEqual(TEXT("bait-gated, no bait owned → -1"), PickCatchIndex(Bait, Empty, 0.5f), -1);

        FMythicCatchContext WithTool;
        WithTool.Tool.AddTag(TagTool);
        TestEqual(TEXT("tool-gated, tool owned → eligible"), PickCatchIndex(Tool, WithTool, 0.5f), 0);

        FMythicCatchContext WithBait;
        WithBait.Bait.AddTag(TagBait);
        TestEqual(TEXT("bait-gated, bait owned → eligible"), PickCatchIndex(Bait, WithBait, 0.5f), 0);
    }

    {
        FMythicCatchTableEntry RiverOnly = MakeEntry(1.0f);
        RiverOnly.Location.AddTag(TagLoc);
        TArray<FMythicCatchTableEntry> T = {RiverOnly};

        FMythicCatchContext NoLoc;
        TestEqual(TEXT("location-gated, spot has no location → -1"), PickCatchIndex(T, NoLoc, 0.5f), -1);

        FMythicCatchContext RiverLoc;
        RiverLoc.Location.AddTag(TagLoc);
        TestEqual(TEXT("location-gated, spot location matches → eligible"), PickCatchIndex(T, RiverLoc, 0.5f), 0);
    }

    {
        TArray<FMythicCatchTableEntry> T = {MakeEntry(1.0f), MakeEntry(3.0f)};
        FMythicCatchContext Ctx;

        TestEqual(TEXT("roll 0.0 → A (index 0)"), PickCatchIndex(T, Ctx, 0.0f), 0);
        TestEqual(TEXT("roll 0.24 (just below boundary) → A"), PickCatchIndex(T, Ctx, 0.24f), 0);
        TestEqual(TEXT("roll 0.25 (at boundary) → B (index 1)"), PickCatchIndex(T, Ctx, 0.25f), 1);
        TestEqual(TEXT("roll 0.99 → B"), PickCatchIndex(T, Ctx, 0.99f), 1);
        TestEqual(TEXT("roll 1.0 clamps → last eligible (B)"), PickCatchIndex(T, Ctx, 1.0f), 1);

        TestEqual(TEXT("deterministic: repeat roll 0.6 → same index"),
                  PickCatchIndex(T, Ctx, 0.6f), PickCatchIndex(T, Ctx, 0.6f));
    }

    {
        TArray<FMythicCatchTableEntry> T = {MakeEntry(1.0f), MakeEntry(3.0f, 0, 9.0f)};
        FMythicCatchContext Ctx;
        TestEqual(TEXT("override: roll 0.05 → A"), PickCatchIndex(T, Ctx, 0.05f), 0);
        TestEqual(TEXT("override: roll 0.15 → B (override widened B's band)"), PickCatchIndex(T, Ctx, 0.15f), 1);
    }

    {
        TArray<FMythicCatchTableEntry> T = {MakeEntry(5.0f, 10), MakeEntry(5.0f, 0)};
        FMythicCatchContext Ctx;
        Ctx.ProficiencyLevel = 3;
        TestEqual(TEXT("skips ineligible index 0 → returns 1 @roll 0.0"), PickCatchIndex(T, Ctx, 0.0f), 1);
        TestEqual(TEXT("skips ineligible index 0 → returns 1 @roll 0.99"), PickCatchIndex(T, Ctx, 0.99f), 1);
    }

    return true;
}
