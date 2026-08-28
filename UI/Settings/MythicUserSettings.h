// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "MythicUserSettings.generated.h"

UCLASS(Config = GameUserSettings, ConfigDoNotCheckDefaults)
class MYTHIC_API UMythicUserSettings : public UGameUserSettings {
    GENERATED_BODY()

public:
    UMythicUserSettings();

    /** Convenience accessor, so callers do not have to cast the engine singleton themselves. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    static UMythicUserSettings *Get();

    /** Returns the master audio volume in the normalized 0..1 range. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    float GetMasterVolume() const { return MasterVolume; }

    /** Set and apply immediately — a volume slider that only takes effect on Apply feels broken. Clamped to [0,1]. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetMasterVolume(float NewVolume);

    /** Invert the vertical look axis. Read by the camera input path. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    bool GetInvertLookY() const { return bInvertLookY; }

    /** Enables or disables inversion of the vertical look axis. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetInvertLookY(bool bInvert);

    /** Returns the legacy shared look-sensitivity multiplier. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    float GetLookSensitivity() const { return LookSensitivity; }

    /** Clamped to [0.1, 3.0] — a sensitivity of zero would look like a broken controller, not a setting. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetLookSensitivity(float NewSensitivity);


    /** 0 None, 1 FXAA, 2 TAA, 3 MSAA, 4 TSR. Matches r.AntiAliasingMethod exactly. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    int32 GetAntiAliasingMethod() const { return AntiAliasingMethod; }

    /** Sets and applies the renderer anti-aliasing method. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetAntiAliasingMethod(int32 Method);

    /** Post-tonemap sharpening, 0..2. Temporal AA softens; this is how a player gets the edge back. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    float GetSharpness() const { return Sharpness; }

    /** Sets post-tonemap sharpening, clamped to the supported range. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetSharpness(float NewSharpness);


    /**
     * Hold the whole HUD at full strength, permanently.
     *
     * The contextual HUD hides what it thinks you do not need. That is right for most players and wrong for anyone
     * who needs their vitals to simply be there — low vision, cognitive load, or just preference. This is the
     * off switch, and UMythicHUDLayout treats it exactly like the reveal key being held down forever.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    bool GetAlwaysShowHUD() const { return bAlwaysShowHUD; }

    /** Keeps the contextual HUD fully visible when enabled. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetAlwaysShowHUD(bool bAlways);

    DECLARE_MULTICAST_DELEGATE(FMythicAccessibilityChanged);
    FMythicAccessibilityChanged OnAccessibilityChanged;


    /**
     * Screen gamma. It lives on UEngine as a config property of its own, so we keep a copy and re-push it on load --
     * otherwise it reverts to 2.2 every boot and reads as "brightness does not save".
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    float GetDisplayGamma() const { return DisplayGamma; }

    /** Sets and immediately applies the display gamma. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetDisplayGamma(float NewGamma);

    /** Frames per second while the window is not focused. 0 = no separate limit. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    float GetBackgroundFrameRateLimit() const { return BackgroundFrameRateLimit; }

    /** Sets the frame-rate limit used while the application is unfocused; zero disables it. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetBackgroundFrameRateLimit(float NewLimit);

    void ApplyFrameRateLimit();


    /** 0 off .. 4 very high. Off by default: motion blur is the commonest motion-sickness trigger. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    int32 GetMotionBlurQuality() const { return MotionBlurQuality; }

    /** Sets motion-blur quality from off through very high. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetMotionBlurQuality(int32 Quality);

    /** Returns whether bloom is enabled. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    bool GetBloom() const { return bBloom; }

    /** Enables or disables bloom. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetBloom(bool bEnabled);

    /** Texture filtering, 1..16. Both r.MaxAnisotropy and its virtual-texture twin are pushed. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    int32 GetMaxAnisotropy() const { return MaxAnisotropy; }

    /** Sets the anisotropic-filtering sample count. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetMaxAnisotropy(int32 Samples);


    /**
     * Ambient occlusion: 0 off, 1 SSAO, 2 GTAO. GTAO is the newer ground-truth solver - it costs more and is
     * markedly better in interiors and under foliage, which is most of this game.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    int32 GetAmbientOcclusionMode() const { return AmbientOcclusionMode; }

    /** Sets the ambient-occlusion implementation. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetAmbientOcclusionMode(int32 Mode);

    /**
     * How indirect light is traced: 0 software ray tracing, 1 hardware ray tracing, 2 screen space, 3 none.
     *
     * Software and hardware are both Lumen - the difference is whether the traces run against a distance-field
     * approximation of the scene or against the real geometry on RT cores. That is why they are two entries in
     * one list rather than a separate toggle: a player picks how their light is traced, once.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    int32 GetGlobalIlluminationMethod() const { return GlobalIlluminationMethod; }

    /** Sets the global-illumination tracing method. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetGlobalIlluminationMethod(int32 Method);

    /** Reflections: 0 none, 1 Lumen, 2 screen space. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    int32 GetReflectionMethod() const { return ReflectionMethod; }

    /** Sets the reflection method. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetReflectionMethod(int32 Method);

    /** Returns whether virtual shadow maps are enabled. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    bool GetVirtualShadowMaps() const { return bVirtualShadowMaps; }

    /** Enables or disables virtual shadow maps. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetVirtualShadowMaps(bool bEnabled);

    /** Returns whether Nanite rendering is enabled. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    bool GetNanite() const { return bNanite; }

    /** Enables or disables Nanite rendering. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetNanite(bool bEnabled);

    /** True on an NVIDIA card. Gates the vendor-specific rows so they grey out rather than lying elsewhere. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    static bool IsNvidiaGpu();

    /** True when Reflex is present AND the driver supports it, so the row can grey out on its own terms. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    static bool IsReflexAvailable();

    /** NVIDIA Reflex low latency: 0 off, 1 on, 2 on plus boost. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    int32 GetReflexMode() const { return ReflexMode; }

    /** Sets NVIDIA Reflex to off, on, or on with boost. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetReflexMode(int32 Mode);

    /** True when the DLSS plugin is compiled in AND this card and driver can actually run it. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    static bool IsDLSSAvailable();

    /** Returns whether DLSS frame generation is available on this machine. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    static bool IsFrameGenerationAvailable();

    /** Returns whether DLSS ray reconstruction is available on this machine. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    static bool IsRayReconstructionAvailable();

    /** True when this machine can actually trace against real geometry. Gates the hardware option away. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    static bool IsHardwareRayTracingAvailable();

    /** 0 off, 1 DLAA, 2 Quality, 3 Balanced, 4 Performance, 5 Ultra Performance. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    int32 GetDLSSMode() const { return DLSSMode; }

    /** Sets the DLSS quality mode. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetDLSSMode(int32 Mode);

    /** 0 off, 1 auto, 2 two frames, 3 three, 4 four. Multi-frame needs a 50-series card. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    int32 GetFrameGenerationMode() const { return FrameGenerationMode; }

    /** Sets the DLSS frame-generation mode. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetFrameGenerationMode(int32 Mode);

    /** Returns whether DLSS ray reconstruction is enabled. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    bool GetRayReconstruction() const { return bRayReconstruction; }

    /** Enables or disables DLSS ray reconstruction. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetRayReconstruction(bool bEnabled);


    /** Returns whether audio is muted while the application is unfocused. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    bool GetMuteWhenUnfocused() const { return bMuteWhenUnfocused; }

    /** Enables or disables muting while the application is unfocused. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetMuteWhenUnfocused(bool bMute);


    /** Returns the mouse look-sensitivity multiplier. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    float GetMouseLookSensitivity() const { return MouseLookSensitivity; }

    /** Sets the mouse look-sensitivity multiplier. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetMouseLookSensitivity(float V);

    /** Returns the gamepad look-sensitivity multiplier. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    float GetGamepadLookSensitivity() const { return GamepadLookSensitivity; }

    /** Sets the gamepad look-sensitivity multiplier. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetGamepadLookSensitivity(float V);

    /** Multiplier on the vertical axis only, so a player can slow Y without slowing the turn. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    float GetVerticalLookScale() const { return VerticalLookScale; }

    /** Sets the vertical-axis look multiplier. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetVerticalLookScale(float V);

    /** Returns whether the horizontal look axis is inverted. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    bool GetInvertLookX() const { return bInvertLookX; }

    /** Enables or disables inversion of the horizontal look axis. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetInvertLookX(bool bInvert);

    /** Returns the left-stick deadzone in the normalized 0..1 range. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    float GetGamepadDeadzoneLeft() const { return GamepadDeadzoneLeft; }

    /** Sets the left-stick deadzone. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetGamepadDeadzoneLeft(float V);

    /** Returns the right-stick deadzone in the normalized 0..1 range. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    float GetGamepadDeadzoneRight() const { return GamepadDeadzoneRight; }

    /** Sets the right-stick deadzone. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetGamepadDeadzoneRight(float V);

    /** Returns the gamepad-vibration strength in the normalized 0..1 range. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    float GetVibrationScale() const { return VibrationScale; }

    /** Sets and applies the gamepad-vibration strength. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetVibrationScale(float Scale);

    void ApplyVibration() const;


    /** Returns the user-interface scale multiplier. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    float GetUIScale() const { return UIScale; }

    /** Sets and applies the user-interface scale multiplier. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetUIScale(float Scale);

    /** Returns the global HUD opacity in the normalized 0..1 range. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    float GetHUDOpacity() const { return HUDOpacity; }

    /** Sets and applies the global HUD opacity. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetHUDOpacity(float Opacity);

    /** Returns combat-number visibility: 0 off, 1 damage you deal, 2 damage you deal and receive. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    uint8 GetDamageNumberMode() const { return DamageNumberMode; }

    /** Sets combat-number visibility: 0 off, 1 damage you deal, 2 damage you deal and receive. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetDamageNumberMode(uint8 Mode);

    /** Returns the accessibility scale applied to every combat number. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    float GetDamageNumberScale() const { return DamageNumberScale; }

    /** Sets the accessibility scale applied to every combat number, clamped to the supported UI range. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetDamageNumberScale(float Scale);

    /**
     * The quality preset the eleven scalability groups agree on, or -1 for Custom.
     *
     * NOT GetOverallScalabilityLevel(): that also demands ResolutionQuality match a fixed table, and our
     * render-scale restore guarantees it does not -- so the preset row would read "Custom" the moment it was set.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    int32 GetQualityPresetLevel() const;

    DECLARE_MULTICAST_DELEGATE(FMythicInputSettingsChanged);
    FMythicInputSettingsChanged OnInputSettingsChanged;

    DECLARE_MULTICAST_DELEGATE(FMythicInterfaceChanged);
    FMythicInterfaceChanged OnInterfaceChanged;

    virtual void ApplySettings(bool bCheckForCommandLineOverrides) override;
    virtual void ApplyNonResolutionSettings() override;
    virtual void SetToDefaults() override;

    virtual void SetOverallScalabilityLevel(int32 Value) override;

    virtual float GetEffectiveFrameRateLimit() override;

    /** Screen percentage, 25-100. Kept separate from the quality preset on purpose. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Settings")
    void SetRenderScale(float Percent);

    /** Returns the configured render scale as a percentage. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Settings")
    float GetRenderScale() const;

private:
    void ApplyMasterVolume() const;

    UPROPERTY(Config)
    float MasterVolume = 1.0f;

    UPROPERTY(Config)
    bool bInvertLookY = false;

    UPROPERTY(Config)
    float LookSensitivity = 1.0f;

    void ApplyImageSettings() const;

    /** Pushes the renderer-feature cvars. Separate from image settings because these are the expensive ones. */
    void ApplyRenderingSettings() const;

