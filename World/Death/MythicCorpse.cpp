
#include "MythicCorpse.h"

#include "MythicCorpseConfig.h"
#include "MythicCorpseHazardSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Player/MythicPlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GAS/MythicTags_GAS.h"
#include "Subsystem/SaveSystem/Character/SavedInventory.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

AMythicCorpse::AMythicCorpse() {
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    bReplicateUsingRegisteredSubObjectList = true;
    SetNetCullDistanceSquared(FMath::Square(6000.f));

    NetDormancy = DORM_DormantAll;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    InteractionBounds = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionBounds"));
    InteractionBounds->SetupAttachment(SceneRoot);
    InteractionBounds->InitSphereRadius(60.0f);
    InteractionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionBounds->SetCollisionObjectType(ECC_WorldDynamic);
    InteractionBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    CorpseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CorpseMesh"));
    CorpseMesh->SetupAttachment(SceneRoot);
    CorpseMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    ContainerInventory = CreateDefaultSubobject<UMythicInventoryComponent>(TEXT("ContainerInventory"));
    ContainerInventory->SetIsReplicated(true);
}

void AMythicCorpse::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AMythicCorpse, DecompStage);
    DOREPLIFETIME(AMythicCorpse, DeathTime);
    DOREPLIFETIME(AMythicCorpse, SourceNameHash);
    DOREPLIFETIME(AMythicCorpse, Faction);
    DOREPLIFETIME(AMythicCorpse, RoleTag);
    DOREPLIFETIME(AMythicCorpse, SourceTier);
    DOREPLIFETIME(AMythicCorpse, bRaisable);
    DOREPLIFETIME(AMythicCorpse, bAlreadyRaised);
    DOREPLIFETIME(AMythicCorpse, SourceKind);
    DOREPLIFETIME(AMythicCorpse, bSkinnable);
    DOREPLIFETIME(AMythicCorpse, bSkinned);
    DOREPLIFETIME(AMythicCorpse, KillContext);
}

void AMythicCorpse::BeginPlay() {
    Super::BeginPlay();
    RegisterCorpseHazard();
}

void AMythicCorpse::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (UWorld *W = GetWorld()) {
        W->GetTimerManager().ClearTimer(DecayTimerHandle);
        W->GetTimerManager().ClearTimer(OpenerSweepHandle);
    }
    UnregisterCorpseHazard();
    Openers.Empty();
    Super::EndPlay(EndPlayReason);
}


void AMythicCorpse::RegisterCorpseHazard() {
    if (bHazardRegistered || !HasAuthority()) {
        return;
    }
    if (const UWorld *W = GetWorld()) {
        if (UMythicCorpseHazardSubsystem *Hazard = W->GetSubsystem<UMythicCorpseHazardSubsystem>()) {
            Hazard->RegisterCorpse(this);
            bHazardRegistered = true;
        }
    }
}

void AMythicCorpse::UnregisterCorpseHazard() {
    if (!bHazardRegistered) {
        return;
    }
    if (const UWorld *W = GetWorld()) {
        if (UMythicCorpseHazardSubsystem *Hazard = W->GetSubsystem<UMythicCorpseHazardSubsystem>()) {
            Hazard->UnregisterCorpse(this);
        }
    }
    bHazardRegistered = false;
}

void AMythicCorpse::MuteHazardSignals() {
    bHazardSignalsMuted = true;
}

AController *AMythicCorpse::ResolveController(AActor *Interactor) {
    if (AController *C = Cast<AController>(Interactor)) {
        return C;
    }
    if (const APawn *P = Cast<APawn>(Interactor)) {
        return P->GetController();
    }
    return nullptr;
}


void AMythicCorpse::ResolveEffectiveConfig() {
    const float BaseLifetime = Config ? Config->DecayLifetime : 300.0f;
    const float PerTier = Config ? Config->DecayLifetimePerTier : 120.0f;
    EffectiveThresholds = Config ? Config->StageThresholds : TArray<float>({60.0f, 150.0f, 240.0f});
    EffectiveMaxRaisableStage = Config ? Config->MaxRaisableStage : EMythicDecompStage::Decayed;
    bRaisable = Config ? Config->bRaisable : true;
    EffectiveDecayLifetime = FMythicCorpseRules::DecayLifetimeForTier(SourceTier, BaseLifetime, PerTier);
}

float AMythicCorpse::GetAgeSeconds() const {
    const UWorld *W = GetWorld();
    const float Now = W ? W->GetTimeSeconds() : 0.0f;
    return FMath::Max(0.0f, Now - DeathTime);
}

