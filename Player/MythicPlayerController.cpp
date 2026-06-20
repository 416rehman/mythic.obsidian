#include "MythicPlayerController.h"

#include "Mythic.h"
#include "MythicPlayerState.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "GameModes/MythicCheatManager.h"
#include "GameModes/GameState/MythicGameState.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Itemization/Crafting/CraftingComponent.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Conversion/MythicConversionStation.h"
#include "Itemization/Conversion/ConversionStationComponent.h"
#include "Itemization/Storage/MythicStorageContainer.h"
#include "Proficiency/ProficiencyComponent.h"
#include "Objectives/ObjectiveTracker.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "AI/Cognition/CognitiveBrainComponent.h" // NPC->CognitiveBrain->GetSourceEntity() for recruit
#include "AI/Party/PartySubsystem.h" // server-authoritative party recruit
#include "UI/MythicDamageNumberSubsystem.h" // recruit-result callout
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/Fragments/Passive/AffixesFragment.h"
#include "Itemization/Loot/MythicLootManagerSubsystem.h"
#include "World/EnvironmentController/MythicEnvironmentHazardComponent.h"
#include "World/LivingWorld/Chronicle/MythicChronicleRelayComponent.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"   // zone-entry: settlement-at-cell snapshot
#include "World/LivingWorld/Territory/TerritoryGrid.h" // WorldToCell
#include "World/LivingWorld/Settlements/MythicSettlement.h" // FMythicSettlementData (DisplayName + SettlementId)

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

    // Create the Objective/Quest Tracker (replicates owner-only; binds to GAS kill events server-side).
    ObjectiveTracker = CreateDefaultSubobject<UObjectiveTracker>(TEXT("ObjectiveTracker"));
    ObjectiveTracker->SetIsReplicated(true);

    // Environmental hazard component (server-side; applies weather/season/time GEs to the player's ASC).
    EnvironmentHazard = CreateDefaultSubobject<UMythicEnvironmentHazardComponent>(TEXT("EnvironmentHazard"));

    // World Chronicle relay (replicates the server-built world-news feed to the owning client; owner-only).
    ChronicleRelay = CreateDefaultSubobject<UMythicChronicleRelayComponent>(TEXT("ChronicleRelay"));
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

    // Server-side zone-entry poll: fire "Welcome to <settlement>" when the player crosses into a new settlement. A
    // position-based check has no delegate to bind, so a light repeating timer is the right mechanism (mirrors the
    // LifeComponent regen timer). The pawn may not be possessed yet — CheckZoneEntry no-ops until it is.
    if (HasAuthority() && GetWorld() && ZoneCheckInterval > 0.0f) {
        GetWorld()->GetTimerManager().SetTimer(ZoneCheckTimerHandle, this, &AMythicPlayerController::CheckZoneEntry,
                                               ZoneCheckInterval, true);
    }

    // EOS Login
    if (auto LocalPlayer = this->GetLocalPlayer()) {
        auto LocalIndex = LocalPlayer->GetLocalPlayerIndex();
        this->Login(LocalIndex);
    }
}

void AMythicPlayerController::Login(int32 LocalUserNum) {
    // TODO: Could call an event dispatcher here to notify the UI that connection to Online Services is being established.
    UE_LOG(Myth, Log, TEXT("EOS: Connecting to Online Services"));

    // Get the OSS identity interface. Online::GetSubsystem returns null when no OSS is configured (a PIE/cooked config
    // without the EOS module, or offline mode), and GetIdentityInterface() can return an invalid ptr — either is an
    // immediate crash on the unguarded deref. Guard them, matching this file's existing defensive-null-check convention.
    IOnlineSubsystem *OSS = Online::GetSubsystem(GetWorld());
    if (!OSS) {
        UE_LOG(Myth, Error, TEXT("EOS: No Online Subsystem available — skipping login"));
        return;
    }
    IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
    if (!Identity.IsValid()) {
        UE_LOG(Myth, Error, TEXT("EOS: Identity interface unavailable — skipping login"));
        return;
    }

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

    // Clear the delegate handle. Guard the subsystem/identity derefs (see Login) — a null OSS / invalid Identity here
    // would crash the login-completion callback in a no-OSS/offline configuration.
    IOnlineSubsystem *OSS = Online::GetSubsystem(GetWorld());
    if (!OSS) {
        UE_LOG(Myth, Error, TEXT("EOS: No Online Subsystem available — cannot clear login delegate"));
        return;
    }
    IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
    if (!Identity.IsValid()) {
        UE_LOG(Myth, Error, TEXT("EOS: Identity interface unavailable — cannot clear login delegate"));
        return;
    }

    Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, LoginDelegateHandle);
    LoginDelegateHandle.Reset();
}

