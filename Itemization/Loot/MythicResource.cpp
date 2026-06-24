#include "MythicResource.h"
#include "Mythic.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/MythicTags_GAS.h" // GAS_EVENT_DMG_RECEIVED
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"


// Sets default values
AMythicResource::AMythicResource() {
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    // Initialize the ability system component
    this->AbilitySystemComponent = CreateDefaultSubobject<UMythicAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    this->ReplacementMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ReplacementMeshComponent"));

    // Replicate to everyone
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
    AbilitySystemComponent->SetIsReplicated(true);
    this->ReplacementMeshComponent->SetIsReplicated(true);
    this->bReplicates = true;

    // Attribute Set
    this->LifeAttributes = CreateDefaultSubobject<UMythicAttributeSet_Life>(TEXT("LifeAttributes"));
}

bool AMythicResource::TakePlaceOfISM() {
    auto our_location = GetActorLocation();
    // Try 3 times, each time increasing the Z by 10 to see if we are overlapping an ISM via SphereTraceSingleForObjects
    FHitResult HitResult;
    for (int i = 0; i < 3; i++) {
        // The Start vector's Z coordinate is offset by i * 20
        FVector Start = FVector(our_location.X, our_location.Y, our_location.Z + (i * 20));
        // The End vector's Z coordinate is offset by (i + 1) * 20 - This way it checks the first 20 units, then the next 20 units, etc.
        FVector End = FVector(our_location.X, our_location.Y, our_location.Z + ((i + 1) * 20));

        UKismetSystemLibrary::SphereTraceSingleForObjects(GetWorld(), Start, End, this->Radius, this->ObjectTypes, this->bTraceComplex, TArray<AActor *>(),
                                                          this->ISMDetectionTraceDebug, HitResult, true);

        // if we hit something, check if the component is an ISM and the hit item index is greater than -1
        auto hit_component = HitResult.GetComponent();
        if (hit_component && HitResult.Item > -1) {
            // Get the component
            UInstancedStaticMeshComponent *ISM_Component = Cast<UInstancedStaticMeshComponent>(hit_component);
            if (ISM_Component) {
                // Cache the ISM component
                this->Source_ISM = ISM_Component;
                // Remove the ISM from the ISM component
                this->Source_ISM->RemoveInstance(HitResult.Item);
                // Set the replacement mesh component's mesh to the ISM's mesh
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
    // The instigator of the hit (the harvester) — credited to the server-side BP events.
    const AActor *InstConst = Payload ? Payload->Instigator : nullptr;
    AActor *HitInstigator = const_cast<AActor *>(InstConst);
    // Broadcast to clients
    OnResourceHit(HitInstigator, HitsTillDestruction);

    if (HitsTillDestruction <= 0) {
        OnResourceDestroyed(HitInstigator);
    }
}

// Called when the game starts or when spawned
// Since this actor will be spawned by the Server at the location of the ISM and is replicated to all clients,
// On BeginPlay, do a trace to see if we are within range of the ISM and if so, remove the ISM and set our transform to the ISM's transform.
void AMythicResource::BeginPlay() {
    Super::BeginPlay();

    // Set the hits till destruction based on the ISM's size, if scale is 0-0.6 3 hits, 0.6-0.8 4 hits, 0.8-1 5 hits, else 6 hits
    this->HitsTillDestruction = CalculateHitsTillDestruction();

    if (TakePlaceOfISM()) {
        UE_LOG(Myth, Warning, TEXT("AMythicResource::BeginPlay: Successfully replaced ISM"));
        // If server, initialize default abilities and effects
        if (GetLocalRole() == ROLE_Authority) {
            // Initialize the ASC's actor info before granting/applying — every other ASC owner (GameState/NPC/Player)
            // does this; without it the granted abilities have null Owner/Avatar actor info.
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
                        UE_LOG(LogTemp, Warning, TEXT("Gave default ability %s to %s"), *Ability->GetName(), *GetOwner()->GetName());
                    }
                    else {
                        UE_LOG(LogTemp, Warning, TEXT("Failed to give default ability"));
                    }
                }
                else {
                    UE_LOG(LogTemp, Warning, TEXT("Default ability is null"));
                }
            }

            // Decrement remaining hits on each damaging hit. The Life set fires GAS_EVENT_DMG_RECEIVED on this ASC per
            // hit (DamageDone > 0); listen for that LIVE event — the same GenericGameplayEventCallbacks mechanism the
            // LifeComponent uses for death/kill. (The old OnHealthChanged delegate this used is no longer broadcast, so
            // harvest-by-hit was silently dead.)
            this->AbilitySystemComponent->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_DMG_RECEIVED).AddUObject(this, &AMythicResource::HandleDamageReceived);
        }
        else {
            UE_LOG(LogTemp, Warning, TEXT("Not server, not initializing default abilities and effects"));
        }
    }
    else {
        UE_LOG(Myth, Error, TEXT("AMythicResource::BeginPlay: Failed to find an ISM to replace. Destroying."));
        Destroy();
    }
}

// Default implementation of CalculateHitsTillDestruction
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

    // Replicate the hits till destruction
    DOREPLIFETIME(AMythicResource, HitsTillDestruction);
}
