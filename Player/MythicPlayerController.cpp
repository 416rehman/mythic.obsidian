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
#include "GAS/MythicTags_GAS.h"
#include "Interfaces/OnlineIdentityInterface.h"

#include "EngineUtils.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Conversion/MythicConversionStation.h"
#include "Itemization/Conversion/ConversionStationComponent.h"
#include "Itemization/MythicTags_Conversion.h"
#include "Itemization/Storage/MythicStorageContainer.h"
#include "World/Death/MythicCorpse.h"
#include "World/Ownership/MythicOwnership.h"
#include "World/Trading/MythicPlayerStall.h"
#include "Itemization/Vendor/MythicVendor.h"
#include "Itemization/Inventory/MythicTrade.h"
#include "Itemization/Inventory/MythicLootFilter.h"
#include "Itemization/MythicTags_Inventory.h"
#include "Proficiency/ProficiencyComponent.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Proficiencies.h"
#include "Proficiency/ProficiencyDefinition.h"
#include "Objectives/ObjectiveTracker.h"
#include "Objectives/MythicObjectiveEvents.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "AI/Cognition/CognitiveBrainComponent.h"
#include "AI/Party/PartySubsystem.h"
#include "UI/MythicDamageNumberSubsystem.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/Fragments/Passive/YieldQualityFragment.h"
#include "Itemization/Inventory/MythicCurrency.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Player/MythicGift.h"
#include "Settings/MythicDeveloperSettings.h"
#include "Player/FastTravel/MythicFastTravelRules.h"
#include "Itemization/Inventory/MythicEncumbrance.h"
#include "Itemization/Inventory/Fragments/Passive/AffixesFragment.h"
#include "Itemization/Inventory/Fragments/Passive/DurabilityFragment.h"
#include "Itemization/Loot/MythicLootManagerSubsystem.h"
#include "Itemization/Inventory/Fragments/Passive/PlaceableFragment.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/StreamableManager.h"
#include "Interaction/IMythicInteractable.h"
#include "Interaction/MythicInteractionComponent.h"
#include "World/EnvironmentController/MythicEnvironmentHazardComponent.h"
#include "World/LivingWorld/Chronicle/MythicChronicleRelayComponent.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/POI/MythicPOIDiscoverySubsystem.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Acquaintance/MythicAcquaintanceComponent.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "World/LivingWorld/Events/ActionEventSubsystem.h"
#include "World/LivingWorld/Events/ActionEventTypes.h"
#include "World/LivingWorld/Morality/MoralSignature.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "Player/MythicFactionStandingComponent.h"

namespace {
FName MythicRarityTagName(EItemRarity Rarity) {
    switch (Rarity) {
    case EItemRarity::Rare:      return FName(TEXT("Itemization.Rarity.Rare"));
    case EItemRarity::Epic:      return FName(TEXT("Itemization.Rarity.Epic"));
    case EItemRarity::Legendary: return FName(TEXT("Itemization.Rarity.Legendary"));
    case EItemRarity::Mythic:    return FName(TEXT("Itemization.Rarity.Mythic"));
    case EItemRarity::Common:
    default:                     return FName(TEXT("Itemization.Rarity.Common"));
    }
}

void MythicStampItemIdentity(FGameplayEventData &Payload, const UItemDefinition *ItemDef) {
    if (!ItemDef) {
        return;
    }
    if (ItemDef->ItemType.IsValid()) {
        Payload.TargetTags.AddTag(ItemDef->ItemType);
    }
    Payload.TargetTags.AddTag(FGameplayTag::RequestGameplayTag(MythicRarityTagName(ItemDef->Rarity)));
    if (const UYieldQualityFragment *Q =
            UItemDefinition::GetFragment<UYieldQualityFragment>(const_cast<UItemDefinition *>(ItemDef))) {
        if (const FGameplayTag QT = Q->GetQualityTag(); QT.IsValid()) {
            Payload.TargetTags.AddTag(QT);
        }
    }
}
}


AMythicPlayerController::AMythicPlayerController() {
    bShowMouseCursor = true;
    DefaultMouseCursor = EMouseCursor::Default;
    bReplicateUsingRegisteredSubObjectList = true;

    CheatClass = UMythicCheatManager::StaticClass();

    ProficiencyComponent = CreateDefaultSubobject<UProficiencyComponent>(TEXT("ProficiencyComponent"));
    ProficiencyComponent->SetIsReplicated(true);

    InventoryComponent = CreateDefaultSubobject<UMythicInventoryComponent>(TEXT("InventoryComponent"));
    InventoryComponent->SetIsReplicated(true);

    ObjectiveTracker = CreateDefaultSubobject<UObjectiveTracker>(TEXT("ObjectiveTracker"));
    ObjectiveTracker->SetIsReplicated(true);

    EnvironmentHazard = CreateDefaultSubobject<UMythicEnvironmentHazardComponent>(TEXT("EnvironmentHazard"));

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
    if (UMythicPlayerRegistrySubsystem *Registry = GetWorld() ? GetWorld()->GetSubsystem<UMythicPlayerRegistrySubsystem>() : nullptr) {
        Registry->UnregisterObject(this);
    }

    Super::OnUnPossess();
}

void AMythicPlayerController::OnRep_PlayerState() {
    Super::OnRep_PlayerState();


    OnPossessedOnClient();
}

void AMythicPlayerController::BeginPlay() {
    Super::BeginPlay();

    if (HasAuthority() && GetWorld() && ZoneCheckInterval > 0.0f) {
        GetWorld()->GetTimerManager().SetTimer(ZoneCheckTimerHandle, this, &AMythicPlayerController::CheckZoneEntry,
                                               ZoneCheckInterval, true);
    }

    if (auto LocalPlayer = this->GetLocalPlayer()) {
        auto LocalIndex = LocalPlayer->GetLocalPlayerIndex();
        this->Login(LocalIndex);
    }
}

void AMythicPlayerController::Login(int32 LocalUserNum) {
    UE_LOG(Myth, Log, TEXT("EOS: Connecting to Online Services"));

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


    LoginDelegateHandle = Identity->
        AddOnLoginCompleteDelegate_Handle(LocalUserNum, FOnLoginCompleteDelegate::CreateUObject(this, &ThisClass::CB_LoginResponse));

    UE_LOG(Myth, Log, TEXT("EOS: Using Online Subsystem: %s"), *OSS->GetSubsystemName().ToString());

    FString AuthType;
    FParse::Value(FCommandLine::Get(), TEXT("AUTH_TYPE="), AuthType);

    if (!AuthType.IsEmpty()) {
        if (!Identity->AutoLogin(LocalUserNum)) {
            UE_LOG(Myth, Error, TEXT("EOS: Failed to start AutoLogin"));

            Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, LoginDelegateHandle);

            LoginDelegateHandle.Reset();
        }
    }
    else {
        FOnlineAccountCredentials Credentials("AccountPortal", "", "");

        UE_LOG(Myth, Log, TEXT("EOS: Logging in to Online service"));

        if (!Identity->Login(LocalUserNum, Credentials)) {
            UE_LOG(Myth, Error, TEXT("EOS: Failed to start Login"));

            Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, LoginDelegateHandle);

            LoginDelegateHandle.Reset();
        }
    }
}

