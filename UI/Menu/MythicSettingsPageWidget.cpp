// Copyright Stellar Games. All Rights Reserved.

#include "MythicSettingsPageWidget.h"

#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "CommonButtonBase.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/MythicUIStyle.h"
#include "UI/Settings/MythicUserSettings.h"

namespace {
const FName Stp_IconName(TEXT("Icon"));
const FName Stp_SigilIndexParam(TEXT("Sigil"));
const FName Stp_SigilInkParam(TEXT("Ink"));
const TCHAR *Stp_SigilMaterialPath = TEXT("/Game/Mythic/UI/Globals/materials/M_UI_Sigil.M_UI_Sigil");
constexpr float Stp_ChevronSigil = 16.0f;
const FLinearColor Stp_ChevronRest(0.42f, 0.29f, 0.09f, 1.0f);
const FLinearColor Stp_ChevronHover(0.95f, 0.78f, 0.40f, 1.0f);

UMaterialInterface *Stp_SigilMaterial() {
    static TWeakObjectPtr<UMaterialInterface> Cached;
    if (!Cached.IsValid()) {
        Cached = LoadObject<UMaterialInterface>(nullptr, Stp_SigilMaterialPath);
    }
    return Cached.Get();
}

bool Stp_MakeArrow(UWidget *Button, UCommonTextBlock *GlyphText, bool bPointsLeft) {
    UCommonButtonBase *Common = Cast<UCommonButtonBase>(Button);
    UImage *Icon = Common ? Cast<UImage>(Common->GetWidgetFromName(Stp_IconName)) : nullptr;
    UMaterialInterface *Sigil = Stp_SigilMaterial();
    if (!Icon || !Sigil) {
        return false;
    }
    Icon->SetBrushFromMaterial(Sigil);
    UMaterialInstanceDynamic *MID = Icon->GetDynamicMaterial();
    if (!MID) {
        return false;
    }
    MID->SetScalarParameterValue(Stp_SigilIndexParam, Stp_ChevronSigil);
    MID->SetVectorParameterValue(Stp_SigilInkParam, Stp_ChevronRest);
    Icon->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
    Icon->SetRenderScale(FVector2D(bPointsLeft ? -1.0f : 1.0f, 1.0f));
    Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
    if (GlyphText) {
        GlyphText->SetVisibility(ESlateVisibility::Collapsed);
    }
    TWeakObjectPtr<UMaterialInstanceDynamic> WeakMID(MID);
    Common->OnHovered().AddWeakLambda(Icon, [WeakMID]() {
        if (UMaterialInstanceDynamic *M = WeakMID.Get()) {
            M->SetVectorParameterValue(Stp_SigilInkParam, Stp_ChevronHover);
        }
    });
    Common->OnUnhovered().AddWeakLambda(Icon, [WeakMID]() {
        if (UMaterialInstanceDynamic *M = WeakMID.Get()) {
            M->SetVectorParameterValue(Stp_SigilInkParam, Stp_ChevronRest);
        }
    });
    return true;
}

int32 WrapIndex(int32 Index, int32 Count) {
    if (Count <= 0) {
        return 0;
    }
    return ((Index % Count) + Count) % Count;
}

const TCHAR *QualityWord(int32 Level) {
    switch (Level) {
        case 0:
            return TEXT("Low");
        case 1:
            return TEXT("Medium");
        case 2:
            return TEXT("High");
        case 3:
            return TEXT("Epic");
        case 4:
            return TEXT("Cinematic");
        default:
            return TEXT("Custom");
    }
}
}

void UMythicSettingStepProxy::HandleClicked() {
    if (UMythicSettingsPageWidget *Owner = Page.Get()) {
        Owner->StepSetting(RowIndex, Delta);
    }
}


void UMythicSettingsPageWidget::NativeConstruct() {
    if (!bBuilt) {
        bBuilt = true;

        BuildDefinitions();

        if (SettingsList) {
            for (int32 i = 0; i < Definitions.Num(); ++i) {
                GetOrCreateRow(i);
            }
        }
        if (Btn_Apply) {
            Btn_Apply->OnClicked.AddDynamic(this, &UMythicSettingsPageWidget::HandleApplyClicked);
        }
        if (Btn_Defaults) {
            Btn_Defaults->OnClicked.AddDynamic(this, &UMythicSettingsPageWidget::HandleDefaultsClicked);
        }
    }

    Super::NativeConstruct();
}

void UMythicSettingsPageWidget::NativeOnActivated() {
    Super::NativeOnActivated();
    Refresh();
}


