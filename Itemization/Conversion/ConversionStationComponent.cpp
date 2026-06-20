#include "ConversionStationComponent.h"

#include "AbilitySystemComponent.h"
#include "ConversionRecipe.h"
#include "ConversionSubsystem.h"
#include "Mythic.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameStateBase.h"
#include "Interaction/MythicInteractionComponent.h"
#include "Itemization/InventoryProviderInterface.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/Fragments/Passive/DurabilityFragment.h"
#include "Itemization/Loot/MythicLootManagerSubsystem.h"
#include "Itemization/MythicTags_Conversion.h"

namespace {
    constexpr double kMinRequestInterval = 0.2;
}

// ============================ FastArray ============================

int32 FConversionJobArray::AddJob(const FGameplayTag &RecipeId, int32 Quantity, int32 JobId, int32 SnapshotInputLevel) {
    FConversionJobEntry Entry;
    Entry.RecipeId = RecipeId;
    Entry.Quantity = Quantity;
    Entry.JobId = JobId;
    Entry.SnapshotInputLevel = SnapshotInputLevel;
    Entry.State = EConversionJobState::Pending;
    const int32 Index = Items.Add(Entry);
    MarkItemDirty(Items[Index]);
    return Index;
}

void FConversionJobArray::RemoveAt(int32 Index) {
    if (!Items.IsValidIndex(Index)) {
        return;
    }
    Items.RemoveAt(Index);
    MarkArrayDirty();
}

FConversionJobEntry *FConversionJobArray::EditHead() {
    return Items.Num() > 0 ? &Items[0] : nullptr;
}

FConversionJobEntry *FConversionJobArray::FindById(int32 JobId) {
    for (FConversionJobEntry &E : Items) {
        if (E.JobId == JobId) {
            return &E;
        }
    }
    return nullptr;
}

int32 FConversionJobArray::IndexOfId(int32 JobId) const {
    for (int32 i = 0; i < Items.Num(); i++) {
        if (Items[i].JobId == JobId) {
            return i;
        }
    }
    return INDEX_NONE;
}

void FConversionJobArray::MarkDirtyAt(int32 Index) {
    if (Items.IsValidIndex(Index)) {
        MarkItemDirty(Items[Index]);
    }
}

void FConversionJobArray::PreReplicatedRemove(const TArrayView<int32> &R, int32 N) {
    if (Owner) { Owner->NotifyJobsReplicated(); }
}

void FConversionJobArray::PostReplicatedAdd(const TArrayView<int32> &A, int32 N) {
    if (Owner) { Owner->NotifyJobsReplicated(); }
}

void FConversionJobArray::PostReplicatedChange(const TArrayView<int32> &C, int32 N) {
    if (Owner) { Owner->NotifyJobsReplicated(); }
}

// ============================ Component ============================

UConversionStationComponent::UConversionStationComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);

    InputGroupTag = INVENTORY_GROUP_STATION_INPUT;
    FuelGroupTag = INVENTORY_GROUP_STATION_FUEL;
    CatalystGroupTag = INVENTORY_GROUP_STATION_CATALYST;
    OutputGroupTag = INVENTORY_GROUP_STATION_OUTPUT;
}

void UConversionStationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UConversionStationComponent, Jobs);
    DOREPLIFETIME(UConversionStationComponent, FuelState);
}

bool UConversionStationComponent::HasAuthority() const {
    return GetOwner() && GetOwner()->HasAuthority();
}

double UConversionStationComponent::ServerNow() const {
    if (const UWorld *W = GetWorld()) {
        if (const AGameStateBase *GS = W->GetGameState()) {
            return GS->GetServerWorldTimeSeconds();
        }
    }
    return 0.0;
}

UMythicInventoryComponent *UConversionStationComponent::GetStationInventory() const {
    return StationInv.Get();
}

void UConversionStationComponent::BeginPlay() {
    Super::BeginPlay();

    Jobs.SetOwner(this);

    if (const UGameInstance *GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr) {
        Subsystem = GI->GetSubsystem<UConversionSubsystem>();
    }

    // Resolve the station inventory from the owning actor's provider interface.
    if (IInventoryProviderInterface *Provider = Cast<IInventoryProviderInterface>(GetOwner())) {
        const TArray<UMythicInventoryComponent *> Invs = Provider->GetAllInventoryComponents();
        if (Invs.Num() > 0) {
            StationInv = Invs[0];
        }
    }

    if (!HasAuthority()) {
        return;
    }

    // Single source for the use-range gate: the interaction range (+ tolerance).
    const float BaseRange = GetDefault<UMythicInteractionComponent>()->InteractionRange;
    ServerUseRangeSq = FMath::Square(BaseRange + UseRangeTolerance);

    if (StationMode == EConversionTrigger::Automatic) {
        if (StationInv.IsValid()) {
            StationInv->OnSlotUpdated.AddDynamic(this, &UConversionStationComponent::HandleStationInventoryChanged);
        }
        if (Subsystem.IsValid() && Subsystem->AreRecipesReady()) {
            ScheduleAutoEnqueue();
        }
        else if (Subsystem.IsValid()) {
            Subsystem->OnRecipesReady.RemoveAll(this);
            Subsystem->OnRecipesReady.AddUObject(this, &UConversionStationComponent::HandleRecipesReady);
        }
    }
}

void UConversionStationComponent::EndPlay(const EEndPlayReason::Type Reason) {
    if (HasAuthority() && Reason != EEndPlayReason::LevelTransition) {
        // Refund every outstanding job so nothing is lost on teardown.
        for (const FConversionJobEntry &Job : Jobs.GetItems()) {
            RefundJob(Job);
        }
        // Route back any held transform instances not tied to a refunded job (defensive).
        for (FHeldTransform &Held : HeldTransforms) {
            if (IsValid(Held.Instance)) {
                RouteInstance(Held.Instance, EConversionOutputRouting::DropInWorld, FConversionJobEntry());
            }
        }
        HeldTransforms.Reset();
    }

    if (Subsystem.IsValid()) {
        Subsystem->OnRecipesReady.RemoveAll(this);
    }

    if (GetWorld()) {
        GetWorld()->GetTimerManager().ClearTimer(ProcessTimerHandle);
        GetWorld()->GetTimerManager().ClearTimer(AutoEnqueueTimerHandle);
    }

    Super::EndPlay(Reason);
}

