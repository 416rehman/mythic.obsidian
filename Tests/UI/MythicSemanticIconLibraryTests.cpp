#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Components/RichTextBlockImageDecorator.h"
#include "Engine/DataTable.h"
#include "UI/MythicRichTextIconLibrary.h"
#include "UI/Nameplate/MythicNameplateVisualStyle.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

namespace {

struct FSemanticIconExpectation {
    EMythicSemanticIcon Icon;
    FName RowName;
};

const FSemanticIconExpectation SemanticIconExpectations[] = {
    {EMythicSemanticIcon::AttentionFocus, TEXT("NP_Attention_Focus")},
    {EMythicSemanticIcon::AttentionInteraction,
     TEXT("NP_Attention_Interaction")},
    {EMythicSemanticIcon::AttentionSoftTarget,
     TEXT("NP_Attention_SoftTarget")},
    {EMythicSemanticIcon::AttentionHardTarget,
     TEXT("NP_Attention_HardTarget")},
    {EMythicSemanticIcon::AttentionLockedTarget,
     TEXT("NP_Attention_LockedTarget")},
    {EMythicSemanticIcon::CueTalk, TEXT("NP_Cue_Talk")},
    {EMythicSemanticIcon::CueQuestOffer, TEXT("NP_Cue_QuestOffer")},
    {EMythicSemanticIcon::CueQuestTurnIn, TEXT("NP_Cue_QuestTurnIn")},
    {EMythicSemanticIcon::CueService, TEXT("NP_Cue_Service")},
    {EMythicSemanticIcon::CueFaction, TEXT("NP_Cue_Faction")},
    {EMythicSemanticIcon::CueRole, TEXT("NP_Cue_Role")},
    {EMythicSemanticIcon::StateSurrender, TEXT("NP_State_Surrender")},
    {EMythicSemanticIcon::StateFleeing, TEXT("NP_State_Fleeing")},
    {EMythicSemanticIcon::StateAttacking, TEXT("NP_State_Attacking")},
    {EMythicSemanticIcon::StateDying, TEXT("NP_State_Dying")},
    {EMythicSemanticIcon::StateDowned, TEXT("NP_State_Downed")},
    {EMythicSemanticIcon::StateDead, TEXT("NP_State_Dead")},
    {EMythicSemanticIcon::RankSuperior, TEXT("NP_Rank_Superior")},
    {EMythicSemanticIcon::RankElite, TEXT("NP_Rank_Elite")},
    {EMythicSemanticIcon::RankChampion, TEXT("NP_Rank_Champion")},
    {EMythicSemanticIcon::RankBoss, TEXT("NP_Rank_Boss")},
    {EMythicSemanticIcon::RankWorldBoss, TEXT("NP_Rank_WorldBoss")},
    {EMythicSemanticIcon::ThreatRisky, TEXT("NP_Threat_Risky")},
    {EMythicSemanticIcon::ThreatDeadly, TEXT("NP_Threat_Deadly")},
    {EMythicSemanticIcon::ThreatOverwhelming,
     TEXT("NP_Threat_Overwhelming")},
    {EMythicSemanticIcon::RelationHostile, TEXT("NP_Relation_Hostile")},
};

UDataTable *LoadSemanticImageTable() {
    return LoadObject<UDataTable>(
        nullptr, TEXT("/Game/Mythic/UI/DT_ImageRow.DT_ImageRow"));
}

const FRichImageRow *FindSemanticImageRow(const UDataTable &ImageTable,
                                           const FName RowName) {
    return ImageTable.FindRow<FRichImageRow>(
        RowName, TEXT("Mythic semantic icon automation test"), false);
}

bool TestTokenMatchesRichImageRow(FAutomationTestBase &Test,
                                  const UDataTable &ImageTable,
                                  const FMythicNameplateIconToken &Token,
                                  const FString &Where) {
    bool bSuccess = true;
    bSuccess &= Test.TestTrue(*FString::Printf(TEXT("%s is renderable"), *Where),
                              Token.IsRenderable());

    const FName RowName =
        UMythicRichTextIconLibrary::GetSemanticIconRowName(Token.SemanticIcon);
    bSuccess &= Test.TestFalse(
        *FString::Printf(TEXT("%s has a Rich Text row"), *Where),
        RowName.IsNone());
    if (RowName.IsNone()) {
        return false;
    }

    const FRichImageRow *Row = FindSemanticImageRow(ImageTable, RowName);
    bSuccess &= Test.TestNotNull(
        *FString::Printf(TEXT("%s row '%s' exists"), *Where,
                            *RowName.ToString()),
        Row);
    if (!Row) {
        return false;
    }

    bSuccess &= Test.TestTrue(
        *FString::Printf(TEXT("%s uses the Rich Text row texture"), *Where),
        Row->Brush.GetResourceObject() == Token.Texture.Get());
    bSuccess &= Test.TestTrue(
        *FString::Printf(TEXT("%s uses the Rich Text row tint"), *Where),
        Token.Tint.Equals(Row->Brush.TintColor.GetSpecifiedColor()));

    const FVector2f RowSize = Row->Brush.GetImageSize();
    bSuccess &= Test.TestTrue(
        *FString::Printf(TEXT("%s uses the Rich Text row width"), *Where),
        FMath::IsNearlyEqual(Token.LogicalSize.X, RowSize.X));
    bSuccess &= Test.TestTrue(
        *FString::Printf(TEXT("%s uses the Rich Text row height"), *Where),
        FMath::IsNearlyEqual(Token.LogicalSize.Y, RowSize.Y));
    return bSuccess;
}

} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSemanticIconVocabularyTest,
    "Mythic.UI.RichText.SemanticIconVocabulary",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMythicSemanticIconVocabularyTest::RunTest(const FString &Parameters) {
    const UEnum *IconEnum = StaticEnum<EMythicSemanticIcon>();
    if (!TestNotNull(TEXT("semantic icon enum is reflected"), IconEnum)) {
        return false;
    }
    TestEqual(TEXT("every non-None semantic icon has an explicit expectation"),
              static_cast<int32>(UE_ARRAY_COUNT(SemanticIconExpectations)) + 1,
              static_cast<int32>(IconEnum->NumEnums()) - 1);

    TSet<uint8> SeenIcons;
    TSet<FName> SeenRows;
    for (const FSemanticIconExpectation &Expectation : SemanticIconExpectations) {
        const uint8 IconValue = static_cast<uint8>(Expectation.Icon);
        TestFalse(*FString::Printf(TEXT("semantic icon %d is listed once"),
                                   IconValue),
                  SeenIcons.Contains(IconValue));
        SeenIcons.Add(IconValue);
        TestFalse(*FString::Printf(TEXT("semantic row '%s' is listed once"),
                                   *Expectation.RowName.ToString()),
                  SeenRows.Contains(Expectation.RowName));
        SeenRows.Add(Expectation.RowName);
        TestEqual(*FString::Printf(TEXT("semantic icon %d maps to its stable row"),
                                   IconValue),
                  UMythicRichTextIconLibrary::GetSemanticIconRowName(
                      Expectation.Icon),
                  Expectation.RowName);

        const FString ExpectedMarkup = FString::Printf(
            TEXT("<img id=\"%s\"/>"), *Expectation.RowName.ToString());
        TestEqual(*FString::Printf(TEXT("semantic icon %d emits valid img markup"),
                                   IconValue),
                  UMythicRichTextIconLibrary::MakeSemanticIconMarkup(
                      Expectation.Icon)
                      .ToString(),
                  ExpectedMarkup);
    }

    TestTrue(TEXT("None has no Rich Text row"),
             UMythicRichTextIconLibrary::GetSemanticIconRowName(
                 EMythicSemanticIcon::None)
                 .IsNone());
    TestTrue(TEXT("None emits no Rich Text markup"),
             UMythicRichTextIconLibrary::MakeSemanticIconMarkup(
                 EMythicSemanticIcon::None)
                 .IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSemanticIconDataTableCoverageTest,
    "Mythic.UI.RichText.SemanticIconDataTableCoverage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMythicSemanticIconDataTableCoverageTest::RunTest(
    const FString &Parameters) {
    UDataTable *ImageTable = LoadSemanticImageTable();
    if (!TestNotNull(TEXT("the shipped Rich Text image table loads"),
                     ImageTable)) {
        return false;
    }
    TestTrue(TEXT("the image table uses RichImageRow entries"),
             ImageTable->GetRowStruct() == FRichImageRow::StaticStruct());

    for (const FSemanticIconExpectation &Expectation : SemanticIconExpectations) {
        const FRichImageRow *Row =
            FindSemanticImageRow(*ImageTable, Expectation.RowName);
        TestNotNull(*FString::Printf(TEXT("semantic row '%s' exists"),
                                    *Expectation.RowName.ToString()),
                    Row);
        if (Row) {
            TestNotNull(*FString::Printf(TEXT("semantic row '%s' has a texture"),
                                        *Expectation.RowName.ToString()),
                        Row->Brush.GetResourceObject());
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicNameplateSemanticIconParityTest,
    "Mythic.UI.Nameplate.VisualStyle.RichTextSemanticIconParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMythicNameplateSemanticIconParityTest::RunTest(
    const FString &Parameters) {
    UDataTable *ImageTable = LoadSemanticImageTable();
    const UMythicNameplateVisualStyle *Style =
        LoadObject<UMythicNameplateVisualStyle>(
            nullptr,
            TEXT("/Game/Mythic/UI/Nameplates/Policies/DA_NameplateVisualStyle_Default.DA_NameplateVisualStyle_Default"));
    if (!TestNotNull(TEXT("the shipped Rich Text image table loads"),
                     ImageTable)
        || !TestNotNull(TEXT("the default nameplate visual style loads"),
                        Style)) {
        return false;
    }

    TestEqual(TEXT("passive Whisper uses the 14 pixel identity profile"),
              Style->IdentityProfile.IdentityFontSize, 14);
    TestEqual(TEXT("compact Context uses the 15 pixel identity profile"),
              Style->ContextProfile.IdentityFontSize, 15);
    TestEqual(TEXT("standard Focus combat uses the 16 pixel identity profile"),
              Style->FocusCombatProfile.IdentityFontSize, 16);
    TestEqual(TEXT("elite combat uses the 17 pixel identity profile"),
              Style->EliteCombatProfile.IdentityFontSize, 17);
    TestEqual(TEXT("boss combat uses the 18 pixel identity profile"),
              Style->BossProfile.IdentityFontSize, 18);

    TArray<TPair<FString, const FMythicNameplateIconToken *>> Tokens;
    TestEqual(TEXT("default style has all live player-facing contextual cue slots"),
              Style->CueIcons.Num(), 11);
    TestFalse(TEXT("dead is never authored as a persistent nameplate cue"),
              Style->CueIcons.ContainsByPredicate(
                  [](const FMythicNameplateCueIconBinding &Binding) {
                      return Binding.Cue
                          == EMythicNameplatePrimaryCue::Dead;
                  }));
    TestEqual(TEXT("default style has all presented-rank slots"),
              Style->RankIcons.Num(), 5);
    TestEqual(TEXT("default style has all threat-warning slots"),
              Style->ThreatIcons.Num(), 3);
    for (int32 Index = 0; Index < Style->CueIcons.Num(); ++Index) {
        Tokens.Emplace(FString::Printf(TEXT("cue icon %d"), Index),
                       &Style->CueIcons[Index].Icon);
    }
    for (int32 Index = 0; Index < Style->RankIcons.Num(); ++Index) {
        Tokens.Emplace(FString::Printf(TEXT("rank icon %d"), Index),
                       &Style->RankIcons[Index].Icon);
    }
    for (int32 Index = 0; Index < Style->ThreatIcons.Num(); ++Index) {
        Tokens.Emplace(FString::Printf(TEXT("threat icon %d"), Index),
                       &Style->ThreatIcons[Index].Icon);
    }
    Tokens.Emplace(TEXT("hard target icon"), &Style->HardTargetIcon);
    Tokens.Emplace(TEXT("locked target icon"), &Style->LockedTargetIcon);

    TestNull(TEXT("ordinary camera Focus has no target icon"),
             Style->ResolveTargetIcon(
                 EMythicNameplateAttentionState::Focused));
    TestNotNull(TEXT("explicit hard target has one target icon"),
                Style->ResolveTargetIcon(
                    EMythicNameplateAttentionState::HardCombatTarget));

    TestEqual(TEXT("default style has one fixed token for every player-facing semantic"),
              Tokens.Num(), 21);
    for (const TPair<FString, const FMythicNameplateIconToken *> &Entry :
         Tokens) {
        if (Entry.Value) {
            TestTokenMatchesRichImageRow(*this, *ImageTable, *Entry.Value,
                                         Entry.Key);
        } else {
            AddError(FString::Printf(TEXT("%s has no icon token"),
                                     *Entry.Key));
        }
    }
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
