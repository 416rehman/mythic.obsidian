#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MythicSkillDefinition.h"
#include "MythicAbilityCatalog.generated.h"

DECLARE_MULTICAST_DELEGATE(FMythicAbilityCatalogReady);

UCLASS()
class MYTHIC_API UMythicAbilityCatalog : public UGameInstanceSubsystem {
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    bool IsReady() const { return bReady; }
    bool Contains(const UMythicSkillDefinition* Skill) const;
    /** Configuration membership is available before async validation, solely for durable reward intake. */
    bool IsConfigured(const UMythicSkillDefinition* Skill) const { return Skill && ConfiguredPaths.Contains(FSoftObjectPath(Skill)); }
    UMythicSkillDefinition* Find(const FSoftObjectPath& Path) const;
    const TArray<TObjectPtr<UMythicSkillDefinition>>& GetDefinitions() const { return Definitions; }
    FMythicAbilityCatalogReady OnReady;
private:
    friend struct FMythicAbilityCatalogTestAccess;
#if WITH_DEV_AUTOMATION_TESTS
    bool bTestOverride = false;
#endif
    UPROPERTY() TArray<TObjectPtr<UMythicSkillDefinition>> Definitions;
    TMap<FSoftObjectPath, int32> Index;
    TSet<FSoftObjectPath> ConfiguredPaths;
    bool bReady = false;
};