void AMythicCorpse::ServerInitializeFromDeath(const FMythicCorpseIdentity &Identity, int32 Tier, const FTransform &DeathTransform,
                                              UMythicInventoryComponent *ContentsToAbsorb) {
    if (!HasAuthority()) {
        return;
    }
    SetActorTransform(DeathTransform);

    SourceNameHash = Identity.SourceNameHash;
    Faction = Identity.Faction;
    RoleTag = Identity.RoleTag;
    SourceKind = Identity.SourceKind;
    SourceTier = (Tier != 0) ? Tier : Identity.SourceTier;
    KillContext = Identity.KillContext;

    ResolveEffectiveConfig();

    bSkinnable = SourceKind.MatchesTag(AI_KIND_CREATURE);
    bSkinned = false;

    bAlreadyRaised = false;
    SetDecompStage(EMythicDecompStage::Fresh);
    if (const UWorld *W = GetWorld()) {
        DeathTime = W->GetTimeSeconds();
    }

    if (ContentsToAbsorb && ContainerInventory) {
        ContentsToAbsorb->ServerDepositAll(ContainerInventory, FGameplayTag());
    }

    bDecayStarted = false;
    AdvanceDecay();
    bDecayStarted = true;

    RegisterCorpseHazard();

    FlushNetDormancy();
}

void AMythicCorpse::AdvanceDecay() {
    if (!HasAuthority()) {
        return;
    }
    UWorld *W = GetWorld();
    if (!W) {
        return;
    }

    const float Age = GetAgeSeconds();
    if (Age >= EffectiveDecayLifetime) {
        Destroy();
        return;
    }

    const EMythicDecompStage AgeStage = FMythicCorpseRules::StageForAge(Age, EffectiveThresholds);
    if (static_cast<uint8>(AgeStage) > static_cast<uint8>(DecompStage)) {
        SetDecompStage(AgeStage);
    }

    float NextEventTime = EffectiveDecayLifetime;
    for (const float T : EffectiveThresholds) {
        if (T > Age) {
            NextEventTime = FMath::Min(NextEventTime, T);
            break;
        }
    }
    const float Delay = FMath::Max(0.05f, NextEventTime - Age);
    W->GetTimerManager().SetTimer(DecayTimerHandle, this, &AMythicCorpse::AdvanceDecay, Delay,false);
}

void AMythicCorpse::SetDecompStage(EMythicDecompStage NewStage) {
    if (DecompStage == NewStage) {
        return;
    }
    DecompStage = NewStage;
    OnDecompStageChanged(DecompStage);
    FlushNetDormancy();
}

void AMythicCorpse::OnRep_DecompStage() {
    OnDecompStageChanged(DecompStage);
}

bool AMythicCorpse::CanBeRaised() const {
    const EMythicDecompStage MaxStage = Config ? Config->MaxRaisableStage : EMythicDecompStage::Decayed;
    const bool bConfigAllows = Config ? Config->bRaisable : true;
    return bConfigAllows && bRaisable && FMythicCorpseRules::IsRaisable(DecompStage, MaxStage, bAlreadyRaised);
}

void AMythicCorpse::ServerMarkRaised() {
    if (!HasAuthority()) {
        return;
    }
    bAlreadyRaised = true;
    MuteHazardSignals();
    FlushNetDormancy();
}

void AMythicCorpse::ServerBurnCorpse() {
    if (!HasAuthority()) {
        return;
    }
    UnregisterCorpseHazard();
    Destroy();
}

void AMythicCorpse::ServerNotifyLooted() {
    if (!HasAuthority()) {
        return;
    }
    MuteHazardSignals();
    FlushNetDormancy();
}

void AMythicCorpse::ServerMarkSkinned() {
    if (!HasAuthority()) {
        return;
    }
    bSkinned = true;
    FlushNetDormancy();
}


TArray<UMythicInventoryComponent *> AMythicCorpse::GetAllInventoryComponents() const {
    return {ContainerInventory};
}

UAbilitySystemComponent *AMythicCorpse::GetSchematicsASC() const {
    return nullptr;
}


void AMythicCorpse::OnPrimaryInteract_Implementation(AActor *Interactor) {
    const bool bLootable = Config ? Config->bLootable : true;
    if (!bLootable) {
        return;
    }

    AMythicPlayerController *PC = Cast<AMythicPlayerController>(ResolveController(Interactor));
    if (!PC) {
        return;
    }

    if (HasAuthority()) {
        if (IsActorInRange(PC->GetPawn())) {
            Server_AddOpener(PC);
        }
    }
    else {
        PC->ServerInteractPrimary(this);
    }

    if (PC->IsLocalController()) {
        OnCorpseOpened(PC);
    }
}

void AMythicCorpse::OnSecondaryInteract_Implementation(AActor *Interactor) {
    if (!IsSkinnable() || IsSkinned()) {
        return;
    }
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(ResolveController(Interactor));
    if (!PC) {
        return;
    }
}

USceneComponent *AMythicCorpse::GetWidgetAttachmentComponent_Implementation() const {
    return SceneRoot;
}

