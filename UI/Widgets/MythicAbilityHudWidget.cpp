#include "MythicAbilityHudWidget.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "Input/MythicInputConfig.h"
#include "Input/MythicControlsComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Player/MythicCharacter.h"
#include "Player/MythicPlayerState.h"
#include "Player/MythicPlayerController.h"
#include "Progression/Skills/MythicSkillComponent.h"
#include "UI/MythicUIKit.h"
#include "UI/MythicUIStyle.h"
#include "UI/Controls/MythicControlGlyph.h"
#include "UI/Controls/MythicQuickUseHudCell.h"
#include "TimerManager.h"

UMythicAbilityHudWidget::UMythicAbilityHudWidget(const FObjectInitializer& Initializer):Super(Initializer) { SetIsFocusable(false); }

void UMythicAbilityHudWidget::NativeOnInitialized() { Super::NativeOnInitialized(); BuildCells(); }

void UMythicAbilityHudWidget::BuildCells() {
    if (!WidgetTree || !Cells.IsEmpty()) return;
    auto* Root=WidgetTree->ConstructWidget<UVerticalBox>(); WidgetTree->RootWidget=Root;
    CameraAdjustmentHint=WidgetTree->ConstructWidget<UHorizontalBox>();
    Root->AddChildToVerticalBox(CameraAdjustmentHint)->SetPadding(FMargin(0,0,0,6));
    const auto AddHint=[this](const FText& Text) {
        auto* Label=FMythicUIStyle::MakeText(this,EMythicTextRole::Subtle); Label->SetText(Text);
        CameraAdjustmentHint->AddChildToHorizontalBox(Label)->SetPadding(FMargin(4,0));
    };
    const auto AddGlyph=[this](EMythicControl Control) {
        auto* Glyph=WidgetTree->ConstructWidget<UMythicControlGlyph>(); Glyph->SetControl(Control); Glyph->GlyphHeight=22;
        CameraAdjustmentHint->AddChildToHorizontalBox(Glyph)->SetVerticalAlignment(VAlign_Center);
    };
    AddHint(NSLOCTEXT("Mythic","CameraAdjustmentMode","Camera adjustment"));
    AddGlyph(EMythicControl::Secondary); AddGlyph(EMythicControl::Primary);
    AddHint(NSLOCTEXT("Mythic","CameraAdjustmentZoom","Zoom"));
    AddGlyph(EMythicControl::CameraMode); AddHint(NSLOCTEXT("Mythic","CameraAdjustmentExit","Exit"));
    CameraAdjustmentHint->SetVisibility(ESlateVisibility::Collapsed);
    auto* Row=WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),TEXT("AbilityRow"));
    Root->AddChildToVerticalBox(Row);
    const auto* Kit=UMythicUIKit::Get();
    auto* Heal=WidgetTree->ConstructWidget<UMythicQuickUseHudCell>(); Heal->InitializePurpose(true);
    auto* HealSlot=Row->AddChildToHorizontalBox(Heal); HealSlot->SetVerticalAlignment(VAlign_Bottom); HealSlot->SetPadding(FMargin(0,0,14,0));
    Cells.SetNum(UMythicSkillComponent::SlotCount);
    for (int32 Index=0;Index<UMythicSkillComponent::SlotCount;++Index) {
        auto& Cell=Cells[Index];
        auto* Column=WidgetTree->ConstructWidget<UVerticalBox>();
        auto* CellWidth=WidgetTree->ConstructWidget<USizeBox>(); CellWidth->SetWidthOverride(72); CellWidth->AddChild(Column);
        Row->AddChildToHorizontalBox(CellWidth)->SetPadding(FMargin(Index ? FMythicUIStyle::Get().SpaceS : 0,0,0,0));
        auto* Well=WidgetTree->ConstructWidget<USizeBox>(); Well->SetWidthOverride(64); Well->SetHeightOverride(64);
        Column->AddChildToVerticalBox(Well)->SetHorizontalAlignment(HAlign_Center);
        auto* Layers=WidgetTree->ConstructWidget<UOverlay>(); Well->AddChild(Layers);
        auto* Plate=WidgetTree->ConstructWidget<UImage>(); Plate->SetBrush(FMythicUIStyle::MakeSlotBrush()); Layers->AddChildToOverlay(Plate);
        Cell.Icon=WidgetTree->ConstructWidget<UImage>(); Cell.Icon->SetDesiredSizeOverride(FVector2D(52,52));
        auto* IconSlot=Layers->AddChildToOverlay(Cell.Icon); IconSlot->SetHorizontalAlignment(HAlign_Center); IconSlot->SetVerticalAlignment(VAlign_Center);
        Cell.Sweep=WidgetTree->ConstructWidget<UImage>();
        if (Kit) Cell.Sweep->SetBrush(Kit->MakeBrush(TEXT("Radial.Sweep"),EMythicUIState::Normal,FVector2D(64,64)));
        Layers->AddChildToOverlay(Cell.Sweep); Cell.SweepMaterial=Cell.Sweep->GetDynamicMaterial();
        Cell.Countdown=FMythicUIStyle::MakeText(this,EMythicTextRole::Body);
        auto* NumberSlot=Layers->AddChildToOverlay(Cell.Countdown); NumberSlot->SetHorizontalAlignment(HAlign_Center); NumberSlot->SetVerticalAlignment(VAlign_Center);
        Cell.Glyph=WidgetTree->ConstructWidget<UMythicControlGlyph>(); Cell.Glyph->GlyphHeight=22;
        auto* GlyphSlot=Column->AddChildToVerticalBox(Cell.Glyph); GlyphSlot->SetHorizontalAlignment(HAlign_Center); GlyphSlot->SetPadding(FMargin(0,2,0,0));
        Cell.State=FMythicUIStyle::MakeText(this,EMythicTextRole::Subtle);
        auto Font=Cell.State->GetFont(); Font.Size=12; Cell.State->SetFont(Font);
        auto* StateSlot=Layers->AddChildToOverlay(Cell.State); StateSlot->SetHorizontalAlignment(HAlign_Center); StateSlot->SetVerticalAlignment(VAlign_Bottom);
        Cell.Icon->SetVisibility(ESlateVisibility::Collapsed); Cell.Sweep->SetVisibility(ESlateVisibility::Collapsed); Cell.Countdown->SetVisibility(ESlateVisibility::Collapsed);
    }
    SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UMythicAbilityHudWidget::NativeConstruct() {
    Super::NativeConstruct(); bConstructed=true; BuildCells(); BindAttempts=0;
    if (!BindToLocalPlayer()) RetryBinding();
}

