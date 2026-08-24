// Copyright Stellar Games. All Rights Reserved.

#include "MythicProficiencyPageWidget.h"

#include <initializer_list>

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Mythic.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "UI/Widgets/MythicSectionHeader.h"
#include "Components/WrapBoxSlot.h"
#include "Engine/Texture2D.h"
#include "CommonTextBlock.h"
#include "UI/MythicUIKit.h"
#include "UI/MythicUIStyle.h"
#include "GameFramework/PlayerState.h"
#include "GAS/MythicTags_GAS.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Player/MythicPlayerController.h"
#include "Player/Proficiency/ProficiencyComponent.h"
#include "Player/Proficiency/ProficiencyDefinition.h"

namespace {
const FName Prof_Percent(TEXT("Percent"));
const FName Prof_ChipPercent(TEXT("ChipPercent"));
const FName Prof_FillStart(TEXT("FillColorStart"));
const FName Prof_FillEnd(TEXT("FillColorEnd"));
const FName Prof_Background(TEXT("BackgroundColor"));
const FName Prof_ChipAlpha(TEXT("ChipAlpha"));
const FLinearColor Prof_Track(0.14f, 0.11f, 0.075f, 1.0f);

constexpr float CardWidth = 432.0f;
constexpr float CardHeight = 132.0f;
constexpr float CardPadX = 18.0f;
constexpr float CardPadY = 16.0f;
constexpr float CardBarHeight = 12.0f;
constexpr float CardIconSize = 48.0f;

FSlateBrush MakeCardBrush() {
    // A track card is a grouped block, so it takes the catalogue's card plate rather than a path typed here.
    if (const UMythicUIKit *Kit = UMythicUIKit::Get()) {
        return Kit->MakeBrush(TEXT("Plate.Card"), EMythicUIState::Normal, FVector2D(432.0, 132.0));
    }
    FSlateBrush Brush;
    Brush.DrawAs = ESlateBrushDrawType::NoDrawType;
    return Brush;
}
}


namespace {
FMythicProficiencyFamily MakeFamily(const TCHAR *Label, std::initializer_list<const TCHAR *> Tags) {
    FMythicProficiencyFamily Family;
    Family.Label = FText::FromString(Label);
    for (const TCHAR *Tag : Tags) {
        Family.Tracks.Add(FGameplayTag::RequestGameplayTag(FName(Tag), false));
    }
    return Family;
}
}

UMythicProficiencyPageWidget::UMythicProficiencyPageWidget() {
    Families = {
        MakeFamily(TEXT("Battle & the Hunt"), {TEXT("Proficiency.Combat"), TEXT("Proficiency.Hunting"), TEXT("Proficiency.Fishing")}),
        MakeFamily(TEXT("Field & Forage"), {TEXT("Proficiency.Mining"), TEXT("Proficiency.Woodcutting"), TEXT("Proficiency.Harvesting"),
                                            TEXT("Proficiency.Farming"), TEXT("Proficiency.Husbandry"), TEXT("Proficiency.Beekeeping")}),
        MakeFamily(TEXT("Craft & Trade"), {TEXT("Proficiency.Cooking"), TEXT("Proficiency.Alchemy"), TEXT("Proficiency.Crafting"),
                                           TEXT("Proficiency.Construction"), TEXT("Proficiency.Trading")}),
        MakeFamily(TEXT("The Road"), {TEXT("Proficiency.Exploration"), TEXT("Proficiency.Riding")}),
    };
}

void UMythicProficiencyPageWidget::NativeConstruct() {
    EnsureSections();
    if (TrackList) {
        for (int32 i = RowPool.Num(); i < PrewarmRowCount; ++i) {
            FMythicProficiencyRow &Row = GetOrCreateRow(i);
            Row.Box->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    Super::NativeConstruct();
}

void UMythicProficiencyPageWidget::NativeOnActivated() {
    Super::NativeOnActivated();
    Bind();
    Refresh();
}

void UMythicProficiencyPageWidget::NativeOnDeactivated() {
    Unbind();
    Super::NativeOnDeactivated();
}

void UMythicProficiencyPageWidget::Bind() {
    if (bBound) {
        return;
    }
    APlayerController *PC = GetOwningPlayer();
    if (!PC) {
        return;
    }
    UAbilitySystemComponent *ASC = nullptr;
    if (APlayerState *PS = PC->PlayerState) {
        ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PS);
    }
    if (!ASC) {
        return;
    }
    EventHandle = ASC->AddGameplayEventTagContainerDelegate(
        FGameplayTagContainer(GAS_EVENT_PROFICIENCY_GAINED),
        FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &UMythicProficiencyPageWidget::HandleProficiencyEvent));
    bBound = true;
}

