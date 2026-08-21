#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MythicFXChannelSubsystem.generated.h"

class UNiagaraDataChannelAsset;

UENUM(BlueprintType)
enum class EMythicFXKind : uint8 {
    Hit = 0,
    CriticalHit = 1,
    Burn = 2,
    Bleed = 3,
    Freeze = 4,
    Poison = 5,
    Slow = 6,
    Stun = 7,
    Reaction = 8,
    Heal = 9,
    WorldItemGlow = 10,
    Pickup = 11
};

USTRUCT()
struct FMythicFXEvent {
    GENERATED_BODY()

    FVector Location = FVector::ZeroVector;
    FVector Direction = FVector::UpVector;
    FLinearColor Color = FLinearColor::White;
    float Scale = 1.0f;
    int32 Kind = 0;
};

UCLASS()
class MYTHIC_API UMythicFXChannelSubsystem : public UWorldSubsystem {
    GENERATED_BODY()

public:
    static UMythicFXChannelSubsystem *Get(const UObject *WorldContextObject);

    // Queue one effect for this frame's batch. Safe to call from anywhere, any number of times per frame — that is the
    // point. Returns false when the effect was dropped (no channel configured, or culled as unviewable), so a caller
    // can fall back to spawning its own system if it must.
    UFUNCTION(BlueprintCallable, Category = "Mythic|FX")
    bool PushFX(EMythicFXKind Kind, const FVector &Location, const FVector &Direction = FVector::UpVector,
                float Scale = 1.0f, const FLinearColor &Color = FLinearColor::White);

    // How many effects are queued but not yet written. Diagnostics only.
    UFUNCTION(BlueprintPure, Category = "Mythic|FX")
    int32 GetPendingCount() const {
        return Pending.Num();
    }

    // Effects further than this from every local view target are dropped before they ever reach Niagara. 0 disables the
    // cull. Read from the developer settings on first use.
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|FX")
    float CullDistance = 8000.0f;

    // Hard ceiling on entries written in one frame. A pathological frame (a huge AoE into a dense pack) writes this
    // many and drops the rest rather than stalling — the visual difference is nil, the cost difference is not.
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|FX")
    int32 MaxEventsPerFrame = 256;

protected:
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;

private:
    TArray<FMythicFXEvent> Pending;

    bool bFlushScheduled = false;

    UPROPERTY()
    TObjectPtr<UNiagaraDataChannelAsset> Channel = nullptr;

    bool bChannelResolved = false;

    void ResolveChannel();

    void Flush();

    bool IsWorthShowing(const FVector &Location) const;
};
