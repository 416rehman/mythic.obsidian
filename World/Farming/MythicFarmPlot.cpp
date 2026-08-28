
#include "MythicFarmPlot.h"

#include "MythicFarmingRules.h"
#include "MythicFarmingRewardUtil.h"
#include "MythicCropRegistry.h"
#include "MythicTags_Farming.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
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
#include "Settings/MythicDeveloperSettings.h"
#include "TimerManager.h"
#include "World/Camping/MythicInfluenceSourceComponent.h"
#include "World/Death/MythicCorpse.h"
#include "World/EnvironmentController/EnvironmentTags.h"
#include "World/EnvironmentController/MythicEnvironmentSubsystem.h"
#include "World/LivingWorld/Pressure/MythicRegionalPressureSubsystem.h"
#include "Progression/MythicStatLedgerComponent.h"
#include "World/Gathering/MythicYieldQuality.h"

namespace {
FName FarmStageTagName(EMythicCropStage Stage) {
    switch (Stage) {
    case EMythicCropStage::Growing:  return FName(TEXT("Farming.Stage.Growing"));
    case EMythicCropStage::Mature:   return FName(TEXT("Farming.Stage.Mature"));
    case EMythicCropStage::Withered: return FName(TEXT("Farming.Stage.Withered"));
    case EMythicCropStage::Empty:
    default:                         return FName(TEXT("Farming.Stage.Empty"));
    }
}
}


AMythicFarmPlot::AMythicFarmPlot() {
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    NetDormancy = DORM_DormantAll;
    SetNetCullDistanceSquared(FMath::Square(6000.f));

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(SceneRoot);
}

void AMythicFarmPlot::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AMythicFarmPlot, PlotState);
}

void AMythicFarmPlot::BeginPlay() {
    Super::BeginPlay();
    if (const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>()) {
        FarmingConfig = Settings->Farming;
    }
    MoistureSampleTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    OnCropVisualChanged(PlotState.Crop, PlotState.Stage);
    OnPlotConditionChanged(GetMoisture01(), PlotState.bWithered);
}

void AMythicFarmPlot::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (bPressureRegistered) {
        if (UWorld *World = GetWorld()) {
            if (UMythicRegionalPressureSubsystem *Pressure = World->GetSubsystem<UMythicRegionalPressureSubsystem>()) {
                Pressure->UnregisterFarmPlot(this);
            }
        }
        bPressureRegistered = false;
    }
    Super::EndPlay(EndPlayReason);
}

bool AMythicFarmPlot::IsMature() const {
    return PlotState.Crop != nullptr && PlotState.Stage >= PlotState.Crop->GetMatureStageIndex();
}

EMythicCropStage AMythicFarmPlot::GetCropLifecycle() const {
    if (IsEmpty()) {
        return EMythicCropStage::Empty;
    }
    if (PlotState.bWithered) {
        return EMythicCropStage::Withered;
    }
    return IsMature() ? EMythicCropStage::Mature : EMythicCropStage::Growing;
}

float AMythicFarmPlot::GetMoisture01() const {
    const float Snapshot = static_cast<float>(PlotState.MoistureQ) / 255.0f;
    if (HasAuthority() && GetWorld()) {
        return FMythicFarmingRules::MoistureAtTime(Snapshot, MoistureSampleTime, GetWorld()->GetTimeSeconds(),
                                                   FarmingConfig.MoistureDecayPerSecond);
    }
    return Snapshot;
}


void AMythicFarmPlot::ApplyPlotState(UMythicCropDefinition *Crop, int32 Stage) {
    PlotState.Crop = Crop;
    PlotState.Stage = Stage;
    if (!Crop) {
        PlotState.bWithered = false;
        PlotState.FertilizerTag = FGameplayTag();
        PlotState.GraveEssenceTag = FGameplayTag();
        WetUptimeSeconds = 0.0f;
        GrowTimeSeconds = 0.0f;
        DryStreakSeconds = 0.0f;
    }
    CommitPlotState();
}

