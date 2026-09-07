#include "MythicGameplayWheel.h"
#include "UI/Controls/MythicModalInputProcessor.h"
#include "Input/MythicControlsComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/World.h"
#include "UnrealClient.h"
#include "TimerManager.h"
#include "CommonInputSubsystem.h"
#include "CommonTextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SafeZone.h"
#include "Components/SizeBox.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StreamableManager.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateUser.h"
#include "GameFramework/PlayerController.h"
#include "UI/MythicUIStyle.h"
#include "UI/MythicUIManagerSubsystem.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "UI/Settings/MythicUserSettings.h"
#include "UI/Controls/MythicControlGlyph.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"
#include "Components/ScrollBox.h"

namespace {
FString WheelKey(const FMythicQuickUseKey& Key) { return Key.Definition.ToString()+TEXT("|")+Key.Variant.ToString(); }
const FText PingNames[] = {
    NSLOCTEXT("Mythic","PingGo","Go here"),NSLOCTEXT("Mythic","PingDanger","Danger"),
    NSLOCTEXT("Mythic","PingHelp","Need help"),NSLOCTEXT("Mythic","PingLoot","Loot"),
    NSLOCTEXT("Mythic","PingWait","Wait"),NSLOCTEXT("Mythic","PingDefend","Defend here"),
    NSLOCTEXT("Mythic","PingRegroup","Regroup"),NSLOCTEXT("Mythic","PingAck","Acknowledge")};
}

UMythicGameplayWheel::UMythicGameplayWheel(const FObjectInitializer& Initializer):Super(Initializer) {
    SetIsFocusable(true);
    InputConfig=EMythicWidgetInputMode::GameAndMenu;
    GameMouseCaptureMode=EMouseCaptureMode::NoCapture;
}

void UMythicGameplayWheel::NativeOnInitialized() { Super::NativeOnInitialized(); Build(); }

TOptional<FUIInputConfig> UMythicGameplayWheel::GetDesiredInputConfig() const {
    FUIInputConfig Config(ECommonInputMode::All,EMouseCaptureMode::NoCapture);
    Config.bIgnoreLookInput=true; Config.bIgnoreMoveInput=false;
    return Config;
}

