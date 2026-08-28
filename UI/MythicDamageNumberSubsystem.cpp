// Copyright Stellar Games. All Rights Reserved.

#include "MythicDamageNumberSubsystem.h"
#include "UI/Settings/MythicUserSettings.h"
#include "GAS/Effects/MythicStatusEffectDefinition.h"
#include "GAS/Effects/MythicStatusRegistry.h"
#include "Settings/MythicDeveloperSettings.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogMythicDamageNumbers, Log, All);

namespace {
constexpr int32 DefaultActiveNumberBudget = 256;
constexpr int32 MaximumActiveNumberBudget = 2048;
constexpr float MinimumNumberLifetime = 0.05f;

bool IsFiniteVector(const FVector &Value) {
    return FMath::IsFinite(Value.X) && FMath::IsFinite(Value.Y) && FMath::IsFinite(Value.Z);
}

bool IsFiniteColor(const FLinearColor &Value) {
    return FMath::IsFinite(Value.R) && FMath::IsFinite(Value.G)
        && FMath::IsFinite(Value.B) && FMath::IsFinite(Value.A);
}

float ResolveDefaultLifetime(const UMythicDamageNumberConfig *Config) {
    const float AuthoredLifetime = Config && FMath::IsFinite(Config->DefaultLifetime)
                                       ? Config->DefaultLifetime
                                       : 1.0f;
    return FMath::Max(MinimumNumberLifetime, AuthoredLifetime);
}

int32 ResolveActiveNumberBudget(const UMythicDamageNumberConfig *Config) {
    const int32 AuthoredBudget = Config && Config->MaxActiveNumbers > 0
                                     ? Config->MaxActiveNumbers
                                     : DefaultActiveNumberBudget;
    return FMath::Clamp(AuthoredBudget, 1, MaximumActiveNumberBudget);
}

FLinearColor ResolveColor(const FLinearColor &AuthoredColor, const FLinearColor &FallbackColor) {
    return IsFiniteColor(AuthoredColor) ? AuthoredColor : FallbackColor;
}
}

void UMythicDamageNumberSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);

    if (const UMythicDeveloperSettings *DevSettings = GetDefault<UMythicDeveloperSettings>()) {
        Config = DevSettings->DamageNumberConfig.LoadSynchronous();
        if (Config) {
            UE_LOG(LogMythicDamageNumbers, Log, TEXT("Loaded DamageNumberConfig: %s"), *Config->GetName());
        }

        if (!DevSettings->StatusEffectLibrary.IsNull()) {
            LoadedStatusEffectLibrary = DevSettings->StatusEffectLibrary.LoadSynchronous();
            if (!LoadedStatusEffectLibrary) {
                UE_LOG(LogMythicDamageNumbers, Error, TEXT("Failed to preload StatusEffectLibrary: %s"),
                       *DevSettings->StatusEffectLibrary.ToString());
            }
        }
    }

    ActiveDamageNumbers.Reserve(ResolveActiveNumberBudget(Config));

    HUDDrawDelegateHandle = AHUD::OnHUDPostRender.AddUObject(this, &UMythicDamageNumberSubsystem::OnHUDPostRender);

    UE_LOG(LogMythicDamageNumbers, Log, TEXT("DamageNumberSubsystem initialized"));
}

void UMythicDamageNumberSubsystem::Deinitialize() {
    AHUD::OnHUDPostRender.Remove(HUDDrawDelegateHandle);
    ActiveDamageNumbers.Empty();
    LoadedStatusEffectLibrary = nullptr;

    Super::Deinitialize();
}

bool UMythicDamageNumberSubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    if (const UWorld *World = Cast<UWorld>(Outer)) {
        return World->IsGameWorld() && World->GetNetMode() != NM_DedicatedServer;
    }
    return false;
}

bool UMythicDamageNumberSubsystem::ShouldPresentResolvedEvent(const uint8 DamageNumberMode,
                                                              const bool bOutgoingForViewer) {
    switch (DamageNumberMode) {
    case 0:
        return false;
    case 1:
        return bOutgoingForViewer;
    case 2:
        return true;
    default:
        return false;
    }
}

