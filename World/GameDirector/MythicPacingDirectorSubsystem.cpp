
#include "World/GameDirector/MythicPacingDirectorSubsystem.h"
#include "Mythic.h"

#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "GAS/MythicTags_GAS.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffectTypes.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"


bool UMythicPacingDirectorSubsystem::ShouldCreateSubsystem(UObject* Outer) const {
    const UWorld* World = Cast<UWorld>(Outer);
    if (!World || !World->IsGameWorld()) {
        return false;
    }
    return World->GetNetMode() < NM_Client;
}

void UMythicPacingDirectorSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
    Super::Initialize(Collection);

    State = FMythicDirectorPacing::MakeInitialState(Config);
    LastThreatTime = -1.0e9;

    if (UWorld* World = GetWorld()) {
        World->GetTimerManager().SetTimer(
            SampleTimerHandle, this, &UMythicPacingDirectorSubsystem::SampleAndStep,
            SampleInterval,true,SampleInterval);
    }

    UE_LOG(Myth, Log, TEXT("PacingDirector: initialized (server pacing brain online, sample=%.1fs)."), SampleInterval);
}

void UMythicPacingDirectorSubsystem::Deinitialize() {
    if (UWorld* World = GetWorld()) {
        World->GetTimerManager().ClearTimer(SampleTimerHandle);
    }
    UnbindAll();
    Super::Deinitialize();
}


void UMythicPacingDirectorSubsystem::SetConfigAsset(const UMythicDirectorConfigAsset* InAsset) {
    Config = InAsset ? InAsset->Config : FMythicDirectorConfig();
}


void UMythicPacingDirectorSubsystem::SampleAndStep() {
    const float Dt = SampleInterval;

    RefreshPartyBindings();
    LastInputs = GatherInputs(Dt);
    State = FMythicDirectorPacing::Step(LastInputs, Config, State, Dt);
}

FMythicDirectorInputs UMythicPacingDirectorSubsystem::GatherInputs(float DeltaSeconds) {
    FMythicDirectorInputs In;

    const UWorld* World = GetWorld();
    const double Now = World ? World->GetTimeSeconds() : 0.0;

    const double WindowStart = Now - WindowSeconds;
    DownTimestamps.RemoveAll([WindowStart](double T) { return T < WindowStart; });
    KillTimestamps.RemoveAll([WindowStart](double T) { return T < WindowStart; });

    float SumHealth = 0.0f;
    int32 PartyCount = 0;
    float SampleDamageFrac = 0.0f;
    bool bAnyDownedNow = false;

    TSet<TWeakObjectPtr<APawn>> SeenPawns;

    if (World) {
        for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It) {
            const APlayerController* PC = It->Get();
            if (!PC || !PC->HasAuthority()) {
                continue;
            }
            APawn* Pawn = PC->GetPawn();
            if (!Pawn) {
                continue;
            }
            UMythicLifeComponent* Life = UMythicLifeComponent::FindHealthComponent(Pawn);
            if (!Life) {
                continue;
            }

            const float HpNorm = FMath::Clamp(Life->GetHealthNormalized(), 0.0f, 1.0f);
            SumHealth += HpNorm;
            ++PartyCount;

            const TWeakObjectPtr<APawn> PawnKey(Pawn);
            SeenPawns.Add(PawnKey);
            const float* PrevPtr = PrevHealthByPawn.Find(PawnKey);
            const float Prev = PrevPtr ? *PrevPtr : HpNorm;
            SampleDamageFrac += FMath::Max(0.0f, Prev - HpNorm);
            PrevHealthByPawn.Add(PawnKey, HpNorm);

            if (Life->IsDowned()) {
                bAnyDownedNow = true;
            }
        }
    }

    for (auto It = PrevHealthByPawn.CreateIterator(); It; ++It) {
        if (!It.Key().IsValid() || !SeenPawns.Contains(It.Key())) {
            It.RemoveCurrent();
        }
    }

    In.AvgPartyHealthPct = PartyCount > 0 ? (SumHealth / PartyCount) : 1.0f;

    const float FracThisSample = PartyCount > 0 ? (SampleDamageFrac / PartyCount) : 0.0f;
    const float Decay = FMath::Exp(-FMath::Max(0.0f, DeltaSeconds) / FMath::Max(0.01f, WindowSeconds));
    DamageAccumNorm = DamageAccumNorm * Decay + FracThisSample;
    In.RecentDamageTakenNorm = DamageAccumNorm;

    if (FracThisSample > KINDA_SMALL_NUMBER || bAnyDownedNow) {
        LastThreatTime = Now;
    }
    In.TimeSinceLastThreat = static_cast<float>(FMath::Max(0.0, Now - LastThreatTime));

    In.DownsInWindow = DownTimestamps.Num();
    In.KillsPerSec = KillTimestamps.Num() / FMath::Max(0.01f, WindowSeconds);

    return In;
}