void UMythicGameplayWheel::Build() {
    if (!WidgetTree || Cells.Num()) return;
    auto* Safe=WidgetTree->ConstructWidget<USafeZone>(); WidgetTree->RootWidget=Safe;
    auto* Overlay=WidgetTree->ConstructWidget<UOverlay>(); Safe->AddChild(Overlay); SafeContent=Overlay;
    auto* Size=WidgetTree->ConstructWidget<USizeBox>(); Size->SetWidthOverride(320); Size->SetHeightOverride(388); WheelSize=Size;
    auto* Center=Overlay->AddChildToOverlay(Size); Center->SetHorizontalAlignment(HAlign_Center); Center->SetVerticalAlignment(VAlign_Center);
    WheelCanvas=WidgetTree->ConstructWidget<UCanvasPanel>(); Size->AddChild(WheelCanvas);
    const auto Place=[this](UWidget* Child,FVector2D Position,FVector2D Extent) {
        auto* Slot=WheelCanvas->AddChildToCanvas(Child); Slot->SetPosition(Position); Slot->SetSize(Extent);
        Child->SetVisibility(ESlateVisibility::HitTestInvisible);
    };
    Title=FMythicUIStyle::MakeText(this,EMythicTextRole::Heading); Title->SetJustification(ETextJustify::Center);
    Place(Title,{0,0},{320,28});
    Cells.SetNum(8);
    for (int32 Index=0;Index<8;++Index) {
        const float Angle=FMath::DegreesToRadians(-90.0f+Index*45.0f);
        const FVector2D Point(160+FMath::Cos(Angle)*110,174+FMath::Sin(Angle)*110);
        auto& Cell=Cells[Index];
        Cell.Plate=WidgetTree->ConstructWidget<UImage>(); Cell.Plate->SetBrush(FMythicUIStyle::MakeSlotBrush());
        Place(Cell.Plate,Point-FVector2D(31,31),{62,62});
        Cell.Icon=WidgetTree->ConstructWidget<UImage>(); Place(Cell.Icon,Point-FVector2D(20,22),{40,40});
        Cell.Count=FMythicUIStyle::MakeText(this,EMythicTextRole::Subtle); Cell.Count->SetJustification(ETextJustify::Right);
        auto Font=Cell.Count->GetFont(); Font.Size=14; Cell.Count->SetFont(Font);
        Place(Cell.Count,Point+FVector2D(-14,13),{40,18});
        Cell.Label=FMythicUIStyle::MakeText(this,EMythicTextRole::Subtle); Cell.Label->SetJustification(ETextJustify::Center);
        Font=Cell.Label->GetFont(); Font.Size=11; Cell.Label->SetFont(Font); Cell.Label->SetAutoWrapText(true);
        Place(Cell.Label,Point-FVector2D(30,20),{60,42});
    }
    Detail=FMythicUIStyle::MakeText(this,EMythicTextRole::Subtle); Detail->SetJustification(ETextJustify::Center); Detail->SetAutoWrapText(true);
    Place(Detail,{116,145},{88,64});
    PageLabel=FMythicUIStyle::MakeText(this,EMythicTextRole::Subtle); PageLabel->SetJustification(ETextJustify::Center);
    Place(PageLabel,{0,316},{320,22});
    Hint=WidgetTree->ConstructWidget<UVerticalBox>(); Place(Hint,{0,340},{320,48});
    const auto MakeLine=[this]() {
        auto* Line=WidgetTree->ConstructWidget<UHorizontalBox>();
        auto* Slot=Hint->AddChildToVerticalBox(Line); Slot->SetHorizontalAlignment(HAlign_Center); Slot->SetPadding(FMargin(0,2)); return Line;
    };
    const auto Text=[this](UHorizontalBox* Line,const FText& Value) {
        auto* Label=FMythicUIStyle::MakeText(this,EMythicTextRole::Subtle); Label->SetText(Value);
        auto* Slot=Line->AddChildToHorizontalBox(Label); Slot->SetVerticalAlignment(VAlign_Center); Slot->SetPadding(FMargin(3,0));
    };
    const auto Glyph=[this](UHorizontalBox* Line,EMythicControl Control) {
        auto* Value=CreateWidget<UMythicControlGlyph>(GetOwningPlayer());
        if (Value) { Value->GlyphHeight=20; Value->SetControl(Control); Line->AddChildToHorizontalBox(Value)->SetVerticalAlignment(VAlign_Center); }
        return Value;
    };
    auto* SelectLine=MakeLine(); Text(SelectLine,NSLOCTEXT("Mythic","WheelReleaseGlyph","Release"));
    OpenerGlyph=Glyph(SelectLine,Opener); Text(SelectLine,NSLOCTEXT("Mythic","WheelSelectGlyph","to select"));
    auto* ExitLine=MakeLine(); Text(ExitLine,NSLOCTEXT("Mythic","WheelCloseGlyph","Close:"));
    Glyph(ExitLine,EMythicControl::Jump); Text(ExitLine,NSLOCTEXT("Mythic","WheelJumpGlyph","Jump"));
    Glyph(ExitLine,EMythicControl::Dash); Text(ExitLine,NSLOCTEXT("Mythic","WheelDashGlyph","Dash"));

    ItemPreview=WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("HealingItemPreview"));
    auto* PreviewSlot=Overlay->AddChildToOverlay(ItemPreview);
    PreviewSlot->SetHorizontalAlignment(HAlign_Right); PreviewSlot->SetVerticalAlignment(VAlign_Center);
    PreviewSlot->SetPadding(FMargin(12));
    auto* Panel=WidgetTree->ConstructWidget<UBorder>(); Panel->SetPadding(FMargin(16));
    Panel->SetBrushColor(FLinearColor(0.025f,0.02f,0.015f,0.96f)); ItemPreview->AddChild(Panel);
    auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>(); Panel->AddChild(Scroll);
    auto* Column=WidgetTree->ConstructWidget<UVerticalBox>(); Scroll->AddChild(Column);
    PreviewIcon=WidgetTree->ConstructWidget<UImage>(); PreviewIcon->SetDesiredSizeOverride({64,64});
    Column->AddChildToVerticalBox(PreviewIcon)->SetHorizontalAlignment(HAlign_Center);
    PreviewName=FMythicUIStyle::MakeText(this,EMythicTextRole::Heading); PreviewName->SetAutoWrapText(true);
    Column->AddChildToVerticalBox(PreviewName)->SetPadding(FMargin(0,10,0,4));
    PreviewCount=FMythicUIStyle::MakeText(this,EMythicTextRole::Subtle);
    Column->AddChildToVerticalBox(PreviewCount)->SetPadding(FMargin(0,0,0,12));
    PreviewDescription=FMythicUIStyle::MakeText(this,EMythicTextRole::Body); PreviewDescription->SetAutoWrapText(true);
    Column->AddChildToVerticalBox(PreviewDescription);
    ItemPreview->SetVisibility(ESlateVisibility::Collapsed);
}


