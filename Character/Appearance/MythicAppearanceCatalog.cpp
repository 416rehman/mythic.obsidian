#include "MythicAppearanceCatalog.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Misc/DataValidation.h"

namespace {
const FName Skin(TEXT("Skin")), Hair(TEXT("Hair")), Eyes(TEXT("Eyes")), Head(TEXT("Head"));
bool ValidId(FName Id) { return !Id.IsNone() && Id.ToString().Len()<=MythicAppearanceLimits::IdLength; }
}

UMythicAppearanceCatalog* UMythicAppearanceCatalog::Get() {
    // The catalogue is metadata only; meshes, animation classes and materials remain soft dependencies.
    return GetDefault<UMythicAppearanceSettings>()->Catalog.LoadSynchronous();
}

void UMythicAppearanceCatalog::Index() const {
    const int32 Count=Parts.Num()+Frames.Num()+Gear.Num()+Palettes.Num();
    if (IndexedCount==Count) return;
    PartIndex.Reset(); FrameIndex.Reset(); GearIndex.Reset(); PaletteIndex.Reset();
    for (int32 I=0; I<Parts.Num(); ++I) PartIndex.Add(Parts[I].Id,I);
    for (int32 I=0; I<Frames.Num(); ++I) FrameIndex.Add(Frames[I].Id,I);
    for (int32 I=0; I<Gear.Num(); ++I) GearIndex.Add(Gear[I].Item.ToSoftObjectPath(),I);
    for (int32 I=0; I<Palettes.Num(); ++I) PaletteIndex.Add(Palettes[I].Id,I);
    IndexedCount=Count;
}
const FMythicAppearanceFrame* UMythicAppearanceCatalog::FindFrame(FName Id) const {
    Index(); const int32* I=FrameIndex.Find(Id); return I?&Frames[*I]:nullptr;
}
const FMythicAppearancePart* UMythicAppearanceCatalog::FindPart(FName Id) const {
    Index(); const int32* I=PartIndex.Find(Id); return I?&Parts[*I]:nullptr;
}
const FMythicAppearanceGear* UMythicAppearanceCatalog::FindGear(const UItemDefinition* Item) const {
    if (!Item) return nullptr;
    Index(); const int32* I=GearIndex.Find(FSoftObjectPath(Item)); return I?&Gear[*I]:nullptr;
}
const FMythicAppearancePalette* UMythicAppearanceCatalog::FindPalette(FName Id,FName Domain) const {
    Index(); const int32* I=PaletteIndex.Find(Id);
    return I && (Domain.IsNone() || Palettes[*I].Domain==Domain)?&Palettes[*I]:nullptr;
}

bool UMythicAppearanceCatalog::ValidatePersonalChange(const FMythicAppearanceRecipe& Before,const FMythicAppearanceRecipe& After) const {
    const auto* Frame=FindFrame(After.Frame);
    if (After.Frame!=Before.Frame && !Frame) return false;
    if ((After.Head!=Before.Head || After.Frame!=Before.Frame) && (!Frame || After.Head!=Frame->HeadId)) return false;
    if (After.Hair!=Before.Hair && After.Hair!=FName("Bald") && After.Hair!=FName("Adventurer")) return false;
    if (After.SkinPalette!=Before.SkinPalette && !FindPalette(After.SkinPalette,Skin)) return false;
    if (After.HairPalette!=Before.HairPalette && !FindPalette(After.HairPalette,Hair)) return false;
    if (After.EyePalette!=Before.EyePalette && !FindPalette(After.EyePalette,Eyes)) return false;
    return true;
}

