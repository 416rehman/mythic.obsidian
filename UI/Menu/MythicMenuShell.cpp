// Copyright Stellar Games. All Rights Reserved.

#include "MythicMenuShell.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "CommonActivatableWidgetSwitcher.h"
#include "CommonTabListWidgetBase.h"
#include "CommonButtonBase.h"
#include "CommonTextBlock.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "UI/MythicUIStyle.h"
#include "UI/Widgets/MythicInputGlyph.h"
#include "Input/CommonUIInputTypes.h"
#include "Mythic/Mythic.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"

static void MenuShell_FocusPage(UMythicMenuShell *Shell, UCommonActivatableWidget *Page) {
    if (!Shell || !Page || !Shell->IsActivated() || Shell->GetPageWidget(Shell->GetActivePageId()) != Page) {
        return;
    }
    if (UWidget *First = FMythicUIStyle::FindFirstFocusable(Page)) {
        First->SetFocus();
    }
    else {
        Shell->RequestRefreshFocus();
    }
}

static void MenuShell_FocusPageNextTick(UMythicMenuShell *Shell, UCommonActivatableWidget *Page) {
    if (!Shell || !Page) {
        return;
    }
    UWorld *World = Shell->GetWorld();
    if (!World) {
        return;
    }
    TWeakObjectPtr<UMythicMenuShell> WeakShell(Shell);
    TWeakObjectPtr<UCommonActivatableWidget> WeakPage(Page);
    World->GetTimerManager().SetTimerForNextTick([WeakShell, WeakPage]() {
        MenuShell_FocusPage(WeakShell.Get(), WeakPage.Get());
    });
}

void UMythicMenuShell::NativeOnInitialized() {
    Super::NativeOnInitialized();


    if (PreviousTabAction.IsValid()) {
        PrevTabBinding = RegisterUIActionBinding(
            FBindUIActionArgs(FUIActionTag::ConvertChecked(PreviousTabAction), false,
                              FSimpleDelegate::CreateWeakLambda(this, [this]() { CyclePage(-1); })));
    }
    if (NextTabAction.IsValid()) {
        NextTabBinding = RegisterUIActionBinding(
            FBindUIActionArgs(FUIActionTag::ConvertChecked(NextTabAction), false,
                              FSimpleDelegate::CreateWeakLambda(this, [this]() { CyclePage(1); })));
    }
}

void UMythicMenuShell::NativeConstruct() {
    Super::NativeConstruct();
    BuildPages();
    RegisterTabs();

    if (Glyph_PrevTab) {
        Glyph_PrevTab->SetActionBinding(PrevTabBinding);
    }
    if (Glyph_NextTab) {
        Glyph_NextTab->SetActionBinding(NextTabBinding);
    }
}

void UMythicMenuShell::BuildPages() {
    if (bPagesBuilt || !ContentSwitcher) {
        return;
    }
    bPagesBuilt = true;

    for (const FMythicMenuPage &Page : Pages) {
        if (Page.PageId.IsNone() || !Page.PageClass) {
            UE_LOG(Myth, Warning, TEXT("MenuShell: page entry with no id or no class; skipped."));
            continue;
        }
        if (PageWidgets.Contains(Page.PageId)) {
            UE_LOG(Myth, Warning, TEXT("MenuShell: duplicate page id '%s'; keeping the first."), *Page.PageId.ToString());
            continue;
        }
        if (!IsPageUnlocked(Page)) {
            continue;
        }

        UCommonActivatableWidget *PageWidget = CreateWidget<UCommonActivatableWidget>(GetOwningPlayer(), Page.PageClass);
        if (!PageWidget) {
            continue;
        }
        ContentSwitcher->AddChild(PageWidget);
        PageWidgets.Add(Page.PageId, PageWidget);
        OrderedPageIds.Add(Page.PageId);

        TWeakObjectPtr<UMythicMenuShell> WeakThis(this);
        TWeakObjectPtr<UCommonActivatableWidget> WeakPage(PageWidget);
        PageWidget->OnActivated().AddWeakLambda(this, [WeakThis, WeakPage]() {
            MenuShell_FocusPageNextTick(WeakThis.Get(), WeakPage.Get());
        });
    }
}

