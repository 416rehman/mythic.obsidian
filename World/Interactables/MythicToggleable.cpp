
#include "MythicToggleable.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Itemization/InventoryProviderInterface.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Net/UnrealNetwork.h"
#include "Player/MythicPlayerController.h"
#include "Player/Proficiency/ProficiencyComponent.h"
#include "Player/Proficiency/ProficiencyDefinition.h"
#include "World/Ownership/MythicOwnership.h"

AMythicToggleable::AMythicToggleable() {
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    NetDormancy = DORM_DormantAll;
    SetNetCullDistanceSquared(FMath::Square(6000.f));

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(SceneRoot);
}

void AMythicToggleable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AMythicToggleable, bIsOn);
    DOREPLIFETIME(AMythicToggleable, bLocked);
}

void AMythicToggleable::BeginPlay() {
    Super::BeginPlay();
    if (HasAuthority() && bStartsOn && !bIsOn) {
        bIsOn = true;
    }
    OnToggleVisualChanged(bIsOn);
    OnLockStateChanged(bLocked);
}

FMythicToggleOutcome AMythicToggleable::ResolveToggle(const bool bCurrentlyOn, const bool bLocked, const bool bOneShot, const bool bHasActivated) {
    FMythicToggleOutcome Outcome;
    Outcome.bNewIsOn = bCurrentlyOn;

    if (bLocked) {
        return Outcome;
    }
    if (bOneShot && bHasActivated) {
        return Outcome;
    }
    const bool bTarget = bOneShot ? true : !bCurrentlyOn;
    Outcome.bNewIsOn = bTarget;
    Outcome.bChanged = (bTarget != bCurrentlyOn);
    return Outcome;
}

bool AMythicToggleable::DoesKeyOpenLock(const FGameplayTagContainer &KeyTypeProbe, const FGameplayTag &RequiredKeyTag) {
    return RequiredKeyTag.IsValid() && KeyTypeProbe.HasTag(RequiredKeyTag);
}

FMythicUnlockOutcome AMythicToggleable::PlanKeyedUnlock(bool bLocked, bool bHasMatchingKey, bool bConsumeKey) {
    FMythicUnlockOutcome Outcome;
    if (bLocked && bHasMatchingKey) {
        Outcome.bUnlock = true;
        Outcome.bConsumeKey = bConsumeKey;
    }
    return Outcome;
}

UMythicItemInstance *AMythicToggleable::FindMatchingKey(AActor *Interactor) const {
    if (!RequiredKeyTag.IsValid()) {
        return nullptr;
    }
    AController *Controller = Cast<AController>(Interactor);
    if (!Controller) {
        if (const APawn *Pawn = Cast<APawn>(Interactor)) {
            Controller = Pawn->GetController();
        }
    }
    const IInventoryProviderInterface *Provider = Cast<IInventoryProviderInterface>(Controller);
    if (!Provider) {
        return nullptr;
    }

    FGameplayTagContainer Probe;
    for (UMythicInventoryComponent *Inventory : Provider->GetAllInventoryComponents()) {
        if (!Inventory) {
            continue;
        }
        for (const FMythicInventorySlotEntry &Slot : Inventory->GetAllSlots()) {
            if (UMythicItemInstance *Item = Slot.SlottedItemInstance) {
                Probe.Reset();
                Item->GetTypeProbe(Probe);
                if (DoesKeyOpenLock(Probe, RequiredKeyTag)) {
                    return Item;
                }
            }
        }
    }
    return nullptr;
}

bool AMythicToggleable::ServerTryUnlockWithKey(AActor *Interactor) {
    if (!HasAuthority()) {
        return false;
    }
    if (!bLocked) {
        return true;
    }
    UMythicItemInstance *Key = FindMatchingKey(Interactor);
    const FMythicUnlockOutcome Plan = PlanKeyedUnlock(bLocked, Key != nullptr, bConsumeKeyOnUnlock);
    if (!Plan.bUnlock) {
        return false;
    }

    bLocked = false;
    FlushNetDormancy();
    OnLockStateChanged(false);
    if (Plan.bConsumeKey && Key) {
        Key->ConsumeItem(1);
    }
    return true;
}

float AMythicToggleable::ComputePickSuccessChance(const int32 SkillLevel, const int32 LockDifficulty) {
    constexpr float BaseChance = 0.5f;
    constexpr float PerLevelStep = 0.05f;
    constexpr float MinChance = 0.05f;
    constexpr float MaxChance = 0.95f;
    const float Raw = BaseChance + PerLevelStep * static_cast<float>(SkillLevel - LockDifficulty);
    return FMath::Clamp(Raw, MinChance, MaxChance);
}