const UProficiencyComponent *AMythicPlayerController::GetProficiencyComponent() const {
    return ProficiencyComponent;
}

// ---- Conversion station RPCs ----

bool AMythicPlayerController::ServerOpenConversionStation_Validate(AMythicConversionStation *Station) { return Station != nullptr; }

void AMythicPlayerController::ServerOpenConversionStation_Implementation(AMythicConversionStation *Station) {
    if (!HasAuthority() || !Station) {
        return;
    }
    if (UConversionStationComponent *Comp = Station->GetConversionComponent()) {
        Comp->Server_RegisterInstigator(this, GetPawn());
    }
}

bool AMythicPlayerController::ServerConversionRequestStart_Validate(AMythicConversionStation *Station, FGameplayTag RecipeId, int32 Quantity) {
    return Station != nullptr && RecipeId.IsValid() && Quantity >= 1 && Quantity <= 999;
}

void AMythicPlayerController::ServerConversionRequestStart_Implementation(AMythicConversionStation *Station, FGameplayTag RecipeId, int32 Quantity) {
    if (!HasAuthority() || !Station) {
        return;
    }
    if (UConversionStationComponent *Comp = Station->GetConversionComponent()) {
        Comp->Server_RequestStart(this, RecipeId, Quantity);
    }
}

bool AMythicPlayerController::ServerConversionCancelJob_Validate(AMythicConversionStation *Station, int32 JobId) {
    return Station != nullptr && JobId > 0;
}

void AMythicPlayerController::ServerConversionCancelJob_Implementation(AMythicConversionStation *Station, int32 JobId) {
    if (!HasAuthority() || !Station) {
        return;
    }
    if (UConversionStationComponent *Comp = Station->GetConversionComponent()) {
        Comp->Server_CancelJob(this, JobId);
    }
}

bool AMythicPlayerController::ServerConversionSetAutoRepeat_Validate(AMythicConversionStation *Station, bool bRepeat) { return Station != nullptr; }

void AMythicPlayerController::ServerConversionSetAutoRepeat_Implementation(AMythicConversionStation *Station, bool bRepeat) {
    if (!HasAuthority() || !Station) {
        return;
    }
    if (UConversionStationComponent *Comp = Station->GetConversionComponent()) {
        Comp->Server_SetAutoRepeat(this, bRepeat);
    }
}

// ---- Storage container RPCs ----

bool AMythicPlayerController::ServerOpenStorageContainer_Validate(AMythicStorageContainer *Container) { return Container != nullptr; }

void AMythicPlayerController::ServerOpenStorageContainer_Implementation(AMythicStorageContainer *Container) {
    if (!HasAuthority() || !Container) {
        return;
    }
    if (!Container->IsActorInRange(GetPawn())) {
        return;
    }
    Container->Server_AddOpener(this);
}

bool AMythicPlayerController::ServerMoveItemBetweenInventories_Validate(UMythicInventoryComponent *Source, int32 SourceSlot, UMythicInventoryComponent *Target,
                                                                        int32 TargetSlot) {
    // Source must be a concrete slot; Target may be INDEX_NONE (any-slot).
    return Source != nullptr && Target != nullptr && SourceSlot >= 0 && TargetSlot >= -1;
}