void AMythicPlayerController::CB_LoginResponse(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId &UserId, const FString &Error) {
    if (bWasSuccessful) {
        UE_LOG(Myth, Log, TEXT("EOS: Login successful - %s"), *UserId.ToString());
    }
    else {
        UE_LOG(Myth, Error, TEXT("EOS: Login failed: %s"), *Error);
    }

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

    if (const UMythicAttributeSet_Offense *OffSet = ASC->GetSet<UMythicAttributeSet_Offense>()) {
        Stats.Power = OffSet->GetPower();
        Stats.DamagePerHit = OffSet->GetDamagePerHit();
        Stats.AttackSpeed = OffSet->GetAttackSpeed();
        Stats.CritChance = OffSet->GetCriticalHitChance();
        Stats.CritDamage = OffSet->GetCriticalHitDamage();
    }

    if (const UMythicAttributeSet_Defense *DefSet = ASC->GetSet<UMythicAttributeSet_Defense>()) {
        Stats.Armor = DefSet->GetArmor();
        Stats.DodgeChance = DefSet->GetDodgeChance();
        Stats.MaxShield = DefSet->GetMaxShield();
        Stats.ShieldRegenRate = DefSet->GetShieldRegenRate();
        Stats.HealthRegenRate = DefSet->GetHealthRegenRate();
    }

    if (const UMythicAttributeSet_Life *LifeSet = ASC->GetSet<UMythicAttributeSet_Life>()) {
        Stats.MaxHealth = LifeSet->GetMaxHealth();
    }

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


bool AMythicPlayerController::ServerMoveItemBetweenInventories_Validate(UMythicInventoryComponent *Source, int32 SourceSlot, UMythicInventoryComponent *Target,
                                                                        int32 TargetSlot) {
    return Source != nullptr && Target != nullptr && SourceSlot >= 0 && TargetSlot >= -1;
}

bool AMythicPlayerController::CanPlayerAccessInventory(UMythicInventoryComponent *Inventory) const {
    if (!Inventory) {
        return false;
    }
    if (GetAllInventoryComponents().Contains(Inventory)) {
        return true;
    }
    if (AMythicStorageContainer *Container = Cast<AMythicStorageContainer>(Inventory->GetOwner())) {
        return Container->Server_IsOpener(this) && Container->IsActorInRange(GetPawn());
    }
    if (AMythicCorpse *Corpse = Cast<AMythicCorpse>(Inventory->GetOwner())) {
        return Corpse->Server_IsOpener(this) && Corpse->IsActorInRange(GetPawn());
    }
    return false;
}

void AMythicPlayerController::ServerMoveItemBetweenInventories_Implementation(UMythicInventoryComponent *Source, int32 SourceSlot,
                                                                              UMythicInventoryComponent *Target, int32 TargetSlot) {
    if (!HasAuthority() || !Source || !Target) {
        return;
    }

    if (!CanPlayerAccessInventory(Source) || !CanPlayerAccessInventory(Target)) {
        return;
    }

    if (!Source->CanPlayerTakeFromSlot(SourceSlot)) {
        return;
    }

    Source->SendItem(SourceSlot, Target, TargetSlot);

    if (const AActor *SourceOwnerActor = Source->GetOwner()) {
        if (const UMythicOwnershipComponent *Ownership = SourceOwnerActor->FindComponentByClass<UMythicOwnershipComponent>()) {
            if (Ownership->IsOwned() && GetAllInventoryComponents().Contains(Target)) {
                MythicTheftCrime::TrySubmitTheft(GetPawn(), const_cast<AActor *>(SourceOwnerActor), Ownership->GetOwnership());
            }
        }
    }
}

int32 AMythicPlayerController::GetCarriedCurrency() const {
    int32 Total = 0;
    for (UMythicInventoryComponent *Inv : GetAllInventoryComponents()) {
        if (Inv) {
            Total += Inv->GetTotalCurrency();
        }
    }
    return Total;
}


bool AMythicPlayerController::ServerVendorBuy_Validate(AMythicVendor *Vendor, int32 StockSlotIndex, int32 Quantity) {
    return Vendor != nullptr && StockSlotIndex >= 0 && Quantity > 0;
}

void AMythicPlayerController::ServerVendorBuy_Implementation(AMythicVendor *Vendor, int32 StockSlotIndex, int32 Quantity) {
    if (!HasAuthority() || !Vendor) {
        return;
    }
    if (!CanPlayerAccessInventory(Vendor->GetContainerInventory())) {
        return;
    }
    const FMythicTradePlan Plan = Vendor->Server_ExecuteBuy(this, StockSlotIndex, Quantity);
    if (MythicTrade::IsFailureWorthShowing(Plan.Result)) {
        ClientNotifyTradeResult(Plan.Result);
    }
    RecordVendorAcquaintance(Vendor, Plan);
}

bool AMythicPlayerController::ServerVendorSell_Validate(AMythicVendor *Vendor, UMythicInventoryComponent *PlayerInventory, int32 PlayerSlotIndex,
                                                        int32 Quantity) {
    return Vendor != nullptr && PlayerInventory != nullptr && PlayerSlotIndex >= 0 && Quantity > 0;
}

void AMythicPlayerController::ServerVendorSell_Implementation(AMythicVendor *Vendor, UMythicInventoryComponent *PlayerInventory,
                                                              int32 PlayerSlotIndex, int32 Quantity) {
    if (!HasAuthority() || !Vendor || !PlayerInventory) {
        return;
    }
    if (!CanPlayerAccessInventory(Vendor->GetContainerInventory())) {
        return;
    }
    if (!GetAllInventoryComponents().Contains(PlayerInventory)) {
        return;
    }
    const FMythicTradePlan Plan = Vendor->Server_ExecuteSell(this, PlayerInventory, PlayerSlotIndex, Quantity);
    if (MythicTrade::IsFailureWorthShowing(Plan.Result)) {
        ClientNotifyTradeResult(Plan.Result);
    }
    RecordVendorAcquaintance(Vendor, Plan);
}

bool AMythicPlayerController::ServerStallBuy_Validate(AMythicPlayerStall *Stall, int32 StallSlotIndex, int32 Quantity) {
    return Stall != nullptr && StallSlotIndex >= 0 && Quantity > 0;
}

void AMythicPlayerController::ServerStallBuy_Implementation(AMythicPlayerStall *Stall, int32 StallSlotIndex, int32 Quantity) {
    if (!HasAuthority() || !Stall) {
        return;
    }
    if (!CanPlayerAccessInventory(Stall->GetContainerInventory())) {
        return;
    }
    const FMythicTradePlan Plan = Stall->Server_ExecuteStallPurchase(this, StallSlotIndex, Quantity);
    if (MythicTrade::IsFailureWorthShowing(Plan.Result)) {
        ClientNotifyTradeResult(Plan.Result);
    }
}

void AMythicPlayerController::RecordVendorAcquaintance(const AMythicVendor *Vendor, const FMythicTradePlan &Plan) {
    if (!HasAuthority() || !Vendor || Plan.Quantity <= 0) {
        return;
    }
    AMythicPlayerState *PS = GetPlayerState<AMythicPlayerState>();
    UMythicAcquaintanceComponent *Acquaintance = PS ? PS->GetAcquaintanceComponent() : nullptr;
    if (!Acquaintance) {
        return;
    }

    FGameplayTag VendorFactionTag;
    if (const UMythicLivingWorldSubsystem *LW = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMythicLivingWorldSubsystem>() : nullptr) {
        if (const UMythicFactionDatabase *FactionDB = LW->GetFactionDatabase()) {
            FMythicFactionData FactionData;
            if (FactionDB->GetFaction(Vendor->GetVendorFaction(), FactionData)) {
                VendorFactionTag = FactionData.FactionTag;
            }
        }
    }

    Acquaintance->ServerRecordInteraction(GetTypeHash(Vendor->GetFName()), VendorFactionTag, EMythicNpcInteraction::Traded);
}

bool AMythicPlayerController::ServerVendorRepair_Validate(AMythicVendor *Vendor, UMythicInventoryComponent *PlayerInventory, int32 PlayerSlotIndex) {
    return Vendor != nullptr && PlayerInventory != nullptr && PlayerSlotIndex >= 0;
}

void AMythicPlayerController::ServerVendorRepair_Implementation(AMythicVendor *Vendor, UMythicInventoryComponent *PlayerInventory,
                                                               int32 PlayerSlotIndex) {
    if (!HasAuthority() || !Vendor || !PlayerInventory) {
        return;
    }
    if (!CanPlayerAccessInventory(Vendor->GetContainerInventory())) {
        return;
    }
    if (!GetAllInventoryComponents().Contains(PlayerInventory)) {
        return;
    }
    const FMythicTradePlan Plan = Vendor->Server_ExecuteRepair(this, PlayerInventory, PlayerSlotIndex);
    if (Plan.Result == EMythicTradeResult::Success) {
        if (UMythicItemInstance *Item = PlayerInventory->GetItem(PlayerSlotIndex)) {
            if (const UItemDefinition *Def = Item->GetItemDefinition()) {
                ClientNotifyItemDurability(Def->Name, EMythicItemDurabilityBeat::Repaired);
            }
        }
    }
    else if (MythicTrade::IsFailureWorthShowing(Plan.Result)) {
        ClientNotifyTradeResult(Plan.Result);
    }
}

bool AMythicPlayerController::ServerVendorRepairAll_Validate(AMythicVendor *Vendor, UMythicInventoryComponent *PlayerInventory) {
    return Vendor != nullptr && PlayerInventory != nullptr;
}

void AMythicPlayerController::ServerVendorRepairAll_Implementation(AMythicVendor *Vendor, UMythicInventoryComponent *PlayerInventory) {
    if (!HasAuthority() || !Vendor || !PlayerInventory) {
        return;
    }
    if (!CanPlayerAccessInventory(Vendor->GetContainerInventory())) {
        return;
    }
    if (!GetAllInventoryComponents().Contains(PlayerInventory)) {
        return;
    }
    const FMythicTradePlan Plan = Vendor->Server_ExecuteRepairAll(this, PlayerInventory);
    if (Plan.Result == EMythicTradeResult::Success) {
        ClientNotifyItemDurability(FText::GetEmpty(), EMythicItemDurabilityBeat::Repaired);
    }
    else if (MythicTrade::IsFailureWorthShowing(Plan.Result)) {
        ClientNotifyTradeResult(Plan.Result);
    }
}

bool AMythicPlayerController::ServerBuyback_Validate(AMythicVendor *Vendor, int32 BuybackIndex) {
    return Vendor != nullptr && BuybackIndex >= 0;
}

void AMythicPlayerController::ServerBuyback_Implementation(AMythicVendor *Vendor, int32 BuybackIndex) {
    if (!HasAuthority() || !Vendor) {
        return;
    }
    if (!CanPlayerAccessInventory(Vendor->GetContainerInventory())) {
        return;
    }
    const FMythicTradePlan Plan = Vendor->Server_ExecuteBuyback(this, BuybackIndex);
    if (MythicTrade::IsFailureWorthShowing(Plan.Result)) {
        ClientNotifyTradeResult(Plan.Result);
    }
}


bool AMythicPlayerController::ServerSetItemJunk_Validate(UMythicItemInstance *Item, bool bJunk) {
    return Item != nullptr;
}

void AMythicPlayerController::ServerSetItemJunk_Implementation(UMythicItemInstance *Item, bool bJunk) {
    if (!HasAuthority() || !IsValid(Item)) {
        return;
    }
    if (!GetAllInventoryComponents().Contains(Item->GetInventoryComponent())) {
        return;
    }
    Item->ServerSetMarkedJunk(bJunk);
}

bool AMythicPlayerController::ServerSellAllJunk_Validate(AMythicVendor *Vendor) {
    return Vendor != nullptr;
}

void AMythicPlayerController::ServerSellAllJunk_Implementation(AMythicVendor *Vendor) {
    if (!HasAuthority() || !Vendor) {
        return;
    }
    if (!CanPlayerAccessInventory(Vendor->GetContainerInventory())) {
        return;
    }

    for (UMythicInventoryComponent *Inv : GetAllInventoryComponents()) {
        if (!Inv) {
            continue;
        }
        const int32 NumSlots = Inv->GetNumSlots();
        for (int32 SlotIdx = 0; SlotIdx < NumSlots; ++SlotIdx) {
            FMythicInventorySlotEntry Entry;
            if (!Inv->GetSlotEntry(SlotIdx, Entry)) {
                continue;
            }
            UMythicItemInstance *Item = Entry.SlottedItemInstance;
            if (!IsValid(Item)) {
                continue;
            }
            const UItemDefinition *Def = Item->GetItemDefinition();
            if (!Def) {
                continue;
            }
            const bool bIsCurrency = Def->ItemType.MatchesTag(ITEMIZATION_TYPE_CURRENCY);
            const bool bCanTake = Inv->CanPlayerTakeFromSlot(SlotIdx);
            const bool bJunk = MythicLootFilter::IsJunk(Item->IsMarkedJunk(),
                                                        static_cast<int32>(Def->Rarity.GetValue()),
                                                        MythicLootFilter::DefaultMaxJunkRarity,
                                                        Def->Value, bIsCurrency, Entry.bEquipmentSlot, bCanTake);
            if (!bJunk) {
                continue;
            }
            Vendor->Server_ExecuteSell(this, Inv, SlotIdx, Item->GetStacks());
        }
    }
}


bool AMythicPlayerController::IsWithinGiftRange(const AMythicPlayerController *Other) const {
    const APawn *MyPawn = GetPawn();
    const APawn *OtherPawn = Other ? Other->GetPawn() : nullptr;
    if (!MyPawn || !OtherPawn) {
        return false;
    }
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    const float Range = Settings ? FMath::Max(1.0f, Settings->GiftRange) : 350.0f;
    return FVector::DistSquared(MyPawn->GetActorLocation(), OtherPawn->GetActorLocation()) <= FMath::Square(Range);
}

void AMythicPlayerController::ClearPendingGift() {
    if (UWorld *W = GetWorld()) {
        W->GetTimerManager().ClearTimer(PendingGiftTimerHandle);
    }
    PendingGiftGiver = nullptr;
    PendingGiftSourceInv = nullptr;
    PendingGiftItem = nullptr;
    PendingGiftSourceSlot = INDEX_NONE;
    PendingGiftQuantity = 0;
}

void AMythicPlayerController::OnPendingGiftExpired() {
    ClearPendingGift();
}

bool AMythicPlayerController::ServerOfferGift_Validate(AMythicPlayerController *Recipient, UMythicInventoryComponent *SourceInv, int32 SourceSlotIndex, int32 Quantity) {
    return Recipient != nullptr && SourceInv != nullptr && SourceSlotIndex >= 0;
}

void AMythicPlayerController::ServerOfferGift_Implementation(AMythicPlayerController *Recipient, UMythicInventoryComponent *SourceInv, int32 SourceSlotIndex, int32 Quantity) {
    if (!HasAuthority() || !Recipient || !SourceInv) {
        return;
    }
    if (!GetAllInventoryComponents().Contains(SourceInv)) {
        return;
    }
    UMythicItemInstance *Item = SourceInv->GetItem(SourceSlotIndex);
    const bool bTakeable = (Item != nullptr) && SourceInv->CanPlayerTakeFromSlot(SourceSlotIndex);
    if (!MythicGift::CanOfferGift( true, Recipient != this,
                                  IsWithinGiftRange(Recipient), bTakeable)) {
        return;
    }

    Recipient->ClearPendingGift();
    Recipient->PendingGiftGiver = this;
    Recipient->PendingGiftSourceInv = SourceInv;
    Recipient->PendingGiftItem = Item;
    Recipient->PendingGiftSourceSlot = SourceSlotIndex;
    Recipient->PendingGiftQuantity = Quantity;
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    const float Timeout = Settings ? Settings->GiftOfferTimeoutSeconds : 20.0f;
    if (UWorld *W = GetWorld(); W && Timeout > 0.0f) {
        W->GetTimerManager().SetTimer(Recipient->PendingGiftTimerHandle, Recipient,
                                      &AMythicPlayerController::OnPendingGiftExpired, Timeout,false);
    }

    FText ItemName;
    if (const UItemDefinition *Def = Item->GetItemDefinition()) {
        ItemName = Def->Name;
    }
    Recipient->ClientReceiveGiftOffer(this, ItemName);
}

void AMythicPlayerController::ClientReceiveGiftOffer_Implementation(AMythicPlayerController *Giver, const FText &ItemName) {
    OnGiftOffered(Giver, ItemName);
}

void AMythicPlayerController::ServerRespondGift_Implementation(bool bAccept) {
    if (!HasAuthority()) {
        return;
    }
    AMythicPlayerController *Giver = PendingGiftGiver.Get();
    UMythicInventoryComponent *SourceInv = PendingGiftSourceInv.Get();
    UMythicItemInstance *OfferedItem = PendingGiftItem.Get();
    const int32 SourceSlot = PendingGiftSourceSlot;

    const bool bGiverValid = (Giver != nullptr) && (SourceInv != nullptr) && (OfferedItem != nullptr);
    const bool bInRange = bGiverValid && IsWithinGiftRange(Giver);
    const bool bItemStillThere = bGiverValid && (SourceInv->GetItem(SourceSlot) == OfferedItem);

    FText ItemName;
    if (OfferedItem) {
        if (const UItemDefinition *Def = OfferedItem->GetItemDefinition()) {
            ItemName = Def->Name;
        }
    }

    if (!MythicGift::CanCompleteGift(HasPendingGift(), bAccept, bGiverValid, bInRange, bItemStillThere)) {
        if (HasPendingGift()) {
            if (!bAccept && Giver) {
                Giver->ClientNotifyGiftResult(NSLOCTEXT("Gift", "Declined", "Gift declined"), FLinearColor(0.7f, 0.7f, 0.7f));
            }
            else if (bAccept) {
                ClientNotifyGiftResult(NSLOCTEXT("Gift", "Unavailable", "Gift no longer available"), FLinearColor(0.7f, 0.7f, 0.7f));
            }
        }
        ClearPendingGift();
        return;
    }

    UMythicInventoryComponent *DestInv = nullptr;
    if (const UItemDefinition *Def = OfferedItem->GetItemDefinition()) {
        DestInv = GetInventoryForItemType(Def->ItemType);
    }
    EMythicGiftResult Result = EMythicGiftResult::NoRoom;
    int32 Moved = 0;
    if (DestInv) {
        const UItemDefinition *OfferedDef = OfferedItem->GetItemDefinition();
        const int32 StacksBefore = OfferedItem->GetStacks();
        const int32 GiftQty = MythicGift::ComputeGiftQuantity(PendingGiftQuantity, StacksBefore);

        if (GiftQty >= StacksBefore) {
            SourceInv->ServerQuickMoveToInventory(SourceSlot, DestInv);
            int32 RemainingInGiver = 0;
            if (UMythicItemInstance *StillThere = SourceInv->GetItem(SourceSlot)) {
                if (StillThere->GetItemDefinition() == OfferedDef) {
                    RemainingInGiver = StillThere->GetStacks();
                }
            }
            Moved = FMath::Max(0, StacksBefore - RemainingInGiver);
            Result = MythicGift::ClassifyGiftMove(StacksBefore, Moved);
        }
        else {
            const int32 SplitSlot = SourceInv->SplitStackToFreeSlot(SourceSlot, GiftQty);
            if (SplitSlot == INDEX_NONE) {
                Result = EMythicGiftResult::NoRoom;
                Moved = 0;
            }
            else {
                SourceInv->ServerQuickMoveToInventory(SplitSlot, DestInv);
                int32 RemainingInSplit = 0;
                if (UMythicItemInstance *StillThere = SourceInv->GetItem(SplitSlot)) {
                    if (StillThere->GetItemDefinition() == OfferedDef) {
                        RemainingInSplit = StillThere->GetStacks();
                    }
                }
                Moved = FMath::Max(0, GiftQty - RemainingInSplit);
                Result = MythicGift::ClassifyGiftMove(GiftQty, Moved);
            }
        }
    }

    if (Result == EMythicGiftResult::Success || Result == EMythicGiftResult::Partial) {
        ClientNotifyGiftResult(FText::Format(NSLOCTEXT("Gift", "Received", "Received {0} x{1}"), ItemName, FText::AsNumber(Moved)),
                               FLinearColor(0.45f, 0.9f, 0.45f));
    }
    if (Giver) {
        switch (Result) {
        case EMythicGiftResult::Success:
            Giver->ClientNotifyGiftResult(NSLOCTEXT("Gift", "Given", "Gift given"), FLinearColor(0.45f, 0.9f, 0.45f));
            break;
        case EMythicGiftResult::Partial:
            Giver->ClientNotifyGiftResult(NSLOCTEXT("Gift", "Partial", "Gift partly given (no room for all)"), FLinearColor(0.95f, 0.8f, 0.3f));
            break;
        default:
            Giver->ClientNotifyGiftResult(NSLOCTEXT("Gift", "NoRoom", "Recipient has no room"), FLinearColor(1.0f, 0.5f, 0.3f));
            break;
        }
    }
    ClearPendingGift();
}

void AMythicPlayerController::ClientNotifyGiftResult_Implementation(const FText &Message, FLinearColor Color) {
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

    const bool bAuthorized = CanPlayerAccessInventory(Inventory);
    UMythicItemInstance *Item = Inventory->GetItem(SlotIndex);
    const UPlaceableFragment *Placeable = Item ? Item->GetFragment<UPlaceableFragment>() : nullptr;
    const bool bHasPlaceableItem = (Item != nullptr) && (Placeable != nullptr) && Inventory->CanPlayerTakeFromSlot(SlotIndex);
    const bool bHasDeployedClass = (Placeable != nullptr) && !Placeable->DeployedActorClass.IsNull();

    EPlaceablePlacementResult Placement = EPlaceablePlacementResult::NoSurface;
    FVector CandidatePoint = FVector::ZeroVector;
    if (Placeable) {
        const FVector Dir = AimDirection.GetSafeNormal();
        const FVector TraceEnd = AimOrigin + Dir * Placeable->MaxPlacementReach;

        FCollisionQueryParams TraceParams(FName(TEXT("MythicDeployPlaceable")), false, MyPawn);
        FHitResult Hit;
        const bool bHit = World->LineTraceSingleByChannel(Hit, AimOrigin, TraceEnd, ECC_Visibility, TraceParams);
        CandidatePoint = bHit ? Hit.ImpactPoint : TraceEnd;

        const bool bBlocked = World->OverlapAnyTestByChannel(CandidatePoint, FQuat::Identity, ECC_Pawn,
                                                             FCollisionShape::MakeSphere(Placeable->RequiredClearanceRadius), TraceParams);

        const FPlaceablePlacementQuery Query = UPlaceableFragment::BuildPlacementQuery(
            bHit, Hit.ImpactPoint, Hit.ImpactNormal, TraceEnd, MyPawn->GetActorLocation(), bBlocked);
        Placement = Placeable->EvaluatePlacement(Query);
    }

    const EPlaceableDeployResult Decision = UPlaceableFragment::PlanDeploy(bAuthorized, bHasPlaceableItem, bHasDeployedClass, Placement);
    if (Decision != EPlaceableDeployResult::Deployed) {
        const FText Reason = UPlaceableFragment::DescribeDeployFailure(Decision, Placement);
        if (!Reason.IsEmpty()) {
            ClientNotifyDeployRejected(Reason);
        }
        return;
    }

    FMythicPendingDeploy Pending;
    Pending.Inventory = Inventory;
    Pending.Item = Item;
    Pending.SlotIndex = SlotIndex;
    Pending.SpawnTransform = FTransform(FRotator(0.0f, MyPawn->GetActorRotation().Yaw, 0.0f), CandidatePoint);

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
        return;
    }

    if (Inventory->GetItem(Pending.SlotIndex) != Item || !Inventory->CanPlayerTakeFromSlot(Pending.SlotIndex) ||
        !CanPlayerAccessInventory(Inventory)) {
        return;
    }

    const bool bCapped = MaxDeployedPlaceables > 0;
    if (bCapped) {
        DeployedPlaceables.RemoveAll([](const TWeakObjectPtr<AActor> &P) { return !P.IsValid(); });
        if (!CanDeployMore(DeployedPlaceables.Num(), MaxDeployedPlaceables)) {
            ClientNotifyDeployRejected(NSLOCTEXT("Placeable", "DeployAtCap", "Build limit reached"));
            return;
        }
    }

    AActor *Deployed = World->SpawnActorDeferred<AActor>(DeployedClass, Pending.SpawnTransform, this, GetPawn(),
                                                         ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
    if (!Deployed) {
        return;
    }
    Deployed->FinishSpawning(Pending.SpawnTransform);
    Item->ConsumeItem(1);

    if (bCapped) {
        DeployedPlaceables.Add(Deployed);
    }
}

bool AMythicPlayerController::CanDeployMore(int32 CurrentValidCount, int32 MaxAllowed) {
    return MaxAllowed <= 0 || CurrentValidCount < MaxAllowed;
}

void AMythicPlayerController::ClientNotifyDeployRejected_Implementation(const FText &Reason) {
    OnDeployRejected(Reason);
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
            return;
        }
    }

    IMythicInteractable::Execute_OnPrimaryInteract(Interactable, this);
}

