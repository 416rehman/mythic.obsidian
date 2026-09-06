#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "UI/Controls/MythicGameplayWheel.h"
#include "Components/Image.h"
#include "CommonTextBlock.h"
#include "UObject/StrongObjectPtr.h"

struct FMythicGameplayWheelDirectionTestAccess {
    static UMythicGameplayWheel* Create() {
        auto* Wheel=NewObject<UMythicGameplayWheel>();
        // Minimal native projection only. No widget activation, layer, assets, actor, user profile or callback.
        Wheel->Title=NewObject<UCommonTextBlock>(Wheel);
        Wheel->Detail=NewObject<UCommonTextBlock>(Wheel);
        Wheel->PageLabel=NewObject<UCommonTextBlock>(Wheel);
        for (int32 Index=0;Index<8;++Index) {
            auto& Cell=Wheel->Cells.AddDefaulted_GetRef();
            Cell.Plate=NewObject<UImage>(Wheel); Cell.Icon=NewObject<UImage>(Wheel);
            Cell.Count=NewObject<UCommonTextBlock>(Wheel); Cell.Label=NewObject<UCommonTextBlock>(Wheel);
        }
        Wheel->Entries.SetNum(9); // Two healing pages; no inventory/backend mutation.
        return Wheel;
    }
    static void Prime(UMythicGameplayWheel* Wheel,FVector2D Raw,FVector2D Filtered) {
        Wheel->Stick=Raw; Wheel->SelectDirection(Filtered,true);
    }
    static void Device(UMythicGameplayWheel* Wheel) { Wheel->DeviceChanged(ECommonInputType::MouseAndKeyboard); }
    static void Direction(UMythicGameplayWheel* Wheel,FVector2D Value) { Wheel->SelectDirection(Value,true); }
    static void Page(UMythicGameplayWheel* Wheel,int32 Delta) { Wheel->ChangePage(Delta); }
    static int32 Selection(const UMythicGameplayWheel* Wheel) { return Wheel->Selected; }
    static FVector2D Raw(const UMythicGameplayWheel* Wheel) { return Wheel->Stick; }
    static FVector2D Gate(const UMythicGameplayWheel* Wheel) { return Wheel->PageDirection; }
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicWheelDeviceDirection,
    "Mythic.Controls.WheelDeviceRequiresFreshDirection",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FMythicWheelDeviceDirection::RunTest(const FString&) {
    using Access=FMythicGameplayWheelDirectionTestAccess;
    TStrongObjectPtr<UMythicGameplayWheel> Wheel(Access::Create());
    // A diagonal is intentional: dropping one axis would create a different valid sector.
    const FVector2D Raw(.65,-.65),Filtered(.5,-.5);
    Access::Prime(Wheel.Get(),Raw,Filtered);
    TestTrue(TEXT("initial deliberate stick selects a sector"),Access::Selection(Wheel.Get())!=INDEX_NONE);
    Access::Device(Wheel.Get());
    TestEqual(TEXT("device change clears current highlight"),Access::Selection(Wheel.Get()),INDEX_NONE);
    TestTrue(TEXT("modal-owned raw axes both survive device transition"),Access::Raw(Wheel.Get()).Equals(Raw));
    TestTrue(TEXT("fresh-input gate captures the filtered prior direction"),Access::Gate(Wheel.Get()).Equals(Filtered));
    Access::Direction(Wheel.Get(),Filtered);
    TestEqual(TEXT("unchanged held stick cannot select after device transition"),Access::Selection(Wheel.Get()),INDEX_NONE);
    Access::Direction(Wheel.Get(),Filtered+FVector2D(.03,0));
    TestEqual(TEXT("small analog noise cannot rearm selection"),Access::Selection(Wheel.Get()),INDEX_NONE);
    Access::Direction(Wheel.Get(),FVector2D(-.5,-.5));
    TestTrue(TEXT("deliberate changed direction rearms selection"),Access::Selection(Wheel.Get())!=INDEX_NONE);
    Access::Device(Wheel.Get());
    Access::Direction(Wheel.Get(),FVector2D::ZeroVector);
    TestEqual(TEXT("neutral never selects an item"),Access::Selection(Wheel.Get()),INDEX_NONE);
    Access::Direction(Wheel.Get(),FVector2D(-.5,-.5));
    TestTrue(TEXT("neutral then a fresh deflection may select the same sector"),Access::Selection(Wheel.Get())!=INDEX_NONE);
    const FVector2D PageHeld(-.5,-.5);
    Access::Page(Wheel.Get(),1);
    TestEqual(TEXT("new page clears previous highlight"),Access::Selection(Wheel.Get()),INDEX_NONE);
    Access::Direction(Wheel.Get(),PageHeld);
    TestEqual(TEXT("same held direction cannot select a new page occupant"),Access::Selection(Wheel.Get()),INDEX_NONE);
    Access::Device(Wheel.Get());
    Access::Direction(Wheel.Get(),PageHeld);
    TestEqual(TEXT("device transition cannot bypass existing page freshness"),Access::Selection(Wheel.Get()),INDEX_NONE);
    return true;
}
#endif
