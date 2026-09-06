#include "Player/MythicSessionPartySubsystem.h"
#include "Engine/World.h"
#include "GameModes/MythicGameMode.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerState.h"
#include "Player/MythicPlayerRegistrySubsystem.h"
#include "Input/MythicCombatUtilityComponent.h"
#include "TimerManager.h"
#include "World/Harvesting/MythicHarvestClaimMembershipSubsystem.h"

bool UMythicSessionPartySubsystem::ShouldCreateSubsystem(UObject* Outer) const {
    const UWorld* World=Cast<UWorld>(Outer);
    return World && World->IsGameWorld() && World->GetNetMode()!=NM_Client;
}
void UMythicSessionPartySubsystem::Initialize(FSubsystemCollectionBase& Collection) {
    Super::Initialize(Collection);
    Collection.InitializeDependency<UMythicPlayerRegistrySubsystem>();
    Collection.InitializeDependency<UMythicHarvestClaimMembershipSubsystem>();
    SessionParty=FGuid::NewGuid();
}
void UMythicSessionPartySubsystem::Deinitialize() {
    bShuttingDown=true;
    GetWorld()->GetTimerManager().ClearTimer(ReconcileTimer);
    if (auto* Membership=GetWorld()->GetSubsystem<UMythicHarvestClaimMembershipSubsystem>()) {
        const uint64 Previous=Membership->GetLatestRosterRevision();
        if (Previous!=MAX_uint64 && !PublishedKeys.IsEmpty()) Membership->RemoveParty(SessionParty,Previous+1);
    }
    Joined.Reset(); PublishedKeys.Reset(); SessionParty.Invalidate();
    Super::Deinitialize();
}
void UMythicSessionPartySubsystem::RegisterController(AMythicPlayerController* Controller) {
    if (bShuttingDown || !IsValid(Controller) || !Controller->HasAuthority() || Controller->GetWorld()!=GetWorld()) return;
    auto* Player=Controller->GetPlayerState<AMythicPlayerState>();
    if (!Player) return;
    FJoinedPlayer* Existing=Joined.FindByPredicate([Controller](const FJoinedPlayer& Entry){ return Entry.Controller.Get()==Controller; });
    if (Existing) Existing->Player=Player;
    else { FJoinedPlayer Entry; Entry.Controller=Controller; Entry.Player=Player; Joined.Add(Entry); }
    Reconcile();
}
void UMythicSessionPartySubsystem::UnregisterController(AMythicPlayerController* Controller) {
    if (!Controller) return;
    Joined.RemoveAll([Controller](const FJoinedPlayer& Entry){ return !Entry.Controller.IsValid() || Entry.Controller.Get()==Controller; });
    if (auto* Utility=Controller->FindComponentByClass<UMythicCombatUtilityComponent>()) Utility->AuthorityEndUtilitySession();
    Reconcile();
}
void UMythicSessionPartySubsystem::NotifyPlayerReady(AMythicPlayerState* Player) {
    // A late async load may only refresh an existing joined connection; it cannot resurrect a logged-out one.
    if (!bShuttingDown && IsValid(Player) && Player->HasAuthority()
        && Joined.ContainsByPredicate([Player](const FJoinedPlayer& Entry){ return Entry.Player.Get()==Player && Entry.Controller.IsValid(); })) Reconcile();
}
bool UMythicSessionPartySubsystem::IsReadyController(const AMythicPlayerController* Controller) const {
    if (bShuttingDown || !IsValid(Controller) || Controller->IsActorBeingDestroyed() || !Controller->HasAuthority()
        || Controller->GetWorld()!=GetWorld()) return false;
    const auto* Player=Controller->GetPlayerState<AMythicPlayerState>();
    if (!IsValid(Player) || Player->IsActorBeingDestroyed() || Player->IsOnlyASpectator() || Player->IsInactive()
        || !Player->GetPersistentEntityId().IsValid()) return false;
    const FString& Key=Player->GetPersistentCharacterId();
    if (Key.IsEmpty() || Key.Len()>256 || Key.StartsWith(TEXT("session:"),ESearchCase::CaseSensitive)
        || Player->GetCanonicalPlayerKey()!=Key || Player->GetPlayerController()!=Controller) return false;
    const auto* Registry=GetWorld()->GetSubsystem<UMythicPlayerRegistrySubsystem>();
    if (!Registry || Registry->GetPlayerStateForKey(Key)!=Player || Registry->GetPlayerControllerForKey(Key)!=Controller) return false;
    int32 MatchingKey=0; bool bExactJoined=false;
    for (const auto& Entry:Joined) if (Entry.Controller.IsValid() && Entry.Player.IsValid()) {
        bExactJoined |= Entry.Controller.Get()==Controller && Entry.Player.Get()==Player;
        MatchingKey += Entry.Player->GetPersistentCharacterId()==Key ? 1 : 0;
    }
    return bExactJoined && MatchingKey==1;
}
void UMythicSessionPartySubsystem::GetReadyControllers(TArray<AMythicPlayerController*>& Controllers) const {
    Controllers.Reset();
    for (const auto& Entry:Joined) if (auto* PC=Entry.Controller.Get(); IsReadyController(PC)) Controllers.Add(PC);
}
void UMythicSessionPartySubsystem::Reconcile() {
    if (bShuttingDown || !SessionParty.IsValid()) return;
    if (bReconciling) {
        if (!bReconcileQueued) {
            bReconcileQueued=true;
            ReconcileTimer=GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this,[this](){ bReconcileQueued=false; Reconcile(); }));
        }
        return;
    }
    TGuardValue<bool> ReconcileGuard(bReconciling,true);
    Joined.RemoveAll([](const FJoinedPlayer& Entry){ return !Entry.Controller.IsValid() || !Entry.Player.IsValid(); });
    const auto* Mode=GetWorld()->GetAuthGameMode<AMythicGameMode>();
    auto* Membership=GetWorld()->GetSubsystem<UMythicHarvestClaimMembershipSubsystem>();
    if (!Membership) return;
    TArray<FString> Next;
    if (Mode && Mode->bSessionPlayersFormCoopParty) {
        for (const auto& Entry:Joined) if (IsReadyController(Entry.Controller.Get())) Next.Add(Entry.Player->GetPersistentCharacterId());
        Next.Sort();
    }
    if (Next==PublishedKeys) return;
    const uint64 Previous=Membership->GetLatestRosterRevision();
    if (Previous==MAX_uint64) return;
    if (Membership->ReplacePartyMembers(SessionParty,Previous+1,Next)) PublishedKeys=MoveTemp(Next);
}
