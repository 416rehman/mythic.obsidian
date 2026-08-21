
#include "MythicCamaraderieComponent.h"

#include "MythicCamaraderieCore.h"
#include "MythicGE_Camaraderie.h"

#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"

#include "Player/MythicPlayerState.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "Settings/MythicDeveloperSettings.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Buff_Camaraderie, "Buff.Camaraderie", "Owning player is fighting near allies (co-op proximity synergy buff active)");


const FName UMythicGE_Camaraderie::BonusMagnitudeName(TEXT("Camaraderie.Bonus"));

UMythicGE_Camaraderie::UMythicGE_Camaraderie() {
    DurationPolicy = EGameplayEffectDurationType::Infinite;

    {
        FGameplayModifierInfo Mod;
        Mod.Attribute = UMythicAttributeSet_Offense::GetPowerAttribute();
        Mod.ModifierOp = EGameplayModOp::Additive;
        FSetByCallerFloat SBC;
        SBC.DataName = BonusMagnitudeName;
        Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SBC);
        Modifiers.Add(Mod);
    }

    GrantedBuffTags.AddTag(TAG_Buff_Camaraderie);
}


UMythicCamaraderieComponent::UMythicCamaraderieComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UMythicCamaraderieComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMythicCamaraderieComponent, CurrentStacks, COND_OwnerOnly);
}

void UMythicCamaraderieComponent::BeginPlay() {
    Super::BeginPlay();

    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    if (!Settings || !Settings->bCamaraderieEnabled) {
        return;
    }

    CamaraderieRadiusSq = FMath::Square(static_cast<double>(FMath::Max(0.0f, Settings->CamaraderieRadius)));
    MaxAllyStacks = FMath::Max(0, Settings->MaxAllyStacks);
    PerAllyBonus = FMath::Max(0.0f, Settings->PerAllyBonus);

    ResolvedEffectClass = Settings->CamaraderieEffect.IsNull()
                              ? UMythicGE_Camaraderie::StaticClass()
                              : Settings->CamaraderieEffect.LoadSynchronous();
    if (!ResolvedEffectClass) {
        ResolvedEffectClass = UMythicGE_Camaraderie::StaticClass();
    }

    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().SetTimer(SampleTimerHandle, this, &UMythicCamaraderieComponent::Sample, 1.0f,true);
    }
}

void UMythicCamaraderieComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(SampleTimerHandle);
    }
    RemoveBuff();
    LastAppliedStacks = 0;
    HandleOwnerASC.Reset();
    Super::EndPlay(EndPlayReason);
}

void UMythicCamaraderieComponent::Sample() {
    AMythicPlayerState *SelfPS = GetOwner<AMythicPlayerState>();
    if (!SelfPS || !SelfPS->HasAuthority()) {
        return;
    }
    const APawn *SelfPawn = SelfPS->GetPawn();
    if (!SelfPawn) {
        return;
    }
    UAbilitySystemComponent *ASC = SelfPS->GetMythicAbilitySystemComponent();
    if (!ASC) {
        return;
    }

    TArray<FVector, TInlineAllocator<8>> AllyLocs;
    if (const AGameStateBase *GS = GetWorld() ? GetWorld()->GetGameState() : nullptr) {
        for (const APlayerState *PS : GS->PlayerArray) {
            if (!PS || PS == SelfPS) {
                continue;
            }
            if (const APawn *Pawn = PS->GetPawn()) {
                AllyLocs.Add(Pawn->GetActorLocation());
            }
        }
    }

    const int32 Stacks = MythicCamaraderie::CountAlliesInRadius(SelfPawn->GetActorLocation(), AllyLocs, CamaraderieRadiusSq, MaxAllyStacks);
    const float Bonus = MythicCamaraderie::EffectiveBonus(Stacks, PerAllyBonus);
    const int32 Desired = (Bonus > 0.0f) ? Stacks : 0;

    UpdateBuff(ASC, Desired, Bonus);

    if (CurrentStacks != Desired) {
        CurrentStacks = Desired;
        if (LastBroadcastStacks != CurrentStacks) {
            LastBroadcastStacks = CurrentStacks;
            OnCamaraderieChanged.Broadcast(CurrentStacks);
        }
    }
}

void UMythicCamaraderieComponent::UpdateBuff(UAbilitySystemComponent *ASC, int32 DesiredStacks, float EffectiveMagnitude) {
    if (DesiredStacks == LastAppliedStacks) {
        return;
    }
    if (HandleOwnerASC.Get() != ASC) {
        ActiveBuffHandle.Invalidate();
        HandleOwnerASC = ASC;
    }

    RemoveBuff();
    if (DesiredStacks > 0 && ASC && ResolvedEffectClass && EffectiveMagnitude > 0.0f) {
        FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
        Ctx.AddSourceObject(GetOwner());
        const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(ResolvedEffectClass, 1.0f, Ctx);
        if (Spec.IsValid() && Spec.Data.IsValid()) {
            Spec.Data->SetSetByCallerMagnitude(UMythicGE_Camaraderie::BonusMagnitudeName, EffectiveMagnitude);
            ActiveBuffHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
            HandleOwnerASC = ASC;
        }
    }
    LastAppliedStacks = DesiredStacks;
}

void UMythicCamaraderieComponent::RemoveBuff() {
    if (UAbilitySystemComponent *ASC = HandleOwnerASC.Get()) {
        if (ActiveBuffHandle.IsValid()) {
            ASC->RemoveActiveGameplayEffect(ActiveBuffHandle);
        }
    }
    ActiveBuffHandle.Invalidate();
}

void UMythicCamaraderieComponent::OnRep_Stacks() {
    if (LastBroadcastStacks == CurrentStacks) {
        return;
    }
    LastBroadcastStacks = CurrentStacks;
    OnCamaraderieChanged.Broadcast(CurrentStacks);
}