void UMythicProficiencyPageWidget::Unbind() {
    if (!bBound) {
        return;
    }
    if (const APlayerController *PC = GetOwningPlayer()) {
        if (APlayerState *PS = PC->PlayerState) {
            if (UAbilitySystemComponent *ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PS)) {
                ASC->RemoveGameplayEventTagContainerDelegate(FGameplayTagContainer(GAS_EVENT_PROFICIENCY_GAINED), EventHandle);
            }
        }
    }
    EventHandle.Reset();
    bBound = false;
}

void UMythicProficiencyPageWidget::HandleProficiencyEvent(FGameplayTag Tag, const FGameplayEventData *Payload) {
    Refresh();
}

FMythicProficiencyRow &UMythicProficiencyPageWidget::GetOrCreateRow(int32 Index) {
    if (RowPool.IsValidIndex(Index)) {
        return RowPool[Index];
    }

    FMythicProficiencyRow Row;
    /**
     * No width or height on the card.
     *
     * A uniform grid divides the page between its cells, so twelve tracks fill the height whatever the
     * viewport is. A SizeBox with typed pixels here is what left the bottom half of this page empty.
     */
    USizeBox *Box = WidgetTree->ConstructWidget<USizeBox>();
    Row.Box = Box;

    UOverlay *Layers = WidgetTree->ConstructWidget<UOverlay>();
    Box->AddChild(Layers);

    UImage *Sheet = WidgetTree->ConstructWidget<UImage>();
    Sheet->SetBrush(MakeCardBrush());
    Sheet->SetVisibility(ESlateVisibility::HitTestInvisible);
    if (UOverlaySlot *S = Cast<UOverlaySlot>(Layers->AddChild(Sheet))) {
        S->SetHorizontalAlignment(HAlign_Fill);
        S->SetVerticalAlignment(VAlign_Fill);
    }

    UVerticalBox *Column = WidgetTree->ConstructWidget<UVerticalBox>();
    if (UOverlaySlot *S = Cast<UOverlaySlot>(Layers->AddChild(Column))) {
        S->SetHorizontalAlignment(HAlign_Fill);
        // Centred, because the grid decides the cell's height. Content pinned to the top of a tall cell
        // reads as a card that failed to load its bottom half.
        S->SetVerticalAlignment(VAlign_Center);
        S->SetPadding(FMargin(CardPadX, CardPadY, CardPadX, CardPadY));
    }

    UHorizontalBox *Head = WidgetTree->ConstructWidget<UHorizontalBox>();

    Row.Icon = WidgetTree->ConstructWidget<UImage>();
    Row.Icon->SetVisibility(ESlateVisibility::Collapsed);
    // White silhouettes sit in the palette as aged brass, not stark white.
    Row.Icon->SetColorAndOpacity(FLinearColor(0.86f, 0.76f, 0.58f, 0.95f));
    if (UHorizontalBoxSlot *S = Cast<UHorizontalBoxSlot>(Head->AddChild(Row.Icon))) {
        S->SetVerticalAlignment(VAlign_Center);
        S->SetPadding(FMargin(0.0f, 0.0f, 14.0f, 0.0f));
    }

    Row.Name = FMythicUIStyle::MakeText(this, EMythicTextRole::Heading);
    if (UHorizontalBoxSlot *S = Cast<UHorizontalBoxSlot>(Head->AddChild(Row.Name))) {
        S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        S->SetVerticalAlignment(VAlign_Center);
    }
    Row.Level = FMythicUIStyle::MakeText(this, EMythicTextRole::Heading);
    Row.Level->SetJustification(ETextJustify::Right);
    if (UHorizontalBoxSlot *S = Cast<UHorizontalBoxSlot>(Head->AddChild(Row.Level))) {
        S->SetVerticalAlignment(VAlign_Center);
    }
    Column->AddChild(Head);

    Row.Bar = WidgetTree->ConstructWidget<UImage>();
    if (BarMaterialAsset) {
        FSlateBrush Brush;
        Brush.SetResourceObject(BarMaterialAsset);
        Brush.ImageSize = FVector2D(BarWidth, CardBarHeight);
        Row.Bar->SetBrush(Brush);
        Row.BarMaterial = Row.Bar->GetDynamicMaterial();
        if (Row.BarMaterial) {
            Row.BarMaterial->SetVectorParameterValue(Prof_FillStart, BarFillStart);
            Row.BarMaterial->SetVectorParameterValue(Prof_FillEnd, BarFillEnd);
            Row.BarMaterial->SetVectorParameterValue(Prof_Background, Prof_Track);
            Row.BarMaterial->SetScalarParameterValue(Prof_ChipAlpha, 0.0f);
        }
    }
    if (UVerticalBoxSlot *S = Cast<UVerticalBoxSlot>(Column->AddChild(Row.Bar))) {
        S->SetHorizontalAlignment(HAlign_Fill);
        S->SetPadding(FMargin(0.0f, 14.0f, 0.0f, 12.0f));
    }

    UHorizontalBox *Foot = WidgetTree->ConstructWidget<UHorizontalBox>();
    Row.Progress = FMythicUIStyle::MakeText(this, EMythicTextRole::Subtle);
    if (UHorizontalBoxSlot *S = Cast<UHorizontalBoxSlot>(Foot->AddChild(Row.Progress))) {
        S->SetVerticalAlignment(VAlign_Center);
    }
    Row.Milestone = FMythicUIStyle::MakeText(this, EMythicTextRole::Subtle);
    Row.Milestone->SetJustification(ETextJustify::Right);
    if (UHorizontalBoxSlot *S = Cast<UHorizontalBoxSlot>(Foot->AddChild(Row.Milestone))) {
        S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        S->SetVerticalAlignment(VAlign_Center);
        S->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));
    }
    Column->AddChild(Foot);

    Box->SetVisibility(ESlateVisibility::Collapsed);

    RowPool.Add(Row);
    return RowPool.Last();
}

