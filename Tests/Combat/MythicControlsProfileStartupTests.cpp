#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/Progression/MythicAbilityWorldFixture.h"
#include "Input/MythicControlsComponent.h"
#include "Input/MythicControlUserSettings.h"
#include "Player/MythicPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputDeveloperSettings.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "UObject/StrongObjectPtr.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeExit.h"

struct FMythicControlsProfileStartupTestAccess {
    static bool Initialized(const UMythicControlsComponent* Controls) { return Controls->bInitialized; }
    static UEnhancedInputUserSettings* Settings(const UMythicControlsComponent* Controls) { return Controls->UserSettings.Get(); }
    static void Cleanup(UMythicControlsComponent* Controls,UEnhancedInputLocalPlayerSubsystem* Subsystem) {
        // This test does not begin world play; release only its own startup registrations explicitly.
        Subsystem->ControlMappingsRebuiltDelegate.RemoveAll(Controls);
        if (Controls->MappingContext) {
            Subsystem->RemoveMappingContext(Controls->MappingContext);
            if (auto* Settings=Subsystem->GetUserSettings()) Settings->UnregisterInputMappingContext(Controls->MappingContext);
        }
        Controls->CancelHeldInput();
    }
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicControlsProfileStartup,
    "Mythic.Controls.RealLocalPlayerProfileStartup",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FMythicControlsProfileStartup::RunTest(const FString&) {
    auto* Policy=GetMutableDefault<UEnhancedInputDeveloperSettings>();
    if (!TestTrue(TEXT("user settings enabled by actual project policy"),Policy->bEnableUserSettings)) return false;
    const FString FixtureSlot=TEXT("Mythic_Controls_Startup_")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
    TGuardValue<FString> SlotGuard(Policy->InputSettingsSaveSlotName,FixtureSlot);
    // No SaveSettings/AsyncSaveSettings call occurs. The unique nonexistent slot prevents personal-profile reads.
    TestFalse(TEXT("isolated startup slot starts absent"),UGameplayStatics::DoesSaveGameExist(FixtureSlot,0));
    auto* Game=NewObject<UGameInstance>(GEngine); Game->InitializeStandalone();
    ON_SCOPE_EXIT { DestroyOwnedAbilityTestWorld(Game); };
    auto* World=Game->GetWorld();
    auto* Context=Game->GetWorldContext();
    if (!TestNotNull(TEXT("owned standalone world context"),Context)
        || !TestTrue(TEXT("context belongs to this fixture world"),Context->World()==World)
        || !TestNull(TEXT("fixture has no borrowed viewport"),Context->GameViewport.Get())
        || !TestTrue(TEXT("automation Slate application exists"),FSlateApplication::IsInitialized())) return false;
    // LocalPlayer resolves both world and game instance through ViewportClient, even before SetPlayer.
    // Bind an actual viewport client to this owned context; never replace GEngine->GameViewport.
    TStrongObjectPtr<UGameViewportClient> Viewport(NewObject<UGameViewportClient>(GEngine));
    Viewport->Init(*Context,Game,false);
    Context->GameViewport=Viewport.Get();
    // DestroyOwnedAbilityTestWorld owns context teardown: SetCurrentWorld(nullptr) clears Init's world ref.
    if (!TestTrue(TEXT("viewport resolves the owned world"),Viewport->GetWorld()==World)
        || !TestTrue(TEXT("viewport resolves the owned game instance"),Viewport->GetGameInstance()==Game)
        || !TestTrue(TEXT("game instance resolves its own viewport"),Game->GetGameViewportClient()==Viewport.Get())) return false;
    auto* Local=NewObject<ULocalPlayer>(GEngine);
    const int32 Index=Game->AddLocalPlayer(Local,FPlatformUserId::CreateFromInternalId(0));
    if (!TestEqual(TEXT("owned local player registered"),Index,0)) return false;
    ON_SCOPE_EXIT { Game->RemoveLocalPlayer(Local); };
    if (!TestTrue(TEXT("local player has a valid world before input startup"),Local->GetWorld()==World)
        || !TestTrue(TEXT("local player has its owned game instance"),Local->GetGameInstance()==Game)) return false;
    auto* Subsystem=Local->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
    if (!TestNotNull(TEXT("actual local subsystem collection"),Subsystem)) return false;
    TestNull(TEXT("no settings before player input startup"),Subsystem->GetUserSettings());
    // Register spawned controllers through their real component initialization lifecycle.
    World->InitializeActorsForPlay(FURL());
    if (!TestTrue(TEXT("owned world actors initialized"),World->AreActorsInitialized())) return false;
    auto* PC=World->SpawnActor<AMythicPlayerController>();
    if (!TestNotNull(TEXT("real production controller"),PC)) return false;
    auto* Controls=PC->FindComponentByClass<UMythicControlsComponent>();
    if (!TestNotNull(TEXT("real production controls"),Controls)) return false;
    ON_SCOPE_EXIT { FMythicControlsProfileStartupTestAccess::Cleanup(Controls,Subsystem); };
    // Exercise the early pending path, then the exact SetPlayer -> SetupInputComponent -> ReceivedPlayer order.
    Controls->InitializeLocalInput();
    TestFalse(TEXT("early call cannot permanently cache unavailable input/settings"),FMythicControlsProfileStartupTestAccess::Initialized(Controls));
    PC->SetPlayer(Local);
    if (!TestTrue(TEXT("local player resolves its registered controller"),Local->GetPlayerController(World)==PC)) return false;
    auto* Settings=Subsystem->GetUserSettings();
    if (!TestNotNull(TEXT("real engine settings initialized"),Settings)) return false;
    auto* Profile=Settings->GetActiveKeyProfile();
    if (!TestNotNull(TEXT("active engine profile"),Profile)) return false;
    TestTrue(TEXT("router retains the same actual engine settings object"),FMythicControlsProfileStartupTestAccess::Settings(Controls)==Settings);
    TestTrue(TEXT("startup completes only after profile readiness"),FMythicControlsProfileStartupTestAccess::Initialized(Controls));
    for (const auto& Binding:GetDefault<UMythicControlsSettings>()->Bindings) for (bool bGamepad:{false,true}) {
        const FKey Key=bGamepad?Binding.Gamepad:Binding.Keyboard; if (!Key.IsValid()) continue;
        const FName Name=UMythicControlsSettings::MappingName(Binding.Control,bGamepad);
        const auto* Row=Profile->FindKeyMappingRow(Name);
        if (!TestNotNull(*FString::Printf(TEXT("real registered row %s"),*Name.ToString()),Row)) return false;
        int32 FirstCount=0;
        for (const auto& Mapping:Row->Mappings) if (Mapping.GetSlot()==EPlayerMappableKeySlot::First) {
            ++FirstCount; TestEqual(TEXT("new profile contains actual authored key"),Mapping.GetCurrentKey(),Key);
        }
        TestEqual(TEXT("one first-slot mapping per control/device"),FirstCount,1);
    }
    const int32 RowCount=Profile->GetPlayerMappingRows().Num();
    FMapPlayerKeyArgs Args; Args.MappingName=UMythicControlsSettings::MappingName(EMythicControl::MoveForward,false);
    Args.ProfileIdString=Settings->GetActiveKeyProfileId(); Args.Slot=EPlayerMappableKeySlot::First;
    Args.NewKey=EKeys::Up; Args.bCreateMatchingSlotIfNeeded=false;
    FGameplayTagContainer Errors; Settings->MapPlayerKey(Args,Errors);
    TestTrue(TEXT("real registered row accepts an in-memory remap"),Errors.IsEmpty());
    Subsystem->ControlMappingsRebuiltThisFrame();
    TestEqual(TEXT("router consumes the actual active profile remap"),Controls->GetEffectiveKey(EMythicControl::MoveForward,false),EKeys::Up);
    Local->ReceivedPlayerController(PC); Controls->InitializeLocalInput();
    TestTrue(TEXT("later engine notification retains the same settings object"),Subsystem->GetUserSettings()==Settings);
    TestEqual(TEXT("repeat startup does not duplicate profile rows"),Profile->GetPlayerMappingRows().Num(),RowCount);
    TestEqual(TEXT("repeat startup preserves the customized key"),Controls->GetEffectiveKey(EMythicControl::MoveForward,false),EKeys::Up);
    TestFalse(TEXT("no personal or fixture save was issued"),UGameplayStatics::DoesSaveGameExist(FixtureSlot,0));
    return true;
}
#endif
