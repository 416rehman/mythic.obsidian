// Copyright Stellar Games. All Rights Reserved.

#include "MythicPowersPageWidget.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"

#include "Mythic/Mythic.h"
#include "Player/MythicPlayerState.h"
#include "Progression/Skills/MythicSkillComponent.h"
#include "Progression/Skills/MythicSkillDefinition.h"
#include "UI/Widgets/MythicSectionHeader.h"

#define LOCTEXT_NAMESPACE "MythicPowers"

void UMythicPowerRowProxy::HandleClicked() {
    if (UMythicPowersPageWidget *P = Page.Get()) {
        P->SelectRow(RowIndex);
    }
}

void UMythicModifierRowProxy::HandleClicked() {
    if (UMythicPowersPageWidget *P = Page.Get()) {
        P->ToggleModifier(ModifierIndex);
    }
}

void UMythicPowersPageWidget::NativeConstruct() {
    // Pools fill before Super, which activates the page and can draw immediately.
    LoadLibrary();
    BuildRows();
    Super::NativeConstruct();
}

void UMythicPowersPageWidget::NativeOnActivated() {
    Super::NativeOnActivated();

    if (UMythicSkillComponent *Skills = GetSkills()) {
        Skills->OnSkillsChanged.AddUniqueDynamic(this, &UMythicPowersPageWidget::RefreshRows);
    }

    RefreshRows();
    RefreshDetails();
}

void UMythicPowersPageWidget::NativeOnDeactivated() {
    if (UMythicSkillComponent *Skills = GetSkills()) {
        Skills->OnSkillsChanged.RemoveDynamic(this, &UMythicPowersPageWidget::RefreshRows);
    }

    Super::NativeOnDeactivated();
}

UWidget *UMythicPowersPageWidget::NativeGetDesiredFocusTarget() const {
    for (const FPowerRow &Row : Rows) {
        if (Row.bUnlocked && Row.Widget) {
            return Row.Widget;
        }
    }
    return Rows.Num() > 0 ? Rows[0].Widget.Get() : Super::NativeGetDesiredFocusTarget();
}

UMythicSkillComponent *UMythicPowersPageWidget::GetSkills() const {
    const APlayerController *PC = GetOwningPlayer();
    AMythicPlayerState *PS = PC ? PC->GetPlayerState<AMythicPlayerState>() : nullptr;
    return PS ? PS->FindComponentByClass<UMythicSkillComponent>() : nullptr;
}

void UMythicPowersPageWidget::LoadLibrary() {
    if (Library.Num() > 0) {
        return;
    }

    FAssetRegistryModule &Module = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    TArray<FAssetData> Assets;
    Module.Get().GetAssetsByClass(UMythicSkillDefinition::StaticClass()->GetClassPathName(), Assets, true);
    for (const FAssetData &Asset : Assets) {
        if (UMythicSkillDefinition *Skill = Cast<UMythicSkillDefinition>(Asset.GetAsset())) {
            Library.Add(Skill);
        }
    }
    Library.Sort([](const UMythicSkillDefinition &A, const UMythicSkillDefinition &B) {
        return A.Name.ToString() < B.Name.ToString();
    });
}

void UMythicPowersPageWidget::BindFirstButton(UUserWidget *RowWidget, UObject *Proxy, FName FunctionName) {
    if (!RowWidget || !RowWidget->WidgetTree) {
        return;
    }
    // Any button in the row drives the row, so a designer lays the row out however it reads best.
    TArray<UWidget *> Children;
    RowWidget->WidgetTree->GetAllWidgets(Children);
    for (UWidget *Child : Children) {
        if (UButton *Button = Cast<UButton>(Child)) {
            FScriptDelegate Delegate;
            Delegate.BindUFunction(Proxy, FunctionName);
            Button->OnClicked.Add(Delegate);
            return;
        }
    }
}

void UMythicPowersPageWidget::SetRowText(UUserWidget *RowWidget, FName SlotName, const FText &Text) const {
    if (!RowWidget) {
        return;
    }
    if (UTextBlock *Block = Cast<UTextBlock>(RowWidget->GetWidgetFromName(SlotName))) {
        Block->SetText(Text);
    }
}

