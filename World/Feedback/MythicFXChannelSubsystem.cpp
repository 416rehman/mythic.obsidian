
#include "GAS/Feedback/MythicFXChannelSubsystem.h"

#include "Mythic/Mythic.h"
#include "Settings/MythicDeveloperSettings.h"

#include "NiagaraDataChannel.h"
#include "NiagaraDataChannelAsset.h"
#include "NiagaraDataChannelAccessor.h"
#include "NiagaraDataChannelPublic.h"
#include "NiagaraDataChannelFunctionLibrary.h"

#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"

namespace {
    const FName NDC_Position(TEXT("Position"));
    const FName NDC_Direction(TEXT("Direction"));
    const FName NDC_Color(TEXT("Color"));
    const FName NDC_Scale(TEXT("Scale"));
    const FName NDC_Kind(TEXT("Kind"));
}

UMythicFXChannelSubsystem *UMythicFXChannelSubsystem::Get(const UObject *WorldContextObject) {
    if (!WorldContextObject) {
        return nullptr;
    }
    const UWorld *World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    return World ? World->GetSubsystem<UMythicFXChannelSubsystem>() : nullptr;
}

void UMythicFXChannelSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);

    if (const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>()) {
        CullDistance = Settings->WorldFXCullDistance;
        MaxEventsPerFrame = Settings->WorldFXMaxEventsPerFrame;
    }
}

void UMythicFXChannelSubsystem::Deinitialize() {
    Pending.Reset();
    bFlushScheduled = false;
    Super::Deinitialize();
}

void UMythicFXChannelSubsystem::ResolveChannel() {
    if (bChannelResolved) {
        return;
    }
    bChannelResolved = true;

    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    if (!Settings || Settings->WorldFXDataChannel.IsNull()) {
        return;
    }
    Channel = Settings->WorldFXDataChannel.LoadSynchronous();
    if (!Channel) {
        UE_LOG(Myth, Warning, TEXT("MythicFXChannel: WorldFXDataChannel is set but failed to load; batched FX stays off."));
    }
}

bool UMythicFXChannelSubsystem::IsWorthShowing(const FVector &Location) const {
    if (CullDistance <= 0.0f) {
        return true;
    }
    const UWorld *World = GetWorld();
    if (!World) {
        return false;
    }

    const float CullSq = CullDistance * CullDistance;
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It) {
        const APlayerController *PC = It->Get();
        if (!PC || !PC->IsLocalController()) {
            continue;
        }
        FVector ViewLocation;
        FRotator ViewRotation;
        PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
        if (FVector::DistSquared(ViewLocation, Location) <= CullSq) {
            return true;
        }
    }
    return false;
}

bool UMythicFXChannelSubsystem::PushFX(EMythicFXKind Kind, const FVector &Location, const FVector &Direction,
                                       float Scale, const FLinearColor &Color) {
    ResolveChannel();
    if (!Channel) {
        return false;
    }
    if (!IsWorthShowing(Location)) {
        return false;
    }
    if (Pending.Num() >= MaxEventsPerFrame) {
        return false;
    }

    FMythicFXEvent &Event = Pending.AddDefaulted_GetRef();
    Event.Location = Location;
    Event.Direction = Direction.IsNearlyZero() ? FVector::UpVector : Direction.GetSafeNormal();
    Event.Color = Color;
    Event.Scale = Scale;
    Event.Kind = static_cast<int32>(Kind);

    if (!bFlushScheduled) {
        if (UWorld *World = GetWorld()) {
            bFlushScheduled = true;
            TWeakObjectPtr<UMythicFXChannelSubsystem> WeakThis(this);
            World->GetTimerManager().SetTimerForNextTick([WeakThis]() {
                if (UMythicFXChannelSubsystem *Self = WeakThis.Get()) {
                    Self->Flush();
                }
            });
        }
    }
    return true;
}

void UMythicFXChannelSubsystem::Flush() {
    bFlushScheduled = false;

    const int32 Count = Pending.Num();
    if (Count == 0 || !Channel) {
        Pending.Reset();
        return;
    }

    FNiagaraDataChannelSearchParameters SearchParams;
    UNiagaraDataChannelWriter *Writer = UNiagaraDataChannelLibrary::WriteToNiagaraDataChannel(
        this, Channel, SearchParams, Count,
 false, true, true,
        TEXT("MythicFX"));

    if (!Writer) {
        Pending.Reset();
        return;
    }

    for (int32 Index = 0; Index < Count; ++Index) {
        const FMythicFXEvent &Event = Pending[Index];
        Writer->WritePosition(NDC_Position, Index, Event.Location);
        Writer->WriteVector(NDC_Direction, Index, Event.Direction);
        Writer->WriteLinearColor(NDC_Color, Index, Event.Color);
        Writer->WriteFloat(NDC_Scale, Index, Event.Scale);
        Writer->WriteInt(NDC_Kind, Index, Event.Kind);
    }

    Pending.Reset();
}
