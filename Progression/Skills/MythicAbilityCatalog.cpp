#include "MythicAbilityCatalog.h"
#include "Settings/MythicDeveloperSettings.h"
#include "System/MythicAssetManager.h"
#include "Mythic.h"

void UMythicAbilityCatalog::Initialize(FSubsystemCollectionBase& Collection) {
    Super::Initialize(Collection);
    TArray<FSoftObjectPath> Paths;
    for (const auto& Entry : GetDefault<UMythicDeveloperSettings>()->AbilityCatalog) {
        if (!Entry.IsNull()) { Paths.AddUnique(Entry.ToSoftObjectPath()); ConfiguredPaths.Add(Entry.ToSoftObjectPath()); }
    }
    UMythicAssetManager::LoadAsync(this, Paths, [this](const TArray<UObject*>& Objects) {
#if WITH_DEV_AUTOMATION_TESTS
        if (bTestOverride) return;
#endif
        for (UObject* Object : Objects) {
            UMythicSkillDefinition* Definition = Cast<UMythicSkillDefinition>(Object);
            TArray<FText> Errors;
            if (!Definition || !Definition->ValidateDefinition(Errors)) {
                UE_LOG(Myth, Error, TEXT("Invalid ability catalogue entry: %s"), *GetNameSafe(Object));
                for (const FText& Error : Errors) UE_LOG(Myth, Error, TEXT("%s"), *Error.ToString());
                continue;
            }
            Definitions.Add(Definition);
        }
        TMap<FGameplayTag,int32> CooldownOwners;
        for (const auto& Definition:Definitions) if (Definition->HasTimedCooldown()) ++CooldownOwners.FindOrAdd(Definition->CooldownTag);
        TSet<FGameplayTag> Collisions;
        for (const auto& Pair:CooldownOwners) {
            if (Pair.Value>1) Collisions.Add(Pair.Key);
            for (FGameplayTag Parent=Pair.Key.RequestDirectParent(); Parent.IsValid(); Parent=Parent.RequestDirectParent()) {
                if (CooldownOwners.Contains(Parent)) { Collisions.Add(Parent); Collisions.Add(Pair.Key); }
            }
        }
        Definitions.RemoveAll([&Collisions](const UMythicSkillDefinition* Definition) {
            if (!Definition->HasTimedCooldown() || !Collisions.Contains(Definition->CooldownTag)) return false;
            UE_LOG(Myth,Error,TEXT("Ability catalogue rejected shared cooldown identity %s on %s"),*Definition->CooldownTag.ToString(),*Definition->GetPathName());
            return true;
        });
        Definitions.Sort([](const UMythicSkillDefinition& A, const UMythicSkillDefinition& B) {
            const int32 Compare = A.Name.CompareTo(B.Name);
            return Compare == 0 ? A.GetPathName() < B.GetPathName() : Compare < 0;
        });
        for (int32 i = 0; i < Definitions.Num(); ++i) Index.Add(FSoftObjectPath(Definitions[i]), i);
        bReady = true;
        OnReady.Broadcast();
    });
}

UMythicSkillDefinition* UMythicAbilityCatalog::Find(const FSoftObjectPath& Path) const {
    const int32* Found = Index.Find(Path);
    return Found && Definitions.IsValidIndex(*Found) ? Definitions[*Found] : nullptr;
}

bool UMythicAbilityCatalog::Contains(const UMythicSkillDefinition* Skill) const {
    return Skill && Find(FSoftObjectPath(Skill)) == Skill;
}
