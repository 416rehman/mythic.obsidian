// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "CommonPlayerController.h"
#include "Mythic.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "MythicPlayerController.generated.h"

struct FTrackedDestructibleData;
class UMythicItemInstance;
class UItemDefinition;

UCLASS()
class AMythicPlayerController : public ACommonPlayerController, public IAbilitySystemInterface {
    GENERATED_BODY()

protected:
    // Proficiency Component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Proficiency")
    class UProficiencyComponent *ProficiencyComponent;

    // Inventory Component - Backpack
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    class UMythicInventoryComponent *Backpack;

    // Inventory Component - Hotbar
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    class UMythicInventoryComponent *Hotbar;

public:
    AMythicPlayerController();

    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;
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

    UFUNCTION(BlueprintCallable, Category = "Proficiency")
    const UMythicInventoryComponent *GetBackpack() const;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    const UMythicInventoryComponent *GetHotbar() const;

    // Event - Called when the player crafts an item
    UFUNCTION(BlueprintImplementableEvent, Category = "Crafting")
    void OnCraftItem(UItemDefinition *ItemDef, int32 Quantity);

    void OnCraftItem_Implementation(UItemDefinition *ItemDef, int32 Quantity) {
        UE_LOG(Mythic, Log, TEXT("Crafting: %s"), *ItemDef->Name.ToString());
    }

    // CraftingStation should be able to access Hotbar and Backpack
    friend class ACraftingStation;

    /// Resource Sync with Server
    // UFUNCTION(BlueprintCallable, Category = "Resource Sync", Server, Reliable)
    // void Server_RequestDestructiblesState();
    //
    // // Sync resources to owner
    // UFUNCTION(BlueprintCallable, Category = "Resource Sync", Client, Reliable)
    // void Client_RecvDestructiblesState(const TArray<FTrackedDestructibleData> &DestructibleData);
};
