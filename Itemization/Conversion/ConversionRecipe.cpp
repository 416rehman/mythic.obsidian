#include "ConversionRecipe.h"

#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "Itemization/Inventory/Fragments/Passive/DurabilityFragment.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

bool FConversionIngredient::MatchesInstance(const UMythicItemInstance *Inst) const {
    if (!Inst || !Inst->GetItemDefinition()) {
        return false;
    }

    bool bTypeMatch = false;
    if (MatchMode == EConversionMatchMode::ExactItem) {
        bTypeMatch = (Inst->GetItemDefinition() == ExactItem.Get());
    }
    else {
        FGameplayTagContainer Probe;
        Inst->GetTypeProbe(Probe);
        bTypeMatch = TypeQuery.Matches(Probe);
    }
    if (!bTypeMatch) {
        return false;
    }

    if (Inst->GetItemLevel() < MinItemLevel) {
        return false;
    }

    for (const FGameplayTag &T : RequiredItemTags) {
        if (!Inst->HasTag(T)) {
            return false;
        }
    }
    return true;
}

bool UConversionRecipe::MatchesStation(const FGameplayTagContainer &StationOwnedTags) const {
    return Requirements.StationTagQuery.IsEmpty() || Requirements.StationTagQuery.Matches(StationOwnedTags);
}

bool UConversionRecipe::MatchesInputs(const TArray<UMythicItemInstance *> &InInputs) const {
    for (const FConversionIngredient &Ing : Inputs) {
        if (!Ing.bConsumed) {
            continue; // catalysts live in a separate slot group; presence is checked by VerifyCatalystsPresent
        }
        int32 Found = 0;
        for (const UMythicItemInstance *Inst : InInputs) {
            if (Inst && Ing.MatchesInstance(Inst)) {
                Found += Inst->GetStacks();
            }
        }
        if (Found < Ing.RequiredAmount) {
            return false;
        }
    }
    return true;
}

#if WITH_EDITOR

static FString Conversion_IngredientLabel(const FConversionIngredient &Ing) {
    FString Target;
    if (Ing.MatchMode == EConversionMatchMode::ExactItem) {
        Target = Ing.ExactItem.IsNull() ? TEXT("<none>") : Ing.ExactItem.GetAssetName();
    }
    else {
        Target = TEXT("Query");
    }
    return FString::Printf(TEXT("%dx %s%s"), Ing.RequiredAmount, *Target, Ing.bConsumed ? TEXT("") : TEXT(" (tool)"));
}

static FString Conversion_ProductLabel(const FConversionProduct &P) {
    if (P.Mode == EConversionProductMode::Create) {
        const FString Name = P.ItemDefinition.IsNull() ? TEXT("<none>") : P.ItemDefinition.GetAssetName();
        FString Label = FString::Printf(TEXT("%dx %s"), P.Quantity, *Name);
        if (P.Probability < 1.f) {
            Label += FString::Printf(TEXT(" (%.0f%%)"), P.Probability * 100.f);
        }
        return Label;
    }

    FString Target = P.TransformToDefinition.IsNull()
        ? (P.NewItemType.IsValid() ? P.NewItemType.ToString() : TEXT("tags"))
        : P.TransformToDefinition.GetAssetName();
    return FString::Printf(TEXT("Transform -> %s"), *Target);
}

void UConversionRecipe::RebuildDisplayLabels() {
    for (FConversionIngredient &Ing : Inputs) {
        Ing.DisplayLabel = Conversion_IngredientLabel(Ing);
    }
    for (FConversionProduct &P : Products) {
        P.DisplayLabel = Conversion_ProductLabel(P);
    }
    for (FFuelDefinition &Fuel : Process.AcceptedFuels) {
        Fuel.DisplayLabel = FString::Printf(TEXT("%s @ %.1fs/unit"), *Conversion_IngredientLabel(Fuel.FuelMatch), Fuel.BurnSecondsPerUnit);
    }
}