void UMythicPacingDirectorSubsystem::RefreshPartyBindings() {
    UWorld* World = GetWorld();
    if (!World) {
        return;
    }

    const FGameplayTag KillTag = GAS_EVENT_KILL;

    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It) {
        const APlayerController* PC = It->Get();
        if (!PC || !PC->HasAuthority()) {
            continue;
        }
        APawn* Pawn = PC->GetPawn();
        if (!Pawn) {
            continue;
        }

        if (UMythicLifeComponent* Life = UMythicLifeComponent::FindHealthComponent(Pawn)) {
            const TWeakObjectPtr<UMythicLifeComponent> LifeKey(Life);
            if (!BoundLifeComps.Contains(LifeKey)) {
                Life->OnDowned.AddDynamic(this, &UMythicPacingDirectorSubsystem::HandlePlayerDowned);
                BoundLifeComps.Add(LifeKey, true);
            }
        }

        if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn)) {
            const TWeakObjectPtr<UAbilitySystemComponent> ASCKey(ASC);
            if (!BoundKillASCs.Contains(ASCKey)) {
                const FDelegateHandle Handle = ASC->GenericGameplayEventCallbacks.FindOrAdd(KillTag).AddUObject(
                    this, &UMythicPacingDirectorSubsystem::HandleKillEvent);
                BoundKillASCs.Add(ASCKey, Handle);
            }
        }
    }

    for (auto BindIt = BoundLifeComps.CreateIterator(); BindIt; ++BindIt) {
        if (!BindIt.Key().IsValid()) {
            BindIt.RemoveCurrent();
        }
    }
    for (auto BindIt = BoundKillASCs.CreateIterator(); BindIt; ++BindIt) {
        if (!BindIt.Key().IsValid()) {
            BindIt.RemoveCurrent();
        }
    }
}

void UMythicPacingDirectorSubsystem::UnbindAll() {
    const FGameplayTag KillTag = GAS_EVENT_KILL;

    for (auto& Pair : BoundLifeComps) {
        if (UMythicLifeComponent* Life = Pair.Key.Get()) {
            Life->OnDowned.RemoveDynamic(this, &UMythicPacingDirectorSubsystem::HandlePlayerDowned);
        }
    }
    BoundLifeComps.Empty();

    for (auto& Pair : BoundKillASCs) {
        if (UAbilitySystemComponent* ASC = Pair.Key.Get()) {
            if (Pair.Value.IsValid()) {
                ASC->GenericGameplayEventCallbacks.FindOrAdd(KillTag).Remove(Pair.Value);
            }
        }
    }
    BoundKillASCs.Empty();

    PrevHealthByPawn.Empty();
    DownTimestamps.Empty();
    KillTimestamps.Empty();
}

void UMythicPacingDirectorSubsystem::HandlePlayerDowned(AActor* DownedActor) {
    const UWorld* World = GetWorld();
    const double Now = World ? World->GetTimeSeconds() : 0.0;
    DownTimestamps.Add(Now);
    LastThreatTime = Now;
}

void UMythicPacingDirectorSubsystem::HandleKillEvent(const FGameplayEventData*) {
    const UWorld* World = GetWorld();
    const double Now = World ? World->GetTimeSeconds() : 0.0;
    KillTimestamps.Add(Now);
}