void AMythicPlayerController::ServerMoveItemBetweenInventories_Implementation(UMythicInventoryComponent *Source, int32 SourceSlot,
                                                                              UMythicInventoryComponent *Target, int32 TargetSlot) {
    if (!HasAuthority() || !Source || !Target) {
        return;
    }

    // The player may act on an inventory only if it is one of their own OR it belongs to a container they
    // currently have open AND are in range of (prevents touching a third party's items or acting out of range).
    const TArray<UMythicInventoryComponent *> OwnInventories = GetAllInventoryComponents();
    auto IsAllowed = [&](UMythicInventoryComponent *Inv) -> bool {
        if (!Inv) {
            return false;
        }
        if (OwnInventories.Contains(Inv)) {
            return true;
        }
        if (AMythicStorageContainer *Container = Cast<AMythicStorageContainer>(Inv->GetOwner())) {
            return Container->Server_IsOpener(this) && Container->IsActorInRange(GetPawn());
        }
        return false;
    };

    if (!IsAllowed(Source) || !IsAllowed(Target)) {
        return;
    }

    // Player-initiated take must respect the source slot's bCanPlayerTake (SendItem only checks the target's
    // bCanPlayerPut).
    if (!Source->CanPlayerTakeFromSlot(SourceSlot)) {
        return;
    }

    Source->SendItem(SourceSlot, Target, TargetSlot);
}

bool AMythicPlayerController::ServerRequestNpcDialogue_Validate(AMythicNPCCharacter *NPC) {
    return NPC != nullptr;
}

void AMythicPlayerController::OfferNpcQuestIfAny(AMythicNPCCharacter *NPC) {
    // Quest-giver: if this NPC offers a quest, assign it server-side (ServerAddObjective is idempotent, so re-interacting
    // is harmless). SERVER-VALIDATE proximity (the interaction scanner only enforces range client-side) so a modded
    // client can't bulk-enroll across the map. SINGLE SOURCE (Rule 3): called from BOTH the dialogue AND recruit paths
    // — a recruitable quest-giver must still hand out its quest (the recruit verb otherwise bypasses this).
    if (!NPC) {
        return;
    }
    if (UObjectiveDefinition *Offer = NPC->GetQuestOffer()) {
        if (ObjectiveTracker && NPC->IsActorInTradeRange(GetPawn())) {
            ObjectiveTracker->ServerAddObjective(Offer);
        }
    }
}

void AMythicPlayerController::ServerRequestNpcDialogue_Implementation(AMythicNPCCharacter *NPC) {
    if (!HasAuthority() || !IsValid(NPC)) {
        return;
    }

    OfferNpcQuestIfAny(NPC);

    // Pick the line on the server, where the NPC's brain dialogue context (Faction/Role/pressure) is real, then
    // deliver it back to this requesting client for display.
    const FText Line = NPC->SelectDialogueFor(this);
    ClientReceiveNpcDialogue(NPC, Line);
}

void AMythicPlayerController::ClientReceiveNpcDialogue_Implementation(AMythicNPCCharacter *NPC, const FText &Line) {
    if (IsValid(NPC)) {
        NPC->FireBark(Line, this);
    }
}

bool AMythicPlayerController::ServerRecruitNpc_Validate(AMythicNPCCharacter *NPC) {
    return NPC != nullptr;
}

void AMythicPlayerController::ServerRecruitNpc_Implementation(AMythicNPCCharacter *NPC) {
    if (!HasAuthority() || !IsValid(NPC)) {
        return;
    }
    // SERVER-VALIDATE proximity + eligibility — the interaction scanner only enforces range client-side, so a modded
    // client could otherwise recruit across the map. Same single-source range predicate the dialogue/barter RPCs use.
    if (!NPC->IsActorInTradeRange(GetPawn()) || !NPC->IsRecruitable()) {
        return;
    }
    UMythicPartySubsystem *Party = GetWorld() ? GetWorld()->GetSubsystem<UMythicPartySubsystem>() : nullptr;
    if (!Party) {
        return;
    }
    // Parity with the dialogue path: a recruitable quest-giver still hands out its quest (the recruit verb otherwise
    // bypasses ServerRequestNpcDialogue's enrollment). Idempotent, so offering on every recruit interaction is safe.
    OfferNpcQuestIfAny(NPC);
    // Already a companion → the primary verb falls through to a normal dialogue bark (so you can talk to a member).
    if (Party->IsInParty(NPC)) {
        ClientReceiveNpcDialogue(NPC, NPC->SelectDialogueFor(this));
        return;
    }
    // The party/loyalty model tracks companions by their MASS source entity, so only a Tier2+ cognitive NPC that owns
    // one can be recruited. A recruitable NPC with no brain/entity is a designer misconfiguration — log it (rather than
    // show the player a misleading "party full") and bail.
    if (!NPC->CognitiveBrain) {
        UE_LOG(Myth, Warning, TEXT("ServerRecruitNpc: '%s' is recruitable but has no cognitive brain — cannot be a companion."), *GetNameSafe(NPC));
        return;
    }
    const FMassEntityHandle Src = NPC->CognitiveBrain->GetSourceEntity();
    if (!Src.IsValid()) {
        UE_LOG(Myth, Warning, TEXT("ServerRecruitNpc: '%s' has no valid MASS source entity — cannot be a companion."), *GetNameSafe(NPC));
        return;
    }
    // bOk false here means ONLY "party full" (the dup/null cases are handled above), so the client message is accurate.
    const bool bOk = Party->AddCompanion(/*PlayerId*/ 0, NPC, Src);
    ClientReceiveRecruitResult(NPC, bOk);
}