    void ApplyReflex() const;

    void ApplyDLSS() const;

    UPROPERTY(Config)
    int32 AntiAliasingMethod = 4; // TSR: the engine default for UE5 and the right one for an open world

    UPROPERTY(Config)
    float Sharpness = 0.4f;

    UPROPERTY(Config)
    bool bAlwaysShowHUD = false;

    // ---- Display ----
    UPROPERTY(Config)
    float DisplayGamma = 2.2f;

    UPROPERTY(Config)
    float BackgroundFrameRateLimit = 30.0f;

    void ApplyDisplayGamma() const;

    // ---- Image ----
    UPROPERTY(Config)
    int32 MotionBlurQuality = 0;

    UPROPERTY(Config)
    bool bBloom = true;

    UPROPERTY(Config)
    int32 AmbientOcclusionMode = 2;

    UPROPERTY(Config)
    int32 GlobalIlluminationMethod = 0;

    UPROPERTY(Config)
    int32 ReflectionMethod = 1;

    UPROPERTY(Config)
    bool bVirtualShadowMaps = true;

    UPROPERTY(Config)
    bool bNanite = true;

    UPROPERTY(Config)
    int32 ReflexMode = 1;

    UPROPERTY(Config)
    int32 DLSSMode = 0;

    UPROPERTY(Config)
    int32 FrameGenerationMode = 0;

