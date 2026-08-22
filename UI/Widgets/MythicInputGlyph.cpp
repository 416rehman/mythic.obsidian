// Copyright Stellar Games. All Rights Reserved.

#include "MythicInputGlyph.h"

#include "CommonInputBaseTypes.h"
#include "CommonInputSubsystem.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputAction.h"
#include "Input/CommonUIInputSettings.h"
#include "Input/UIActionBinding.h"
#include "Styling/StyleDefaults.h"
#include "UITag.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UI/MythicUIStyle.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"

UMythicInputGlyph::UMythicInputGlyph() {
    KeyCapMaterial = TSoftObjectPtr<UMaterialInterface>(
        FSoftObjectPath(TEXT("/Game/Mythic/UI/Globals/materials/M_UI_KeyCap.M_UI_KeyCap")));

    KeyFont.FontObject = LoadObject<UObject>(
        nullptr, TEXT("/Game/Mythic/UI/Fonts/Aleo/Aleo-Regular_Font.Aleo-Regular_Font"));
    KeyFont.TypefaceFontName = TEXT("Default");
    KeyFont.Size = 11;
    KeyFont.OutlineSettings.OutlineSize = 0;
}

void UMythicInputGlyph::SetActionTag(FGameplayTag InActionTag) {
    if (ActionTag == InActionTag) {
        return;
    }
    ActionTag = InActionTag;
    RefreshGlyph();
}

void UMythicInputGlyph::SetActionBinding(FUIActionBindingHandle InHandle) {
    BindingHandle = InHandle;
    RefreshGlyph();
}

void UMythicInputGlyph::SetEnhancedAction(const UInputAction *InAction) {
    if (EnhancedAction == InAction) {
        return;
    }
    EnhancedAction = InAction;
    RefreshGlyph();
}

FText UMythicInputGlyph::GetShortKeyLabel(const FKey &Key) {
    static const TMap<FName, FString> Shorthand = {
        {EKeys::LeftMouseButton.GetFName(), TEXT("LMB")},
        {EKeys::RightMouseButton.GetFName(), TEXT("RMB")},
        {EKeys::MiddleMouseButton.GetFName(), TEXT("MMB")},
        {EKeys::MouseScrollUp.GetFName(), TEXT("WHUP")},
        {EKeys::MouseScrollDown.GetFName(), TEXT("WHDN")},
        {EKeys::SpaceBar.GetFName(), TEXT("SPACE")},
        {EKeys::LeftShift.GetFName(), TEXT("SHIFT")},
        {EKeys::RightShift.GetFName(), TEXT("SHIFT")},
        {EKeys::LeftControl.GetFName(), TEXT("CTRL")},
        {EKeys::RightControl.GetFName(), TEXT("CTRL")},
        {EKeys::LeftAlt.GetFName(), TEXT("ALT")},
        {EKeys::RightAlt.GetFName(), TEXT("ALT")},
        {EKeys::Escape.GetFName(), TEXT("ESC")},
        {EKeys::Enter.GetFName(), TEXT("ENTER")},
        {EKeys::Tab.GetFName(), TEXT("TAB")},
        {EKeys::BackSpace.GetFName(), TEXT("BKSP")},
        {EKeys::Up.GetFName(), TEXT("UP")},
        {EKeys::Down.GetFName(), TEXT("DN")},
        {EKeys::Left.GetFName(), TEXT("<")},
        {EKeys::Right.GetFName(), TEXT(">")},
    };

    if (const FString *Found = Shorthand.Find(Key.GetFName())) {
        return FText::FromString(*Found);
    }

    FString Name = Key.GetDisplayName(false).ToString();
    Name.RemoveSpacesInline();
    return FText::FromString(Name.ToUpper());
}

TSharedRef<SWidget> UMythicInputGlyph::RebuildWidget() {
    TSharedRef<SWidget> Image = Super::RebuildWidget();

    return SNew(SOverlay)
           + SOverlay::Slot()
           [
               Image
           ]
           + SOverlay::Slot()
           .HAlign(HAlign_Center)
           .VAlign(VAlign_Center)
           [
               SAssignNew(KeyLabel, STextBlock)
               .Justification(ETextJustify::Center)
               .Visibility(EVisibility::HitTestInvisible)
           ];
}

