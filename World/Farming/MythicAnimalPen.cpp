
#include "MythicAnimalPen.h"

#include "MythicApiaryRules.h"
#include "MythicHusbandryRules.h"
#include "MythicLivestockGenome.h"
#include "MythicFarmPlot.h"
#include "MythicFarmingRewardUtil.h"
#include "MythicLivestockDefinition.h"
#include "MythicTags_Farming.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Itemization/InventoryProviderInterface.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Mythic.h"
#include "Net/UnrealNetwork.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerState.h"
#include "Player/Proficiency/ProficiencyComponent.h"
#include "Player/Proficiency/ProficiencyDefinition.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Settings/MythicDeveloperSettings.h"
#include "TimerManager.h"
#include "Progression/MythicStatLedgerComponent.h"

namespace {
constexpr uint8 PenSaveVersion = 2;
}

AMythicAnimalPen::AMythicAnimalPen() {
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    NetDormancy = DORM_DormantAll;
    SetNetCullDistanceSquared(FMath::Square(6000.f));

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(SceneRoot);
}

void AMythicAnimalPen::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AMythicAnimalPen, PenState);
}

void AMythicAnimalPen::BeginPlay() {
    Super::BeginPlay();
    OnPenVisualChanged(PenState);
}


void AMythicAnimalPen::SampleProduction(double Now) {
    if (!HasAuthority()) {
        return;
    }
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    const bool bBreeding = Settings && Settings->bLivestockBreedingEnabled;
    for (FMythicLivestockRecord &Record : Records) {
        const UMythicLivestockDefinition *Def = Record.Def;
        if (!Def) {
            continue;
        }
        const float Window = FMythicHusbandryRules::FedWindowSeconds(Record.LastSampleTime, Now, Record.FedUntilTime);
        Record.LastSampleTime = Now;
        if (Window <= 0.0f) {
            continue;
        }
        const float EffectiveInterval =
            (bBreeding && !Record.Genome.IsNeutral())
                ? FMythicLivestockGenomeStatics::EffectiveProduceIntervalSeconds(Def->ProduceIntervalSeconds, Record.Genome, Settings->Breeding)
                : Def->ProduceIntervalSeconds;
        const FMythicProductionAccrual Accrual = FMythicApiaryRules::AccrueUnits(
            Record.CarryoverSeconds, Window, 1.0f, EffectiveInterval, Record.StoredUnits,
            Def->MaxStoredUnitsPerAnimal);
        Record.CarryoverSeconds = Accrual.CarryoverSeconds;
        Record.StoredUnits = Accrual.StoredUnits;
    }
    RefreshPenState();
}

void AMythicAnimalPen::RearmProductionTimer() {
    UWorld *World = GetWorld();
    if (!World || !HasAuthority()) {
        return;
    }
    FTimerManager &Timers = World->GetTimerManager();
    const double CurrentTime = World->GetTimeSeconds();

    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    const bool bBreeding = Settings && Settings->bLivestockBreedingEnabled;

    float Soonest = 0.0f;
    for (const FMythicLivestockRecord &Record : Records) {
        const UMythicLivestockDefinition *Def = Record.Def;
        if (!Def || Record.FedUntilTime <= CurrentTime) {
            continue;
        }
        const float EffectiveInterval =
            (bBreeding && !Record.Genome.IsNeutral())
                ? FMythicLivestockGenomeStatics::EffectiveProduceIntervalSeconds(Def->ProduceIntervalSeconds, Record.Genome, Settings->Breeding)
                : Def->ProduceIntervalSeconds;
        const float Next = FMythicApiaryRules::SecondsToNextUnit(Record.CarryoverSeconds, EffectiveInterval,
                                                                 Record.StoredUnits, Def->MaxStoredUnitsPerAnimal);
        if (Next > 0.0f && (Soonest <= 0.0f || Next < Soonest)) {
            Soonest = Next;
        }
    }
    if (Soonest <= 0.0f) {
        Timers.ClearTimer(ProductionTimerHandle);
        return;
    }
    TWeakObjectPtr<AMythicAnimalPen> WeakThis(this);
    Timers.SetTimer(ProductionTimerHandle, FTimerDelegate::CreateLambda([WeakThis]() {
                        if (AMythicAnimalPen *Pen = WeakThis.Get()) {
                            Pen->SampleProduction(Pen->GetWorld()->GetTimeSeconds());
                            Pen->RearmProductionTimer();
                        }
                    }),
                    FMath::Max(1.0f, Soonest), false);
}

