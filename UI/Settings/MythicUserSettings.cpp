// Copyright Stellar Games. All Rights Reserved.

#include "MythicUserSettings.h"

#include "AudioDevice.h"
#include "Engine/Engine.h"
#include "Engine/UserInterfaceSettings.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Misc/App.h"
#include "Scalability.h"

UMythicUserSettings::UMythicUserSettings() {}

UMythicUserSettings *UMythicUserSettings::Get() {
    return GEngine ? Cast<UMythicUserSettings>(GEngine->GetGameUserSettings()) : nullptr;
}

void UMythicUserSettings::SetMasterVolume(float NewVolume) {
    MasterVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);
    ApplyMasterVolume();
}

void UMythicUserSettings::SetInvertLookY(bool bInvert) {
    bInvertLookY = bInvert;
}

void UMythicUserSettings::SetLookSensitivity(float NewSensitivity) {
    LookSensitivity = FMath::Clamp(NewSensitivity, 0.1f, 3.0f);
}

void UMythicUserSettings::SetAntiAliasingMethod(int32 Method) {
    AntiAliasingMethod = FMath::Clamp(Method, 0, 5);
    ApplyImageSettings();
    ApplyRenderingSettings();
}

void UMythicUserSettings::SetSharpness(float NewSharpness) {
    Sharpness = NewSharpness < 0.0f ? -1.0f : FMath::Clamp(NewSharpness, 0.0f, 1.0f);
    ApplyImageSettings();
    ApplyRenderingSettings();
}

void UMythicUserSettings::SetAlwaysShowHUD(bool bAlways) {
    if (bAlwaysShowHUD == bAlways) {
        return;
    }
    bAlwaysShowHUD = bAlways;
    OnAccessibilityChanged.Broadcast();
}

void UMythicUserSettings::ApplyImageSettings() const {
    PushCVar(TEXT("r.AntiAliasingMethod"), AntiAliasingMethod == 3 ? 4 : AntiAliasingMethod);
    PushCVar(TEXT("r.Tonemapper.Sharpen"), Sharpness);
    PushCVar(TEXT("r.MotionBlurQuality"), MotionBlurQuality);
    PushCVar(TEXT("r.BloomQuality"), bBloom ? 5 : 0);
    PushCVar(TEXT("r.MaxAnisotropy"), MaxAnisotropy);
    PushCVar(TEXT("r.VT.MaxAnisotropy"), MaxAnisotropy);
}

void UMythicUserSettings::ApplyRenderingSettings() const {
    // 0 off, 1 SSAO, 2 GTAO. Levels is the actual off switch; the method only picks the solver.
    PushCVar(TEXT("r.AmbientOcclusionLevels"), AmbientOcclusionMode == 0 ? 0 : -1);
    PushCVar(TEXT("r.AmbientOcclusion.Method"), AmbientOcclusionMode == 2 ? 1 : 0);
    if (AmbientOcclusionMode == 2) {
        // The owner's authored GTAO profile. Normals on and four angles is the quality/cost point they chose;
        // the spatial filter is off deliberately, so do not "fix" it back on.
        PushCVar(TEXT("r.GTAO.UseNormals"), 1);
        PushCVar(TEXT("r.GTAO.NumAngles"), 4);
        PushCVar(TEXT("r.GTAO.SpatialFilter"), 0);
    }

    PushCVar(TEXT("r.DynamicGlobalIlluminationMethod"), GlobalIlluminationMethod);
    PushCVar(TEXT("r.ReflectionMethod"), ReflectionMethod);
    PushCVar(TEXT("r.Shadow.Virtual.Enable"), bVirtualShadowMaps ? 1 : 0);
    PushCVar(TEXT("r.Nanite"), bNanite ? 1 : 0);
}

void UMythicUserSettings::SetAmbientOcclusionMode(int32 Mode) {
    AmbientOcclusionMode = FMath::Clamp(Mode, 0, 2);
    ApplyRenderingSettings();
}

void UMythicUserSettings::SetGlobalIlluminationMethod(int32 Method) {
    GlobalIlluminationMethod = FMath::Clamp(Method, 0, 2);
    ApplyRenderingSettings();
}

void UMythicUserSettings::SetReflectionMethod(int32 Method) {
    ReflectionMethod = FMath::Clamp(Method, 0, 2);
    ApplyRenderingSettings();
}

void UMythicUserSettings::SetVirtualShadowMaps(bool bEnabled) {
    bVirtualShadowMaps = bEnabled;
    ApplyRenderingSettings();
}