void UMythicMenuShell::RegisterTabs() {
    if (!TabList) {
        return;
    }

    TabLabels.Reset();

    for (const FMythicMenuPage &Page : Pages) {
        UCommonActivatableWidget *PageWidget = GetPageWidget(Page.PageId);
        if (!PageWidget) {
            continue;
        }

        const TSubclassOf<UCommonButtonBase> ButtonClass = Page.TabButtonClass ? Page.TabButtonClass : DefaultTabButtonClass;
        if (!ButtonClass) {
            continue;
        }
        if (!TabList->GetTabButtonBaseByID(Page.PageId)) {
            TabList->RegisterTab(Page.PageId, ButtonClass, PageWidget);
        }

        if (UCommonButtonBase *TabButton = TabList->GetTabButtonBaseByID(Page.PageId)) {
            if (UTextBlock *Label = Cast<UTextBlock>(TabButton->GetWidgetFromName(TabLabelWidgetName))) {
                Label->SetText(Page.TabLabel);
                TabLabels.Add(Page.PageId, Label);
            }
            else {
                UE_LOG(Myth, Warning, TEXT("MenuShell: tab button for '%s' has no text widget named '%s'; its label is unset."),
                       *Page.PageId.ToString(), *TabLabelWidgetName.ToString());
            }
        }
    }

    TabList->OnTabSelected.AddUniqueDynamic(this, &UMythicMenuShell::HandleTabSelected);
}

static TAutoConsoleVariable<bool> CVarMythicShowUIKitPage(
    TEXT("Mythic.UI.ShowKit"), false,
    TEXT("Show the 'UI Kit' gym page as a tab in the menu shell. Off by default; dev only."));

bool UMythicMenuShell::IsPageUnlocked(const FMythicMenuPage &Page) const {
    if (Page.PageId == FName(TEXT("UIKit")) && !CVarMythicShowUIKitPage.GetValueOnGameThread()) {
        return false;
    }
    if (!Page.RequiredUnlockTag.IsValid()) {
        return true;
    }
    const APlayerController *PC = GetOwningPlayer();
    if (!PC) {
        return false;
    }
    UAbilitySystemComponent *ASC = nullptr;
    if (APlayerState *PS = PC->PlayerState) {
        ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PS);
    }
    return ASC && ASC->HasMatchingGameplayTag(Page.RequiredUnlockTag);
}

void UMythicMenuShell::NativeOnActivated() {
    Super::NativeOnActivated();
    OpenPage(ActivePageId.IsNone() ? DefaultPageId : ActivePageId);
}

void UMythicMenuShell::NativeOnDeactivated() {
    Super::NativeOnDeactivated();
}

UCommonActivatableWidget *UMythicMenuShell::GetPageWidget(FName PageId) const {
    const TObjectPtr<UCommonActivatableWidget> *Found = PageWidgets.Find(PageId);
    return Found ? *Found : nullptr;
}

void UMythicMenuShell::OpenPage(FName PageId) {
    if (!ContentSwitcher) {
        return;
    }

    UCommonActivatableWidget *Target = GetPageWidget(PageId);
    if (!Target) {
        for (const FMythicMenuPage &Page : Pages) {
            if (UCommonActivatableWidget *Fallback = GetPageWidget(Page.PageId)) {
                Target = Fallback;
                PageId = Page.PageId;
                break;
            }
        }
    }
    if (!Target) {
        return;
    }

    ContentSwitcher->SetActiveWidget(Target);
    ActivePageId = PageId;

    if (IsActivated() && Target->IsActivated()) {
        MenuShell_FocusPageNextTick(this, Target);
    }

    if (TabList) {
        TabList->SelectTabByID(PageId);
    }
    ApplyTabSelectionVisuals();

    if (Txt_ScreenTitle) {
        const FMythicMenuPage *Def = Pages.FindByPredicate([PageId](const FMythicMenuPage &P) { return P.PageId == PageId; });
        Txt_ScreenTitle->SetText(Def ? FText::FromString(Def->TabLabel.ToString().ToUpper()) : FText::GetEmpty());
    }

    OnPageChanged(PageId);
}

FName UMythicMenuShell::GetFirstPageId() const {
    return OrderedPageIds.Num() > 0 ? OrderedPageIds[0] : NAME_None;
}

void UMythicMenuShell::CyclePage(int32 Delta) {
    const int32 Count = OrderedPageIds.Num();
    if (Count <= 1 || Delta == 0) {
        return;
    }
    int32 Current = OrderedPageIds.IndexOfByKey(ActivePageId);
    if (Current == INDEX_NONE) {
        Current = 0;
    }
    const int32 Next = ((Current + Delta) % Count + Count) % Count;
    OpenPage(OrderedPageIds[Next]);
}

void UMythicMenuShell::ApplyTabSelectionVisuals() {
    for (const TPair<FName, TObjectPtr<UTextBlock>> &Pair : TabLabels) {
        if (UTextBlock *Label = Pair.Value) {
            Label->SetColorAndOpacity(FSlateColor(Pair.Key == ActivePageId ? SelectedTabColor : UnselectedTabColor));
        }
    }
}

void UMythicMenuShell::HandleTabSelected(FName TabId) {
    if (TabId != ActivePageId) {
        OpenPage(TabId);
    }
}
