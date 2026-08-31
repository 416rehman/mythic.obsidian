#include "UI/Nameplate/MythicEntityInspectPage.h"

#include "Components/TextBlock.h"
#include "UI/Nameplate/MythicEntityInspectViewModel.h"

#define LOCTEXT_NAMESPACE "MythicEntityInspectPage"

namespace {
void ApplyInspectOptionalText(UTextBlock *Widget, const FText &Text) {
    if (!Widget) {
        return;
    }
    Widget->SetText(Text);
    Widget->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed
                                        : ESlateVisibility::HitTestInvisible);
}

FText BuildFactSection(
    const FText &Heading,
    const TArray<FMythicEntityInspectFactProjection> &Facts) {
    TArray<FText> Rows;
    Rows.Reserve(Facts.Num());
    for (const FMythicEntityInspectFactProjection &Fact : Facts) {
        if (Fact.ResolvedLabel.IsEmpty()) {
            continue;
        }
        Rows.Add(Fact.ResolvedDescription.IsEmpty()
                     ? Fact.ResolvedLabel
                     : FText::Format(LOCTEXT("FactDescriptionFormat", "{0} — {1}"),
                                     Fact.ResolvedLabel,
                                     Fact.ResolvedDescription));
    }
    if (Rows.IsEmpty()) {
        return FText::GetEmpty();
    }
    return FText::Format(
        LOCTEXT("FactSectionFormat", "{0}\n{1}"), Heading,
        FText::Join(LOCTEXT("FactSeparator", "\n"), Rows));
}

FText BuildThreatText(const FMythicEntityInspectProjection &Projection) {
    if (Projection.bBoss) {
        return LOCTEXT("BossThreat", "Boss");
    }
    if (!Projection.bCombatCapable) {
        return FText::GetEmpty();
    }
    switch (Projection.ThreatBand) {
    case EMythicThreatBand::None:
        return LOCTEXT("ThreatNone", "Combat capable");
    case EMythicThreatBand::Risky:
        return LOCTEXT("ThreatRisky", "Risky");
    case EMythicThreatBand::Deadly:
        return LOCTEXT("ThreatDeadly", "Deadly");
    case EMythicThreatBand::Overwhelming:
        return LOCTEXT("ThreatOverwhelming", "Overwhelming");
    default:
        return LOCTEXT("ThreatUnknown", "Combat capable");
    }
}
}

void UMythicEntityInspectPage::NativeOnInitialized() {
    Super::NativeOnInitialized();
    if (!ViewModel) {
        ViewModel = NewObject<UMythicEntityInspectViewModel>(this);
    }
}

void UMythicEntityInspectPage::NativeDestruct() {
    if (ViewModel) {
        ViewModel->Reset();
    }
    ResetNativeBindings();
    Super::NativeDestruct();
}

void UMythicEntityInspectPage::ApplyInspectProjection(
    const FMythicEntityInspectProjection &Projection) {
    if (!ViewModel) {
        ViewModel = NewObject<UMythicEntityInspectViewModel>(this);
    }
    ViewModel->Apply(Projection);
    RefreshNativeBindings();
    OnInspectProjectionChanged();
}

void UMythicEntityInspectPage::RefreshNativeBindings() {
    if (!ViewModel) {
        ResetNativeBindings();
        return;
    }

    const FMythicEntityInspectProjection &Projection =
        ViewModel->GetProjection();
    ApplyInspectOptionalText(NameText, Projection.ResolvedName);
    ApplyInspectOptionalText(RoleText, Projection.ResolvedRole);
    ApplyInspectOptionalText(FactionText, Projection.ResolvedFaction);
    ApplyInspectOptionalText(RelationshipText, Projection.ResolvedRelationship);
    ApplyInspectOptionalText(StandingText, Projection.ResolvedStanding);
    ApplyInspectOptionalText(ThreatText, BuildThreatText(Projection));
    ApplyInspectOptionalText(
        LevelText,
        Projection.bShowExactCombatLevel
            ? FText::Format(LOCTEXT("LevelFormat", "Level {0}"),
                            FText::AsNumber(Projection.ExactCombatLevel))
            : FText::GetEmpty());
    ApplyInspectOptionalText(
        TraitsText, BuildFactSection(LOCTEXT("TraitsHeading", "Traits"),
                                     Projection.Traits));
    ApplyInspectOptionalText(
        HistoryText, BuildFactSection(LOCTEXT("HistoryHeading", "History"),
                                      Projection.History));
    ApplyInspectOptionalText(
        LikesText, BuildFactSection(LOCTEXT("LikesHeading", "Likes"),
                                    Projection.Likes));
    ApplyInspectOptionalText(
        DislikesText, BuildFactSection(LOCTEXT("DislikesHeading", "Dislikes"),
                                       Projection.Dislikes));
    ApplyInspectOptionalText(ConnectionsText,
                             BuildFactSection(
                                 LOCTEXT("ConnectionsHeading", "Connections"),
                                 Projection.Connections));
}

void UMythicEntityInspectPage::ResetNativeBindings() {
    ApplyInspectOptionalText(NameText, FText::GetEmpty());
    ApplyInspectOptionalText(RoleText, FText::GetEmpty());
    ApplyInspectOptionalText(FactionText, FText::GetEmpty());
    ApplyInspectOptionalText(RelationshipText, FText::GetEmpty());
    ApplyInspectOptionalText(StandingText, FText::GetEmpty());
    ApplyInspectOptionalText(ThreatText, FText::GetEmpty());
    ApplyInspectOptionalText(LevelText, FText::GetEmpty());
    ApplyInspectOptionalText(TraitsText, FText::GetEmpty());
    ApplyInspectOptionalText(HistoryText, FText::GetEmpty());
    ApplyInspectOptionalText(LikesText, FText::GetEmpty());
    ApplyInspectOptionalText(DislikesText, FText::GetEmpty());
    ApplyInspectOptionalText(ConnectionsText, FText::GetEmpty());
}

#undef LOCTEXT_NAMESPACE