void AMythicPlayerController::ClientReceiveRecruitResult_Implementation(AMythicNPCCharacter *NPC, bool bSucceeded) {
    if (!IsValid(NPC)) {
        return;
    }
    UWorld *World = NPC->GetWorld();
    if (!World) {
        return;
    }
    if (UMythicDamageNumberSubsystem *DamageNumbers = World->GetSubsystem<UMythicDamageNumberSubsystem>()) {
        const FVector Location = NPC->GetActorLocation() + FVector(0.0f, 0.0f, 110.0f);
        if (bSucceeded) {
            DamageNumbers->AddDamageNumberCustom(Location, TEXT("Joined your party!"), FLinearColor(0.1f, 0.9f, 0.3f), 3.0f);
        }
        else {
            DamageNumbers->AddDamageNumberCustom(Location, TEXT("Party is full"), FLinearColor(0.8f, 0.8f, 0.8f), 3.0f);
        }
    }
}

void AMythicPlayerController::ClientShowGatherProgress_Implementation(FVector Location, int32 HitsRemaining) {
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    if (UMythicDamageNumberSubsystem *DamageNumbers = World->GetSubsystem<UMythicDamageNumberSubsystem>()) {
        const FVector Loc = Location + FVector(0.0f, 0.0f, 50.0f); // float just above the node
        DamageNumbers->AddDamageNumberCustom(Loc, FString::Printf(TEXT("%d left"), HitsRemaining),
                                             FLinearColor(0.85f, 0.7f, 0.4f), 1.0f); // tan, brief
    }
}

void AMythicPlayerController::ClientShowGatherDepleted_Implementation(FVector Location) {
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    if (UMythicDamageNumberSubsystem *DamageNumbers = World->GetSubsystem<UMythicDamageNumberSubsystem>()) {
        const FVector Loc = Location + FVector(0.0f, 0.0f, 50.0f);
        DamageNumbers->AddDamageNumberCustom(Loc, TEXT("Depleted!"), FLinearColor(0.6f, 0.85f, 0.4f), 1.5f);
    }
}

void AMythicPlayerController::ClientNotifyProficiencyLevel_Implementation(const FText &ProfName, int32 NewLevel, const FText &MilestoneName) {
    const APawn *AvatarPawn = GetPawn();
    if (!AvatarPawn) {
        return;
    }
    UWorld *World = AvatarPawn->GetWorld();
    if (!World) {
        return;
    }
    if (UMythicDamageNumberSubsystem *DamageNumbers = World->GetSubsystem<UMythicDamageNumberSubsystem>()) {
        const FVector Location = AvatarPawn->GetActorLocation() + FVector(0.0f, 0.0f, 100.0f);
        DamageNumbers->AddDamageNumberCustom(Location, FString::Printf(TEXT("%s  Lv %d"), *ProfName.ToString(), NewLevel),
                                             FLinearColor(1.0f, 0.85f, 0.1f), 2.0f); // gold
        if (!MilestoneName.IsEmpty()) {
            DamageNumbers->AddDamageNumberCustom(Location + FVector(0.0f, 0.0f, 45.0f),
                                                 FString::Printf(TEXT("%s unlocked!"), *MilestoneName.ToString()),
                                                 FLinearColor(1.0f, 0.55f, 0.9f), 2.5f); // pink milestone
        }
    }
}

