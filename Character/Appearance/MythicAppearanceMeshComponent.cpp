#include "MythicAppearanceMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

UMaterialInstanceDynamic* UMythicAppearanceMeshComponent::GetPaletteMaterial(int32 Slot,UMaterialInterface* Base) {
    if (Slot<0 || !Base) return nullptr;
    if (auto* Existing=PaletteMaterials.FindRef(Slot).Get()) {
        // Part bindings are immutable for this component. Updating this exact
        // instance also updates what the camera will restore after occlusion.
        return ensure(Existing->Parent==Base)?Existing:nullptr;
    }
    auto* Material=UMaterialInstanceDynamic::Create(Base,this);
    PaletteMaterials.Add(Slot,Material);
    SetMaterial(Slot,Material);
    return Material;
}