void AMythicFarmPlot::CommitPlotState() {
    if (HasAuthority()) {
        FlushNetDormancy();
        UpdatePressureRegistration();
    }
    OnCropVisualChanged(PlotState.Crop, PlotState.Stage);
    OnPlotConditionChanged(static_cast<float>(PlotState.MoistureQ) / 255.0f, PlotState.bWithered);
}

void AMythicFarmPlot::UpdatePressureRegistration() {
    const bool bShouldEmit = HasAuthority() && !IsEmpty() && !PlotState.bWithered && IsMature();
    if (bShouldEmit == bPressureRegistered) {
        return;
    }
    UWorld *World = GetWorld();
    UMythicRegionalPressureSubsystem *Pressure = World ? World->GetSubsystem<UMythicRegionalPressureSubsystem>() : nullptr;
    if (!Pressure) {
        return;
    }
    if (bShouldEmit) {
        Pressure->RegisterFarmPlot(this);
    }
    else {
        Pressure->UnregisterFarmPlot(this);
    }
    bPressureRegistered = bShouldEmit;
}

void AMythicFarmPlot::ArmGrowthTimerForModelSeconds(float ModelSeconds, float GrowthSpeed) {
    if (!HasAuthority() || !GetWorld()) {
        return;
    }
    ArmedGrowthSpeed = FMath::Max(FMythicFarmingRules::MinGrowthSpeed, GrowthSpeed);
    const float RealSeconds = FMath::Max(0.0f, ModelSeconds) / ArmedGrowthSpeed;
    GetWorld()->GetTimerManager().SetTimer(
        GrowthTimerHandle, this, &AMythicFarmPlot::AdvanceStage, FMath::Max(0.01f, RealSeconds), false);
}

float AMythicFarmPlot::GetRemainingModelSeconds() const {
    if (!GetWorld()) {
        return 0.0f;
    }
    const float RemainingReal = GetWorld()->GetTimerManager().GetTimerRemaining(GrowthTimerHandle);
    return (RemainingReal > 0.0f) ? RemainingReal * ArmedGrowthSpeed : 0.0f;
}

float AMythicFarmPlot::CurrentGrowthSpeed() const {
    return FMythicFarmingRules::GrowthTimeScale(static_cast<float>(PlotState.MoistureQ) / 255.0f,
                                                FarmingConfig.DryGrowthSpeedMultiplier);
}

void AMythicFarmPlot::SampleMoisture(double Now) {
    if (!HasAuthority()) {
        return;
    }
    const double Gap = Now - MoistureSampleTime;
    if (Gap <= 0.0) {
        return;
    }
    MoistureSampleTime = Now;
    if (IsEmpty()) {
        return;
    }

    const bool bGrowing = PlotState.Stage >= 0 && !IsMature() && !PlotState.bWithered;
    const float Prev = static_cast<float>(PlotState.MoistureQ) / 255.0f;
    const float Window = static_cast<float>(Gap);

    bool bCovered = UMythicInfluenceSourceComponent::GetTotalInfluenceAt(GetWorld(), GetActorLocation(), TAG_Influence_Irrigation) > 0.0f;
    if (!bCovered && FarmingConfig.bRainRefillsMoisture) {
        if (const UGameInstance *GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr) {
            if (UMythicEnvironmentSubsystem *Env = GI->GetSubsystem<UMythicEnvironmentSubsystem>()) {
                bCovered = Env->GetWeather() == Environment_Weather_Rain;
            }
        }
    }

    float NewMoisture;
    float WetSeconds;
    if (bCovered) {
        NewMoisture = 1.0f;
        WetSeconds = Window;
        DryStreakSeconds = 0.0f;
    }
    else {
        const FMythicMoistureSample Sample = FMythicFarmingRules::SampleMoistureWindow(
            Prev, Window, FarmingConfig.MoistureDecayPerSecond, FarmingConfig.WetMoistureThreshold);
        NewMoisture = Sample.NewMoisture;
        WetSeconds = Sample.WetSeconds;
        DryStreakSeconds = (NewMoisture > 0.0f) ? 0.0f : DryStreakSeconds + Sample.DrySeconds;
    }

    if (bGrowing) {
        GrowTimeSeconds += Window;
        WetUptimeSeconds += WetSeconds;
    }
    PlotState.MoistureQ = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(NewMoisture * 255.0f), 0, 255));

    if (bGrowing && FMythicFarmingRules::WitherCheck(DryStreakSeconds, FarmingConfig.WitherAfterDrySeconds)) {
        GetWorld()->GetTimerManager().ClearTimer(GrowthTimerHandle);
        PlotState.bWithered = true;
        CommitPlotState();
        UE_LOG(Myth, Log, TEXT("AMythicFarmPlot: %s withered after %.0fs bone-dry"), *GetNameSafe(PlotState.Crop), DryStreakSeconds);
    }
}

