#include "MythicPlayerController.h"

#include "Mythic.h"
#include "MythicPlayerState.h"
#include "Player/MythicPlayerRegistrySubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "TimerManager.h"
#include "GameModes/MythicCheatManager.h"
#include "GameModes/GameState/MythicGameState.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicTags_GAS.h" // GAS_EVENT_ITEM_ACQUIRED (collect-objective event)
#include "Interfaces/OnlineIdentityInterface.h"
#include "Itemization/Crafting/CraftingComponent.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Conversion/MythicConversionStation.h"
#include "Itemization/Conversion/ConversionStationComponent.h"
#include "Itemization/Storage/MythicStorageContainer.h"
#include "Proficiency/ProficiencyComponent.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Proficiencies.h"
#include "Proficiency/ProficiencyDefinition.h"
#include "Objectives/ObjectiveTracker.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "AI/Cognition/CognitiveBrainComponent.h" // NPC->CognitiveBrain->GetSourceEntity() for recruit
#include "AI/Party/PartySubsystem.h" // server-authoritative party recruit
#include "UI/MythicDamageNumberSubsystem.h" // recruit-result callout
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/Fragments/Passive/AffixesFragment.h"
#include "Itemization/Inventory/Fragments/Passive/DurabilityFragment.h"
#include "Itemization/Loot/MythicLootManagerSubsystem.h"
#include "Itemization/Inventory/Fragments/Passive/PlaceableFragment.h" // placeable deploy: rules + decisions
#include "Engine/AssetManager.h"     // async-load the deployed class (no sync load on a gameplay action)
#include "Engine/GameInstance.h"
#include "Engine/StreamableManager.h"
#include "Interaction/IMythicInteractable.h"          // generic server interaction routing
#include "Interaction/MythicInteractionComponent.h"   // reach re-validation reuses the authored InteractionRange
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

    // register player in the registry subsystem
    if (AMythicPlayerState *PS = GetPlayerState<AMythicPlayerState>()) {
        if (UMythicPlayerRegistrySubsystem *Registry = GetWorld() ? GetWorld()->GetSubsystem<UMythicPlayerRegistrySubsystem>() : nullptr) {
            Registry->RegisterPlayer(PS->GetCanonicalPlayerKey(), PS, this);
        }
    }

    if (this->IsLocalPlayerController()) {
        OnPossessedOnClient();
    }
}

