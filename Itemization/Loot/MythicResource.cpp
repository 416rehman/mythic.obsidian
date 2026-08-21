#include "MythicResource.h"
#include "Mythic.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/MythicTags_GAS.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"


AMythicResource::AMythicResource() {
    PrimaryActorTick.bCanEverTick = false;

    this->AbilitySystemComponent = CreateDefaultSubobject<UMythicAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    this->ReplacementMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ReplacementMeshComponent"));

    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
    AbilitySystemComponent->SetIsReplicated(true);
    this->ReplacementMeshComponent->SetIsReplicated(true);
    this->bReplicates = true;

    this->LifeAttributes = CreateDefaultSubobject<UMythicAttributeSet_Life>(TEXT("LifeAttributes"));
}

bool AMythicResource::TakePlaceOfISM() {
    auto our_location = GetActorLocation();
    FHitResult HitResult;
    for (int i = 0; i < 3; i++) {
        FVector Start = FVector(our_location.X, our_location.Y, our_location.Z + (i * 20));
        FVector End = FVector(our_location.X, our_location.Y, our_location.Z + ((i + 1) * 20));

        UKismetSystemLibrary::SphereTraceSingleForObjects(GetWorld(), Start, End, this->Radius, this->ObjectTypes, this->bTraceComplex, TArray<AActor *>(),
                                                          this->ISMDetectionTraceDebug, HitResult, true);

        auto hit_component = HitResult.GetComponent();
        if (hit_component && HitResult.Item > -1) {
            UInstancedStaticMeshComponent *ISM_Component = Cast<UInstancedStaticMeshComponent>(hit_component);
            if (ISM_Component) {
                this->Source_ISM = ISM_Component;
                this->Source_ISM->RemoveInstance(HitResult.Item);
                if (auto mesh = this->Source_ISM->GetStaticMesh()) {
                    this->ReplacementMeshComponent->SetStaticMesh(mesh);
                }
                else {
                    UE_LOG(Myth, Warning, TEXT("AMythicResource::TakePlaceOfISM: ISM has no mesh."));
                }

                return true;
            }
        }
    }

    return false;
}

void AMythicResource::HandleDamageReceived(const FGameplayEventData *Payload) {
    HitsTillDestruction--;
    const AActor *InstConst = Payload ? Payload->Instigator : nullptr;
    AActor *HitInstigator = const_cast<AActor *>(InstConst);
    OnResourceHit(HitInstigator, HitsTillDestruction);

    if (HitsTillDestruction <= 0) {
        OnResourceDestroyed(HitInstigator);
    }
}

void AMythicResource::BeginPlay() {
    Super::BeginPlay();

    this->HitsTillDestruction = CalculateHitsTillDestruction();

    if (TakePlaceOfISM()) {
        UE_LOG(Myth, Warning, TEXT("AMythicResource::BeginPlay: Successfully replaced ISM"));
        if (GetLocalRole() == ROLE_Authority) {
            this->AbilitySystemComponent->InitAbilityActorInfo(this, this);

            for (TSubclassOf<UGameplayEffect> Effect : DefaultGameplayEffects) {
                if (Effect) {
                    FGameplayEffectContextHandle EffectContext = this->AbilitySystemComponent->MakeEffectContext();
                    FGameplayEffectSpecHandle SpecHandle = this->AbilitySystemComponent->MakeOutgoingSpec(Effect, 1, EffectContext);
                    if (this->AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get()).IsValid()) {
                        UE_LOG(Myth, Warning, TEXT("Applied default gameplay effect %s to %s"), *Effect->GetName(), *GetOwner()->GetName());
                    }
                    else {
                        UE_LOG(Myth, Warning, TEXT("Failed to apply default gameplay effect"));
                    }
                }
                else {
                    UE_LOG(Myth, Warning, TEXT("Default gameplay effect is null"));
                }
            }

            for (TSubclassOf<UMythicGameplayAbility> Ability : DefaultAbilities) {
                if (Ability) {
                    auto Ability_CDO = Ability.GetDefaultObject();
                    if (this->AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(Ability_CDO, 1, INDEX_NONE, this)).IsValid()) {
                        UE_LOG(LogTemp, Verbose, TEXT("Gave default ability %s to %s"), *Ability->GetName(), *GetOwner()->GetName());
                    }
                    else {
                        UE_LOG(LogTemp, Warning, TEXT("Failed to give default ability"));
                    }
                }
                else {
                    UE_LOG(LogTemp, Warning, TEXT("Default ability is null"));
                }
            }

            this->AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_DMG_RECEIVED).AddUObject(this, &AMythicResource::HandleDamageReceived);
        }
        else {
            UE_LOG(LogTemp, Verbose, TEXT("Not server, not initializing default abilities and effects"));
        }
    }
    else {
        UE_LOG(Myth, Error, TEXT("AMythicResource::BeginPlay: Failed to find an ISM to replace. Destroying."));
        Destroy();
    }
}

int32 AMythicResource::CalculateHitsTillDestruction_Implementation() {
    auto scale = this->GetActorScale3D().Z;
    if (scale >= 0.0 && scale < 0.6) {
        return 3;
    }
    else if (scale >= 0.6 && scale < 0.8) {
        return 4;
    }
    else if (scale >= 0.8 && scale < 1) {
        return 5;
    }
    else {
        return 6;
    }
}

void AMythicResource::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AMythicResource, HitsTillDestruction);
}
