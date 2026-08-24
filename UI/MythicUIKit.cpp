// Copyright Stellar Games. All Rights Reserved.

#include "MythicUIKit.h"

#include "Materials/MaterialInterface.h"
#include "UI/MythicUIStyle.h"
#include "Mythic.h"

#if WITH_EDITOR
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "Mythic"

const TSoftObjectPtr<UMaterialInterface> &FMythicUIComponent::ForState(EMythicUIState State) const {
    switch (State) {
        case EMythicUIState::Hovered:
            return Hovered.IsNull() ? Normal : Hovered;
        case EMythicUIState::Pressed:
            return Pressed.IsNull() ? Normal : Pressed;
        case EMythicUIState::Disabled:
            return Disabled.IsNull() ? Normal : Disabled;
        case EMythicUIState::Selected:
            return Selected.IsNull() ? Normal : Selected;
        default:
            return Normal;
    }
}

const UMythicUIKit *UMythicUIKit::Get() {
    return FMythicUIStyle::Get().Kit.LoadSynchronous();
}

const FMythicUIComponent *UMythicUIKit::Find(FName Id) const {
    return Components.FindByPredicate([Id](const FMythicUIComponent &C) { return C.Id == Id; });
}

FSlateBrush UMythicUIKit::MakeBrush(FName Id, EMythicUIState State, FVector2D Size) const {
    FSlateBrush Brush;
    Brush.ImageSize = Size;

    const FMythicUIComponent *Component = Find(Id);
    if (!Component) {
        UE_LOG(Myth, Warning, TEXT("UI kit has no component '%s'"), *Id.ToString());
        Brush.DrawAs = ESlateBrushDrawType::NoDrawType;
        return Brush;
    }
    if (UMaterialInterface *Material = Component->ForState(State).LoadSynchronous()) {
        Brush.SetResourceObject(Material);
        Brush.DrawAs = ESlateBrushDrawType::Image;
    }
    else {
        Brush.DrawAs = ESlateBrushDrawType::NoDrawType;
    }
    return Brush;
}

#if WITH_EDITOR
EDataValidationResult UMythicUIKit::IsDataValid(FDataValidationContext &Context) const {
    EDataValidationResult Result = Super::IsDataValid(Context);

    TSet<FName> SeenIds;
    TSet<FString> Claimed;

    for (const FMythicUIComponent &C : Components) {
        if (C.Id.IsNone()) {
            Context.AddError(LOCTEXT("KitNoId", "A component has no Id, so nothing can ask for it."));
            Result = EDataValidationResult::Invalid;
            continue;
        }
        if (SeenIds.Contains(C.Id)) {
            Context.AddError(FText::Format(LOCTEXT("KitDupId", "'{0}' is catalogued twice."),
                                           FText::FromName(C.Id)));
            Result = EDataValidationResult::Invalid;
        }
        SeenIds.Add(C.Id);

        if (C.Purpose.IsEmpty()) {
            Context.AddWarning(FText::Format(
                LOCTEXT("KitNoPurpose", "'{0}' says nothing about when to use it, so nobody will."),
                FText::FromName(C.Id)));
        }

        // Art that has moved or been deleted is worse than art that was never there: callers keep asking
        // for it and silently draw nothing.
        const TSoftObjectPtr<UMaterialInterface> States[] = {C.Normal, C.Hovered, C.Pressed, C.Disabled,
                                                            C.Selected};
        bool bAnyState = false;
        for (const TSoftObjectPtr<UMaterialInterface> &Ptr : States) {
            if (Ptr.IsNull()) {
                continue;
            }
            bAnyState = true;
            Claimed.Add(Ptr.ToSoftObjectPath().GetLongPackageName());
            if (!Ptr.LoadSynchronous()) {
                Context.AddError(FText::Format(LOCTEXT("KitMissingArt", "'{0}' points at missing art: {1}"),
                                               FText::FromName(C.Id), FText::FromString(Ptr.ToString())));
                Result = EDataValidationResult::Invalid;
            }
        }
        if (!bAnyState) {
            Context.AddError(FText::Format(LOCTEXT("KitNoArt", "'{0}' names no material for any state."),
                                           FText::FromName(C.Id)));
            Result = EDataValidationResult::Invalid;
        }
    }

    // The other direction. Art nobody claims is how a second style grows beside the first: it is in the
    // project, it looks finished, and no rule says which of the two a new screen should reach for.
    const FAssetRegistryModule &ARM =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    FARFilter Filter;
    Filter.PackagePaths.Add(FName(*KitFolder));
    Filter.bRecursivePaths = true;
    TArray<FAssetData> Assets;
    ARM.Get().GetAssets(Filter, Assets);

    int32 Unclaimed = 0;
    for (const FAssetData &Asset : Assets) {
        const FString Package = Asset.PackageName.ToString();
        if (!Claimed.Contains(Package)) {
            ++Unclaimed;
            Context.AddWarning(FText::Format(
                LOCTEXT("KitUnclaimed", "{0} is in the kit folder but no component claims it."),
                FText::FromString(Package)));
        }
    }

    // Say how much was examined. Without the denominator a catalogue that matched nothing at all would
    // report exactly the same clean bill of health as one that matched everything.
    Context.AddMessage(EMessageSeverity::Info,
                       FText::Format(LOCTEXT("KitSummary",
                                             "UI kit: {0} components claiming {1} materials; {2} of {3} "
                                             "assets under {4} are unclaimed."),
                                     FText::AsNumber(Components.Num()), FText::AsNumber(Claimed.Num()),
                                     FText::AsNumber(Unclaimed), FText::AsNumber(Assets.Num()),
                                     FText::FromString(KitFolder)));
    return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