void UMythicGameplayWheel::HandleViewportResize(FViewport*,uint32) { QueueLayout(); }
void UMythicGameplayWheel::QueueLayout() {
    if (!bOpen || !GetWorld()) return;
    GetWorld()->GetTimerManager().ClearTimer(LayoutTimer);
    LayoutTimer=GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this,&ThisClass::RefreshLayout));
}
void UMythicGameplayWheel::RefreshLayout() {
    if (!bOpen || !WheelSize || !SafeContent) return;
    FVector2D Available=SafeContent->GetCachedGeometry().GetLocalSize();
    if (Available.X<=0 || Available.Y<=0) Available=UWidgetLayoutLibrary::GetPlayerScreenWidgetGeometry(GetOwningPlayer()).GetLocalSize();
    if (Available.X<=0 || Available.Y<=0) return;
    // At the supported 640x482 minimum, maximum user UI scale still leaves this readable compact body.
    // Text is never render-scaled; title, count, page and help retain their original font sizes.
    // Reserve the preview's space for the entire opening, so highlighting cannot move the wheel under the pointer.
    const bool bHasItems=Opener==EMythicControl::Heal && !Entries.IsEmpty();
    const float PreviewWidth=bHasItems ? FMath::Clamp(Available.X*0.4,160.0,320.0):0;
    const float ReservedWidth=bHasItems ? PreviewWidth+24:0;
    const float Scale=FMath::Clamp(FMath::Min((Available.X-ReservedWidth-16)/320.0,(Available.Y-124)/280.0),0.65,1.0);
    WheelSize->SetRenderTranslation({-ReservedWidth/2,0});
    if (ItemPreview) { ItemPreview->SetWidthOverride(PreviewWidth); ItemPreview->SetMaxDesiredHeight(FMath::Max(100.0,Available.Y-32)); }
    const float Width=320*Scale,PageY=36+280*Scale;
    WheelSize->SetWidthOverride(Width); WheelSize->SetHeightOverride(PageY+72);
    WheelCenter={160*Scale,36+138*Scale}; PointerNeutralRadius=48*Scale;
    const auto Place=[](UWidget* Child,FVector2D Position,FVector2D Extent) {
        if (auto* Slot=Cast<UCanvasPanelSlot>(Child->Slot)) { Slot->SetPosition(Position); Slot->SetSize(Extent); }
    };
    Place(Title,{0,0},{Width,28});
    for (int32 Index=0;Index<Cells.Num();++Index) {
        const float Angle=FMath::DegreesToRadians(-90.0f+Index*45.0f);
        const FVector2D Point=WheelCenter+FVector2D(FMath::Cos(Angle),FMath::Sin(Angle))*110*Scale;
        auto& Cell=Cells[Index];
        Place(Cell.Plate,Point-FVector2D(31,31)*Scale,FVector2D(62,62)*Scale);
        Place(Cell.Icon,Point-FVector2D(20,22)*Scale,FVector2D(40,40)*Scale);
        Place(Cell.Count,Point+FVector2D(-14*Scale,31*Scale-18),{40*Scale,18});
        Place(Cell.Label,Point-FVector2D(30*Scale,21),{60*Scale,42});
    }
    Place(Detail,WheelCenter-FVector2D(44*Scale,29),{88*Scale,64});
    Place(PageLabel,{0,PageY},{Width,22}); Place(Hint,{0,PageY+24},{Width,48});
}

