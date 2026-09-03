#include "Misc/AutomationTest.h"

#include "Abilities/GameplayAbility.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameplayTagContainer.h"
#include "Itemization/Inventory/Fragments/FragmentTypes.h"
#include "Misc/ScopeExit.h"
#include "NativeGameplayTags.h"
#include "Player/MythicPlayerState.h"
#include "Progression/MythicAchievementDefinition.h"
#include "Progression/MythicAchievementSet.h"
#include "Progression/MythicUnlockRule.h"
#include "Progression/MythicUnlockRuleSet.h"
#include "Progression/Runes/MythicRuneComponent.h"
#include "Progression/Runes/MythicRuneDefinition.h"
#include "Settings/MythicDeveloperSettings.h"
#include "UI/ViewModels/MythicRuneDescriber.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_RuneDescriberTestDistance, "Rune.Param.Automation.Describer.Distance");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_RuneDescriberTestBonus, "Rune.Param.Automation.Describer.Bonus");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_RuneDescriberTestWindow, "Rune.Param.Automation.Describer.Window");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_RuneDescriberTestUnrolled, "Rune.Param.Automation.Describer.Unrolled");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_RuneDescriberTestTalentLeech, "Rune.Param.Automation.Describer.TalentLeech");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_RuneDescriberTestTalentPower, "Rune.Param.Automation.Describer.TalentPower");