void AMythicPlayerController::ClientNotifyCompanionDeparted_Implementation(const FText &Name, FVector Location) {
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    if (UMythicDamageNumberSubsystem *DamageNumbers = World->GetSubsystem<UMythicDamageNumberSubsystem>()) {
        const FString Who = Name.IsEmpty() ? TEXT("A companion") : Name.ToString();
        DamageNumbers->AddDamageNumberCustom(Location + FVector(0.0f, 0.0f, 110.0f),
                                             FString::Printf(TEXT("%s has left your party"), *Who),
                                             FLinearColor(0.7f, 0.7f, 0.75f), 3.0f); // grey, lingers
    }
}

void AMythicPlayerController::ClientNotifyCompanionBetrayed_Implementation(const FText &Name, FVector Location) {
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    if (UMythicDamageNumberSubsystem *DamageNumbers = World->GetSubsystem<UMythicDamageNumberSubsystem>()) {
        const FString Who = Name.IsEmpty() ? TEXT("A companion") : Name.ToString();
        DamageNumbers->AddDamageNumberCustom(Location + FVector(0.0f, 0.0f, 110.0f),
                                             FString::Printf(TEXT("%s turns on you!"), *Who),
                                             FLinearColor(0.9f, 0.1f, 0.1f), 3.5f); // red, lingers
    }
}

void AMythicPlayerController::ClientNotifyObjective_Implementation(const FText &DisplayText, int32 Current, int32 Required, bool bCompleted, int32 StackIndex) {
    const APawn *AvatarPawn = GetPawn();
    if (!AvatarPawn) {
        return;
    }
    UWorld *World = AvatarPawn->GetWorld();
    if (!World) {
        return;
    }
    if (UMythicDamageNumberSubsystem *DamageNumbers = World->GetSubsystem<UMythicDamageNumberSubsystem>()) {
        // Stack offset so 2+ same-event objectives don't overlap (mirrors the proficiency milestone +45 idiom).
        const FVector Location = AvatarPawn->GetActorLocation() + FVector(0.0f, 0.0f, 120.0f + StackIndex * 45.0f);
        if (bCompleted) {
            DamageNumbers->AddDamageNumberCustom(Location, FString::Printf(TEXT("Objective Complete: %s"), *DisplayText.ToString()),
                                                 FLinearColor(1.0f, 0.85f, 0.1f), 3.0f); // gold, lingers
        }
        else {
            DamageNumbers->AddDamageNumberCustom(Location, FString::Printf(TEXT("%s  %d/%d"), *DisplayText.ToString(), Current, Required),
                                                 FLinearColor(0.9f, 0.9f, 0.95f), 1.5f); // white, brief
        }
    }
}

void AMythicPlayerController::ClientNotifyLootPickup_Implementation(const FText &ItemName, int32 Quantity, FLinearColor RarityColor) {
    const APawn *AvatarPawn = GetPawn();
    if (!AvatarPawn) {
        return;
    }
    UWorld *World = AvatarPawn->GetWorld();
    if (!World) {
        return;
    }
    if (UMythicDamageNumberSubsystem *DamageNumbers = World->GetSubsystem<UMythicDamageNumberSubsystem>()) {
        const FVector Location = AvatarPawn->GetActorLocation() + FVector(0.0f, 0.0f, 100.0f); // float over the player's head
        const FString Text = (Quantity > 1)
            ? FString::Printf(TEXT("+%d %s"), Quantity, *ItemName.ToString())
            : ItemName.ToString();
        DamageNumbers->AddDamageNumberCustom(Location, Text, RarityColor, 1.5f);
    }
}