void AMythicAnimalPen::RefreshPenState() {
    FMythicPenState NewState;
    const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    for (const FMythicLivestockRecord &Record : Records) {
        if (!Record.Def) {
            continue;
        }
        ++NewState.AnimalCount;
        NewState.ReadyUnits += Record.StoredUnits;
        NewState.bAnyFed |= Record.FedUntilTime > CurrentTime;
    }
    if (NewState.AnimalCount != PenState.AnimalCount || NewState.ReadyUnits != PenState.ReadyUnits ||
        NewState.bAnyFed != PenState.bAnyFed) {
        PenState = NewState;
        if (HasAuthority()) {
            FlushNetDormancy();
        }
        OnPenVisualChanged(PenState);
    }
}


bool AMythicAnimalPen::ServerTryCollect(AActor *Interactor) {
    if (!HasAuthority() || !GetWorld()) {
        return false;
    }
    SampleProduction(GetWorld()->GetTimeSeconds());
    APlayerController *PC = Cast<APlayerController>(AMythicFarmPlot::ResolveController(Interactor));
    if (!PC) {
        return false;
    }

    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    const bool bBreeding = Settings && Settings->bLivestockBreedingEnabled;

    int32 TotalUnits = 0;
    const FVector PenLoc = GetActorLocation();
    for (FMythicLivestockRecord &Record : Records) {
        const UMythicLivestockDefinition *Def = Record.Def;
        if (!Def || Record.StoredUnits <= 0) {
            continue;
        }
        EMythicYieldQuality Tier = FMythicHusbandryRules::FeedToProduceTier(Record.FeedTier);
        if (bBreeding && !Record.Genome.IsNeutral()) {
            const int32 Bonus = FMythicLivestockGenomeStatics::ProduceTierBonusFromGenome(Record.Genome, Settings->Breeding);
            Tier = FMythicYieldQuality::TierFromIndex(FMythicYieldQuality::TierIndex(Tier) + Bonus);
        }
        const FRewardsToGive &Rewards = MythicFarmingRewards::RouteByTier(Tier, Def->ProduceRewards, Def->ProduceRewardsFine,
                                                                          Def->ProduceRewardsPristine);
        for (int32 i = 0; i < Record.StoredUnits; ++i) {
            Rewards.Give(PC, true, 0, PenLoc);
        }
        if (MythicFarmingRewards::HasAny(Def->ManureRewards)) {
            Def->ManureRewards.Give(PC, true, 0, PenLoc);
        }

        const int32 Level = AMythicFarmPlot::ResolveProficiencyLevel(Interactor, Def->HusbandryProficiency);
        float Xp = Def->HusbandryXPPerUnit * Record.StoredUnits;
        if (Def->XpNoGainAtOrAboveLevel > 0 && Level >= Def->XpNoGainAtOrAboveLevel) {
            Xp = 0.0f;
        }
        if (Xp > 0.0f && Def->HusbandryProficiency) {
            if (AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(AMythicFarmPlot::ResolveController(Interactor))) {
                if (UProficiencyComponent *ProfComp = const_cast<UProficiencyComponent *>(MythicPC->GetProficiencyComponent())) {
                    ProfComp->GrantProficiencyXP(Def->HusbandryProficiency, Xp);
                }
            }
        }

        TotalUnits += Record.StoredUnits;
        Record.StoredUnits = 0;
        Record.CarryoverSeconds = 0.0f;
    }
    if (TotalUnits <= 0) {
        return false;
    }
    if (const AMythicPlayerState *PS = PC->GetPlayerState<AMythicPlayerState>()) {
        if (UMythicStatLedgerComponent *Ledger = PS->GetStatLedgerComponent()) {
            Ledger->RecordStat(TAG_Stat_Husbandry_ProduceCollected, TotalUnits);
        }
    }
    UE_LOG(Myth, Log, TEXT("AMythicAnimalPen: collected %d produce unit(s)"), TotalUnits);
    RefreshPenState();
    RearmProductionTimer();
    return true;
}