void AMythicFarmPlot::AdvanceStage() {
    if (!HasAuthority() || IsEmpty()) {
        return;
    }
    SampleMoisture(GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0);
    if (PlotState.bWithered) {
        return;
    }
    const int32 MatureStage = PlotState.Crop->GetMatureStageIndex();
    const int32 NewStage = PlotState.Stage + 1;
    ApplyPlotState(PlotState.Crop, NewStage);

    if (NewStage < MatureStage && PlotState.Crop->StageDurations.IsValidIndex(NewStage)) {
        ArmGrowthTimerForModelSeconds(PlotState.Crop->StageDurations[NewStage], CurrentGrowthSpeed());
    }
}


bool AMythicFarmPlot::ServerTryPlant(AActor *Interactor) {
    UMythicCropDefinition *Crop = nullptr;
    UMythicItemInstance *Seed = FindMatchingSeed(Interactor, Crop);
    if (!Seed || !Crop) {
        return false;
    }

    const int32 FarmingLevel = ResolveFarmingLevel(Interactor, Crop);
    if (!FMythicFarmingRules::CanPlant(IsEmpty(), true, FarmingLevel, Crop->MinFarmingLevelToPlant)) {
        return false;
    }

    FGameplayTag SeasonTag;
    if (const UGameInstance *GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr) {
        if (UMythicEnvironmentSubsystem *Env = GI->GetSubsystem<UMythicEnvironmentSubsystem>()) {
            SeasonTag = Env->GetSeasonTag();
        }
    }
    if (!FMythicFarmingRules::CanPlantInSeason(Crop->AllowedSeasonTags, SeasonTag)) {
        UE_LOG(Myth, Log, TEXT("AMythicFarmPlot: %s is out of season (%s)"), *GetNameSafe(Crop), *SeasonTag.ToString());
        return false;
    }

    Seed->ConsumeItem(1);

    WetUptimeSeconds = 0.0f;
    GrowTimeSeconds = 0.0f;
    DryStreakSeconds = 0.0f;
    MoistureSampleTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    PlotState.MoistureQ = 255;
    PlotState.bWithered = false;

    const int32 MatureStage = Crop->GetMatureStageIndex();
    ApplyPlotState(Crop, 0);
    if (MatureStage > 0 && Crop->StageDurations.IsValidIndex(0)) {
        ArmGrowthTimerForModelSeconds(Crop->StageDurations[0], CurrentGrowthSpeed());
    }
    UE_LOG(Myth, Log, TEXT("AMythicFarmPlot: planted %s (%d growth stages)"), *GetNameSafe(Crop), MatureStage);
    return true;
}

