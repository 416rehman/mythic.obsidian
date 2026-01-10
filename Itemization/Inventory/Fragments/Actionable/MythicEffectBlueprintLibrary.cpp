#include "MythicEffectBlueprintLibrary.h"

FText UMythicEffectBlueprintLibrary::GetEffectRichText(const FMythicEffectDisplayData &Data, UAbilitySystemComponent *ASC, bool bShowContext) {
    if (ASC) {
        return Data.GetRichText(ASC, bShowContext);
    }
    return Data.GetRichText();
}

FText UMythicEffectBlueprintLibrary::GetModifierRichText(const FMythicEffectModifierDisplayData &Data, UAbilitySystemComponent *ASC, bool bShowContext) {
    if (ASC) {
        return Data.GetRichText(ASC, bShowContext);
    }
    return Data.GetRichText();
}

FText UMythicEffectBlueprintLibrary::GetDurationRichText(const FMythicEffectDurationDisplayData &Data, UAbilitySystemComponent *ASC, bool bShowContext) {
    if (ASC) {
        return Data.GetRichText(ASC, bShowContext);
    }
    return Data.GetRichText();
}

FText UMythicEffectBlueprintLibrary::GetStackingRichText(const FMythicEffectStackingDisplayData &Data) {
    return Data.GetRichText();
}