void UConversionStationComponent::NotifyJobsReplicated() {
    OnJobsChanged.Broadcast();
}

void UConversionStationComponent::NotifyFuelReplicated() {
    OnFuelChanged.Broadcast();
}

void UConversionStationComponent::OnRep_Fuel() {
    OnFuelChanged.Broadcast();
}

void UConversionStationComponent::HandleRecipesReady() {
    if (HasAuthority() && StationMode == EConversionTrigger::Automatic) {
        ScheduleAutoEnqueue();
    }
    // A stalled head waiting on NotLoaded should retry now that recipes resolved.
    AdvanceProcessing();
}

bool UConversionStationComponent::IsActorInRange(const AActor *A) const {
    if (!A || !GetOwner()) {
        return false;
    }
    if (ServerUseRangeSq <= 0.f) {
        return true; // not configured -> do not block
    }
    return FVector::DistSquared(A->GetActorLocation(), GetOwner()->GetActorLocation()) <= ServerUseRangeSq;
}

AController *UConversionStationComponent::ResolveInstigatorController(int32 JobId) const {
    if (const FJobRefundInfo *Info = JobRefundTargets.Find(JobId)) {
        return Info->Controller.Get();
    }
    return nullptr;
}

void UConversionStationComponent::GetSourceInventories(AController *JobController, TArray<UMythicInventoryComponent *> &OutInvs,
                                                       FGameplayTag &OutInputGroup, FGameplayTag &OutCatalystGroup) const {
    OutInvs.Reset();
    if (JobController) {
        if (IInventoryProviderInterface *Prov = Cast<IInventoryProviderInterface>(JobController)) {
            OutInvs = Prov->GetAllInventoryComponents();
        }
        OutInputGroup = FGameplayTag(); // all player slots
        OutCatalystGroup = FGameplayTag();
    }
    else {
        if (StationInv.IsValid()) {
            OutInvs.Add(StationInv.Get());
        }
        OutInputGroup = InputGroupTag;
        OutCatalystGroup = CatalystGroupTag;
    }
}

void UConversionStationComponent::GatherInstances(const TArray<UMythicInventoryComponent *> &Invs, const FGameplayTag &GroupFilter,
                                                  TArray<UMythicItemInstance *> &Out) const {
    for (UMythicInventoryComponent *Inv : Invs) {
        if (!Inv) {
            continue;
        }
        for (const FMythicInventorySlotEntry &Slot : Inv->GetAllSlots()) {
            if (!Slot.SlottedItemInstance) {
                continue;
            }
            if (GroupFilter.IsValid() && Slot.GroupTag != GroupFilter) {
                continue;
            }
            Out.Add(Slot.SlottedItemInstance);
        }
    }
}

bool UConversionStationComponent::VerifyInputs(UConversionRecipe *R, AController *JobController, int32 Cycles, bool bCheckCatalysts) const {
    if (!R) {
        return false;
    }
    TArray<UMythicInventoryComponent *> Invs;
    FGameplayTag InGroup, CatGroup;
    GetSourceInventories(JobController, Invs, InGroup, CatGroup);

    TArray<UMythicItemInstance *> Insts;
    GatherInstances(Invs, InGroup, Insts);

    for (const FConversionIngredient &Ing : R->Inputs) {
        if (!Ing.bConsumed) {
            continue;
        }
        const int32 Need = Ing.RequiredAmount * Cycles;
        int32 Have = 0;
        for (UMythicItemInstance *I : Insts) {
            if (I && Ing.MatchesInstance(I)) {
                Have += I->GetStacks();
            }
        }
        if (Have < Need) {
            return false;
        }
    }

    if (bCheckCatalysts) {
        return VerifyCatalystsPresent(R, JobController);
    }
    return true;
}

bool UConversionStationComponent::VerifyCatalystsPresent(UConversionRecipe *R, AController *JobController) const {
    if (!R) {
        return false;
    }
    TArray<UMythicInventoryComponent *> Invs;
    FGameplayTag InGroup, CatGroup;
    GetSourceInventories(JobController, Invs, InGroup, CatGroup);

    TArray<UMythicItemInstance *> Insts;
    GatherInstances(Invs, CatGroup, Insts);

    for (const FConversionIngredient &Ing : R->Inputs) {
        if (Ing.bConsumed) {
            continue;
        }
        int32 Have = 0;
        for (UMythicItemInstance *I : Insts) {
            if (I && Ing.MatchesInstance(I)) {
                Have += I->GetStacks();
            }
        }
        if (Have < Ing.RequiredAmount) {
            return false;
        }
    }
    return true;
}

