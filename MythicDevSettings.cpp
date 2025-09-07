// 


#include "MythicDevSettings.h"

UMythicDevSettings *UMythicDevSettings::Get() {
    return GetMutableDefault<UMythicDevSettings>();
}

UMythicDevSettings::UMythicDevSettings(const FObjectInitializer &ObjectInitializer) {
    this->CategoryName = TEXT("Project");
    this->SectionName = TEXT("Mythic");
}