void UMythicAbilityHudWidget::RetryBinding() {
    if (!bConstructed || !GetWorld()) return;
    BindAttempts=0;
    GetWorld()->GetTimerManager().SetTimer(BindRetry,FTimerDelegate::CreateWeakLambda(this,[this]() {
        if (BindToLocalPlayer() || ++BindAttempts>=20) GetWorld()->GetTimerManager().ClearTimer(BindRetry);
    }),0.25f,true);
}

bool UMythicAbilityHudWidget::BindToLocalPlayer() {
    auto* PC=GetOwningPlayer();
    auto* PS=PC && PC->IsLocalController() ? PC->GetPlayerState<AMythicPlayerState>() : nullptr;
    auto* NewASC=PS ? PS->GetMythicAbilitySystemComponent() : nullptr;
    auto* NewSkills=PS ? PS->GetSkillComponent() : nullptr;
    if (!bConstructed) return false;
    if (!PC || !PC->GetLocalPlayer() || !NewASC || !NewSkills) {
        Unbind();
        // Possession may be announced before PlayerState arrives. Keep this local controller's event
        // route while the bounded initial binding retry waits for its private components.
        if (PC && PC->GetLocalPlayer()) BindController(PC);
        return false;
    }
    if (ASC.Get()!=NewASC || Skills.Get()!=NewSkills || BoundController.Get()!=PC) {
        Unbind(); ASC=NewASC; Skills=NewSkills; BindController(PC);
        NewSkills->OnSkillsChanged.AddUniqueDynamic(this,&ThisClass::QueueRefresh);
        Activated=NewASC->AbilityActivatedCallbacks.AddUObject(this,&ThisClass::HandleAbilityChanged);
        Ended=NewASC->AbilityEndedCallbacks.AddUObject(this,&ThisClass::HandleAbilityChanged);
        Added=NewASC->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this,&ThisClass::HandleEffectAdded);
        Removed=NewASC->OnAnyGameplayEffectRemovedDelegate().AddUObject(this,&ThisClass::HandleEffectRemoved);
        TagsChanged=NewASC->RegisterGenericGameplayTagEvent().AddUObject(this,&ThisClass::HandleTagChanged);
        ProjectionChanged=NewASC->OnAbilityProjectionChanged.AddUObject(this,&ThisClass::QueueRefresh);
        for (const auto Attribute:{UMythicAttributeSet_Life::GetHealthAttribute(),UMythicAttributeSet_Life::GetMaxHealthAttribute(),UMythicAttributeSet_Defense::GetShieldAttribute(),
            UMythicAttributeSet_Utility::GetCurrentStaminaAttribute(),UMythicAttributeSet_Utility::GetStaminaCostReductionAttribute()})
            AttributeListeners.Emplace(Attribute,NewASC->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(this,&ThisClass::HandleAttributeChanged));
    }
    QueueRefresh(); return true;
}