bool UConversionStationComponent::ConsumeInputs(UConversionRecipe *R, AController *JobController, int32 Cycles, int32 JobId, int32 &OutSnapshotLevel) {
    OutSnapshotLevel = 0;
    if (!R) {
        return false;
    }

    TArray<UMythicInventoryComponent *> Invs;
    FGameplayTag InGroup, CatGroup;
    GetSourceInventories(JobController, Invs, InGroup, CatGroup);

    TArray<UMythicItemInstance *> Insts;
    GatherInstances(Invs, InGroup, Insts);

    // Atomic verify first so we never partially consume.
    for (const FConversionIngredient &Ing : R->Inputs) {
        if (!Ing.bConsumed) {
            continue;
        }
        const int32 Need = Ing.RequiredAmount * Cycles;
        int32 Have = 0;
        for (UMythicItemInstance *I : Insts) {
            if (I && Ing.MatchesInstance(I)) {
                Have += I->GetStacks();
            }
        }
        if (Have < Need) {
            return false;
        }
    }

    // Pair each Transform product with the consumed input it will transform (released + held, not destroyed). For
    // a bRepairToFull product the paired input MUST carry a UDurabilityFragment (the durable item being repaired):
    // otherwise the repair would no-op on the wrong instance AND the real durable item would fall into the
    // destroy branch below — the exact data-loss the embodiment-arc review caught. Selecting by which matched
    // instance actually has durability (rather than raw input order) makes authoring order irrelevant. Plain
    // transforms take the first free consumed input (prior behavior). IsDataValid enforces this at author time;
    // this is the runtime guard, aborting BEFORE any consumption if a repair product has no durable input.
    TSet<int32> TransformInputIdx;
    for (const FConversionProduct &P : R->Products) {
        if (P.Mode != EConversionProductMode::Transform) {
            continue;
        }
        int32 ChosenIdx = INDEX_NONE;
        for (int32 ii = 0; ii < R->Inputs.Num(); ii++) {
            if (!R->Inputs[ii].bConsumed || TransformInputIdx.Contains(ii)) {
                continue;
            }
            if (P.bRepairToFull) {
                bool bHasDurable = false;
                for (UMythicItemInstance *I : Insts) {
                    if (IsValid(I) && R->Inputs[ii].MatchesInstance(I) && I->GetFragment<UDurabilityFragment>()) {
                        bHasDurable = true;
                        break;
                    }
                }
                if (!bHasDurable) {
                    continue;
                }
            }
            ChosenIdx = ii;
            break;
        }
        if (ChosenIdx == INDEX_NONE) {
            UE_LOG(Myth, Error, TEXT("ConversionStation: transform/repair product has no suitable consumed input "
                       "(repair requires a durable item); aborting job %d before consuming."), JobId);
            return false;
        }
        TransformInputIdx.Add(ChosenIdx);
    }

    // Reset the reservation record for this job/cycle.
    FJobReservation &Res = JobReservations.FindOrAdd(JobId);
    Res = FJobReservation{};
    Res.ReservedCycles = Cycles;

    bool bAnyLevel = false;
    int32 MinLevel = MAX_int32;
    auto NoteLevel = [&](const UMythicItemInstance *I) {
        bAnyLevel = true;
        MinLevel = FMath::Min(MinLevel, I->GetItemLevel());
    };

    for (int32 ii = 0; ii < R->Inputs.Num(); ii++) {
        const FConversionIngredient &Ing = R->Inputs[ii];
        if (!Ing.bConsumed) {
            continue;
        }
        const bool bTransform = TransformInputIdx.Contains(ii);
        int32 Remaining = Ing.RequiredAmount * Cycles;

        for (int32 si = 0; si < Insts.Num() && Remaining > 0; si++) {
            UMythicItemInstance *I = Insts[si];
            if (!IsValid(I) || !Ing.MatchesInstance(I)) {
                continue;
            }

            if (bTransform) {
                // Release whole (non-stackable) instances and hold them for the transform step.
                UMythicInventoryComponent *OwnInv = I->GetInventoryComponent();
                const int32 SlotIdx = I->GetSlot();
                if (!OwnInv || SlotIdx == INDEX_NONE) {
                    continue;
                }
                NoteLevel(I);
                if (UMythicItemInstance *Released = OwnInv->ReleaseFromSlot(SlotIdx)) {
                    FHeldTransform Held;
                    Held.JobId = JobId;
                    Held.Instance = Released;
                    HeldTransforms.Add(Held);
                    Remaining -= 1;
                }
                Insts[si] = nullptr;
            }
            else {
                const int32 Take = FMath::Min(Remaining, I->GetStacks());
                NoteLevel(I);

                // Record per-job reserved totals for dupe-safe refund.
                bool bMerged = false;
                for (FReservedRefundEntry &E : Res.Items) {
                    if (E.Def.Get() == I->GetItemDefinition()) {
                        E.TotalQty += Take;
                        bMerged = true;
                        break;
                    }
                }
                if (!bMerged) {
                    FReservedRefundEntry E;
                    E.Def = I->GetItemDefinition();
                    E.TotalQty = Take;
                    Res.Items.Add(E);
                }

                if (UMythicInventoryComponent *OwnInv = I->GetInventoryComponent()) {
                    OwnInv->ServerRemoveItem(I, Take);
                }
                else {
                    I->ConsumeItem(Take);
                }
                Remaining -= Take;
                if (!IsValid(I)) {
                    Insts[si] = nullptr;
                }
            }
        }

        if (Remaining > 0) {
            UE_LOG(Myth, Error, TEXT("ConversionStation: input shortfall after verify (race), job %d. Aborting + refunding."), JobId);
            // Refund everything already taken for this aborted job so nothing is lost.
            if (const FJobReservation *RR = JobReservations.Find(JobId)) {
                for (const FReservedRefundEntry &E : RR->Items) {
                    if (UItemDefinition *D = E.Def.Get()) {
                        MintTo(D, E.TotalQty, JobController, bAnyLevel ? MinLevel : 0);
                    }
                }
            }
            for (int32 hi = HeldTransforms.Num() - 1; hi >= 0; hi--) {
                if (HeldTransforms[hi].JobId == JobId) {
                    if (IsValid(HeldTransforms[hi].Instance)) {
                        RouteInstance(HeldTransforms[hi].Instance, EConversionOutputRouting::DropInWorld, FConversionJobEntry());
                    }
                    HeldTransforms.RemoveAt(hi);
                }
            }
            JobReservations.Remove(JobId);
            return false;
        }
    }

    OutSnapshotLevel = bAnyLevel ? MinLevel : 0;
    if (FJobReservation *RR = JobReservations.Find(JobId)) {
        RR->SnapshotLevel = OutSnapshotLevel; // so refunds restore the consumed input level
    }
    return true;
}