void UMythicInputGlyph::OnWidgetRebuilt() {
    Super::OnWidgetRebuilt();
    Listen(true);
    RefreshGlyph();
}

void UMythicInputGlyph::ReleaseSlateResources(bool bReleaseChildren) {
    Listen(false);
    KeyLabel.Reset();
    Super::ReleaseSlateResources(bReleaseChildren);
}

void UMythicInputGlyph::SynchronizeProperties() {
    Super::SynchronizeProperties();
    RefreshGlyph();
}

#if WITH_EDITOR
const FText UMythicInputGlyph::GetPaletteCategory() {
    return NSLOCTEXT("Mythic", "MythicPalette", "Mythic");
}
#endif

void UMythicInputGlyph::Listen(bool bListen) {
    const ULocalPlayer *LocalPlayer = GetOwningLocalPlayer();
    UCommonInputSubsystem *Input = LocalPlayer ? UCommonInputSubsystem::Get(LocalPlayer) : nullptr;
    if (!Input) {
        return;
    }

    if (bListen) {
        if (!InputMethodHandle.IsValid()) {
            InputMethodHandle = Input->OnInputMethodChangedNative.AddUObject(this, &UMythicInputGlyph::HandleInputMethodChanged);
        }
    }
    else if (InputMethodHandle.IsValid()) {
        Input->OnInputMethodChangedNative.Remove(InputMethodHandle);
        InputMethodHandle.Reset();
    }
}

void UMythicInputGlyph::HandleInputMethodChanged(ECommonInputType NewType) {
    RefreshGlyph();
}

bool UMythicInputGlyph::KeyMatchesInputType(const FKey &Key, ECommonInputType InputType) {
    if (!Key.IsValid()) {
        return false;
    }
    switch (InputType) {
        case ECommonInputType::Gamepad:
            return Key.IsGamepadKey();
        case ECommonInputType::Touch:
            return Key.IsTouch();
        case ECommonInputType::MouseAndKeyboard:
        default:
            return !Key.IsGamepadKey() && !Key.IsTouch();
    }
}

void UMythicInputGlyph::GatherKeys(TArray<FKey> &OutKeys) const {
    if (EnhancedAction) {
        if (const ULocalPlayer *LocalPlayer = GetOwningLocalPlayer()) {
            if (const UEnhancedInputLocalPlayerSubsystem *Enhanced = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()) {
                OutKeys = Enhanced->QueryKeysMappedToAction(EnhancedAction);
                if (OutKeys.Num() > 0) {
                    return;
                }
            }
        }
    }

    if (BindingHandle.IsValid()) {
        if (const TSharedPtr<FUIActionBinding> Binding = FUIActionBinding::FindBinding(BindingHandle)) {
            // An enhanced-input binding carries no key mappings of its own - NormalMappings is the legacy
            // tag path. Its keys live in the input subsystem, keyed by the action asset.
            if (const UInputAction *Action = Binding->InputAction.Get()) {
                if (const ULocalPlayer *LocalPlayer = GetOwningLocalPlayer()) {
                    if (const UEnhancedInputLocalPlayerSubsystem *Enhanced =
                            LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()) {
                        OutKeys = Enhanced->QueryKeysMappedToAction(Action);
                        if (OutKeys.Num() > 0) {
                            return;
                        }
                    }
                }
            }
            for (const FUIActionKeyMapping &Mapping : Binding->NormalMappings) {
                OutKeys.Add(Mapping.Key);
            }
            for (const FUIActionKeyMapping &Mapping : Binding->HoldMappings) {
                OutKeys.AddUnique(Mapping.Key);
            }
            if (OutKeys.Num() > 0) {
                return;
            }
        }
    }

    if (ActionTag.IsValid()) {
        if (const FUIInputAction *Action = UCommonUIInputSettings::Get().FindAction(FUIActionTag::ConvertChecked(ActionTag))) {
            for (const FUIActionKeyMapping &Mapping : Action->KeyMappings) {
                OutKeys.Add(Mapping.Key);
            }
        }
    }
}

