#include "MythicAbilityCueLooping.h"
#include "TimerManager.h"
#include "Engine/World.h"

AMythicAbilityCueLooping::AMythicAbilityCueLooping() {
    bUniqueInstancePerInstigator=true; bUniqueInstancePerSourceObject=true;
    bAllowMultipleOnActiveEvents=false; bAllowMultipleWhileActiveEvents=false;
    bAutoDestroyOnRemove=true; AutoDestroyDelay=0;
    PrimaryActorTick.bCanEverTick=false;
}
void AMythicAbilityCueLooping::HandleGameplayCue(AActor* Target,EGameplayCueEvent::Type Event,const FGameplayCueParameters& Parameters) {
    if (Event!=EGameplayCueEvent::Removed) { Super::HandleGameplayCue(Target,Event,Parameters); return; }
    if (bHasHandledOnRemoveEvent || bInRecycleQueue) return;
    // UE's default removal guard checks the recipient's aggregate tag count. Another caster's
    // contribution must not keep this source's ring or tether alive. GAS has already matched this
    // notify by instigator/source; retain its normal removal event and pooling lifecycle.
    bHasHandledOnRemoveEvent=true;
    if (IsValid(Target)) K2_HandleGameplayCue(Target,Event,Parameters);
    SetLifeSpan(0);
    OnRemove(IsValid(Target) ? Target : nullptr,Parameters);
    if (bAutoDestroyOnRemove) {
        if (AutoDestroyDelay>0 && GetWorld()) GetWorld()->GetTimerManager().SetTimer(FinishTimerHandle,FTimerDelegate::CreateUObject(this,&ThisClass::GameplayCueFinishedCallback),AutoDestroyDelay,false);
        else GameplayCueFinishedCallback();
    }
}
