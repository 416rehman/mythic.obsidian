#include "MythicStatusGroupReservation.h"
#include "MythicStatusRegistry.h"
#include "MythicStatusEffectDefinition.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Engine/World.h"
#include "World/Entity/MythicEntityPresentationComponent.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_STATUS_GROUP_REMOVAL_PROTECTED,"Status.Policy.GroupRemovalProtected",
    "An active effect with this asset tag cannot be consumed by whole-source-group removal.");

namespace {
bool Eligible(const FActiveGameplayEffect* Effect,const UMythicStatusEffectDefinition* Definition,float Now) {
    if (!Effect || !Definition || !Definition->bAllowSourceGroupRemoval || !Definition->EffectToApply || !Effect->Spec.Def
        || Effect->IsPendingRemove || Effect->bIsInhibited
        || Effect->Spec.Def != Definition->EffectToApply.GetDefaultObject()
        || Effect->Spec.GetContext().GetSourceObject()!=Definition
        || Effect->Spec.Def->GetStackingType()!=EGameplayEffectStackingType::AggregateBySource
        || Effect->Spec.GetStackCount()<1 || !FMath::IsFinite(Effect->GetDuration()) || Effect->GetDuration()<=0
        || !FMath::IsFinite(Effect->GetTimeRemaining(Now)) || Effect->GetTimeRemaining(Now)<=0) return false;
    FGameplayTagContainer Tags;
    Effect->Spec.GetAllAssetTags(Tags);
    return !Tags.HasTag(GAS_STATUS_GROUP_REMOVAL_PROTECTED);
}
}

TSharedPtr<FMythicStatusGroupReservation> FMythicStatusGroupReservation::Reserve(
    UMythicAbilitySystemComponent* InRecipient,const TArray<const UMythicStatusEffectDefinition*>& Priority) {
    if (!IsInGameThread() || !IsValid(InRecipient) || !InRecipient->IsOwnerActorAuthoritative()
        || !IsValid(InRecipient->GetAvatarActor()) || !InRecipient->GetWorld() || Priority.IsEmpty() || Priority.Num()>8) return {};
    auto* InAvatar=InRecipient->GetAvatarActor();
    auto* InPresentation=InAvatar->FindComponentByClass<UMythicEntityPresentationComponent>();
    const auto InInstance=InPresentation ? InPresentation->GetPresentationInstance() : FMythicEntityPresentationInstance{};
    if (InPresentation && !InInstance.IsValid()) return {};
    TArray<FActiveGameplayEffectHandle> Handles;
    for (auto Effect=InRecipient->GetActiveGameplayEffects().CreateConstIterator(); Effect; ++Effect) {
        if (Handles.Num()>=512) return {}; // Bound both allocation and traversal, before scanning excess effects.
        Handles.Add(Effect->Handle);
    }
    const float Now=InRecipient->GetWorld()->GetTimeSeconds();
    FActiveGameplayEffectHandle Selected;
    const UMythicStatusEffectDefinition* SelectedDefinition=nullptr;
    float Longest=0;
    for (const auto* Candidate:Priority) {
        if (!Candidate || !Candidate->bAllowSourceGroupRemoval) continue;
        for (const auto CandidateHandle:Handles) {
            const auto* Effect=InRecipient->GetActiveGameplayEffect(CandidateHandle);
            if (!Eligible(Effect,Candidate,Now)) continue;
            const float Remaining=Effect->GetTimeRemaining(Now);
            if (!Selected.IsValid() || Remaining>Longest
                || (Remaining==Longest && GetTypeHash(CandidateHandle)<GetTypeHash(Selected))) {
                Selected=CandidateHandle; SelectedDefinition=Candidate; Longest=Remaining;
            }
        }
        if (Selected.IsValid()) break;
    }
    const auto* Effect=InRecipient->GetActiveGameplayEffect(Selected);
    auto* Events=InRecipient->GetActiveEffectEventSet(Selected);
    if (!Effect || !Events) return {};
    TSharedPtr<FMythicStatusGroupReservation> Result=MakeShared<FMythicStatusGroupReservation>();
    Result->Recipient=InRecipient; Result->Avatar=InAvatar; Result->Presentation=InPresentation;
    Result->bHadPresentation=InPresentation!=nullptr; Result->Instance=InInstance;
    Result->Definition=SelectedDefinition; Result->Source=Effect->Spec.GetContext().GetOriginalInstigatorAbilitySystemComponent();
    Result->Handle=Selected; Result->Context=Effect->Spec.GetContext();
    Result->StartTime=Effect->StartWorldTime; Result->Duration=Effect->GetDuration(); Result->Stacks=Effect->Spec.GetStackCount();
    Result->Damage=Effect->Spec.GetSetByCallerMagnitude(GAS_SETBYCALLER_STATUS_DAMAGE,false,0);
    Result->Control=Effect->Spec.GetSetByCallerMagnitude(GAS_SETBYCALLER_STATUS_CONTROL_MAGNITUDE,false,0);
    Result->Applying=InRecipient->OnStatusGroupWillApply.AddSP(Result.ToSharedRef(),&FMythicStatusGroupReservation::OnCanonicalApply);
    Result->StackChanged=Events->OnStackChanged.AddSP(Result.ToSharedRef(),&FMythicStatusGroupReservation::OnStackChanged);
    Result->TimeChanged=Events->OnTimeChanged.AddSP(Result.ToSharedRef(),&FMythicStatusGroupReservation::OnTimeChanged);
    Result->Removed=Events->OnEffectRemoved.AddSP(Result.ToSharedRef(),&FMythicStatusGroupReservation::OnRemoved);
    Result->Inhibited=Events->OnInhibitionChanged.AddSP(Result.ToSharedRef(),&FMythicStatusGroupReservation::OnInhibited);
    if (InPresentation) Result->IncarnationEnded=InPresentation->OnPresentationDeactivated.AddSP(Result.ToSharedRef(),&FMythicStatusGroupReservation::OnIncarnationEnded);
    return Result;
}

