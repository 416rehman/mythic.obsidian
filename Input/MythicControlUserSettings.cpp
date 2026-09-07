#include "MythicControlUserSettings.h"
#include "MythicControlsSettings.h"
#include "EnhancedActionKeyMapping.h"
#include "PlayerMappableKeySettings.h"
#include "UObject/UnrealType.h"

bool MythicControlMapping::Configure(FEnhancedActionKeyMapping& Mapping,UObject* Owner,FName Name,const FText& Label) {
    // UE5.8 exposes per-mapping metadata through reflected authoring fields, without native setter methods.
    static const auto* Behavior=FindFProperty<FEnumProperty>(FEnhancedActionKeyMapping::StaticStruct(),TEXT("SettingBehavior"));
    static const auto* SettingsProperty=FindFProperty<FObjectPropertyBase>(FEnhancedActionKeyMapping::StaticStruct(),TEXT("PlayerMappableKeySettings"));
    if (!Behavior || !SettingsProperty || !Owner || Name.IsNone()) return false;
    auto* Settings=NewObject<UPlayerMappableKeySettings>(Owner); Settings->Name=Name; Settings->DisplayName=Label;
    Behavior->GetUnderlyingProperty()->SetIntPropertyValue(Behavior->ContainerPtrToValuePtr<void>(&Mapping),static_cast<uint64>(EPlayerMappableKeySettingBehaviors::OverrideSettings));
    SettingsProperty->SetObjectPropertyValue_InContainer(&Mapping,Settings);
    return Mapping.GetMappingName()==Name;
}


int32 UMythicControlKeyProfile::GetPlayerMappedKeysForRebuildControlMappings(const FEnhancedActionKeyMapping& DefaultMapping,TArray<FKey>& OutKeys) const {
    const FName Name=DefaultMapping.GetMappingName();
    if (Name==UMythicControlsSettings::MappingName(EMythicControl::Utility,false)
        || Name==UMythicControlsSettings::MappingName(EMythicControl::Utility,true)) {
        OutKeys.AddUnique(EKeys::Invalid);
        return OutKeys.Num();
    }
    if (Name.ToString().StartsWith(TEXT("Mythic.Controls."))) if (const auto* Row=FindKeyMappingRow(Name)) {
        for (const auto& Mapping:Row->Mappings) if (Mapping.GetSlot()==EPlayerMappableKeySlot::First) {
            // Invalid is an explicit unbind, not an absent row that permits restoration of the default.
            OutKeys.AddUnique(Mapping.GetCurrentKey()); return OutKeys.Num();
        }
    }
    return Super::GetPlayerMappedKeysForRebuildControlMappings(DefaultMapping,OutKeys);
}
void UMythicControlUserSettings::AsyncSaveSettings() {
    if (!GetLocalPlayer()) { bSaveFailed=true; OnSaveState.Broadcast(false,false); return; }
    if (bSaving) { bSaveAgain=true; return; }
    bSaving=true; bSaveFailed=false; OnSaveState.Broadcast(true,false); Super::AsyncSaveSettings();
}
void UMythicControlUserSettings::OnAsyncSaveComplete(const FString&,int32,bool bSuccess) {
    bSaving=false; bSaveFailed=!bSuccess;
    if (bSaveAgain) { bSaveAgain=false; AsyncSaveSettings(); return; }
    OnSaveState.Broadcast(false,bSuccess);
}