void UMythicSettingsPageWidget::BuildDefinitions() {
    Definitions.Reset();

    const auto Settings = []() -> UMythicUserSettings * { return UMythicUserSettings::Get(); };

    const auto Heading = [this](const FText &Text) {
        FSettingDef Def;
        Def.Label = Text;
        Def.bHeading = true;
        Def.Category = Text;
        Definitions.Add(MoveTemp(Def));
    };


    const auto AddSlider = [this](const FText &Label, const FText &Desc, float StepFrac,
                                  TFunction<float()> GetNorm, TFunction<void(float)> SetNorm,
                                  TFunction<FText()> Display, TFunction<void()> Default,
                                  bool bNeedsApply = false) {
        FSettingDef Def;
        Def.Label = Label;
        Def.Description = Desc;
        Def.Control = ESettingControl::Slider;
        Def.StepFraction = StepFrac;
        Def.ReadNormalised = GetNorm;
        Def.SetNormalised = SetNorm;
        Def.ReadDisplay = Display;
        Def.RestoreDefault = Default;
        Def.bNeedsApply = bNeedsApply;
        Def.Read = Display;
        Def.Step = [GetNorm, SetNorm, StepFrac](int32 Delta) {
            SetNorm(FMath::Clamp(GetNorm() + Delta * StepFrac, 0.0f, 1.0f));
        };
        Definitions.Add(MoveTemp(Def));
    };

    const auto AddToggle = [this](const FText &Label, const FText &Desc,
                                  TFunction<bool()> Get, TFunction<void(bool)> Set,
                                  bool bNeedsApply = false) {
        FSettingDef Def;
        Def.Label = Label;
        Def.Description = Desc;
        Def.Control = ESettingControl::Toggle;
        Def.ReadBool = Get;
        Def.SetBool = Set;
        Def.bNeedsApply = bNeedsApply;
        Def.Read = [Get]() {
            return Get() ? NSLOCTEXT("Mythic", "SetOn", "On") : NSLOCTEXT("Mythic", "SetOff", "Off");
        };
        Def.Step = [Get, Set](int32 Delta) { Set(Delta > 0); };
        Definitions.Add(MoveTemp(Def));
    };

    const auto AddChoice = [this](const FText &Label, const FText &Desc, ESettingControl Kind,
                                  TFunction<int32()> Count, TFunction<int32()> Index,
                                  TFunction<void(int32)> SetIdx, TFunction<FText(int32)> OptLabel,
                                  bool bNeedsApply = false) {
        FSettingDef Def;
        Def.Label = Label;
        Def.Description = Desc;
        Def.Control = Kind;
        Def.OptionCount = Count;
        Def.ReadIndex = Index;
        Def.SetIndex = SetIdx;
        Def.OptionLabel = OptLabel;
        Def.bNeedsApply = bNeedsApply;
        Def.Read = [Index, OptLabel]() { return OptLabel(Index()); };
        Def.Step = [Count, Index, SetIdx](int32 Delta) {
            SetIdx(FMath::Clamp(Index() + Delta, 0, FMath::Max(Count() - 1, 0)));
        };
        Definitions.Add(MoveTemp(Def));
    };

    const auto AddQuality = [this, Settings, AddChoice](const FText &Label, const FText &Desc,
                                                        TFunction<int32(UMythicUserSettings *)> Get,
                                                        TFunction<void(UMythicUserSettings *, int32)> Set) {
        AddChoice(Label, Desc, ESettingControl::Stepper,
                  []() { return 5; },
                  [Settings, Get]() { UMythicUserSettings *S = Settings(); return S ? Get(S) : 0; },
                  [Settings, Set](int32 I) { if (UMythicUserSettings *S = Settings()) { Set(S, I); } },
                  [](int32 I) { return FText::FromString(QualityWord(I)); },
 true);
    };

    Heading(NSLOCTEXT("Mythic", "SetDisplay", "DISPLAY"));

    AddChoice(NSLOCTEXT("Mythic", "SetWindowMode", "Window Mode"),
              NSLOCTEXT("Mythic", "DescWindowMode",
                        "Fullscreen gives the game the whole screen and the best performance. Borderless keeps the "
                        "desktop behind it so alt-tab is instant, at a small cost. Windowed runs in a movable window."),
              ESettingControl::Stepper,
              []() { return 3; },
              [Settings]() {
                  UMythicUserSettings *S = Settings();
                  if (!S) { return 0; }
                  switch (S->GetFullscreenMode()) {
                      case EWindowMode::Fullscreen: return 0;
                      case EWindowMode::WindowedFullscreen: return 1;
                      default: return 2;
                  }
              },
              [Settings](int32 I) {
                  if (UMythicUserSettings *S = Settings()) {
                      const EWindowMode::Type Modes[] = {EWindowMode::Fullscreen, EWindowMode::WindowedFullscreen,
                                                          EWindowMode::Windowed};
                      S->SetFullscreenMode(Modes[FMath::Clamp(I, 0, 2)]);
                  }
              },
              [](int32 I) {
                  switch (I) {
                      case 0: return NSLOCTEXT("Mythic", "WMFull", "Fullscreen");
                      case 1: return NSLOCTEXT("Mythic", "WMBorderless", "Borderless");
                      default: return NSLOCTEXT("Mythic", "WMWindowed", "Windowed");
                  }
              },
 true);

    {
        FSettingDef Def;
        Def.Label = NSLOCTEXT("Mythic", "SetResolution", "Resolution");
        Def.Description = NSLOCTEXT("Mythic", "DescResolution",
                                    "How many pixels the game renders and displays. Lower resolutions run faster but "
                                    "look softer. Your monitor's own resolution is usually the sharpest choice.");
        Def.Control = ESettingControl::Dropdown;
        Def.bNeedsApply = true;
        Def.OptionCount = []() {
            TArray<FIntPoint> Res;
            UKismetSystemLibrary::GetSupportedFullscreenResolutions(Res);
            return Res.Num();
        };
        Def.OptionLabel = [](int32 I) {
            TArray<FIntPoint> Res;
            UKismetSystemLibrary::GetSupportedFullscreenResolutions(Res);
            return Res.IsValidIndex(I) ? FText::FromString(FString::Printf(TEXT("%d x %d"), Res[I].X, Res[I].Y))
                                       : FText::GetEmpty();
        };
        Def.ReadIndex = [Settings]() {
            UMythicUserSettings *S = Settings();
            if (!S) { return 0; }
            TArray<FIntPoint> Res;
            UKismetSystemLibrary::GetSupportedFullscreenResolutions(Res);
            const int32 Found = Res.IndexOfByKey(S->GetScreenResolution());
            return Found == INDEX_NONE ? 0 : Found;
        };
        Def.SetIndex = [Settings](int32 I) {
            TArray<FIntPoint> Res;
            UKismetSystemLibrary::GetSupportedFullscreenResolutions(Res);
            if (Res.IsValidIndex(I)) {
                if (UMythicUserSettings *S = Settings()) { S->SetScreenResolution(Res[I]); }
            }
        };
        Def.Read = [Settings]() {
            UMythicUserSettings *S = Settings();
            if (!S) { return FText::GetEmpty(); }
            const FIntPoint R = S->GetScreenResolution();
            return FText::FromString(FString::Printf(TEXT("%d x %d"), R.X, R.Y));
        };
        Def.Step = [Def](int32 Delta) {
            Def.SetIndex(FMath::Clamp(Def.ReadIndex() + Delta, 0, FMath::Max(Def.OptionCount() - 1, 0)));
        };
        Definitions.Add(MoveTemp(Def));
    }

    AddToggle(NSLOCTEXT("Mythic", "SetVSync", "V-Sync"),
              NSLOCTEXT("Mythic", "DescVSync",
                        "Locks the game's frame rate to your monitor so images never tear halfway down the screen. "
                        "Costs a little input responsiveness."),
              [Settings]() { UMythicUserSettings *S = Settings(); return S && S->IsVSyncEnabled(); },
              [Settings](bool b) { if (UMythicUserSettings *S = Settings()) { S->SetVSyncEnabled(b); } },
 true);

    {
        static const int32 Caps[] = {0, 30, 60, 90, 120, 144, 165, 240};
        AddChoice(NSLOCTEXT("Mythic", "SetFrameCap", "Frame Rate Limit"),
                  NSLOCTEXT("Mythic", "DescFrameCap",
                            "The most frames per second the game will draw. A cap below what your machine can manage "
                            "keeps the frame rate steady, and runs the hardware cooler and quieter."),
                  ESettingControl::Dropdown,
                  []() { return UE_ARRAY_COUNT(Caps); },
                  [Settings]() {
                      UMythicUserSettings *S = Settings();
                      const float Cur = S ? S->GetFrameRateLimit() : 0.0f;
                      for (int32 i = 0; i < UE_ARRAY_COUNT(Caps); ++i) {
                          if (FMath::IsNearlyEqual(static_cast<float>(Caps[i]), Cur)) { return i; }
                      }
                      return 0;
                  },
                  [Settings](int32 I) {
                      if (UMythicUserSettings *S = Settings()) {
                          S->SetFrameRateLimit(static_cast<float>(Caps[FMath::Clamp(I, 0, UE_ARRAY_COUNT(Caps) - 1)]));
                      }
                  },
                  [](int32 I) {
                      const int32 V = Caps[FMath::Clamp(I, 0, UE_ARRAY_COUNT(Caps) - 1)];
                      return V == 0 ? NSLOCTEXT("Mythic", "CapOff", "Unlimited")
                                    : FText::FromString(FString::Printf(TEXT("%d fps"), V));
                  },
 true);
    }

    AddSlider(NSLOCTEXT("Mythic", "SetBrightness", "Brightness"),
              NSLOCTEXT("Mythic", "DescBrightness",
                        "Screen gamma. Raise it until the darkest parts of a night scene are just readable, and no "
                        "further -- too high washes the colour out of everything."),
              0.0625f,
              [Settings]() { UMythicUserSettings *S = Settings(); return S ? (S->GetDisplayGamma() - 1.8f) / 0.8f : 0.5f; },
              [Settings](float N) { if (UMythicUserSettings *S = Settings()) { S->SetDisplayGamma(1.8f + N * 0.8f); } },
              [Settings]() {
                  UMythicUserSettings *S = Settings();
                  return FText::FromString(FString::Printf(TEXT("%.2f"), S ? S->GetDisplayGamma() : 2.2f));
              },
              [Settings]() { if (UMythicUserSettings *S = Settings()) { S->SetDisplayGamma(2.2f); } });

    Heading(NSLOCTEXT("Mythic", "SetGraphics", "GRAPHICS"));

    AddChoice(NSLOCTEXT("Mythic", "SetOverall", "Overall Quality"),
              NSLOCTEXT("Mythic", "DescOverall",
                        "Sets every quality option below at once. Change any one of them afterwards and this reads "
                        "Custom."),
              ESettingControl::Stepper,
              []() { return 5; },
              [Settings]() { UMythicUserSettings *S = Settings(); return S ? FMath::Max(S->GetQualityPresetLevel(), 0) : 0; },
              [Settings](int32 I) { if (UMythicUserSettings *S = Settings()) { S->SetOverallScalabilityLevel(I); } },
              [Settings](int32 I) {
                  UMythicUserSettings *S = Settings();
                  return (S && S->GetQualityPresetLevel() < 0) ? NSLOCTEXT("Mythic", "QCustom", "Custom")
                                                               : FText::FromString(QualityWord(I));
              },
 true);

    AddSlider(NSLOCTEXT("Mythic", "SetRenderScale", "Render Scale"),
              NSLOCTEXT("Mythic", "DescRenderScale",
                        "Renders the world at a fraction of your resolution and scales it back up. The single biggest "
                        "performance lever there is. Below about 70% the image starts to look soft."),
              0.1f,
              [Settings]() { UMythicUserSettings *S = Settings(); return S ? (S->GetRenderScale() - 50.0f) / 50.0f : 1.0f; },
              [Settings](float N) { if (UMythicUserSettings *S = Settings()) { S->SetRenderScale(50.0f + N * 50.0f); } },
              [Settings]() {
                  UMythicUserSettings *S = Settings();
                  return FText::FromString(FString::Printf(TEXT("%.0f%%"), S ? S->GetRenderScale() : 100.0f));
              },
              [Settings]() { if (UMythicUserSettings *S = Settings()) { S->SetRenderScale(100.0f); } },
 true);

    AddQuality(NSLOCTEXT("Mythic", "SetViewDist", "View Distance"),
               NSLOCTEXT("Mythic", "DescViewDist", "How far away objects keep being drawn. The most visible setting in "
                                                   "an open world, and one of the more expensive."),
               [](UMythicUserSettings *S) { return S->GetViewDistanceQuality(); },
               [](UMythicUserSettings *S, int32 V) { S->SetViewDistanceQuality(V); });
    AddQuality(NSLOCTEXT("Mythic", "SetShadows", "Shadows"),
               NSLOCTEXT("Mythic", "DescShadows", "Shadow resolution and how far shadows are cast. Usually the first "
                                                  "thing to lower when the frame rate is short."),
               [](UMythicUserSettings *S) { return S->GetShadowQuality(); },
               [](UMythicUserSettings *S, int32 V) { S->SetShadowQuality(V); });
    AddQuality(NSLOCTEXT("Mythic", "SetGI", "Global Illumination"),
               NSLOCTEXT("Mythic", "DescGI", "Indirect light bouncing off surfaces. Costs a lot and mostly shows in "
                                             "interiors and shade."),
               [](UMythicUserSettings *S) { return S->GetGlobalIlluminationQuality(); },
               [](UMythicUserSettings *S, int32 V) { S->SetGlobalIlluminationQuality(V); });
    AddQuality(NSLOCTEXT("Mythic", "SetReflections", "Reflections"),
               NSLOCTEXT("Mythic", "DescReflections", "Reflection detail on water, metal and glass."),
               [](UMythicUserSettings *S) { return S->GetReflectionQuality(); },
               [](UMythicUserSettings *S, int32 V) { S->SetReflectionQuality(V); });
    AddQuality(NSLOCTEXT("Mythic", "SetTextures", "Textures"),
               NSLOCTEXT("Mythic", "DescTextures", "Texture detail. Bound by video memory rather than raw speed, so on "
                                                   "a card with headroom this is nearly free."),
               [](UMythicUserSettings *S) { return S->GetTextureQuality(); },
               [](UMythicUserSettings *S, int32 V) { S->SetTextureQuality(V); });
    AddQuality(NSLOCTEXT("Mythic", "SetEffects", "Effects"),
               NSLOCTEXT("Mythic", "DescEffects", "Particle counts and the detail of spell and impact effects."),
               [](UMythicUserSettings *S) { return S->GetVisualEffectQuality(); },
               [](UMythicUserSettings *S, int32 V) { S->SetVisualEffectQuality(V); });
    AddQuality(NSLOCTEXT("Mythic", "SetPostProcess", "Post Processing"),
               NSLOCTEXT("Mythic", "DescPostProcess", "Screen effects applied after the world is drawn -- depth of "
                                                      "field, ambient occlusion and the like."),
               [](UMythicUserSettings *S) { return S->GetPostProcessingQuality(); },
               [](UMythicUserSettings *S, int32 V) { S->SetPostProcessingQuality(V); });
    AddQuality(NSLOCTEXT("Mythic", "SetFoliage", "Foliage"),
               NSLOCTEXT("Mythic", "DescFoliage", "How much grass and undergrowth is drawn, and how far out."),
               [](UMythicUserSettings *S) { return S->GetFoliageQuality(); },
               [](UMythicUserSettings *S, int32 V) { S->SetFoliageQuality(V); });
    AddQuality(NSLOCTEXT("Mythic", "SetShading", "Shading"),
               NSLOCTEXT("Mythic", "DescShading", "Material and lighting detail on surfaces."),
               [](UMythicUserSettings *S) { return S->GetShadingQuality(); },
               [](UMythicUserSettings *S, int32 V) { S->SetShadingQuality(V); });

    Heading(NSLOCTEXT("Mythic", "SetImage", "IMAGE"));

    {
        static const int32 AAMethods[] = {0, 1, 2, 4, 5};
        AddChoice(NSLOCTEXT("Mythic", "SetAA", "Anti-Aliasing"),
                  NSLOCTEXT("Mythic", "DescAA",
                            "Smooths the jagged stair-steps on edges. TSR is sharpest and steadiest in motion and is "
                            "the right default. TAA is cheaper but softer. FXAA is the cheapest and blurs most. Off "
                            "is sharpest of all and shimmers on every edge as you move."),
                  ESettingControl::Stepper,
                  []() { return UE_ARRAY_COUNT(AAMethods); },
                  [Settings]() {
                      UMythicUserSettings *S = Settings();
                      const int32 Cur = S ? S->GetAntiAliasingMethod() : 4;
                      for (int32 i = 0; i < UE_ARRAY_COUNT(AAMethods); ++i) {
                          if (AAMethods[i] == Cur) { return i; }
                      }
                      return 3;
                  },
                  [Settings](int32 I) {
                      if (UMythicUserSettings *S = Settings()) {
                          S->SetAntiAliasingMethod(AAMethods[FMath::Clamp(I, 0, UE_ARRAY_COUNT(AAMethods) - 1)]);
                      }
                  },
                  [](int32 I) {
                      switch (AAMethods[FMath::Clamp(I, 0, UE_ARRAY_COUNT(AAMethods) - 1)]) {
                          case 0: return NSLOCTEXT("Mythic", "AAOff", "Off");
                          case 1: return NSLOCTEXT("Mythic", "AAFXAA", "FXAA");
                          case 2: return NSLOCTEXT("Mythic", "AATAA", "TAA");
                          case 5: return NSLOCTEXT("Mythic", "AASMAA", "SMAA");
                          default: return NSLOCTEXT("Mythic", "AATSR", "TSR");
                      }
                  });
    }

    AddSlider(NSLOCTEXT("Mythic", "SetSharpness", "Sharpness"),
              NSLOCTEXT("Mythic", "DescSharpness",
                        "Puts edge definition back after anti-aliasing softens the image. Too much and everything "
                        "grows a bright outline."),
              0.05f,
              [Settings]() { UMythicUserSettings *S = Settings(); return S ? FMath::Max(S->GetSharpness(), 0.0f) : 0.4f; },
              [Settings](float N) { if (UMythicUserSettings *S = Settings()) { S->SetSharpness(N); } },
              [Settings]() {
                  UMythicUserSettings *S = Settings();
                  const float V = S ? S->GetSharpness() : 0.4f;
                  return V < 0.0f ? NSLOCTEXT("Mythic", "SharpScene", "Scene")
                                  : FText::FromString(FString::Printf(TEXT("%.2f"), V));
              },
              [Settings]() { if (UMythicUserSettings *S = Settings()) { S->SetSharpness(0.4f); } });

    AddChoice(NSLOCTEXT("Mythic", "SetMotionBlur", "Motion Blur"),
              NSLOCTEXT("Mythic", "DescMotionBlur",
                        "Smears the image in the direction things are moving. Some players find it cinematic; it is "
                        "also the commonest cause of motion sickness, so it ships off."),
              ESettingControl::Stepper,
              []() { return 5; },
              [Settings]() { UMythicUserSettings *S = Settings(); return S ? S->GetMotionBlurQuality() : 0; },
              [Settings](int32 I) { if (UMythicUserSettings *S = Settings()) { S->SetMotionBlurQuality(I); } },
              [](int32 I) { return I == 0 ? NSLOCTEXT("Mythic", "MBOff", "Off")
                                          : FText::FromString(QualityWord(FMath::Clamp(I - 1, 0, 4))); });

    {
        static const int32 Aniso[] = {1, 2, 4, 8, 16};
        AddChoice(NSLOCTEXT("Mythic", "SetAniso", "Texture Filtering"),
                  NSLOCTEXT("Mythic", "DescAniso",
                            "Keeps textures sharp on surfaces seen at a shallow angle, like a road running away from "
                            "you. Nearly free on modern hardware."),
                  ESettingControl::Stepper,
                  []() { return UE_ARRAY_COUNT(Aniso); },
                  [Settings]() {
                      UMythicUserSettings *S = Settings();
                      const int32 Cur = S ? S->GetMaxAnisotropy() : 8;
                      for (int32 i = 0; i < UE_ARRAY_COUNT(Aniso); ++i) { if (Aniso[i] == Cur) { return i; } }
                      return 3;
                  },
                  [Settings](int32 I) {
                      if (UMythicUserSettings *S = Settings()) {
                          S->SetMaxAnisotropy(Aniso[FMath::Clamp(I, 0, UE_ARRAY_COUNT(Aniso) - 1)]);
                      }
                  },
                  [](int32 I) {
                      return FText::FromString(FString::Printf(TEXT("%dx"),
                          Aniso[FMath::Clamp(I, 0, UE_ARRAY_COUNT(Aniso) - 1)]));
                  });
    }

    AddToggle(NSLOCTEXT("Mythic", "SetBloom", "Bloom"),
              NSLOCTEXT("Mythic", "DescBloom", "Lets bright light bleed softly past its edges, the way it does through "
                                               "a lens or a squint."),
              [Settings]() { UMythicUserSettings *S = Settings(); return S && S->GetBloom(); },
              [Settings](bool b) { if (UMythicUserSettings *S = Settings()) { S->SetBloom(b); } });

    Heading(NSLOCTEXT("Mythic", "SetAudio", "AUDIO"));

    AddSlider(NSLOCTEXT("Mythic", "SetMasterVolume", "Master Volume"),
              NSLOCTEXT("Mythic", "DescMasterVolume", "Overall loudness of everything the game plays."),
              0.05f,
              [Settings]() { UMythicUserSettings *S = Settings(); return S ? S->GetMasterVolume() : 1.0f; },
              [Settings](float N) { if (UMythicUserSettings *S = Settings()) { S->SetMasterVolume(N); } },
              [Settings]() {
                  UMythicUserSettings *S = Settings();
                  return FText::FromString(FString::Printf(TEXT("%.0f%%"), (S ? S->GetMasterVolume() : 1.0f) * 100.0f));
              },
              [Settings]() { if (UMythicUserSettings *S = Settings()) { S->SetMasterVolume(1.0f); } });

    AddToggle(NSLOCTEXT("Mythic", "SetMuteUnfocused", "Mute When Unfocused"),
              NSLOCTEXT("Mythic", "DescMuteUnfocused", "Silences the game while you are in another window."),
              [Settings]() { UMythicUserSettings *S = Settings(); return S && S->GetMuteWhenUnfocused(); },
              [Settings](bool b) { if (UMythicUserSettings *S = Settings()) { S->SetMuteWhenUnfocused(b); } });

    Heading(NSLOCTEXT("Mythic", "SetControls", "CONTROLS"));

    AddSlider(NSLOCTEXT("Mythic", "SetMouseSens", "Mouse Sensitivity"),
              NSLOCTEXT("Mythic", "DescMouseSens", "How far the camera turns for a given mouse movement."),
              0.0345f,
              [Settings]() { UMythicUserSettings *S = Settings(); return S ? (S->GetMouseLookSensitivity() - 0.1f) / 2.9f : 0.31f; },
              [Settings](float N) { if (UMythicUserSettings *S = Settings()) { S->SetMouseLookSensitivity(0.1f + N * 2.9f); } },
              [Settings]() {
                  UMythicUserSettings *S = Settings();
                  return FText::FromString(FString::Printf(TEXT("%.2f"), S ? S->GetMouseLookSensitivity() : 1.0f));
              },
              [Settings]() { if (UMythicUserSettings *S = Settings()) { S->SetMouseLookSensitivity(1.0f); } });

    AddSlider(NSLOCTEXT("Mythic", "SetPadSens", "Gamepad Sensitivity"),
              NSLOCTEXT("Mythic", "DescPadSens", "How fast the camera turns while the stick is held. A stick sets a "
                                                 "turn rate, which is why it needs its own number."),
              0.0345f,
              [Settings]() { UMythicUserSettings *S = Settings(); return S ? (S->GetGamepadLookSensitivity() - 0.1f) / 2.9f : 0.31f; },
              [Settings](float N) { if (UMythicUserSettings *S = Settings()) { S->SetGamepadLookSensitivity(0.1f + N * 2.9f); } },
              [Settings]() {
                  UMythicUserSettings *S = Settings();
                  return FText::FromString(FString::Printf(TEXT("%.2f"), S ? S->GetGamepadLookSensitivity() : 1.0f));
              },
              [Settings]() { if (UMythicUserSettings *S = Settings()) { S->SetGamepadLookSensitivity(1.0f); } });

    AddSlider(NSLOCTEXT("Mythic", "SetVertSens", "Vertical Sensitivity"),
              NSLOCTEXT("Mythic", "DescVertSens", "Scales up-and-down look only, so you can slow the vertical without "
                                                  "slowing the turn."),
              0.05f,
              [Settings]() { UMythicUserSettings *S = Settings(); return S ? (S->GetVerticalLookScale() - 0.5f) : 0.5f; },
              [Settings](float N) { if (UMythicUserSettings *S = Settings()) { S->SetVerticalLookScale(0.5f + N); } },
              [Settings]() {
                  UMythicUserSettings *S = Settings();
                  return FText::FromString(FString::Printf(TEXT("%.2f"), S ? S->GetVerticalLookScale() : 1.0f));
              },
              [Settings]() { if (UMythicUserSettings *S = Settings()) { S->SetVerticalLookScale(1.0f); } });

    AddToggle(NSLOCTEXT("Mythic", "SetInvertY", "Invert Look Y"),
              NSLOCTEXT("Mythic", "DescInvertY", "Pushing up looks down, as on a flight stick."),
              [Settings]() { UMythicUserSettings *S = Settings(); return S && S->GetInvertLookY(); },
              [Settings](bool b) { if (UMythicUserSettings *S = Settings()) { S->SetInvertLookY(b); } });

    AddToggle(NSLOCTEXT("Mythic", "SetInvertX", "Invert Look X"),
              NSLOCTEXT("Mythic", "DescInvertX", "Pushing left looks right."),
              [Settings]() { UMythicUserSettings *S = Settings(); return S && S->GetInvertLookX(); },
              [Settings](bool b) { if (UMythicUserSettings *S = Settings()) { S->SetInvertLookX(b); } });

    AddSlider(NSLOCTEXT("Mythic", "SetDeadLeft", "Left Stick Deadzone"),
              NSLOCTEXT("Mythic", "DescDeadLeft", "How far the stick must move before the game listens. Raise it if "
                                                  "your character drifts when you let go."),
              0.025f,
              [Settings]() { UMythicUserSettings *S = Settings(); return S ? S->GetGamepadDeadzoneLeft() / 0.4f : 0.375f; },
              [Settings](float N) { if (UMythicUserSettings *S = Settings()) { S->SetGamepadDeadzoneLeft(N * 0.4f); } },
              [Settings]() {
                  UMythicUserSettings *S = Settings();
                  return FText::FromString(FString::Printf(TEXT("%.2f"), S ? S->GetGamepadDeadzoneLeft() : 0.15f));
              },
              [Settings]() { if (UMythicUserSettings *S = Settings()) { S->SetGamepadDeadzoneLeft(0.15f); } });

    AddSlider(NSLOCTEXT("Mythic", "SetDeadRight", "Right Stick Deadzone"),
              NSLOCTEXT("Mythic", "DescDeadRight", "The same, for the camera stick."),
              0.025f,
              [Settings]() { UMythicUserSettings *S = Settings(); return S ? S->GetGamepadDeadzoneRight() / 0.4f : 0.375f; },
              [Settings](float N) { if (UMythicUserSettings *S = Settings()) { S->SetGamepadDeadzoneRight(N * 0.4f); } },
              [Settings]() {
                  UMythicUserSettings *S = Settings();
                  return FText::FromString(FString::Printf(TEXT("%.2f"), S ? S->GetGamepadDeadzoneRight() : 0.15f));
              },
              [Settings]() { if (UMythicUserSettings *S = Settings()) { S->SetGamepadDeadzoneRight(0.15f); } });

    AddSlider(NSLOCTEXT("Mythic", "SetVibration", "Controller Vibration"),
              NSLOCTEXT("Mythic", "DescVibration", "How strongly the controller rumbles. Zero turns it off entirely."),
              0.1f,
              [Settings]() { UMythicUserSettings *S = Settings(); return S ? S->GetVibrationScale() : 1.0f; },
              [Settings](float N) { if (UMythicUserSettings *S = Settings()) { S->SetVibrationScale(N); } },
              [Settings]() {
                  UMythicUserSettings *S = Settings();
                  return FText::FromString(FString::Printf(TEXT("%.0f%%"), (S ? S->GetVibrationScale() : 1.0f) * 100.0f));
              },
              [Settings]() { if (UMythicUserSettings *S = Settings()) { S->SetVibrationScale(1.0f); } });

    Heading(NSLOCTEXT("Mythic", "SetInterface", "INTERFACE"));

    AddSlider(NSLOCTEXT("Mythic", "SetUIScale", "Interface Scale"),
              NSLOCTEXT("Mythic", "DescUIScale", "Size of every menu and HUD element. Raise it if text is hard to "
                                                 "read from where you sit."),
              0.125f,
              [Settings]() { UMythicUserSettings *S = Settings(); return S ? (S->GetUIScale() - 0.8f) / 0.45f : 0.44f; },
              [Settings](float N) { if (UMythicUserSettings *S = Settings()) { S->SetUIScale(0.8f + N * 0.45f); } },
              [Settings]() {
                  UMythicUserSettings *S = Settings();
                  return FText::FromString(FString::Printf(TEXT("%.0f%%"), (S ? S->GetUIScale() : 1.0f) * 100.0f));
              },
              [Settings]() { if (UMythicUserSettings *S = Settings()) { S->SetUIScale(1.0f); } });

    AddSlider(NSLOCTEXT("Mythic", "SetHUDOpacity", "HUD Opacity"),
              NSLOCTEXT("Mythic", "DescHUDOpacity", "How solid the HUD is over the world."),
              0.1f,
              [Settings]() { UMythicUserSettings *S = Settings(); return S ? (S->GetHUDOpacity() - 0.4f) / 0.6f : 1.0f; },
              [Settings](float N) { if (UMythicUserSettings *S = Settings()) { S->SetHUDOpacity(0.4f + N * 0.6f); } },
              [Settings]() {
                  UMythicUserSettings *S = Settings();
                  return FText::FromString(FString::Printf(TEXT("%.0f%%"), (S ? S->GetHUDOpacity() : 1.0f) * 100.0f));
              },
              [Settings]() { if (UMythicUserSettings *S = Settings()) { S->SetHUDOpacity(1.0f); } });

    AddChoice(NSLOCTEXT("Mythic", "SetDamageNumbers", "Damage Numbers"),
              NSLOCTEXT("Mythic", "DescDamageNumbers", "Whether hits show a number, and whose."),
              ESettingControl::Stepper,
              []() { return 3; },
              [Settings]() { UMythicUserSettings *S = Settings(); return S ? static_cast<int32>(S->GetDamageNumberMode()) : 1; },
              [Settings](int32 I) { if (UMythicUserSettings *S = Settings()) { S->SetDamageNumberMode(static_cast<uint8>(I)); } },
              [](int32 I) {
                  switch (I) {
                      case 0: return NSLOCTEXT("Mythic", "DNOff", "Off");
                      case 2: return NSLOCTEXT("Mythic", "DNAll", "Everything");
                      default: return NSLOCTEXT("Mythic", "DNMine", "Mine only");
                  }
              });

    AddSlider(NSLOCTEXT("Mythic", "SetDamageScale", "Damage Number Size"),
              NSLOCTEXT("Mythic", "DescDamageScale", "How large those numbers are drawn."),
              0.125f,
              [Settings]() { UMythicUserSettings *S = Settings(); return S ? (S->GetDamageNumberScale() - 0.75f) / 0.75f : 0.33f; },
              [Settings](float N) { if (UMythicUserSettings *S = Settings()) { S->SetDamageNumberScale(0.75f + N * 0.75f); } },
              [Settings]() {
                  UMythicUserSettings *S = Settings();
                  return FText::FromString(FString::Printf(TEXT("%.0f%%"), (S ? S->GetDamageNumberScale() : 1.0f) * 100.0f));
              },
              [Settings]() { if (UMythicUserSettings *S = Settings()) { S->SetDamageNumberScale(1.0f); } });

    Heading(NSLOCTEXT("Mythic", "SetAccessibility", "ACCESSIBILITY"));

    AddToggle(NSLOCTEXT("Mythic", "SetAlwaysHUD", "Always Show HUD"),
              NSLOCTEXT("Mythic", "DescAlwaysHUD",
                        "The HUD normally fades what you do not currently need. This holds all of it at full "
                        "strength, permanently."),
              [Settings]() { UMythicUserSettings *S = Settings(); return S && S->GetAlwaysShowHUD(); },
              [Settings](bool b) { if (UMythicUserSettings *S = Settings()) { S->SetAlwaysShowHUD(b); } });

    // Every definition inherits the heading above it. Done in one pass here rather than threaded through the
    // eight Add* helpers, so adding a ninth helper cannot forget to set it.
    FText CurrentCategory;
    for (FSettingDef &Def : Definitions) {
        if (Def.bHeading) {
            CurrentCategory = Def.Label;
        }
        else {
            Def.Category = CurrentCategory;
        }
    }
}