void UMythicGameplayWheel::OpenFor(UMythicControlsComponent* Owner,const FMythicWheelOpenToken& Token) {
    CloseWithoutCommit();
    if (!IsValid(Owner) || !Owner->IsWheelOpenCurrent(Token)
        || (Token.Control!=EMythicControl::Heal && Token.Control!=EMythicControl::Ping)) return;
    Controls=Owner; Opener=Token.Control;
    const uint64 OpeningSerial=++OpenSerial;
    const TWeakObjectPtr<UMythicControlsComponent> ExpectedOwner=Owner;
    const auto Current=[this,ExpectedOwner,Token,OpeningSerial]() {
        return IsValid(this) && OpenSerial==OpeningSerial && ExpectedOwner.IsValid() && Controls==ExpectedOwner
            && ExpectedOwner->IsWheelOpenCurrent(Token);
    };
    Build();
    if (!Current()) { CloseWithoutCommit(); return; }
    QuickUse=GetOwningPlayer() ? GetOwningPlayer()->FindComponentByClass<UMythicQuickUseComponent>() : nullptr;
    Entries=QuickUse.IsValid() && Opener!=EMythicControl::Ping ? QuickUse->GetEntries(Opener==EMythicControl::Heal) : TArray<FMythicQuickUseEntry>();
    Page=0; Selected=INDEX_NONE;
    Stick=GetOwningPlayer() ? FVector2D(GetOwningPlayer()->GetInputAnalogKeyState(EKeys::Gamepad_RightX),-GetOwningPlayer()->GetInputAnalogKeyState(EKeys::Gamepad_RightY)):FVector2D::ZeroVector;
    const auto* Settings=UMythicUserSettings::Get(); const double Deadzone=Settings ? FMath::Clamp(Settings->GetGamepadDeadzoneRight(),0.0f,0.9f):0.15;
    const double Size=Stick.Size(); PageDirection=Size>Deadzone ? Stick/Size*FMath::Min(1.0,(Size-Deadzone)/(1-Deadzone)):FVector2D::ZeroVector;
    LastSelectionDirection=PageDirection;
    bAwaitFreshDirection=true; bCommitted=false; bOpen=true;
    if (QuickUse.IsValid()) InventoryChanged=QuickUse->OnEntriesChanged.AddUObject(this,&ThisClass::RefreshCounts);
    if (auto* Input=UCommonInputSubsystem::Get(GetOwningLocalPlayer())) DeviceHandle=Input->OnInputMethodChangedNative.AddUObject(this,&ThisClass::DeviceChanged);
    RefreshPage();
    if (!Current() || !bOpen) { CloseWithoutCommit(); return; }
    bool Attached=false;
    if (auto* GI=GetGameInstance()) if (auto* Manager=GI->GetSubsystem<UMythicUIManagerSubsystem>()) {
        bOnLayer=true; // Provisional; the exact layer is recorded before activation can call CloseWithoutCommit.
        Attached=Manager->TryAddWidgetInstanceToPlayerLayer(FGameplayTag::RequestGameplayTag(TEXT("UI.Layer.GameMenu")),
            GetOwningPlayer(),this,AttachedLayer);
    }
    if (!Attached || !Current() || !bOpen || !bOnLayer) { CloseWithoutCommit(); return; }
    if (!IsActivated()) ActivateWidget();
    if (!Current() || !bOpen || !IsActivated()) { CloseWithoutCommit(); return; }
    SetUserFocus(GetOwningPlayer());
    if (!Current() || !bOpen) { CloseWithoutCommit(); return; }
    if (!StartModalInput() || !Current() || !bOpen) {
        if (Controls.IsValid()) Controls->CloseWheel(false); else CloseWithoutCommit(); return;
    }
    ViewportHandle=FViewport::ViewportResizedEvent.AddUObject(this,&ThisClass::HandleViewportResize); QueueLayout();
}

