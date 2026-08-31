

#include "MythicHUDLayout.h"

#include "CommonUIExtensions.h"
#include "PrimaryGameLayout.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Components/NamedSlot.h"
#include "Components/PanelWidget.h"
#include "Input/CommonUIInputTypes.h"
#include "Input/UIActionBinding.h"
#include "NativeGameplayTags.h"
#include "Interaction/ContextActions/MythicContextActionProjectionPolicy.h"
#include "UI/Menu/MythicMenuShell.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"
#include "UObject/UObjectIterator.h"
#include "GameFramework/PlayerState.h"
#include "GAS/MythicTags_GAS.h"
#include "TimerManager.h"
#include "UI/Settings/MythicUserSettings.h"
#include "UI/Nameplate/MythicNameplateLayer.h"
#include "UI/Nameplate/MythicNameplateDirector.h"
#include "UI/Nameplate/MythicNameplatePolicy.h"
#include "UI/MythicTags_UI.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_LAYER_MENU, "UI.Layer.Menu");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_ACTION_ESCAPE, "UI.Action.Escape");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_ACTION_INVENTORY, "UI.Action.Inventory");

namespace {
bool ConfigureAuthoredCommonUIHold(const FUIActionBindingHandle Handle,
                                   const float HoldDurationSeconds) {
    if (!FMythicContextActionProjectionRules::IsHoldDurationValid(
            HoldDurationSeconds)
        || HoldDurationSeconds <= 0.0f) {
        return false;
    }

    const TSharedPtr<FUIActionBinding> Binding =
        FUIActionBinding::FindBinding(Handle);
    if (!Binding) {
        return false;
    }

    for (FUIActionKeyMapping &Mapping : Binding->HoldMappings) {
        Mapping.HoldTime = HoldDurationSeconds;
        Mapping.HoldRollbackTime = 0.0f;
    }
    for (const FUIActionKeyMapping &NormalMapping : Binding->NormalMappings) {
        FUIActionKeyMapping HeldMapping = NormalMapping;
        HeldMapping.HoldTime = HoldDurationSeconds;
        HeldMapping.HoldRollbackTime = 0.0f;
        if (FUIActionKeyMapping *Existing =
                Binding->HoldMappings.FindByPredicate(
                    [&HeldMapping](const FUIActionKeyMapping &Candidate) {
                        return Candidate.Key == HeldMapping.Key;
                    })) {
            *Existing = HeldMapping;
        } else {
            Binding->HoldMappings.Add(MoveTemp(HeldMapping));
        }
    }
    Binding->NormalMappings.Reset();
    return !Binding->HoldMappings.IsEmpty();
}
}


UMythicHUDLayout::UMythicHUDLayout(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer) {}

