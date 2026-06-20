#include "ConversionViewModels.h"

#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameStateBase.h"
#include "Itemization/Conversion/ConversionRecipe.h"
#include "Itemization/Conversion/ConversionStationComponent.h"
#include "Itemization/Conversion/ConversionSubsystem.h"
#include "Itemization/Conversion/MythicConversionStation.h"
#include "Itemization/InventoryProviderInterface.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Player/MythicPlayerController.h"

namespace {
    UTexture2D *LoadIcon(const TSoftObjectPtr<UTexture2D> &Ref) {
        return Ref.IsNull() ? nullptr : Ref.LoadSynchronous();
    }

    UTexture2D *FirstProductIcon(const UConversionRecipe *Recipe) {
        if (!Recipe) {
            return nullptr;
        }
        for (const FConversionProduct &P : Recipe->Products) {
            if (P.Mode == EConversionProductMode::Create && !P.ItemDefinition.IsNull()) {
                if (UItemDefinition *Def = P.ItemDefinition.LoadSynchronous()) {
                    return LoadIcon(Def->Icon2d);
                }
            }
        }
        return LoadIcon(Recipe->Icon);
    }

    FText StallReasonToText(EConversionStallReason Reason) {
        switch (Reason) {
        case EConversionStallReason::NoFuel:
            return NSLOCTEXT("Conversion", "StallNoFuel", "Out of fuel");
        case EConversionStallReason::OutputFull:
            return NSLOCTEXT("Conversion", "StallOutputFull", "Output full");
        case EConversionStallReason::MissingInput:
            return NSLOCTEXT("Conversion", "StallInput", "Missing input");
        case EConversionStallReason::MissingCatalyst:
            return NSLOCTEXT("Conversion", "StallCatalyst", "Missing tool");
        case EConversionStallReason::NotLoaded:
            return NSLOCTEXT("Conversion", "StallLoading", "Loading...");
        default:
            return FText::GetEmpty();
        }
    }

    void GatherProviderInstances(AActor *Interactor, TArray<UMythicItemInstance *> &Out) {
        if (IInventoryProviderInterface *Prov = Cast<IInventoryProviderInterface>(Interactor)) {
            for (UMythicInventoryComponent *Inv : Prov->GetAllInventoryComponents()) {
                if (!Inv) {
                    continue;
                }
                for (const FMythicInventorySlotEntry &Slot : Inv->GetAllSlots()) {
                    if (Slot.SlottedItemInstance) {
                        Out.Add(Slot.SlottedItemInstance);
                    }
                }
            }
        }
    }
} // namespace

