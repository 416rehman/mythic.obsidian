#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "UI/Controls/MythicGameplayWheel.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
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
        Wheel->ItemPreview=NewObject<USizeBox>(Wheel);
        Wheel->PreviewIcon=NewObject<UImage>(Wheel);
        Wheel->PreviewName=NewObject<UCommonTextBlock>(Wheel);
        Wheel->PreviewCount=NewObject<UCommonTextBlock>(Wheel);
        Wheel->PreviewDescription=NewObject<UCommonTextBlock>(Wheel);
        for (int32 Index=0;Index<Wheel->Entries.Num();++Index) {
            Wheel->Entries[Index].Name=FText::FromString(FString::Printf(TEXT("Healing %d"),Index));
            Wheel->Entries[Index].Description=FText::FromString(FString::Printf(TEXT("Description %d"),Index));
            Wheel->Entries[Index].Quantity=Index+1;
        }
        Wheel->bOpen=true;
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
    static bool PreviewVisible(const UMythicGameplayWheel* Wheel) { return Wheel->ItemPreview->GetVisibility()!=ESlateVisibility::Collapsed; }
    static FString Description(const UMythicGameplayWheel* Wheel) { return Wheel->PreviewDescription->GetText().ToString(); }
    static void DepleteSelected(UMythicGameplayWheel* Wheel) { Wheel->Entries[Wheel->ItemIndex(Wheel->Selected)].Quantity=0; Wheel->Highlight(Wheel->Selected); }
    static void Pointer(UMythicGameplayWheel* Wheel,FVector2D Value) { Wheel->SelectDirection(Value,false); }
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicWheelItemPreview,
    "Mythic.Controls.HealingWheelItemPreview",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FMythicWheelItemPreview::RunTest(const FString&) {
    using Access=FMythicGameplayWheelDirectionTestAccess;
    TStrongObjectPtr<UMythicGameplayWheel> Wheel(Access::Create());
    Access::Pointer(Wheel.Get(),{0,-120});
    TestTrue(TEXT("pointer sector shows selected item"),Access::PreviewVisible(Wheel.Get()));
    TestEqual(TEXT("preview uses authored entry description"),Access::Description(Wheel.Get()),FString(TEXT("Description 0")));
    Access::Direction(Wheel.Get(),{1,0});
    TestEqual(TEXT("controller sector updates same preview"),Access::Description(Wheel.Get()),FString(TEXT("Description 2")));
    Access::DepleteSelected(Wheel.Get());
    TestFalse(TEXT("depleted item cannot leave a stale preview"),Access::PreviewVisible(Wheel.Get()));
    Access::Pointer(Wheel.Get(),{0,-120});
    Access::Direction(Wheel.Get(),FVector2D::ZeroVector);
    TestFalse(TEXT("neutral clears item preview"),Access::PreviewVisible(Wheel.Get()));
    Access::Pointer(Wheel.Get(),{0,-120}); Access::Page(Wheel.Get(),1);
    TestFalse(TEXT("page change clears old occupant"),Access::PreviewVisible(Wheel.Get()));
    Access::Pointer(Wheel.Get(),{0,-120});
    TestEqual(TEXT("second page displays its own identity"),Access::Description(Wheel.Get()),FString(TEXT("Description 8")));
    Access::Device(Wheel.Get()); TestFalse(TEXT("device change clears old selection"),Access::PreviewVisible(Wheel.Get()));
    Access::Pointer(Wheel.Get(),{0,-120}); Wheel->CloseWithoutCommit();
    TestFalse(TEXT("closing releases the preview"),Access::PreviewVisible(Wheel.Get()));
    TestTrue(TEXT("closed description is cleared"),Access::Description(Wheel.Get()).IsEmpty());
    return true;
}
#endif