    UPROPERTY(Config)
    bool bRayReconstruction = false;

    UPROPERTY(Config)
    int32 MaxAnisotropy = 8;

    // ---- Audio ----
    UPROPERTY(Config)
    bool bMuteWhenUnfocused = true;

    void ApplyAudioSettings() const;

    // ---- Controls ----
    UPROPERTY(Config)
    float MouseLookSensitivity = 1.0f;

    UPROPERTY(Config)
    float GamepadLookSensitivity = 1.0f;

    UPROPERTY(Config)
    float VerticalLookScale = 1.0f;

    UPROPERTY(Config)
    bool bInvertLookX = false;

    UPROPERTY(Config)
    float GamepadDeadzoneLeft = 0.15f;

    UPROPERTY(Config)
    float GamepadDeadzoneRight = 0.15f;

    UPROPERTY(Config)
    float VibrationScale = 1.0f;

    // ---- Interface ----
    UPROPERTY(Config)
    float UIScale = 1.0f;

    UPROPERTY(Config)
    float HUDOpacity = 1.0f;

    UPROPERTY(Config)
    uint8 DamageNumberMode = 1;

    UPROPERTY(Config)
    float DamageNumberScale = 1.0f;

    void ApplyInterfaceSettings() const;

    static void PushCVar(const TCHAR *Name, int32 Value);
    static void PushCVar(const TCHAR *Name, float Value);
};