void UConversionRecipe::PostLoad() {
    Super::PostLoad();
    RebuildDisplayLabels();
}

void UConversionRecipe::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) {
    Super::PostEditChangeProperty(PropertyChangedEvent);
    RebuildDisplayLabels();
}

EDataValidationResult UConversionRecipe::IsDataValid(FDataValidationContext &Context) const {
    EDataValidationResult Result = Super::IsDataValid(Context);
    auto Fail = [&](const FString &Msg) {
        Context.AddError(FText::FromString(Msg));
        Result = EDataValidationResult::Invalid;
    };

    int32 ConsumedInputs = 0;
    for (const FConversionIngredient &Ing : Inputs) {
        if (Ing.bConsumed) {
            ConsumedInputs++;
        }
    }

    // 1. Must consume or produce something.
    if (ConsumedInputs == 0 && Products.Num() == 0) {
        Fail(TEXT("Recipe has no consumed inputs and no products."));
    }

    if (!RecipeId.IsValid()) {
        Fail(TEXT("Recipe has no RecipeId (stable identity is required)."));
    }

    // Process checks.
    if (Process.Timing == EConversionTiming::Timed && Process.Duration <= UE_KINDA_SMALL_NUMBER) {
        Fail(TEXT("Timed recipe must have Duration > 0 (avoids divide-by-zero in progress)."));
    }
    if (Process.Timing == EConversionTiming::Continuous && Process.Duration < 0.5f) {
        Fail(TEXT("Continuous recipe must have Duration >= 0.5 (replication-rate floor)."));
    }
    if (Process.bRequiresFuel && Process.AcceptedFuels.Num() == 0) {
        Fail(TEXT("Recipe requires fuel but lists no AcceptedFuels."));
    }
    // Instant cycles have zero duration, so they would burn zero fuel per cycle (free production after one load).
    if (Process.bRequiresFuel && Process.Timing == EConversionTiming::Instant) {
        Fail(TEXT("Instant recipes cannot require fuel (a 0-duration cycle burns no fuel). Use Timed."));
    }

    // Product checks.
    for (int32 i = 0; i < Products.Num(); i++) {
        const FConversionProduct &P = Products[i];

        if (P.Probability < 0.f || P.Probability > 1.f) {
            Fail(FString::Printf(TEXT("Product %d: Probability must be within [0,1]."), i));
        }
        else if (FMath::IsNearlyZero(P.Probability)) {
            Context.AddWarning(FText::FromString(FString::Printf(TEXT("Product %d: Probability is 0 (item will never be produced)."), i)));
        }

        if (P.Mode == EConversionProductMode::Create) {
            UItemDefinition *Def = P.ItemDefinition.LoadSynchronous();
            if (!Def) {
                Fail(FString::Printf(TEXT("Product %d (Create): ItemDefinition is not set."), i));
            }
            else if (Def->StackSizeMax > 0 && P.Quantity > Def->StackSizeMax) {
                Fail(FString::Printf(TEXT("Product %d (Create): Quantity %d exceeds the item's StackSizeMax %d. Add multiple product lines."),
                                     i, P.Quantity, Def->StackSizeMax));
            }
        }
        else { // Transform
            if (ConsumedInputs == 0) {
                Fail(FString::Printf(TEXT("Product %d (Transform): recipe has no consumed input to transform."), i));
            }
            // A failed probability roll would strip the already-consumed input and produce nothing.
            if (P.Probability < 1.f) {
                Fail(FString::Printf(TEXT("Product %d (Transform): Probability must be 1.0 (transforms cannot be chance-based)."), i));
            }

            UItemDefinition *NewDef = P.TransformToDefinition.LoadSynchronous();
            if (NewDef && P.NewItemType.IsValid() && NewDef->ItemType != P.NewItemType) {
                Fail(FString::Printf(TEXT("Product %d (Transform): NewItemType must equal TransformToDefinition->ItemType."), i));
            }

            // bRepairToFull needs a consumed input that can supply a durable item (UDurabilityFragment). Only
            // ExactItem inputs are statically checkable; a TypeQuery consumed input may match a durable item at
            // runtime (the runtime guard in UConversionStationComponent verifies durability before consuming), so
            // don't fail when a TypeQuery consumed input is present. Fail only the clearly-broken cases.
            if (P.bRepairToFull) {
                bool bAnyTypeQueryConsumed = false;
                int32 DurableExactCount = 0;
                for (const FConversionIngredient &In : Inputs) {
                    if (!In.bConsumed) {
                        continue;
                    }
                    if (In.MatchMode != EConversionMatchMode::ExactItem) {
                        bAnyTypeQueryConsumed = true;
                        continue;
                    }
                    if (UItemDefinition *D = In.ExactItem.LoadSynchronous()) {
                        if (UItemDefinition::GetFragment<UDurabilityFragment>(D)) {
                            DurableExactCount++;
                        }
                    }
                }
                if (DurableExactCount == 0 && !bAnyTypeQueryConsumed) {
                    Fail(FString::Printf(TEXT("Product %d (Transform): bRepairToFull is set but no consumed input can "
                        "supply a durable item (no consumed ExactItem carries a UDurabilityFragment, "
                        "and no TypeQuery consumed input) — the repair has nothing to repair."), i));
                }
                if (DurableExactCount > 1) {
                    Fail(FString::Printf(TEXT("Product %d (Transform): bRepairToFull is ambiguous — %d consumed ExactItem "
                        "inputs carry a UDurabilityFragment. Use exactly one durable input."), i, DurableExactCount));
                }
            }

            for (const FConversionIngredient &Ing : Inputs) {
                if (!Ing.bConsumed || Ing.MatchMode != EConversionMatchMode::ExactItem) {
                    continue;
                }
                UItemDefinition *InDef = Ing.ExactItem.LoadSynchronous();
                if (InDef && InDef->StackSizeMax > 1) {
                    Fail(FString::Printf(TEXT("Product %d (Transform): consumed input '%s' is stackable; transform requires a non-stackable input."),
                                         i, *InDef->GetName()));
                }
                if (NewDef && InDef) {
                    bool bSameSchema = (InDef->Fragments.Num() == NewDef->Fragments.Num());
                    for (int32 f = 0; bSameSchema && f < InDef->Fragments.Num(); f++) {
                        UClass *A = InDef->Fragments[f] ? InDef->Fragments[f]->GetClass() : nullptr;
                        UClass *B = NewDef->Fragments[f] ? NewDef->Fragments[f]->GetClass() : nullptr;
                        if (A != B) {
                            bSameSchema = false;
                        }
                    }
                    if (!bSameSchema) {
                        Fail(FString::Printf(TEXT("Product %d (Transform): TransformToDefinition fragment schema differs from input '%s'."),
                                             i, *InDef->GetName()));
                    }
                }
            }

            // A Transform consumes/produces exactly ONE non-stackable item per cycle, so every consumed input able to
            // pair with a Transform product must require exactly one. A higher RequiredAmount releases extra instances
            // that are held for the job but never transformed/routed (then refunded untouched) — a consumed-vs-produced
            // accounting mismatch the player sees as items vanishing and reappearing.
            for (const FConversionIngredient &Ing : Inputs) {
                if (Ing.bConsumed
                    && (Ing.MatchMode == EConversionMatchMode::ExactItem || Ing.MatchMode == EConversionMatchMode::TypeQuery)
                    && Ing.RequiredAmount != 1) {
                    Fail(FString::Printf(TEXT("Product %d (Transform): a consumed input has RequiredAmount=%d, but a Transform "
                                              "consumes exactly ONE non-stackable item per cycle — set RequiredAmount=1."),
                                         i, Ing.RequiredAmount));
                }
            }
        }
    }

    return Result;
}

#endif // WITH_EDITOR