bool UMythicGameplayWheel::StartModalInput() {
    const TWeakObjectPtr<UMythicGameplayWheel> Weak(this);
    FMythicModalInputProcessor::FRoutes Routes;
    Routes.Key=[Weak](FKey Key,EInputEvent Event,bool Repeat) {
        auto* Self=Weak.Get(); return Self && Self->RouteKey(Key,Event,Repeat).IsEventHandled();
    };
    Routes.Analog=[Weak](const FAnalogInputEvent& Event) {
        auto* Self=Weak.Get(); if (!Self) return false;
        // The original Enhanced Input/navigation path receives left-stick input; do not synthesize or duplicate it.
        if (Event.GetKey()==EKeys::Gamepad_LeftX || Event.GetKey()==EKeys::Gamepad_LeftY) return false;
        return Self->NativeOnAnalogValueChanged(Self->GetCachedGeometry(),Event).IsEventHandled();
    };
    Routes.MouseMove=[Weak](const FPointerEvent& Event) {
        auto* Self=Weak.Get(); return Self && Self->NativeOnMouseMove(Self->GetCachedGeometry(),Event).IsEventHandled();
    };
    Routes.MouseWheel=[Weak](const FPointerEvent& Event) {
        auto* Self=Weak.Get(); return Self && Self->NativeOnMouseWheel(Self->GetCachedGeometry(),Event).IsEventHandled();
    };
    Routes.FocusLost=[Weak]() {
        if (auto* Self=Weak.Get()) {
            if (auto* Owner=Self->Controls.Get()) { Owner->CloseWheel(false); Owner->CancelHeldInput(); }
            else Self->CloseWithoutCommit();
        }
    };
    ModalProcessor=MakeShared<FMythicModalInputProcessor>(this,MoveTemp(Routes));
    return ModalProcessor->Start();
}
void UMythicGameplayWheel::ReleaseObservers() {
    auto Previous=MoveTemp(ModalProcessor); ModalProcessor.Reset(); if (Previous) Previous->Stop();
    FViewport::ViewportResizedEvent.Remove(ViewportHandle); ViewportHandle.Reset();
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(LayoutTimer);
    if (QuickUse.IsValid()) QuickUse->OnEntriesChanged.Remove(InventoryChanged);
    if (auto* Input=UCommonInputSubsystem::Get(GetOwningLocalPlayer())) Input->OnInputMethodChangedNative.Remove(DeviceHandle);
    InventoryChanged.Reset(); DeviceHandle.Reset(); ++IconRevision;
    for (auto& Load:IconLoads) if (Load) Load->CancelHandle(); IconLoads.Reset();
    if (ItemPreview) {
        ItemPreview->SetVisibility(ESlateVisibility::Collapsed); PreviewIcon->SetBrushFromTexture(nullptr);
        PreviewName->SetText(FText::GetEmpty()); PreviewCount->SetText(FText::GetEmpty()); PreviewDescription->SetText(FText::GetEmpty());
    }
}
void UMythicGameplayWheel::CloseWithoutCommit() {
    ++OpenSerial; bOpen=false; ReleaseObservers();
    const bool WasOnLayer=bOnLayer; bOnLayer=false;
    auto* Layer=AttachedLayer.Get(); AttachedLayer.Reset();
    // Cleanup uses the actual accepted local layer even if possession or the manager's primary player changed.
    if (WasOnLayer && Layer && Layer->GetWidgetList().Contains(this)) Layer->RemoveWidget(*this);
    if (IsActivated()) DeactivateWidget();
}
void UMythicGameplayWheel::NativeOnDeactivated() { ++OpenSerial; const bool WasOpen=bOpen; bOpen=false; ReleaseObservers(); Super::NativeOnDeactivated(); if (WasOpen && Controls.IsValid()) Controls->CloseWheel(false); }
void UMythicGameplayWheel::NativeDestruct() { ReleaseObservers(); Super::NativeDestruct(); }
UWidget* UMythicGameplayWheel::NativeGetDesiredFocusTarget() const { return const_cast<UMythicGameplayWheel*>(this); }
bool UMythicGameplayWheel::NativeOnHandleBackAction() { if (Controls.IsValid()) Controls->CloseWheel(false); else CloseWithoutCommit(); return true; }