void UMythicDamageNumberSubsystem::OnHUDPostRender(AHUD *HUD, UCanvas *Canvas) {
    if (!HUD || !Canvas) {
        return;
    }

    if (HUD->GetWorld() != GetWorld()) {
        return;
    }

    APlayerController *PC = HUD->GetOwningPlayerController();
    if (!PC) {
        return;
    }

    DrawDamageNumbers(Canvas, PC);
}

void UMythicDamageNumberSubsystem::CleanupExpired() {
    const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    for (int32 i = ActiveDamageNumbers.Num() - 1; i >= 0; --i) {
        if (ActiveDamageNumbers[i].IsExpired(CurrentTime)) {
            ActiveDamageNumbers.RemoveAtSwap(i, EAllowShrinking::No);
        }
    }
}

void UMythicDamageNumberSubsystem::EnforceActiveBudget(const int32 PendingEntries) {
    const int32 Budget = ResolveActiveNumberBudget(Config);
    const int32 SafePendingEntries = FMath::Max(0, PendingEntries);
    while (ActiveDamageNumbers.Num() + SafePendingEntries > Budget && !ActiveDamageNumbers.IsEmpty()) {
        int32 OldestIndex = 0;
        for (int32 Index = 1; Index < ActiveDamageNumbers.Num(); ++Index) {
            const FMythicDamageNumberData &Candidate = ActiveDamageNumbers[Index];
            const FMythicDamageNumberData &Oldest = ActiveDamageNumbers[OldestIndex];
            if (Candidate.SpawnTime < Oldest.SpawnTime
                || (Candidate.SpawnTime == Oldest.SpawnTime && Candidate.ID < Oldest.ID)) {
                OldestIndex = Index;
            }
        }
        ActiveDamageNumbers.RemoveAtSwap(OldestIndex, EAllowShrinking::No);
    }
}

void UMythicDamageNumberSubsystem::ApplyRandomMotion(FMythicDamageNumberData &Data) const {
    if (!Config) {
        return;
    }
    const float HorizontalRange = FMath::IsFinite(Config->RandomHorizontalOffsetRange)
                                      ? FMath::Max(0.0f, Config->RandomHorizontalOffsetRange)
                                      : 0.0f;
    const float VerticalRange = FMath::IsFinite(Config->RandomVerticalSpeedRange)
                                    ? FMath::Max(0.0f, Config->RandomVerticalSpeedRange)
                                    : 0.0f;
    Data.RandomOffsetX = FMath::RandRange(-HorizontalRange, HorizontalRange);
    Data.ExtraVerticalSpeed = FMath::RandRange(0.0f, VerticalRange);
}

FMythicDamageNumberData *UMythicDamageNumberSubsystem::FindMergeCandidate(
    const FMythicResolvedCombatTextEvent &Event,
    const float CurrentTime,
    const FVector &ResolvedWorldLocation) {
    const float MergeWindow = Config && FMath::IsFinite(Config->MergeWindowSeconds)
                                  ? FMath::Clamp(Config->MergeWindowSeconds, 0.0f, 0.25f)
                                  : 0.075f;
    if (MergeWindow <= 0.0f) {
        return nullptr;
    }

    AActor *const EventTarget = Event.TargetActor.Get();
    AActor *const EventSource = Event.SourceActor.Get();
    const TWeakObjectPtr<AActor> EventTargetKey(EventTarget);
    const TWeakObjectPtr<AActor> EventSourceKey(EventSource);
    UMythicStatusEffectDefinition *const EventStatus = Event.StatusDefinition.Get();
    for (int32 Index = ActiveDamageNumbers.Num() - 1; Index >= 0; --Index) {
        FMythicDamageNumberData &Candidate = ActiveDamageNumbers[Index];
        const float Age = CurrentTime - Candidate.SpawnTime;
        if (!Candidate.bResolvedEvent || Age < 0.0f || Age > MergeWindow
            || Candidate.TargetActor != EventTargetKey
            || Candidate.SourceActor != EventSourceKey
            || Candidate.StatusDefinition.Get() != EventStatus
            || Candidate.Origin != Event.Origin
            || Candidate.bOutgoingForViewer != Event.bOutgoingForViewer) {
            continue;
        }

        // Actor-less fallback events need a spatial key so unrelated hazards do not collapse into one number.
        if (!EventTarget && !Candidate.WorldLocation.Equals(ResolvedWorldLocation, 1.0f)) {
            continue;
        }
        return &Candidate;
    }
    return nullptr;
}

