#include "MythicQuickUseHudCell.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "UI/Controls/MythicControlGlyph.h"
#include "UI/MythicUIStyle.h"

UMythicQuickUseHudCell::UMythicQuickUseHudCell(const FObjectInitializer& Initializer):Super(Initializer) { SetIsFocusable(false); }
void UMythicQuickUseHudCell::NativeOnInitialized() { Super::NativeOnInitialized(); Build(); }
void UMythicQuickUseHudCell::InitializePurpose(bool bInHealing) { bHealing=bInHealing; Build(); Refresh(); }
void UMythicQuickUseHudCell::Build() {
    if (!WidgetTree || Icon) return;
    auto* Column=WidgetTree->ConstructWidget<UVerticalBox>(); WidgetTree->RootWidget=Column;
    auto* Well=WidgetTree->ConstructWidget<USizeBox>(); Well->SetWidthOverride(52); Well->SetHeightOverride(52);
    Column->AddChildToVerticalBox(Well)->SetHorizontalAlignment(HAlign_Center);
    auto* Layers=WidgetTree->ConstructWidget<UOverlay>(); Well->AddChild(Layers);
    auto* Plate=WidgetTree->ConstructWidget<UImage>(); Plate->SetBrush(FMythicUIStyle::MakeSlotBrush()); Layers->AddChildToOverlay(Plate);
    Icon=WidgetTree->ConstructWidget<UImage>(); Icon->SetDesiredSizeOverride({40,40});
    auto* IconSlot=Layers->AddChildToOverlay(Icon); IconSlot->SetHorizontalAlignment(HAlign_Center); IconSlot->SetVerticalAlignment(VAlign_Center);
    Empty=FMythicUIStyle::MakeText(this,EMythicTextRole::Subtle);
    auto Font=Empty->GetFont(); Font.Size=11; Empty->SetFont(Font);
    auto* EmptySlot=Layers->AddChildToOverlay(Empty); EmptySlot->SetHorizontalAlignment(HAlign_Center); EmptySlot->SetVerticalAlignment(VAlign_Center);
    Count=FMythicUIStyle::MakeText(this,EMythicTextRole::Body); Font=Count->GetFont(); Font.Size=14; Count->SetFont(Font);
    auto* CountSlot=Layers->AddChildToOverlay(Count); CountSlot->SetHorizontalAlignment(HAlign_Right); CountSlot->SetVerticalAlignment(VAlign_Bottom); CountSlot->SetPadding(FMargin(0,0,4,1));
    Glyph=WidgetTree->ConstructWidget<UMythicControlGlyph>(); Glyph->GlyphHeight=22;
    auto* GlyphSlot=Column->AddChildToVerticalBox(Glyph); GlyphSlot->SetHorizontalAlignment(HAlign_Center); GlyphSlot->SetPadding(FMargin(0,2,0,0));
    Icon->SetVisibility(ESlateVisibility::Collapsed); SetVisibility(ESlateVisibility::HitTestInvisible);
}
void UMythicQuickUseHudCell::NativeConstruct() {
    Super::NativeConstruct(); bConstructed=true; Build();
    QuickUse=GetOwningPlayer() ? GetOwningPlayer()->FindComponentByClass<UMythicQuickUseComponent>():nullptr;
    if (QuickUse.IsValid()) EntriesChanged=QuickUse->OnEntriesChanged.AddUObject(this,&ThisClass::Refresh);
    Refresh();
}
void UMythicQuickUseHudCell::ClearLoads() {
    ++SelectionRevision; ++IconRevision;
    if (DefinitionLoad) DefinitionLoad->CancelHandle(); DefinitionLoad.Reset();
    if (IconLoad) IconLoad->CancelHandle(); IconLoad.Reset(); IconPath.Reset();
}
void UMythicQuickUseHudCell::NativeDestruct() {
    bConstructed=false; if (QuickUse.IsValid()) QuickUse->OnEntriesChanged.Remove(EntriesChanged);
    EntriesChanged.Reset(); QuickUse.Reset(); ClearLoads(); ShownKey={};
    Super::NativeDestruct();
}
void UMythicQuickUseHudCell::Refresh() {
    if (!Icon) return;
    Glyph->SetControl(bHealing ? EMythicControl::Heal:EMythicControl::Utility);
    Empty->SetText(bHealing ? NSLOCTEXT("Mythic","QuickHudHeal","Heal"):NSLOCTEXT("Mythic","QuickHudItem","Item"));
    const auto Key=QuickUse.IsValid() ? QuickUse->GetSelected(bHealing):FMythicQuickUseKey{};
    const FMythicQuickUseEntry* Entry=QuickUse.IsValid() ? QuickUse->GetEntries(bHealing).FindByPredicate([&](const auto& Row){return Row.Key==Key;}):nullptr;
    const bool Changed=!(Key==ShownKey);
    if (Changed) { ClearLoads(); ShownKey=Key; Icon->SetBrushFromTexture(nullptr); Icon->SetVisibility(ESlateVisibility::Collapsed); SetToolTipText(FText::GetEmpty()); }
    // An absent row means zero currently eligible units, not "full health", "loading", or a failed request.
    Count->SetText(Key.IsValid() ? FText::AsNumber(Entry ? FMath::Max(0,Entry->Quantity):0):FText::GetEmpty());
    Icon->SetRenderOpacity(Entry && Entry->Quantity>0 ? 1.0f:0.35f);
    Count->SetColorAndOpacity(FSlateColor(Entry && Entry->Quantity>0 ? FMythicUIStyle::Get().Ink:FMythicUIStyle::Get().AccentBright));
    Empty->SetVisibility(Icon->GetVisibility()==ESlateVisibility::Collapsed ? ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    if (Entry) { SetToolTipText(Entry->Name); RefreshIcon(Entry->Icon.ToSoftObjectPath()); }
    else if (Changed && Key.IsValid()) {
        const uint32 Revision=SelectionRevision;
        if (Key.Definition.ResolveObject()) ResolveSelectedDefinition(Revision,Key);
        else DefinitionLoad=UAssetManager::GetStreamableManager().RequestAsyncLoad(Key.Definition,FStreamableDelegate::CreateWeakLambda(this,[this,Revision,Key](){ResolveSelectedDefinition(Revision,Key);}));
    }
}
void UMythicQuickUseHudCell::ResolveSelectedDefinition(uint32 Revision,FMythicQuickUseKey Key) {
    if (!bConstructed || Revision!=SelectionRevision || !(ShownKey==Key)) return;
    if (const auto* Definition=Cast<UItemDefinition>(Key.Definition.ResolveObject())) { SetToolTipText(Definition->Name); RefreshIcon(Definition->Icon2d.ToSoftObjectPath()); }
}
void UMythicQuickUseHudCell::RefreshIcon(const FSoftObjectPath& Path) {
    if (Path==IconPath) return;
    if (IconLoad) IconLoad->CancelHandle(); IconLoad.Reset(); IconPath=Path;
    const uint32 Revision=++IconRevision; Icon->SetBrushFromTexture(nullptr); Icon->SetVisibility(ESlateVisibility::Collapsed);
    if (Path.IsNull()) return;
    const auto Apply=[this,Revision,Path]() {
        if (!bConstructed || Revision!=IconRevision || IconPath!=Path) return;
        if (auto* Texture=Cast<UTexture2D>(Path.ResolveObject())) { Icon->SetBrushFromTexture(Texture); Icon->SetVisibility(ESlateVisibility::HitTestInvisible); Empty->SetVisibility(ESlateVisibility::Collapsed); }
    };
    if (Path.ResolveObject()) Apply();
    else IconLoad=UAssetManager::GetStreamableManager().RequestAsyncLoad(Path,FStreamableDelegate::CreateWeakLambda(this,Apply));
}
