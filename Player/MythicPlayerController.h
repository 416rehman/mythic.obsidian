// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "CommonPlayerController.h"
#include "Itemization/InventoryProviderInterface.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "MythicPlayerController.generated.h"

struct FTrackedDestructibleData;
class UMythicItemInstance;
class UItemDefinition;

class UMythicCheatManager;

UCLASS(Config=Game)
class AMythicPlayerController : public ACommonPlayerController, public IAbilitySystemInterface, public IInventoryProviderInterface {
    GENERATED_BODY()

protected:
    // Proficiency Component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Proficiency")
    class UProficiencyComponent *ProficiencyComponent;

    // Inventory Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    class UMythicInventoryComponent *InventoryComponent;

    // Crafting Component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crafting")
    class UCraftingComponent *CraftingComponent;

public:
    AMythicPlayerController();

    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    virtual TArray<UMythicInventoryComponent *> GetAllInventoryComponents() const override;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    virtual UAbilitySystemComponent *GetSchematicsASC() const override;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    virtual UMythicInventoryComponent *GetInventoryForItemType(const FGameplayTag &ItemType) const override;

    virtual void OnPossess(APawn *InPawn) override;
    virtual void OnRep_PlayerState() override;

    // Client-side event when the player is possessed
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic")
    void OnPossessedOnClient();

protected:
    virtual void SetupInputComponent() override;
    virtual void BeginPlay() override;

    // Login with the saved credentials if passed in via command line arguments. Otherwise, start the login process.
    // I.e Before this function is called, you could display a "Logging in..." message to the user.
    void Login(int32 LocalUserNum);

    // Handle Login Response. This is called when the login process is complete.
    void CB_LoginResponse(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId &UserId, const FString &Error);

    // Delegate for handling the login response
    FDelegateHandle LoginDelegateHandle;

public:
    UFUNCTION(BlueprintCallable, Category = "Proficiency")
    const UProficiencyComponent *GetProficiencyComponent() const;
};
