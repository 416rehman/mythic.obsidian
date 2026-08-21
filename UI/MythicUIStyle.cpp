// Copyright Stellar Games. All Rights Reserved.

#include "MythicUIStyle.h"

#include "CommonButtonBase.h"
#include "CommonTextBlock.h"
#include "Components/Button.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Materials/MaterialInterface.h"

const UMythicUIStyleSettings &FMythicUIStyle::Get() {
    const UMythicUIStyleSettings *Settings = GetDefault<UMythicUIStyleSettings>();
    check(Settings);
    return *Settings;
}

FSlateBrush FMythicUIStyle::MakeSlotBrush(bool bLocked) {
    static const FSoftObjectPath SlotPath(
        TEXT("/Game/Mythic/UI/Globals/materials/kit/MI_UI_Plate_Slot.MI_UI_Plate_Slot"));
    static const FSoftObjectPath LockedPath(
        TEXT("/Game/Mythic/UI/Globals/materials/kit/MI_UI_Plate_SlotLocked.MI_UI_Plate_SlotLocked"));

    FSlateBrush Brush;

    if (UMaterialInterface *Plate = Cast<UMaterialInterface>((bLocked ? LockedPath : SlotPath).TryLoad())) {
        Brush.SetResourceObject(Plate);
        Brush.DrawAs = ESlateBrushDrawType::Image;
        Brush.TintColor = FSlateColor(FLinearColor::White);
        return Brush;
    }

    const UMythicUIStyleSettings &S = Get();
    Brush.DrawAs = ESlateBrushDrawType::Box;
    Brush.TintColor = FSlateColor(S.SlotFill);
    Brush.Margin = FMargin(0.0f);
    Brush.OutlineSettings.CornerRadii = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
    Brush.OutlineSettings.Color = FSlateColor(bLocked ? S.SlotEdgeLocked : S.SlotEdge);
    Brush.OutlineSettings.Width = S.SlotEdgeWidth;
    Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
    return Brush;
}

void FMythicUIStyle::ApplyTextStyle(UCommonTextBlock *Text, EMythicTextRole Role) {
    if (!Text) {
        return;
    }
    const UMythicUIStyleSettings &S = Get();

    TSoftClassPtr<UCommonTextStyle> Wanted;
    switch (Role) {
        case EMythicTextRole::Title:
            Wanted = S.TitleStyle;
            break;
        case EMythicTextRole::Heading:
            Wanted = S.HeadingStyle;
            break;
        case EMythicTextRole::Subtle:
            Wanted = S.SubtleStyle;
            break;
        case EMythicTextRole::Body:
        default:
            Wanted = S.BodyStyle;
            break;
    }

    if (UClass *StyleClass = Wanted.IsNull() ? nullptr : Wanted.LoadSynchronous()) {
        Text->SetStyle(StyleClass);
    }
}

UObject *FMythicUIStyle::WidgetOuterFor(UWidget *Owner) {
    if (!Owner) {
        return nullptr;
    }
    UUserWidget *UserWidget = Cast<UUserWidget>(Owner);
    if (!UserWidget) {
        UserWidget = Owner->GetTypedOuter<UUserWidget>();
    }
    if (UserWidget && UserWidget->WidgetTree) {
        return UserWidget->WidgetTree;
    }
    return Owner;
}

UCommonTextBlock *FMythicUIStyle::MakeText(UWidget *Owner, EMythicTextRole Role) {
    if (!Owner) {
        return nullptr;
    }
    UCommonTextBlock *Text = NewObject<UCommonTextBlock>(WidgetOuterFor(Owner));
    ApplyTextStyle(Text, Role);
    return Text;
}

UWidget *FMythicUIStyle::MakeButton(UWidget *Owner, EMythicTextRole LabelRole, UCommonTextBlock *&OutLabel,
                                   UCommonTextBlock **OutValue) {
    OutLabel = nullptr;
    if (OutValue) {
        *OutValue = nullptr;
    }
    if (!Owner) {
        return nullptr;
    }
    const UMythicUIStyleSettings &S = Get();

    if (!S.MenuButtonClass.IsNull()) {
        if (UClass *ButtonClass = S.MenuButtonClass.LoadSynchronous()) {
            if (UCommonButtonBase *Button = NewObject<UCommonButtonBase>(WidgetOuterFor(Owner), ButtonClass)) {
                Button->Initialize();
                if (UCommonTextBlock *Label = Cast<UCommonTextBlock>(Button->GetWidgetFromName(S.MenuButtonLabelName))) {
                    OutLabel = Label;
                }
                if (OutValue) {
                    if (UCommonTextBlock *Value = Cast<UCommonTextBlock>(Button->GetWidgetFromName(S.MenuButtonValueName))) {
                        Value->SetVisibility(ESlateVisibility::Collapsed);
                        *OutValue = Value;
                        if (OutLabel) {
                            OutLabel->SetJustification(ETextJustify::Left);
                        }
                    }
                }
                if (OutLabel) {
                    WireFocusRing(Button);
                    return Button;
                }
            }
        }
    }

    UButton *Plain = NewObject<UButton>(WidgetOuterFor(Owner));
    OutLabel = MakeText(Owner, LabelRole);
    Plain->AddChild(OutLabel);
    return Plain;
}