bool AMythicToggleable::ResolvePickLock(const int32 SkillLevel, const int32 LockDifficulty, const float Roll01) {
    return Roll01 < ComputePickSuccessChance(SkillLevel, LockDifficulty);
}

int32 AMythicToggleable::ResolveLockpickLevel(AActor *Interactor) const {
    if (!LockpickProficiency) {
        return 0;
    }
    AController *Controller = Cast<AController>(Interactor);
    if (!Controller) {
        if (const APawn *Pawn = Cast<APawn>(Interactor)) {
            Controller = Pawn->GetController();
        }
    }
    const AMythicPlayerController *PC = Cast<AMythicPlayerController>(Controller);
    if (!PC) {
        return 0;
    }
    const UProficiencyComponent *Prof = PC->GetProficiencyComponent();
    if (!Prof) {
        return 0;
    }
    for (int32 i = 0; i < Prof->Proficiencies.Num(); ++i) {
        if (Prof->Proficiencies[i].Definition == LockpickProficiency) {
            return Prof->GetSummary(i).Level;
        }
    }
    return 0;
}

bool AMythicToggleable::ServerTryPickLock(AActor *Interactor) {
    if (!HasAuthority()) {
        return false;
    }
    if (!bLocked) {
        return true;
    }
    if (!bLockPickable) {
        return false;
    }

    if (UMythicOwnershipComponent *Ownership = FindComponentByClass<UMythicOwnershipComponent>()) {
        Ownership->TrySubmitTheft(Interactor);
    }

    const int32 SkillLevel = ResolveLockpickLevel(Interactor);
    if (!ResolvePickLock(SkillLevel, LockpickDifficulty, FMath::FRand())) {
        return false;
    }

    bLocked = false;
    FlushNetDormancy();
    OnLockStateChanged(false);
    return true;
}

void AMythicToggleable::ServerToggle(AActor *Interactor) {
    if (!HasAuthority()) {
        return;
    }
    if (bLocked) {
        const bool bUnlockedByKey = ServerTryUnlockWithKey(Interactor);
        if (!bUnlockedByKey && bLockPickable) {
            ServerTryPickLock(Interactor);
        }
    }
    const FMythicToggleOutcome Outcome = ResolveToggle(bIsOn, bLocked, bOneShot, bHasActivated);
    if (bOneShot && Outcome.bChanged) {
        bHasActivated = true;
    }
    if (!Outcome.bChanged) {
        return;
    }
    ApplyState(Outcome.bNewIsOn);

    for (const TObjectPtr<AMythicToggleable> &Linked : LinkedToggleables) {
        if (Linked && Linked != this) {
            Linked->ApplyState(Outcome.bNewIsOn);
        }
    }
}

void AMythicToggleable::ApplyState(const bool bNewIsOn) {
    if (!HasAuthority() || bIsOn == bNewIsOn) {
        return;
    }
    bIsOn = bNewIsOn;
    FlushNetDormancy();
    OnToggleVisualChanged(bIsOn);
}

void AMythicToggleable::OnRep_IsOn() {
    OnToggleVisualChanged(bIsOn);
}

void AMythicToggleable::OnRep_Locked() {
    OnLockStateChanged(bLocked);
}

void AMythicToggleable::DeserializeCustomData(const TArray<uint8> &) {
    if (HasAuthority()) {
        FlushNetDormancy();
    }
    OnToggleVisualChanged(bIsOn);
    OnLockStateChanged(bLocked);
}

void AMythicToggleable::OnPrimaryInteract_Implementation(AActor *Interactor) {
    if (HasAuthority()) {
        ServerToggle(Interactor);
        return;
    }
    if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(Interactor)) {
        if (PC->IsLocalController()) {
            PC->ServerInteractPrimary(this);
        }
    }
}

void AMythicToggleable::OnSecondaryInteract_Implementation(AActor *Interactor) {
}

USceneComponent *AMythicToggleable::GetWidgetAttachmentComponent_Implementation() const {
    return SceneRoot;
}

bool AMythicToggleable::GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const {
    OutInteractionData.InputActionDataTable = InputActionDataTable;
    OutInteractionData.PrimaryInteractionName = PrimaryInteractionName;
    return true;
}

void AMythicToggleable::OnFocused_Implementation(AActor *Interactor) {
}

void AMythicToggleable::OnUnfocused_Implementation(AActor *Interactor) {
}