void UMythicDamageNumberSubsystem::AddResolvedCombatText(const FMythicResolvedCombatTextEvent &Event) {
    if (!FMath::IsFinite(Event.Magnitude) || Event.Magnitude <= 0.0f || !GetWorld()) {
        return;
    }
    if (const UMythicUserSettings *UserSettings = UMythicUserSettings::Get()) {
        if (!ShouldPresentResolvedEvent(UserSettings->GetDamageNumberMode(), Event.bOutgoingForViewer)) {
            return;
        }
    }

    const bool bStatusTick = Event.Origin == EMythicCombatTextOrigin::StatusTick;
    UMythicStatusEffectDefinition *const StatusDefinition = Event.StatusDefinition.Get();
    if (bStatusTick && !::IsValid(StatusDefinition)) {
        UE_LOG(LogMythicDamageNumbers, Warning,
               TEXT("Rejected a status-tick combat number without its canonical status definition"));
        return;
    }

    const FVector PresentationOffset = Config && IsFiniteVector(Config->WorldOffset)
                                           ? Config->WorldOffset
                                           : FVector(0.0f, 0.0f, 50.0f);
    AActor *const TargetActor = Event.TargetActor.Get();
    const FVector BaseLocation = ::IsValid(TargetActor) ? TargetActor->GetActorLocation() : FVector(Event.WorldLocation);
    if (!IsFiniteVector(BaseLocation)) {
        return;
    }
    const FVector ResolvedWorldLocation = BaseLocation + PresentationOffset;
    if (!IsFiniteVector(ResolvedWorldLocation)) {
        return;
    }

    CleanupExpired();
    const float CurrentTime = GetWorld()->GetTimeSeconds();
    if (FMythicDamageNumberData *Existing = FindMergeCandidate(Event, CurrentTime, ResolvedWorldLocation)) {
        const double CombinedMagnitude = static_cast<double>(Existing->Magnitude) + static_cast<double>(Event.Magnitude);
        if (FMath::IsFinite(CombinedMagnitude)
            && CombinedMagnitude <= static_cast<double>(TNumericLimits<float>::Max())) {
            Existing->Magnitude = static_cast<float>(CombinedMagnitude);
            Existing->CachedText = FText::FromString(FormatMagnitude(Existing->Magnitude));
            Existing->WorldLocation = ResolvedWorldLocation;
            Existing->TargetOffset = PresentationOffset;
            Existing->bIsCritical |= Event.bCritical && !bStatusTick
                && Event.Origin != EMythicCombatTextOrigin::Healing;
            if (Existing->bIsCritical && !bStatusTick) {
                Existing->Color = Config ? ResolveColor(Config->CriticalHitColor, FLinearColor::Yellow) : FLinearColor::Yellow;
                Existing->AnimStyle = Config ? Config->CriticalAnimStyle : EMythicDamageNumberAnimStyle::Bounce;
            }
            return;
        }
    }

    EnforceActiveBudget(1);
    FMythicDamageNumberData NewData;
    NewData.WorldLocation = ResolvedWorldLocation;
    NewData.TargetOffset = PresentationOffset;
    NewData.TargetActor = TargetActor;
    NewData.SourceActor = Event.SourceActor.Get();
    NewData.StatusDefinition = StatusDefinition;
    NewData.Origin = Event.Origin;
    NewData.Magnitude = Event.Magnitude;
    NewData.CachedText = FText::FromString(FormatMagnitude(Event.Magnitude));
    NewData.SpawnTime = CurrentTime;
    NewData.Lifetime = ResolveDefaultLifetime(Config);
    NewData.ID = NextID++;
    NewData.bIsCritical = Event.bCritical && !bStatusTick && Event.Origin != EMythicCombatTextOrigin::Healing;
    NewData.bOutgoingForViewer = Event.bOutgoingForViewer;
    NewData.bResolvedEvent = true;

    if (bStatusTick) {
        const FLinearColor StatusColor = StatusDefinition->DisplayColor;
        const FLinearColor DefaultColor = Config ? ResolveColor(Config->DefaultColor, FLinearColor::White) : FLinearColor::White;
        NewData.Color = ResolveColor(StatusColor, DefaultColor);
        NewData.AnimStyle = Config ? Config->StatusAnimStyle : EMythicDamageNumberAnimStyle::Shake;
        const float StatusScale = Config && FMath::IsFinite(Config->StatusScaleMultiplier)
                                      ? Config->StatusScaleMultiplier
                                      : 0.85f;
        const float StatusLifetime = Config && FMath::IsFinite(Config->StatusLifetimeMultiplier)
                                         ? Config->StatusLifetimeMultiplier
                                         : 0.85f;
        NewData.ScaleMultiplier = FMath::Clamp(StatusScale, 0.1f, 2.0f);
        const float ScaledLifetime = NewData.Lifetime * FMath::Clamp(StatusLifetime, 0.1f, 3.0f);
        if (FMath::IsFinite(ScaledLifetime)) {
            NewData.Lifetime = FMath::Max(MinimumNumberLifetime, ScaledLifetime);
        }
    }
    else if (Event.Origin == EMythicCombatTextOrigin::ShieldAbsorption) {
        NewData.Color = Config
                            ? ResolveColor(Config->ShieldAbsorptionColor, FLinearColor(0.4f, 0.7f, 1.0f))
                            : FLinearColor(0.4f, 0.7f, 1.0f);
        NewData.AnimStyle = Config
                                ? Config->ShieldAbsorptionAnimStyle
                                : EMythicDamageNumberAnimStyle::FloatUp;
    }
    else if (Event.Origin == EMythicCombatTextOrigin::Healing) {
        NewData.Color = Config
                            ? ResolveColor(Config->HealColor, FLinearColor(0.0f, 1.0f, 0.3f))
                            : FLinearColor(0.0f, 1.0f, 0.3f);
        NewData.AnimStyle = Config ? Config->HealAnimStyle : EMythicDamageNumberAnimStyle::Pulse;
    }
    else if (NewData.bIsCritical) {
        NewData.Color = Config ? ResolveColor(Config->CriticalHitColor, FLinearColor::Yellow) : FLinearColor::Yellow;
        NewData.AnimStyle = Config ? Config->CriticalAnimStyle : EMythicDamageNumberAnimStyle::Bounce;
    }
    else {
        NewData.Color = Config ? ResolveColor(Config->DefaultColor, FLinearColor::White) : FLinearColor::White;
        NewData.AnimStyle = Config ? Config->DefaultAnimStyle : EMythicDamageNumberAnimStyle::FloatUp;
    }

    ApplyRandomMotion(NewData);
    ActiveDamageNumbers.Add(MoveTemp(NewData));
}