bool AMythicFarmPlot::ServerTryHarvest(AActor *Interactor) {
    UMythicCropDefinition *Crop = PlotState.Crop;
    if (!Crop) {
        return false;
    }
    SampleMoisture(GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0);
    if (PlotState.bWithered) {
        return ServerTryClearWithered(Interactor);
    }
    const int32 MatureStage = Crop->GetMatureStageIndex();
    if (!FMythicFarmingRules::CanHarvest(PlotState.Stage, MatureStage)) {
        return false;
    }

    AController *Controller = ResolveController(Interactor);
    APlayerController *PC = Cast<APlayerController>(Controller);
    if (!PC) {
        return false;
    }
    const int32 FarmingLevel = ResolveFarmingLevel(Interactor, Crop);
    const FVector PlotLoc = GetActorLocation();

    const float Pollination = UMythicInfluenceSourceComponent::GetTotalInfluenceAt(GetWorld(), PlotLoc, TAG_Influence_Pollination);
    FGameplayTag SeasonTag;
    if (const UGameInstance *GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr) {
        if (UMythicEnvironmentSubsystem *Env = GI->GetSubsystem<UMythicEnvironmentSubsystem>()) {
            SeasonTag = Env->GetSeasonTag();
        }
    }
    FMythicCropQualityInputs QualityIn;
    QualityIn.WetUptimeFraction = FMythicFarmingRules::WetUptimeFraction(WetUptimeSeconds, GrowTimeSeconds);
    QualityIn.bFertilized = PlotState.FertilizerTag.IsValid();
    QualityIn.bGraveEssence = PlotState.GraveEssenceTag.IsValid();
    QualityIn.PollinationMagnitude = Pollination;
    QualityIn.bSeasonMatch = Crop->PreferredSeasonTag.IsValid() && SeasonTag.IsValid() && Crop->PreferredSeasonTag == SeasonTag;
    QualityIn.FarmingLevel = FarmingLevel;

    const FMythicYieldQualityRules &QualityRules = GetDefault<UMythicDeveloperSettings>()->YieldQuality;
    const EMythicYieldQuality Tier = FMythicFarmingRules::ComputeCropQuality(QualityIn, QualityRules, FarmingConfig, FMath::FRand());

    const FRewardsToGive &Rewards = MythicFarmingRewards::RouteByTier(Tier, Crop->HarvestRewards, Crop->HarvestRewardsFine,
                                                                      Crop->HarvestRewardsPristine);

    const int32 MinY = FMath::Min(Crop->YieldMin, Crop->YieldMax);
    const int32 MaxY = FMath::Max(Crop->YieldMin, Crop->YieldMax);
    const int32 BaseYield = FMath::RandRange(MinY, MaxY);
    int32 Yield = FMath::Max(1, FMythicFarmingRules::ComputeHarvestYield(
                                    BaseYield, FarmingLevel, Crop->YieldBonusPerFarmingLevel, FMath::FRand()));
    Yield += FMythicFarmingRules::PollinationBonusYield(Pollination, FarmingConfig, FMath::FRand());

    for (int32 i = 0; i < Yield; ++i) {
        Rewards.Give(PC, true, 0, PlotLoc);
    }

    float Xp = Crop->FarmingXPOnHarvest;
    if (Crop->XpNoGainAtOrAboveFarmingLevel > 0 && FarmingLevel >= Crop->XpNoGainAtOrAboveFarmingLevel) {
        Xp = 0.0f;
    }
    if (Xp > 0.0f && Crop->FarmingProficiency) {
        if (AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(Controller)) {
            if (UProficiencyComponent *ProfComp = const_cast<UProficiencyComponent *>(MythicPC->GetProficiencyComponent())) {
                FGameplayTagContainer Context;
                Context.AddTag(FGameplayTag::RequestGameplayTag(FarmStageTagName(GetCropLifecycle())));
                Context.AddTag(FGameplayTag::RequestGameplayTag(FMythicYieldQuality::QualityTagName(Tier)));
                ProfComp->GrantProficiencyXPWithContext(Crop->FarmingProficiency, Xp, Context);
            }
        }
    }

    if (const AMythicPlayerState *PS = PC->GetPlayerState<AMythicPlayerState>()) {
        if (UMythicStatLedgerComponent *Ledger = PS->GetStatLedgerComponent()) {
            Ledger->RecordStat(TAG_Stat_Farming_Harvests);
        }
    }

    UE_LOG(Myth, Log, TEXT("AMythicFarmPlot: harvested %s x%d (farming lvl %d, tier %d, wet %.0f%%)"), *GetNameSafe(Crop),
           Yield, FarmingLevel, static_cast<int32>(Tier), QualityIn.WetUptimeFraction * 100.0f);

    PlotState.FertilizerTag = FGameplayTag();
    PlotState.GraveEssenceTag = FGameplayTag();
    WetUptimeSeconds = 0.0f;
    GrowTimeSeconds = 0.0f;
    DryStreakSeconds = 0.0f;

    if (Crop->bRegrowable) {
        const int32 RegrowStage = FMath::Clamp(Crop->RegrowToStage, 0, MatureStage);
        ApplyPlotState(Crop, RegrowStage);
        if (RegrowStage < MatureStage && Crop->StageDurations.IsValidIndex(RegrowStage)) {
            ArmGrowthTimerForModelSeconds(Crop->StageDurations[RegrowStage], CurrentGrowthSpeed());
        }
    }
    else {
        if (GetWorld()) {
            GetWorld()->GetTimerManager().ClearTimer(GrowthTimerHandle);
        }
        ApplyPlotState(nullptr, STAGE_EMPTY);
    }
    return true;
}