void UMythicHUDLayout::NativeOnInitialized() {
    Super::NativeOnInitialized();

    RegisterUIActionBinding(FBindUIActionArgs(FUIActionTag::ConvertChecked(TAG_UI_ACTION_ESCAPE), false,
                                              FSimpleDelegate::CreateUObject(this, &ThisClass::HandleEscapeAction)));
    FBindUIActionArgs InspectBindArgs(
        FUIActionTag::ConvertChecked(UI_ACTION_INSPECT_ENTITY), false,
        FSimpleDelegate::CreateUObject(
            this, &ThisClass::HandleInspectEntityAction));
    InspectBindArgs.bForceHold = true;
    InspectBindArgs.InputMode = ECommonInputMode::Game;
    InspectBindArgs.bConsumeInput = true;
    InspectActionBinding = RegisterUIActionBinding(InspectBindArgs);

    if (bRouteInventoryToShell) {
        RegisterUIActionBinding(FBindUIActionArgs(FUIActionTag::ConvertChecked(TAG_UI_ACTION_INVENTORY), false,
                                                  FSimpleDelegate::CreateUObject(this, &ThisClass::HandleInventoryAction)));
    }

    if (OpenMenuAction.IsValid()) {
        RegisterUIActionBinding(FBindUIActionArgs(FUIActionTag::ConvertChecked(OpenMenuAction), false,
                                                  FSimpleDelegate::CreateWeakLambda(this, [this]() {
                                                      OpenMenuOnPage(NAME_None);
                                                  })));
    }

    for (const FMythicMenuHotkey &Hotkey : MenuHotkeys) {
        if (!Hotkey.ActionTag.IsValid() || Hotkey.PageId.IsNone()) {
            continue;
        }
        const FName PageId = Hotkey.PageId;
        RegisterUIActionBinding(FBindUIActionArgs(FUIActionTag::ConvertChecked(Hotkey.ActionTag), false,
                                                  FSimpleDelegate::CreateWeakLambda(this, [this, PageId]() {
                                                      OpenMenuOnPage(PageId);
                                                  })));
    }

    if (WorldOverlaySlot && NameplateLayerClass && !NameplateLayer) {
        NameplateLayer = CreateWidget<UMythicNameplateLayer>(GetOwningPlayer(), NameplateLayerClass);
        if (NameplateLayer) {
            WorldOverlaySlot->AddChild(NameplateLayer);
        }
    }

    if (ULocalPlayer *LocalPlayer = GetOwningLocalPlayer()) {
        if (UMythicNameplateDirector *Director =
                LocalPlayer->GetSubsystem<UMythicNameplateDirector>()) {
            BoundNameplateDirector = Director;
            Director->OnNameplateProjectionsChanged.AddDynamic(
                this, &ThisClass::HandleNameplateProjectionsChanged);
            RefreshContextActionBindings();
        }
    }

    const UMythicNameplatePolicy *NameplatePolicy = NameplateLayer
        ? NameplateLayer->GetNameplatePolicy() : nullptr;
    const float InspectHoldSeconds = NameplatePolicy
        ? NameplatePolicy->Inspect.HoldDurationSeconds : 0.55f;
    if (InspectActionBinding.IsValid()
        && !ConfigureAuthoredCommonUIHold(
            InspectActionBinding, InspectHoldSeconds)) {
        InspectActionBinding.Unregister();
        InspectActionBinding = FUIActionBindingHandle();
        UE_LOG(LogTemp, Error,
               TEXT("MythicHUDLayout: Inspect requires a %.2fs hold, but CommonUI supplied no usable keyboard/controller mapping."),
               InspectHoldSeconds);
    }

    if (RevealHUDAction.IsValid()) {
        RegisterUIActionBinding(FBindUIActionArgs(FUIActionTag::ConvertChecked(RevealHUDAction), false,
                                                  FSimpleDelegate::CreateUObject(this, &ThisClass::HandleRevealHUD)));
    }

    BindContextualElements();
    BindCombatState();

    if (UMythicUserSettings *Settings = UMythicUserSettings::Get()) {
        AccessibilityHandle = Settings->OnAccessibilityChanged.AddUObject(this, &UMythicHUDLayout::HandleAccessibilityChanged);
    }
}

void UMythicHUDLayout::HandleAccessibilityChanged() {
    SetSalienceTicking(true);
}

void UMythicHUDLayout::NativeDestruct() {
    ClearContextActionBindings();
    InspectActionBinding.Unregister();
    InspectActionBinding = FUIActionBindingHandle();
    if (UMythicNameplateDirector *Director = BoundNameplateDirector.Get()) {
        Director->OnNameplateProjectionsChanged.RemoveDynamic(
            this, &ThisClass::HandleNameplateProjectionsChanged);
    }
    BoundNameplateDirector.Reset();
    if (const UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(SalienceTimer);
        World->GetTimerManager().ClearTimer(CombatBindTimer);
    }
    if (UMythicUserSettings *Settings = UMythicUserSettings::Get(); Settings && AccessibilityHandle.IsValid()) {
        Settings->OnAccessibilityChanged.Remove(AccessibilityHandle);
        AccessibilityHandle.Reset();
    }
    if (const APlayerController *PC = GetOwningPlayer()) {
        if (APlayerState *PS = PC->PlayerState) {
            if (UAbilitySystemComponent *ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PS)) {
                ASC->RegisterGameplayTagEvent(GAS_STATE_INCOMBAT, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
            }
        }
    }
    NameplateLayer = nullptr;
    Super::NativeDestruct();
}

void UMythicHUDLayout::BindContextualElements() {
    for (const FMythicHUDElementBinding &Binding : ContextualElements) {
        if (Binding.WidgetName.IsNone()) {
            continue;
        }
        UWidget *Widget = GetWidgetFromName(Binding.WidgetName);
        if (!Widget) {
            UE_LOG(LogTemp, Warning, TEXT("MythicHUDLayout: contextual element '%s' not found in %s"),
                   *Binding.WidgetName.ToString(), *GetName());
            continue;
        }
        RegisterHUDElement(Widget, Binding.Rule.Resting);
        if (FMythicHUDElementState *State = FindElementState(Widget)) {
            State->bHasRule = true;
            State->Rule = Binding.Rule;
        }
    }
    ApplyRules();
}