bool AMythicPlayerController::ServerRequestNpcDialogue_Validate(AMythicNPCCharacter *NPC) {
    return NPC != nullptr;
}

void AMythicPlayerController::OfferNpcQuestIfAny(AMythicNPCCharacter *NPC) {
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
            Result == EObjectiveOfferResult::OutOfRange ||
            Result == EObjectiveOfferResult::PrerequisitesNotMet) {
            const EObjectiveNotifyCategory Category = (Result == EObjectiveOfferResult::Assigned
                                                       || Result == EObjectiveOfferResult::OutOfRange
                                                       || Result == EObjectiveOfferResult::PrerequisitesNotMet)
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
    if (NPC->IsActorInTradeRange(GetPawn())) {
        NotifyTalkedToNPC(NPC->GetQuestNpcTag());
        if (ObjectiveTracker && InventoryComponent) {
            ObjectiveTracker->ServerTurnInDeliveriesTo(NPC->GetQuestNpcTag(), InventoryComponent);
        }
    }

    const FText Line = NPC->SelectDialogueFor(this);
    ClientReceiveNpcDialogue(NPC, Line);
}

void AMythicPlayerController::ClientReceiveNpcDialogue_Implementation(AMythicNPCCharacter *NPC, const FText &Line) {
    if (IsValid(NPC)) {
        NPC->FireBark(Line, this);
    }
}