bool AMythicFarmPlot::ServerTryClearWithered(AActor *Interactor) {
    UMythicCropDefinition *Crop = PlotState.Crop;
    if (!Crop || !PlotState.bWithered) {
        return false;
    }
    APlayerController *PC = Cast<APlayerController>(ResolveController(Interactor));
    if (!PC) {
        return false;
    }
    if (MythicFarmingRewards::HasAny(Crop->WitheredHarvestRewards)) {
        Crop->WitheredHarvestRewards.Give(PC, true, 0, GetActorLocation());
    }
    if (GetWorld()) {
        GetWorld()->GetTimerManager().ClearTimer(GrowthTimerHandle);
    }
    UE_LOG(Myth, Log, TEXT("AMythicFarmPlot: withered %s cleared for compost feedstock"), *GetNameSafe(Crop));
    ApplyPlotState(nullptr, STAGE_EMPTY);
    return true;
}

bool AMythicFarmPlot::ServerTryTend(AActor *Interactor) {
    if (!HasAuthority() || IsEmpty() || PlotState.bWithered || !GetWorld()) {
        return false;
    }
    const double Now = GetWorld()->GetTimeSeconds();
    SampleMoisture(Now);
    if (PlotState.bWithered) {
        return false;
    }

    const float RemainingModel = GetRemainingModelSeconds();
    PlotState.MoistureQ = 255;
    DryStreakSeconds = 0.0f;
    if (!IsMature() && RemainingModel > 0.0f) {
        ArmGrowthTimerForModelSeconds(RemainingModel, CurrentGrowthSpeed());
    }

    if (!PlotState.FertilizerTag.IsValid()) {
        FGameplayTag FertilizerTag;
        if (UMythicItemInstance *Fertilizer = FindMatchingFertilizer(Interactor, FertilizerTag)) {
            Fertilizer->ConsumeItem(1);
            PlotState.FertilizerTag = FertilizerTag;
            UE_LOG(Myth, Log, TEXT("AMythicFarmPlot: fertilized with %s"), *FertilizerTag.ToString());
        }
    }

    CommitPlotState();
    return true;
}

bool AMythicFarmPlot::ServerTryBury(AActor *Interactor) {
    if (!HasAuthority() || !IsEmpty()) {
        return false;
    }
    AMythicCorpse *Corpse = FindNearbyCorpse();
    if (!Corpse) {
        return false;
    }
    const FGameplayTag Essence = Corpse->GetSourceKind().IsValid() ? Corpse->GetSourceKind() : TAG_Farming_GraveEssence;
    Corpse->ServerBurnCorpse();

    PlotState.GraveEssenceTag = Essence;
    CommitPlotState();

    if (const APlayerController *PC = Cast<APlayerController>(ResolveController(Interactor))) {
        if (const AMythicPlayerState *PS = PC->GetPlayerState<AMythicPlayerState>()) {
            if (UMythicStatLedgerComponent *Ledger = PS->GetStatLedgerComponent()) {
                Ledger->RecordStat(TAG_Stat_Farming_Graveblooms);
            }
        }
    }
    UE_LOG(Myth, Log, TEXT("AMythicFarmPlot: Gravebloom — corpse buried, essence %s"), *Essence.ToString());
    return true;
}