void UMythicHUDLayout::BindCombatState() {
    constexpr int32 MaxCombatBindAttempts = 20;

    const APlayerController *PC = GetOwningPlayer();
    UAbilitySystemComponent *ASC = nullptr;
    if (PC && PC->PlayerState) {
        ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PC->PlayerState);
    }

    if (!ASC) {
        if (UWorld *World = GetWorld(); World && ++CombatBindAttempts <= MaxCombatBindAttempts) {
            World->GetTimerManager().SetTimer(CombatBindTimer, FTimerDelegate::CreateWeakLambda(this, [this]() {
                BindCombatState();
            }), 0.25f, false);
        }
        return;
    }

    bInCombat = ASC->HasMatchingGameplayTag(GAS_STATE_INCOMBAT);
    ASC->RegisterGameplayTagEvent(GAS_STATE_INCOMBAT, EGameplayTagEventType::NewOrRemoved)
       .AddUObject(this, &UMythicHUDLayout::HandleCombatTagChanged);
    ApplyRules();
}

void UMythicHUDLayout::HandleCombatTagChanged(const FGameplayTag Tag, int32 NewCount) {
    const bool bNow = NewCount > 0;
    if (bNow == bInCombat) {
        return;
    }
    bInCombat = bNow;
    ApplyRules();
}

bool UMythicHUDLayout::ApplyRules() {
    const UWorld *World = GetWorld();
    const double Now = World ? World->GetTimeSeconds() : 0.0;
    bool bAnyActive = false;

    for (FMythicHUDElementState &State : HUDElements) {
        if (!State.bHasRule || !State.Widget.IsValid()) {
            continue;
        }
        EMythicHUDSalience Want;
        if (Now < State.ActivityUntil) {
            Want = EMythicHUDSalience::Lit;
            bAnyActive = true;
        }
        else {
            Want = bInCombat ? State.Rule.InCombat : State.Rule.Resting;
        }
        if (State.Want != Want) {
            State.Want = Want;
            SetSalienceTicking(true);
        }
    }
    return bAnyActive;
}

void UMythicHUDLayout::PokeElement(UWidget *Element) {
    FMythicHUDElementState *State = FindElementState(Element);
    if (!State || !State->bHasRule || State->Rule.ActivityHoldSeconds <= 0.0f) {
        return;
    }
    const UWorld *World = GetWorld();
    State->ActivityUntil = (World ? World->GetTimeSeconds() : 0.0) + State->Rule.ActivityHoldSeconds;
    ApplyRules();
    SetSalienceTicking(true);
}

void UMythicHUDLayout::PokeElementByName(FName WidgetName) {
    PokeElement(GetWidgetFromName(WidgetName));
}

UMythicHUDLayout::FMythicHUDElementState *UMythicHUDLayout::FindElementState(const UWidget *Element) {
    if (!Element) {
        return nullptr;
    }
    for (FMythicHUDElementState &State : HUDElements) {
        if (State.Widget.Get() == Element) {
            return &State;
        }
    }
    return nullptr;
}


namespace {
    float SalienceTargetAlpha(EMythicHUDSalience Want, bool bRevealAll) {
        return (!bRevealAll && Want == EMythicHUDSalience::Hidden) ? 0.0f : 1.0f;
    }

    float SalienceTargetTint(EMythicHUDSalience Want, bool bRevealAll, float DimTint) {
        return (bRevealAll || Want == EMythicHUDSalience::Lit) ? 1.0f : FMath::Clamp(DimTint, 0.0f, 1.0f);
    }

    float GetWidgetTint(const UWidget *Widget) {
        const UUserWidget *AsUser = Cast<UUserWidget>(Widget);
        return AsUser ? AsUser->GetColorAndOpacity().R : 1.0f;
    }

    bool SetWidgetTint(UWidget *Widget, float Tint) {
        if (UUserWidget *AsUser = Cast<UUserWidget>(Widget)) {
            AsUser->SetColorAndOpacity(FLinearColor(Tint, Tint, Tint, 1.0f));
            return true;
        }
        return false;
    }
}

