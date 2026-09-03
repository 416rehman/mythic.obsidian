#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GAS/Feedback/MythicCombatTextTypes.h"
#include "GAS/MythicTags_GAS.h"
#include "UI/MythicDamageNumberSubsystem.h"
#include "UI/Settings/MythicUserSettings.h"

struct FMythicDamageNumberTestAccess {
    static const TArray<FMythicDamageNumberData> &Active(const UMythicDamageNumberSubsystem &Subsystem) {
        return Subsystem.ActiveDamageNumbers;
    }

    static FVector2D Offset(const UMythicDamageNumberSubsystem &Subsystem, const FMythicDamageNumberData &Data,
                            const float Time) {
        return Subsystem.CalculateAnimationOffset(Data, Time);
    }
};

namespace {

using FAccess = FMythicDamageNumberTestAccess;

const FLinearColor FirstStrikeColor(0.95f, 0.2f, 0.85f);
const FLinearColor RuneColor(0.2f, 0.9f, 0.95f);
const FLinearColor CalloutGold(1.0f, 0.8f, 0.2f);

// The subsystem hides whatever the machine's damage-number setting says to hide, so the fixture forces the
// show-everything mode for its lifetime and hands the previous mode back on the way out.
struct FMythicCombatTextFixture {
    UGameInstance *GameInstance = nullptr;
    UWorld *World = nullptr;
    UMythicDamageNumberSubsystem *Numbers = nullptr;
    UMythicDamageNumberConfig *Config = nullptr;
    AActor *Target = nullptr;
    AActor *Source = nullptr;
    uint8 PreviousMode = 1;
    bool bModeChanged = false;