void UMythicInputGlyph::RefreshGlyph() {
    if (!MyImage.IsValid()) {
        return;
    }

    const ULocalPlayer *LocalPlayer = GetOwningLocalPlayer();
    const UCommonInputSubsystem *Input = LocalPlayer ? UCommonInputSubsystem::Get(LocalPlayer) : nullptr;
    if (!Input) {
        UE_LOG(LogTemp, Verbose, TEXT("MythicInputGlyph: no CommonInputSubsystem (localplayer=%d)"), LocalPlayer ? 1 : 0);
        SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    TArray<FKey> Keys;
    GatherKeys(Keys);
    if (Keys.Num() == 0) {
        UE_LOG(LogTemp, Verbose, TEXT("MythicInputGlyph: no keys for action '%s' / tag '%s'"),
               *GetNameSafe(EnhancedAction), *ActionTag.ToString());
        SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    const ECommonInputType InputType = Input->GetCurrentInputType();
    const FName GamepadName = Input->GetCurrentGamepadName();

    if (InputType == ECommonInputType::MouseAndKeyboard) {
        for (const FKey &Key : Keys) {
            if (!KeyMatchesInputType(Key, InputType)) {
                continue;
            }
            UMaterialInterface *CapMaterial = KeyCapMaterial.IsNull() ? nullptr : KeyCapMaterial.LoadSynchronous();
            if (!CapMaterial) {
                break;
            }
            if (!KeyCapMID || KeyCapMID->Parent != CapMaterial) {
                KeyCapMID = UMaterialInstanceDynamic::Create(CapMaterial, this);
            }
            const FLinearColor LetterInk = KeyInk.Equals(GetDefault<UMythicInputGlyph>()->KeyInk) ? FMythicUIStyle::Get().Ink : KeyInk;
            if (KeyCapMID) {
                KeyCapMID->SetVectorParameterValue(TEXT("LineColor"), LetterInk);
            }

            const FText Label = GetShortKeyLabel(Key);

            const int32 Chars = FMath::Max(Label.ToString().Len(), 1);
            float Width = GlyphHeight;
            if (KeyFont.HasValidFont()) {
                const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
                const float TextWidth = Measure->Measure(Label, KeyFont).X;
                Width = FMath::Max(GlyphHeight, TextWidth + GlyphHeight * 0.55f);
            }
            else if (Chars > 1) {
                Width = GlyphHeight * (0.55f + 0.42f * Chars);
            }

            FSlateBrush CapBrush;
            CapBrush.SetResourceObject(KeyCapMID);
            CapBrush.ImageSize = FVector2D(Width, GlyphHeight);
            CapBrush.DrawAs = ESlateBrushDrawType::Image;
            SetBrush(CapBrush);

            if (KeyLabel.IsValid()) {
                KeyLabel->SetText(Label);
                if (KeyFont.HasValidFont()) {
                    KeyLabel->SetFont(KeyFont);
                }
                KeyLabel->SetColorAndOpacity(FSlateColor(LetterInk));
                KeyLabel->SetVisibility(EVisibility::HitTestInvisible);
            }
            SetVisibility(ESlateVisibility::HitTestInvisible);
            return;
        }
    }

    if (KeyLabel.IsValid()) {
        KeyLabel->SetVisibility(EVisibility::Collapsed);
    }

    FSlateBrush Found;
    bool bHasBrush = false;
    for (const FKey &Key : Keys) {
        if (!KeyMatchesInputType(Key, InputType)) {
            continue;
        }
        if (UCommonInputPlatformSettings::Get()->TryGetInputBrush(Found, Key, InputType, GamepadName)) {
            bHasBrush = true;
            break;
        }
    }

    if (!bHasBrush) {
        UE_LOG(LogTemp, Verbose, TEXT("MythicInputGlyph: %d key(s) for '%s' but none has art for input type %d"),
               Keys.Num(), *GetNameSafe(EnhancedAction), static_cast<int32>(InputType));
        SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    const FVector2D Source = Found.GetImageSize();
    const float Aspect = (Source.Y > KINDA_SMALL_NUMBER) ? (Source.X / Source.Y) : 1.0f;
    Found.ImageSize = FVector2D(GlyphHeight * Aspect, GlyphHeight);

    Found.TintColor = FSlateColor(FLinearColor(0.62f, 0.60f, 0.55f, 0.92f));

    SetBrush(Found);
    SetVisibility(ESlateVisibility::HitTestInvisible);
}
