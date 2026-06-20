#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "GameplayTagContainer.h"
#include "Containers/Ticker.h"
#include "ConversionViewModels.generated.h"

class UConversionStationComponent;
class UConversionRecipe;
class UConversionSubsystem;
class UInventoryVM;
class UMythicInventoryComponent;
class AController;
class UTexture2D;
struct FConversionJobEntry;

/** One ingredient row of a recipe (have / need / catalyst flag). */
UCLASS(BlueprintType)
class MYTHIC_API UConversionIngredientVM : public UMVVMViewModelBase {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    TObjectPtr<UTexture2D> Icon;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    int32 HaveCount = 0;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    int32 NeedCount = 0;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    bool Sufficient = false;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    bool Consumed = true;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    FText TooltipName;

public:
    void SetIcon(UTexture2D *In);
    UTexture2D *GetIcon() const { return Icon; }
    void SetHaveCount(int32 In);
    int32 GetHaveCount() const { return HaveCount; }
    void SetNeedCount(int32 In);
    int32 GetNeedCount() const { return NeedCount; }
    void SetSufficient(bool In);
    bool GetSufficient() const { return Sufficient; }
    void SetConsumed(bool In);
    bool GetConsumed() const { return Consumed; }
    void SetTooltipName(FText In);
    FText GetTooltipName() const { return TooltipName; }
};

/** One output row of a recipe. */
UCLASS(BlueprintType)
class MYTHIC_API UConversionOutputVM : public UMVVMViewModelBase {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    TObjectPtr<UTexture2D> Icon;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    int32 Quantity = 1;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    FText ChanceText;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    bool PreservesAffixes = false;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    FText TooltipName;

public:
    void SetIcon(UTexture2D *In);
    UTexture2D *GetIcon() const { return Icon; }
    void SetQuantity(int32 In);
    int32 GetQuantity() const { return Quantity; }
    void SetChanceText(FText In);
    FText GetChanceText() const { return ChanceText; }
    void SetPreservesAffixes(bool In);
    bool GetPreservesAffixes() const { return PreservesAffixes; }
    void SetTooltipName(FText In);
    FText GetTooltipName() const { return TooltipName; }
};

/** One recipe row in the station's recipe list. */
UCLASS(BlueprintType)
class MYTHIC_API UConversionRecipeVM : public UMVVMViewModelBase {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    FGameplayTag RecipeId;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    FText DisplayName;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    TObjectPtr<UTexture2D> ResultIcon;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    bool CanCraft = false;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    bool IsLockedByTag = false;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    FText ReasonItCant;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    float TimePerBatchSeconds = 0.f;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    TArray<TObjectPtr<UConversionIngredientVM>> Ingredients;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    TArray<TObjectPtr<UConversionOutputVM>> Outputs;

public:
    void SetRecipeId(FGameplayTag In);
    FGameplayTag GetRecipeId() const { return RecipeId; }
    void SetDisplayName(FText In);
    FText GetDisplayName() const { return DisplayName; }
    void SetResultIcon(UTexture2D *In);
    UTexture2D *GetResultIcon() const { return ResultIcon; }
    void SetCanCraft(bool In);
    bool GetCanCraft() const { return CanCraft; }
    void SetIsLockedByTag(bool In);
    bool GetIsLockedByTag() const { return IsLockedByTag; }
    void SetReasonItCant(FText In);
    FText GetReasonItCant() const { return ReasonItCant; }
    void SetTimePerBatchSeconds(float In);
    float GetTimePerBatchSeconds() const { return TimePerBatchSeconds; }
    void SetIngredients(TArray<TObjectPtr<UConversionIngredientVM>> In);
    TArray<TObjectPtr<UConversionIngredientVM>> GetIngredients() const { return Ingredients; }
    void SetOutputs(TArray<TObjectPtr<UConversionOutputVM>> In);
    TArray<TObjectPtr<UConversionOutputVM>> GetOutputs() const { return Outputs; }

    // Rebuild from the recipe + station + interactor (eligibility, have/need counts).
    void RefreshFor(UConversionRecipe *Recipe, UConversionStationComponent *Component, AActor *Interactor);
};

/** One active/queued job row, with live progress. */
UCLASS(BlueprintType)
class MYTHIC_API UConversionJobVM : public UMVVMViewModelBase {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    int32 JobId = 0;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    FText DisplayName;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    TObjectPtr<UTexture2D> ResultIcon;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    FText BatchText;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    float Progress = 0.f;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    FText TimeRemainingText;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    bool IsActive = false;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    bool IsStalled = false;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    FText StallReasonText;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    bool CanCancel = false;