bool UMythicAppearanceCatalog::ValidateItemChanges(const FMythicAppearanceGear& Adapter,const TArray<FMythicAppearanceColor>& Changes) const {
    if (Changes.Num()>MythicAppearanceLimits::Channels) return false;
    TSet<FName> Seen;
    for (const auto& Change:Changes) {
        if (!ValidId(Change.Channel) || Seen.Contains(Change.Channel)) return false;
        Seen.Add(Change.Channel);
        // A semantic channel must be offered for each supported fit of this physical item.
        for (const auto& Fit:Adapter.Fits) {
            bool bFound=false;
            for (const FName Id:Fit.Parts) if (const auto* Part=FindPart(Id)) {
                for (const auto& Binding:Part->Materials) if (Binding.Channel==Change.Channel) {
                    if (Binding.Channel==Skin || Binding.Channel==Hair || Binding.Channel==Eyes) return false;
                    if (!Change.Palette.IsNone() && !FindPalette(Change.Palette,Binding.PaletteDomain)) return false;
                    bFound=true;
                }
            }
            if (!bFound) return false;
        }
        if (Adapter.Fits.IsEmpty()) return false;
    }
    return true;
}

bool UMythicAppearanceCatalog::ResolveOutfit(const FMythicAppearanceRecipe& Recipe,
    const TArray<FMythicAppearanceEquippedItem>& Equipment,FMythicAppearancePublicState& Out,
    TArray<FText>* Diagnostics,bool bInspectHair) const {
    Out={};
    const auto Warn=[&](const FString& Message) { if (Diagnostics) Diagnostics->Add(FText::FromString(Message)); };
    const auto* Frame=FindFrame(Recipe.Frame);
    if (!Frame) { Frame=FindFrame(DefaultRecipe.Frame); Warn(TEXT("A saved body style is unavailable. Showing the default body.")); }
    if (!Frame) return false;
    Out.Frame=Frame->Id;
    TMap<FName,FMythicAppearancePartState> ByRegion;
    TSet<FName> EquippedRegions;
    bool bHideHair=false;
    auto AddPart=[&](FName Id,const FMythicItemAppearanceData* ItemColors) {
        const auto* Part=FindPart(Id);
        if (!Part || (!Part->Frame.IsNone() && Part->Frame!=Frame->Id) || Part->Mesh.IsNull()) return false;
        FMythicAppearancePartState State; State.Part=Id;
        TSet<FName> AddedChannels;
        for (const auto& Binding:Part->Materials) {
            if (Binding.Channel.IsNone() || AddedChannels.Contains(Binding.Channel)) continue;
            FName Palette, Fallback;
            if (Binding.Channel==Skin) { Palette=Recipe.SkinPalette; Fallback=DefaultRecipe.SkinPalette; }
            else if (Binding.Channel==Hair) { Palette=Recipe.HairPalette; Fallback=DefaultRecipe.HairPalette; }
            else if (Binding.Channel==Eyes) { Palette=Recipe.EyePalette; Fallback=DefaultRecipe.EyePalette; }
            else if (ItemColors) {
                if (const auto* Color=ItemColors->Colors.FindByPredicate([&](const auto& C){return C.Channel==Binding.Channel;})) Palette=Color->Palette;
            }
            if (!Palette.IsNone() && !FindPalette(Palette,Binding.PaletteDomain)) {
                Warn(TEXT("A saved color is unavailable. Showing the original compatible color."));
                Palette=Fallback;
            }
            if (!Palette.IsNone() && FindPalette(Palette,Binding.PaletteDomain)) State.Colors.Add({Binding.Channel,Palette});
            AddedChannels.Add(Binding.Channel);
        }
        State.Colors.Sort([](const auto& A,const auto& B){return A.Channel.LexicalLess(B.Channel);});
        ByRegion.Add(Part->Region,MoveTemp(State));
        return true;
    };
    for (FName Id:Frame->BaseParts) if (!AddPart(Id,nullptr)) return false;
    for (const auto& Item:Equipment) {
        const auto* Adapter=FindGear(Item.Item);
        if (!Adapter) continue; // Nonvisual accessories are intentionally absent from this catalogue.
        if (Adapter->Slot!=Item.Slot) return false;
        const auto* Fit=Adapter->Fits.FindByPredicate([&](const auto& F){return F.Frame==Frame->Id;});
        if (!Fit) { Warn(TEXT("This equipment's fitted model is unavailable. Keeping compatible body coverage.")); continue; }
        for (FName Region:Adapter->ReplacesRegions) {
            if (Region==Head || EquippedRegions.Contains(Region)) return false;
            ByRegion.Remove(Region);
        }
        for (FName Id:Fit->Parts) {
            const auto* Part=FindPart(Id);
            if (!Part || EquippedRegions.Contains(Part->Region)) return false;
            if (!AddPart(Id,&Item.Appearance)) return false;
            EquippedRegions.Add(Part->Region);
        }
        bHideHair|=Adapter->bCoversHair;
    }
    if (Recipe.Hair!=FName("Bald") && (!bHideHair || bInspectHair) && !Frame->HairPart.IsNone()) {
        if (!AddPart(Frame->HairPart,nullptr)) return false;
    }
    if (ByRegion.Num()>MythicAppearanceLimits::Parts) return false;
    for (FName Required:{Head,FName("Torso"),FName("Arms"),FName("Legs"),FName("Feet")}) if (!ByRegion.Contains(Required)) return false;
    TArray<FName> Regions; ByRegion.GetKeys(Regions);
    Regions.Sort([](FName A,FName B){return A.LexicalLess(B);});
    for (FName Region:Regions) Out.Parts.Add(ByRegion[Region]);
    return true;
}