bool UMythicHUDLayout::ShouldRevealEverything() const {
    const UMythicUserSettings *Settings = UMythicUserSettings::Get();
    return bHUDRevealed || (Settings && Settings->GetAlwaysShowHUD());
}

float UMythicHUDLayout::AccessibilityHUDOpacity() {
    const UMythicUserSettings *UserSettings = UMythicUserSettings::Get();
    return UserSettings ? UserSettings->GetHUDOpacity() : 1.0f;
}

float UMythicHUDLayout::TargetOpacityFor(EMythicHUDSalience Want) const {
    return SalienceTargetAlpha(Want, ShouldRevealEverything()) * AccessibilityHUDOpacity();
}

void UMythicHUDLayout::RegisterHUDElement(UWidget *Element, EMythicHUDSalience InitialSalience) {
    if (!Element) {
        return;
    }
    for (FMythicHUDElementState &State : HUDElements) {
        if (State.Widget.Get() == Element) {
            State.Want = InitialSalience;
            SetSalienceTicking(true);
            return;
        }
    }
    const bool bRevealAll = ShouldRevealEverything();
    const float StartAlpha = SalienceTargetAlpha(InitialSalience, bRevealAll) * AccessibilityHUDOpacity();
    Element->SetRenderOpacity(StartAlpha);
    SetWidgetTint(Element, SalienceTargetTint(InitialSalience, bRevealAll, DimOpacity));
    Element->SetVisibility(StartAlpha > 0.0f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    HUDElements.Add({Element, InitialSalience});
}

void UMythicHUDLayout::UnregisterHUDElement(UWidget *Element) {
    HUDElements.RemoveAll([Element](const FMythicHUDElementState &State) {
        return !State.Widget.IsValid() || State.Widget.Get() == Element;
    });
    if (HUDElements.Num() == 0) {
        SetSalienceTicking(false);
    }
}

void UMythicHUDLayout::SetElementSalience(UWidget *Element, EMythicHUDSalience Salience) {
    for (FMythicHUDElementState &State : HUDElements) {
        if (State.Widget.Get() == Element) {
            if (State.Want != Salience) {
                State.Want = Salience;
                SetSalienceTicking(true);
            }
            return;
        }
    }
    RegisterHUDElement(Element, Salience);
}

void UMythicHUDLayout::SetElementDimTint(UWidget *Element, float DimTint) {
    for (FMythicHUDElementState &State : HUDElements) {
        if (State.Widget.Get() == Element) {
            State.DimTintOverride = FMath::Clamp(DimTint, 0.0f, 1.0f);
            SetWidgetTint(Element, SalienceTargetTint(State.Want, ShouldRevealEverything(), State.DimTintOverride));
            return;
        }
    }
}

EMythicHUDSalience UMythicHUDLayout::GetElementSalience(const UWidget *Element) const {
    for (const FMythicHUDElementState &State : HUDElements) {
        if (State.Widget.Get() == Element) {
            return State.Want;
        }
    }
    return EMythicHUDSalience::Hidden;
}

void UMythicHUDLayout::HandleRevealHUD() {
    bHUDRevealed = !bHUDRevealed;
    SetSalienceTicking(true);
    OnHUDRevealChanged.Broadcast(bHUDRevealed);
}

void UMythicHUDLayout::SetSalienceTicking(bool bEnabled) {
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    constexpr float Interval = 1.0f / 30.0f;
    if (bEnabled) {
        if (!World->GetTimerManager().IsTimerActive(SalienceTimer)) {
            World->GetTimerManager().SetTimer(SalienceTimer, FTimerDelegate::CreateWeakLambda(this, [this]() {
                TickSalience(1.0f / 30.0f);
            }), Interval, true);
        }
    }
    else {
        World->GetTimerManager().ClearTimer(SalienceTimer);
    }
}

void UMythicHUDLayout::TickSalience(float DeltaSeconds) {
    bool bAnyMoving = false;
    const bool bRevealAll = ShouldRevealEverything();

    for (int32 Index = HUDElements.Num() - 1; Index >= 0; --Index) {
        UWidget *Widget = HUDElements[Index].Widget.Get();
        if (!Widget) {
            HUDElements.RemoveAtSwap(Index);
            continue;
        }

        const EMythicHUDSalience Want = HUDElements[Index].Want;
        const float TargetAlpha = SalienceTargetAlpha(Want, bRevealAll) * AccessibilityHUDOpacity();
        const float DimTint = HUDElements[Index].DimTintOverride >= 0.0f ? HUDElements[Index].DimTintOverride : DimOpacity;
        const float TargetTint = SalienceTargetTint(Want, bRevealAll, DimTint);

        const float CurrentAlpha = Widget->GetRenderOpacity();
        const float CurrentTint = GetWidgetTint(Widget);
        const bool bTintable = Cast<UUserWidget>(Widget) != nullptr;

        const bool bAlphaSettled = FMath::IsNearlyEqual(CurrentAlpha, TargetAlpha, 0.004f);
        const bool bTintSettled = !bTintable || FMath::IsNearlyEqual(CurrentTint, TargetTint, 0.004f);

        if (bAlphaSettled && bTintSettled) {
            if (CurrentAlpha != TargetAlpha) {
                Widget->SetRenderOpacity(TargetAlpha);
                Widget->SetVisibility(TargetAlpha > 0.0f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
            }
            if (bTintable && CurrentTint != TargetTint) {
                SetWidgetTint(Widget, TargetTint);
            }
            continue;
        }

        if (!bAlphaSettled) {
            const float Speed = TargetAlpha > CurrentAlpha ? SalienceFadeInSpeed : SalienceFadeOutSpeed;
            Widget->SetRenderOpacity(FMath::FInterpConstantTo(CurrentAlpha, TargetAlpha, DeltaSeconds, Speed));
        }
        if (!bTintSettled) {
            const float Speed = TargetTint > CurrentTint ? SalienceFadeInSpeed : SalienceFadeOutSpeed;
            SetWidgetTint(Widget, FMath::FInterpConstantTo(CurrentTint, TargetTint, DeltaSeconds, Speed));
        }
        Widget->SetVisibility(bAlphaSettled && TargetAlpha <= 0.0f
                                  ? ESlateVisibility::Collapsed
                                  : ESlateVisibility::HitTestInvisible);
        bAnyMoving = true;
    }

    const bool bWindowOpen = ApplyRules();

    if (!bAnyMoving && !bWindowOpen) {
        SetSalienceTicking(false);
    }
}

void UMythicHUDLayout::HandleInventoryAction() {
    OpenMenuOnPage(InventoryPageId);
}

void UMythicHUDLayout::HandleInspectEntityAction() {
    if (ULocalPlayer *LocalPlayer = GetOwningLocalPlayer()) {
        if (UMythicNameplateDirector *Director =
                LocalPlayer->GetSubsystem<UMythicNameplateDirector>()) {
            Director->OpenFocusedEntityInspect();
        }
    }
}

void UMythicHUDLayout::HandleNameplateProjectionsChanged(
    const int32 LocalRevision) {
    (void)LocalRevision;
    RefreshContextActionBindings();
}

void UMythicHUDLayout::ClearContextActionBindings() {
    TArray<FGameplayTag> InputTags;
    ContextActionBindings.GenerateKeyArray(InputTags);
    for (const FGameplayTag InputTag : InputTags) {
        RemoveContextActionBinding(InputTag);
    }
    ContextActionBindings.Reset();
}

void UMythicHUDLayout::RemoveContextActionBinding(
    const FGameplayTag InputActionTag) {
    FContextActionBindingRecord *Record =
        ContextActionBindings.Find(InputActionTag);
    if (!Record) {
        return;
    }
    if (Record->HoldDurationSeconds > 0.0f) {
        if (UMythicNameplateDirector *Director =
                BoundNameplateDirector.Get()) {
            Director->CancelFocusedContextActionHold(Record->ActionTag);
        }
    }
    Record->Handle.Unregister();
    ContextActionBindings.Remove(InputActionTag);
}

void UMythicHUDLayout::RefreshContextActionBindings() {
    UMythicNameplateDirector *Director = BoundNameplateDirector.Get();
    if (!Director) {
        ClearContextActionBindings();
        return;
    }

    FMythicNameplateActionRailProjection Projection;
    if (!Director->GetFocusedActionRailProjection(Projection)
        || !Projection.Instance.IsValid()) {
        ClearContextActionBindings();
        return;
    }

    TSet<FGameplayTag> DesiredInputTags;
    DesiredInputTags.Reserve(Projection.Actions.Num());
    for (const FMythicNameplateActionProjection &Action : Projection.Actions) {
        if (!Action.ActionTag.IsValid()
            || !Action.InputActionTag.IsValid()
            || DesiredInputTags.Contains(Action.InputActionTag)
            || !FMythicContextActionProjectionRules::IsHoldDurationValid(
                Action.HoldDurationSeconds)) {
            continue;
        }
        DesiredInputTags.Add(Action.InputActionTag);

        if (const FContextActionBindingRecord *Existing =
                ContextActionBindings.Find(Action.InputActionTag);
            Existing && Existing->Matches(
                Projection.Instance, Action.ActionTag,
                Action.OfferRevision, Action.HoldDurationSeconds)) {
            continue;
        }
        RemoveContextActionBinding(Action.InputActionTag);

        const FGameplayTag ContextActionTag = Action.ActionTag;
        FBindUIActionArgs BindArgs(
            FUIActionTag::ConvertChecked(Action.InputActionTag), false,
            FSimpleDelegate::CreateWeakLambda(
                this, [this, ContextActionTag]() {
                    if (UMythicNameplateDirector *CurrentDirector =
                            BoundNameplateDirector.Get()) {
                        CurrentDirector->ExecuteFocusedContextAction(
                            ContextActionTag);
                    }
                }));
        BindArgs.OverrideDisplayName = Action.ResolvedLabel;
        // World-context input belongs only to active gameplay. Consuming it prevents the same F/A press from reaching
        // attack or another Enhanced Input agent after this LocalPlayer-owned action has accepted it.
        BindArgs.InputMode = ECommonInputMode::Game;
        BindArgs.bConsumeInput = true;
        const bool bRequiresHold = Action.HoldDurationSeconds > 0.0f;
        if (bRequiresHold) {
            BindArgs.bForceHold = true;
            BindArgs.OnHoldActionPressed.BindWeakLambda(
                this, [this, ContextActionTag]() {
                    if (UMythicNameplateDirector *CurrentDirector =
                            BoundNameplateDirector.Get()) {
                        CurrentDirector->BeginFocusedContextActionHold(
                            ContextActionTag);
                    }
                });
            BindArgs.OnHoldActionReleased.BindWeakLambda(
                this, [this, ContextActionTag]() {
                    if (UMythicNameplateDirector *CurrentDirector =
                            BoundNameplateDirector.Get()) {
                        CurrentDirector->CancelFocusedContextActionHold(
                            ContextActionTag);
                    }
                });
        }
        FUIActionBindingHandle Handle = RegisterUIActionBinding(BindArgs);
        if (!Handle.IsValid()) {
            continue;
        }
        if (bRequiresHold
            && !ConfigureAuthoredCommonUIHold(
                Handle, Action.HoldDurationSeconds)) {
            Handle.Unregister();
            UE_LOG(LogTemp, Error,
                   TEXT("MythicHUDLayout: context action '%s' requires a %.2fs hold, but CommonUI supplied no usable keyboard/controller mapping."),
                   *Action.ActionTag.ToString(),
                   Action.HoldDurationSeconds);
            continue;
        }

        FContextActionBindingRecord &Record =
            ContextActionBindings.Add(Action.InputActionTag);
        Record.Handle = Handle;
        Record.Subject = Projection.Instance;
        Record.ActionTag = Action.ActionTag;
        Record.InputActionTag = Action.InputActionTag;
        Record.OfferRevision = Action.OfferRevision;
        Record.HoldDurationSeconds = Action.HoldDurationSeconds;
    }

    TArray<FGameplayTag> ExistingInputTags;
    ContextActionBindings.GenerateKeyArray(ExistingInputTags);
    for (const FGameplayTag ExistingInputTag : ExistingInputTags) {
        if (!DesiredInputTags.Contains(ExistingInputTag)) {
            RemoveContextActionBinding(ExistingInputTag);
        }
    }
}

void UMythicHUDLayout::OpenMenuOnPage(FName PageId) {
    if (!MenuShellClass) {
        return;
    }

    if (UMythicMenuShell *Existing = ActiveMenuShell.Get()) {
        if (Existing->IsActivated()) {
            Existing->OpenPage(PageId);
            return;
        }
    }

    UCommonActivatableWidget *Pushed =
        UCommonUIExtensions::PushContentToLayer_ForPlayer(GetOwningLocalPlayer(), TAG_UI_LAYER_MENU, MenuShellClass);
    if (UMythicMenuShell *Shell = Cast<UMythicMenuShell>(Pushed)) {
        ActiveMenuShell = Shell;
        Shell->OpenPage(PageId);
    }
}

UWidget *UMythicHUDLayout::BorrowInventoryWidget() {
    if (!InventorySlot || InventorySlot->GetChildrenCount() == 0) {
        return nullptr;
    }

    UWidget *Inventory = InventorySlot->GetChildAt(0);
    if (!Inventory) {
        return nullptr;
    }

    InventorySlot->RemoveChild(Inventory);
    InventorySlot->SetVisibility(ESlateVisibility::Collapsed);
    ClearBlueprintInventoryOpenFlag();
    return Inventory;
}

void UMythicHUDLayout::ReturnInventoryWidget(UWidget *Widget) {
    if (!Widget || !InventorySlot) {
        return;
    }
    if (Widget->GetParent() == InventorySlot) {
        return;
    }
    if (UPanelWidget *Current = Widget->GetParent()) {
        Current->RemoveChild(Widget);
    }
    InventorySlot->AddChild(Widget);
    InventorySlot->SetVisibility(ESlateVisibility::Collapsed);
    ClearBlueprintInventoryOpenFlag();
}

void UMythicHUDLayout::ClearBlueprintInventoryOpenFlag() {
    static const FName ShowName(TEXT("Show"));
    if (FBoolProperty *Flag = FindFProperty<FBoolProperty>(GetClass(), ShowName)) {
        Flag->SetPropertyValue_InContainer(this, false);
    }
}

void UMythicHUDLayout::HandleEscapeAction() {
    if (APlayerController *PC = GetOwningPlayer()) {
        if (UPrimaryGameLayout *Layout = UPrimaryGameLayout::GetPrimaryGameLayout(PC)) {
            if (UCommonActivatableWidgetContainerBase *MenuLayer = Layout->GetLayerWidget(TAG_UI_LAYER_MENU)) {
                if (UCommonActivatableWidget *Top = MenuLayer->GetActiveWidget()) {
                    Top->DeactivateWidget();
                    return;
                }
            }
        }
    }

    if (ensure(!EscapeMenuClass.IsNull()) && !PreEscapeMenuOpen()) {
        UCommonUIExtensions::PushStreamedContentToLayer_ForPlayer(GetOwningLocalPlayer(), TAG_UI_LAYER_MENU, EscapeMenuClass);
    }
}

static void MythicOpenMenu(const TArray<FString> &Args, UWorld *World) {
    if (!World) {
        UE_LOG(LogTemp, Warning, TEXT("Mythic.OpenMenu: no world"));
        return;
    }

    const APlayerController *PC = World->GetFirstPlayerController();
    if (!PC) {
        UE_LOG(LogTemp, Warning, TEXT("Mythic.OpenMenu: no player controller - is PIE running?"));
        return;
    }

    for (TObjectIterator<UMythicHUDLayout> It; It; ++It) {
        UMythicHUDLayout *Layout = *It;
        if (!IsValid(Layout) || Layout->HasAnyFlags(RF_ClassDefaultObject) || Layout->GetOwningPlayer() != PC) {
            continue;
        }
        const FString Arg = Args.Num() > 0 ? Args[0] : FString(TEXT("Character"));
        if (Arg.Equals(TEXT("Escape"), ESearchCase::IgnoreCase)) {
            UE_LOG(LogTemp, Log, TEXT("Mythic.OpenMenu: opening the escape menu"));
            Layout->HandleEscapeAction();
            return;
        }
        UE_LOG(LogTemp, Log, TEXT("Mythic.OpenMenu: opening page '%s'"), *Arg);
        Layout->OpenMenuOnPage(FName(*Arg));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Mythic.OpenMenu: no HUD layout for the local player"));
}

static FAutoConsoleCommandWithWorldAndArgs GMythicOpenMenuCmd(
    TEXT("Mythic.OpenMenu"),
    TEXT("Open the menu shell on a page id (default Character). The menu is a widget, so no aiming or key "
         "injection is involved - this is how a UI change gets verified in one step."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&MythicOpenMenu));