void AMythicFarmPlot::ServerApplyRaidStageRegression(int32 Stages) {
    if (!HasAuthority() || IsEmpty() || PlotState.bWithered || Stages <= 0) {
        return;
    }
    const int32 NewStage = FMath::Max(0, PlotState.Stage - Stages);
    if (NewStage == PlotState.Stage) {
        return;
    }
    SampleMoisture(GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0);
    UMythicCropDefinition *Crop = PlotState.Crop;
    ApplyPlotState(Crop, NewStage);
    const int32 MatureStage = Crop->GetMatureStageIndex();
    if (NewStage < MatureStage && Crop->StageDurations.IsValidIndex(NewStage)) {
        ArmGrowthTimerForModelSeconds(Crop->StageDurations[NewStage], CurrentGrowthSpeed());
    }
    UE_LOG(Myth, Log, TEXT("AMythicFarmPlot: raid regressed %s to stage %d (never uprooted)"), *GetNameSafe(Crop), NewStage);
}

void AMythicFarmPlot::ServerHandlePrimaryInteract(AActor *Interactor) {
    if (!HasAuthority()) {
        return;
    }
    if (PlotState.bWithered) {
        ServerTryClearWithered(Interactor);
    }
    else if (IsEmpty()) {
        UMythicCropDefinition *Crop = nullptr;
        if (FindMatchingSeed(Interactor, Crop)) {
            ServerTryPlant(Interactor);
        }
        else {
            ServerTryBury(Interactor);
        }
    }
    else if (IsMature()) {
        ServerTryHarvest(Interactor);
    }
    else {
        ServerTryTend(Interactor);
    }
}

void AMythicFarmPlot::ServerHandleSecondaryInteract(AActor *Interactor) {
    if (!HasAuthority()) {
        return;
    }
    if (IsEmpty()) {
        ServerTryBury(Interactor);
    }
    else if (!IsMature() && !PlotState.bWithered) {
        ServerTryTend(Interactor);
    }
}


AController *AMythicFarmPlot::ResolveController(AActor *Interactor) {
    AController *Controller = Cast<AController>(Interactor);
    if (!Controller) {
        if (const APawn *Pawn = Cast<APawn>(Interactor)) {
            Controller = Pawn->GetController();
        }
    }
    return Controller;
}

UMythicItemInstance *AMythicFarmPlot::FindMatchingSeed(AActor *Interactor, UMythicCropDefinition *&OutCrop) const {
    OutCrop = nullptr;
    if (!CropRegistry) {
        return nullptr;
    }
    AController *Controller = ResolveController(Interactor);
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
                if (UMythicCropDefinition *Crop = CropRegistry->ResolveCropForSeedProbe(Probe)) {
                    OutCrop = Crop;
                    return Item;
                }
            }
        }
    }
    return nullptr;
}

UMythicItemInstance *AMythicFarmPlot::FindMatchingFertilizer(AActor *Interactor, FGameplayTag &OutFertilizerTag) const {
    OutFertilizerTag = FGameplayTag();
    AController *Controller = ResolveController(Interactor);
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
            if (!Probe.HasTag(TAG_Item_Fertilizer)) {
                continue;
            }
            for (const FGameplayTag &Tag : Probe) {
                if (Tag.MatchesTag(TAG_Item_Fertilizer)) {
                    OutFertilizerTag = Tag;
                    break;
                }
            }
            return Item;
        }
    }
    return nullptr;
}