    ~FMythicCombatTextFixture() {
        if (bModeChanged) {
            if (UMythicUserSettings *Settings = UMythicUserSettings::Get()) {
                Settings->SetDamageNumberMode(PreviousMode);
            }
        }
        if (GameInstance) {
            GameInstance->Shutdown();
        }
    }
};

bool BuildCombatTextFixture(FAutomationTestBase &Test, FMythicCombatTextFixture &Out) {
    if (!Test.TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }
    Out.GameInstance = NewObject<UGameInstance>(GEngine);
    Out.GameInstance->InitializeStandalone();
    Out.World = Out.GameInstance->GetWorld();
    if (!Test.TestNotNull(TEXT("standalone world exists"), Out.World)) {
        return false;
    }
    Out.Numbers = Out.World->GetSubsystem<UMythicDamageNumberSubsystem>();
    if (!Test.TestNotNull(TEXT("a game world owns the damage-number subsystem"), Out.Numbers)) {
        return false;
    }
    Out.Config = NewObject<UMythicDamageNumberConfig>();
    Out.Config->DefaultColor = FLinearColor::White;
    Out.Config->CriticalHitColor = FLinearColor::Yellow;
    Out.Config->DefaultAnimStyle = EMythicDamageNumberAnimStyle::FloatUp;
    Out.Config->CriticalAnimStyle = EMythicDamageNumberAnimStyle::Bounce;
    Out.Config->DefaultLifetime = 1.0f;
    Out.Config->MergeWindowSeconds = 0.075f;
    Out.Config->RandomHorizontalOffsetRange = 0.0f;
    Out.Config->RandomVerticalSpeedRange = 0.0f;
    Out.Numbers->SetConfig(Out.Config);
    Out.Numbers->ClearAll();

    Out.Target = Out.World->SpawnActor<AActor>();
    Out.Source = Out.World->SpawnActor<AActor>();
    if (!Test.TestNotNull(TEXT("the target actor spawned"), Out.Target)
        || !Test.TestNotNull(TEXT("the source actor spawned"), Out.Source)) {
        return false;
    }

    if (UMythicUserSettings *Settings = UMythicUserSettings::Get()) {
        Out.PreviousMode = Settings->GetDamageNumberMode();
        Settings->SetDamageNumberMode(2);
        Out.bModeChanged = true;
    }
    return true;
}

FMythicResolvedCombatTextEvent CombatTextTest_MakeHit(const FMythicCombatTextFixture &Fixture, const float Magnitude,
                                       const bool bCritical, const FGameplayTag PresentationTag = FGameplayTag()) {
    FMythicResolvedCombatTextEvent Event;
    Event.SourceActor = Fixture.Source;
    Event.TargetActor = Fixture.Target;
    Event.WorldLocation = Fixture.Target->GetActorLocation();
    Event.Magnitude = Magnitude;
    Event.Origin = EMythicCombatTextOrigin::DirectDamage;
    Event.bCritical = bCritical;
    Event.bOutgoingForViewer = true;
    Event.PresentationTag = PresentationTag;
    return Event;
}

const FMythicDamageNumberData *CombatTextTest_OnlyEntry(FAutomationTestBase &Test, const FMythicCombatTextFixture &Fixture,
                                         const TCHAR *What) {
    const TArray<FMythicDamageNumberData> &Active = FAccess::Active(*Fixture.Numbers);
    if (!Test.TestEqual(FString::Printf(TEXT("%s leaves exactly one entry"), What), Active.Num(), 1)) {
        return nullptr;
    }
    return &Active[0];
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCombatTextTaggedStyleTest,
    "Mythic.UI.CombatText.TaggedStyle",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCombatTextTaggedStyleTest::RunTest(const FString &Parameters) {
    FMythicCombatTextFixture Fixture;
    if (!BuildCombatTextFixture(*this, Fixture)) {
        return false;
    }
    UMythicDamageNumberSubsystem *Numbers = Fixture.Numbers;
    UMythicDamageNumberConfig *Config = Fixture.Config;
    Config->TaggedStyles.Add(GAS_HIT_RUNE_FIRSTSTRIKE,
                             FMythicCombatTextStyle(FirstStrikeColor, EMythicDamageNumberAnimStyle::ArcLeft, 1.4f, 1.5f));

    Numbers->AddResolvedCombatText(CombatTextTest_MakeHit(Fixture, 120.0f, true, GAS_HIT_RUNE_FIRSTSTRIKE));
    if (const FMythicDamageNumberData *Entry = CombatTextTest_OnlyEntry(*this, Fixture, TEXT("a tagged critical hit"))) {
        TestTrue(TEXT("a tagged critical hit takes the tagged colour, not the critical colour"),
                 Entry->Color.Equals(FirstStrikeColor));
        TestTrue(TEXT("a tagged hit takes the tagged motion"),
                 Entry->AnimStyle == EMythicDamageNumberAnimStyle::ArcLeft);
        TestEqual(TEXT("a tagged hit takes the tagged scale"), Entry->ScaleMultiplier, 1.4f);
        TestTrue(TEXT("a tagged hit takes the tagged lifetime"), FMath::IsNearlyEqual(Entry->Lifetime, 1.5f));
        TestTrue(TEXT("a tagged critical hit still reads as critical for the critical scale"), Entry->bIsCritical);
        TestTrue(TEXT("the entry remembers its presentation tag"), Entry->PresentationTag == GAS_HIT_RUNE_FIRSTSTRIKE);
    }

    Numbers->ClearAll();
    Numbers->AddResolvedCombatText(CombatTextTest_MakeHit(Fixture, 40.0f, false, GAS_HIT_RUNE_FIRSTSTRIKE));
    if (const FMythicDamageNumberData *Entry = CombatTextTest_OnlyEntry(*this, Fixture, TEXT("a tagged ordinary hit"))) {
        TestTrue(TEXT("a tagged ordinary hit takes the tagged colour"), Entry->Color.Equals(FirstStrikeColor));
        TestFalse(TEXT("a tagged ordinary hit is not critical"), Entry->bIsCritical);
    }

    Numbers->ClearAll();
    Numbers->AddResolvedCombatText(CombatTextTest_MakeHit(Fixture, 120.0f, true));
    if (const FMythicDamageNumberData *Entry = CombatTextTest_OnlyEntry(*this, Fixture, TEXT("an untagged critical hit"))) {
        TestTrue(TEXT("an untagged critical hit takes the critical colour"), Entry->Color.Equals(FLinearColor::Yellow));
        TestTrue(TEXT("an untagged critical hit takes the critical motion"),
                 Entry->AnimStyle == EMythicDamageNumberAnimStyle::Bounce);
        TestEqual(TEXT("an untagged critical hit keeps the base scale"), Entry->ScaleMultiplier, 1.0f);
        TestTrue(TEXT("an untagged critical hit keeps the base lifetime"), FMath::IsNearlyEqual(Entry->Lifetime, 1.0f));
        TestFalse(TEXT("an untagged entry carries no presentation tag"), Entry->PresentationTag.IsValid());
    }

    Numbers->ClearAll();
    Numbers->AddResolvedCombatText(CombatTextTest_MakeHit(Fixture, 40.0f, false));
    if (const FMythicDamageNumberData *Entry = CombatTextTest_OnlyEntry(*this, Fixture, TEXT("an untagged ordinary hit"))) {
        TestTrue(TEXT("an untagged ordinary hit takes the default colour"), Entry->Color.Equals(FLinearColor::White));
        TestTrue(TEXT("an untagged ordinary hit takes the default motion"),
                 Entry->AnimStyle == EMythicDamageNumberAnimStyle::FloatUp);
    }

    // One authored row on the parent tag covers every rune hit that has no row of its own.
    Config->TaggedStyles.Empty();
    Config->TaggedStyles.Add(GAS_HIT_RUNE,
                             FMythicCombatTextStyle(RuneColor, EMythicDamageNumberAnimStyle::Shake, 1.1f, 1.0f));
    Numbers->ClearAll();
    Numbers->AddResolvedCombatText(CombatTextTest_MakeHit(Fixture, 40.0f, true, GAS_HIT_RUNE_FIRSTSTRIKE));
    if (const FMythicDamageNumberData *Entry = CombatTextTest_OnlyEntry(*this, Fixture, TEXT("a hit tagged under an authored parent"))) {
        TestTrue(TEXT("a child tag with no row falls back to its parent's style"), Entry->Color.Equals(RuneColor));
        TestTrue(TEXT("the parent's motion comes with it"), Entry->AnimStyle == EMythicDamageNumberAnimStyle::Shake);
    }

    Config->TaggedStyles.Empty();
    Numbers->ClearAll();
    Numbers->AddResolvedCombatText(CombatTextTest_MakeHit(Fixture, 40.0f, true, GAS_HIT_RUNE_FIRSTSTRIKE));
    if (const FMythicDamageNumberData *Entry = CombatTextTest_OnlyEntry(*this, Fixture, TEXT("a tagged hit with no authored row"))) {
        TestTrue(TEXT("a tag nobody authored falls through to the critical style"),
                 Entry->Color.Equals(FLinearColor::Yellow));
        TestTrue(TEXT("the tag itself is still carried so it never merges with untagged numbers"),
                 Entry->PresentationTag == GAS_HIT_RUNE_FIRSTSTRIKE);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCombatTextTaggedMergeTest,
    "Mythic.UI.CombatText.TaggedMerge",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCombatTextTaggedMergeTest::RunTest(const FString &Parameters) {
    FMythicCombatTextFixture Fixture;
    if (!BuildCombatTextFixture(*this, Fixture)) {
        return false;
    }
    UMythicDamageNumberSubsystem *Numbers = Fixture.Numbers;
    Fixture.Config->TaggedStyles.Add(GAS_HIT_RUNE_FIRSTSTRIKE,
                                     FMythicCombatTextStyle(FirstStrikeColor, EMythicDamageNumberAnimStyle::ArcLeft,
                                                            1.4f, 1.0f));
    const TArray<FMythicDamageNumberData> &Active = FAccess::Active(*Numbers);

    Numbers->AddResolvedCombatText(CombatTextTest_MakeHit(Fixture, 10.0f, false));
    Numbers->AddResolvedCombatText(CombatTextTest_MakeHit(Fixture, 20.0f, false));
    if (TestEqual(TEXT("two untagged hits on one target in one frame merge"), Active.Num(), 1)) {
        TestEqual(TEXT("the merged number is the sum"), Active[0].Magnitude, 30.0f);
    }

    Numbers->ClearAll();
    Numbers->AddResolvedCombatText(CombatTextTest_MakeHit(Fixture, 10.0f, false));
    Numbers->AddResolvedCombatText(CombatTextTest_MakeHit(Fixture, 20.0f, false, GAS_HIT_RUNE_FIRSTSTRIKE));
    if (TestEqual(TEXT("a tagged hit never merges into an untagged one"), Active.Num(), 2)) {
        int32 TaggedIndex = INDEX_NONE;
        int32 UntaggedIndex = INDEX_NONE;
        for (int32 Index = 0; Index < Active.Num(); ++Index) {
            (Active[Index].PresentationTag.IsValid() ? TaggedIndex : UntaggedIndex) = Index;
        }
        if (TestTrue(TEXT("one entry is tagged and one is not"), TaggedIndex != INDEX_NONE && UntaggedIndex != INDEX_NONE)) {
            TestTrue(TEXT("the tagged entry keeps its own colour"), Active[TaggedIndex].Color.Equals(FirstStrikeColor));
            TestEqual(TEXT("the tagged entry keeps its own magnitude"), Active[TaggedIndex].Magnitude, 20.0f);
            TestTrue(TEXT("the untagged entry keeps the default colour"), Active[UntaggedIndex].Color.Equals(FLinearColor::White));
            TestEqual(TEXT("the untagged entry keeps its own magnitude"), Active[UntaggedIndex].Magnitude, 10.0f);
        }
    }

    Numbers->ClearAll();
    Numbers->AddResolvedCombatText(CombatTextTest_MakeHit(Fixture, 20.0f, false, GAS_HIT_RUNE_FIRSTSTRIKE));
    Numbers->AddResolvedCombatText(CombatTextTest_MakeHit(Fixture, 10.0f, false));
    TestEqual(TEXT("an untagged hit never merges into a tagged one either"), Active.Num(), 2);

    Numbers->ClearAll();
    Numbers->AddResolvedCombatText(CombatTextTest_MakeHit(Fixture, 20.0f, false, GAS_HIT_RUNE_FIRSTSTRIKE));
    Numbers->AddResolvedCombatText(CombatTextTest_MakeHit(Fixture, 30.0f, true, GAS_HIT_RUNE_FIRSTSTRIKE));
    if (TestEqual(TEXT("two hits under the same tag merge"), Active.Num(), 1)) {
        TestEqual(TEXT("the merged tagged number is the sum"), Active[0].Magnitude, 50.0f);
        TestTrue(TEXT("a merged critical keeps the tagged colour instead of turning critical yellow"),
                 Active[0].Color.Equals(FirstStrikeColor));
        TestTrue(TEXT("the merged entry still reads as critical for scale"), Active[0].bIsCritical);
    }

    Numbers->ClearAll();
    Numbers->AddResolvedCombatText(CombatTextTest_MakeHit(Fixture, 20.0f, false, GAS_HIT_RUNE_FIRSTSTRIKE));
    Numbers->AddResolvedCombatText(CombatTextTest_MakeHit(Fixture, 30.0f, false, GAS_HIT_RUNE));
    TestEqual(TEXT("hits under different tags never merge"), Active.Num(), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCombatTextCalloutVerbatimTest,
    "Mythic.UI.CombatText.CalloutVerbatim",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCombatTextCalloutVerbatimTest::RunTest(const FString &Parameters) {
    FMythicCombatTextFixture Fixture;
    if (!BuildCombatTextFixture(*this, Fixture)) {
        return false;
    }
    UMythicDamageNumberSubsystem *Numbers = Fixture.Numbers;
    UMythicDamageNumberConfig *Config = Fixture.Config;
    Config->CalloutStyle = FMythicCombatTextStyle(FLinearColor::White, EMythicDamageNumberAnimStyle::RiseAndSettle,
                                                  1.25f, 1.0f);
    Config->CalloutRiseHeight = 36.0f;
    Config->CalloutSettleSeconds = 0.35f;
    Config->CalloutOvershoot = 1.7f;
    Config->VerticalFloatSpeed = 50.0f;
    const TArray<FMythicDamageNumberData> &Active = FAccess::Active(*Numbers);
    const FVector Location = Fixture.Target->GetActorLocation() + FVector(0.0f, 0.0f, 90.0f);

    Numbers->AddCombatText(Location, FText::FromString(TEXT("CHEATED DEATH")), CalloutGold, 1.6f,
                           EMythicCombatTextOrigin::Callout);
    if (const FMythicDamageNumberData *Entry = CombatTextTest_OnlyEntry(*this, Fixture, TEXT("a callout"))) {
        TestEqual(TEXT("a callout keeps its text verbatim"), Entry->CachedText.ToString(), FString(TEXT("CHEATED DEATH")));
        TestTrue(TEXT("a callout is tagged with the Callout origin"), Entry->Origin == EMythicCombatTextOrigin::Callout);
        TestTrue(TEXT("a callout keeps the caller's colour"), Entry->Color.Equals(CalloutGold));
        TestTrue(TEXT("a callout takes the callout motion"),
                 Entry->AnimStyle == EMythicDamageNumberAnimStyle::RiseAndSettle);
        TestEqual(TEXT("a callout takes the callout scale"), Entry->ScaleMultiplier, 1.25f);
        TestTrue(TEXT("a callout keeps the caller's lifetime"), FMath::IsNearlyEqual(Entry->Lifetime, 1.6f));
        TestFalse(TEXT("a callout is not a resolved number"), Entry->bResolvedEvent);
        TestEqual(TEXT("a callout is not scattered sideways"), Entry->RandomOffsetX, 0.0f);

        const float T0 = Entry->SpawnTime;
        const float YAtSpawn = FAccess::Offset(*Numbers, *Entry, T0).Y;
        const float YAtPeak = FAccess::Offset(*Numbers, *Entry, T0 + 0.35f * 0.58f).Y;
        const float YAtRest = FAccess::Offset(*Numbers, *Entry, T0 + 0.35f).Y;
        const float YLater = FAccess::Offset(*Numbers, *Entry, T0 + 0.35f + 0.5f).Y;
        TestTrue(TEXT("a callout starts at its anchor"), FMath::IsNearlyZero(YAtSpawn, 0.01f));
        TestTrue(TEXT("a callout rises to its rest height"), FMath::IsNearlyEqual(YAtRest, -36.0f, 0.01f));
        TestTrue(TEXT("a callout overshoots its rest height on the way up"), YAtPeak < YAtRest);
        TestTrue(TEXT("a callout holds its rest height instead of drifting"), FMath::IsNearlyEqual(YLater, YAtRest, 0.01f));

        FMythicDamageNumberData Drifting = *Entry;
        Drifting.AnimStyle = EMythicDamageNumberAnimStyle::FloatUp;
        const float DriftAtRest = FAccess::Offset(*Numbers, Drifting, T0 + 0.35f).Y;
        const float DriftLater = FAccess::Offset(*Numbers, Drifting, T0 + 0.35f + 0.5f).Y;
        TestTrue(TEXT("the ordinary float style keeps drifting, so the hold is the callout's own behaviour"),
                 DriftLater < DriftAtRest - 1.0f);
    }

    Numbers->AddCombatText(Location, FText::FromString(TEXT("CHEATED DEATH")), CalloutGold, 1.6f,
                           EMythicCombatTextOrigin::Callout);
    TestEqual(TEXT("two identical callouts in one frame never merge"), Active.Num(), 2);

    Numbers->ClearAll();
    Numbers->AddCombatText(Location, FString(TEXT("Cheated death: -500 gold!")), CalloutGold, 1.0f,
                           EMythicCombatTextOrigin::Callout);
    if (const FMythicDamageNumberData *Entry = CombatTextTest_OnlyEntry(*this, Fixture, TEXT("a string callout"))) {
        TestEqual(TEXT("the string entry point keeps punctuation and digits verbatim"),
                  Entry->CachedText.ToString(), FString(TEXT("Cheated death: -500 gold!")));
        TestTrue(TEXT("the string entry point routes to the callout motion"),
                 Entry->AnimStyle == EMythicDamageNumberAnimStyle::RiseAndSettle);
    }

    Numbers->ClearAll();
    Numbers->AddCombatText(Location, FString(TEXT("Winded!")), CalloutGold, 1.0f);
    if (const FMythicDamageNumberData *Entry = CombatTextTest_OnlyEntry(*this, Fixture, TEXT("an ordinary combat text"))) {
        TestTrue(TEXT("combat text without an origin keeps the default motion"),
                 Entry->AnimStyle == EMythicDamageNumberAnimStyle::FloatUp);
        TestTrue(TEXT("combat text without an origin is not a callout"),
                 Entry->Origin != EMythicCombatTextOrigin::Callout);
    }

    Numbers->ClearAll();
    Numbers->AddCombatText(Location, FText::GetEmpty(), CalloutGold, 1.0f, EMythicCombatTextOrigin::Callout);
    Numbers->AddCombatText(Location, FText::FromString(TEXT("   ")), CalloutGold, 1.0f, EMythicCombatTextOrigin::Callout);
    TestEqual(TEXT("an empty or blank callout draws nothing"), Active.Num(), 0);

    AddExpectedError(TEXT("Rejected a callout through the resolved-number path"),
                     EAutomationExpectedErrorFlags::Contains, 1);
    FMythicResolvedCombatTextEvent NumberAsCallout = CombatTextTest_MakeHit(Fixture, 500.0f, false);
    NumberAsCallout.Origin = EMythicCombatTextOrigin::Callout;
    Numbers->AddResolvedCombatText(NumberAsCallout);
    TestEqual(TEXT("a magnitude cannot masquerade as a callout"), Active.Num(), 0);
    return true;
}

#endif