bool UConversionStationComponent::EnsureFuel(UConversionRecipe *R) {
    if (!R || !R->Process.bRequiresFuel) {
        return true;
    }
    if (FuelState.BufferedBurnSeconds >= R->Process.Duration) {
        return true;
    }
    if (!StationInv.IsValid()) {
        return false;
    }

    bool bChanged = false;
    bool bExhausted = false;
    while (FuelState.BufferedBurnSeconds < R->Process.Duration && !bExhausted) {
        UMythicItemInstance *FuelInst = nullptr;
        const FFuelDefinition *MatchedFuel = nullptr;

        for (const FMythicInventorySlotEntry &Slot : StationInv->GetAllSlots()) {
            if (!Slot.SlottedItemInstance || (FuelGroupTag.IsValid() && Slot.GroupTag != FuelGroupTag)) {
                continue;
            }
            for (const FFuelDefinition &Fuel : R->Process.AcceptedFuels) {
                if (Fuel.FuelMatch.MatchesInstance(Slot.SlottedItemInstance)) {
                    FuelInst = Slot.SlottedItemInstance;
                    MatchedFuel = &Fuel;
                    break;
                }
            }
            if (FuelInst) {
                break;
            }
        }

        if (!FuelInst || !MatchedFuel) {
            bExhausted = true;
            break;
        }

        // A fuel that yields no burn time (BurnSecondsPerUnit <= 0, authorable since ClampMin is inclusive) would
        // otherwise be consumed forever for zero progress — BufferedBurnSeconds never reaches Duration, so the loop
        // drains the entire stock. Treat it as unusable and stop rather than destroy the player's fuel for nothing.
        if (MatchedFuel->BurnSecondsPerUnit <= 0.0f) {
            bExhausted = true;
            break;
        }

        StationInv->ServerRemoveItem(FuelInst, 1);
        FuelState.BufferedBurnSeconds += MatchedFuel->BurnSecondsPerUnit;
        FuelState.CapacityHintSeconds = FMath::Max(FuelState.CapacityHintSeconds, FuelState.BufferedBurnSeconds);
        bChanged = true;
    }

    if (bChanged) {
        FuelState.LastSampleServerTime = ServerNow();
        OnFuelChanged.Broadcast();
    }

    return FuelState.BufferedBurnSeconds >= R->Process.Duration;
}

int32 UConversionStationComponent::ComputeProductLevel(const FConversionProduct &P, const FConversionJobEntry &Job) const {
    switch (P.LevelMode) {
    case EProductLevelMode::InheritInputLevel:
        return Job.SnapshotInputLevel;
    case EProductLevelMode::InheritStationLevel:
        return 1; // no station-level concept yet
    case EProductLevelMode::FixedLevel:
    default:
        return P.FixedLevel;
    }
}

void UConversionStationComponent::RouteInstance(UMythicItemInstance *Inst, EConversionOutputRouting Routing, const FConversionJobEntry &Job) {
    if (!IsValid(Inst)) {
        return;
    }
    UMythicLootManagerSubsystem *Loot = GetWorld() && GetWorld()->GetGameInstance()
        ? GetWorld()->GetGameInstance()->GetSubsystem<UMythicLootManagerSubsystem>()
        : nullptr;
    const FVector Loc = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;

    switch (Routing) {
    case EConversionOutputRouting::GiveToInstigator: {
        AController *C = ResolveInstigatorController(Job.JobId);
        IInventoryProviderInterface *Prov = C ? Cast<IInventoryProviderInterface>(C) : nullptr;
        if (Prov && C->GetPawn()) {
            UMythicInventoryComponent *Dest = Inst->GetItemDefinition()
                ? Prov->GetInventoryForItemType(Inst->GetItemDefinition()->ItemType)
                : nullptr;
            if (!Dest) {
                const TArray<UMythicInventoryComponent *> Invs = Prov->GetAllInventoryComponents();
                Dest = Invs.Num() > 0 ? Invs[0] : nullptr;
            }
            if (Dest) {
                Dest->AddItem(Inst, C); // adds to a slot; overflow drops as a world item targeted to C
                return;
            }
        }
        if (Loot) { Loot->Spawn(Inst, Loc, 100.f, nullptr); }
        return;
    }
    case EConversionOutputRouting::DropInWorld: {
        if (Loot) { Loot->Spawn(Inst, Loc, 100.f, nullptr); }
        return;
    }
    case EConversionOutputRouting::StationOutputSlots:
    default: {
        const int32 Original = Inst->GetStacks();
        const int32 Added = StationInv.IsValid() ? StationInv->AddToAnySlot(Inst, /*bFromPlayer=*/false) : 0;
        if (Added < Original && IsValid(Inst) && Loot) {
            // Output full or type not whitelisted -> spill the remainder to the world so production is
            // never lost. (The job keeps running; we do not stall, to avoid a phantom stall on a head
            // that is still producing.)
            Loot->Spawn(Inst, Loc, 100.f, nullptr);
            UE_LOG(Myth, Verbose, TEXT("ConversionStation: output full/blocked; spilled product to world."));
        }
        return;
    }
    }
}

void UConversionStationComponent::ProduceAndRoute(UConversionRecipe *R, const FConversionJobEntry &Job) {
    UMythicLootManagerSubsystem *Loot = GetWorld() && GetWorld()->GetGameInstance()
        ? GetWorld()->GetGameInstance()->GetSubsystem<UMythicLootManagerSubsystem>()
        : nullptr;
    if (!Loot || !R) {
        return;
    }

    // Pull this job's held transform instances (FIFO).
    TArray<UMythicItemInstance *> Held;
    for (FHeldTransform &H : HeldTransforms) {
        if (H.JobId == Job.JobId && IsValid(H.Instance)) {
            Held.Add(H.Instance);
        }
    }
    int32 TransformCursor = 0;

    for (const FConversionProduct &P : R->Products) {
        if (FMath::FRand() >= P.Probability) {
            continue;
        }

        if (P.Mode == EConversionProductMode::Create) {
            UItemDefinition *Def = P.ItemDefinition.Get();
            if (!Def) {
                UE_LOG(Myth, Warning, TEXT("ConversionStation: product item def unresolved; skipped."));
                continue;
            }
            const int32 Level = ComputeProductLevel(P, Job);
            const int32 MaxStack = Def->StackSizeMax > 0 ? Def->StackSizeMax : 1;
            int32 Remaining = P.Quantity;
            while (Remaining > 0) {
                const int32 Take = FMath::Min(Remaining, MaxStack);
                if (UMythicItemInstance *Inst = Loot->Create(Def, Take, nullptr, Level)) {
                    RouteInstance(Inst, R->Process.OutputRouting, Job);
                }
                Remaining -= Take;
            }
        }
        else { // Transform
            UMythicItemInstance *T = Held.IsValidIndex(TransformCursor) ? Held[TransformCursor] : nullptr;
            TransformCursor++;
            if (!IsValid(T)) {
                UE_LOG(Myth, Warning, TEXT("ConversionStation: transform has no held input; skipped."));
                continue;
            }
            T->ServerApplyTransform(P.NewItemType, P.TagsToAdd, P.TagsToRemove, P.TransformToDefinition.Get());
            // Repair job: restore the transformed instance's durability to full (clears the broken latch). This is
            // the recovery path for the otherwise-permanent breakage from durability wear. const_cast mirrors the
            // wear chokepoints (the only writers of the durability fragment). No-op without a durability fragment.
            if (P.bRepairToFull) {
                if (const UDurabilityFragment *Dura = T->GetFragment<UDurabilityFragment>()) {
                    const_cast<UDurabilityFragment *>(Dura)->ServerRepairFull();
                }
            }
            RouteInstance(T, R->Process.OutputRouting, Job);
        }
    }

    // Remove exactly the held instances routed this cycle, matched by identity (not by tail position).
    for (int32 c = 0; c < TransformCursor; c++) {
        UMythicItemInstance *Routed = Held[c];
        for (int32 i = HeldTransforms.Num() - 1; i >= 0; i--) {
            if (HeldTransforms[i].JobId == Job.JobId && HeldTransforms[i].Instance == Routed) {
                HeldTransforms.RemoveAt(i);
                break;
            }
        }
    }
}