bool UMythicAppearanceCatalog::ValidateCatalog(TArray<FText>& Errors) const {
    const auto Error=[&](const FString& S){Errors.Add(FText::FromString(S));};
    auto Unique=[&](const auto& Rows,const TCHAR* Kind) {
        TSet<FName> Seen;
        for (const auto& R:Rows) {
            if (!ValidId(R.Id) || Seen.Contains(R.Id)) Error(FString::Printf(TEXT("Invalid or duplicate %s ID: %s"),Kind,*R.Id.ToString()));
            Seen.Add(R.Id);
        }
    };
    Unique(Frames,TEXT("frame")); Unique(Parts,TEXT("part")); Unique(Palettes,TEXT("palette"));
    if (CopyPoseAnimation.IsNull() || !FindFrame(DefaultRecipe.Frame)) Error(TEXT("Missing default frame or Copy Pose animation."));
    if (!FindPalette(DefaultRecipe.SkinPalette,Skin) || !FindPalette(DefaultRecipe.HairPalette,Hair) || !FindPalette(DefaultRecipe.EyePalette,Eyes)) Error(TEXT("Missing complete personal default palettes."));
    for (const auto& Palette:Palettes) {
        if (Palette.Domain.IsNone() || (Palette.Scalars.IsEmpty() && Palette.Vectors.IsEmpty())) Error(TEXT("Palette has no domain or material recipe: ")+Palette.Id.ToString());
        for (const auto& P:Palette.Scalars) if (!ValidId(P.Parameter) || !FMath::IsFinite(P.Value)) Error(TEXT("Invalid scalar in ")+Palette.Id.ToString());
        for (const auto& P:Palette.Vectors) if (!ValidId(P.Parameter) || !FMath::IsFinite(P.Value.R) || !FMath::IsFinite(P.Value.G) || !FMath::IsFinite(P.Value.B) || !FMath::IsFinite(P.Value.A)) Error(TEXT("Invalid vector in ")+Palette.Id.ToString());
    }
    for (const auto& Part:Parts) {
        if (Part.Mesh.IsNull() || !ValidId(Part.Region) || Part.Materials.Num()>MythicAppearanceLimits::MaterialSlots*MythicAppearanceLimits::Channels) Error(TEXT("Invalid mesh/region/material bound: ")+Part.Id.ToString());
        TSet<FString> Bindings;
        TMap<FName,FSoftObjectPath> MaterialsBySlot;
        for (const auto& Binding:Part.Materials) {
            const FString Key=Binding.Slot.ToString()+TEXT("/")+Binding.Channel.ToString();
            const auto* Existing=MaterialsBySlot.Find(Binding.Slot);
            if (!ValidId(Binding.Slot) || Bindings.Contains(Key) || (!Binding.Channel.IsNone() && !ValidId(Binding.PaletteDomain))
                || (Existing && *Existing!=Binding.Material.ToSoftObjectPath())) Error(TEXT("Invalid material binding: ")+Part.Id.ToString());
            Bindings.Add(Key); MaterialsBySlot.Add(Binding.Slot,Binding.Material.ToSoftObjectPath());
        }
        if (MaterialsBySlot.Num()>MythicAppearanceLimits::MaterialSlots) Error(TEXT("Too many material slots: ")+Part.Id.ToString());
    }
    TSet<FSoftObjectPath> Items;
    for (const auto& Adapter:Gear) {
        if (Adapter.Item.IsNull() || Items.Contains(Adapter.Item.ToSoftObjectPath()) || Adapter.Slot==EInventorySlotType::None) Error(TEXT("Invalid or duplicate gear adapter."));
        Items.Add(Adapter.Item.ToSoftObjectPath());
        TSet<FName> FitFrames;
        for (const auto& Fit:Adapter.Fits) {
            if (FitFrames.Contains(Fit.Frame) || !FindFrame(Fit.Frame)) Error(TEXT("Duplicate or unknown gear fit."));
            FitFrames.Add(Fit.Frame);
            TSet<FName> Regions;
            for (FName Id:Fit.Parts) {
                const auto* Part=FindPart(Id);
                if (!Part || (!Part->Frame.IsNone() && Part->Frame!=Fit.Frame) || (Part && Regions.Contains(Part->Region))) Error(TEXT("Invalid fitted region: ")+Id.ToString());
                if (Part) Regions.Add(Part->Region);
            }
            for (FName Removed:Adapter.ReplacesRegions) if (!Regions.Contains(Removed) || Removed==Head) Error(TEXT("Gear removes a body region without replacing it."));
        }
        for (const auto& Frame:Frames) {
            const auto* Fit=Adapter.Fits.FindByPredicate([&](const auto& F){return F.Frame==Frame.Id;});
            if (!Fit || Fit->Parts.IsEmpty()) { Error(TEXT("Missing equipment fit for ")+Frame.Id.ToString()); continue; }
            for (FName Id:Fit->Parts) if (!FindPart(Id)) Error(TEXT("Missing fitted part: ")+Id.ToString());
        }
    }
    for (const auto& Frame:Frames) {
        if (Frame.BodyAnimation.IsNull() || Frame.GameplayAnimation.IsNull() || Frame.PreviewIdle.IsNull()) Error(TEXT("Missing frame animation: ")+Frame.Id.ToString());
        TSet<FName> Regions;
        for (FName Id:Frame.BaseParts) {
            const auto* Part=FindPart(Id);
            if (!Part || (!Part->Frame.IsNone() && Part->Frame!=Frame.Id)) { Error(TEXT("Invalid bare frame part: ")+Id.ToString()); continue; }
            if (Regions.Contains(Part->Region)) Error(TEXT("Duplicate bare body region: ")+Id.ToString());
            Regions.Add(Part->Region);
        }
        for (FName Required:{Head,FName("Torso"),FName("Arms"),FName("Legs"),FName("Feet")}) if (!Regions.Contains(Required)) Error(TEXT("Incomplete bare frame: ")+Frame.Id.ToString());
        FMythicAppearanceRecipe Recipe=DefaultRecipe; Recipe.Frame=Frame.Id;
        FMythicAppearancePublicState Plan;
        if (!ResolveOutfit(Recipe,{},Plan)) Error(TEXT("Cannot resolve bare frame: ")+Frame.Id.ToString());
    }
    return Errors.IsEmpty();
}

#if WITH_EDITOR
EDataValidationResult UMythicAppearanceCatalog::IsDataValid(FDataValidationContext& Context) const {
    TArray<FText> Errors; ValidateCatalog(Errors);
    for (const auto& E:Errors) Context.AddError(E);
    return Errors.IsEmpty()?EDataValidationResult::Valid:EDataValidationResult::Invalid;
}
void UMythicAppearanceCatalog::PostEditChangeProperty(FPropertyChangedEvent& Event) {
    IndexedCount=INDEX_NONE; Super::PostEditChangeProperty(Event);
}
#endif