bool AMythicAnimalPen::ServerTryAddLivestock(AActor *Interactor) {
    if (!HasAuthority() || !GetWorld()) {
        return false;
    }
    int32 LiveCount = 0;
    for (const FMythicLivestockRecord &Record : Records) {
        if (Record.Def) {
            ++LiveCount;
        }
    }
    if (LiveCount >= MaxAnimals) {
        return false;
    }
    UMythicLivestockDefinition *Def = nullptr;
    UMythicItemInstance *Item = FindLivestockItem(Interactor, Def);
    if (!Item || !Def) {
        return false;
    }
    Item->ConsumeItem(1);

    FMythicLivestockRecord Record;
    Record.Def = Def;
    Record.FeedTier = EMythicYieldQuality::Common;
    Record.FedUntilTime = 0.0;
    Record.LastSampleTime = GetWorld()->GetTimeSeconds();
    Records.Add(Record);

    UE_LOG(Myth, Log, TEXT("AMythicAnimalPen: penned %s (%d/%d)"), *Def->DisplayName.ToString(), LiveCount + 1, MaxAnimals);
    RefreshPenState();
    RearmProductionTimer();
    return true;
}

bool AMythicAnimalPen::ServerTryFeed(AActor *Interactor) {
    if (!HasAuthority() || !GetWorld()) {
        return false;
    }
    const double CurrentTime = GetWorld()->GetTimeSeconds();
    SampleProduction(CurrentTime);

    FMythicLivestockRecord *Target = nullptr;
    for (FMythicLivestockRecord &Record : Records) {
        if (!Record.Def) {
            continue;
        }
        if (!Target || Record.FedUntilTime < Target->FedUntilTime) {
            Target = &Record;
        }
    }
    if (!Target) {
        return false;
    }
    EMythicYieldQuality FeedTier = EMythicYieldQuality::Common;
    UMythicItemInstance *Feed = FindFeedItem(Interactor, FeedTier);
    if (!Feed) {
        return false;
    }
    Feed->ConsumeItem(1);

    Target->FedUntilTime = FMythicHusbandryRules::ExtendFedUntil(CurrentTime, Target->FedUntilTime,
                                                                 Target->Def->FeedSecondsPerUnit, Target->Def->MaxFeedBankSeconds);
    Target->FeedTier = FeedTier;
    if (Target->LastSampleTime <= 0.0) {
        Target->LastSampleTime = CurrentTime;
    }

    UE_LOG(Myth, Log, TEXT("AMythicAnimalPen: fed %s (tier %d)"), *Target->Def->DisplayName.ToString(), static_cast<int32>(FeedTier));
    RefreshPenState();
    RearmProductionTimer();
    return true;
}

bool AMythicAnimalPen::ServerTryBreed(AActor *Interactor) {
    if (!HasAuthority() || !GetWorld()) {
        return false;
    }
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    if (!Settings || !Settings->bLivestockBreedingEnabled) {
        return false;
    }

    int32 LiveCount = 0;
    for (const FMythicLivestockRecord &Record : Records) {
        if (Record.Def) {
            ++LiveCount;
        }
    }
    if (LiveCount >= MaxAnimals) {
        return false;
    }

    const double CurrentTime = GetWorld()->GetTimeSeconds();
    SampleProduction(CurrentTime);

    FMythicLivestockRecord *ParentA = nullptr;
    FMythicLivestockRecord *ParentB = nullptr;
    for (FMythicLivestockRecord &Candidate : Records) {
        if (!Candidate.Def || Candidate.FedUntilTime <= CurrentTime) {
            continue;
        }
        FMythicLivestockRecord *BestA = nullptr;
        FMythicLivestockRecord *BestB = nullptr;
        for (FMythicLivestockRecord &Other : Records) {
            if (Other.Def != Candidate.Def || Other.FedUntilTime <= CurrentTime) {
                continue;
            }
            if (!BestA || Other.FedUntilTime > BestA->FedUntilTime) {
                BestB = BestA;
                BestA = &Other;
            }
            else if (!BestB || Other.FedUntilTime > BestB->FedUntilTime) {
                BestB = &Other;
            }
        }
        if (BestA && BestB) {
            ParentA = BestA;
            ParentB = BestB;
            break;
        }
    }
    if (!ParentA || !ParentB) {
        return false;
    }

    float Rolls[FMythicLivestockGenome::NumTraits];
    for (int32 i = 0; i < FMythicLivestockGenome::NumTraits; ++i) {
        Rolls[i] = FMath::FRand();
    }
    const FMythicLivestockGenome ChildGenome = FMythicLivestockGenomeStatics::Breed(
        ParentA->Genome, ParentB->Genome, TConstArrayView<float>(Rolls, FMythicLivestockGenome::NumTraits), Settings->Breeding);
    UMythicLivestockDefinition *Species = ParentA->Def;

    ParentA->FedUntilTime = CurrentTime;
    ParentB->FedUntilTime = CurrentTime;

    FMythicLivestockRecord Child;
    Child.Def = Species;
    Child.FeedTier = EMythicYieldQuality::Common;
    Child.FedUntilTime = 0.0;
    Child.LastSampleTime = CurrentTime;
    Child.Genome = ChildGenome;
    Records.Add(Child);

    UE_LOG(Myth, Log, TEXT("AMythicAnimalPen: bred %s (Yield %.2f, Quality %.2f) -> %d/%d"), *Species->DisplayName.ToString(),
           ChildGenome.Yield, ChildGenome.Quality, LiveCount + 1, MaxAnimals);
    RefreshPenState();
    RearmProductionTimer();
    return true;
}