void UMythicUserSettings::SetNanite(bool bEnabled) {
    bNanite = bEnabled;
    ApplyRenderingSettings();
}

void UMythicUserSettings::ApplyMasterVolume() const {
    if (!GEngine) {
        return;
    }
    if (FAudioDeviceHandle Device = GEngine->GetMainAudioDevice()) {
        Device->SetTransientPrimaryVolume(MasterVolume);
    }
}


void UMythicUserSettings::PushCVar(const TCHAR *Name, int32 Value) {
    if (IConsoleVariable *CVar = IConsoleManager::Get().FindConsoleVariable(Name)) {
        CVar->Set(Value, ECVF_SetByGameOverride);
    }
}

void UMythicUserSettings::PushCVar(const TCHAR *Name, float Value) {
    if (IConsoleVariable *CVar = IConsoleManager::Get().FindConsoleVariable(Name)) {
        CVar->Set(Value, ECVF_SetByGameOverride);
    }
}


void UMythicUserSettings::SetDisplayGamma(float NewGamma) {
    DisplayGamma = FMath::Clamp(NewGamma, 0.5f, 5.0f);
    ApplyDisplayGamma();
}

void UMythicUserSettings::ApplyDisplayGamma() const {
    if (GEngine) {
        GEngine->DisplayGamma = DisplayGamma;
    }
}

void UMythicUserSettings::SetBackgroundFrameRateLimit(float NewLimit) {
    BackgroundFrameRateLimit = FMath::Clamp(NewLimit, 0.0f, 240.0f);
    ApplyFrameRateLimit();
}

float UMythicUserSettings::GetEffectiveFrameRateLimit() {
    if (BackgroundFrameRateLimit > 0.0f && FApp::IsGame() && !FApp::HasFocus()) {
        const float Foreground = Super::GetEffectiveFrameRateLimit();
        return Foreground > 0.0f ? FMath::Min(Foreground, BackgroundFrameRateLimit) : BackgroundFrameRateLimit;
    }
    return Super::GetEffectiveFrameRateLimit();
}

void UMythicUserSettings::ApplyFrameRateLimit() {
    UGameUserSettings::SetFrameRateLimitCVar(GetEffectiveFrameRateLimit());
}


void UMythicUserSettings::SetMotionBlurQuality(int32 Quality) {
    MotionBlurQuality = FMath::Clamp(Quality, 0, 4);
    ApplyImageSettings();
}

void UMythicUserSettings::SetBloom(bool bEnabled) {
    bBloom = bEnabled;
    ApplyImageSettings();
}

void UMythicUserSettings::SetMaxAnisotropy(int32 Samples) {
    MaxAnisotropy = FMath::Clamp(Samples, 1, 16);
    ApplyImageSettings();
}


void UMythicUserSettings::SetMuteWhenUnfocused(bool bMute) {
    bMuteWhenUnfocused = bMute;
    ApplyAudioSettings();
}

void UMythicUserSettings::ApplyAudioSettings() const {
    ApplyMasterVolume();
    FApp::SetUnfocusedVolumeMultiplier(bMuteWhenUnfocused ? 0.0f : 1.0f);
}


void UMythicUserSettings::SetMouseLookSensitivity(float V) {
    MouseLookSensitivity = FMath::Clamp(V, 0.1f, 3.0f);
    OnInputSettingsChanged.Broadcast();
}

void UMythicUserSettings::SetGamepadLookSensitivity(float V) {
    GamepadLookSensitivity = FMath::Clamp(V, 0.1f, 3.0f);
    OnInputSettingsChanged.Broadcast();
}

void UMythicUserSettings::SetVerticalLookScale(float V) {
    VerticalLookScale = FMath::Clamp(V, 0.5f, 1.5f);
    OnInputSettingsChanged.Broadcast();
}

void UMythicUserSettings::SetInvertLookX(bool bInvert) {
    bInvertLookX = bInvert;
    OnInputSettingsChanged.Broadcast();
}

void UMythicUserSettings::SetGamepadDeadzoneLeft(float V) {
    GamepadDeadzoneLeft = FMath::Clamp(V, 0.0f, 0.4f);
    OnInputSettingsChanged.Broadcast();
}

void UMythicUserSettings::SetGamepadDeadzoneRight(float V) {
    GamepadDeadzoneRight = FMath::Clamp(V, 0.0f, 0.4f);
    OnInputSettingsChanged.Broadcast();
}