bool AMythicPlayerController::ServerPerformSocialVerb_Validate(AMythicNPCCharacter *NPC, EMythicSocialVerb Verb) {
    return NPC != nullptr;
}

void AMythicPlayerController::ServerPerformSocialVerb_Implementation(AMythicNPCCharacter *NPC, EMythicSocialVerb Verb) {
    if (!HasAuthority() || !IsValid(NPC)) {
        return;
    }
    if (!NPC->IsActorInTradeRange(GetPawn())) {
        return;
    }
    if (Verb >= EMythicSocialVerb::COUNT) {
        return;
    }

    const FMythicSocialReactionResult Result = NPC->ResolveSocialVerb(Verb, this);
    NPC->ApplySocialReaction(Result, Verb, this);

    ClientReceiveSocialReaction(NPC, Verb, Result.Reaction, UMythicSocialVerbLibrary::DefaultBarkFor(Verb, Result.Reaction));
}

void AMythicPlayerController::ClientReceiveSocialReaction_Implementation(AMythicNPCCharacter *NPC, EMythicSocialVerb Verb, EMythicSocialReaction Reaction, const FText &Line) {
    if (IsValid(NPC)) {
        NPC->FireReaction(Verb, Reaction, Line, this);
    }
}

bool AMythicPlayerController::ServerRecruitNpc_Validate(AMythicNPCCharacter *NPC) {
    return NPC != nullptr;
}

