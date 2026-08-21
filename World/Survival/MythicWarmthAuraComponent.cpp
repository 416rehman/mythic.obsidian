#include "MythicWarmthAuraComponent.h"

#include "SurvivalTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"

UMythicWarmthAuraComponent::UMythicWarmthAuraComponent() {
    PrimaryComponentTick.bCanEverTick = false;

    InitSphereRadius(WarmthRadius);
    SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    SetGenerateOverlapEvents(true);
}

void UMythicWarmthAuraComponent::BeginPlay() {
    Super::BeginPlay();

    SetSphereRadius(WarmthRadius);

    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }

    OnComponentBeginOverlap.AddDynamic(this, &UMythicWarmthAuraComponent::OnAuraBeginOverlap);
    OnComponentEndOverlap.AddDynamic(this, &UMythicWarmthAuraComponent::OnAuraEndOverlap);

    TArray<AActor *> Overlapping;
    GetOverlappingActors(Overlapping, APawn::StaticClass());
    for (AActor *Actor : Overlapping) {
        if (UAbilitySystemComponent *ASC = ResolvePlayerASC(Actor)) {
            ASC->AddLooseGameplayTag(TAG_Status_Warm);
            WarmedASCs.Add(ASC);
        }
    }
}

void UMythicWarmthAuraComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    for (const TWeakObjectPtr<UAbilitySystemComponent> &WeakASC : WarmedASCs) {
        if (UAbilitySystemComponent *ASC = WeakASC.Get()) {
            ASC->RemoveLooseGameplayTag(TAG_Status_Warm);
        }
    }
    WarmedASCs.Reset();

    if (GetOwner() && GetOwner()->HasAuthority()) {
        OnComponentBeginOverlap.RemoveDynamic(this, &UMythicWarmthAuraComponent::OnAuraBeginOverlap);
        OnComponentEndOverlap.RemoveDynamic(this, &UMythicWarmthAuraComponent::OnAuraEndOverlap);
    }

    Super::EndPlay(EndPlayReason);
}

void UMythicWarmthAuraComponent::OnAuraBeginOverlap(UPrimitiveComponent *, AActor *OtherActor,
                                                    UPrimitiveComponent *, int32,
                                                    bool, const FHitResult &) {
    UAbilitySystemComponent *ASC = ResolvePlayerASC(OtherActor);
    if (!ASC || WarmedASCs.Contains(ASC)) {
        return;
    }
    ASC->AddLooseGameplayTag(TAG_Status_Warm);
    WarmedASCs.Add(ASC);
}

void UMythicWarmthAuraComponent::OnAuraEndOverlap(UPrimitiveComponent *, AActor *OtherActor,
                                                  UPrimitiveComponent *, int32) {
    UAbilitySystemComponent *ASC = ResolvePlayerASC(OtherActor);
    if (!ASC || !WarmedASCs.Contains(ASC)) {
        return;
    }
    ASC->RemoveLooseGameplayTag(TAG_Status_Warm);
    WarmedASCs.Remove(ASC);
}

UAbilitySystemComponent *UMythicWarmthAuraComponent::ResolvePlayerASC(AActor *Actor) {
    const APawn *Pawn = Cast<APawn>(Actor);
    if (!Pawn) {
        return nullptr;
    }
    if (const IAbilitySystemInterface *ASI = Cast<IAbilitySystemInterface>(Pawn->GetController())) {
        return ASI->GetAbilitySystemComponent();
    }
    if (const IAbilitySystemInterface *PawnASI = Cast<IAbilitySystemInterface>(Actor)) {
        return PawnASI->GetAbilitySystemComponent();
    }
    return nullptr;
}