void AMythicAnimalPen::ServerHandlePrimaryInteract(AActor *Interactor) {
    if (!HasAuthority()) {
        return;
    }
    if (ServerTryCollect(Interactor)) {
        return;
    }
    if (ServerTryAddLivestock(Interactor)) {
        return;
    }
    ServerTryFeed(Interactor);
}


UMythicItemInstance *AMythicAnimalPen::FindLivestockItem(AActor *Interactor, UMythicLivestockDefinition *&OutDef) const {
    OutDef = nullptr;
    if (!LivestockRegistry) {
        return nullptr;
    }
    AController *Controller = AMythicFarmPlot::ResolveController(Interactor);
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
                if (UMythicLivestockDefinition *Def = LivestockRegistry->ResolveLivestockForProbe(Probe)) {
                    OutDef = Def;
                    return Item;
                }
            }
        }
    }
    return nullptr;
}

UMythicItemInstance *AMythicAnimalPen::FindFeedItem(AActor *Interactor, EMythicYieldQuality &OutFeedTier) const {
    OutFeedTier = EMythicYieldQuality::Common;
    AController *Controller = AMythicFarmPlot::ResolveController(Interactor);
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
            UMythicItemInstance *Item = Slot.SlottedItemInstance;
            if (!Item) {
                continue;
            }
            Probe.Reset();
            Item->GetTypeProbe(Probe);
            if (!Probe.HasTag(TAG_Item_Feed)) {
                continue;
            }
            OutFeedTier = FMythicYieldQuality::TierFromTags(Probe);
            return Item;
        }
    }
    return nullptr;
}


void AMythicAnimalPen::OnPrimaryInteract_Implementation(AActor *Interactor) {
    if (HasAuthority()) {
        ServerHandlePrimaryInteract(Interactor);
        return;
    }
    if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(Interactor)) {
        if (PC->IsLocalController()) {
            PC->ServerInteractPrimary(this);
        }
    }
}

void AMythicAnimalPen::OnSecondaryInteract_Implementation(AActor *Interactor) {
    if (HasAuthority()) {
        ServerTryFeed(Interactor);
    }
}

USceneComponent *AMythicAnimalPen::GetWidgetAttachmentComponent_Implementation() const {
    return SceneRoot;
}

bool AMythicAnimalPen::GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const {
    OutInteractionData.InputActionDataTable = InputActionDataTable;
    if (PenState.ReadyUnits > 0) {
        OutInteractionData.PrimaryInteractionName = CollectInteractionName;
    }
    else if (PenState.AnimalCount == 0) {
        OutInteractionData.PrimaryInteractionName = AddAnimalInteractionName;
    }
    else {
        OutInteractionData.PrimaryInteractionName = FeedInteractionName;
    }
    OutInteractionData.SecondaryInteractionName = FeedInteractionName;
    return true;
}

