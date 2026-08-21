
#include "World/Camping/MythicInfluenceSourceComponent.h"

#include "World/Camping/MythicTags_Camping.h"
#include "World/Farming/MythicTags_Farming.h"
#include "World/Survival/SurvivalTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Engine/World.h"

TArray<TWeakObjectPtr<UMythicInfluenceSourceComponent>> UMythicInfluenceSourceComponent::GLiveSources;

UMythicInfluenceSourceComponent::UMythicInfluenceSourceComponent() {
    PrimaryComponentTick.bCanEverTick = false;

    InitSphereRadius(InfluenceRadius);
    SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    SetGenerateOverlapEvents(true);
}

void UMythicInfluenceSourceComponent::BeginPlay() {
    Super::BeginPlay();

    SetSphereRadius(InfluenceRadius);

    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }
    if (RoleTag.IsValid()) {
        GLiveSources.Add(this);
    }
    if (!GrantedStatusTag.IsValid()) {
        return;
    }

    OnComponentBeginOverlap.AddDynamic(this, &UMythicInfluenceSourceComponent::OnInfluenceBeginOverlap);
    OnComponentEndOverlap.AddDynamic(this, &UMythicInfluenceSourceComponent::OnInfluenceEndOverlap);

    TArray<AActor *> Overlapping;
    GetOverlappingActors(Overlapping, APawn::StaticClass());
    for (AActor *Actor : Overlapping) {
        if (UAbilitySystemComponent *ASC = ResolveQualifyingASC(Actor)) {
            if (!GrantedASCs.Contains(ASC)) {
                ASC->AddLooseGameplayTag(GrantedStatusTag);
                GrantedASCs.Add(ASC);
            }
        }
    }
}

void UMythicInfluenceSourceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    for (const TWeakObjectPtr<UAbilitySystemComponent> &WeakASC : GrantedASCs) {
        if (UAbilitySystemComponent *ASC = WeakASC.Get()) {
            ASC->RemoveLooseGameplayTag(GrantedStatusTag);
        }
    }
    GrantedASCs.Reset();
    GLiveSources.RemoveSingleSwap(this, EAllowShrinking::No);

    if (GetOwner() && GetOwner()->HasAuthority() && GrantedStatusTag.IsValid()) {
        OnComponentBeginOverlap.RemoveDynamic(this, &UMythicInfluenceSourceComponent::OnInfluenceBeginOverlap);
        OnComponentEndOverlap.RemoveDynamic(this, &UMythicInfluenceSourceComponent::OnInfluenceEndOverlap);
    }

    Super::EndPlay(EndPlayReason);
}

void UMythicInfluenceSourceComponent::OnInfluenceBeginOverlap(UPrimitiveComponent *, AActor *OtherActor,
                                                              UPrimitiveComponent *, int32,
                                                              bool, const FHitResult &) {
    UAbilitySystemComponent *ASC = ResolveQualifyingASC(OtherActor);
    if (!ASC || GrantedASCs.Contains(ASC)) {
        return;
    }
    ASC->AddLooseGameplayTag(GrantedStatusTag);
    GrantedASCs.Add(ASC);
}

void UMythicInfluenceSourceComponent::OnInfluenceEndOverlap(UPrimitiveComponent *, AActor *OtherActor,
                                                            UPrimitiveComponent *, int32) {
    UAbilitySystemComponent *ASC = ResolveQualifyingASC(OtherActor);
    if (!ASC || !GrantedASCs.Contains(ASC)) {
        return;
    }
    ASC->RemoveLooseGameplayTag(GrantedStatusTag);
    GrantedASCs.Remove(ASC);
}

UAbilitySystemComponent *UMythicInfluenceSourceComponent::ResolveQualifyingASC(AActor *Actor) const {
    const APawn *Pawn = Cast<APawn>(Actor);
    if (!Pawn) {
        return nullptr;
    }
    if (RequiredActorClass && !Actor->IsA(RequiredActorClass)) {
        return nullptr;
    }
    if (const IAbilitySystemInterface *ASI = Cast<IAbilitySystemInterface>(Pawn->GetController())) {
        if (UAbilitySystemComponent *ASC = ASI->GetAbilitySystemComponent()) {
            return ASC;
        }
    }
    if (bAffectPlayersOnly) {
        return nullptr;
    }
    if (const IAbilitySystemInterface *PawnASI = Cast<IAbilitySystemInterface>(Actor)) {
        return PawnASI->GetAbilitySystemComponent();
    }
    return nullptr;
}


void UMythicInfluenceSourceComponent::GetInfluencesAt(const UWorld *World, const FVector &Location,
                                                      const FGameplayTag &QueryRoleTag, TArray<FMythicInfluenceHit> &OutHits) {
    OutHits.Reset();
    if (!World || !QueryRoleTag.IsValid()) {
        return;
    }
    for (int32 i = GLiveSources.Num() - 1; i >= 0; --i) {
        const UMythicInfluenceSourceComponent *Source = GLiveSources[i].Get();
        if (!Source || !Source->GetOwner()) {
            GLiveSources.RemoveAtSwap(i, 1, EAllowShrinking::No);
            continue;
        }
        if (Source->GetWorld() != World || Source->RoleTag != QueryRoleTag) {
            continue;
        }
        const float DistSq = FVector::DistSquared(Source->GetComponentLocation(), Location);
        if (DistSq > FMath::Square(Source->InfluenceRadius)) {
            continue;
        }
        FMythicInfluenceHit &Hit = OutHits.AddDefaulted_GetRef();
        Hit.Source = Source;
        Hit.Magnitude = Source->Magnitude;
        Hit.DistSq = DistSq;
    }
}

float UMythicInfluenceSourceComponent::GetTotalInfluenceAt(const UWorld *World, const FVector &Location,
                                                           const FGameplayTag &QueryRoleTag) {
    TArray<FMythicInfluenceHit> Hits;
    GetInfluencesAt(World, Location, QueryRoleTag, Hits);
    float Total = 0.0f;
    for (const FMythicInfluenceHit &Hit : Hits) {
        Total += Hit.Magnitude;
    }
    return Total;
}


UMythicShelterAuraComponent::UMythicShelterAuraComponent() {
    RoleTag = TAG_Influence_Shelter;
    GrantedStatusTag = TAG_Status_Sheltered;
    InfluenceRadius = 300.0f;
    Magnitude = 1.0f;
    bAffectPlayersOnly = true;
}


UMythicIrrigationAuraComponent::UMythicIrrigationAuraComponent() {
    RoleTag = TAG_Influence_Irrigation;
    InfluenceRadius = 900.0f;
    Magnitude = 1.0f;
}

UMythicPollinationAuraComponent::UMythicPollinationAuraComponent() {
    RoleTag = TAG_Influence_Pollination;
    InfluenceRadius = 1500.0f;
    Magnitude = 1.0f;
}

UMythicScarecrowAuraComponent::UMythicScarecrowAuraComponent() {
    RoleTag = TAG_Influence_Deterrence;
    InfluenceRadius = 1200.0f;
    Magnitude = 1.0f;
}