void FMythicStatusGroupReservation::OnCanonicalApply(const UMythicStatusEffectDefinition* InDefinition,const UAbilitySystemComponent* InSource) {
    if (Definition==InDefinition && Source==InSource) Invalidate();
}

bool FMythicStatusGroupReservation::IsCurrent() const {
    auto* ASC=Recipient.Get();
    if (!IsInGameThread() || bReleased || Revision!=1 || !ASC || !ASC->IsOwnerActorAuthoritative()
        || !Avatar.IsValid() || ASC->GetAvatarActor()!=Avatar.Get() || !ASC->GetWorld()
        || (bHadPresentation && (!Presentation.IsValid() || !Presentation->RepresentsInstance(Instance)))) return false;
    const auto* Effect=ASC->GetActiveGameplayEffect(Handle);
    return Eligible(Effect,Definition.Get(),ASC->GetWorld()->GetTimeSeconds())
        && Effect->Spec.GetContext().Get()==Context.Get() && Effect->StartWorldTime==StartTime
        && Effect->GetDuration()==Duration && Effect->Spec.GetStackCount()==Stacks
        && Effect->Spec.GetSetByCallerMagnitude(GAS_SETBYCALLER_STATUS_DAMAGE,false,0)==Damage
        && Effect->Spec.GetSetByCallerMagnitude(GAS_SETBYCALLER_STATUS_CONTROL_MAGNITUDE,false,0)==Control;
}

FMythicStatusGroupRemoval FMythicStatusGroupReservation::Consume() {
    FMythicStatusGroupRemoval Result;
    if (!IsCurrent()) { Release(); return Result; }
    auto* ASC=Recipient.Get();
    Result.Definition=const_cast<UMythicStatusEffectDefinition*>(Definition.Get());
    Result.StackCount=Stacks; Result.DamagePerTick=Damage; Result.ControlMagnitude=Control;
    Result.RemainingSeconds=ASC->GetActiveGameplayEffect(Handle)->GetTimeRemaining(ASC->GetWorld()->GetTimeSeconds());
    Result.RemovedContext=Context;
    Release(); // Claim before GAS removal callbacks. Reentry and a second consumer cannot replay this lease.
    if (!ASC->RemoveActiveGameplayEffect(Handle)) return {};
    Result.bRemoved=true; Result.ReceiptId=FGuid::NewGuid();
    return Result;
}

void FMythicStatusGroupReservation::Release() {
    if (bReleased) return;
    bReleased=true;
    if (auto* ASC=Recipient.Get()) {
        ASC->OnStatusGroupWillApply.Remove(Applying);
        if (auto* Events=ASC->GetActiveEffectEventSet(Handle)) {
            Events->OnStackChanged.Remove(StackChanged); Events->OnTimeChanged.Remove(TimeChanged);
            Events->OnEffectRemoved.Remove(Removed); Events->OnInhibitionChanged.Remove(Inhibited);
        }
    }
    if (auto* Adapter=Presentation.Get()) Adapter->OnPresentationDeactivated.Remove(IncarnationEnded);
}

FMythicStatusGroupReservation::~FMythicStatusGroupReservation() { Release(); }