void UMythicDamageNumberSubsystem::AddDodgeNumber(FVector WorldLocation) {
    const FLinearColor DodgeColor = Config ? ResolveColor(Config->DodgeColor, FLinearColor::Gray) : FLinearColor::Gray;
    const EMythicDamageNumberAnimStyle DodgeStyle = Config
                                                        ? Config->DodgeAnimStyle
                                                        : EMythicDamageNumberAnimStyle::FloatUpSlow;
    AddCombatTextInternal(WorldLocation, TEXT("DODGE"), DodgeColor, 1.0f, DodgeStyle);
}

void UMythicDamageNumberSubsystem::AddCombatText(FVector WorldLocation, const FString &Text, FLinearColor Color, float Lifetime) {
    const EMythicDamageNumberAnimStyle DefaultStyle = Config
                                                          ? Config->DefaultAnimStyle
                                                          : EMythicDamageNumberAnimStyle::FloatUp;
    AddCombatTextInternal(WorldLocation, Text, Color, Lifetime, DefaultStyle);
}

void UMythicDamageNumberSubsystem::AddCombatTextInternal(FVector WorldLocation, const FString &Text,
                                                         FLinearColor Color, float Lifetime,
                                                         EMythicDamageNumberAnimStyle AnimStyle) {
    if (!IsFiniteVector(WorldLocation) || !IsFiniteColor(Color) || !FMath::IsFinite(Lifetime) || Lifetime <= 0.0f
        || Text.IsEmpty() || !GetWorld()) {
        return;
    }
    CleanupExpired();
    EnforceActiveBudget(1);
    FMythicDamageNumberData NewData;
    NewData.WorldLocation = WorldLocation;
    NewData.CachedText = FText::FromString(Text);
    NewData.Color = Color;
    NewData.SpawnTime = GetWorld()->GetTimeSeconds();
    NewData.Lifetime = Lifetime;
    NewData.ID = NextID++;
    NewData.bIsCritical = false;
    NewData.AnimStyle = AnimStyle;

    ApplyRandomMotion(NewData);

    ActiveDamageNumbers.Add(MoveTemp(NewData));
}