void AMythicPlayerController::ServerRecruitNpc_Implementation(AMythicNPCCharacter *NPC) {
    if (!HasAuthority() || !IsValid(NPC)) {
        return;
    }
    if (!NPC->IsActorInTradeRange(GetPawn()) || !NPC->IsRecruitable()) {
        return;
    }
    UMythicPartySubsystem *Party = GetWorld() ? GetWorld()->GetSubsystem<UMythicPartySubsystem>() : nullptr;
    if (!Party) {
        return;
    }
    OfferNpcQuestIfAny(NPC);
    if (Party->IsInParty(NPC)) {
        ClientReceiveNpcDialogue(NPC, NPC->SelectDialogueFor(this));
        return;
    }
    if (!NPC->CognitiveBrain) {
        UE_LOG(Myth, Warning, TEXT("ServerRecruitNpc: '%s' is recruitable but has no cognitive brain — cannot be a companion."), *GetNameSafe(NPC));
        return;
    }
    const FMassEntityHandle Src = NPC->CognitiveBrain->GetSourceEntity();
    if (!Src.IsValid()) {
        UE_LOG(Myth, Warning, TEXT("ServerRecruitNpc: '%s' has no valid MASS source entity — cannot be a companion."), *GetNameSafe(NPC));
        return;
    }
    FString RecruiterKey;
    if (const AMythicPlayerState *RecruiterPS = GetPlayerState<AMythicPlayerState>()) {
        RecruiterKey = RecruiterPS->GetCanonicalPlayerKey();
    }
    const bool bOk = Party->AddCompanion(RecruiterKey, NPC, Src);
    ClientReceiveRecruitResult(NPC, bOk);
}