namespace {
FGameplayTag DescriberRegisteredTag(const TCHAR *Name) {
    return FGameplayTag::RequestGameplayTag(FName(Name), false);
}

FString DescriberPlaceholder(const FGameplayTag &Tag) {
    return FString::Printf(TEXT("<#%s>"), *Tag.ToString());
}

FRollDefinition DescriberRoll(float Min, float Max, bool bWholeNumber, bool bIsPercentage, int32 Decimals, const TCHAR *Suffix) {
    FRollDefinition Roll;
    Roll.Min = Min;
    Roll.Max = Max;
    Roll.bWholeNumber = bWholeNumber;
    Roll.bIsPercentage = bIsPercentage;
    Roll.DisplayDecimals = Decimals;
    Roll.DisplaySuffix = FText::FromString(Suffix);
    return Roll;
}

// Vantage Strike's shape: a whole-number distance in metres, a percentage bonus, a sub-second window, and one
// placeholder the rune never rolls.
UMythicRuneDefinition *MakeDescriberRune() {
    UMythicRuneDefinition *Rune = NewObject<UMythicRuneDefinition>();
    Rune->Parameters.Add(TAG_RuneDescriberTestDistance, DescriberRoll(8.0f, 12.0f, true, false, 0, TEXT(" m")));
    Rune->Parameters.Add(TAG_RuneDescriberTestBonus, DescriberRoll(4.0f, 6.0f, false, true, 0, TEXT("")));
    Rune->Parameters.Add(TAG_RuneDescriberTestWindow, DescriberRoll(0.25f, 0.45f, false, false, 2, TEXT(" s")));
    Rune->Description = FText::FromString(FString::Printf(
        TEXT("Travel %s to arm your next strike: it deals %s extra damage. Raise your block within %s to parry. %s stays."),
        *DescriberPlaceholder(TAG_RuneDescriberTestDistance), *DescriberPlaceholder(TAG_RuneDescriberTestBonus),
        *DescriberPlaceholder(TAG_RuneDescriberTestWindow), *DescriberPlaceholder(TAG_RuneDescriberTestUnrolled)));
    return Rune;
}

FMythicRuneRollSet MakeDescriberRollSet(const UMythicRuneDefinition *Rune, float Distance, float Bonus, float Window) {
    FMythicRuneRollSet Set;
    Set.Rune = TSoftObjectPtr<UMythicRuneDefinition>(FSoftObjectPath(Rune));
    auto Roll = [&Set](const FGameplayTag &Parameter, float Value) {
        FMythicRuneRollValue &Rolled = Set.Values.AddDefaulted_GetRef();
        Rolled.Parameter = Parameter;
        Rolled.Value = Value;
    };
    Roll(TAG_RuneDescriberTestDistance.GetTag(), Distance);
    Roll(TAG_RuneDescriberTestBonus.GetTag(), Bonus);
    Roll(TAG_RuneDescriberTestWindow.GetTag(), Window);
    return Set;
}

// The describer reads an owner's rolls off its rune component, so the owner is the player state that carries one.
struct FMythicDescriberOwnerFixture {
    UGameInstance *GameInstance = nullptr;
    AMythicPlayerState *PlayerState = nullptr;
    UMythicRuneComponent *Runes = nullptr;
};

bool BuildDescriberOwner(FAutomationTestBase &Test, FMythicDescriberOwnerFixture &Out) {
    if (!Test.TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }
    Out.GameInstance = NewObject<UGameInstance>(GEngine);
    Out.GameInstance->InitializeStandalone();
    UWorld *World = Out.GameInstance->GetWorld();
    if (!Test.TestNotNull(TEXT("standalone world exists"), World)) {
        return false;
    }
    Out.PlayerState = World->SpawnActor<AMythicPlayerState>();
    if (!Test.TestNotNull(TEXT("the player state spawned"), Out.PlayerState)) {
        return false;
    }
    Out.Runes = Out.PlayerState->GetRuneComponent();
    if (!Test.TestNotNull(TEXT("the player state owns a rune component"), Out.Runes)) {
        return false;
    }
    if (!Out.Runes->IsRegistered()) {
        Out.Runes->RegisterComponent();
    }
    return Test.TestTrue(TEXT("the owner holds authority"), Out.PlayerState->HasAuthority());
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneDescriberBehaviourTest,
    "Mythic.UI.RuneDescriber.Behaviour",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneDescriberBehaviourTest::RunTest(const FString &Parameters) {
    FMythicDescriberOwnerFixture Fixture;
    const bool bReady = BuildDescriberOwner(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }

    UMythicRuneDefinition *Rune = MakeDescriberRune();
    const FString Unrolled = DescriberPlaceholder(TAG_RuneDescriberTestUnrolled);
    int32 Checked = 0;

    // No owner: every placeholder draws the middle of its range. 0.35 s is the sub-second case the decimals exist for.
    {
        const FString Midpoint = FString::Printf(
            TEXT("Travel <Roll>10 m</><Context>[8-12]</> to arm your next strike: it deals <Roll>500%%</><Context>[400-600]</> extra damage. "
                 "Raise your block within <Roll>0.35 s</><Context>[0.25-0.45]</> to parry. %s stays."),
            *Unrolled);
        TestEqual(TEXT("with no owner every placeholder reads its midpoint"),
                  UMythicRuneDescriber::DescribeBehaviour(Rune, nullptr).ToString(), Midpoint);
        TestEqual(TEXT("an owner that never socketed the rune reads the same midpoints"),
                  UMythicRuneDescriber::DescribeBehaviour(Rune, Fixture.Runes).ToString(), Midpoint);
        Checked += 2;
    }

    // The owner's own numbers, once it has them. Every roll sits off-centre so a midpoint could not pass for it.
    {
        Fixture.Runes->RestoreRuneRolls({MakeDescriberRollSet(Rune, 9.0f, 4.5f, 0.3f)});
        float Read = 0.0f;
        if (!TestTrue(TEXT("the owner now carries a roll for the rune"),
                      Fixture.Runes->GetRolledRuneValue(Rune, TAG_RuneDescriberTestWindow, Read))) {
            return false;
        }
        const FString Rolled = FString::Printf(
            TEXT("Travel <Roll>9 m</><Context>[8-12]</> to arm your next strike: it deals <Roll>450%%</><Context>[400-600]</> extra damage. "
                 "Raise your block within <Roll>0.30 s</><Context>[0.25-0.45]</> to parry. %s stays."),
            *Unrolled);
        TestEqual(TEXT("an owner with a roll set reads its own numbers against the range"),
                  UMythicRuneDescriber::DescribeBehaviour(Rune, Fixture.Runes).ToString(), Rolled);
        Checked += 1;
    }

    // The single-parameter case the contract names: 0.35, two decimals, suffix " s".
    {
        UMythicRuneDefinition *Window = NewObject<UMythicRuneDefinition>();
        Window->Parameters.Add(TAG_RuneDescriberTestWindow, DescriberRoll(0.25f, 0.45f, false, false, 2, TEXT(" s")));
        Window->Description = FText::FromString(FString::Printf(TEXT("Parry within %s."), *DescriberPlaceholder(TAG_RuneDescriberTestWindow)));
        TestEqual(TEXT("a sub-second roll renders with its decimals and unit"),
                  UMythicRuneDescriber::DescribeBehaviour(Window, nullptr).ToString(),
                  FString(TEXT("Parry within <Roll>0.35 s</><Context>[0.25-0.45]</>.")));
        Checked += 1;
    }

    // Text with nothing to draw passes through untouched, whether or not the rune rolls anything.
    {
        UMythicRuneDefinition *Plain = NewObject<UMythicRuneDefinition>();
        Plain->Description = FText::FromString(TEXT("You take no damage from falling."));
        TestEqual(TEXT("a description without placeholders passes through unchanged"),
                  UMythicRuneDescriber::DescribeBehaviour(Plain, Fixture.Runes).ToString(), Plain->Description.ToString());

        Plain->Parameters.Add(TAG_RuneDescriberTestDistance, DescriberRoll(8.0f, 12.0f, true, false, 0, TEXT(" m")));
        TestEqual(TEXT("a rolled parameter the text never mentions changes nothing"),
                  UMythicRuneDescriber::DescribeBehaviour(Plain, Fixture.Runes).ToString(), Plain->Description.ToString());

        UMythicRuneDefinition *OnlyUnknown = NewObject<UMythicRuneDefinition>();
        OnlyUnknown->Parameters.Add(TAG_RuneDescriberTestDistance, DescriberRoll(8.0f, 12.0f, true, false, 0, TEXT(" m")));
        OnlyUnknown->Description = FText::FromString(FString::Printf(TEXT("%s stays readable."), *Unrolled));
        TestEqual(TEXT("a placeholder the rune does not roll is left as written"),
                  UMythicRuneDescriber::DescribeBehaviour(OnlyUnknown, Fixture.Runes).ToString(),
                  FString::Printf(TEXT("%s stays readable."), *Unrolled));

        TestTrue(TEXT("no rune describes as nothing"), UMythicRuneDescriber::DescribeBehaviour(nullptr, Fixture.Runes).IsEmpty());
        Checked += 4;
    }

    AddInfo(FString::Printf(TEXT("renderings checked: %d over %d rolled parameters"), Checked, Rune->Parameters.Num()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneDescriberTalentRichTextTest,
    "Mythic.UI.RuneDescriber.TalentRichText",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneDescriberTalentRichTextTest::RunTest(const FString &Parameters) {
    // Talents roll through the same FAbilityDefinition::GetRichText. At the presentation defaults its output must
    // be what it always was: whole numbers, a % sign on a percentage value only, a bare range.
    FAbilityDefinition Talent;
    Talent.Ability = UGameplayAbility::StaticClass();
    Talent.ParameterRolls.Add(TAG_RuneDescriberTestTalentLeech, DescriberRoll(0.10f, 0.25f, false, true, 0, TEXT("")));
    Talent.ParameterRolls.Add(TAG_RuneDescriberTestTalentPower, DescriberRoll(4.0f, 9.0f, true, false, 0, TEXT("")));
    Talent.RichText = FText::FromString(FString::Printf(TEXT("%s of damage dealt returns as health and grants %s power"),
                                                        *DescriberPlaceholder(TAG_RuneDescriberTestTalentLeech),
                                                        *DescriberPlaceholder(TAG_RuneDescriberTestTalentPower)));

    TArray<FRolledTagSpec> Rolled;
    Rolled.Emplace(TAG_RuneDescriberTestTalentLeech, 0.15f);
    Rolled.Emplace(TAG_RuneDescriberTestTalentPower, 8.0f);

    FText Drawn;
    if (!TestTrue(TEXT("a talent's rich text still resolves"), Talent.GetRichText(Drawn, Rolled))) {
        return false;
    }
    TestEqual(TEXT("at the defaults a talent reads exactly as before"), Drawn.ToString(),
              FString(TEXT("<Roll>15%</><Context>[10-25]</> of damage dealt returns as health and grants <Roll>8</><Context>[4-9]</> power")));

    // A fractional value at zero decimals still prints whole, as it always has.
    Rolled[1].Value = 8.7f;
    TestTrue(TEXT("the rich text resolves with a fractional roll"), Talent.GetRichText(Drawn, Rolled));
    TestTrue(TEXT("a whole-number talent roll never shows a fraction"), Drawn.ToString().Contains(TEXT("<Roll>8</><Context>[4-9]</>")));

    // Today's contract for a roll the text never mentions is a refusal, which FAbilityRollSpec turns into "???".
    Rolled.Emplace(TAG_RuneDescriberTestDistance, 10.0f);
    TestFalse(TEXT("a rolled tag the text never mentions is still refused"), Talent.GetRichText(Drawn, Rolled));

    // A rune's presentation fields change the reading only where they are set.
    FAbilityDefinition Rune;
    Rune.ParameterRolls.Add(TAG_RuneDescriberTestWindow, DescriberRoll(0.25f, 0.45f, false, false, 2, TEXT(" s")));
    Rune.RichText = FText::FromString(DescriberPlaceholder(TAG_RuneDescriberTestWindow));
    TArray<FRolledTagSpec> Window;
    Window.Emplace(TAG_RuneDescriberTestWindow, 0.35f);
    TestTrue(TEXT("a rune's rich text resolves"), Rune.GetRichText(Drawn, Window));
    TestEqual(TEXT("decimals and a unit apply to the value, decimals alone to the range"), Drawn.ToString(),
              FString(TEXT("<Roll>0.35 s</><Context>[0.25-0.45]</>")));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneDescriberSealedSocketTest,
    "Mythic.UI.RuneDescriber.SealedSocket",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneDescriberSealedSocketTest::RunTest(const FString &Parameters) {
    int32 Checked = 0;

    // No rule set at all: the rule id is printed, never an empty string.
    {
        FText Deed;
        const FString Line = UMythicRuneDescriber::DescribeSealedSocket(1, nullptr, nullptr, Deed).ToString();
        TestEqual(TEXT("slot 1 asks for the second rune-slot rule"), UMythicRuneDescriber::SocketRuleId(1),
                  FString(TEXT("Unlock.Rule.RuneSlot2")));
        TestEqual(TEXT("no rule set prints the rule id"), Line, FString(TEXT("Sealed - earn Unlock.Rule.RuneSlot2")));
        TestEqual(TEXT("the deed name is the rule id"), Deed.ToString(), FString(TEXT("Unlock.Rule.RuneSlot2")));
        Checked += 3;
    }

    // Resolved through an in-memory rule set and achievement set.
    {
        const FGameplayTag RuleSlot2 = DescriberRegisteredTag(TEXT("Unlock.Rule.RuneSlot2"));
        const FGameplayTag RuleSlot3 = DescriberRegisteredTag(TEXT("Unlock.Rule.RuneSlot3"));
        const FGameplayTag Wanderer = DescriberRegisteredTag(TEXT("Achievement.Wanderer"));
        const FGameplayTag Slayer = DescriberRegisteredTag(TEXT("Achievement.Slayer"));
        if (TestTrue(TEXT("the rune-slot rule tags and deed tags are registered"),
                     RuleSlot2.IsValid() && RuleSlot3.IsValid() && Wanderer.IsValid() && Slayer.IsValid())) {
            UMythicUnlockRule *Slot2 = NewObject<UMythicUnlockRule>(GetTransientPackage());
            Slot2->RuleId = RuleSlot2;
            Slot2->Precondition.RequireAll.AddTag(Wanderer);
            UMythicUnlockRule *Slot3 = NewObject<UMythicUnlockRule>(GetTransientPackage());
            Slot3->RuleId = RuleSlot3;
            Slot3->Precondition.RequireAny.AddTag(Slayer);
            UMythicUnlockRuleSet *Rules = NewObject<UMythicUnlockRuleSet>(GetTransientPackage());
            Rules->Rules.Add(Slot2);
            Rules->Rules.Add(Slot3);

            UMythicAchievementDefinition *WandererDeed = NewObject<UMythicAchievementDefinition>(GetTransientPackage());
            WandererDeed->AchievementTag = Wanderer;
            WandererDeed->DisplayName = FText::FromString(TEXT("Wanderer"));
            WandererDeed->Description = FText::FromString(TEXT("Find ten places nobody sent you to."));
            UMythicAchievementSet *Deeds = NewObject<UMythicAchievementSet>(GetTransientPackage());
            Deeds->Achievements.Add(WandererDeed);

            FText Deed;
            TestEqual(TEXT("a matched deed reads name and description"),
                      UMythicRuneDescriber::DescribeSealedSocket(1, Rules, Deeds, Deed).ToString(),
                      FString(TEXT("Sealed - earn Wanderer: Find ten places nobody sent you to.")));
            TestEqual(TEXT("the deed name is the achievement's display name"), Deed.ToString(), FString(TEXT("Wanderer")));

            TestEqual(TEXT("a precondition tag with no achievement prints the tag"),
                      UMythicRuneDescriber::DescribeSealedSocket(2, Rules, Deeds, Deed).ToString(),
                      FString(TEXT("Sealed - earn Achievement.Slayer")));
            TestEqual(TEXT("and names the tag as the deed"), Deed.ToString(), FString(TEXT("Achievement.Slayer")));

            TestEqual(TEXT("a slot with no rule in the set prints its rule id"),
                      UMythicRuneDescriber::DescribeSealedSocket(3, Rules, Deeds, Deed).ToString(),
                      FString(TEXT("Sealed - earn Unlock.Rule.RuneSlot4")));
            Checked += 5;
        }
    }

    // The shipped data: the describer must agree with what the sockets actually load.
    {
        const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
        const UMythicUnlockRuleSet *Rules = Settings ? Settings->DefaultUnlockRuleSet.LoadSynchronous() : nullptr;
        const UMythicAchievementSet *Deeds = Settings ? Settings->DefaultAchievementSet.LoadSynchronous() : nullptr;
        if (TestNotNull(TEXT("the default unlock rule set loads"), Rules)
            && TestNotNull(TEXT("the default achievement set loads"), Deeds)) {
            FText Deed;
            TestEqual(TEXT("socket 2 is sealed behind Wanderer"),
                      UMythicRuneDescriber::DescribeSealedSocket(1, Rules, Deeds, Deed).ToString(),
                      FString(TEXT("Sealed - earn Wanderer: Find ten places nobody sent you to.")));
            Checked += 1;
        }
    }

    AddInfo(FString::Printf(TEXT("sealed-socket lines checked: %d"), Checked));
    return true;
}