void UMythicDamageNumberSubsystem::SetConfig(UMythicDamageNumberConfig *NewConfig) {
    Config = NewConfig;
    EnforceActiveBudget();
    ActiveDamageNumbers.Reserve(ResolveActiveNumberBudget(Config));
}

void UMythicDamageNumberSubsystem::ClearAll() {
    ActiveDamageNumbers.Empty();
}

void UMythicDamageNumberSubsystem::DrawDamageNumbers(UCanvas *Canvas, APlayerController *PC) {
    if (ActiveDamageNumbers.Num() == 0) {
        return;
    }

    const float CurrentTime = GetWorld()->GetTimeSeconds();

    FVector CameraLocation;
    FRotator CameraRotation;
    PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

    UFont *Font = nullptr;
    if (Config && Config->Font.Get()) {
        Font = Config->Font.Get();
    }
    if (!Font) {
        Font = GEngine->GetSmallFont();
    }

    float BaseScale = Config ? Config->FontScaleMultiplier : 1.0f;
    if (const UMythicUserSettings *UserSettings = UMythicUserSettings::Get()) {
        BaseScale *= UserSettings->GetDamageNumberScale();
    }
    const bool bOutline = Config ? Config->bEnableOutline : true;
    const FLinearColor OutlineColor = Config ? Config->OutlineColor : FLinearColor::Black;
    const float CritScaleMultiplier = Config ? Config->CriticalHitScaleMultiplier : 1.3f;

    for (int32 i = ActiveDamageNumbers.Num() - 1; i >= 0; --i) {
        FMythicDamageNumberData &Data = ActiveDamageNumbers[i];

        if (Data.IsExpired(CurrentTime)) {
            ActiveDamageNumbers.RemoveAtSwap(i, EAllowShrinking::No);
            continue;
        }

        FVector DrawLocation = Data.WorldLocation;
        if (AActor *TargetActor = Data.TargetActor.Get(); ::IsValid(TargetActor)) {
            DrawLocation = TargetActor->GetActorLocation() + Data.TargetOffset;
        }

        FVector2D ScreenPos;
        if (!UGameplayStatics::ProjectWorldToScreen(PC, DrawLocation, ScreenPos, true)) {
            continue;
        }

        const FVector2D AnimOffset = CalculateAnimationOffset(Data, CurrentTime);
        ScreenPos.X += AnimOffset.X;
        ScreenPos.Y += AnimOffset.Y;

        const float Alpha = Data.GetAlpha(CurrentTime);

        const float BurstScale = Config ? Config->BurstScaleMultiplier : 1.5f;
        const float BurstDuration = Config ? Config->BurstDuration : 0.15f;
        const float BurstScaleFactor = Data.GetBurstScale(CurrentTime, BurstScale, BurstDuration);

        const float AnimScaleFactor = CalculateAnimationScale(Data, CurrentTime);

        float FinalScale = BaseScale * Data.ScaleMultiplier * BurstScaleFactor * AnimScaleFactor;
        if (Data.bIsCritical) {
            FinalScale *= CritScaleMultiplier;
        }

        FLinearColor FinalColor = Data.Color;
        FinalColor.A *= Alpha;

        FCanvasTextItem TextItem(
            FVector2D(ScreenPos.X, ScreenPos.Y),
            Data.CachedText,
            Font,
            FinalColor
            );
        TextItem.Scale = FVector2D(FinalScale, FinalScale);
        TextItem.bCentreX = true;
        TextItem.bCentreY = true;
        TextItem.bOutlined = bOutline;
        TextItem.OutlineColor = OutlineColor;

        Canvas->DrawItem(TextItem);
    }
}