void UMythicProficiencyPageWidget::EnsureSections() {
    if (SectionStack || !TrackList) {
        return;
    }
    TrackList->ClearChildren();
    SectionStack = WidgetTree->ConstructWidget<UVerticalBox>();
    if (UPanelSlot *Added = TrackList->AddChild(SectionStack)) {
        if (UUniformGridSlot *G = Cast<UUniformGridSlot>(Added)) {
            G->SetHorizontalAlignment(HAlign_Fill);
            G->SetVerticalAlignment(VAlign_Fill);
        }
    }

    // One header and one grid per authored family, plus a trailing section for anything unclaimed.
    const int32 SectionCount = Families.Num() + 1;
    for (int32 i = 0; i < SectionCount; ++i) {
        UMythicSectionHeader *Header = nullptr;
        if (SectionHeaderClass) {
            Header = CreateWidget<UMythicSectionHeader>(this, SectionHeaderClass);
            const FText Label = Families.IsValidIndex(i) ? Families[i].Label
                                                         : NSLOCTEXT("Mythic", "ProficiencyFamilyMore", "More");
            Header->SetHeader(Label, FText::GetEmpty(), nullptr);
            Header->SetVisibility(ESlateVisibility::Collapsed);
            if (UVerticalBoxSlot *S = Cast<UVerticalBoxSlot>(SectionStack->AddChild(Header))) {
                S->SetPadding(FMargin(0.0f, i == 0 ? 0.0f : 26.0f, 0.0f, 10.0f));
            }
        }
        SectionHeaders.Add(Header);

        UUniformGridPanel *Grid = WidgetTree->ConstructWidget<UUniformGridPanel>();
        Grid->SetSlotPadding(FMargin(0.0f, 0.0f, RowGap * 2.0f, RowGap));
        Grid->SetVisibility(ESlateVisibility::Collapsed);
        if (UVerticalBoxSlot *S = Cast<UVerticalBoxSlot>(SectionStack->AddChild(Grid))) {
            S->SetHorizontalAlignment(HAlign_Fill);
        }
        FamilyGrids.Add(Grid);
    }
}