void UMythicAbilityHudWidget::Unbind() {
    if (auto* Controls=BoundControls.Get()) Controls->OnCameraAdjustmentChanged.Remove(CameraAdjustmentChanged);
    CameraAdjustmentChanged.Reset(); BoundControls.Reset(); HandleCameraAdjustmentChanged(false);
    if (auto* PC=Cast<AMythicPlayerController>(BoundController.Get())) PC->OnLocalGameplayOwnerChanged.Remove(OwnerChanged);
    if (auto* PC=BoundController.Get()) PC->OnPossessedPawnChanged.RemoveDynamic(this,&ThisClass::HandlePawnChanged);
    if (auto* Component=Skills.Get()) Component->OnSkillsChanged.RemoveDynamic(this,&ThisClass::QueueRefresh);
    if (auto* Component=ASC.Get()) {
        Component->AbilityActivatedCallbacks.Remove(Activated); Component->AbilityEndedCallbacks.Remove(Ended);
        Component->OnActiveGameplayEffectAddedDelegateToSelf.Remove(Added); Component->OnAnyGameplayEffectRemovedDelegate().Remove(Removed);
        Component->RegisterGenericGameplayTagEvent().Remove(TagsChanged); Component->OnAbilityProjectionChanged.Remove(ProjectionChanged);
        for (const auto& Pair:AttributeListeners) Component->GetGameplayAttributeValueChangeDelegate(Pair.Key).Remove(Pair.Value);
        for (const auto& Pair:CooldownTimeListeners) if (auto* Events=Component->GetActiveEffectEventSet(Pair.Key)) Events->OnTimeChanged.Remove(Pair.Value);
    }
    CooldownTimeListeners.Reset(); AttributeListeners.Reset(); ASC.Reset(); Skills.Reset(); BoundController.Reset();
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(CooldownTimer);
    for (auto& Cell:Cells) {
        ++Cell.IconRevision; if (Cell.IconLoad) Cell.IconLoad->CancelHandle(); Cell.IconLoad.Reset(); Cell.IconPath.Reset();
        Cell.Definition=nullptr; Cell.SpecHandle={}; Cell.CooldownEnds=0; Cell.ShownSeconds=INDEX_NONE;
        Cell.Icon->SetBrushFromTexture(nullptr); Cell.Icon->SetVisibility(ESlateVisibility::Collapsed);
        Cell.Sweep->SetVisibility(ESlateVisibility::Collapsed); Cell.Sweep->ForceVolatile(false);
        Cell.Countdown->SetVisibility(ESlateVisibility::Collapsed); Cell.Glyph->SetControl(EMythicControl::Count);
        Cell.State->SetText(NSLOCTEXT("Mythic","AbilityHudLoading","Loading"));
    }
}