void UConversionStationComponent::MintTo(UItemDefinition *Def, int32 Qty, AController *C, int32 Level) const {
    if (!Def || Qty <= 0) {
        return;
    }
    UMythicLootManagerSubsystem *Loot = GetWorld() && GetWorld()->GetGameInstance()
        ? GetWorld()->GetGameInstance()->GetSubsystem<UMythicLootManagerSubsystem>()
        : nullptr;
    if (!Loot) {
        return;
    }
    const FVector Loc = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;

    // Give to the player's inventory when they have one. CreateAndGive places the item and world-drops any
    // overflow INTERNALLY — its non-null return means "overflow handled", NOT "failed", so gating CreateAndSpawn
    // on its return (the old code) double-minted the refund on the common fit case. Gate on player-validity
    // instead: valid player -> CreateAndGive (handles fit + overflow), done; no player -> world-drop.
    if (C && Cast<IInventoryProviderInterface>(C)) {
        Loot->CreateAndGive(Def, Qty, C, C, Level);
        return;
    }
    Loot->CreateAndSpawn(Def, Loc, nullptr, Level, Qty, 100.f);
}

void UConversionStationComponent::RefundJob(const FConversionJobEntry &Job) {
    AController *C = ResolveInstigatorController(Job.JobId);

    // Route back any held (released, not destroyed) transform inputs.
    for (int32 i = HeldTransforms.Num() - 1; i >= 0; i--) {
        if (HeldTransforms[i].JobId == Job.JobId) {
            if (IsValid(HeldTransforms[i].Instance)) {
                const EConversionOutputRouting Routing = C
                    ? EConversionOutputRouting::GiveToInstigator
                    : EConversionOutputRouting::DropInWorld;
                RouteInstance(HeldTransforms[i].Instance, Routing, Job);
            }
            HeldTransforms.RemoveAt(i);
        }
    }

    const FJobReservation *Res = JobReservations.Find(Job.JobId);
    if (!Res || Res->ReservedCycles <= 0) {
        return;
    }

    UConversionRecipe *R = Subsystem.IsValid() ? Subsystem->GetRecipeById(Job.RecipeId) : nullptr;
    const bool bContinuous = R && R->Process.Timing == EConversionTiming::Continuous;

    // Continuous reserves one cycle at a time and Items is cleared the moment a cycle is produced, so a
    // non-empty Items here is always the current un-produced cycle -> refund it in full (no dupe).
    // Timed/Instant reserve all cycles up front; refund the fraction still un-produced (remaining Quantity).
    const int32 RefundCycles = bContinuous ? Res->ReservedCycles : Job.Quantity;
    if (RefundCycles <= 0) {
        return;
    }

    for (const FReservedRefundEntry &E : Res->Items) {
        if (UItemDefinition *Def = E.Def.Get()) {
            const int32 Qty = E.TotalQty * RefundCycles / FMath::Max(1, Res->ReservedCycles);
            MintTo(Def, Qty, C, Res->SnapshotLevel);
        }
    }
}

void UConversionStationComponent::ClearJobBookkeeping(int32 JobId) {
    if (const FJobRefundInfo *Info = JobRefundTargets.Find(JobId)) {
        if (AController *C = Info->Controller.Get()) {
            if (FConversionSession *S = Sessions.Find(C)) {
                S->OwnedJobs = FMath::Max(0, S->OwnedJobs - 1);
            }
        }
    }
    JobRefundTargets.Remove(JobId);
    JobReservations.Remove(JobId);
}

// ============================ Processing ============================