void AMythicPlayerController::ClientReceiveRecruitResult_Implementation(AMythicNPCCharacter *NPC, bool bSucceeded) {
}

bool AMythicPlayerController::ServerIssueCompanionOrder_Validate(AMythicNPCCharacter *Companion, EMythicCompanionOrder Order, AActor *OrderTarget) {
    return true;
}

void AMythicPlayerController::ServerIssueCompanionOrder_Implementation(AMythicNPCCharacter *Companion, EMythicCompanionOrder Order, AActor *OrderTarget) {
    UMythicPartySubsystem *Party = GetWorld() ? GetWorld()->GetSubsystem<UMythicPartySubsystem>() : nullptr;
    if (!Party) {
        return;
    }
    FString PlayerKey;
    if (const AMythicPlayerState *PS = GetPlayerState<AMythicPlayerState>()) {
        PlayerKey = PS->GetCanonicalPlayerKey();
    }
    if (PlayerKey.IsEmpty()) {
        return;
    }
    Party->IssueCompanionOrder(PlayerKey, Companion, Order, OrderTarget);
}

void AMythicPlayerController::ClientShowGatherProgress_Implementation(FVector Location, int32 HitsRemaining) {
}

void AMythicPlayerController::ClientShowGatherDepleted_Implementation(FVector Location) {
}

void AMythicPlayerController::ClientNotifyProficiencyLevel_Implementation(const FText &ProfName, int32 NewLevel, const FText &MilestoneName) {
    FMythicHudNotice Notice;
    Notice.Kind = EMythicNoticeKind::Progression;
    Notice.Text = FText::Format(NSLOCTEXT("Mythic", "ProfLevelUp", "{0}  Lv {1}"), ProfName, FText::AsNumber(NewLevel));
    Notice.Detail = MilestoneName.IsEmpty()
                        ? FText::GetEmpty()
                        : FText::Format(NSLOCTEXT("Mythic", "MilestoneUnlocked", "{0} unlocked"), MilestoneName);
    RaiseHudNotice(Notice);
}

void AMythicPlayerController::ClientNotifyCompanionDeparted_Implementation(const FText &Name, FVector Location) {
}

void AMythicPlayerController::ClientNotifyCompanionBetrayed_Implementation(const FText &Name, FVector Location) {
}

void AMythicPlayerController::ClientNotifyObjective_Implementation(const FText &DisplayText, int32 Current, int32 Required, bool bCompleted, int32 StackIndex,
                                                                   const FText &QuestTitle) {
    FMythicHudNotice Notice;
    Notice.Kind = EMythicNoticeKind::Objective;
    Notice.Text = DisplayText;
    Notice.Detail = QuestTitle;
    Notice.Accent = FLinearColor(0.86f, 0.81f, 0.70f);
    Notice.StackKey = FName(*DisplayText.ToString());
    Notice.Count = Current;
    Notice.Total = Required;
    Notice.bTerminal = bCompleted;
    RaiseHudNotice(Notice);
}

void AMythicPlayerController::ClientNotifyObjectiveResult_Implementation(const FText &DisplayText, EObjectiveNotifyCategory Category,
                                                                         EObjectiveOfferResult OfferResult, int32 Current, int32 Required,
                                                                         bool bRewardSucceeded, bool bRewardDroppedNearby, int32 StackIndex) {
}

void AMythicPlayerController::RaiseHudNotice(const FMythicHudNotice &Notice) {
    OnHudNotice.Broadcast(Notice);
}

void AMythicPlayerController::ClientNotifyLootPickup_Implementation(const FText &ItemName, int32 Quantity, FLinearColor RarityColor) {
    FMythicHudNotice Notice;
    Notice.Kind = EMythicNoticeKind::Loot;
    Notice.Text = FText::Format(NSLOCTEXT("Mythic", "LootPickup", "+{0}"), ItemName);
    Notice.Accent = RarityColor;
    Notice.Count = FMath::Max(1, Quantity);
    Notice.StackKey = FName(*ItemName.ToString());
    RaiseHudNotice(Notice);
}

void AMythicPlayerController::ClientNotifyRewardCelebration_Implementation(UItemDefinition *ItemDef, int32 Quantity) {
    OnRewardCelebration(ItemDef, Quantity);
}

void AMythicPlayerController::ClientNotifyTradeResult_Implementation(EMythicTradeResult Result) {
    FMythicHudNotice Notice;
    Notice.Kind = EMythicNoticeKind::Warning;
    Notice.Text = MythicTrade::DescribeResult(Result);
    Notice.Accent = FLinearColor(0.78f, 0.35f, 0.30f);
    RaiseHudNotice(Notice);
}

void AMythicPlayerController::ClientNotifyEnvironmentHazard_Implementation(const FText &HazardName, bool bOnset) {
    FMythicHudNotice Notice;
    Notice.Kind = EMythicNoticeKind::Warning;
    Notice.Text = bOnset ? HazardName : FText::Format(NSLOCTEXT("Mythic", "HazardEnded", "{0} passed"), HazardName);
    Notice.Accent = bOnset ? FLinearColor(0.85f, 0.55f, 0.22f) : FLinearColor(0.60f, 0.66f, 0.60f);
    Notice.StackKey = FName(*HazardName.ToString());
    Notice.bTerminal = !bOnset;
    RaiseHudNotice(Notice);
}

void AMythicPlayerController::ClientNotifyItemDurability_Implementation(const FText &ItemName, EMythicItemDurabilityBeat Beat) {
    FMythicHudNotice Notice;
    Notice.Kind = EMythicNoticeKind::Warning;
    switch (Beat) {
        case EMythicItemDurabilityBeat::Broken:
            Notice.Text = FText::Format(NSLOCTEXT("Mythic", "ItemBroke", "{0} broke"), ItemName);
            Notice.Accent = FLinearColor(0.80f, 0.28f, 0.24f);
            break;
        case EMythicItemDurabilityBeat::Repaired:
            Notice.Text = FText::Format(NSLOCTEXT("Mythic", "ItemRepaired", "{0} repaired"), ItemName);
            Notice.Accent = FLinearColor(0.45f, 0.72f, 0.42f);
            break;
        default:
            Notice.Text = FText::Format(NSLOCTEXT("Mythic", "ItemWorn", "{0} is nearly broken"), ItemName);
            Notice.Accent = FLinearColor(0.85f, 0.70f, 0.30f);
            break;
    }
    Notice.StackKey = FName(*ItemName.ToString());
    RaiseHudNotice(Notice);
}

void AMythicPlayerController::NotifyItemAcquired(const UItemDefinition *ItemDef, int32 Quantity) {
    if (!ItemDef || Quantity <= 0) {
        return;
    }
    UAbilitySystemComponent *ASC = GetAbilitySystemComponent();
    if (!ASC || !ASC->IsOwnerActorAuthoritative()) {
        return;
    }
    FGameplayEventData Payload;
    Payload.EventTag = GAS_EVENT_ITEM_ACQUIRED;
    Payload.Instigator = GetPawn();
    Payload.Target = ASC->GetAvatarActor();
    Payload.OptionalObject = ItemDef;
    MythicStampItemIdentity(Payload, ItemDef);
    Payload.EventMagnitude = static_cast<float>(Quantity);
    ASC->HandleGameplayEvent(GAS_EVENT_ITEM_ACQUIRED, &Payload);
}

