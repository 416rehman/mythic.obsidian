#include "MythicPlayerController.h"

#include "Mythic.h"
#include "MythicPlayerState.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "GameModes/MythicCheatManager.h"
#include "GameModes/GameState/MythicGameState.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Itemization/Crafting/CraftingComponent.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Proficiency/ProficiencyComponent.h"

AMythicPlayerController::AMythicPlayerController() {
    bShowMouseCursor = true;
    DefaultMouseCursor = EMouseCursor::Default;
    bReplicateUsingRegisteredSubObjectList = true;

    // Set the CheatManager class
    CheatClass = UMythicCheatManager::StaticClass();

    // Create the ProficiencyComponent
    ProficiencyComponent = CreateDefaultSubobject<UProficiencyComponent>(TEXT("ProficiencyComponent"));
    ProficiencyComponent->SetIsReplicated(true);

    // Create the Inventory Components
    InventoryComponent = CreateDefaultSubobject<UMythicInventoryComponent>(TEXT("InventoryComponent"));
    InventoryComponent->SetIsReplicated(true);

    // Create the Crafting Component
    CraftingComponent = CreateDefaultSubobject<UCraftingComponent>(TEXT("CraftingComponent"));
    CraftingComponent->SetIsReplicated(true);
}


UAbilitySystemComponent *AMythicPlayerController::GetAbilitySystemComponent() const {
    auto PS = GetPlayerState<AMythicPlayerState>();
    return PS ? PS->GetAbilitySystemComponent() : nullptr;
}

TArray<UMythicInventoryComponent *> AMythicPlayerController::GetAllInventoryComponents() const {
    return {
        InventoryComponent
    };
}

UAbilitySystemComponent *AMythicPlayerController::GetSchematicsASC() const {
    return this->GetAbilitySystemComponent();
}

UMythicInventoryComponent *AMythicPlayerController::GetInventoryForItemType(const FGameplayTag &ItemType) const {
    return IInventoryProviderInterface::GetInventoryForItemType(ItemType);
}

void AMythicPlayerController::OnPossess(APawn *InPawn) {
    Super::OnPossess(InPawn);

    if (this->IsLocalPlayerController()) {
        OnPossessedOnClient();
    }
}

void AMythicPlayerController::OnRep_PlayerState() {
    Super::OnRep_PlayerState();

    // Request Destructibles State
    // if (this->IsLocalPlayerController()) {
    //     Server_RequestDestructiblesState();
    // }

    OnPossessedOnClient();
}

void AMythicPlayerController::BeginPlay() {
    // Call the base class  
    Super::BeginPlay();

    // EOS Login
    if (auto LocalPlayer = this->GetLocalPlayer()) {
        auto LocalIndex = LocalPlayer->GetLocalPlayerIndex();
        this->Login(LocalIndex);
    }
}

void AMythicPlayerController::Login(int32 LocalUserNum) {
    // TODO: Could call an event dispatcher here to notify the UI that connection to Online Services is being established.
    UE_LOG(Myth, Log, TEXT("EOS: Connecting to Online Services"));

    // Get the OSS identity interface
    IOnlineSubsystem *OSS = Online::GetSubsystem(GetWorld());
    IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();

    // If logged in, don't try to login again.
    // This can happen if your player travels to a dedicated server or different maps as BeginPlay() will be called each time.
    // FUniqueNetIdPtr NetId = Identity->GetUniquePlayerId(LocalUserNum);
    // if (NetId != nullptr && Identity->GetLoginStatus(LocalUserNum) == ELoginStatus::LoggedIn) {
    //     return;
    // }

    // EOS stuff now.
    // BIND the CB_LoginResponse to the OnLoginCompleteDelegate on the Identity interface, so when we try logging in we can handle the response.
    LoginDelegateHandle = Identity->
        AddOnLoginCompleteDelegate_Handle(LocalUserNum, FOnLoginCompleteDelegate::CreateUObject(this, &ThisClass::CB_LoginResponse));

    // Print the name of the OSS we are using.
    UE_LOG(Myth, Log, TEXT("EOS: Using Online Subsystem: %s"), *OSS->GetSubsystemName().ToString());

    // Grab command line arguments. Presence of these indicates, using previously saved credentials to auto-login.
    // Even though, this parameter is not used here explicity, the AutoLogin function will use it internally.
    FString AuthType;
    FParse::Value(FCommandLine::Get(), TEXT("AUTH_TYPE="), AuthType);

    // CALL the autologin method if the game was started with AUTH_TYPE and AUTH_TOKEN parameters.
    if (!AuthType.IsEmpty()) {
        // AutoLogin is async, return value indicates if the call was started. Actual result is in the delegate.
        if (!Identity->AutoLogin(LocalUserNum)) {
            // If we failed to start the login call, remove the delegate.
            UE_LOG(Myth, Error, TEXT("EOS: Failed to start AutoLogin"));

            // Tell the identity interface to stop calling our delegate.
            Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, LoginDelegateHandle);

            // Reset the delegate handle.
            LoginDelegateHandle.Reset();
        }
    }
    // CALL the login method to initiate the login process if the game was not started with AUTH_TYPE and AUTH_TOKEN parameters.
    else {
        /* 
        Fallback if the CLI parameters are empty.Useful for PIE.
        The type here could be developer if using the DevAuthTool, ExchangeCode if the game is launched via the Epic Games Launcher, etc...
        */

        // Type, Id, Token
        // "AccountPortal" will use the built-in onboarding flow provided by the SDK.
        FOnlineAccountCredentials Credentials("AccountPortal", "", "");

        UE_LOG(Myth, Log, TEXT("EOS: Logging in to Online service"));

        // Login is async, return value indicates if the call was started. Actual result is in the delegate.
        if (!Identity->Login(LocalUserNum, Credentials)) {
            // If we failed to start the login call, remove the delegate.
            UE_LOG(Myth, Error, TEXT("EOS: Failed to start Login"));

            // Tell the identity interface to stop calling our delegate.
            Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, LoginDelegateHandle);

            // Reset the delegate handle.
            LoginDelegateHandle.Reset();
        }
    }
}

void AMythicPlayerController::CB_LoginResponse(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId &UserId, const FString &Error) {
    /*
This function handles the callback from logging in. You should not proceed with any EOS features until this function is called.
This function will remove the delegate that was bound in the Login() function.
*/

    // TODO: Could call an event dispatcher here to notify the UI that connection to Online Services failed or succeeded.
    if (bWasSuccessful) {
        UE_LOG(Myth, Log, TEXT("EOS: Login successful - %s"), *UserId.ToString());
    }
    else {
        // If online only, do not allow the player to continue.
        // Otherwise, you can display a message to the user and allow them to continue in offline mode.
        UE_LOG(Myth, Error, TEXT("EOS: Login failed: %s"), *Error);
    }

    // Clear the delegate handle.
    IOnlineSubsystem *OSS = Online::GetSubsystem(GetWorld());
    IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();

    Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, LoginDelegateHandle);
    LoginDelegateHandle.Reset();
}

const UProficiencyComponent *AMythicPlayerController::GetProficiencyComponent() const {
    return ProficiencyComponent;
}

void AMythicPlayerController::SetupInputComponent() {
    // set up gameplay key bindings
    Super::SetupInputComponent();
}