void UConversionStationComponent::AdvanceProcessing() {
    if (!HasAuthority() || !GetWorld()) {
        return;
    }
    if (GetWorld()->GetTimerManager().IsTimerActive(ProcessTimerHandle)) {
        return;
    }

    FConversionJobEntry *Head = Jobs.EditHead();
    if (!Head) {
        // Queue drained — nothing is burning. Clear the replicated lit flag (BeginCycle sets bBurning true for any fuel
        // recipe; previously only the NoFuel stall cleared it, so a completed/cancelled fuel job left the furnace
        // showing lit forever). Both the completion path and Server_CancelJob funnel here once the queue empties.
        if (FuelState.bBurning) {
            FuelState.bBurning = false;
            OnFuelChanged.Broadcast();
        }
        return;
    }
    if (Head->State == EConversionJobState::Processing) {
        return;
    }

    UConversionRecipe *R = Subsystem.IsValid() ? Subsystem->GetRecipeById(Head->RecipeId) : nullptr;
    if (!R) {
        Head->State = EConversionJobState::Stalled;
        Head->StallReason = EConversionStallReason::NotLoaded;
        Jobs.MarkDirtyAt(0);
        OnJobStalled.Broadcast(Head->JobId, EConversionStallReason::NotLoaded);
        OnJobsChanged.Broadcast();
        if (Subsystem.IsValid() && !Subsystem->AreRecipesReady()) {
            Subsystem->OnRecipesReady.RemoveAll(this);
            Subsystem->OnRecipesReady.AddUObject(this, &UConversionStationComponent::HandleRecipesReady);
        }
        return;
    }

    // A Continuous head resuming from a stall must re-consume this cycle's inputs (its reserved cycle was
    // never produced, but a fresh Pending head already paid at enqueue and must NOT re-consume).
    const bool bReConsume = (R->Process.Timing == EConversionTiming::Continuous
        && Head->State == EConversionJobState::Stalled);

    // Guard the whole synchronous StationInv-mutating window (EnsureFuel removes fuel, which fires
    // OnSlotUpdated -> HandleStationInventoryChanged -> AdvanceProcessing re-entrantly otherwise).
    bMutatingStationInv = true;
    const bool bStarted = BeginCycle(*Head, R, bReConsume);
    bMutatingStationInv = false;

    if (!bStarted) {
        return; // BeginCycle stalled the head
    }
    if (R->Process.Timing == EConversionTiming::Instant) {
        CompleteCurrentCycleAndContinue();
    }
    else {
        GetWorld()->GetTimerManager().SetTimer(ProcessTimerHandle, this, &UConversionStationComponent::OnCycleComplete,
                                               FMath::Max(Head->CycleDuration, UE_KINDA_SMALL_NUMBER), false);
    }
}

bool UConversionStationComponent::BeginCycle(FConversionJobEntry &Head, UConversionRecipe *R, bool bReConsume) {
    AController *C = ResolveInstigatorController(Head.JobId);

    auto Stall = [&](EConversionStallReason Reason) {
        Head.State = EConversionJobState::Stalled;
        Head.StallReason = Reason;
        Jobs.MarkDirtyAt(0);
        OnJobStalled.Broadcast(Head.JobId, Reason);
        OnJobsChanged.Broadcast();
    };

    // 1. Catalysts must be present this cycle.
    if (!VerifyCatalystsPresent(R, C)) {
        Stall(EConversionStallReason::MissingCatalyst);
        return false;
    }

    // 2. Fuel must cover this whole cycle.
    if (R->Process.bRequiresFuel && !EnsureFuel(R)) {
        FuelState.bBurning = false;
        OnFuelChanged.Broadcast();
        Stall(EConversionStallReason::NoFuel);
        return false;
    }

    // 3. Continuous re-cycles consume this cycle's inputs now (reserve-1 model). Timed/Instant reserved up front.
    if (bReConsume && R->Process.Timing == EConversionTiming::Continuous) {
        int32 SnapLevel = 0;
        if (!VerifyInputs(R, C, 1, false) || !ConsumeInputs(R, C, 1, Head.JobId, SnapLevel)) {
            Stall(EConversionStallReason::MissingInput);
            return false;
        }
        Head.SnapshotInputLevel = SnapLevel;
    }

    Head.State = EConversionJobState::Processing;
    Head.StallReason = EConversionStallReason::None;
    Head.CycleStartServerTime = ServerNow();
    Head.CycleDuration = (R->Process.Timing == EConversionTiming::Instant) ? 0.f : R->Process.Duration;
    FuelState.bBurning = R->Process.bRequiresFuel;
    Jobs.MarkDirtyAt(0);
    OnJobsChanged.Broadcast();
    return true;
}

void UConversionStationComponent::OnCycleComplete() {
    CompleteCurrentCycleAndContinue();
}

void UConversionStationComponent::CompleteCurrentCycleAndContinue() {
    if (!HasAuthority() || !GetWorld()) {
        return;
    }
    GetWorld()->GetTimerManager().ClearTimer(ProcessTimerHandle);

    int32 Safety = 0;
    while (true) {
        if (++Safety > 4096) {
            UE_LOG(Myth, Error, TEXT("ConversionStation: cycle loop guard tripped."));
            return;
        }

        FConversionJobEntry *Head = Jobs.EditHead();
        if (!Head) {
            return;
        }
        UConversionRecipe *R = Subsystem.IsValid() ? Subsystem->GetRecipeById(Head->RecipeId) : nullptr;
        if (!R) {
            Head->State = EConversionJobState::Stalled;
            Head->StallReason = EConversionStallReason::NotLoaded;
            Jobs.MarkDirtyAt(0);
            OnJobsChanged.Broadcast();
            return;
        }

        bMutatingStationInv = true;

        const FConversionJobEntry CycleSnapshot = *Head; // stable copy for production
        ProduceAndRoute(R, CycleSnapshot);

        // For Continuous (reserve-1), this cycle's reserved inputs are now produced and no longer refundable.
        // Clearing them keeps a cancel after this point from re-refunding an already-produced cycle.
        if (R->Process.Timing == EConversionTiming::Continuous) {
            if (FJobReservation *Res = JobReservations.Find(CycleSnapshot.JobId)) {
                Res->Items.Reset();
            }
        }

        // Burn fuel for the cycle just completed.
        if (R->Process.bRequiresFuel) {
            FuelState.BufferedBurnSeconds = FMath::Max(0.f, FuelState.BufferedBurnSeconds - CycleSnapshot.CycleDuration);
            FuelState.LastSampleServerTime = ServerNow();
            OnFuelChanged.Broadcast();
        }

        // The head may have moved if ProduceAndRoute spilled; re-fetch by id.
        const int32 HeadIdx = Jobs.IndexOfId(CycleSnapshot.JobId);
        Head = (HeadIdx == 0) ? Jobs.EditHead() : nullptr;
        if (!Head) {
            bMutatingStationInv = false;
            AdvanceProcessing();
            return;
        }

        const bool bUnbounded = (Head->Quantity == 0);
        if (!bUnbounded) {
            Head->Quantity = FMath::Max(0, Head->Quantity - 1);
        }
        const bool bMoreCycles = bUnbounded || Head->Quantity > 0;

        if (bMoreCycles) {
            if (BeginCycle(*Head, R, /*bReConsume=*/true)) {
                bMutatingStationInv = false;
                if (R->Process.Timing == EConversionTiming::Instant) {
                    continue; // produce the next cycle immediately
                }
                GetWorld()->GetTimerManager().SetTimer(ProcessTimerHandle, this, &UConversionStationComponent::OnCycleComplete,
                                                       FMath::Max(Head->CycleDuration, UE_KINDA_SMALL_NUMBER), false);
                return;
            }
            // BeginCycle stalled the head (missing fuel/input/catalyst).
            bMutatingStationInv = false;
            return;
        }

        // Job finished.
        const FConversionJobEntry Completed = *Head;
        Jobs.RemoveAt(0);
        ClearJobBookkeeping(Completed.JobId);
        bMutatingStationInv = false;
        OnJobCompleted.Broadcast(Completed);
        OnJobsChanged.Broadcast();
        AdvanceProcessing();
        return;
    }
}