FMythicSettingsRow &UMythicSettingsPageWidget::GetOrCreateRow(int32 Index) {
    if (RowPool.IsValidIndex(Index)) {
        return RowPool[Index];
    }

    FMythicSettingsRow Row;
    UVerticalBox *Outer = WidgetTree->ConstructWidget<UVerticalBox>();
    Row.Box = Outer;
    UHorizontalBox *Box = WidgetTree->ConstructWidget<UHorizontalBox>();
    Outer->AddChild(Box);

    Row.Label = FMythicUIStyle::MakeText(this, EMythicTextRole::Body);
    if (UHorizontalBoxSlot *S = Cast<UHorizontalBoxSlot>(Box->AddChild(Row.Label))) {
        S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        S->SetVerticalAlignment(VAlign_Center);
    }

    {
        static const FSoftObjectPath RulePath(
            TEXT("/Game/Mythic/UI/Globals/materials/kit/MI_UI_Rule_Heading.MI_UI_Rule_Heading"));
        UImage *Rule = WidgetTree->ConstructWidget<UImage>();
        if (UMaterialInterface *RuleMat = Cast<UMaterialInterface>(RulePath.TryLoad())) {
            FSlateBrush Brush;
            Brush.SetResourceObject(RuleMat);
            Brush.DrawAs = ESlateBrushDrawType::Image;
            Brush.ImageSize = FVector2D(160.0f, 8.0f);
            Rule->SetBrush(Brush);
        }
        Rule->SetVisibility(ESlateVisibility::Collapsed);
        if (UHorizontalBoxSlot *S = Cast<UHorizontalBoxSlot>(Box->AddChild(Rule))) {
            S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            S->SetVerticalAlignment(VAlign_Center);
            S->SetPadding(FMargin(16.0f, 0.0f, 8.0f, 0.0f));
        }
    }

    UCommonTextBlock *LeftGlyph = nullptr;
    Row.Left = FMythicUIStyle::MakeButton(this, EMythicTextRole::Body, LeftGlyph);
    LeftGlyph->SetText(FText::FromString(TEXT("<")));
    Stp_MakeArrow(Row.Left, LeftGlyph, true);
    if (UHorizontalBoxSlot *S = Cast<UHorizontalBoxSlot>(Box->AddChild(Row.Left))) {
        S->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        S->SetVerticalAlignment(VAlign_Center);
    }

    Row.Value = FMythicUIStyle::MakeText(this, EMythicTextRole::Body);
    Row.Value->SetJustification(ETextJustify::Center);
    Row.Value->SetMinDesiredWidth(150.0f);
    if (UHorizontalBoxSlot *S = Cast<UHorizontalBoxSlot>(Box->AddChild(Row.Value))) {
        S->SetPadding(FMargin(10.0f, 0.0f, 10.0f, 0.0f));
        S->SetVerticalAlignment(VAlign_Center);
    }

    UCommonTextBlock *RightGlyph = nullptr;
    Row.Right = FMythicUIStyle::MakeButton(this, EMythicTextRole::Body, RightGlyph);
    RightGlyph->SetText(FText::FromString(TEXT(">")));
    Stp_MakeArrow(Row.Right, RightGlyph, false);
    if (UHorizontalBoxSlot *S = Cast<UHorizontalBoxSlot>(Box->AddChild(Row.Right))) {
        S->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        S->SetVerticalAlignment(VAlign_Center);
    }

    Row.LeftProxy = NewObject<UMythicSettingStepProxy>(this);
    Row.LeftProxy->Page = this;
    Row.LeftProxy->RowIndex = Index;
    Row.LeftProxy->Delta = -1;
    FMythicUIStyle::BindButtonClicked(Row.Left, Row.LeftProxy,
                                      GET_FUNCTION_NAME_CHECKED(UMythicSettingStepProxy, HandleClicked));

    Row.RightProxy = NewObject<UMythicSettingStepProxy>(this);
    Row.RightProxy->Page = this;
    Row.RightProxy->RowIndex = Index;
    Row.RightProxy->Delta = 1;
    FMythicUIStyle::BindButtonClicked(Row.Right, Row.RightProxy,
                                      GET_FUNCTION_NAME_CHECKED(UMythicSettingStepProxy, HandleClicked));

    Row.Description = FMythicUIStyle::MakeText(this, EMythicTextRole::Subtle);
    Row.Description->SetAutoWrapText(true);
    Row.Description->SetVisibility(ESlateVisibility::HitTestInvisible);
    Row.Description->SetRenderOpacity(0.0f);
    if (UVerticalBoxSlot *S = Cast<UVerticalBoxSlot>(Outer->AddChild(Row.Description))) {
        S->SetPadding(FMargin(4.0f, 0.0f, 64.0f, 6.0f));
    }

    Outer->SetVisibility(ESlateVisibility::Collapsed);
    SettingsList->AddChild(Outer);

    RowPool.Add(Row);
    return RowPool.Last();
}

