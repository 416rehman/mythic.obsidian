#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "NativeGameplayTags.h"
#include "MythicCamaraderieComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Buff_Camaraderie);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMythicOnCamaraderieChanged, int32, Stacks);

UCLASS(ClassGroup = (Mythic), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicCamaraderieComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicCamaraderieComponent();

    // ── Reads (server + owning client) ──
    // The current ACTIVE camaraderie stacks (0 when the buff is not applied — including the inert PerAllyBonus=0 case).
    UFUNCTION(BlueprintPure, Category = "Coop|Camaraderie")
    int32 GetCamaraderieStacks() const { return CurrentStacks; }

    // ── Events ──
    UPROPERTY(BlueprintAssignable, Category = "Coop|Camaraderie")
    FMythicOnCamaraderieChanged OnCamaraderieChanged;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Owning player's active camaraderie stack count. COND_OwnerOnly: private per-player feedback state on a PlayerState
    // (net-relevant to EVERY client) — an unconditional rep would leak every player's ally proximity to all peers.
    // Mirrors the RegionTracker/FactionStanding siblings. ReplicatedUsing so the owning client re-broadcasts the change.
    UPROPERTY(ReplicatedUsing = OnRep_Stacks, BlueprintReadOnly, Category = "Coop|Camaraderie")
    int32 CurrentStacks = 0;

    UFUNCTION()
    void OnRep_Stacks();

private:
    void Sample();

    void UpdateBuff(UAbilitySystemComponent *ASC, int32 DesiredStacks, float EffectiveMagnitude);

    void RemoveBuff();

    FTimerHandle SampleTimerHandle;

    FActiveGameplayEffectHandle ActiveBuffHandle;
    TWeakObjectPtr<UAbilitySystemComponent> HandleOwnerASC;

    int32 LastAppliedStacks = 0;

    UPROPERTY()
    TSubclassOf<UGameplayEffect> ResolvedEffectClass;

    double CamaraderieRadiusSq = 0.0;
    int32 MaxAllyStacks = 0;
    float PerAllyBonus = 0.0f;

    int32 LastBroadcastStacks = 0;
};