void UMythicProficiencyPageWidget::ApplyRow(FMythicProficiencyRow &Row, const FProficiencySummary &Summary) {
    Row.Box->SetVisibility(ESlateVisibility::HitTestInvisible);

    if (Row.Icon) {
        // Twelve never-streamed UI textures, loaded the first time the page draws and then held by the brush.
        if (UTexture2D *Mark = Summary.Icon.LoadSynchronous()) {
            FSlateBrush Brush;
            Brush.SetResourceObject(Mark);
            Brush.DrawAs = ESlateBrushDrawType::Image;
            Brush.ImageSize = FVector2D(CardIconSize, CardIconSize);
            Row.Icon->SetBrush(Brush);
            Row.Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else {
            Row.Icon->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
    if (Row.Name) {
        Row.Name->SetText(Summary.Name);
        Row.Name->SetColorAndOpacity(FSlateColor(NameColor));
    }
    if (Row.Level) {
        Row.Level->SetText(FText::Format(NSLOCTEXT("Mythic", "ProfLevel", "Lv {0}"), FText::AsNumber(Summary.Level)));
        Row.Level->SetColorAndOpacity(FSlateColor(NameColor));
    }
    if (Row.BarMaterial) {
        const float Fraction = FMath::Clamp(Summary.ProgressFraction, 0.0f, 1.0f);
        Row.BarMaterial->SetScalarParameterValue(Prof_Percent, FMath::Max(Fraction, 0.015f));
        Row.BarMaterial->SetScalarParameterValue(Prof_ChipPercent, Fraction);
    }
    if (Row.Progress) {
        const float Into = Summary.CurrentXP - Summary.LevelXPStart;
        const float Needed = Summary.LevelXPEnd - Summary.LevelXPStart;
        Row.Progress->SetText(Needed > 0.0f
                                  ? FText::Format(NSLOCTEXT("Mythic", "ProfXP", "{0} / {1}"),
                                                  FText::AsNumber(FMath::FloorToInt(Into)),
                                                  FText::AsNumber(FMath::FloorToInt(Needed)))
                                  : NSLOCTEXT("Mythic", "ProfMaxed", "Mastered"));
        Row.Progress->SetColorAndOpacity(FSlateColor(SubtleColor));
    }
    if (Row.Milestone) {
        if (Summary.NextMilestoneName.IsEmpty()) {
            Row.Milestone->SetText(NSLOCTEXT("Mythic", "ProfNoMilestone", "All milestones earned"));
            Row.Milestone->SetColorAndOpacity(FSlateColor(SubtleColor));
        }
        else {
            Row.Milestone->SetText(FText::Format(NSLOCTEXT("Mythic", "ProfNextMilestone", "Next at Lv {1} — {0}"),
                                                 Summary.NextMilestoneName,
                                                 FText::AsNumber(Summary.NextMilestoneLevel)));
            Row.Milestone->SetColorAndOpacity(FSlateColor(SubtleColor));
        }
    }
}

void UMythicProficiencyPageWidget::Refresh() {
    if (!TrackList) {
        return;
    }
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer());
    if (!PC) {
        return;
    }

    const TArray<FProficiencySummary> Summaries = PC->GetProficiencySummaries();
    EnsureSections();

    // Bucket by family; a track no family claims lands in the trailing section rather than vanishing.
    TArray<TArray<int32>> Buckets;
    Buckets.SetNum(Families.Num() + 1);
    for (int32 i = 0; i < Summaries.Num(); ++i) {
        int32 Family = Families.Num();
        for (int32 f = 0; f < Families.Num(); ++f) {
            if (Families[f].Tracks.Contains(Summaries[i].TrackTag)) {
                Family = f;
                break;
            }
        }
        Buckets[Family].Add(i);
    }

    int32 RowIndex = 0;
    for (int32 f = 0; f < Buckets.Num(); ++f) {
        const bool bHasAny = Buckets[f].Num() > 0;
        if (SectionHeaders.IsValidIndex(f) && SectionHeaders[f]) {
            SectionHeaders[f]->SetVisibility(bHasAny ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
        }
        UUniformGridPanel *Grid = FamilyGrids.IsValidIndex(f) ? FamilyGrids[f].Get() : nullptr;
        if (Grid) {
            Grid->SetVisibility(bHasAny ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
        }
        for (int32 c = 0; c < Buckets[f].Num(); ++c) {
            FMythicProficiencyRow &Row = GetOrCreateRow(RowIndex++);
            // Re-parent only when the assignment changed: child order is the costliest invalidation there is.
            if (Grid && (Row.FamilyIndex != f || Row.CellIndex != c)) {
                if (UUniformGridSlot *G = Grid->AddChildToUniformGrid(Row.Box, c / TrackColumns, c % TrackColumns)) {
                    G->SetHorizontalAlignment(HAlign_Fill);
                    G->SetVerticalAlignment(VAlign_Fill);
                }
                Row.FamilyIndex = f;
                Row.CellIndex = c;
            }
            ApplyRow(Row, Summaries[Buckets[f][c]]);
        }
    }
    for (int32 i = RowIndex; i < RowPool.Num(); ++i) {
        if (RowPool[i].Box) {
            RowPool[i].Box->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if (Txt_Empty) {
        Txt_Empty->SetVisibility(Summaries.Num() == 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
}