int32 UMythicGameplayWheel::PageCount() const { return Opener==EMythicControl::Ping ? 1:FMath::Max(1,FMath::DivideAndRoundUp(Entries.Num(),8)); }
int32 UMythicGameplayWheel::ItemIndex(int32 Sector) const {
    if (Sector<0 || Sector>=8 || Opener!=EMythicControl::Heal) return INDEX_NONE;
    return Page*8+Sector;
}
void UMythicGameplayWheel::RefreshCounts() {
    if (!bOpen || !QuickUse.IsValid() || Opener==EMythicControl::Ping) return;
    TMap<FString,int32> Counts;
    for (const auto& Entry:QuickUse->GetEntries(Opener==EMythicControl::Heal)) Counts.Add(WheelKey(Entry.Key),Entry.Quantity);
    for (auto& Entry:Entries) Entry.Quantity=Counts.FindRef(WheelKey(Entry.Key));
    for (int32 Index=0;Index<8;++Index) if (const int32 Item=ItemIndex(Index); Entries.IsValidIndex(Item)) {
        Cells[Index].Count->SetText(FText::AsNumber(Entries[Item].Quantity));
        Cells[Index].Icon->SetRenderOpacity(Entries[Item].Quantity>0 ? 1.0f:0.35f);
    }
    Highlight(Selected);
}
void UMythicGameplayWheel::RefreshPage() {
    ++IconRevision; for (auto& Load:IconLoads) if (Load) Load->CancelHandle(); IconLoads.Reset();
    Title->SetText(Opener==EMythicControl::Heal ? NSLOCTEXT("Mythic","WheelHealing","Healing") : NSLOCTEXT("Mythic","WheelPing","Communicate"));
    PageLabel->SetText(FText::Format(NSLOCTEXT("Mythic","WheelPage","{0} / {1}"),FText::AsNumber(Page+1),FText::AsNumber(PageCount())));
    if (OpenerGlyph) OpenerGlyph->SetControl(Opener);
    for (int32 Index=0;Index<8;++Index) {
        auto& Cell=Cells[Index]; Cell.Icon->SetVisibility(ESlateVisibility::Hidden); Cell.Icon->SetBrushFromTexture(nullptr); Cell.Count->SetText(FText::GetEmpty()); Cell.Label->SetText(FText::GetEmpty());
        const int32 Item=ItemIndex(Index);
        if (Opener==EMythicControl::Ping) Cell.Label->SetText(PingNames[Index]);
        else if (Entries.IsValidIndex(Item)) {
            Cell.Count->SetText(FText::AsNumber(Entries[Item].Quantity)); Cell.Icon->SetRenderOpacity(Entries[Item].Quantity>0 ? 1:0.35f);
            const auto Icon=Entries[Item].Icon;
            if (auto* Texture=Icon.Get()) {
                Cell.Icon->SetBrushFromTexture(Texture);
                Cell.Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
            }
            else if (!Icon.IsNull()) {
                const uint32 Revision=IconRevision; const TWeakObjectPtr<UMythicGameplayWheel> Weak(this);
                IconLoads.Add(UAssetManager::GetStreamableManager().RequestAsyncLoad(Icon.ToSoftObjectPath(),[Weak,Revision,Index,Icon]() {
                    if (auto* Self=Weak.Get(); Self && Self->bOpen && Self->IconRevision==Revision && Self->Cells.IsValidIndex(Index)) {
                        if (auto* Texture=Icon.Get()) {
                            Self->Cells[Index].Icon->SetBrushFromTexture(Texture);
                            Self->Cells[Index].Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
                            if (Self->Selected==Index) Self->RefreshItemPreview();
                        }
                    }
                }));
            }
        }
        Cell.Plate->SetRenderOpacity(Entries.IsValidIndex(Item) || Opener==EMythicControl::Ping ? 1:0.3f);
    }
    Highlight(INDEX_NONE);
}
void UMythicGameplayWheel::Highlight(int32 Index) {
    Selected=Index;
    for (int32 I=0;I<Cells.Num();++I) Cells[I].Plate->SetColorAndOpacity(I==Index ? FMythicUIStyle::Get().AccentBright:FLinearColor::White);
    FText Text;
    if (Index!=INDEX_NONE && Opener==EMythicControl::Ping) Text=PingNames[Index];
    else if (const int32 Item=ItemIndex(Index); Entries.IsValidIndex(Item)) Text=FText::Format(NSLOCTEXT("Mythic","WheelItem","{0}\n{1}"),Entries[Item].Name,FText::AsNumber(Entries[Item].Quantity));
    else Text=Entries.IsEmpty() && Opener==EMythicControl::Heal ? NSLOCTEXT("Mythic","NoHealingItems","No healing items") : NSLOCTEXT("Mythic","WheelNeutral","Keep current selection");
    Detail->SetText(Text);
    RefreshItemPreview();
}
void UMythicGameplayWheel::RefreshItemPreview() {
    if (!ItemPreview) return;
    const int32 Index=ItemIndex(Selected);
    if (!bOpen || !Entries.IsValidIndex(Index) || Entries[Index].Quantity<=0) {
        ItemPreview->SetVisibility(ESlateVisibility::Collapsed); PreviewIcon->SetBrushFromTexture(nullptr);
        PreviewName->SetText(FText::GetEmpty()); PreviewCount->SetText(FText::GetEmpty()); PreviewDescription->SetText(FText::GetEmpty());
        return;
    }
    const auto& Entry=Entries[Index];
    PreviewName->SetText(Entry.Name);
    PreviewCount->SetText(FText::Format(NSLOCTEXT("Mythic","WheelItemQuantity","In inventory: {0}"),FText::AsNumber(Entry.Quantity)));
    PreviewDescription->SetText(Entry.Description);
    PreviewIcon->SetBrush(Cells[Selected].Icon->GetBrush());
    PreviewIcon->SetVisibility(Cells[Selected].Icon->GetVisibility());
    ItemPreview->SetVisibility(ESlateVisibility::HitTestInvisible);
}
void UMythicGameplayWheel::ChangePage(int32 Delta) {
    const int32 Next=FMath::Clamp(Page+Delta,0,PageCount()-1); if (Next==Page) return;
    Page=Next; bAwaitFreshDirection=true; PageDirection=LastSelectionDirection; RefreshPage();
}
void UMythicGameplayWheel::DeviceChanged(ECommonInputType) {
    // The modal owner has the latest raw axes. Keep both axes so a one-axis replay
    // cannot fabricate a new direction; the filtered previous direction gates rearming.
    PageDirection=LastSelectionDirection; bAwaitFreshDirection=true; RefreshPage();
}
void UMythicGameplayWheel::SelectDirection(FVector2D Direction,bool bStick) {
    if (bStick) LastSelectionDirection=Direction;
    const double Size=Direction.Size(); const double Neutral=bStick ? 0.22:PointerNeutralRadius;
    if (Size<Neutral) { Highlight(INDEX_NONE); if (bStick) bAwaitFreshDirection=false; return; }
    if (bStick && (Size<0.30 || (bAwaitFreshDirection && Direction.Equals(PageDirection,0.10)))) return;
    bAwaitFreshDirection=false;
    const float Degrees=FMath::Fmod(FMath::RadiansToDegrees(FMath::Atan2(Direction.Y,Direction.X))+450.0f,360.0f);
    if (Selected!=INDEX_NONE && FMath::Abs(FMath::FindDeltaAngleDegrees(Selected*45.0f,Degrees))<27.5f) return;
    Highlight(FMath::FloorToInt((Degrees+22.5f)/45.0f)%8);
}
bool UMythicGameplayWheel::OwnsUser(int32 UserIndex) const { const auto* LP=GetOwningLocalPlayer(); const auto User=LP ? LP->GetSlateUser():nullptr; return User && User->GetUserIndex()==UserIndex; }
FReply UMythicGameplayWheel::RouteKey(FKey Key,EInputEvent Event,bool bRepeat) {
    if (!bOpen || !Controls.IsValid()) return FReply::Unhandled();
    if (Event==IE_Released) { Controls->HandleKey(Key,Event); return FReply::Handled(); }
    if (bRepeat) return FReply::Handled();
    if (Controls->IsControlKey(Key,EMythicControl::Jump)) { Controls->HandleKey(Key,IE_Pressed); return FReply::Handled(); }
    if (Controls->IsControlKey(Key,EMythicControl::Dash)) { Controls->HandleKey(Key,IE_Pressed); return FReply::Handled(); }
    if (Controls->IsControlKey(Key,EMythicControl::CharacterMenu) || Controls->IsControlKey(Key,EMythicControl::SystemMenu)) { Controls->CloseWheel(false); Controls->HandleKey(Key,Event); return FReply::Handled(); }
    if (Key==EKeys::Gamepad_LeftShoulder || Key==EKeys::PageUp) { ChangePage(-1); return FReply::Handled(); }
    if (Key==EKeys::Gamepad_RightShoulder || Key==EKeys::PageDown) { ChangePage(1); return FReply::Handled(); }
    Controls->HandleKey(Key,Event); return FReply::Handled();
}
FReply UMythicGameplayWheel::NativeOnPreviewKeyDown(const FGeometry&,const FKeyEvent& Event) { return OwnsUser(Event.GetUserIndex()) ? RouteKey(Event.GetKey(),IE_Pressed,Event.IsRepeat()):FReply::Unhandled(); }
FReply UMythicGameplayWheel::NativeOnKeyUp(const FGeometry&,const FKeyEvent& Event) { return OwnsUser(Event.GetUserIndex()) ? RouteKey(Event.GetKey(),IE_Released):FReply::Unhandled(); }
FReply UMythicGameplayWheel::NativeOnMouseButtonDown(const FGeometry&,const FPointerEvent& Event) { return OwnsUser(Event.GetUserIndex()) ? RouteKey(Event.GetEffectingButton(),IE_Pressed):FReply::Unhandled(); }
FReply UMythicGameplayWheel::NativeOnMouseButtonUp(const FGeometry&,const FPointerEvent& Event) { return OwnsUser(Event.GetUserIndex()) ? RouteKey(Event.GetEffectingButton(),IE_Released):FReply::Unhandled(); }
FReply UMythicGameplayWheel::NativeOnAnalogValueChanged(const FGeometry&,const FAnalogInputEvent& Event) {
    if (!OwnsUser(Event.GetUserIndex())) return FReply::Unhandled();
    const FKey Key=Event.GetKey(); if (Key==EKeys::Gamepad_LeftX || Key==EKeys::Gamepad_LeftY) return FReply::Unhandled();
    if (Key==EKeys::Gamepad_RightX || Key==EKeys::Gamepad_RightY) {
        if (Key==EKeys::Gamepad_RightX) Stick.X=Event.GetAnalogValue(); else Stick.Y=-Event.GetAnalogValue();
        const auto* Settings=UMythicUserSettings::Get(); const double Deadzone=Settings ? FMath::Clamp(Settings->GetGamepadDeadzoneRight(),0.0f,0.9f):0.15;
        const double Size=Stick.Size(); const FVector2D Filtered=Size>Deadzone ? Stick/Size*FMath::Min(1.0,(Size-Deadzone)/(1-Deadzone)):FVector2D::ZeroVector;
        SelectDirection(Filtered,true);
    }
    return FReply::Handled();
}
FReply UMythicGameplayWheel::NativeOnMouseMove(const FGeometry&,const FPointerEvent& Event) {
    if (!OwnsUser(Event.GetUserIndex())) return FReply::Unhandled();
    if (bOpen && WheelCanvas && !Event.GetCursorDelta().IsNearlyZero()) SelectDirection(WheelCanvas->GetCachedGeometry().AbsoluteToLocal(Event.GetScreenSpacePosition())-WheelCenter,false);
    return FReply::Handled();
}
FReply UMythicGameplayWheel::NativeOnMouseWheel(const FGeometry&,const FPointerEvent& Event) { if (!OwnsUser(Event.GetUserIndex())) return FReply::Unhandled(); ChangePage(Event.GetWheelDelta()>0 ? -1:1); return FReply::Handled(); }
void UMythicGameplayWheel::CommitHighlighted() {
    if (!bOpen || bCommitted || Selected==INDEX_NONE || !Controls.IsValid()) { CloseWithoutCommit(); return; }
    bCommitted=true;
    const int32 Sector=Selected;
    if (Opener==EMythicControl::Ping) { CloseWithoutCommit(); Controls->ExecuteWheelCommand(EMythicControl::Ping,Sector); }
    else if (const int32 Item=ItemIndex(Sector); Entries.IsValidIndex(Item) && Entries[Item].Quantity>0 && QuickUse.IsValid()) {
        QuickUse->Select(Opener==EMythicControl::Heal,Entries[Item].Key);
        CloseWithoutCommit();
    } else CloseWithoutCommit();
}
