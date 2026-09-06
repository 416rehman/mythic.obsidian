#include "MythicAbilitySystemComponent.h"
#include "Abilities/MythicGA_AbilityWindow.h"
#include "AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "MythicGameplayEffectContext.h"
#include "MythicTags_GAS.h"
#include "Settings/MythicCombatSettings.h"
#include "TimerManager.h"
#include "GameplayEffectAggregator.h"
#include "GameplayPrediction.h"

namespace {
// The ledger records the shield the player can actually lose. Convert that evaluated value
// through GAS before writing its base so a multiplicative modifier cannot erase another grant.
bool WriteEvaluatedShield(UMythicAbilitySystemComponent* ASC,float Desired) {
    const auto Attribute=UMythicAttributeSet_Defense::GetShieldAttribute();
    FGameplayEffectAttributeCaptureSpec Capture(FGameplayEffectAttributeCaptureDefinition(Attribute,EGameplayEffectAttributeCaptureSource::Target,true));
    ASC->CaptureAttributeForGameplayEffect(Capture);
    FAggregator Snapshot; FAggregatorEvaluateParameters Parameters;
    if (!Capture.AttemptGetAttributeAggregatorSnapshot(Snapshot)) return false;
    const float Base=Snapshot.ReverseEvaluate(Desired,Parameters);
    if (!FMath::IsFinite(Base) || !FMath::IsNearlyEqual(Snapshot.EvaluateWithBase(Base,Parameters),Desired,0.01f)) return false;
    ASC->SetNumericAttributeBase(Attribute,FMath::Max(0.0f,Base));
    return true;
}
}

void UMythicAbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) {
    Super::AbilitySpecInputPressed(Spec);
    if (Spec.IsActive()) {
        const UGameplayAbility* Instance = Spec.GetPrimaryInstance();
        const FPredictionKey Key = Instance ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey() : Spec.ActivationInfo.GetActivationPredictionKey();
        InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, Key);
    }
}
void UMythicAbilitySystemComponent::AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) {
    Super::AbilitySpecInputReleased(Spec);
    if (Spec.IsActive()) {
        const UGameplayAbility* Instance = Spec.GetPrimaryInstance();
        const FPredictionKey Key = Instance ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey() : Spec.ActivationInfo.GetActivationPredictionKey();
        InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, Key);
    }
}
void UMythicAbilitySystemComponent::CancelAbilityWindows() {
    CancelAbilitiesByFunc([](const UMythicGameplayAbility* Ability,FGameplayAbilitySpecHandle){ return Ability && Ability->IsA<UMythicGA_AbilityWindow>(); },true);
}
int32 UMythicAbilitySystemComponent::AddAbilityGameplayCue(FGameplayTag Tag,const FGameplayCueParameters& Parameters) {
    const auto* Context=FMythicGameplayEffectContext::ExtractEffectContext(Parameters.EffectContext);
    if (!IsOwnerActorAuthoritative() || !Tag.IsValid() || !Context || !Context->SourceActivationId.IsValid() || ActiveGameplayCues.GameplayCues.Num()>=256) return INDEX_NONE;
    const FGuid Activation=Context->SourceActivationId;
    const auto Matches=[Tag,Activation](const FActiveGameplayCue& Cue) {
        const auto* Ctx=FMythicGameplayEffectContext::ExtractEffectContext(Cue.Parameters.EffectContext);
        return Cue.GameplayCueTag==Tag && Ctx && Ctx->SourceActivationId==Activation;
    };
    if (const auto* Existing=ActiveGameplayCues.GameplayCues.FindByPredicate(Matches)) return Existing->ReplicationID;
    const bool AlreadyPresent=ActiveGameplayCues.HasCue(Tag);
    FScopedPredictionWindow AuthoritativeCue(this,FPredictionKey(),false);
    AddGameplayCue(Tag,Parameters);
    // The loose-cue API coalesces WhileActive on the server by tag. These notifies are authored
    // per instigator, so each source needs its own start just as each replicated FastArray item does.
    if (AlreadyPresent) InvokeGameplayCueEvent(Tag,EGameplayCueEvent::WhileActive,Parameters);
    const auto* Added=ActiveGameplayCues.GameplayCues.FindByPredicate(Matches);
    return Added ? Added->ReplicationID : INDEX_NONE;
}
bool UMythicAbilitySystemComponent::RemoveAbilityGameplayCue(int32 ReplicationId) {
    if (!IsOwnerActorAuthoritative() || ReplicationId==INDEX_NONE) return false;
    const int32 Index=ActiveGameplayCues.GameplayCues.IndexOfByPredicate([ReplicationId](const FActiveGameplayCue& Cue){return Cue.ReplicationID==ReplicationId;});
    if (Index==INDEX_NONE) return false;
    const FActiveGameplayCue Removed=ActiveGameplayCues.GameplayCues[Index];
    ActiveGameplayCues.GameplayCues.RemoveAt(Index); ActiveGameplayCues.MarkArrayDirty();
    UpdateTagMap(Removed.GameplayCueTag,-1,EGameplayTagReplicationState::None);
    InvokeGameplayCueEvent(Removed.GameplayCueTag,EGameplayCueEvent::Removed,Removed.Parameters);
    ForceReplication(); return true;
}
FGuid UMythicAbilitySystemComponent::GrantOwnedShield(UObject* Source,FGuid ActivationId,FName Key,float Amount,float Duration) {
    if (!IsOwnerActorAuthoritative() || !IsValid(Source) || !ActivationId.IsValid() || Key.IsNone()
        || !FMath::IsFinite(Amount) || Amount < 0 || !FMath::IsFinite(Duration) || Duration <= 0 || Duration > 120
        || !HasAttributeSetForAttribute(UMythicAttributeSet_Defense::GetShieldAttribute())) return {};
    ExpireOwnedShields();
    FGuid Replaced;
    for (const auto& Entry : OwnedShields) if (Entry.Source == Source && Entry.ActivationId == ActivationId && Entry.LocalKey == Key) { Replaced = Entry.Handle; break; }
    if (Replaced.IsValid()) RemoveOwnedShield(Replaced);
    if (Amount == 0) return {};
    if (OwnedShields.Num() >= 32) return {};
    const auto* Settings = GetDefault<UMythicCombatSettings>();
    UClass* MaximumClass = Settings->RuneGuardMaxShieldEffect.Get();
    if (!MaximumClass) MaximumClass = Settings->RuneGuardMaxShieldEffect.LoadSynchronous();
    if (!MaximumClass) return {};
    auto Spec = MakeOutgoingSpec(MaximumClass,1,MakeEffectContext());
    if (!Spec.IsValid()) return {};
    Spec.Data->SetSetByCallerMagnitude(GAS_SETBYCALLER_GENERIC,Amount);
    Spec.Data->SetSetByCallerMagnitude(GAS_SETBYCALLER_DURATION,120);
    // The ledger owns expiry; a second GE expiry could clamp somebody else's residual first.
    Spec.Data->SetDuration(UGameplayEffect::INFINITE_DURATION,true);
    const auto Maximum = ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    if (!Maximum.IsValid()) return {};
    TGuardValue<bool> Writing(bWritingOwnedShield,true);
    const float Before = GetNumericAttribute(UMythicAttributeSet_Defense::GetShieldAttribute());
    if (!WriteEvaluatedShield(this,Before+Amount)) { RemoveActiveGameplayEffect(Maximum); return {}; }
    const float Added = FMath::Max(0.0f,GetNumericAttribute(UMythicAttributeSet_Defense::GetShieldAttribute())-Before);
    if (Added <= 0) { RemoveActiveGameplayEffect(Maximum); return {}; }
    FOwnedShield Entry;
    Entry.Handle=FGuid::NewGuid(); Entry.Source=Source; Entry.ActivationId=ActivationId; Entry.LocalKey=Key;
    Entry.Remaining=Added; Entry.Expiry=GetWorld()->GetTimeSeconds()+Duration; Entry.Sequence=++ShieldSequence; Entry.MaximumEffect=Maximum;
    OwnedShields.Add(Entry);
    if (auto* Removed=OnGameplayEffectRemoved_InfoDelegate(Maximum))
        Removed->AddWeakLambda(this,[this,Id=Entry.Handle](const FGameplayEffectRemovalInfo&){ RemoveOwnedShieldInternal(Id,true); });
    ScheduleShieldExpiry();
    return Entry.Handle;
}
bool UMythicAbilitySystemComponent::RemoveOwnedShield(FGuid Handle) {
    return RemoveOwnedShieldInternal(Handle,false);
}
bool UMythicAbilitySystemComponent::RemoveOwnedShieldInternal(FGuid Handle,bool bMaximumAlreadyRemoved) {
    if (!IsOwnerActorAuthoritative()) return false;
    const int32 Index=OwnedShields.IndexOfByPredicate([Handle](const auto& S){ return S.Handle==Handle; });
    if (Index==INDEX_NONE) return false;
    const FOwnedShield Removed=OwnedShields[Index];
    OwnedShields.RemoveAt(Index);
    {
        TGuardValue<bool> Writing(bWritingOwnedShield,true);
        const float Current=GetNumericAttribute(UMythicAttributeSet_Defense::GetShieldAttribute());
        const float Residual=FMath::Min(Current,FMath::Max(0.0f,Removed.Remaining-Removed.RemovalClamp));
        WriteEvaluatedShield(this,FMath::Max(0.0f,Current-Residual));
        if (!bMaximumAlreadyRemoved) RemoveActiveGameplayEffect(Removed.MaximumEffect);
    }
    ScheduleShieldExpiry();
    OnOwnedShieldRemoved.Broadcast(Handle);
    return true;
}
float UMythicAbilitySystemComponent::GetOwnedShieldRemaining(FGuid Handle) const {
    const auto* Entry=OwnedShields.FindByPredicate([Handle](const auto& S){ return S.Handle==Handle; });
    return Entry ? FMath::Max(0.0f,Entry->Remaining-Entry->RemovalClamp) : 0;
}
void UMythicAbilitySystemComponent::ReconcileOwnedShields(float Actual,float AttributeLoss) {
    if (!IsOwnerActorAuthoritative() || bWritingOwnedShield || bProcessingShieldDamage) return;
    // UE removes maximum modifiers before its removal delegate. Attribute that cap loss to the
    // disappearing contribution before reconciling another source's residual.
    float ClampLoss=FMath::Max(0.0f,AttributeLoss);
    for (auto& Entry:OwnedShields) if (!GetActiveGameplayEffect(Entry.MaximumEffect)) {
        const float Removed=FMath::Min(ClampLoss,FMath::Max(0.0f,Entry.Remaining-Entry.RemovalClamp)); Entry.RemovalClamp+=Removed; ClampLoss-=Removed;
    }
    float Total=0; for (const auto& Entry:OwnedShields) Total+=FMath::Max(0.0f,Entry.Remaining-Entry.RemovalClamp);
    float Excess=FMath::Max(0.0f,Total-FMath::Max(0.0f,Actual));
    OwnedShields.Sort([](const auto& A,const auto& B){ return A.Expiry==B.Expiry ? A.Sequence<B.Sequence : A.Expiry<B.Expiry; });
    for (auto& Entry:OwnedShields) { const float Removed=FMath::Min(Excess,FMath::Max(0.0f,Entry.Remaining-Entry.RemovalClamp)); Entry.Remaining-=Removed; Excess-=Removed; }
}
void UMythicAbilitySystemComponent::ConsumeOwnedShieldDamage(float Amount,const FGameplayEffectContextHandle& Context) {
    const bool IsDamage=bProcessingShieldDamage;
    bProcessingShieldDamage=false;
    if (!IsOwnerActorAuthoritative() || !IsDamage || Amount<=0) { ReconcileOwnedShields(GetNumericAttribute(UMythicAttributeSet_Defense::GetShieldAttribute())); return; }
    auto* Mythic=FMythicGameplayEffectContext::ExtractEffectContext(Context);
    OwnedShields.Sort([](const auto& A,const auto& B){ return A.Expiry==B.Expiry ? A.Sequence<B.Sequence : A.Expiry<B.Expiry; });
    for (auto& Entry:OwnedShields) {
        const float Absorbed=FMath::Min(Amount,Entry.Remaining);
        Entry.Remaining-=Absorbed; Amount-=Absorbed;
        if (Mythic && Absorbed>0) Mythic->OwnedShieldAbsorption.Add({Entry.Handle,Absorbed});
        if (Amount<=0) break;
    }
}
void UMythicAbilitySystemComponent::PublishOwnedShieldDamage(const FGameplayEffectContextHandle& Context) {
    if (auto* Mythic=FMythicGameplayEffectContext::ExtractEffectContext(Context)) {
        const auto Receipts=MoveTemp(Mythic->OwnedShieldAbsorption);
        Mythic->OwnedShieldAbsorption.Reset();
        for (const auto& Receipt:Receipts) OnOwnedShieldAbsorbed.Broadcast(Receipt.Handle,Receipt.Amount,Context);
    }
}
void UMythicAbilitySystemComponent::ScheduleShieldExpiry() {
    if (!GetWorld()) return;
    GetWorld()->GetTimerManager().ClearTimer(OwnedShieldTimer);
    if (OwnedShields.IsEmpty()) return;
    double Earliest=DBL_MAX;
    for (const auto& Entry:OwnedShields) Earliest=FMath::Min(Earliest,Entry.Expiry);
    GetWorld()->GetTimerManager().SetTimer(OwnedShieldTimer,this,&ThisClass::ExpireOwnedShields,FMath::Max(0.001, Earliest-GetWorld()->GetTimeSeconds()),false);
}
void UMythicAbilitySystemComponent::ExpireOwnedShields() {
    if (!GetWorld()) return;
    const double Now=GetWorld()->GetTimeSeconds();
    TArray<FGuid> Expired;
    for (const auto& Entry:OwnedShields) if (!Entry.Source.IsValid() || Entry.Expiry<=Now) Expired.Add(Entry.Handle);
    for (FGuid Id:Expired) RemoveOwnedShield(Id);
    ScheduleShieldExpiry();
}