// ============================ Auto enqueue ============================

void UConversionStationComponent::HandleStationInventoryChanged(int32 Slot) {
    // Ignore changes we caused mid-mutation; the mutating path re-schedules explicitly afterwards.
    if (!HasAuthority() || bMutatingStationInv || StationMode != EConversionTrigger::Automatic) {
        return;
    }
    ScheduleAutoEnqueue();
    // Inventory changed (e.g. output drained, fuel added) -> a stalled head may resume.
    AdvanceProcessing();
}

void UConversionStationComponent::ScheduleAutoEnqueue() {
    if (!HasAuthority() || !GetWorld()) {
        return;
    }
    if (GetWorld()->GetTimerManager().IsTimerActive(AutoEnqueueTimerHandle)) {
        return;
    }
    GetWorld()->GetTimerManager().SetTimer(AutoEnqueueTimerHandle, this, &UConversionStationComponent::TryAutoEnqueue, 0.01f, false);
}

void UConversionStationComponent::TryAutoEnqueue() {
    if (!HasAuthority() || bMutatingStationInv || !Subsystem.IsValid() || !StationInv.IsValid()) {
        return;
    }

    // Only enqueue when idle (one auto job at a time).
    if (const FConversionJobEntry *Head = Jobs.GetItems().Num() > 0 ? &Jobs.GetItems()[0] : nullptr) {
        if (Head->State == EConversionJobState::Processing) {
            return;
        }
    }
    if (Jobs.Num() >= MaxQueueLength) {
        return;
    }

    // Resolve the bounded auto recipe set.
    TArray<UConversionRecipe *> RestrictTo;
    for (const TSoftObjectPtr<UConversionRecipe> &Ref : AutoRecipes) {
        if (UConversionRecipe *R = Ref.Get()) {
            RestrictTo.Add(R);
        }
    }
    if (RestrictTo.Num() == 0) {
        return; // not loaded yet, or none configured
    }

    TArray<UMythicItemInstance *> Inputs;
    GatherInstances({StationInv.Get()}, InputGroupTag, Inputs);

    UConversionRecipe *R = Subsystem->FindMatchingRecipe(StationTags, Inputs, nullptr, &RestrictTo);
    if (!R) {
        return;
    }

    // Fuel before inputs, then catalysts, then inputs.
    if (R->Process.bRequiresFuel && !EnsureFuel(R)) {
        return; // wait for fuel
    }
    if (!VerifyCatalystsPresent(R, nullptr)) {
        return;
    }
    if (!VerifyInputs(R, nullptr, 1, false)) {
        return;
    }

    bMutatingStationInv = true;
    int32 SnapLevel = 0;
    if (!ConsumeInputs(R, nullptr, 1, NextJobId, SnapLevel)) {
        bMutatingStationInv = false;
        return;
    }

    const int32 Id = NextJobId++;
    const int32 Q = (R->Process.Timing == EConversionTiming::Continuous && R->Process.bRepeatWhileInputsAvailable) ? 0 : 1;
    Jobs.AddJob(R->RecipeId, Q, Id, SnapLevel);
    JobRefundTargets.Add(Id, FJobRefundInfo{nullptr, R->RecipeId});
    bMutatingStationInv = false;

    OnJobsChanged.Broadcast();
    AdvanceProcessing();
}

// ============================ Eligibility ============================

bool UConversionStationComponent::EvaluateEligibility(const UConversionRecipe *Recipe, const AActor *Interactor, FText &OutReason) const {
    if (!Recipe) {
        OutReason = NSLOCTEXT("Conversion", "UnknownRecipe", "Unknown recipe");
        return false;
    }
    if (!Recipe->MatchesStation(StationTags)) {
        OutReason = NSLOCTEXT("Conversion", "WrongStation", "Not available at this station");
        return false;
    }

    FGameplayTagContainer Owned;
    if (IInventoryProviderInterface *Prov = Cast<IInventoryProviderInterface>(const_cast<AActor *>(Interactor))) {
        if (UAbilitySystemComponent *ASC = Prov->GetSchematicsASC()) {
            ASC->GetOwnedGameplayTags(Owned);
        }
    }

    if (!StationUseRequirement.IsEmpty() && !StationUseRequirement.Matches(Owned)) {
        OutReason = NSLOCTEXT("Conversion", "StationLocked", "Station locked");
        return false;
    }
    if (!Recipe->Requirements.InstigatorTagQuery.IsEmpty() && !Recipe->Requirements.InstigatorTagQuery.Matches(Owned)) {
        OutReason = NSLOCTEXT("Conversion", "RecipeLocked", "Requires a schematic you have not learned");
        return false;
    }

    OutReason = FText::GetEmpty();
    return true;
}

// ============================ Server entry points ============================