void UMythicPowersPageWidget::BuildRows() {
    if (!AbilityHost || !AbilityRowClass || Rows.Num() > 0) {
        return;
    }

    if (SectionHeaderClass && !AbilityHeader) {
        AbilityHeader = CreateWidget<UMythicSectionHeader>(GetOwningPlayer(), SectionHeaderClass);
        if (AbilityHeader) {
            AbilityHeader->SetHeader(LOCTEXT("Abilities", "Abilities"), FText::GetEmpty(), nullptr);
            AbilityHost->AddChild(AbilityHeader);
        }
    }

    for (int32 Index = 0; Index < Library.Num(); ++Index) {
        UUserWidget *RowWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), AbilityRowClass);
        if (!RowWidget) {
            continue;
        }

        FPowerRow Row;
        Row.Widget = RowWidget;
        Row.Skill = Library[Index];
        Row.Proxy = NewObject<UMythicPowerRowProxy>(this);
        Row.Proxy->Page = this;
        Row.Proxy->RowIndex = Index;
        BindFirstButton(RowWidget, Row.Proxy, GET_FUNCTION_NAME_CHECKED(UMythicPowerRowProxy, HandleClicked));

        AbilityHost->AddChild(RowWidget);
        Rows.Add(Row);
    }
}

void UMythicPowersPageWidget::RefreshRows() {
    const UMythicSkillComponent *Skills = GetSkills();

    for (FPowerRow &Row : Rows) {
        if (!Row.Widget || !Row.Skill) {
            continue;
        }
        Row.bUnlocked = Skills && Skills->IsSkillUnlocked(Row.Skill);

        SetRowText(Row.Widget, RowNameText, Row.Skill->Name);

        // The detail line carries the one thing that changes: where it is bound, or what earns it.
        FText Detail = Row.Skill->Hint;
        if (Row.bUnlocked && Skills) {
            int32 BoundSlot = INDEX_NONE;
            for (int32 SlotIndex = 0; SlotIndex < Skills->GetUnlockedSlots(); ++SlotIndex) {
                if (Skills->GetSkillInSlot(SlotIndex) == Row.Skill) {
                    BoundSlot = SlotIndex;
                    break;
                }
            }
            const int32 Level = Skills->GetSkillLevel(Row.Skill);
            const int32 Unspent = Skills->GetAvailablePoints(Row.Skill);
            if (BoundSlot != INDEX_NONE) {
                Detail = FText::Format(LOCTEXT("BoundDetail", "Level {0} - bound to slot {1}"),
                                       FText::AsNumber(Level), FText::AsNumber(BoundSlot + 1));
            }
            else {
                Detail = FText::Format(LOCTEXT("LevelDetail", "Level {0}"), FText::AsNumber(Level));
            }
            if (Unspent > 0) {
                Detail = FText::Format(LOCTEXT("UnspentDetail", "{0} - {1} unspent"), Detail, FText::AsNumber(Unspent));
            }
        }
        SetRowText(Row.Widget, RowDetailText, Detail);

        if (UImage *Icon = Cast<UImage>(Row.Widget->GetWidgetFromName(RowIconImage))) {
            if (UTexture2D *Texture = Row.Skill->Icon.LoadSynchronous()) {
                Icon->SetBrushFromTexture(Texture, true);
            }
        }

        Row.Widget->SetRenderOpacity(Row.bUnlocked ? 1.0f : LockedRowOpacity);
        Row.Widget->SetIsEnabled(Row.bUnlocked);
    }

    if (AbilityHeader) {
        int32 Unlocked = 0;
        for (const FPowerRow &Row : Rows) {
            Unlocked += Row.bUnlocked ? 1 : 0;
        }
        AbilityHeader->SetHeader(LOCTEXT("Abilities", "Abilities"),
                                 FText::Format(LOCTEXT("AbilityCount", "{0} / {1}"),
                                               FText::AsNumber(Unlocked), FText::AsNumber(Rows.Num())),
                                 nullptr);
    }

    RefreshDetails();
}

void UMythicPowersPageWidget::SelectRow(int32 RowIndex) {
    if (!Rows.IsValidIndex(RowIndex) || !Rows[RowIndex].bUnlocked) {
        return;
    }
    SelectedRow = RowIndex;
    RefreshDetails();
}