bool AMythicCorpse::GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const {
    OutInteractionData.InputActionDataTable = InputActionDataTable;
    OutInteractionData.PrimaryInteractionName = PrimaryInteractionName;
    if (IsSkinnable() && !IsSkinned()) {
        OutInteractionData.SecondaryInteractionName = SkinInteractionName;
    }
    return true;
}

void AMythicCorpse::OnFocused_Implementation(AActor *Interactor) {
}

void AMythicCorpse::OnUnfocused_Implementation(AActor *Interactor) {
}

bool AMythicCorpse::IsActorInRange(const AActor *Actor) const {
    if (ServerUseRangeSq <= 0.0f) {
        return true;
    }
    if (!Actor) {
        return false;
    }
    return FVector::DistSquared(Actor->GetActorLocation(), GetActorLocation()) <= ServerUseRangeSq;
}

void AMythicCorpse::Server_AddOpener(AMythicPlayerController *PC) {
    if (PC && HasAuthority()) {
        Openers.Add(PC);
        SetNetDormancy(DORM_Awake);
        if (UWorld *W = GetWorld()) {
            if (!W->GetTimerManager().IsTimerActive(OpenerSweepHandle)) {
                W->GetTimerManager().SetTimer(OpenerSweepHandle, this, &AMythicCorpse::Server_SweepOpeners,
                                              5.0f,true);
            }
        }
    }
}

bool AMythicCorpse::Server_IsOpener(const AMythicPlayerController *PC) const {
    return PC && Openers.Contains(const_cast<AMythicPlayerController *>(PC));
}

void AMythicCorpse::Server_BeginChannelLock() {
    if (!HasAuthority()) {
        return;
    }
    ++ActiveChannelRefs;
}

void AMythicCorpse::Server_EndChannelLock() {
    if (!HasAuthority()) {
        return;
    }
    ActiveChannelRefs = FMath::Max(0, ActiveChannelRefs - 1);
}

void AMythicCorpse::Server_RemoveOpener(AMythicPlayerController *PC) {
    if (PC) {
        Openers.Remove(PC);
    }
    for (auto It = Openers.CreateIterator(); It; ++It) {
        if (!It->IsValid()) {
            It.RemoveCurrent();
        }
    }
    if (HasAuthority() && Openers.Num() == 0) {
        if (UWorld *W = GetWorld()) {
            W->GetTimerManager().ClearTimer(OpenerSweepHandle);
        }
        if (NetDormancy != DORM_DormantAll) {
            SetNetDormancy(DORM_DormantAll);
            FlushNetDormancy();
        }
    }
}

void AMythicCorpse::Server_SweepOpeners() {
    if (!HasAuthority()) {
        return;
    }
    for (auto It = Openers.CreateIterator(); It; ++It) {
        const AMythicPlayerController *PC = It->Get();
        if (!PC || !IsActorInRange(PC->GetPawn())) {
            It.RemoveCurrent();
        }
    }
    if (Openers.Num() == 0) {
        if (UWorld *W = GetWorld()) {
            W->GetTimerManager().ClearTimer(OpenerSweepHandle);
        }
        if (NetDormancy != DORM_DormantAll) {
            SetNetDormancy(DORM_DormantAll);
            FlushNetDormancy();
        }
    }
}


void AMythicCorpse::SerializeCustomData(TArray<uint8> &OutCustomData) {
    if (!ContainerInventory) {
        return;
    }
    FSerializedInventoryData Data;
    if (!FSerializedInventoryData::Serialize(ContainerInventory, Data)) {
        UE_LOG(MythSaveLoad, Error, TEXT("Corpse inventory serialization failed"));
        OutCustomData.Reset();
        return;
    }

    FMemoryWriter MemWriter(OutCustomData);
    FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
    FSerializedInventoryData::StaticStruct()->SerializeItem(Ar, &Data, nullptr);
}

void AMythicCorpse::DeserializeCustomData(const TArray<uint8> &InCustomData) {
    if (ContainerInventory && InCustomData.Num() > 0) {
        FMemoryReader MemReader(InCustomData);
        FObjectAndNameAsStringProxyArchive Ar(MemReader, false);

        FSerializedInventoryData Data;
        FSerializedInventoryData::StaticStruct()->SerializeItem(Ar, &Data, nullptr);

        if (Ar.IsError() || !FSerializedInventoryData::Deserialize(ContainerInventory, Data)) {
            UE_LOG(MythSaveLoad, Error, TEXT("Corpse inventory restore failed closed"));
        }
    }

    if (HasAuthority() && !bDecayStarted) {
        ResolveEffectiveConfig();
        if (const UWorld *W = GetWorld()) {
            const float Now = W->GetTimeSeconds();
            if (DeathTime <= 0.0f || DeathTime > Now) {
                DeathTime = Now;
            }
        }
        AdvanceDecay();
        bDecayStarted = true;
    }
    RegisterCorpseHazard();
}