void UConversionStationComponent::Server_RegisterInstigator(AController *Controller, AActor *Pawn) {
    if (!HasAuthority() || !Controller) {
        return;
    }
    if (!Cast<IInventoryProviderInterface>(Controller)) {
        return;
    }
    AActor *RangeActor = Pawn ? Pawn : Cast<AActor>(Controller->GetPawn());
    if (!IsActorInRange(RangeActor)) {
        return;
    }

    FGameplayTagContainer Owned;
    if (UAbilitySystemComponent *ASC = Cast<IInventoryProviderInterface>(Controller)->GetSchematicsASC()) {
        ASC->GetOwnedGameplayTags(Owned);
    }
    if (!StationUseRequirement.IsEmpty() && !StationUseRequirement.Matches(Owned)) {
        return;
    }

    FConversionSession &S = Sessions.FindOrAdd(Controller);
    S.Controller = Controller;
    S.Pawn = RangeActor;
    // Back-date so the first craft right after opening isn't swallowed by the rate limiter.
    S.LastRequestServerTime = ServerNow() - kMinRequestInterval;
}

void UConversionStationComponent::Server_RequestStart(AController *Controller, FGameplayTag RecipeId, int32 Quantity) {
    if (!HasAuthority() || !Controller) {
        return;
    }
    FConversionSession *S = Sessions.Find(Controller);
    if (!S) {
        return; // must register (open) first
    }
    AActor *Pawn = S->Pawn.Get();
    if (!IsActorInRange(Pawn)) {
        return;
    }

    const double Now = ServerNow();
    if (Now - S->LastRequestServerTime < kMinRequestInterval) {
        return;
    }
    if (S->OwnedJobs >= MaxJobsPerInstigator || Jobs.Num() >= MaxQueueLength) {
        return;
    }
    S->LastRequestServerTime = Now;

    Quantity = FMath::Clamp(Quantity, 1, 999);

    UConversionRecipe *R = Subsystem.IsValid() ? Subsystem->GetRecipeById(RecipeId) : nullptr;
    if (!R || !R->MatchesStation(StationTags)) {
        return;
    }

    // Re-check the station-use + instigator gates server-side (defense in depth).
    FGameplayTagContainer Owned;
    if (UAbilitySystemComponent *ASC = Cast<IInventoryProviderInterface>(Controller)->GetSchematicsASC()) {
        ASC->GetOwnedGameplayTags(Owned);
    }
    if (!StationUseRequirement.IsEmpty() && !StationUseRequirement.Matches(Owned)) {
        return;
    }
    if (!R->Requirements.InstigatorTagQuery.IsEmpty() && !R->Requirements.InstigatorTagQuery.Matches(Owned)) {
        return;
    }

    // Continuous batch quantity is meaningless: 0 (unbounded+repeat) or 1.
    if (R->Process.Timing == EConversionTiming::Continuous) {
        Quantity = R->Process.bRepeatWhileInputsAvailable ? 0 : 1;
    }

    // A Transform consumes a distinct non-stackable input per cycle; reserve-all batching is unsupported for
    // Timed/Instant, so cap to a single cycle. (Continuous transform reserves one input per cycle, which is fine.)
    bool bHasTransform = false;
    for (const FConversionProduct &P : R->Products) {
        if (P.Mode == EConversionProductMode::Transform) {
            bHasTransform = true;
            break;
        }
    }
    if (bHasTransform && R->Process.Timing != EConversionTiming::Continuous) {
        Quantity = 1;
    }

    const int32 EffectiveCycles = (Quantity == 0) ? 1 : Quantity;
    const int32 ReserveCycles = (R->Process.Timing == EConversionTiming::Continuous) ? 1 : EffectiveCycles;

    if (!VerifyInputs(R, Controller, EffectiveCycles, /*bCheckCatalysts=*/true)) {
        return;
    }

    const int32 Id = NextJobId; // tentative; only commit if consume succeeds
    int32 SnapLevel = 0;
    if (!ConsumeInputs(R, Controller, ReserveCycles, Id, SnapLevel)) {
        return;
    }
    NextJobId++;

    Jobs.AddJob(RecipeId, Quantity, Id, SnapLevel);
    JobRefundTargets.Add(Id, FJobRefundInfo{Controller, RecipeId});
    S->OwnedJobs++;

    OnJobsChanged.Broadcast();
    AdvanceProcessing();
}

void UConversionStationComponent::Server_CancelJob(AController *Controller, int32 JobId) {
    if (!HasAuthority() || !Controller) {
        return;
    }
    FConversionSession *S = Sessions.Find(Controller);
    if (!S || !IsActorInRange(S->Pawn.Get())) {
        return;
    }

    const int32 Idx = Jobs.IndexOfId(JobId);
    if (Idx == INDEX_NONE) {
        return;
    }
    const FJobRefundInfo *Info = JobRefundTargets.Find(JobId);
    if (!Info || Info->Controller.Get() != Controller) {
        return; // only the owner may cancel; auto jobs (null controller) are not player-cancelable
    }

    const FConversionJobEntry JobCopy = Jobs.GetItems()[Idx];

    if (Idx == 0 && GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(ProcessTimerHandle)) {
        GetWorld()->GetTimerManager().ClearTimer(ProcessTimerHandle);
    }

    RefundJob(JobCopy);

    Jobs.RemoveAt(Idx);
    ClearJobBookkeeping(JobId);

    OnJobsChanged.Broadcast();
    if (Idx == 0) {
        AdvanceProcessing();
    }
}

void UConversionStationComponent::Server_SetAutoRepeat(AController *Controller, bool bRepeat) {
    if (!HasAuthority() || !Controller) {
        return;
    }
    const FConversionSession *S = Sessions.Find(Controller);
    if (!S || !IsActorInRange(S->Pawn.Get())) {
        return; // same session + range gate as every other mutating entry point
    }
    FConversionJobEntry *Head = Jobs.EditHead();
    if (!Head) {
        return;
    }
    const FJobRefundInfo *Info = JobRefundTargets.Find(Head->JobId);
    if (!Info || Info->Controller.Get() != Controller) {
        return;
    }
    UConversionRecipe *R = Subsystem.IsValid() ? Subsystem->GetRecipeById(Head->RecipeId) : nullptr;
    if (R && R->Process.Timing == EConversionTiming::Continuous) {
        Head->Quantity = bRepeat ? 0 : 1;
        Jobs.MarkDirtyAt(0);
        OnJobsChanged.Broadcast();
    }
}