TArray<FText> UMythicSettingsPageWidget::GetCategoryNames() const {
    TArray<FText> Names;
    for (const FSettingDef &Def : Definitions) {
        if (Def.bHeading) {
            Names.Add(Def.Label);
        }
    }
    return Names;
}

bool UMythicSettingsPageWidget::IsRowVisible(bool bIsHeading, const FText &RowCategory, int32 ActiveCategoryIndex,
                                             const TArray<FText> &Categories) {
    if (!Categories.IsValidIndex(ActiveCategoryIndex)) {
        return true;
    }
    // The tab already names the category, so its heading row would be the same label twice.
    return !bIsHeading && RowCategory.EqualTo(Categories[ActiveCategoryIndex]);
}

void UMythicSettingsPageWidget::SetActiveCategory(int32 CategoryIndex) {
    ActiveCategory = CategoryIndex;
    Refresh();
}

void UMythicSettingsPageWidget::Refresh() {
    if (!SettingsList) {
        return;
    }

    const TArray<FText> Categories = GetCategoryNames();

    for (int32 i = 0; i < Definitions.Num(); ++i) {
        FMythicSettingsRow &Row = GetOrCreateRow(i);
        const FSettingDef &Def = Definitions[i];

        if (!IsRowVisible(Def.bHeading, Def.Category, ActiveCategory, Categories)) {
            Row.Box->SetVisibility(ESlateVisibility::Collapsed);
            continue;
        }

        Row.Box->SetVisibility(ESlateVisibility::Visible);
        Row.Label->SetText(Def.Label);

        UPanelWidget *Line = Cast<UPanelWidget>(Row.Box->GetChildAt(0));
        UWidget *RuleWidget = Line ? Line->GetChildAt(1) : nullptr;
        UHorizontalBoxSlot *LabelSlot = Cast<UHorizontalBoxSlot>(Row.Label->Slot);

        if (Row.Description) {
            Row.Description->SetText(Def.Description);
            if (Def.bHeading || Def.Description.IsEmpty()) {
                Row.Description->SetRenderOpacity(0.0f);
            }
        }

        if (Def.bHeading) {
            FMythicUIStyle::ApplyTextStyle(Row.Label, EMythicTextRole::Heading);
            Row.Label->SetColorAndOpacity(FSlateColor(FMythicUIStyle::Get().InkSubtle));
            Row.Left->SetVisibility(ESlateVisibility::Collapsed);
            Row.Right->SetVisibility(ESlateVisibility::Collapsed);
            Row.Value->SetVisibility(ESlateVisibility::Collapsed);
            if (RuleWidget) {
                RuleWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
            }
            if (LabelSlot && LabelSlot->GetSize().SizeRule != ESlateSizeRule::Automatic) {
                LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            }
            continue;
        }

        FMythicUIStyle::ApplyTextStyle(Row.Label, EMythicTextRole::Body);
        Row.Label->SetColorAndOpacity(FSlateColor(FMythicUIStyle::Get().Ink));
        Row.Left->SetVisibility(ESlateVisibility::Visible);
        Row.Right->SetVisibility(ESlateVisibility::Visible);
        Row.Value->SetVisibility(ESlateVisibility::HitTestInvisible);
        Row.Value->SetText(Def.Read ? Def.Read() : FText::GetEmpty());
        if (RuleWidget) {
            RuleWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
        if (LabelSlot && LabelSlot->GetSize().SizeRule != ESlateSizeRule::Fill) {
            LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }
    }

    for (int32 i = Definitions.Num(); i < RowPool.Num(); ++i) {
        RowPool[i].Box->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (Txt_Status) {
        Txt_Status->SetText(bPendingApply
                                ? NSLOCTEXT("Mythic", "SettingsPending", "Changes are waiting on Apply.")
                                : NSLOCTEXT("Mythic", "SettingsClean", "All changes saved."));
        Txt_Status->SetColorAndOpacity(FSlateColor(bPendingApply ? FMythicUIStyle::Get().Caution
                                                                 : FMythicUIStyle::Get().InkSubtle));
    }
}


void UMythicSettingsPageWidget::StepSetting(int32 RowIndex, int32 Delta) {
    if (!Definitions.IsValidIndex(RowIndex)) {
        return;
    }
    const FSettingDef &Def = Definitions[RowIndex];
    if (Def.bHeading || !Def.Step) {
        return;
    }
    Def.Step(Delta);
    if (Def.bNeedsApply) {
        bPendingApply = true;
    }
    Refresh();
}

void UMythicSettingsPageWidget::ApplyAndSave() {
    if (UMythicUserSettings *S = UMythicUserSettings::Get()) {
        S->ApplySettings(false);
        S->SaveSettings();
    }
    bPendingApply = false;
    Refresh();
}

void UMythicSettingsPageWidget::RestoreDefaults() {
    if (UMythicUserSettings *S = UMythicUserSettings::Get()) {
        S->SetToDefaults();
        S->ApplySettings(false);
        S->SaveSettings();
    }
    bPendingApply = false;
    Refresh();
}

void UMythicSettingsPageWidget::HandleApplyClicked() {
    ApplyAndSave();
}

void UMythicSettingsPageWidget::HandleDefaultsClicked() {
    RestoreDefaults();
}