void AMythicPlayerController::ClientShowShieldAbsorbed_Implementation(int32 Absorbed, bool bBroke) {
    const APawn *AvatarPawn = GetPawn();
    if (!AvatarPawn) {
        return;
    }
    UWorld *World = AvatarPawn->GetWorld();
    if (!World) {
        return;
    }
    if (UMythicDamageNumberSubsystem *DamageNumbers = World->GetSubsystem<UMythicDamageNumberSubsystem>()) {
        const FVector Location = AvatarPawn->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
        DamageNumbers->AddDamageNumberCustom(Location, FString::Printf(TEXT("%d"), Absorbed),
                                             FLinearColor(0.4f, 0.7f, 1.0f), 1.0f); // light blue = absorbed by shield
        if (bBroke) {
            DamageNumbers->AddDamageNumberCustom(Location + FVector(0.0f, 0.0f, 40.0f), TEXT("Shield Broken!"),
                                                 FLinearColor(0.6f, 0.9f, 1.0f), 1.5f); // bright cyan, the dramatic beat
        }
    }
}

void AMythicPlayerController::ClientShowDodge_Implementation() {
    const APawn *AvatarPawn = GetPawn();
    if (!AvatarPawn) {
        return;
    }
    UWorld *World = AvatarPawn->GetWorld();
    if (!World) {
        return;
    }
    if (UMythicDamageNumberSubsystem *DamageNumbers = World->GetSubsystem<UMythicDamageNumberSubsystem>()) {
        DamageNumbers->AddDodgeNumber(AvatarPawn->GetActorLocation() + FVector(0.0f, 0.0f, 90.0f));
    }
}

void AMythicPlayerController::CheckZoneEntry() {
    // Server-authoritative poll (the timer is only armed on authority). Map the pawn's cell -> governing settlement and,
    // on a change of the stable runtime SettlementId, announce it to the owning client.
    if (!HasAuthority()) {
        return;
    }
    const APawn *AvatarPawn = GetPawn();
    if (!AvatarPawn) {
        return; // not possessed yet
    }
    UMythicLivingWorldSubsystem *LW = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMythicLivingWorldSubsystem>() : nullptr;
    const UMythicTerritoryGrid *Grid = LW ? LW->GetTerritoryGrid() : nullptr;
    if (!Grid) {
        return; // living world not up (e.g. a menu map)
    }
    const FMythicCellCoord Cell = Grid->WorldToCell(AvatarPawn->GetActorLocation());
    FMythicSettlementData Data;
    // CopySettlementAtCell is the SimulationLock-guarded snapshot wrapper — safe from this game-thread timer (never hold
    // the raw registry pointer). Returns false in the wilderness (no settlement covers the cell).
    const int32 NewSettlementId = LW->CopySettlementAtCell(Cell, Data) ? Data.SettlementId : INDEX_NONE;
    if (NewSettlementId == LastSettlementId) {
        return; // still in the same settlement (or still in open wilderness)
    }
    LastSettlementId = NewSettlementId;
    if (NewSettlementId != INDEX_NONE) {
        ClientNotifyZoneEntry(Data.DisplayName); // crossed into a settlement
    }
    // else: crossed into wilderness — intentionally silent (a "leaving <X>" beat would need the prior name cached).
}

void AMythicPlayerController::ClientNotifyZoneEntry_Implementation(const FText &SettlementName) {
    const APawn *AvatarPawn = GetPawn();
    if (!AvatarPawn) {
        return;
    }
    UWorld *World = AvatarPawn->GetWorld();
    if (!World) {
        return;
    }
    if (UMythicDamageNumberSubsystem *DamageNumbers = World->GetSubsystem<UMythicDamageNumberSubsystem>()) {
        const FVector Location = AvatarPawn->GetActorLocation() + FVector(0.0f, 0.0f, 130.0f);
        DamageNumbers->AddDamageNumberCustom(Location, FString::Printf(TEXT("Welcome to %s"), *SettlementName.ToString()),
                                             FLinearColor(1.0f, 0.85f, 0.1f), 3.0f); // gold, lingers (matches objectives)
    }
}

void AMythicPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (GetWorld()) {
        GetWorld()->GetTimerManager().ClearTimer(ZoneCheckTimerHandle);
    }
    Super::EndPlay(EndPlayReason);
}

bool AMythicPlayerController::ServerExecuteBarterOffer_Validate(AMythicNPCCharacter *NPC, int32 OfferIndex) {
    return NPC != nullptr && OfferIndex >= 0;
}