void AMythicPlayerController::OnUnPossess() {
    // unregister player from the registry subsystem
    if (UMythicPlayerRegistrySubsystem *Registry = GetWorld() ? GetWorld()->GetSubsystem<UMythicPlayerRegistrySubsystem>() : nullptr) {
        Registry->UnregisterObject(this);
    }

    Super::OnUnPossess();
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

UProficiencyComponent *AMythicPlayerController::GetProficiencyComponent() const {
    return ProficiencyComponent;
}

int32 AMythicPlayerController::GetPlayerLevel() const {
    UAbilitySystemComponent *ASC = GetAbilitySystemComponent();
    if (!ASC) {
        return 1;
    }

    bool bFound = false;
    int32 Level = UMythicAttributeSet_Proficiencies::GetLevel(ASC, bFound);
    return bFound ? Level : 1;
}

float AMythicPlayerController::GetPlayerLevelProgress() const {
    UAbilitySystemComponent *ASC = GetAbilitySystemComponent();
    if (!ASC) {
        return 0.0f;
    }

    const UMythicAttributeSet_Proficiencies *ProfSet = ASC->GetSet<UMythicAttributeSet_Proficiencies>();
    if (!ProfSet) {
        return 0.0f;
    }

    float Current = ProfSet->GetOverallXp();
    float MaxVal = ProfSet->GetOverallXpMax();
    if (MaxVal <= 0.0f) {
        return 0.0f;
    }

    return FMath::Clamp(Current / MaxVal, 0.0f, 1.0f);
}

TArray<FProficiencySummary> AMythicPlayerController::GetProficiencySummaries() const {
    TArray<FProficiencySummary> Summaries;
    if (!ProficiencyComponent) {
        return Summaries;
    }

    for (int32 i = 0; i < ProficiencyComponent->Proficiencies.Num(); ++i) {
        Summaries.Add(ProficiencyComponent->GetSummary(i));
    }

    return Summaries;
}

FPlayerStatsSummary AMythicPlayerController::GetPlayerStats() const {
    FPlayerStatsSummary Stats;
    UAbilitySystemComponent *ASC = GetAbilitySystemComponent();
    if (!ASC) {
        return Stats;
    }

    // offense
    if (const UMythicAttributeSet_Offense *OffSet = ASC->GetSet<UMythicAttributeSet_Offense>()) {
        Stats.Power = OffSet->GetPower();
        Stats.DamagePerHit = OffSet->GetDamagePerHit();
        Stats.AttackSpeed = OffSet->GetAttackSpeed();
        Stats.CritChance = OffSet->GetCriticalHitChance();
        Stats.CritDamage = OffSet->GetCriticalHitDamage();
    }

    // defense
    if (const UMythicAttributeSet_Defense *DefSet = ASC->GetSet<UMythicAttributeSet_Defense>()) {
        Stats.Armor = DefSet->GetArmor();
        Stats.DodgeChance = DefSet->GetDodgeChance();
        Stats.MaxShield = DefSet->GetMaxShield();
        Stats.ShieldRegenRate = DefSet->GetShieldRegenRate();
        Stats.HealthRegenRate = DefSet->GetHealthRegenRate();
    }

    // life
    if (const UMythicAttributeSet_Life *LifeSet = ASC->GetSet<UMythicAttributeSet_Life>()) {
        Stats.MaxHealth = LifeSet->GetMaxHealth();
    }

    // utility
    if (const UMythicAttributeSet_Utility *UtilSet = ASC->GetSet<UMythicAttributeSet_Utility>()) {
        Stats.MaxStamina = UtilSet->GetMaxStamina();
        Stats.StaminaRegenRate = UtilSet->GetStaminaRegenRate();
        Stats.CooldownReduction = UtilSet->GetCooldownReduction();
        Stats.ProficiencyXPBonus = UtilSet->GetProficiencyXPBonus();
        Stats.BonusSprintSpeed = UtilSet->GetBonusSprintSpeed();
    }

    Stats.PlayerLevel = GetPlayerLevel();

    return Stats;
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

bool AMythicPlayerController::ServerMoveItemBetweenInventories_Validate(UMythicInventoryComponent *Source, int32 SourceSlot, UMythicInventoryComponent *Target,
                                                                        int32 TargetSlot) {
    // Source must be a concrete slot; Target may be INDEX_NONE (any-slot).
    return Source != nullptr && Target != nullptr && SourceSlot >= 0 && TargetSlot >= -1;
}

bool AMythicPlayerController::CanPlayerAccessInventory(UMythicInventoryComponent *Inventory) const {
    if (!Inventory) {
        return false;
    }
    // One of the player's own inventories is always allowed.
    if (GetAllInventoryComponents().Contains(Inventory)) {
        return true;
    }
    // Otherwise it must belong to a storage container the player currently has open AND is in range of.
    if (AMythicStorageContainer *Container = Cast<AMythicStorageContainer>(Inventory->GetOwner())) {
        return Container->Server_IsOpener(this) && Container->IsActorInRange(GetPawn());
    }
    return false;
}

void AMythicPlayerController::ServerMoveItemBetweenInventories_Implementation(UMythicInventoryComponent *Source, int32 SourceSlot,
                                                                              UMythicInventoryComponent *Target, int32 TargetSlot) {
    if (!HasAuthority() || !Source || !Target) {
        return;
    }

    // The player may act on an inventory only if it is one of their own OR an open, in-range container (prevents
    // touching a third party's items or acting out of range).
    if (!CanPlayerAccessInventory(Source) || !CanPlayerAccessInventory(Target)) {
        return;
    }

    // Player-initiated take must respect the source slot's bCanPlayerTake (SendItem only checks the target's
    // bCanPlayerPut).
    if (!Source->CanPlayerTakeFromSlot(SourceSlot)) {
        return;
    }

    Source->SendItem(SourceSlot, Target, TargetSlot);
}

bool AMythicPlayerController::ServerDeployPlaceable_Validate(UMythicInventoryComponent *Inventory, int32 SlotIndex,
                                                            FVector AimOrigin, FVector AimDirection) {
    return Inventory != nullptr && SlotIndex >= 0 && !AimDirection.IsNearlyZero();
}

void AMythicPlayerController::ServerDeployPlaceable_Implementation(UMythicInventoryComponent *Inventory, int32 SlotIndex,
                                                                  FVector AimOrigin, FVector AimDirection) {
    if (!HasAuthority() || !Inventory) {
        return;
    }
    UWorld *World = GetWorld();
    APawn *MyPawn = GetPawn();
    if (!World || !MyPawn) {
        return;
    }

    // Gate inputs for the (tested) deploy decision.
    const bool bAuthorized = CanPlayerAccessInventory(Inventory);
    UMythicItemInstance *Item = Inventory->GetItem(SlotIndex);
    const UPlaceableFragment *Placeable = Item ? Item->GetFragment<UPlaceableFragment>() : nullptr;
    const bool bHasPlaceableItem = (Item != nullptr) && (Placeable != nullptr) && Inventory->CanPlayerTakeFromSlot(SlotIndex);
    const bool bHasDeployedClass = (Placeable != nullptr) && !Placeable->DeployedActorClass.IsNull();

    // Authoritative trace from the supplied aim — only meaningful when we actually have a placeable (its reach /
    // rules drive it); otherwise PlanDeploy rejects on the slot/content gate first, so the placement value is unused.
    EPlaceablePlacementResult Placement = EPlaceablePlacementResult::NoSurface;
    FVector CandidatePoint = FVector::ZeroVector;
    if (Placeable) {
        const FVector Dir = AimDirection.GetSafeNormal();
        const FVector TraceEnd = AimOrigin + Dir * Placeable->MaxPlacementReach;

        FCollisionQueryParams TraceParams(FName(TEXT("MythicDeployPlaceable")), /*bTraceComplex*/ false, MyPawn);
        FHitResult Hit;
        const bool bHit = World->LineTraceSingleByChannel(Hit, AimOrigin, TraceEnd, ECC_Visibility, TraceParams);
        CandidatePoint = bHit ? Hit.ImpactPoint : TraceEnd;

        // Clearance: reject if a pawn already occupies the spot (don't deploy inside a player / creature). Geometry
        // clearance against static walls is a logged refinement, not done here.
        const bool bBlocked = World->OverlapAnyTestByChannel(CandidatePoint, FQuat::Identity, ECC_Pawn,
                                                             FCollisionShape::MakeSphere(Placeable->RequiredClearanceRadius), TraceParams);

        const FPlaceablePlacementQuery Query = UPlaceableFragment::BuildPlacementQuery(
            bHit, Hit.ImpactPoint, Hit.ImpactNormal, TraceEnd, MyPawn->GetActorLocation(), bBlocked);
        Placement = Placeable->EvaluatePlacement(Query);
    }

    const EPlaceableDeployResult Decision = UPlaceableFragment::PlanDeploy(bAuthorized, bHasPlaceableItem, bHasDeployedClass, Placement);
    if (Decision != EPlaceableDeployResult::Deployed) {
        // The verdict already carries the rejection reason; a client-facing "can't place here" callout is a follow-up.
        return;
    }

    // Deploy upright, facing away from the player, at the validated point.
    FMythicPendingDeploy Pending;
    Pending.Inventory = Inventory;
    Pending.Item = Item;
    Pending.SlotIndex = SlotIndex;
    Pending.SpawnTransform = FTransform(FRotator(0.0f, MyPawn->GetActorRotation().Yaw, 0.0f), CandidatePoint);

    // Spawn from a soft class without a synchronous load (banned on a gameplay action). If it is already resident,
    // finish immediately; otherwise async-load then finish (re-validating across the gap).
    if (UClass *Resident = Placeable->DeployedActorClass.Get()) {
        FinishDeployPlaceable(Resident, Pending);
        return;
    }
    const FSoftObjectPath ClassPath = Placeable->DeployedActorClass.ToSoftObjectPath();
    UAssetManager::GetStreamableManager().RequestAsyncLoad(
        ClassPath, FStreamableDelegate::CreateUObject(this, &AMythicPlayerController::HandleDeployClassLoaded, ClassPath, Pending));
}

void AMythicPlayerController::HandleDeployClassLoaded(FSoftObjectPath ClassPath, FMythicPendingDeploy Pending) {
    UClass *DeployedClass = Cast<UClass>(ClassPath.ResolveObject());
    FinishDeployPlaceable(DeployedClass, Pending);
}

void AMythicPlayerController::FinishDeployPlaceable(UClass *DeployedClass, const FMythicPendingDeploy &Pending) {
    if (!HasAuthority() || !DeployedClass) {
        return;
    }
    UWorld *World = GetWorld();
    UMythicInventoryComponent *Inventory = Pending.Inventory.Get();
    UMythicItemInstance *Item = Pending.Item.Get();
    if (!World || !Inventory || !Item) {
        return; // the inventory / item went away during the async gap — abort without consuming.
    }

    // Re-validate across the (possibly async) load gap: the slot must still hold THIS item, still be takeable, and
    // the player must still have access (e.g. didn't walk away from a container).
    if (Inventory->GetItem(Pending.SlotIndex) != Item || !Inventory->CanPlayerTakeFromSlot(Pending.SlotIndex) ||
        !CanPlayerAccessInventory(Inventory)) {
        return;
    }

    // Deferred spawn so the actor is fully initialized before BeginPlay; consume the item ONLY after a real spawn
    // (so a failed deploy never eats the item).
    AActor *Deployed = World->SpawnActorDeferred<AActor>(DeployedClass, Pending.SpawnTransform, this, GetPawn(),
                                                         ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
    if (!Deployed) {
        return;
    }
    Deployed->FinishSpawning(Pending.SpawnTransform);
    Item->ConsumeItem(1);
}

bool AMythicPlayerController::ServerInteractPrimary_Validate(AActor *Interactable) {
    return Interactable != nullptr;
}

void AMythicPlayerController::ServerInteractPrimary_Implementation(AActor *Interactable) {
    if (!HasAuthority() || !IsValid(Interactable)) {
        return;
    }
    if (!Interactable->GetClass()->ImplementsInterface(UMythicInteractable::StaticClass())) {
        return;
    }

    // Anti-exploit: re-validate reach server-side (a client could forge this RPC for any actor). Reuse the
    // interaction component's authored range (single source — no duplicated constant) with latency slack; fall back
    // to a sane default if no interaction component is present.
    if (const APawn *MyPawn = GetPawn()) {
        float ReachSq = FMath::Square(400.0f);
        const UMythicInteractionComponent *Interaction = FindComponentByClass<UMythicInteractionComponent>();
        if (!Interaction) {
            Interaction = MyPawn->FindComponentByClass<UMythicInteractionComponent>();
        }
        if (Interaction) {
            ReachSq = FMath::Square(Interaction->InteractionRange * 1.5f);
        }
        if (FVector::DistSquared(MyPawn->GetActorLocation(), Interactable->GetActorLocation()) > ReachSq) {
            return; // beyond plausible interaction range
        }
    }

    // Re-run the interaction SERVER-side; the interactable's OnPrimaryInteract sees HasAuthority() and acts.
    IMythicInteractable::Execute_OnPrimaryInteract(Interactable, this);
}

bool AMythicPlayerController::ServerRequestNpcDialogue_Validate(AMythicNPCCharacter *NPC) {
    return NPC != nullptr;
}

void AMythicPlayerController::OfferNpcQuestIfAny(AMythicNPCCharacter *NPC) {
    // Quest-giver: if this NPC offers a quest, assign it server-side (ServerAddObjective is idempotent, so re-interacting
    // is harmless). SERVER-VALIDATE proximity (the interaction scanner only enforces range client-side) so a modded
    // client can't bulk-enroll across the map. SINGLE SOURCE (Rule 3): called from BOTH the dialogue AND recruit paths
    // — a recruitable quest-giver must still hand out its quest (the recruit verb otherwise bypasses this).
    if (!IsValid(NPC) || NPC->GetWorld() != GetWorld()) {
        return;
    }
    if (UObjectiveDefinition *Offer = NPC->GetQuestOffer()) {
        FObjectiveProgress Progress;
        EObjectiveOfferResult Result = EObjectiveOfferResult::Invalid;
        if (!NPC->IsActorInTradeRange(GetPawn())) {
            Result = EObjectiveOfferResult::OutOfRange;
        }
        else if (ObjectiveTracker) {
            Result = ObjectiveTracker->ServerTryAddObjective(Offer, Progress);
        }

        if (Result == EObjectiveOfferResult::Assigned ||
            Result == EObjectiveOfferResult::AlreadyActive ||
            Result == EObjectiveOfferResult::AlreadyCompleted ||
            Result == EObjectiveOfferResult::OutOfRange) {
            const EObjectiveNotifyCategory Category = (Result == EObjectiveOfferResult::Assigned || Result == EObjectiveOfferResult::OutOfRange)
                ? EObjectiveNotifyCategory::Assignment
                : EObjectiveNotifyCategory::Duplicate;
            ClientNotifyObjectiveResult(Offer->GetCalloutText(Progress.bCompleted), Category, Result,
                                        Progress.CurrentCount, Offer->RequiredCount, true, false, 0);
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
    // Key the party by the RECRUITER's canonical key (both membership + follow): the companion follows THIS player's
    // pawn via the registry (co-op-correct), and the party persists under the cross-session key.
    FString RecruiterKey;
    if (const AMythicPlayerState *RecruiterPS = GetPlayerState<AMythicPlayerState>()) {
        RecruiterKey = RecruiterPS->GetCanonicalPlayerKey();
    }
    // bOk false here means ONLY "party full" (the dup/null cases are handled above), so the client message is accurate.
    const bool bOk = Party->AddCompanion(RecruiterKey, NPC, Src);
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

void AMythicPlayerController::ClientNotifyObjectiveResult_Implementation(const FText &DisplayText, EObjectiveNotifyCategory Category,
                                                                         EObjectiveOfferResult OfferResult, int32 Current, int32 Required,
                                                                         bool bRewardSucceeded, bool bRewardDroppedNearby, int32 StackIndex) {
    const APawn *AvatarPawn = GetPawn();
    if (!AvatarPawn) {
        return;
    }
    UWorld *World = AvatarPawn->GetWorld();
    if (!World) {
        return;
    }
    UMythicDamageNumberSubsystem *DamageNumbers = World->GetSubsystem<UMythicDamageNumberSubsystem>();
    if (!DamageNumbers) {
        return;
    }

    const FText Message = UObjectiveTracker::BuildObjectiveNotificationText(DisplayText, Category, OfferResult,
                                                                            Current, Required, bRewardSucceeded,
                                                                            bRewardDroppedNearby);
    FLinearColor Color(0.9f, 0.9f, 0.95f);
    float Duration = 2.0f;
    if (Category == EObjectiveNotifyCategory::Completed || Category == EObjectiveNotifyCategory::Assignment) {
        Color = FLinearColor(1.0f, 0.85f, 0.1f);
        Duration = 3.0f;
    }
    else if (Category == EObjectiveNotifyCategory::RewardResult) {
        Color = bRewardSucceeded ? FLinearColor(0.4f, 0.9f, 0.45f) : FLinearColor(0.9f, 0.2f, 0.15f);
        Duration = 2.5f;
    }

    const FVector Location = AvatarPawn->GetActorLocation() + FVector(0.0f, 0.0f, 145.0f + StackIndex * 45.0f);
    DamageNumbers->AddDamageNumberCustom(Location, Message.ToString(), Color, Duration);
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

void AMythicPlayerController::ClientNotifyEnvironmentHazard_Implementation(const FText &HazardName, bool bOnset) {
    const APawn *AvatarPawn = GetPawn();
    if (!AvatarPawn) {
        return;
    }
    UWorld *World = AvatarPawn->GetWorld();
    if (!World) {
        return;
    }
    UMythicDamageNumberSubsystem *DamageNumbers = World->GetSubsystem<UMythicDamageNumberSubsystem>();
    if (!DamageNumbers) {
        return;
    }
    const FVector Location = AvatarPawn->GetActorLocation() + FVector(0.0f, 0.0f, 110.0f);
    if (bOnset) {
        DamageNumbers->AddDamageNumberCustom(Location, HazardName.ToString(),
                                             FLinearColor(1.0f, 0.55f, 0.15f), 2.5f); // amber — a hazard takes hold
    }
    else {
        DamageNumbers->AddDamageNumberCustom(Location, FString::Printf(TEXT("%s subsides"), *HazardName.ToString()),
                                             FLinearColor(0.7f, 0.8f, 0.85f), 2.0f); // cool grey — it passes
    }
}

void AMythicPlayerController::ClientNotifyItemDurability_Implementation(const FText &ItemName, EMythicItemDurabilityBeat Beat) {
    const APawn *AvatarPawn = GetPawn();
    if (!AvatarPawn) {
        return;
    }
    UWorld *World = AvatarPawn->GetWorld();
    if (!World) {
        return;
    }
    UMythicDamageNumberSubsystem *DamageNumbers = World->GetSubsystem<UMythicDamageNumberSubsystem>();
    if (!DamageNumbers) {
        return;
    }
    const FString Item = ItemName.IsEmpty() ? TEXT("Item") : ItemName.ToString();
    const FVector Location = AvatarPawn->GetActorLocation() + FVector(0.0f, 0.0f, 90.0f);
    switch (Beat) {
        case EMythicItemDurabilityBeat::Broken:
            DamageNumbers->AddDamageNumberCustom(Location, FString::Printf(TEXT("%s broke!"), *Item),
                                                 FLinearColor(0.9f, 0.1f, 0.1f), 3.0f); // red, lingers — the dramatic beat
            break;
        case EMythicItemDurabilityBeat::Repaired:
            DamageNumbers->AddDamageNumberCustom(Location, FString::Printf(TEXT("%s repaired"), *Item),
                                                 FLinearColor(0.1f, 0.9f, 0.3f), 2.0f); // green (matches the recruit beat)
            break;
        case EMythicItemDurabilityBeat::LowWarning:
        default:
            DamageNumbers->AddDamageNumberCustom(Location, FString::Printf(TEXT("%s durability low"), *Item),
                                                 FLinearColor(1.0f, 0.6f, 0.1f), 2.0f); // amber warning
            break;
    }
}

void AMythicPlayerController::NotifyItemAcquired(const UItemDefinition *ItemDef, int32 Quantity) {
    if (!ItemDef || Quantity <= 0) {
        return;
    }
    UAbilitySystemComponent *ASC = GetAbilitySystemComponent();
    if (!ASC || !ASC->IsOwnerActorAuthoritative()) {
        return; // server-authoritative: the ObjectiveTracker binds + the event fires on authority only
    }
    // Push the acquisition through the GAS event bus the ObjectiveTracker already listens on. Magnitude carries the
    // quantity (for bCountByEventMagnitude objectives); TargetTags carries the item's own ItemType (for the objective
    // payload-tag filter) — no fabricated taxonomy, just the item's existing type.
    FGameplayEventData Payload;
    Payload.EventTag = GAS_EVENT_ITEM_ACQUIRED;
    Payload.Instigator = GetPawn();
    Payload.Target = ASC->GetAvatarActor();
    Payload.OptionalObject = ItemDef;
    if (ItemDef->ItemType.IsValid()) {
        Payload.TargetTags.AddTag(ItemDef->ItemType);
    }
    Payload.EventMagnitude = static_cast<float>(Quantity);
    ASC->HandleGameplayEvent(GAS_EVENT_ITEM_ACQUIRED, &Payload);
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