// ==================== UConversionIngredientVM ====================
void UConversionIngredientVM::SetIcon(UTexture2D *In) { if (UE_MVVM_SET_PROPERTY_VALUE(Icon, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Icon); } }
void UConversionIngredientVM::SetHaveCount(int32 In) { if (UE_MVVM_SET_PROPERTY_VALUE(HaveCount, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(HaveCount); } }
void UConversionIngredientVM::SetNeedCount(int32 In) { if (UE_MVVM_SET_PROPERTY_VALUE(NeedCount, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(NeedCount); } }
void UConversionIngredientVM::SetSufficient(bool In) { if (UE_MVVM_SET_PROPERTY_VALUE(Sufficient, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Sufficient); } }
void UConversionIngredientVM::SetConsumed(bool In) { if (UE_MVVM_SET_PROPERTY_VALUE(Consumed, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Consumed); } }

void UConversionIngredientVM::SetTooltipName(FText In) {
    if (UE_MVVM_SET_PROPERTY_VALUE(TooltipName, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(TooltipName); }
}

// ==================== UConversionOutputVM ====================
void UConversionOutputVM::SetIcon(UTexture2D *In) { if (UE_MVVM_SET_PROPERTY_VALUE(Icon, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Icon); } }
void UConversionOutputVM::SetQuantity(int32 In) { if (UE_MVVM_SET_PROPERTY_VALUE(Quantity, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Quantity); } }
void UConversionOutputVM::SetChanceText(FText In) { if (UE_MVVM_SET_PROPERTY_VALUE(ChanceText, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ChanceText); } }

void UConversionOutputVM::SetPreservesAffixes(bool In) {
    if (UE_MVVM_SET_PROPERTY_VALUE(PreservesAffixes, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(PreservesAffixes); }
}

void UConversionOutputVM::SetTooltipName(FText In) { if (UE_MVVM_SET_PROPERTY_VALUE(TooltipName, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(TooltipName); } }

// ==================== UConversionRecipeVM ====================
void UConversionRecipeVM::SetRecipeId(FGameplayTag In) { if (UE_MVVM_SET_PROPERTY_VALUE(RecipeId, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(RecipeId); } }
void UConversionRecipeVM::SetDisplayName(FText In) { if (UE_MVVM_SET_PROPERTY_VALUE(DisplayName, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(DisplayName); } }

void UConversionRecipeVM::SetResultIcon(UTexture2D *In) {
    if (UE_MVVM_SET_PROPERTY_VALUE(ResultIcon, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ResultIcon); }
}

void UConversionRecipeVM::SetCanCraft(bool In) { if (UE_MVVM_SET_PROPERTY_VALUE(CanCraft, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CanCraft); } }

void UConversionRecipeVM::SetIsLockedByTag(bool In) {
    if (UE_MVVM_SET_PROPERTY_VALUE(IsLockedByTag, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsLockedByTag); }
}

void UConversionRecipeVM::SetReasonItCant(FText In) {
    if (UE_MVVM_SET_PROPERTY_VALUE(ReasonItCant, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ReasonItCant); }
}

void UConversionRecipeVM::SetTimePerBatchSeconds(float In) {
    if (UE_MVVM_SET_PROPERTY_VALUE(TimePerBatchSeconds, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(TimePerBatchSeconds); }
}

void UConversionRecipeVM::SetIngredients(TArray<TObjectPtr<UConversionIngredientVM>> In) {
    if (UE_MVVM_SET_PROPERTY_VALUE(Ingredients, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Ingredients); }
}

void UConversionRecipeVM::SetOutputs(TArray<TObjectPtr<UConversionOutputVM>> In) {
    if (UE_MVVM_SET_PROPERTY_VALUE(Outputs, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Outputs); }
}

void UConversionRecipeVM::RefreshFor(UConversionRecipe *Recipe, UConversionStationComponent *Component, AActor *Interactor) {
    if (!Recipe) {
        SetDisplayName(NSLOCTEXT("Conversion", "Loading", "Loading..."));
        return;
    }

    SetRecipeId(Recipe->RecipeId);
    SetDisplayName(Recipe->DisplayName);
    SetResultIcon(FirstProductIcon(Recipe));
    SetTimePerBatchSeconds(Recipe->Process.Duration);

    FText Reason;
    const bool bEligible = Component ? Component->EvaluateEligibility(Recipe, Interactor, Reason) : true;
    SetIsLockedByTag(!bEligible);
    SetReasonItCant(Reason);

    // Available materials (from the interactor's inventories).
    TArray<UMythicItemInstance *> Owned;
    GatherProviderInstances(Interactor, Owned);

    TArray<TObjectPtr<UConversionIngredientVM>> IngVMs;
    bool bAllSufficient = true;
    for (const FConversionIngredient &Ing : Recipe->Inputs) {
        UConversionIngredientVM *VM = NewObject<UConversionIngredientVM>(this);
        int32 Have = 0;
        for (UMythicItemInstance *I : Owned) {
            if (I && Ing.MatchesInstance(I)) {
                Have += I->GetStacks();
            }
        }
        VM->SetNeedCount(Ing.RequiredAmount);
        VM->SetHaveCount(Have);
        VM->SetConsumed(Ing.bConsumed);
        VM->SetSufficient(Have >= Ing.RequiredAmount);
        if (Ing.MatchMode == EConversionMatchMode::ExactItem && !Ing.ExactItem.IsNull()) {
            if (UItemDefinition *Def = Ing.ExactItem.LoadSynchronous()) {
                VM->SetIcon(LoadIcon(Def->Icon2d));
                VM->SetTooltipName(Def->Name);
            }
        }
        if (Have < Ing.RequiredAmount) {
            bAllSufficient = false;
        }
        IngVMs.Add(VM);
    }
    SetIngredients(IngVMs);

    TArray<TObjectPtr<UConversionOutputVM>> OutVMs;
    for (const FConversionProduct &P : Recipe->Products) {
        UConversionOutputVM *VM = NewObject<UConversionOutputVM>(this);
        VM->SetQuantity(P.Mode == EConversionProductMode::Create ? P.Quantity : 1);
        VM->SetPreservesAffixes(P.Mode == EConversionProductMode::Transform);
        if (P.Probability < 1.f) {
            VM->SetChanceText(FText::FromString(FString::Printf(TEXT("~%.0f%%"), P.Probability * 100.f)));
        }
        if (P.Mode == EConversionProductMode::Create && !P.ItemDefinition.IsNull()) {
            if (UItemDefinition *Def = P.ItemDefinition.LoadSynchronous()) {
                VM->SetIcon(LoadIcon(Def->Icon2d));
                VM->SetTooltipName(Def->Name);
            }
        }
        OutVMs.Add(VM);
    }
    SetOutputs(OutVMs);

    SetCanCraft(bEligible && bAllSufficient);
}

// ==================== UConversionJobVM ====================
void UConversionJobVM::SetJobId(int32 In) { if (UE_MVVM_SET_PROPERTY_VALUE(JobId, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(JobId); } }
void UConversionJobVM::SetDisplayName(FText In) { if (UE_MVVM_SET_PROPERTY_VALUE(DisplayName, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(DisplayName); } }
void UConversionJobVM::SetResultIcon(UTexture2D *In) { if (UE_MVVM_SET_PROPERTY_VALUE(ResultIcon, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ResultIcon); } }
void UConversionJobVM::SetBatchText(FText In) { if (UE_MVVM_SET_PROPERTY_VALUE(BatchText, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(BatchText); } }
void UConversionJobVM::SetProgress(float In) { if (UE_MVVM_SET_PROPERTY_VALUE(Progress, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Progress); } }

void UConversionJobVM::SetTimeRemainingText(FText In) {
    if (UE_MVVM_SET_PROPERTY_VALUE(TimeRemainingText, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(TimeRemainingText); }
}

void UConversionJobVM::SetIsActive(bool In) { if (UE_MVVM_SET_PROPERTY_VALUE(IsActive, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsActive); } }
void UConversionJobVM::SetIsStalled(bool In) { if (UE_MVVM_SET_PROPERTY_VALUE(IsStalled, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsStalled); } }

void UConversionJobVM::SetStallReasonText(FText In) {
    if (UE_MVVM_SET_PROPERTY_VALUE(StallReasonText, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(StallReasonText); }
}

void UConversionJobVM::SetCanCancel(bool In) { if (UE_MVVM_SET_PROPERTY_VALUE(CanCancel, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CanCancel); } }

void UConversionJobVM::RefreshFromEntry(const FConversionJobEntry &Entry, UConversionSubsystem *Subsystem, bool bOwnedByViewer) {
    SetJobId(Entry.JobId);

    UConversionRecipe *Recipe = Subsystem ? Subsystem->GetRecipeById(Entry.RecipeId) : nullptr;
    if (!Recipe) {
        SetDisplayName(NSLOCTEXT("Conversion", "Loading", "Loading..."));
    }
    else {
        SetDisplayName(Recipe->DisplayName);
        SetResultIcon(FirstProductIcon(Recipe));
    }

    SetBatchText(Entry.Quantity == 0
        ? NSLOCTEXT("Conversion", "Auto", "Auto")
        : FText::FromString(FString::Printf(TEXT("x%d"), Entry.Quantity)));
    SetIsActive(Entry.State == EConversionJobState::Processing);
    SetIsStalled(Entry.State == EConversionJobState::Stalled);
    SetStallReasonText(StallReasonToText(Entry.StallReason));
    SetCanCancel(bOwnedByViewer);

    StartServerTime = Entry.CycleStartServerTime;
    EndServerTime = Entry.CycleStartServerTime + Entry.CycleDuration;
}

void UConversionJobVM::TickProgress(double NowServerTime) {
    if (IsStalled) {
        return; // frozen
    }
    if (EndServerTime - StartServerTime <= UE_KINDA_SMALL_NUMBER) {
        SetProgress(1.f);
        return;
    }
    const double Alpha = (NowServerTime - StartServerTime) / (EndServerTime - StartServerTime);
    SetProgress((float)FMath::Clamp(Alpha, 0.0, 1.0));

    const double Remaining = FMath::Max(0.0, EndServerTime - NowServerTime);
    SetTimeRemainingText(FText::FromString(FString::Printf(TEXT("%.0fs"), Remaining)));
}

// ==================== UConversionStationVM ====================
void UConversionStationVM::SetStationName(FText In) { if (UE_MVVM_SET_PROPERTY_VALUE(StationName, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(StationName); } }
void UConversionStationVM::SetUsesFuel(bool In) { if (UE_MVVM_SET_PROPERTY_VALUE(UsesFuel, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(UsesFuel); } }

void UConversionStationVM::SetFuelFraction(float In) {
    if (UE_MVVM_SET_PROPERTY_VALUE(FuelFraction, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(FuelFraction); }
}

void UConversionStationVM::SetIsLit(bool In) { if (UE_MVVM_SET_PROPERTY_VALUE(IsLit, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsLit); } }
void UConversionStationVM::SetAutoRepeat(bool In) { if (UE_MVVM_SET_PROPERTY_VALUE(AutoRepeat, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(AutoRepeat); } }

void UConversionStationVM::SetSelectedRecipeId(FGameplayTag In) {
    if (UE_MVVM_SET_PROPERTY_VALUE(SelectedRecipeId, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectedRecipeId); }
}

void UConversionStationVM::SetSelectedBatchCount(int32 In) {
    if (UE_MVVM_SET_PROPERTY_VALUE(SelectedBatchCount, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectedBatchCount); }
}

void UConversionStationVM::SetRecipes(TArray<TObjectPtr<UConversionRecipeVM>> In) {
    if (UE_MVVM_SET_PROPERTY_VALUE(Recipes, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Recipes); }
}

void UConversionStationVM::SetJobs(TArray<TObjectPtr<UConversionJobVM>> In) {
    if (UE_MVVM_SET_PROPERTY_VALUE(Jobs, In)) { UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Jobs); }
}

void UConversionStationVM::InitializeForStation(UConversionStationComponent *Component, AController *Interactor) {
    StationComponent = Component;
    InteractorController = Interactor;

    if (Component && Component->GetWorld() && Component->GetWorld()->GetGameInstance()) {
        Subsystem = Component->GetWorld()->GetGameInstance()->GetSubsystem<UConversionSubsystem>();
    }

    if (Component) {
        Component->OnJobsChanged.AddDynamic(this, &UConversionStationVM::HandleJobsChanged);
        Component->OnFuelChanged.AddDynamic(this, &UConversionStationVM::HandleFuelChanged);

        FGameplayTag FirstTag;
        for (const FGameplayTag &T : Component->GetStationTags()) {
            FirstTag = T;
            break;
        }
        if (FirstTag.IsValid()) {
            SetStationName(FText::FromName(FirstTag.GetTagName()));
        }
    }

    if (Subsystem.IsValid() && !Subsystem->AreRecipesReady()) {
        Subsystem->OnRecipesReady.AddUObject(this, &UConversionStationVM::HandleRecipesReady);
    }
    else {
        RebuildRecipes();
    }

    RebuildJobs();
    HandleFuelChanged();

    if (!TickerHandle.IsValid()) {
        TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateUObject(this, &UConversionStationVM::TickProgress), 0.0f);
    }
}

void UConversionStationVM::HandleRecipesReady() {
    RebuildRecipes();
}

void UConversionStationVM::RebuildRecipes() {
    UConversionStationComponent *Component = StationComponent.Get();
    if (!Component || !Subsystem.IsValid()) {
        return;
    }

    UAbilitySystemComponent *ASC = nullptr;
    if (IInventoryProviderInterface *Prov = Cast<IInventoryProviderInterface>(InteractorController.Get())) {
        ASC = Prov->GetSchematicsASC();
    }

    TArray<UConversionRecipe *> Found;
    Subsystem->GetRecipesForStation(Component->GetStationTags(), ASC, Found);

    bool bAnyFuel = false;
    TArray<TObjectPtr<UConversionRecipeVM>> VMs;
    for (UConversionRecipe *R : Found) {
        if (!R) {
            continue;
        }
        UConversionRecipeVM *VM = NewObject<UConversionRecipeVM>(this);
        VM->RefreshFor(R, Component, InteractorController.Get());
        VMs.Add(VM);
        bAnyFuel |= R->Process.bRequiresFuel;
    }
    SetRecipes(VMs);
    SetUsesFuel(bAnyFuel);
}

void UConversionStationVM::RebuildJobs() {
    UConversionStationComponent *Component = StationComponent.Get();
    if (!Component) {
        return;
    }

    TArray<TObjectPtr<UConversionJobVM>> VMs;
    for (const FConversionJobEntry &Entry : Component->GetJobs().GetItems()) {
        UConversionJobVM *VM = NewObject<UConversionJobVM>(this);
        // Heuristic ownership: unbounded (auto) jobs are not player-cancelable; the server re-validates anyway.
        VM->RefreshFromEntry(Entry, Subsystem.Get(), Entry.Quantity != 0);
        VMs.Add(VM);
    }
    SetJobs(VMs);

    // Reflect the head job's auto-repeat state.
    const TArray<FConversionJobEntry> &Items = Component->GetJobs().GetItems();
    SetAutoRepeat(Items.Num() > 0 && Items[0].Quantity == 0);
}

void UConversionStationVM::HandleJobsChanged() {
    RebuildJobs();
}

void UConversionStationVM::HandleFuelChanged() {
    UConversionStationComponent *Component = StationComponent.Get();
    if (!Component) {
        return;
    }
    const FConversionFuelState &Fuel = Component->GetFuelState();
    SetIsLit(Fuel.bBurning);
    SetFuelFraction(Fuel.CapacityHintSeconds > 0.f ? FMath::Clamp(Fuel.BufferedBurnSeconds / Fuel.CapacityHintSeconds, 0.f, 1.f) : 0.f);
}

bool UConversionStationVM::TickProgress(float DeltaTime) {
    UConversionStationComponent *Component = StationComponent.Get();
    if (!Component || !Component->GetWorld()) {
        return true;
    }
    const AGameStateBase *GS = Component->GetWorld()->GetGameState();
    if (!GS) {
        return true; // freeze while no game state
    }
    const double Now = GS->GetServerWorldTimeSeconds();
    for (const TObjectPtr<UConversionJobVM> &JobVM : Jobs) {
        if (JobVM && JobVM->GetIsActive()) {
            JobVM->TickProgress(Now);
        }
    }
    return true;
}

void UConversionStationVM::SelectRecipe(FGameplayTag RecipeId) {
    SetSelectedRecipeId(RecipeId);
}

void UConversionStationVM::SetBatchCount(int32 Count) {
    SetSelectedBatchCount(FMath::Clamp(Count, 1, 999));
}

void UConversionStationVM::RequestStartSelected() {
    UConversionStationComponent *Component = StationComponent.Get();
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(InteractorController.Get());
    if (!Component || !PC || !SelectedRecipeId.IsValid()) {
        return;
    }
    if (AMythicConversionStation *Station = Cast<AMythicConversionStation>(Component->GetOwner())) {
        PC->ServerConversionRequestStart(Station, SelectedRecipeId, FMath::Max(1, SelectedBatchCount));
    }
}

void UConversionStationVM::RequestCancelJob(int32 InJobId) {
    UConversionStationComponent *Component = StationComponent.Get();
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(InteractorController.Get());
    if (!Component || !PC) {
        return;
    }
    if (AMythicConversionStation *Station = Cast<AMythicConversionStation>(Component->GetOwner())) {
        PC->ServerConversionCancelJob(Station, InJobId);
    }
}

void UConversionStationVM::RequestSetAutoRepeat(bool bRepeat) {
    SetAutoRepeat(bRepeat);
    UConversionStationComponent *Component = StationComponent.Get();
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(InteractorController.Get());
    if (!Component || !PC) {
        return;
    }
    if (AMythicConversionStation *Station = Cast<AMythicConversionStation>(Component->GetOwner())) {
        PC->ServerConversionSetAutoRepeat(Station, bRepeat);
    }
}

void UConversionStationVM::BeginDestroy() {
    if (TickerHandle.IsValid()) {
        FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
        TickerHandle.Reset();
    }
    if (UConversionStationComponent *Component = StationComponent.Get()) {
        Component->OnJobsChanged.RemoveDynamic(this, &UConversionStationVM::HandleJobsChanged);
        Component->OnFuelChanged.RemoveDynamic(this, &UConversionStationVM::HandleFuelChanged);
    }
    if (Subsystem.IsValid()) {
        Subsystem->OnRecipesReady.RemoveAll(this);
    }
    Super::BeginDestroy();
}