void UMythicAbilityHudWidget::NativeDestruct() {
    bConstructed=false; Unbind();
    if (GetWorld()) { GetWorld()->GetTimerManager().ClearTimer(BindRetry); GetWorld()->GetTimerManager().ClearTimer(RefreshTimer); GetWorld()->GetTimerManager().ClearTimer(CooldownTimer); }
    bRefreshQueued=false;
    for (auto& Cell:Cells) { ++Cell.IconRevision; if (Cell.IconLoad) Cell.IconLoad->CancelHandle(); Cell.IconLoad.Reset(); Cell.IconPath.Reset(); Cell.Definition=nullptr; }
    Super::NativeDestruct();
}

void UMythicAbilityHudWidget::QueueRefresh() {
    if (!bConstructed || bRefreshQueued || !GetWorld()) return;
    bRefreshQueued=true; RefreshTimer=GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this,&ThisClass::Refresh));
}
void UMythicAbilityHudWidget::BindController(APlayerController* PC) {
    BoundController=PC; PC->OnPossessedPawnChanged.AddUniqueDynamic(this,&ThisClass::HandlePawnChanged);
    if (auto* MythicPC=Cast<AMythicPlayerController>(PC)) OwnerChanged=MythicPC->OnLocalGameplayOwnerChanged.AddUObject(this,&ThisClass::HandleOwnerChanged);
    BoundControls=PC->FindComponentByClass<UMythicControlsComponent>();
    if (auto* Controls=BoundControls.Get()) {
        CameraAdjustmentChanged=Controls->OnCameraAdjustmentChanged.AddUObject(this,&ThisClass::HandleCameraAdjustmentChanged);
        HandleCameraAdjustmentChanged(Controls->IsCameraAdjustmentOpen());
    }
}
void UMythicAbilityHudWidget::HandleCameraAdjustmentChanged(bool bOpen) {
    if (CameraAdjustmentHint) CameraAdjustmentHint->SetVisibility(bOpen ? ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
}
void UMythicAbilityHudWidget::HandleOwnerChanged() { if (!BindToLocalPlayer()) RetryBinding(); QueueRefresh(); }
void UMythicAbilityHudWidget::HandlePawnChanged(APawn*,APawn*) { HandleOwnerChanged(); }
void UMythicAbilityHudWidget::HandleTagChanged(FGameplayTag,int32) { QueueRefresh(); }
void UMythicAbilityHudWidget::HandleAttributeChanged(const FOnAttributeChangeData&) { QueueRefresh(); }
void UMythicAbilityHudWidget::HandleAbilityChanged(UGameplayAbility* Ability) {
    if (const auto* Mythic=Cast<UMythicGameplayAbility>(Ability); Mythic && Mythic->GetActivationGroup()!=EMythicAbilityActivationGroup::Independent) { QueueRefresh(); return; }
    if (Ability) for (const auto& Cell:Cells) if (Cell.SpecHandle==Ability->GetCurrentAbilitySpecHandle()) { QueueRefresh(); break; }
}
bool UMythicAbilityHudWidget::IsCooldownTag(const FGameplayTagContainer& Tags) const {
    for (const auto& Cell:Cells) if (Cell.Definition && Cell.Definition->HasTimedCooldown() && Tags.HasTagExact(Cell.Definition->CooldownTag)) return true;
    return false;
}
void UMythicAbilityHudWidget::HandleEffectAdded(UAbilitySystemComponent*,const FGameplayEffectSpec& Spec,FActiveGameplayEffectHandle) {
    FGameplayTagContainer Tags; Spec.GetAllGrantedTags(Tags); if (IsCooldownTag(Tags)) QueueRefresh();
}
void UMythicAbilityHudWidget::HandleEffectRemoved(const FActiveGameplayEffect& Effect) {
    FGameplayTagContainer Tags; Effect.Spec.GetAllGrantedTags(Tags); if (IsCooldownTag(Tags)) QueueRefresh();
}

void UMythicAbilityHudWidget::RefreshIcon(int32 Index,const FSoftObjectPath& Path) {
    auto& Cell=Cells[Index]; if (Cell.IconPath==Path) return;
    Cell.IconPath=Path; const uint32 Revision=++Cell.IconRevision;
    if (Cell.IconLoad) Cell.IconLoad->CancelHandle(); Cell.IconLoad.Reset();
    Cell.Icon->SetBrushFromTexture(nullptr); Cell.Icon->SetVisibility(ESlateVisibility::Collapsed);
    if (Path.IsNull()) return;
    auto SetLoaded=[this,Index,Path,Revision]() {
        if (!bConstructed || !Cells.IsValidIndex(Index) || Cells[Index].IconRevision!=Revision || Cells[Index].IconPath!=Path) return;
        if (auto* Texture=Cast<UTexture2D>(Path.ResolveObject())) { Cells[Index].Icon->SetBrushFromTexture(Texture); Cells[Index].Icon->SetVisibility(ESlateVisibility::HitTestInvisible); }
    };
    if (Path.ResolveObject()) SetLoaded();
    else Cell.IconLoad=UAssetManager::GetStreamableManager().RequestAsyncLoad(Path,FStreamableDelegate::CreateWeakLambda(this,SetLoaded));
}

void UMythicAbilityHudWidget::Refresh() {
    bRefreshQueued=false;
    if (!bConstructed || !GetWorld()) return;
    auto* Component=ASC.Get(); auto* OwnerSkills=Skills.Get();
    const auto* PS=GetOwningPlayer() ? GetOwningPlayer()->GetPlayerState<AMythicPlayerState>() : nullptr;
    if (!Component || !OwnerSkills || !PS || PS->GetMythicAbilitySystemComponent()!=Component || PS->GetSkillComponent()!=OwnerSkills) {
        if (!BindToLocalPlayer()) RetryBinding(); return;
    }
    for (const auto& Pair:CooldownTimeListeners) if (auto* Events=Component->GetActiveEffectEventSet(Pair.Key)) Events->OnTimeChanged.Remove(Pair.Value);
    CooldownTimeListeners.Reset();
    const double Now=GetWorld()->GetTimeSeconds();
    bool Timed=false;
    const auto* Character=GetOwningPlayerPawn<AMythicCharacter>();
    for (int32 Index=0;Index<Cells.Num();++Index) {
        auto& Cell=Cells[Index]; Cell.Definition=OwnerSkills->GetSkillInSlot(Index); Cell.SpecHandle={};
        Cell.Glyph->SetControl(static_cast<EMythicControl>(static_cast<uint8>(EMythicControl::SkillQ)+Index));
        RefreshIcon(Index,Cell.Definition ? Cell.Definition->Icon.ToSoftObjectPath() : FSoftObjectPath{});
        FGameplayAbilitySpec* Spec=nullptr;
        if (Cell.Definition) for (auto& Candidate:Component->GetActivatableAbilities())
            if (Candidate.SourceObject.Get()==Cell.Definition && Candidate.GetDynamicSpecSourceTags().HasTagExact(OwnerSkills->GetSlotInputTag(Index))) { Spec=&Candidate; Cell.SpecHandle=Candidate.Handle; break; }
        float Remaining=0,Duration=0; OwnerSkills->GetCooldownDisplay(Cell.Definition,Remaining,Duration);
        Cell.CooldownEnds=Remaining>0 ? Now+Remaining : 0; Cell.CooldownDuration=Duration; Timed|=Remaining>0;
        if (Cell.Definition && Cell.Definition->HasTimedCooldown()) {
            const auto Handles=Component->GetActiveEffects(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(Cell.Definition->CooldownTag.GetSingleTagContainer()));
            for (const auto Handle:Handles) if (!CooldownTimeListeners.Contains(Handle)) if (auto* Events=Component->GetActiveEffectEventSet(Handle))
                CooldownTimeListeners.Add(Handle,Events->OnTimeChanged.AddWeakLambda(this,[this](FActiveGameplayEffectHandle,float,float) { QueueRefresh(); }));
        }
        FText State;
        const bool Active=Spec && Spec->IsActive();
        const auto* Ability=Spec ? (Spec->GetPrimaryInstance() ? Spec->GetPrimaryInstance() : Spec->Ability.Get()) : nullptr;
        const bool Ready=Ability && Component->AbilityActorInfo.IsValid() && OwnerSkills->IsReady()
            && Character && Component->GetAvatarActor()==Character
            && Component->CanAcceptAbilityInput() && Ability->CanActivateAbility(Spec->Handle,Component->AbilityActorInfo.Get());
        if (!OwnerSkills->IsReady()) State=NSLOCTEXT("Mythic","AbilityHudLoading","Loading");
        else if (!OwnerSkills->IsSlotUnlocked(Index)) State=NSLOCTEXT("Mythic","AbilityHudLocked","Locked");
        else if (!Cell.Definition) State=NSLOCTEXT("Mythic","AbilityHudEmpty","Empty");
        else if (Active) State=NSLOCTEXT("Mythic","AbilityHudActive","Active");
        else if (Remaining<=0 && !Ready) State=NSLOCTEXT("Mythic","AbilityHudUnavailable","Blocked");
        Cell.State->SetText(State); Cell.State->SetColorAndOpacity(FSlateColor(Active ? FMythicUIStyle::Get().AccentBright : FMythicUIStyle::Get().Ink));
        Cell.Icon->SetColorAndOpacity(Active || Ready ? FLinearColor::White : FLinearColor(0.38f,0.38f,0.38f,1));
        Cell.Sweep->SetVisibility(Remaining>0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed); Cell.Sweep->ForceVolatile(Remaining>0);
    }
    AdvanceCooldowns();
    if (Timed && !GetWorld()->GetTimerManager().IsTimerActive(CooldownTimer)) GetWorld()->GetTimerManager().SetTimer(CooldownTimer,this,&ThisClass::AdvanceCooldowns,0.1f,true);
    else if (!Timed) GetWorld()->GetTimerManager().ClearTimer(CooldownTimer);
}

void UMythicAbilityHudWidget::AdvanceCooldowns() {
    if (!bConstructed || !GetWorld()) return;
    const double Now=GetWorld()->GetTimeSeconds(); bool Timed=false,Expired=false;
    for (auto& Cell:Cells) {
        const float Remaining=FMath::Max(0.0,Cell.CooldownEnds-Now); Timed|=Remaining>0;
        if (Cell.CooldownEnds>0 && Remaining<=0) { Cell.CooldownEnds=0; Expired=true; Cell.Sweep->SetVisibility(ESlateVisibility::Collapsed); Cell.Sweep->ForceVolatile(false); }
        const int32 Seconds=Remaining>0 ? FMath::Max(1,FMath::CeilToInt(Remaining)) : 0;
        if (Cell.ShownSeconds!=Seconds) { Cell.ShownSeconds=Seconds; Cell.Countdown->SetText(Seconds>0 ? FText::AsNumber(Seconds) : FText::GetEmpty()); Cell.Countdown->SetVisibility(Seconds>0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed); }
        if (Cell.SweepMaterial && Remaining>0 && Cell.CooldownDuration>0) Cell.SweepMaterial->SetScalarParameterValue(TEXT("Progress"),1-FMath::Clamp(Remaining/Cell.CooldownDuration,0.0f,1.0f));
    }
    if (!Timed) GetWorld()->GetTimerManager().ClearTimer(CooldownTimer);
    if (Expired) QueueRefresh(); // Re-query authority once at the expiry edge, never from every cosmetic beat.
}