void AMythicPlayerController::ServerExecuteBarterOffer_Implementation(AMythicNPCCharacter *NPC, int32 OfferIndex) {
    if (!HasAuthority() || !IsValid(NPC) || !NPC->IsMerchant() || !NPC->IsActorInTradeRange(GetPawn())) {
        return;
    }
    const TArray<FMythicMerchantOffer> &Offers = NPC->GetMerchantOffers();
    if (!Offers.IsValidIndex(OfferIndex)) {
        return;
    }
    const FMythicMerchantOffer &Offer = Offers[OfferIndex];
    UItemDefinition *CostDef = Offer.CostItem.LoadSynchronous();
    UItemDefinition *RewardDef = Offer.RewardItem.LoadSynchronous();
    if (!CostDef || !RewardDef || Offer.CostQty < 1 || Offer.RewardQty < 1) {
        return;
    }

    UMythicInventoryComponent *PlayerInv = GetInventoryComponent();
    if (!PlayerInv) {
        return;
    }
    // Atomic: confirm the FULL cost is present before removing anything (no partial charge).
    if (PlayerInv->GetItemCount(CostDef) < Offer.CostQty) {
        return;
    }
    PlayerInv->ServerRemoveItemByDefinition(CostDef, Offer.CostQty);

    // Mint the reward into the player's inventory. CreateAndGive places it and, if it doesn't fit, ALREADY
    // world-drops the overflow internally — returning the spawned world item. A non-null return therefore means
    // "overflow handled", NOT "failed", so we must NOT branch on it to mint again (doing so double-minted the
    // reward on every successful barter — the dupe the economy-arc review caught). Mirror UItemReward::Give: call
    // once, never fall back to CreateAndSpawn.
    if (UMythicLootManagerSubsystem *Loot = GetGameInstance()
        ? GetGameInstance()->GetSubsystem<UMythicLootManagerSubsystem>()
        : nullptr) {
        Loot->CreateAndGive(RewardDef, Offer.RewardQty, this, this, 0);
    }
}

bool AMythicPlayerController::ServerRerollItemAffixes_Validate(UMythicItemInstance *Item) {
    return Item != nullptr;
}

void AMythicPlayerController::ServerRerollItemAffixes_Implementation(UMythicItemInstance *Item) {
    if (!HasAuthority() || !IsValid(Item)) {
        return;
    }
    // Ownership: the item must live in one of THIS player's own inventories (same model as ServerMoveItem), so a
    // player cannot reroll a third party's gear.
    if (!GetAllInventoryComponents().Contains(Item->GetInventoryComponent())) {
        return;
    }
    if (const UAffixesFragment *Affixes = Item->GetFragment<UAffixesFragment>()) {
        // GetFragment returns const, but the fragment is genuinely mutable (OnItemActivated etc. mutate it too); the
        // reroll re-checks authority internally.
        const_cast<UAffixesFragment *>(Affixes)->RerollUnlockedAffixes(Item->GetItemLevel());
    }
}

bool AMythicPlayerController::ServerSetItemAffixLocked_Validate(UMythicItemInstance *Item, int32 AffixIndex, bool bLocked) {
    return Item != nullptr && AffixIndex >= 0;
}

void AMythicPlayerController::ServerSetItemAffixLocked_Implementation(UMythicItemInstance *Item, int32 AffixIndex, bool bLocked) {
    if (!HasAuthority() || !IsValid(Item)) {
        return;
    }
    if (!GetAllInventoryComponents().Contains(Item->GetInventoryComponent())) {
        return;
    }
    if (const UAffixesFragment *Affixes = Item->GetFragment<UAffixesFragment>()) {
        const_cast<UAffixesFragment *>(Affixes)->SetAffixLocked(AffixIndex, bLocked);
    }
}

void AMythicPlayerController::SetupInputComponent() {
    // set up gameplay key bindings
    Super::SetupInputComponent();
}

void AMythicPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused) {
    if (UMythicAbilitySystemComponent *MythicASC = Cast<UMythicAbilitySystemComponent>(GetAbilitySystemComponent())) {
        MythicASC->ProcessAbilityInput(DeltaTime, bGamePaused);
    }

    Super::PostProcessInput(DeltaTime, bGamePaused);
}
