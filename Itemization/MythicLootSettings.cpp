#include "MythicLootSettings.h"

#include "Itemization/Affixes/MythicAffixCatalogue.h"
#include "UObject/UnrealType.h"

const UMythicAffixCatalogue *UMythicLootSettings::GetAffixCatalogue() const {
    // Developer settings live on the CDO, so the cache is resolved there: the accessor stays const and no
    // const_cast is needed to hold the GC edge.
    UMythicLootSettings *Settings = GetMutableDefault<UMythicLootSettings>();
    if (!Settings->bAffixCatalogueLoadAttempted) {
        Settings->bAffixCatalogueLoadAttempted = true;
        if (!Settings->AffixCatalogue.IsNull()) {
            Settings->LoadedAffixCatalogue = Settings->AffixCatalogue.LoadSynchronous();
        }
    }
    return Settings->LoadedAffixCatalogue;
}

#if WITH_EDITOR
void UMythicLootSettings::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) {
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UMythicLootSettings, AffixCatalogue)) {
        LoadedAffixCatalogue = nullptr;
        bAffixCatalogueLoadAttempted = false;
    }
}
#endif