void UMythicPowersPageWidget::RefreshDetails() {
    const bool bHasSelection = Rows.IsValidIndex(SelectedRow) && Rows[SelectedRow].Skill != nullptr;
    const UMythicSkillDefinition *Skill = bHasSelection ? Rows[SelectedRow].Skill.Get() : nullptr;
    const UMythicSkillComponent *Skills = GetSkills();

    if (DetailsPlaceholder) {
        DetailsPlaceholder->SetVisibility(bHasSelection ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    }

    if (Txt_SelectedName) {
        Txt_SelectedName->SetText(Skill ? Skill->Name : FText::GetEmpty());
    }
    if (Txt_SelectedDescription) {
        Txt_SelectedDescription->SetText(Skill ? Skill->Description : FText::GetEmpty());
    }
    if (Img_SelectedIcon && Skill) {
        if (UTexture2D *Texture = Skill->Icon.LoadSynchronous()) {
            Img_SelectedIcon->SetBrushFromTexture(Texture, true);
        }
    }
    if (Txt_Progress) {
        FText Progress = FText::GetEmpty();
        if (Skill && Skills) {
            const int32 Level = Skills->GetSkillLevel(Skill);
            const int32 Ceiling = Skills->GetMaxSkillLevel(Skill);
            const int32 Unspent = Skills->GetAvailablePoints(Skill);
            const int32 Uses = Skills->GetSkillUses(Skill);
            const int32 Needed = Skills->GetUsesForNextLevel(Skill);
            Progress = Level >= Ceiling
                           ? FText::Format(LOCTEXT("Mastered", "Level {0} - mastered - {1} unspent"),
                                           FText::AsNumber(Level), FText::AsNumber(Unspent))
                           : FText::Format(LOCTEXT("Progress", "Level {0} of {1} - {2}/{3} casts - {4} unspent"),
                                           FText::AsNumber(Level), FText::AsNumber(Ceiling), FText::AsNumber(Uses),
                                           FText::AsNumber(Needed), FText::AsNumber(Unspent));
        }
        Txt_Progress->SetText(Progress);
    }

    if (!ModifierHost || !ModifierRowClass) {
        return;
    }

    const int32 Wanted = Skill ? Skill->Modifiers.Num() : 0;

    // Rows are built once and re-texted after. Clearing and re-adding children is the most expensive thing a
    // widget can do, and a modifier list redraws on every point spent.
    while (ModifierRows.Num() < Wanted) {
        UUserWidget *RowWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), ModifierRowClass);
        if (!RowWidget) {
            break;
        }
        FModifierRow Row;
        Row.Widget = RowWidget;
        Row.Proxy = NewObject<UMythicModifierRowProxy>(this);
        Row.Proxy->Page = this;
        Row.Proxy->ModifierIndex = ModifierRows.Num();
        BindFirstButton(RowWidget, Row.Proxy, GET_FUNCTION_NAME_CHECKED(UMythicModifierRowProxy, HandleClicked));
        ModifierHost->AddChild(RowWidget);
        ModifierRows.Add(Row);
    }

    for (int32 Index = 0; Index < ModifierRows.Num(); ++Index) {
        UUserWidget *RowWidget = ModifierRows[Index].Widget;
        if (!RowWidget) {
            continue;
        }
        if (Index >= Wanted) {
            RowWidget->SetVisibility(ESlateVisibility::Collapsed);
            continue;
        }

        const FMythicSkillModifier &Modifier = Skill->Modifiers[Index];
        const bool bActive = Skills && Skills->IsModifierActive(Skill, Index);
        const bool bAffordable = Skills && Skills->GetAvailablePoints(Skill) >= Modifier.PointCost;

        RowWidget->SetVisibility(ESlateVisibility::Visible);
        SetRowText(RowWidget, RowNameText, Modifier.Name);
        SetRowText(RowWidget, RowDetailText,
                   bActive ? FText::Format(LOCTEXT("ActiveModifier", "Active - {0}"), Modifier.Description)
                           : Modifier.Description);
        RowWidget->SetRenderOpacity(bActive || bAffordable ? 1.0f : LockedRowOpacity);
        RowWidget->SetIsEnabled(bActive || bAffordable);
    }

    if (ModifierHeader) {
        ModifierHeader->SetHeader(LOCTEXT("Modifiers", "Modifiers"),
                                  Skills ? FText::Format(LOCTEXT("Capacity", "{0} at once"),
                                                         FText::AsNumber(Skills->GetModifierCapacity()))
                                         : FText::GetEmpty(),
                                  nullptr);
    }
}

void UMythicPowersPageWidget::ToggleModifier(int32 ModifierIndex) {
    if (!Rows.IsValidIndex(SelectedRow)) {
        return;
    }
    UMythicSkillDefinition *Skill = Rows[SelectedRow].Skill;
    UMythicSkillComponent *Skills = GetSkills();
    if (!Skill || !Skills || !Skill->Modifiers.IsValidIndex(ModifierIndex)) {
        return;
    }

    // The server re-runs every gate - capacity, cost and whether the modifier does anything. This is a request.
    Skills->ServerSetModifierActive(Skill, ModifierIndex, !Skills->IsModifierActive(Skill, ModifierIndex));
}

#undef LOCTEXT_NAMESPACE
