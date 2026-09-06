#pragma once
#if WITH_DEV_AUTOMATION_TESTS
#include "Progression/Skills/MythicAbilityCatalog.h"
#include "Progression/Skills/MythicSkillComponent.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "Player/MythicPlayerState.h"
#include "Player/MythicCharacter_Player.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "World/Entity/MythicEntityId.h"

/** Test worlds own their catalogue. No transient-asset exception exists in the shipping grant path. */
struct FMythicAbilityCatalogTestAccess {
    static void Reset(UMythicAbilityCatalog* Catalog) {
        check(Catalog); Catalog->bTestOverride=true; Catalog->Definitions.Reset(); Catalog->Index.Reset(); Catalog->ConfiguredPaths.Reset(); Catalog->bReady=true;
    }
    static void Add(UMythicAbilityCatalog* Catalog,UMythicSkillDefinition* Definition) {
        check(Catalog && Definition);
        Catalog->ConfiguredPaths.Add(FSoftObjectPath(Definition));
        if (!Catalog->Index.Contains(FSoftObjectPath(Definition))) {
            Catalog->Index.Add(FSoftObjectPath(Definition),Catalog->Definitions.Num()); Catalog->Definitions.Add(Definition);
        }
    }
    static void BeginPending(UMythicAbilityCatalog* Catalog,const TArray<UMythicSkillDefinition*>& Configured) {
        Reset(Catalog); Catalog->bReady=false;
        for (const auto* Definition:Configured) Catalog->ConfiguredPaths.Add(FSoftObjectPath(Definition));
    }
    static void Complete(UMythicAbilityCatalog* Catalog,const TArray<UMythicSkillDefinition*>& Validated) {
        for (auto* Definition:Validated) Add(Catalog,Definition);
        Catalog->bReady=true; Catalog->OnReady.Broadcast();
    }
};
inline void InitializeAbilityTestPlayer(AMythicPlayerState* Player,bool bResetCatalogue=true) {
    Player->AuthoritySetPersistentEntityId(FMythicEntityId::FromAuthorityGuid(EMythicEntityDomain::PlayerCharacter,FGuid::NewGuid()));
    Player->SetPersistentCharacterId(FGuid::NewGuid().ToString(EGuidFormats::Digits));
    auto* ASC=Player->GetMythicAbilitySystemComponent();
    if (!ASC->IsRegistered()) ASC->RegisterComponent();
    TArray<UObject*> Subobjects;
    Player->GetDefaultSubobjects(Subobjects);
    for (UObject* Subobject:Subobjects) if (auto* Set=Cast<UAttributeSet>(Subobject))
        if (!ASC->GetSpawnedAttributes().Contains(Set)) ASC->AddAttributeSetSubobject(Set);
    ASC->InitAbilityActorInfo(Player,Player);
    auto* Skills=Player->GetSkillComponent();
    if (bResetCatalogue) FMythicAbilityCatalogTestAccess::Reset(Skills->GetCatalog());
    Skills->RestoreSkills({},1); Skills->SetRestoring(false);
}
inline void RegisterAbilityTestDefinition(UMythicSkillComponent* Skills,UMythicSkillDefinition* Definition,bool bLearn=true,bool bLegacyProgression=true) {
    // Existing point/capacity regressions explicitly exercise the legacy compatibility contract.
    Definition->bUseLegacyModifierProgression=bLegacyProgression;
    for (int32 i=0;i<Definition->Modifiers.Num();++i) if (Definition->Modifiers[i].ModifierId.IsNone())
        Definition->Modifiers[i].ModifierId=FName(*FString::Printf(TEXT("Modifier_%d"),i));
    FMythicAbilityCatalogTestAccess::Add(Skills->GetCatalog(),Definition);
    Skills->ReconcilePrerequisites();
    if (bLearn) Skills->LearnSkill(Definition);
    Skills->SetRestoring(false);
}

/** Real avatar/life/presentation setup for tests that commit gameplay, beyond the ownership-only fixture. */
inline AMythicCharacter_Player* InitializeAbilityTestAvatar(AMythicPlayerState* Player) {
    auto* World=Player->GetWorld();
    // Route the production component/cache lifecycle in the fixture's own world.
    if (!World->AreActorsInitialized()) World->InitializeActorsForPlay(FURL());
    auto* Controller=World->SpawnActor<APlayerController>();
    if (!Controller) return nullptr;
    // Standalone GAS still requires a real local player for LocalPredicted abilities.
    Controller->SetPlayer(NewObject<ULocalPlayer>(GEngine));
    Controller->PlayerState=Player; Player->SetOwner(Controller);
    auto* Avatar=World->SpawnActor<AMythicCharacter_Player>();
    if (!Avatar) return nullptr;
    Controller->Possess(Avatar); // Production PossessedBy initializes ASC, native life and persistent presentation.
    return Avatar;
}
#endif
