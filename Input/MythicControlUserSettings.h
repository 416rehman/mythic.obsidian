#pragma once
#include "CoreMinimal.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "MythicControlUserSettings.generated.h"

namespace MythicControlMapping { MYTHIC_API bool Configure(FEnhancedActionKeyMapping& Mapping,UObject* Owner,FName Name,const FText& Label); }

UCLASS()
class MYTHIC_API UMythicControlKeyProfile : public UEnhancedPlayerMappableKeyProfile {
    GENERATED_BODY()
public:
    virtual int32 GetPlayerMappedKeysForRebuildControlMappings(const FEnhancedActionKeyMapping& DefaultMapping,TArray<FKey>& OutKeys) const override;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FMythicControlSaveState,bool,bool);
/** Serializes asynchronous writes; a pending older save cannot overwrite a newer binding. */
UCLASS()
class MYTHIC_API UMythicControlUserSettings : public UEnhancedInputUserSettings {
    GENERATED_BODY()
public:
    virtual void AsyncSaveSettings() override;
    bool IsSaving() const { return bSaving; }
    bool DidSaveFail() const { return bSaveFailed; }
    FMythicControlSaveState OnSaveState;
private:
    virtual void OnAsyncSaveComplete(const FString& SlotName,int32 UserIndex,bool bSuccess) override;
    bool bSaving=false;
    bool bSaveAgain=false;
    bool bSaveFailed=false;
};
