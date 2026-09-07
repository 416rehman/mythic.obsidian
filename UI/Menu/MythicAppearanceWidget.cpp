#include "MythicAppearanceWidget.h"
#include "MythicCharacterPageWidget.h"
#include "Character/Appearance/MythicAppearanceComponent.h"
#include "Character/Appearance/MythicAppearancePreview.h"
#include "Player/MythicCharacter.h"
#include "Player/MythicPlayerState.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "UI/MythicUIManagerSubsystem.h"
#include "UI/MythicUIStyle.h"
#include "UI/Widgets/MythicBoundActionButton.h"
#include "UI/Controls/MythicModalInputProcessor.h"
#include "CommonInputSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "CommonButtonBase.h"
#include "Input/CommonUIInputTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/WrapBox.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Engine/TextureRenderTarget2D.h"
#include "TimerManager.h"

namespace {
FGameplayTag AppearanceLayer() { return FGameplayTag::RequestGameplayTag(TEXT("UI.Layer.Modal")); }
FText OriginalColor() { return NSLOCTEXT("Appearance","Original","Original"); }
}
UMythicAppearanceWidget::UMythicAppearanceWidget(const FObjectInitializer& Initializer):Super(Initializer) {
    InputConfig=EMythicWidgetInputMode::Menu; bIsBackHandler=true; bIsModal=true;
}
UMythicBoundActionButton* UMythicAppearanceWidget::Button(UPanelWidget* Host,const FText& Label,TFunction<void()> Action,bool bPrimary) {
    const auto& Style=FMythicUIStyle::Get(); auto* Class=Style.ActionButtonClass.LoadSynchronous();
    if (!Class || !Host) return nullptr;
    auto* B=WidgetTree->ConstructWidget<UMythicBoundActionButton>(Class);
    B->SetActionBarPromptOnly(false); B->SetLabelOverride(Label);
    B->SetStyle((bPrimary?Style.PrimaryActionButtonStyle:Style.SecondaryActionButtonStyle).LoadSynchronous());
    B->SetMinDimensions(64,38); B->bNavigateToNextWidgetOnDisable=true;
    FMythicUIStyle::WireFocusRing(B);
    B->OnClicked().AddWeakLambda(this,[Action=MoveTemp(Action)](){ Action(); });
    B->OnHovered().AddWeakLambda(this,[this,B](){ if (ChoiceScroll) ChoiceScroll->ScrollWidgetIntoView(B,true,EDescendantScrollDestination::IntoView); });
    B->OnFocusReceived().AddWeakLambda(this,[this,B](){ LastFocus=B; if (ChoiceScroll) ChoiceScroll->ScrollWidgetIntoView(B,true,EDescendantScrollDestination::IntoView); });
    Host->AddChild(B); return B;
}
void UMythicAppearanceWidget::NativeOnInitialized() {
    Super::NativeOnInitialized(); Build();
    PreviousTabBinding=RegisterUIActionBinding(FBindUIActionArgs(FUIActionTag::ConvertChecked(FGameplayTag::RequestGameplayTag(TEXT("UI.Action.TabLeft"))),false,FSimpleDelegate::CreateWeakLambda(this,[this](){SetTab(false);})));
    NextTabBinding=RegisterUIActionBinding(FBindUIActionArgs(FUIActionTag::ConvertChecked(FGameplayTag::RequestGameplayTag(TEXT("UI.Action.TabRight"))),false,FSimpleDelegate::CreateWeakLambda(this,[this](){SetTab(true);})));
}
void UMythicAppearanceWidget::Build() {
    if (!WidgetTree || WidgetTree->RootWidget) return;
    auto* Card=WidgetTree->ConstructWidget<UBorder>(); Card->SetPadding(FMargin(12));
    Card->SetBrushColor(FMythicUIStyle::Get().Surface); WidgetTree->RootWidget=Card;
    auto* Stack=WidgetTree->ConstructWidget<UVerticalBox>(); Card->AddChild(Stack);
    auto* Title=FMythicUIStyle::MakeText(this,EMythicTextRole::Heading); Title->SetText(NSLOCTEXT("Appearance","Title","Customize Character")); Stack->AddChild(Title);
    auto* Tabs=WidgetTree->ConstructWidget<UHorizontalBox>(); Stack->AddChild(Tabs);
    BodyTab=Button(Tabs,NSLOCTEXT("Appearance","BodyTab","Body"),[this](){SetTab(false);});
    GearTab=Button(Tabs,NSLOCTEXT("Appearance","GearTab","Equipment colors"),[this](){SetTab(true);});
    auto* Body=WidgetTree->ConstructWidget<UHorizontalBox>(); Stack->AddChildToVerticalBox(Body)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    PreviewImage=WidgetTree->ConstructWidget<UImage>(); PreviewImage->SetVisibility(ESlateVisibility::Visible);
    auto* PreviewFit=WidgetTree->ConstructWidget<UScaleBox>(); PreviewFit->SetStretch(EStretch::ScaleToFit);
    auto* PreviewSize=WidgetTree->ConstructWidget<USizeBox>(); PreviewSize->SetWidthOverride(768); PreviewSize->SetHeightOverride(1024);
    PreviewSize->AddChild(PreviewImage); PreviewFit->AddChild(PreviewSize);
    Body->AddChildToHorizontalBox(PreviewFit)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ChoiceScroll=WidgetTree->ConstructWidget<UScrollBox>(); ChoiceScroll->SetScrollWhenFocusChanges(EScrollWhenFocusChanges::AnimatedScroll);
    Body->AddChildToHorizontalBox(ChoiceScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Choices=WidgetTree->ConstructWidget<UVerticalBox>(); ChoiceScroll->AddChild(Choices);
    PreviewStatus=FMythicUIStyle::MakeText(this,EMythicTextRole::Body); PreviewStatus->SetAutoWrapText(true); Stack->AddChild(PreviewStatus);
    auto* ViewControls=WidgetTree->ConstructWidget<UWrapBox>(); ViewControls->SetInnerSlotPadding(FVector2D(4,4)); Stack->AddChild(ViewControls);
    FirstViewControl=Button(ViewControls,NSLOCTEXT("Appearance","RotateLeft","Rotate left"),[this](){if(Preview)Preview->Orbit(-30);});
    Button(ViewControls,NSLOCTEXT("Appearance","RotateRight","Rotate right"),[this](){if(Preview)Preview->Orbit(30);});
    Button(ViewControls,NSLOCTEXT("Appearance","ResetView","Reset view"),[this](){if(Preview)Preview->ResetView();});
    Button(ViewControls,NSLOCTEXT("Appearance","ZoomIn","Zoom in"),[this](){if(Preview)Preview->Zoom(-30);});
    Button(ViewControls,NSLOCTEXT("Appearance","ZoomOut","Zoom out"),[this](){if(Preview)Preview->Zoom(30);});
    Notice=FMythicUIStyle::MakeText(this,EMythicTextRole::Body); Notice->SetAutoWrapText(true); Stack->AddChild(Notice);
    ExitPrompt=WidgetTree->ConstructWidget<UVerticalBox>(); Stack->AddChild(ExitPrompt);
    auto* Warning=FMythicUIStyle::MakeText(this,EMythicTextRole::Body); Warning->SetText(NSLOCTEXT("Appearance","DirtyExit","Apply these changes before leaving?")); ExitPrompt->AddChild(Warning);
    Button(ExitPrompt,NSLOCTEXT("Appearance","ApplyAndClose","Apply and close"),[this](){SetExitPrompt(false); Apply(true);},true);
    Button(ExitPrompt,NSLOCTEXT("Appearance","Discard","Discard changes"),[this](){ForceClose();});
    Button(ExitPrompt,NSLOCTEXT("Appearance","KeepEditing","Keep editing"),[this](){SetExitPrompt(false);});
    ExitPrompt->SetVisibility(ESlateVisibility::Collapsed);
    auto* Footer=WidgetTree->ConstructWidget<UWrapBox>(); Footer->SetInnerSlotPadding(FVector2D(4,4)); Stack->AddChild(Footer);
    ApplyButton=Button(Footer,NSLOCTEXT("Appearance","Apply","Apply"),[this](){Apply();},true);
    RetryButton=Button(Footer,NSLOCTEXT("Appearance","RetrySave","Retry save"),[this](){RetrySave();});
    ReloadButton=Button(Footer,NSLOCTEXT("Appearance","Reload","Review current"),[this](){ReadAccepted(); BuildChoices(); RefreshPreview(); SetNotice(FText::GetEmpty()); RequestRefreshFocus();});
    CancelButton=Button(Footer,NSLOCTEXT("Appearance","Back","Back"),[this](){Close();});
    EditingPanels={Tabs,Body,ViewControls,Footer};
}
void UMythicAppearanceWidget::Open(UMythicCharacterPageWidget* InPage,UWidget* InReturn) {
    Page=InPage; ReturnFocus=InReturn; Catalog=UMythicAppearanceCatalog::Get();
    auto* PS=GetOwningPlayer()?GetOwningPlayer()->GetPlayerState<AMythicPlayerState>():nullptr;
    Appearance=PS?PS->FindComponentByClass<UMythicAppearanceComponent>():nullptr;
    if (!Appearance.IsValid() || !Catalog) return;
    Appearance->MarkEquipmentDirty();
    Appearance->OnAppearanceChanged.AddUniqueDynamic(this,&ThisClass::StateChanged);
    Appearance->OnAppearanceResult.AddUniqueDynamic(this,&ThisClass::Result);
    ReadAccepted(); BuildChoices(); bCloseAfter=false; bAwaiting=false; bUncertain=false; bCommitReconciled=false; Request.Invalidate(); SubmittedItemRevisions.Reset(); AppliedRevision=INDEX_NONE;
    if (!bOnLayer) if (auto* Manager=GetGameInstance()->GetSubsystem<UMythicUIManagerSubsystem>()) {
        bOnLayer=true;
        if (!Manager->TryAddWidgetInstanceToPlayerLayer(AppearanceLayer(),GetOwningPlayer(),this,AttachedLayer)) { bOnLayer=false; StopLocalWork(); return; }
    }
    if (!bOnLayer) { StopLocalWork(); return; }
    if (!IsActivated()) ActivateWidget();
    if (Appearance->GetLastReceipt().Session==Baseline.Session) Result(Appearance->GetLastReceipt());
    RefreshChoices();
}
void UMythicAppearanceWidget::NativeOnActivated() {
    Super::NativeOnActivated(); AddUIInputContext();
    if(auto* Input=UCommonInputSubsystem::Get(GetOwningLocalPlayer()))DeviceHandle=Input->OnInputMethodChangedNative.AddWeakLambda(this,[this](ECommonInputType){ResetPreviewAxes();});
    if(FSlateApplication::IsInitialized())InputActivationHandle=FSlateApplication::Get().OnApplicationActivationStateChanged().AddWeakLambda(this,[this](bool bActive){if(bActive && PreviewInput)PreviewInput->Start();else ResetPreviewAxes();});
    DeviceConnectionHandle=IPlatformInputDeviceMapper::Get().GetOnInputDeviceConnectionChange().AddWeakLambda(this,[this](EInputDeviceConnectionState,FPlatformUserId User,FInputDeviceId){if(const auto* Local=GetOwningLocalPlayer();Local && Local->GetPlatformUserId()==User)ResetPreviewAxes();});
    Preview=NewObject<UMythicAppearancePreview>(this);
    auto* Character=GetOwningPlayer()?Cast<AMythicCharacter>(GetOwningPlayer()->GetPawn()):nullptr;
    if (Character && Preview->Start(Character->GetMesh())) PreviewImage->SetBrushResourceObject(Preview->GetTexture());
    RefreshPreview();
}
void UMythicAppearanceWidget::ReadAccepted() {
    if (!Appearance.IsValid()) return;
    Baseline=Appearance->GetState(); Draft=Baseline.Recipe; Appearance->GetEquipment(Equipment); DraftItems.Reset();
    for (const auto& Item:Equipment) if (Catalog->FindGear(Item.Item)) DraftItems.Add(Item.ItemGuid,Item.Appearance);
}
void UMythicAppearanceWidget::AddChoice(FChoiceRow Row) {
    const int32 Index=Rows.Num();
    auto* Box=WidgetTree->ConstructWidget<UHorizontalBox>(); Choices->AddChild(Box);
    Row.Previous=Button(Box,NSLOCTEXT("Appearance","Previous","‹"),[this,Index](){Cycle(Index,-1);});
    Row.Button=Button(Box,FText::GetEmpty(),[this,Index](){Cycle(Index,1);});
    if (auto* RowSlot=Cast<UHorizontalBoxSlot>(Row.Button.IsValid()?Row.Button->Slot:nullptr)) RowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Rows.Add(MoveTemp(Row));
}
void UMythicAppearanceWidget::BuildChoices() {
    if (!Choices || !Catalog) return;
    Rows.Reset(); Choices->ClearChildren();
    auto Personal=[&](FText Label,FName FMythicAppearanceRecipe::* Field,FName Domain) {
        FChoiceRow R; R.Label=Label; R.Read=[this,Field](){return Draft.*Field;}; R.Write=[this,Field](FName V){Draft.*Field=V;};
        for (const auto& P:Catalog->Palettes) if(P.Domain==Domain) {R.Values.Add(P.Id); R.Names.Add(P.Label);}
        AddChoice(MoveTemp(R));
    };
    if (!bGearTab) {
        FChoiceRow Body; Body.Label=NSLOCTEXT("Appearance","BodyStyle","Body style");
        Body.Read=[this](){return Draft.Frame;}; Body.Write=[this](FName V){Draft.Frame=V; if(const auto* F=Catalog->FindFrame(V))Draft.Head=F->HeadId;};
        for(const auto& F:Catalog->Frames){Body.Values.Add(F.Id); Body.Names.Add(F.Label);} AddChoice(MoveTemp(Body));
        FChoiceRow Hair; Hair.Label=NSLOCTEXT("Appearance","Hair","Hair"); Hair.Values={FName("Adventurer"),FName("Bald")};
        Hair.Names={NSLOCTEXT("Appearance","Adventurer","Adventurer"),NSLOCTEXT("Appearance","Bald","Bald")};
        Hair.Read=[this](){return Draft.Hair;}; Hair.Write=[this](FName V){Draft.Hair=V;}; AddChoice(MoveTemp(Hair));
        Personal(NSLOCTEXT("Appearance","Skin","Skin"),&FMythicAppearanceRecipe::SkinPalette,FName("Skin"));
        Personal(NSLOCTEXT("Appearance","HairColor","Hair color"),&FMythicAppearanceRecipe::HairPalette,FName("Hair"));
        Personal(NSLOCTEXT("Appearance","EyeColor","Eye color"),&FMythicAppearanceRecipe::EyePalette,FName("Eyes"));
    } else {
        for (const auto& Item:Equipment) {
            const auto* Gear=Catalog->FindGear(Item.Item); if(!Gear)continue;
            auto* Name=FMythicUIStyle::MakeText(this,EMythicTextRole::Heading); Name->SetText(Item.Item->Name); Choices->AddChild(Name);
            const auto* Fit=Gear->Fits.FindByPredicate([this](const auto& F){return F.Frame==Draft.Frame;}); if(!Fit)continue;
            TSet<FName> Added;
            for(FName PartId:Fit->Parts) if(const auto* Part=Catalog->FindPart(PartId)) for(const auto& M:Part->Materials) {
                if(M.Channel==FName("Skin") || M.Channel==FName("Hair") || M.Channel==FName("Eyes") || M.Channel.IsNone() || Added.Contains(M.Channel))continue;
                Added.Add(M.Channel); const FGuid Guid=Item.ItemGuid; const FName Channel=M.Channel;
                FChoiceRow R; R.Label=M.Label; R.Values.Add(NAME_None); R.Names.Add(OriginalColor());
                for(const auto& P:Catalog->Palettes)if(P.Domain==M.PaletteDomain){R.Values.Add(P.Id);R.Names.Add(P.Label);}
                R.Read=[this,Guid,Channel](){const auto* D=DraftItems.Find(Guid); const auto* C=D?D->Colors.FindByPredicate([Channel](const auto& X){return X.Channel==Channel;}):nullptr;return C?C->Palette:NAME_None;};
                R.Write=[this,Guid,Channel](FName V){if(auto* D=DraftItems.Find(Guid)){D->Colors.RemoveAll([Channel](const auto& X){return X.Channel==Channel;});if(!V.IsNone())D->Colors.Add({Channel,V});}};
                AddChoice(MoveTemp(R));
            }
        }
        if(Rows.IsEmpty()){auto* Empty=FMythicUIStyle::MakeText(this,EMythicTextRole::Body);Empty->SetText(NSLOCTEXT("Appearance","NoGear","Equip compatible gear to customize its colors."));Empty->SetAutoWrapText(true);Choices->AddChild(Empty);}
    }
    for(int32 I=0;I<Rows.Num();++I){
        auto* B=Rows[I].Button.Get();auto* P=Rows[I].Previous.Get();
        if(!B || !P)continue;
        B->SetNavigationRuleExplicit(EUINavigation::Left,P);P->SetNavigationRuleExplicit(EUINavigation::Right,B);
        B->SetNavigationRuleExplicit(EUINavigation::Up,I?Rows[I-1].Button.Get():(bGearTab?GearTab.Get():BodyTab.Get()));
        P->SetNavigationRuleExplicit(EUINavigation::Up,I?Rows[I-1].Previous.Get():(bGearTab?GearTab.Get():BodyTab.Get()));
        B->SetNavigationRuleExplicit(EUINavigation::Down,I+1<Rows.Num()?Rows[I+1].Button.Get():FirstViewControl.Get());
        P->SetNavigationRuleExplicit(EUINavigation::Down,I+1<Rows.Num()?Rows[I+1].Previous.Get():FirstViewControl.Get());
    }
    auto* First=Rows.IsEmpty()?FirstViewControl.Get():Rows[0].Button.Get();
    if(BodyTab)BodyTab->SetNavigationRuleExplicit(EUINavigation::Down,First);
    if(GearTab)GearTab->SetNavigationRuleExplicit(EUINavigation::Down,First);
    if(FirstViewControl)FirstViewControl->SetNavigationRuleExplicit(EUINavigation::Up,Rows.IsEmpty()?(bGearTab?GearTab.Get():BodyTab.Get()):Rows.Last().Button.Get());
    RefreshChoices();
}
void UMythicAppearanceWidget::Cycle(int32 Index,int32 Direction) {
    if(bAwaiting || bUncertain || IsConflict() || !Rows.IsValidIndex(Index))return;
    auto& R=Rows[Index]; if(R.Values.IsEmpty())return;
    const int32 Old=R.Values.IndexOfByKey(R.Read()); const int32 Next=Old==INDEX_NONE?(Direction>0?0:R.Values.Num()-1):(Old+Direction+R.Values.Num())%R.Values.Num();
    R.Write(R.Values[Next]); SetNotice(NSLOCTEXT("Appearance","Draft","Preview only · Unapplied changes")); RefreshChoices(); RefreshPreview();
}
bool UMythicAppearanceWidget::IsDirty() const {
    if(Draft!=Baseline.Recipe)return true;
    for(const auto& Item:Equipment)if(const auto* D=DraftItems.Find(Item.ItemGuid);D && !D->HasSameColors(Item.Appearance))return true;
    return false;
}
bool UMythicAppearanceWidget::IsConflict() const {
    if(!Appearance.IsValid() || !Appearance->IsReady())return true;
    const auto Current=Appearance->GetState();return Current.Session!=Baseline.Session || Current.Revision!=Baseline.Revision;
}
void UMythicAppearanceWidget::RefreshChoices() {
    for(auto& R:Rows)if(auto* B=R.Button.Get()) {
        const int32 Index=R.Values.IndexOfByKey(R.Read());
        const FText Name=R.Names.IsValidIndex(Index)?R.Names[Index]:NSLOCTEXT("Appearance","Unavailable","Unavailable saved choice");
        B->SetLabelOverride(FText::Format(NSLOCTEXT("Appearance","Choice","{0}: {1}"),R.Label,Name));
    }
    if(ApplyButton)ApplyButton->SetIsEnabled(!bAwaiting && !bUncertain && !IsConflict() && IsDirty() && bPreviewResolved && Preview && Preview->IsReady());
    if(BodyTab)BodyTab->SetStyle((bGearTab?FMythicUIStyle::Get().SecondaryActionButtonStyle:FMythicUIStyle::Get().PrimaryActionButtonStyle).LoadSynchronous());
    if(GearTab)GearTab->SetStyle((bGearTab?FMythicUIStyle::Get().PrimaryActionButtonStyle:FMythicUIStyle::Get().SecondaryActionButtonStyle).LoadSynchronous());
    if(ReloadButton)ReloadButton->SetVisibility(IsConflict() && !bAwaiting?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    if(CancelButton)CancelButton->SetLabelOverride(NSLOCTEXT("Appearance","Back","Back"));
    const auto Last=Appearance.IsValid()?Appearance->GetLastReceipt():FMythicAppearanceReceipt{};
    if(RetryButton)RetryButton->SetVisibility(Last.Session==Baseline.Session && Last.Result==EMythicAppearanceResult::SaveFailed?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
}
void UMythicAppearanceWidget::RefreshPreview() {
    if(!Preview || !Catalog)return;
    auto Items=Equipment;for(auto& Item:Items)if(const auto* D=DraftItems.Find(Item.ItemGuid))Item.Appearance=*D;
    FMythicAppearancePublicState Plan; bPreviewResolved=Catalog->ResolveOutfit(Draft,Items,Plan);
    if(bPreviewResolved)Preview->Apply(Plan);
    RefreshChoices();
}
void UMythicAppearanceWidget::SetTab(bool bGear) { if(!IsActivated() || ExitPrompt->GetVisibility()!=ESlateVisibility::Collapsed || bGearTab==bGear)return;bGearTab=bGear;BuildChoices();RequestRefreshFocus(); }
void UMythicAppearanceWidget::Apply(bool bInCloseAfter) {
    if(bAwaiting || bUncertain || IsConflict() || !IsDirty() || !bPreviewResolved || !Preview || !Preview->IsReady())return;
    FMythicAppearanceEditRequest Edit; Edit.RequestId=FGuid::NewGuid(); Edit.Session=Baseline.Session; Edit.ExpectedRevision=Baseline.Revision;
    Edit.bChangePersonal=Draft!=Baseline.Recipe;Edit.Personal=Draft;
    for(const auto& Item:Equipment)if(const auto* D=DraftItems.Find(Item.ItemGuid);D && !D->HasSameColors(Item.Appearance)) {
        FMythicAppearanceItemEdit E; E.ItemGuid=Item.ItemGuid;E.ExpectedRevision=Item.Appearance.Revision;
        TSet<FName> Channels;for(const auto& C:D->Colors)Channels.Add(C.Channel);for(const auto& C:Item.Appearance.Colors)Channels.Add(C.Channel);
        for(FName Channel:Channels){const auto* Old=Item.Appearance.Colors.FindByPredicate([Channel](const auto& C){return C.Channel==Channel;});const auto* New=D->Colors.FindByPredicate([Channel](const auto& C){return C.Channel==Channel;});if((Old?Old->Palette:NAME_None)!=(New?New->Palette:NAME_None))E.Changes.Add({Channel,New?New->Palette:NAME_None});}
        Edit.Items.Add(MoveTemp(E));
    }
    SubmittedItemRevisions.Reset(); for(const auto& Item:Edit.Items)SubmittedItemRevisions.Add(Item.ItemGuid,Item.ExpectedRevision+1);
    Request=Edit.RequestId;bAwaiting=true;bCloseAfter=bInCloseAfter;bCommitReconciled=false;AppliedRevision=INDEX_NONE;
    SetNotice(NSLOCTEXT("Appearance","Applying","Applying…"));RefreshChoices();
    GetWorld()->GetTimerManager().SetTimer(ResponseDeadline,FTimerDelegate::CreateWeakLambda(this,[this](){
        bAwaiting=false;bUncertain=true;bCloseAfter=false;
        SetNotice(NSLOCTEXT("Appearance","Uncertain","Server response delayed. You can close this window; the submitted change may still complete."));RefreshChoices();
    }),10.f,false);
    Appearance->ServerApplyAppearance(Edit);
}
void UMythicAppearanceWidget::RetrySave() {
    if(!Appearance.IsValid())return;const auto Last=Appearance->GetLastReceipt();
    Appearance->ServerRetryAppearanceSave(Last.RequestId,Last.Session,Last.Revision);SetNotice(NSLOCTEXT("Appearance","Retrying","Retrying save…"));
}
void UMythicAppearanceWidget::Result(const FMythicAppearanceReceipt& Receipt) {
    if(Receipt.Session!=Baseline.Session || (Request.IsValid() && Receipt.RequestId!=Request))return;
    SetNotice(UMythicAppearanceComponent::DescribeResult(Receipt.Result));
    if(Receipt.RequestId==Request && !bCommitReconciled) {
        bUncertain=false;
        if(Receipt.Result==EMythicAppearanceResult::Applied || Receipt.Result==EMythicAppearanceResult::Saved || Receipt.Result==EMythicAppearanceResult::SaveFailed){AppliedRevision=Receipt.Revision;bAwaiting=true;}
        else {GetWorld()->GetTimerManager().ClearTimer(ResponseDeadline);bAwaiting=false;Request.Invalidate();bCloseAfter=false;}
    }
    StateChanged();
}
void UMythicAppearanceWidget::StateChanged() {
    if(!Appearance.IsValid())return;
    const auto Current=Appearance->GetState();
    if(Current.Session!=Baseline.Session){bAwaiting=false;bUncertain=false;bCloseAfter=false;Request.Invalidate();GetWorld()->GetTimerManager().ClearTimer(ResponseDeadline);}
    TArray<FMythicAppearanceEquippedItem> Arrived; Appearance->GetEquipment(Arrived);
    bool bItemsArrived=true;
    for(const auto& Expected:SubmittedItemRevisions) bItemsArrived &= Arrived.ContainsByPredicate([&Expected](const auto& Item){return Item.ItemGuid==Expected.Key && Item.Appearance.Revision>=Expected.Value;});
    if((bAwaiting || bUncertain) && AppliedRevision!=INDEX_NONE && Current.Session==Baseline.Session && Current.Revision>=AppliedRevision && bItemsArrived) {
        GetWorld()->GetTimerManager().ClearTimer(ResponseDeadline);
        bAwaiting=false;bUncertain=false;bCommitReconciled=true;AppliedRevision=INDEX_NONE;SubmittedItemRevisions.Reset();ReadAccepted();BuildChoices();RefreshPreview();if(bCloseAfter){ForceClose();return;}
    } else if(IsConflict() && !bAwaiting)SetNotice(NSLOCTEXT("Appearance","Conflict","Your character or gear changed. Review current appearance before applying."));
    RefreshChoices();
}
void UMythicAppearanceWidget::SetNotice(const FText& Text){if(Notice)Notice->SetText(Text);}
void UMythicAppearanceWidget::SetExitPrompt(bool bShow){for(const auto& Panel:EditingPanels)if(Panel)Panel->SetIsEnabled(!bShow);OrbitAxis=ZoomAxis=LeftZoom=RightZoom=0;ExitPrompt->SetVisibility(bShow?ESlateVisibility::Visible:ESlateVisibility::Collapsed);if(bShow && ExitPrompt->GetChildrenCount()>3)ExitPrompt->GetChildAt(3)->SetUserFocus(GetOwningPlayer());else if(CancelButton)CancelButton->SetUserFocus(GetOwningPlayer());}
void UMythicAppearanceWidget::Close(){if(bAwaiting || bUncertain){ForceClose();return;}if(ExitPrompt->GetVisibility()!=ESlateVisibility::Collapsed){SetExitPrompt(false);return;}if(IsDirty()){SetExitPrompt(true);return;}ForceClose();}
void UMythicAppearanceWidget::ForceClose() {
    const bool bShown=bOnLayer;bOnLayer=false;SetExitPrompt(false);
    if(auto* Layer=AttachedLayer.Get())Layer->RemoveWidget(*this);
    AttachedLayer.Reset();
    if(IsActivated())DeactivateWidget();else StopLocalWork();
    if(bShown && Page.IsValid() && Page->IsActivated() && ReturnFocus.IsValid())ReturnFocus->SetUserFocus(GetOwningPlayer());
}
bool UMythicAppearanceWidget::NativeOnHandleBackAction(){Close();return true;}
UWidget* UMythicAppearanceWidget::NativeGetDesiredFocusTarget() const {
    auto* Focus=LastFocus.Get();auto* Player=GetOwningPlayer();
    if(Player && Focus && Focus->GetIsEnabled() && Focus->IsVisible() && (Focus->HasUserFocus(Player)||Focus->HasUserFocusedDescendants(Player)))return Focus;
    if(ExitPrompt && ExitPrompt->GetVisibility()!=ESlateVisibility::Collapsed && ExitPrompt->GetChildrenCount()>3)return ExitPrompt->GetChildAt(3);
    return bGearTab?GearTab:BodyTab;
}
void UMythicAppearanceWidget::StopLocalWork(){StopPreviewInput();if(GetWorld())GetWorld()->GetTimerManager().ClearTimer(ResponseDeadline);bDragging=false;OrbitAxis=ZoomAxis=LeftZoom=RightZoom=0;if(PreviewImage)PreviewImage->SetBrushResourceObject(nullptr);if(Preview)Preview->Stop();Preview=nullptr;if(Appearance.IsValid()){Appearance->OnAppearanceChanged.RemoveDynamic(this,&ThisClass::StateChanged);Appearance->OnAppearanceResult.RemoveDynamic(this,&ThisClass::Result);}Appearance.Reset();Rows.Reset();DraftItems.Reset();Equipment.Reset();}
void UMythicAppearanceWidget::NativeOnDeactivated(){
    StopPreviewInput();
    bDragging=false;OrbitAxis=ZoomAxis=LeftZoom=RightZoom=0;
    if(PreviewImage)PreviewImage->SetBrushResourceObject(nullptr);if(Preview)Preview->Stop();Preview=nullptr;
    if(!bOnLayer)StopLocalWork();
    RemoveUIInputContext();Super::NativeOnDeactivated();
}
void UMythicAppearanceWidget::NativeDestruct(){StopLocalWork();Super::NativeDestruct();}
void UMythicAppearanceWidget::NativeTick(const FGeometry& Geometry,float Delta){
    Super::NativeTick(Geometry,Delta);
    if(!FSlateApplication::IsInitialized() || !FSlateApplication::Get().IsActive())OrbitAxis=ZoomAxis=LeftZoom=RightZoom=0;
    if(IsActivated() && Preview){
        Preview->Orbit(OrbitAxis*90*Delta);Preview->Zoom(ZoomAxis*100*Delta);Preview->Tick(Delta);
        const bool bReady=bPreviewResolved && Preview->IsReady();
        if(bReady!=bPreviewWasReady){bPreviewWasReady=bReady;RefreshChoices();}
        const FText Status=bReady?FText::GetEmpty():(!bPreviewResolved || Preview->HasFailed()?NSLOCTEXT("Appearance","PreviewFailed","Preview unavailable. Choose another option or reopen customization."):NSLOCTEXT("Appearance","PreviewLoading","Loading preview…"));
        if(PreviewStatus && !PreviewStatus->GetText().EqualTo(Status))PreviewStatus->SetText(Status);
    }
}
FReply UMythicAppearanceWidget::NativeOnAnalogValueChanged(const FGeometry& G,const FAnalogInputEvent& E){const float V=FMath::Abs(E.GetAnalogValue())>0.18f?E.GetAnalogValue():0;if(E.GetKey()==EKeys::Gamepad_RightX){OrbitAxis=V;return FReply::Handled();}if(E.GetKey()==EKeys::Gamepad_LeftTriggerAxis){LeftZoom=V;ZoomAxis=LeftZoom-RightZoom;return FReply::Handled();}if(E.GetKey()==EKeys::Gamepad_RightTriggerAxis){RightZoom=V;ZoomAxis=LeftZoom-RightZoom;return FReply::Handled();}return Super::NativeOnAnalogValueChanged(G,E);}
FReply UMythicAppearanceWidget::NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& E){if(E.GetEffectingButton()==EKeys::LeftMouseButton && PreviewImage && PreviewImage->GetCachedGeometry().IsUnderLocation(E.GetScreenSpacePosition())){bDragging=true;return FReply::Handled().CaptureMouse(TakeWidget());}return Super::NativeOnMouseButtonDown(G,E);}
FReply UMythicAppearanceWidget::NativeOnMouseButtonUp(const FGeometry& G,const FPointerEvent& E){if(bDragging && E.GetEffectingButton()==EKeys::LeftMouseButton){bDragging=false;return FReply::Handled().ReleaseMouseCapture();}return Super::NativeOnMouseButtonUp(G,E);}
FReply UMythicAppearanceWidget::NativeOnMouseMove(const FGeometry& G,const FPointerEvent& E){OrbitAxis=ZoomAxis=LeftZoom=RightZoom=0;if(bDragging && Preview){Preview->Orbit(E.GetCursorDelta().X*0.5f);return FReply::Handled();}return Super::NativeOnMouseMove(G,E);}
FReply UMythicAppearanceWidget::NativeOnMouseWheel(const FGeometry& G,const FPointerEvent& E){if(Preview && PreviewImage->GetCachedGeometry().IsUnderLocation(E.GetScreenSpacePosition())){Preview->Zoom(-E.GetWheelDelta()*20);return FReply::Handled();}return Super::NativeOnMouseWheel(G,E);}
void UMythicAppearanceWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& E){bDragging=false;Super::NativeOnMouseCaptureLost(E);}
void UMythicAppearanceWidget::ResetPreviewAxes(){
    if(OrbitAxis!=0)AxesAwaitingNeutral.Add(EKeys::Gamepad_RightX);
    if(LeftZoom!=0)AxesAwaitingNeutral.Add(EKeys::Gamepad_LeftTriggerAxis);
    if(RightZoom!=0)AxesAwaitingNeutral.Add(EKeys::Gamepad_RightTriggerAxis);
    OrbitAxis=ZoomAxis=LeftZoom=RightZoom=0;
}
void UMythicAppearanceWidget::StopPreviewInput(){
    ResetPreviewAxes();auto Old=MoveTemp(PreviewInput);if(Old)Old->Stop();
    if(auto* Input=UCommonInputSubsystem::Get(GetOwningLocalPlayer()))Input->OnInputMethodChangedNative.Remove(DeviceHandle);
    DeviceHandle.Reset();
    if(FSlateApplication::IsInitialized())FSlateApplication::Get().OnApplicationActivationStateChanged().Remove(InputActivationHandle);
    InputActivationHandle.Reset();
    IPlatformInputDeviceMapper::Get().GetOnInputDeviceConnectionChange().Remove(DeviceConnectionHandle);DeviceConnectionHandle.Reset();
}
void UMythicAppearanceWidget::NativeOnAddedToFocusPath(const FFocusEvent& Event){
    Super::NativeOnAddedToFocusPath(Event);
    if(!PreviewInput){
        const TWeakObjectPtr<UMythicAppearanceWidget> Weak(this);
        FMythicModalInputProcessor::FRoutes Routes;
        Routes.Analog=[Weak](const FAnalogInputEvent& Axis){
            auto* Self=Weak.Get();if(!Self)return false;
            const auto Key=Axis.GetKey();
            if(Key!=EKeys::Gamepad_RightX && Key!=EKeys::Gamepad_RightY && Key!=EKeys::Gamepad_LeftTriggerAxis && Key!=EKeys::Gamepad_RightTriggerAxis)return false;
            if(Self->AxesAwaitingNeutral.Contains(Key)){if(FMath::Abs(Axis.GetAnalogValue())<=0.18f)Self->AxesAwaitingNeutral.Remove(Key);return true;}
            if(Self->ExitPrompt->GetVisibility()!=ESlateVisibility::Collapsed){Self->ResetPreviewAxes();return true;}
            if(Key!=EKeys::Gamepad_RightY)Self->NativeOnAnalogValueChanged(Self->GetCachedGeometry(),Axis);
            return true;
        };
        Routes.FocusLost=[Weak](){if(auto* Self=Weak.Get())Self->ResetPreviewAxes();};
        PreviewInput=MakeShared<FMythicModalInputProcessor>(this,MoveTemp(Routes));
    }
    PreviewInput->Start();
}