AMythicCorpse *AMythicFarmPlot::FindNearbyCorpse() const {
    UWorld *World = GetWorld();
    if (!World) {
        return nullptr;
    }
    const float RadiusSq = FMath::Square(FMath::Max(100.0f, FarmingConfig.GraveburyRadius));
    const FVector PlotLoc = GetActorLocation();
    AMythicCorpse *Best = nullptr;
    float BestDistSq = RadiusSq;
    for (TActorIterator<AMythicCorpse> It(World); It; ++It) {
        AMythicCorpse *Corpse = *It;
        if (!IsValid(Corpse)) {
            continue;
        }
        const float DistSq = FVector::DistSquared(PlotLoc, Corpse->GetActorLocation());
        if (DistSq <= BestDistSq) {
            BestDistSq = DistSq;
            Best = Corpse;
        }
    }
    return Best;
}

int32 AMythicFarmPlot::ResolveProficiencyLevel(AActor *Interactor, const UProficiencyDefinition *Proficiency) {
    if (!Proficiency) {
        return 0;
    }
    AController *Controller = ResolveController(Interactor);
    const AMythicPlayerController *PC = Cast<AMythicPlayerController>(Controller);
    UProficiencyComponent *ProfComp = PC ? const_cast<UProficiencyComponent *>(PC->GetProficiencyComponent()) : nullptr;
    if (!ProfComp) {
        return 0;
    }
    for (const FProficiency &Prof : ProfComp->Proficiencies) {
        if (Prof.Definition == Proficiency) {
            UAbilitySystemComponent *ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Controller);
            if (!ASC) {
                return 0;
            }
            const float CurrentXP = ASC->GetNumericAttribute(Proficiency->GetProgressAttribute());
            return UProficiencyDefinition::CalcLevelAtXP(CurrentXP, Proficiency);
        }
    }
    return 0;
}

int32 AMythicFarmPlot::ResolveFarmingLevel(AActor *Interactor, UMythicCropDefinition *Crop) const {
    return Crop ? ResolveProficiencyLevel(Interactor, Crop->FarmingProficiency) : 0;
}