void AMythicPlayerController::NotifyItemUsed(const UItemDefinition *ItemDef, int32 Quantity) {
    UAbilitySystemComponent *ASC = GetAbilitySystemComponent();
    const bool bServerAuth = ASC && ASC->IsOwnerActorAuthoritative();
    const bool bValidPayload = ItemDef && ItemDef->ItemType.IsValid();
    if (!MythicObjectiveEvents::ShouldEmitObjectiveEvent(bServerAuth, bValidPayload) || Quantity <= 0) {
        return;
    }
    FGameplayEventData Payload;
    Payload.EventTag = GAS_EVENT_ITEM_USED;
    Payload.Instigator = GetPawn();
    Payload.Target = ASC->GetAvatarActor();
    Payload.OptionalObject = ItemDef;
    MythicStampItemIdentity(Payload, ItemDef);
    Payload.EventMagnitude = static_cast<float>(Quantity);
    ASC->HandleGameplayEvent(GAS_EVENT_ITEM_USED, &Payload);
}

void AMythicPlayerController::NotifyItemEquipped(const UItemDefinition *ItemDef) {
    UAbilitySystemComponent *ASC = GetAbilitySystemComponent();
    const bool bServerAuth = ASC && ASC->IsOwnerActorAuthoritative();
    const bool bValidPayload = ItemDef && ItemDef->ItemType.IsValid();
    if (!MythicObjectiveEvents::ShouldEmitObjectiveEvent(bServerAuth, bValidPayload)) {
        return;
    }
    FGameplayEventData Payload;
    Payload.EventTag = GAS_EVENT_ITEM_EQUIPPED;
    Payload.Instigator = GetPawn();
    Payload.Target = ASC->GetAvatarActor();
    Payload.OptionalObject = ItemDef;
    MythicStampItemIdentity(Payload, ItemDef);
    Payload.EventMagnitude = 1.0f;
    ASC->HandleGameplayEvent(GAS_EVENT_ITEM_EQUIPPED, &Payload);
}