void FMythicUIStyle::SetOptionalText(UCommonTextBlock *Text, const FText &Value, FLinearColor Colour) {
    if (!Text) {
        return;
    }
    if (Value.IsEmpty()) {
        Text->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }
    Text->SetText(Value);
    Text->SetColorAndOpacity(FSlateColor(Colour));
    Text->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void FMythicUIStyle::ShowEmptyState(UUserWidget *Page, FName BlockName, bool bEmpty) {
    if (!Page || BlockName.IsNone()) {
        return;
    }
    if (UWidget *Block = Page->GetWidgetFromName(BlockName)) {
        Block->SetVisibility(bEmpty ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
}

void FMythicUIStyle::WireFocusRing(UCommonButtonBase *Button) {
    if (!Button || Button->OnFocusReceived().IsBound()) {
        return;
    }
    UWidget *Ring = Button->GetWidgetFromName(TEXT("FocusRing"));
    if (!Ring) {
        return;
    }
    TWeakObjectPtr<UWidget> WeakRing(Ring);
    Button->OnFocusReceived().AddWeakLambda(Ring, [WeakRing]() {
        if (UWidget *R = WeakRing.Get()) {
            R->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
    });
    Button->OnFocusLost().AddWeakLambda(Ring, [WeakRing]() {
        if (UWidget *R = WeakRing.Get()) {
            R->SetVisibility(ESlateVisibility::Collapsed);
        }
    });
}

void FMythicUIStyle::WireFocusRings(UUserWidget *Root) {
    if (!Root || !Root->WidgetTree) {
        return;
    }
    Root->WidgetTree->ForEachWidget([](UWidget *W) {
        if (UCommonButtonBase *Button = Cast<UCommonButtonBase>(W)) {
            WireFocusRing(Button);
        }
    });
}

static bool UIStyle_IsShown(UWidget *W, UUserWidget *Root) {
    for (UWidget *Cur = W; Cur && Cur != Root;) {
        const ESlateVisibility Vis = Cur->GetVisibility();
        if (Vis == ESlateVisibility::Collapsed || Vis == ESlateVisibility::Hidden) {
            return false;
        }
        if (UWidget *Parent = Cur->GetParent()) {
            Cur = Parent;
        }
        else {
            UUserWidget *Owner = Cur->GetTypedOuter<UUserWidget>();
            Cur = (Owner && Owner != Cur) ? static_cast<UWidget *>(Owner) : nullptr;
        }
    }
    return true;
}

UWidget *FMythicUIStyle::FindFirstFocusable(UUserWidget *Root) {
    if (!Root || !Root->WidgetTree) {
        return nullptr;
    }
    UWidget *Found = nullptr;
    Root->WidgetTree->ForEachWidget([&Found, Root](UWidget *W) {
        if (Found || !W) {
            return;
        }
        if (UCommonButtonBase *Button = Cast<UCommonButtonBase>(W)) {
            if (Button->GetIsEnabled() && UIStyle_IsShown(Button, Root)) {
                Found = Button;
            }
        }
        else if (UUserWidget *Nested = Cast<UUserWidget>(W)) {
            if (UIStyle_IsShown(Nested, Root)) {
                Found = FindFirstFocusable(Nested);
            }
        }
    });
    return Found;
}

void FMythicUIStyle::BindButtonClicked(UWidget *Button, UObject *Handler, FName FunctionName) {
    if (!Button || !Handler || FunctionName.IsNone()) {
        return;
    }
    if (UCommonButtonBase *Common = Cast<UCommonButtonBase>(Button)) {
        UFunction *Fn = Handler->FindFunction(FunctionName);
        if (!Fn) {
            return;
        }
        Common->OnClicked().AddWeakLambda(Handler, [Handler, Fn]() { Handler->ProcessEvent(Fn, nullptr); });
        return;
    }
    if (UButton *Plain = Cast<UButton>(Button)) {
        FScriptDelegate Del;
        Del.BindUFunction(Handler, FunctionName);
        Plain->OnClicked.AddUnique(Del);
    }
}