void AMythicFarmPlot::OnPrimaryInteract_Implementation(AActor *Interactor) {
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

void AMythicFarmPlot::OnSecondaryInteract_Implementation(AActor *Interactor) {
    if (HasAuthority()) {
        ServerHandleSecondaryInteract(Interactor);
    }
}

USceneComponent *AMythicFarmPlot::GetWidgetAttachmentComponent_Implementation() const {
    return SceneRoot;
}

bool AMythicFarmPlot::GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const {
    OutInteractionData.InputActionDataTable = InputActionDataTable;
    if (PlotState.bWithered) {
        OutInteractionData.PrimaryInteractionName = ClearInteractionName;
    }
    else if (IsEmpty()) {
        UMythicCropDefinition *Crop = nullptr;
        const bool bHasSeed = FindMatchingSeed(Interactor, Crop) != nullptr;
        if (!bHasSeed && FindNearbyCorpse()) {
            OutInteractionData.PrimaryInteractionName = BuryInteractionName;
        }
        else {
            OutInteractionData.PrimaryInteractionName = PlantInteractionName;
        }
    }
    else if (IsMature()) {
        OutInteractionData.PrimaryInteractionName = HarvestInteractionName;
    }
    else {
        OutInteractionData.PrimaryInteractionName = TendInteractionName;
    }
    OutInteractionData.SecondaryInteractionName = TendInteractionName;
    return true;
}

void AMythicFarmPlot::OnFocused_Implementation(AActor *Interactor) {
}

void AMythicFarmPlot::OnUnfocused_Implementation(AActor *Interactor) {
}

void AMythicFarmPlot::OnRep_PlotState() {
    OnCropVisualChanged(PlotState.Crop, PlotState.Stage);
    OnPlotConditionChanged(static_cast<float>(PlotState.MoistureQ) / 255.0f, PlotState.bWithered);
}


void AMythicFarmPlot::SerializeCustomData(TArray<uint8> &OutCustomData) {
    SampleMoisture(GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0);

    FMythicFarmPlotSaveData Data;
    Data.CropPath = PlotState.Crop ? PlotState.Crop->GetPathName() : FString();
    Data.Stage = PlotState.Stage;
    if (PlotState.Crop && !PlotState.bWithered && PlotState.Stage >= 0 && PlotState.Stage < PlotState.Crop->GetMatureStageIndex()) {
        Data.RemainingSeconds = GetRemainingModelSeconds();
    }
    Data.MoistureQ = PlotState.MoistureQ;
    Data.WetUptimeSeconds = WetUptimeSeconds;
    Data.GrowTimeSeconds = GrowTimeSeconds;
    Data.DryStreakSeconds = DryStreakSeconds;
    Data.bWithered = PlotState.bWithered;
    Data.FertilizerTagName = PlotState.FertilizerTag.IsValid() ? PlotState.FertilizerTag.ToString() : FString();
    Data.GraveEssenceTagName = PlotState.GraveEssenceTag.IsValid() ? PlotState.GraveEssenceTag.ToString() : FString();

    FMythicFarmingRules::EncodePlotSave(Data, OutCustomData);
}

void AMythicFarmPlot::DeserializeCustomData(const TArray<uint8> &InCustomData) {
    if (InCustomData.Num() == 0) {
        OnCropVisualChanged(PlotState.Crop, PlotState.Stage);
        OnPlotConditionChanged(GetMoisture01(), PlotState.bWithered);
        return;
    }

    FMythicFarmPlotSaveData Data;
    if (!FMythicFarmingRules::DecodePlotSave(InCustomData, Data)) {
        UE_LOG(MythSaveLoad, Warning, TEXT("AMythicFarmPlot: unreadable saved payload (%d bytes); restoring plot empty"),
               InCustomData.Num());
        Data = FMythicFarmPlotSaveData();
    }

    UMythicCropDefinition *Crop = nullptr;
    if (!Data.CropPath.IsEmpty()) {
        Crop = LoadObject<UMythicCropDefinition>(nullptr, *Data.CropPath);
        if (!Crop) {
            UE_LOG(MythSaveLoad, Warning, TEXT("AMythicFarmPlot: failed to load saved crop %s; restoring plot empty"), *Data.CropPath);
        }
    }

    if (!Crop) {
        PlotState.Crop = nullptr;
        PlotState.Stage = STAGE_EMPTY;
        PlotState.bWithered = false;
        PlotState.FertilizerTag = FGameplayTag();
        PlotState.GraveEssenceTag = FGameplayTag();
    }
    else {
        PlotState.Crop = Crop;
        PlotState.Stage = Data.Stage;
        PlotState.MoistureQ = Data.MoistureQ;
        PlotState.bWithered = Data.bWithered;
        PlotState.FertilizerTag = Data.FertilizerTagName.IsEmpty()
                                      ? FGameplayTag()
                                      : FGameplayTag::RequestGameplayTag(FName(*Data.FertilizerTagName), false);
        PlotState.GraveEssenceTag = Data.GraveEssenceTagName.IsEmpty()
                                        ? FGameplayTag()
                                        : FGameplayTag::RequestGameplayTag(FName(*Data.GraveEssenceTagName), false);
        WetUptimeSeconds = Data.WetUptimeSeconds;
        GrowTimeSeconds = Data.GrowTimeSeconds;
        DryStreakSeconds = Data.DryStreakSeconds;
        MoistureSampleTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

        const int32 MatureStage = Crop->GetMatureStageIndex();
        if (HasAuthority() && !Data.bWithered && Data.Stage >= 0 && Data.Stage < MatureStage) {
            ArmGrowthTimerForModelSeconds(static_cast<float>(Data.RemainingSeconds), CurrentGrowthSpeed());
        }
    }

    if (HasAuthority()) {
        FlushNetDormancy();
        UpdatePressureRegistration();
    }
    OnCropVisualChanged(PlotState.Crop, PlotState.Stage);
    OnPlotConditionChanged(static_cast<float>(PlotState.MoistureQ) / 255.0f, PlotState.bWithered);
}