    double StartServerTime = 0.0;
    double EndServerTime = 0.0;

public:
    void SetJobId(int32 In);
    int32 GetJobId() const { return JobId; }
    void SetDisplayName(FText In);
    FText GetDisplayName() const { return DisplayName; }
    void SetResultIcon(UTexture2D *In);
    UTexture2D *GetResultIcon() const { return ResultIcon; }
    void SetBatchText(FText In);
    FText GetBatchText() const { return BatchText; }
    void SetProgress(float In);
    float GetProgress() const { return Progress; }
    void SetTimeRemainingText(FText In);
    FText GetTimeRemainingText() const { return TimeRemainingText; }
    void SetIsActive(bool In);
    bool GetIsActive() const { return IsActive; }
    void SetIsStalled(bool In);
    bool GetIsStalled() const { return IsStalled; }
    void SetStallReasonText(FText In);
    FText GetStallReasonText() const { return StallReasonText; }
    void SetCanCancel(bool In);
    bool GetCanCancel() const { return CanCancel; }

    void RefreshFromEntry(const FConversionJobEntry &Entry, UConversionSubsystem *Subsystem, bool bOwnedByViewer);
    void TickProgress(double NowServerTime);
};

/** Root station ViewModel: recipe list, job list, fuel gauge, and player-intent wrappers. */
UCLASS(BlueprintType)
class MYTHIC_API UConversionStationVM : public UMVVMViewModelBase {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    FText StationName;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    bool UsesFuel = false;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    float FuelFraction = 0.f;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    bool IsLit = false;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    bool AutoRepeat = false;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    FGameplayTag SelectedRecipeId;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    int32 SelectedBatchCount = 1;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    TArray<TObjectPtr<UConversionRecipeVM>> Recipes;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    TArray<TObjectPtr<UConversionJobVM>> Jobs;

    UPROPERTY(Transient)
    TWeakObjectPtr<UConversionStationComponent> StationComponent;
    UPROPERTY(Transient)
    TWeakObjectPtr<AController> InteractorController;
    UPROPERTY(Transient)
    TWeakObjectPtr<UConversionSubsystem> Subsystem;

    FTSTicker::FDelegateHandle TickerHandle;

public:
    void SetStationName(FText In);
    FText GetStationName() const { return StationName; }
    void SetUsesFuel(bool In);
    bool GetUsesFuel() const { return UsesFuel; }
    void SetFuelFraction(float In);
    float GetFuelFraction() const { return FuelFraction; }
    void SetIsLit(bool In);
    bool GetIsLit() const { return IsLit; }
    void SetAutoRepeat(bool In);
    bool GetAutoRepeat() const { return AutoRepeat; }
    void SetSelectedRecipeId(FGameplayTag In);
    FGameplayTag GetSelectedRecipeId() const { return SelectedRecipeId; }
    void SetSelectedBatchCount(int32 In);
    int32 GetSelectedBatchCount() const { return SelectedBatchCount; }
    void SetRecipes(TArray<TObjectPtr<UConversionRecipeVM>> In);
    TArray<TObjectPtr<UConversionRecipeVM>> GetRecipes() const { return Recipes; }
    void SetJobs(TArray<TObjectPtr<UConversionJobVM>> In);
    TArray<TObjectPtr<UConversionJobVM>> GetJobs() const { return Jobs; }

    UFUNCTION(BlueprintCallable, Category="Conversion")
    void InitializeForStation(UConversionStationComponent *Component, AController *Interactor);

    UFUNCTION(BlueprintCallable, Category="Conversion")
    void SelectRecipe(FGameplayTag RecipeId);

    UFUNCTION(BlueprintCallable, Category="Conversion")
    void SetBatchCount(int32 Count);

    UFUNCTION(BlueprintCallable, Category="Conversion")
    void RequestStartSelected();

    UFUNCTION(BlueprintCallable, Category="Conversion")
    void RequestCancelJob(int32 JobId);

    UFUNCTION(BlueprintCallable, Category="Conversion")
    void RequestSetAutoRepeat(bool bRepeat);

    virtual void BeginDestroy() override;

protected:
    UFUNCTION()
    void HandleJobsChanged();
    UFUNCTION()
    void HandleFuelChanged();
    void HandleRecipesReady();

    void RebuildRecipes();
    void RebuildJobs();
    bool TickProgress(float DeltaTime);
};