void UMythicUserSettings::SetVibrationScale(float Scale) {
    VibrationScale = FMath::Clamp(Scale, 0.0f, 1.0f);
    ApplyVibration();
}

void UMythicUserSettings::ApplyVibration() const {
    if (!GEngine) {
        return;
    }
    for (const FWorldContext &Ctx : GEngine->GetWorldContexts()) {
        if (const UWorld *World = Ctx.World()) {
            if (APlayerController *PC = World->GetFirstPlayerController()) {
                PC->bForceFeedbackEnabled = VibrationScale > 0.0f;
                PC->ForceFeedbackScale = VibrationScale;
            }
        }
    }
}


void UMythicUserSettings::SetUIScale(float Scale) {
    UIScale = FMath::Clamp(Scale, 0.8f, 1.25f);
    ApplyInterfaceSettings();
}

void UMythicUserSettings::ApplyInterfaceSettings() const {
    GetMutableDefault<UUserInterfaceSettings>()->ApplicationScale = UIScale;
    OnInterfaceChanged.Broadcast();
}

void UMythicUserSettings::SetHUDOpacity(float Opacity) {
    HUDOpacity = FMath::Clamp(Opacity, 0.4f, 1.0f);
    OnAccessibilityChanged.Broadcast();
}

void UMythicUserSettings::SetDamageNumberMode(uint8 Mode) {
    DamageNumberMode = FMath::Min<uint8>(Mode, 2);
    OnInterfaceChanged.Broadcast();
}

void UMythicUserSettings::SetDamageNumberScale(float Scale) {
    DamageNumberScale = FMath::Clamp(Scale, 0.75f, 1.5f);
    OnInterfaceChanged.Broadcast();
}

int32 UMythicUserSettings::GetQualityPresetLevel() const {
    const Scalability::FQualityLevels &Q = ScalabilityQuality;
    const int32 Level = Q.ViewDistanceQuality;
    const int32 Groups[] = {
        Q.AntiAliasingQuality, Q.ShadowQuality, Q.GlobalIlluminationQuality, Q.ReflectionQuality,
        Q.PostProcessQuality, Q.TextureQuality, Q.EffectsQuality, Q.FoliageQuality, Q.ShadingQuality,
    };
    for (const int32 G : Groups) {
        if (G != Level) {
            return -1;
        }
    }
    return Level;
}

void UMythicUserSettings::SetOverallScalabilityLevel(int32 Value) {
    const float KeptRenderScale = GetRenderScale();
    Super::SetOverallScalabilityLevel(Value);
    ScalabilityQuality.ResolutionQuality = KeptRenderScale;
}

void UMythicUserSettings::SetRenderScale(float Percent) {
    ScalabilityQuality.ResolutionQuality = FMath::Clamp(Percent, 50.0f, 100.0f);
}

float UMythicUserSettings::GetRenderScale() const {
    const float Current = ScalabilityQuality.ResolutionQuality;
    return Current > 0.0f ? FMath::Clamp(Current, 50.0f, 100.0f) : 100.0f;
}

void UMythicUserSettings::ApplySettings(bool bCheckForCommandLineOverrides) {
    ScalabilityQuality.ResolutionQuality = GetRenderScale();
    Super::ApplySettings(bCheckForCommandLineOverrides);
    ApplyMasterVolume();
    ApplyAudioSettings();
    ApplyImageSettings();
    ApplyDisplayGamma();
    ApplyInterfaceSettings();
    ApplyVibration();
    ApplyFrameRateLimit();
}

void UMythicUserSettings::SetToDefaults() {
    Super::SetToDefaults();
    MasterVolume = 1.0f;
    bInvertLookY = false;
    LookSensitivity = 1.0f;
    AntiAliasingMethod = 4;
    Sharpness = 0.4f;
    bAlwaysShowHUD = false;
    DisplayGamma = 2.2f;
    BackgroundFrameRateLimit = 30.0f;
    MotionBlurQuality = 0;
    bBloom = true;
    MaxAnisotropy = 8;
    bMuteWhenUnfocused = true;
    MouseLookSensitivity = 1.0f;
    GamepadLookSensitivity = 1.0f;
    VerticalLookScale = 1.0f;
    bInvertLookX = false;
    GamepadDeadzoneLeft = 0.15f;
    GamepadDeadzoneRight = 0.15f;
    VibrationScale = 1.0f;
    UIScale = 1.0f;
    HUDOpacity = 1.0f;
    DamageNumberMode = 1;
    DamageNumberScale = 1.0f;
    ScalabilityQuality.ResolutionQuality = 100.0f;
}
