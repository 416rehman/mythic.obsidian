#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GameFramework/PlayerState.h"
#include "Itemization/InventoryProviderInterface.h"
#include "MythicPlayerState.generated.h"

class UMythicAttributeSet_Life;
class UMythicAttributeSet_Offense;
class UMythicAttributeSet_Proficiencies;

class UMythicAttributeSet_Utility;
class UMythicAttributeSet_Defense;
class UMythicFactionStandingComponent;
/**
 * 
 */
UCLASS()
class MYTHIC_API AMythicPlayerState : public APlayerState, public IAbilitySystemInterface, public IInventoryProviderInterface {
    GENERATED_BODY()

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Ability System")
    TObjectPtr<UMythicAbilitySystemComponent> MythicAbilitySystemComponent;

    // Default Ability Set
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
    TArray<TSubclassOf<UMythicGameplayAbility>> DefaultAbilities;

    // Default Gameplay Effects - Can also be used to initialize attributes.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
    TArray<TSubclassOf<UGameplayEffect>> DefaultGameplayEffects;

    // Life attributes
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Ability System", Replicated)
    UMythicAttributeSet_Life *LifeAttributes;

    // Offense attributes
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Ability System", Replicated)
    UMythicAttributeSet_Offense *OffenseAttributes;

    // Defense attributes
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Ability System", Replicated)
    UMythicAttributeSet_Defense *DefenseAttributes;

    // Utility attributes
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Ability System", Replicated)
    UMythicAttributeSet_Utility *UtilityAttributes;



    // Proficiency attributes
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Ability System", Replicated)
    UMythicAttributeSet_Proficiencies *ProficiencyAttributes;

    // Per-player faction standing (replicated component; drives NPC attitude toward this player).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Faction")
    TObjectPtr<UMythicFactionStandingComponent> FactionStanding;

    // The canonical PERSISTENT player identity = the save-slot CharacterID this player's character was loaded from
    // (a stable GUID-string across sessions). Empty until a character is loaded onto this player. Server sets it from
    // the save-load path; replicated so clients can display/attribute by a stable key. This is the cross-session key
    // the party/companion system and the player registry resolve against (vs the session-transient GetPlayerId()).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Identity")
    FString PersistentCharacterId;

    AMythicPlayerState();

public:
    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;
    UMythicAbilitySystemComponent *GetMythicAbilitySystemComponent() const;

    // Per-player faction standing store (server-authoritative; replicated to owner).
    UFUNCTION(BlueprintPure, Category = "Faction")
    UMythicFactionStandingComponent *GetFactionStanding() const { return FactionStanding; }

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // SERVER: record the persistent save-slot CharacterID this player's character was loaded from. Called from the
    // save-load path. No-op off authority. Re-registers this player in the registry under the (now persistent) key.
    void SetPersistentCharacterId(const FString &InCharacterId);

    UFUNCTION(BlueprintPure, Category = "Identity")
    const FString &GetPersistentCharacterId() const { return PersistentCharacterId; }

    // The canonical key this player is addressed by across the party/companion + registry systems: the persistent
    // CharacterID when one has been loaded, else a session-stable fallback. Built from this player's live state.
    UFUNCTION(BlueprintPure, Category = "Identity")
    FString GetCanonicalPlayerKey() const;

    // Pure key-resolution rule (the single source for GetCanonicalPlayerKey): the persistent id wins when non-empty
    // (stable across sessions); otherwise a session-stable "session:<id>" fallback so an unsaved/transient player
    // still has a usable, unique-within-session key. Static + no engine state so the rule is unit-testable.
    static FString ResolveCanonicalPlayerKey(const FString &PersistentId, int32 SessionPlayerId);

    // IInventoryProviderInterface - delegates to PlayerController
    virtual TArray<UMythicInventoryComponent *> GetAllInventoryComponents() const override;
    virtual UAbilitySystemComponent *GetSchematicsASC() const override;
    virtual UMythicInventoryComponent *GetInventoryForItemType(const FGameplayTag &ItemType) const override;
};