void AMythicAnimalPen::OnFocused_Implementation(AActor *Interactor) {}
void AMythicAnimalPen::OnUnfocused_Implementation(AActor *Interactor) {}

void AMythicAnimalPen::OnRep_PenState() {
    OnPenVisualChanged(PenState);
}


void AMythicAnimalPen::SerializeCustomData(TArray<uint8> &OutCustomData) {
    const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    SampleProduction(CurrentTime);

    FMemoryWriter Writer(OutCustomData);
    uint8 Version = PenSaveVersion;
    Writer << Version;
    int32 Count = 0;
    for (const FMythicLivestockRecord &Record : Records) {
        if (Record.Def) {
            ++Count;
        }
    }
    Writer << Count;
    for (const FMythicLivestockRecord &Record : Records) {
        if (!Record.Def) {
            continue;
        }
        FString DefPath = Record.Def->GetPathName();
        uint8 FeedTier = static_cast<uint8>(Record.FeedTier);
        double FedRemaining = FMath::Max(0.0, Record.FedUntilTime - CurrentTime);
        float Carry = Record.CarryoverSeconds;
        int32 Stored = Record.StoredUnits;
        Writer << DefPath;
        Writer << FeedTier;
        Writer << FedRemaining;
        Writer << Carry;
        Writer << Stored;
        int32 NumTraits = FMythicLivestockGenome::NumTraits;
        Writer << NumTraits;
        for (int32 t = 0; t < NumTraits; ++t) {
            float TraitValue = Record.Genome.GetTrait(t);
            Writer << TraitValue;
        }
    }
}

void AMythicAnimalPen::DeserializeCustomData(const TArray<uint8> &InCustomData) {
    if (InCustomData.Num() == 0) {
        OnPenVisualChanged(PenState);
        return;
    }
    const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

    FMemoryReader Reader(InCustomData);
    uint8 Version = 0;
    int32 Count = 0;
    Reader << Version;
    Reader << Count;
    Records.Reset();
    if (!Reader.IsError() && Version >= 1) {
        for (int32 i = 0; i < Count && !Reader.IsError(); ++i) {
            FString DefPath;
            uint8 FeedTier = static_cast<uint8>(EMythicYieldQuality::Common);
            double FedRemaining = 0.0;
            float Carry = 0.0f;
            int32 Stored = 0;
            Reader << DefPath;
            Reader << FeedTier;
            Reader << FedRemaining;
            Reader << Carry;
            Reader << Stored;
            FMythicLivestockGenome Genome;
            if (Version >= 2) {
                int32 NumTraits = 0;
                Reader << NumTraits;
                for (int32 t = 0; t < NumTraits && !Reader.IsError(); ++t) {
                    float TraitValue = 0.0f;
                    Reader << TraitValue;
                    if (t < FMythicLivestockGenome::NumTraits) {
                        Genome.SetTrait(t, TraitValue);
                    }
                }
            }
            if (Reader.IsError()) {
                break;
            }
            UMythicLivestockDefinition *Def = DefPath.IsEmpty() ? nullptr : LoadObject<UMythicLivestockDefinition>(nullptr, *DefPath);
            if (!Def) {
                UE_LOG(MythSaveLoad, Warning, TEXT("AMythicAnimalPen: failed to load saved livestock %s; record dropped"), *DefPath);
                continue;
            }
            FMythicLivestockRecord Record;
            Record.Def = Def;
            Record.FeedTier = FMythicYieldQuality::TierFromIndex(FeedTier);
            Record.FedUntilTime = CurrentTime + FMath::Max(0.0, FedRemaining);
            Record.LastSampleTime = CurrentTime;
            Record.CarryoverSeconds = FMath::Max(0.0f, Carry);
            Record.StoredUnits = FMath::Max(0, Stored);
            Record.Genome = Genome;
            Records.Add(Record);
        }
    }
    else if (Reader.IsError()) {
        UE_LOG(MythSaveLoad, Warning, TEXT("AMythicAnimalPen: unreadable saved payload (%d bytes); restoring empty"), InCustomData.Num());
    }

    RefreshPenState();
    if (HasAuthority()) {
        FlushNetDormancy();
        RearmProductionTimer();
    }
    OnPenVisualChanged(PenState);
}