void AMythicPlayerController::NotifyTalkedToNPC(const FGameplayTag &NpcTag) {
    UAbilitySystemComponent *ASC = GetAbilitySystemComponent();
    const bool bServerAuth = ASC && ASC->IsOwnerActorAuthoritative();
    if (!MythicObjectiveEvents::ShouldEmitObjectiveEvent(bServerAuth, NpcTag.IsValid())) {
        return;
    }
    FGameplayEventData Payload;
    Payload.EventTag = GAS_EVENT_TALKED_TO_NPC;
    Payload.Instigator = GetPawn();
    Payload.Target = ASC->GetAvatarActor();
    Payload.TargetTags.AddTag(NpcTag);
    Payload.EventMagnitude = 1.0f;
    ASC->HandleGameplayEvent(GAS_EVENT_TALKED_TO_NPC, &Payload);
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
        DamageNumbers->AddCombatText(Location, FString::Printf(TEXT("%d"), Absorbed),
                                             FLinearColor(0.4f, 0.7f, 1.0f), 1.0f);
        if (bBroke) {
            DamageNumbers->AddCombatText(Location + FVector(0.0f, 0.0f, 40.0f), TEXT("Shield Broken!"),
                                                 FLinearColor(0.6f, 0.9f, 1.0f), 1.5f);
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

void AMythicPlayerController::ClientNotifyExhausted_Implementation(bool bExhausted) {
    const APawn *AvatarPawn = GetPawn();
    if (!AvatarPawn) {
        return;
    }
    UWorld *World = AvatarPawn->GetWorld();
    if (!World) {
        return;
    }
    if (UMythicDamageNumberSubsystem *DamageNumbers = World->GetSubsystem<UMythicDamageNumberSubsystem>()) {
        const FVector Loc = AvatarPawn->GetActorLocation() + FVector(0.0f, 0.0f, 90.0f);
        if (bExhausted) {
            DamageNumbers->AddCombatText(Loc, TEXT("Winded!"), FLinearColor(1.0f, 0.55f, 0.1f), 1.2f);
        }
        else {
            DamageNumbers->AddCombatText(Loc, TEXT("Recovered"), FLinearColor(0.45f, 0.9f, 0.45f), 1.0f);
        }
    }
}

void AMythicPlayerController::CheckZoneEntry() {
    if (!HasAuthority()) {
        return;
    }
    APawn *AvatarPawn = GetPawn();
    if (!AvatarPawn) {
        return;
    }
    UMythicLivingWorldSubsystem *LW = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMythicLivingWorldSubsystem>() : nullptr;
    const UMythicTerritoryGrid *Grid = LW ? LW->GetTerritoryGrid() : nullptr;
    if (!Grid) {
        return;
    }
    const FMythicCellCoord Cell = Grid->WorldToCell(AvatarPawn->GetActorLocation());

    FMythicSettlementData Data;
    const int32 NewSettlementId = LW->CopySettlementAtCell(Cell, Data) ? Data.SettlementId : INDEX_NONE;
    if (NewSettlementId != LastSettlementId) {
        LastSettlementId = NewSettlementId;
        if (NewSettlementId != INDEX_NONE) {
            DiscoveredSettlements.Add(NewSettlementId);
            ClientNotifyZoneEntry(Data.DisplayName);
        }
    }

    const FMythicFactionId DomFaction = Grid->GetDominantFaction(Cell);
    if (DomFaction.Index != LastTerritoryFactionIndex) {
        LastTerritoryFactionIndex = DomFaction.Index;

        AMythicPlayerState *PS = GetPlayerState<AMythicPlayerState>();
        UMythicFactionStandingComponent *Standing = PS ? PS->GetFactionStanding() : nullptr;
        const bool bUnwelcome = DomFaction.IsValid() && Standing
            && Standing->TierForStanding(Standing->GetStanding(DomFaction)) == EMythicStandingTier::Hostile;
        if (bUnwelcome) {
            if (UMythicActionEventSubsystem *ActionSub = GetWorld() ? GetWorld()->GetSubsystem<UMythicActionEventSubsystem>() : nullptr) {
                FMythicActionEvent Trespass;
                Trespass.Perpetrator = AvatarPawn;
                Trespass.VictimFactionOverride = DomFaction;
                Trespass.OverrideCell = Cell;
                Trespass.ActionTag = TAG_LIVINGWORLD_ACTION_PROPERTY_TRESPASS;
                Trespass.CategoryFlags = EMythicEventCategory::Social;
                Trespass.Significance = 0.3f;
                Trespass.MoralVector = FMythicMoralSignature::MakeTrespassActionMoralVector();
                if (PS) {
                    Trespass.PerpPlayerKey = PS->GetCanonicalPlayerKey();
                }
                ActionSub->SubmitAction(Trespass);
            }
        }
    }
}

bool AMythicPlayerController::CanFastTravel(const TSet<int32> &Discovered, int32 SettlementId, bool bBlocked) {
    return SettlementId != INDEX_NONE && !bBlocked && Discovered.Contains(SettlementId);
}

bool AMythicPlayerController::ServerFastTravel_Validate(int32 SettlementId) {
    return true;
}

void AMythicPlayerController::ServerFastTravel_Implementation(int32 SettlementId) {
    if (!HasAuthority()) {
        return;
    }
    APawn *AvatarPawn = GetPawn();
    if (!AvatarPawn) {
        return;
    }

    bool bBlocked = false;
    if (const UAbilitySystemComponent *ASC = GetAbilitySystemComponent()) {
        bBlocked = ASC->HasMatchingGameplayTag(GAS_STATE_INCOMBAT);
    }
    const bool bOverloaded = IsOverloadedForFastTravel();

    const bool bBetweenOk =
        MythicFastTravel::CanFastTravelBetween(DiscoveredSettlements, LastSettlementId, SettlementId, bBlocked);
    if (!MythicFastTravel::CanFastTravelWithCargo(bBetweenOk, bOverloaded)) {
        return;
    }

    UMythicLivingWorldSubsystem *LW = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMythicLivingWorldSubsystem>() : nullptr;
    if (!LW) {
        return;
    }

    FVector Anchor = FVector::ZeroVector;
    bool bResolved = false;
    if (const AMythicSettlement *Settlement = LW->GetSettlementActorSafe(SettlementId)) {
        Anchor = Settlement->GetActorLocation();
        bResolved = true;
    }
    else {
        FMythicSettlementData Data;
        if (LW->CopySettlementById(SettlementId, Data)) {
            if (const UMythicTerritoryGrid *Grid = LW->GetTerritoryGrid()) {
                Anchor = Grid->CellToWorld(Data.CenterCell);
                Anchor.Z = AvatarPawn->GetActorLocation().Z;
                bResolved = true;
            }
        }
    }
    if (!bResolved) {
        return;
    }

    Anchor.Z += 100.0f;
    AvatarPawn->TeleportTo(Anchor, AvatarPawn->GetActorRotation());
}

bool AMythicPlayerController::IsOverloadedForFastTravel() const {
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    if (!Settings || !Settings->bEncumbranceEnabled) {
        return false;
    }
    float TotalWeight = 0.0f;
    for (const UMythicInventoryComponent *Inv : GetAllInventoryComponents()) {
        if (Inv) {
            TotalWeight += Inv->GetTotalCarriedWeight();
        }
    }
    return MythicEncumbrance::ComputeTier(TotalWeight, Settings->EncumbranceSoftCapacity, Settings->EncumbranceHardCapacity)
        == EMythicEncumbranceTier::Overloaded;
}

bool AMythicPlayerController::ServerFastTravelToPOI_Validate(int32 POIId) {
    return true;
}

void AMythicPlayerController::ServerFastTravelToPOI_Implementation(int32 POIId) {
    if (!HasAuthority()) {
        return;
    }
    APawn *AvatarPawn = GetPawn();
    if (!AvatarPawn) {
        return;
    }
    UGameInstance *GI = GetGameInstance();
    UMythicPOIDiscoverySubsystem *POI = GI ? GI->GetSubsystem<UMythicPOIDiscoverySubsystem>() : nullptr;
    if (!POI) {
        return;
    }

    if (const UAbilitySystemComponent *ASC = GetAbilitySystemComponent()) {
        if (ASC->HasMatchingGameplayTag(GAS_STATE_INCOMBAT)) {
            ClientNotifyFastTravelRefused(NSLOCTEXT("Mythic", "TravelInCombat", "Not while you are being hunted."));
            return;
        }
    }
    if (IsOverloadedForFastTravel()) {
        ClientNotifyFastTravelRefused(NSLOCTEXT("Mythic", "TravelOverloaded", "Too heavily laden to travel. Drop something, or walk."));
        return;
    }
    if (!POI->IsPOIUnlocked(POIId)) {
        ClientNotifyFastTravelRefused(NSLOCTEXT("Mythic", "TravelUnknownDest", "You have not found that place yet."));
        return;
    }
    if (POI->ResolveCurrentPOI(AvatarPawn->GetActorLocation()) == INDEX_NONE) {
        ClientNotifyFastTravelRefused(NSLOCTEXT("Mythic", "TravelNotAtNode", "You can only depart from a landmark you have found."));
        return;
    }

    POI->ServerFastTravelToPOI(AvatarPawn, POIId);
}

void AMythicPlayerController::ClientNotifyFastTravelRefused_Implementation(const FText &Reason) {
    FMythicHudNotice Notice;
    Notice.Kind = EMythicNoticeKind::Warning;
    Notice.Text = Reason;
    Notice.Accent = FLinearColor(0.85f, 0.70f, 0.30f);
    Notice.StackKey = FName(TEXT("FastTravelRefused"));
    RaiseHudNotice(Notice);
}

void AMythicPlayerController::ClientNotifyZoneEntry_Implementation(const FText &SettlementName) {
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
    if (PlayerInv->GetItemCount(CostDef) < Offer.CostQty) {
        return;
    }
    PlayerInv->ServerRemoveItemByDefinition(CostDef, Offer.CostQty);

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
    if (!GetAllInventoryComponents().Contains(Item->GetInventoryComponent())) {
        return;
    }
    const UAffixesFragment *Affixes = Item->GetFragment<UAffixesFragment>();
    if (!Affixes) {
        return;
    }

    {
        bool bNearForge = false;
        const APawn *MyPawn = GetPawn();
        UWorld *World = GetWorld();
        if (MyPawn && World) {
            const FVector MyLoc = MyPawn->GetActorLocation();
            for (TActorIterator<AMythicConversionStation> It(World); It && !bNearForge; ++It) {
                AMythicConversionStation *Station = *It;
                const UConversionStationComponent *Conv = Station ? Station->GetConversionComponent() : nullptr;
                if (!Conv || !Conv->GetStationTags().HasTag(ITEMIZATION_STATION_FORGE)) {
                    continue;
                }
                const float DistSq = FVector::DistSquared(MyLoc, Station->GetActorLocation());
                if (IsWithinStationRange(DistSq, Conv->GetServerUseRangeSq())) {
                    bNearForge = true;
                }
            }
        }
        if (!bNearForge) {
            ClientNotifyTradeResult(EMythicTradeResult::RequiresStation);
            return;
        }
    }

    int32 RerollCost = 0;
    if (const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>()) {
        const int32 RarityIndex = Item->GetItemDefinition() ? static_cast<int32>(Item->GetItemDefinition()->Rarity.GetValue()) : 0;
        RerollCost = MythicCurrency::ComputeRerollCost(Item->GetItemLevel(), RarityIndex, Settings->RerollBaseCost,
                                                       Settings->RerollCostPerLevelFraction, Settings->RerollCostPerRarityFraction);
    }

    if (RerollCost > 0) {
        int32 Wallet = 0;
        for (const UMythicInventoryComponent *Inv : GetAllInventoryComponents()) {
            if (Inv) {
                Wallet += Inv->GetTotalCurrency();
            }
        }
        if (!MythicCurrency::CanAfford(Wallet, RerollCost)) {
            ClientNotifyTradeResult(EMythicTradeResult::InsufficientFunds);
            return;
        }
        int32 Remaining = RerollCost;
        for (UMythicInventoryComponent *Inv : GetAllInventoryComponents()) {
            if (Remaining <= 0) {
                break;
            }
            if (Inv) {
                Remaining -= Inv->SpendCurrency(Remaining);
            }
        }
    }

    const_cast<UAffixesFragment *>(Affixes)->RerollUnlockedAffixes(Item->GetItemLevel());
}

bool AMythicPlayerController::IsWithinStationRange(float DistSq, float RangeSq) {
    return RangeSq > 0.0f && DistSq <= RangeSq;
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
    Super::SetupInputComponent();
}

void AMythicPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused) {
    if (UMythicAbilitySystemComponent *MythicASC = Cast<UMythicAbilitySystemComponent>(GetAbilitySystemComponent())) {
        MythicASC->ProcessAbilityInput(DeltaTime, bGamePaused);
    }

    Super::PostProcessInput(DeltaTime, bGamePaused);
}