FString UMythicDamageNumberSubsystem::FormatMagnitude(float Magnitude) const {
    const float AbsMagnitude = FMath::Abs(Magnitude);

    if (Config && Config->bAbbreviateLargeNumbers) {
        if (AbsMagnitude >= Config->MillionThreshold) {
            return FString::Printf(TEXT("%.1fM"), AbsMagnitude / 1000000.0f);
        }
        else if (AbsMagnitude >= Config->ThousandThreshold) {
            return FString::Printf(TEXT("%.1fK"), AbsMagnitude / 1000.0f);
        }
    }

    return FString::Printf(TEXT("%lld"), FMath::RoundToInt64(AbsMagnitude));
}

FVector2D UMythicDamageNumberSubsystem::CalculateAnimationOffset(const FMythicDamageNumberData &Data, float CurrentTime) const {
    const float Age = CurrentTime - Data.SpawnTime;
    const float NormalizedAge = FMath::Clamp(Age / Data.Lifetime, 0.0f, 1.0f);
    const float BaseVerticalSpeed = Config ? Config->VerticalFloatSpeed : 50.0f;

    FVector2D Offset(Data.RandomOffsetX, 0.0f);

    switch (Data.AnimStyle) {
    case EMythicDamageNumberAnimStyle::FloatUp:
        Offset.Y = -Age * (BaseVerticalSpeed + Data.ExtraVerticalSpeed);
        break;

    case EMythicDamageNumberAnimStyle::FloatUpSlow:
        Offset.Y = -Age * (BaseVerticalSpeed * 0.5f + Data.ExtraVerticalSpeed);
        break;

    case EMythicDamageNumberAnimStyle::Bounce: {
        const float BouncePhase = FMath::Clamp(Age * 4.0f, 0.0f, 1.0f);
        const float BounceCurve = FMath::Sin(BouncePhase * PI) * 0.3f;
        Offset.Y = -Age * BaseVerticalSpeed - BounceCurve * 30.0f;
    }
    break;

    case EMythicDamageNumberAnimStyle::ArcLeft: {
        Offset.Y = -Age * BaseVerticalSpeed;
        Offset.X += -FMath::Sin(NormalizedAge * PI) * 40.0f;
    }
    break;

    case EMythicDamageNumberAnimStyle::ArcRight: {
        Offset.Y = -Age * BaseVerticalSpeed;
        Offset.X += FMath::Sin(NormalizedAge * PI) * 40.0f;
    }
    break;

    case EMythicDamageNumberAnimStyle::Shake: {
        const float ShakeIntensity = 4.0f * (1.0f - NormalizedAge);
        const float ShakeFrequency = 20.0f;
        Offset.X += FMath::Sin(Age * ShakeFrequency) * ShakeIntensity;
        Offset.Y = -Age * BaseVerticalSpeed;
    }
    break;

    case EMythicDamageNumberAnimStyle::Pulse:
        Offset.Y = -Age * BaseVerticalSpeed;
        break;
    }

    return Offset;
}

float UMythicDamageNumberSubsystem::CalculateAnimationScale(const FMythicDamageNumberData &Data, float CurrentTime) const {
    if (Data.AnimStyle != EMythicDamageNumberAnimStyle::Pulse) {
        return 1.0f;
    }

    const float Age = CurrentTime - Data.SpawnTime;
    const float NormalizedAge = FMath::Clamp(Age / Data.Lifetime, 0.0f, 1.0f);
    const float PulseFrequency = 6.0f;
    const float PulseAmount = 0.1f * (1.0f - NormalizedAge);
    return 1.0f + FMath::Sin(Age * PulseFrequency) * PulseAmount;
}
