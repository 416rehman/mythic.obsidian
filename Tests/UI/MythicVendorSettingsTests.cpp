
#include "Misc/AutomationTest.h"

#include "UI/Settings/MythicUserSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicVendorSettingsTest,
    "Mythic.UI.VendorSettings",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicVendorSettingsTest::RunTest(const FString &Parameters) {
    const bool bCompiledIn = MYTHIC_WITH_DLSS ? true : false;
    AddInfo(FString::Printf(TEXT("MYTHIC_WITH_DLSS=%d nvidia=%d dlss=%d framegen=%d rayrecon=%d reflex=%d"),
                            MYTHIC_WITH_DLSS,
                            UMythicUserSettings::IsNvidiaGpu() ? 1 : 0,
                            UMythicUserSettings::IsDLSSAvailable() ? 1 : 0,
                            UMythicUserSettings::IsFrameGenerationAvailable() ? 1 : 0,
                            UMythicUserSettings::IsRayReconstructionAvailable() ? 1 : 0,
                            UMythicUserSettings::IsReflexAvailable() ? 1 : 0));

    // The invariant that matters, and the one a machine without the plugin must also satisfy: with DLSS
    // compiled out, nothing may claim it is available. Otherwise a row offers a mode that silently does
    // nothing, which is the failure this gating exists to prevent.
    if (!bCompiledIn) {
        TestFalse(TEXT("compiled out, DLSS cannot report available"), UMythicUserSettings::IsDLSSAvailable());
        TestFalse(TEXT("compiled out, frame generation cannot report available"),
                  UMythicUserSettings::IsFrameGenerationAvailable());
        TestFalse(TEXT("compiled out, ray reconstruction cannot report available"),
                  UMythicUserSettings::IsRayReconstructionAvailable());
    }

    // Availability is layered, and each layer must be a subset of the one beneath it. Frame generation needs
    // a newer card than upscaling does, so claiming it without DLSS is incoherent whatever the hardware.
    if (UMythicUserSettings::IsFrameGenerationAvailable()) {
        TestTrue(TEXT("frame generation implies DLSS"), UMythicUserSettings::IsDLSSAvailable());
    }
    if (UMythicUserSettings::IsDLSSAvailable()) {
        TestTrue(TEXT("DLSS implies an NVIDIA card"), UMythicUserSettings::IsNvidiaGpu());
    }

    // Modes clamp rather than trusting the caller, so a stale saved config cannot index off the end.
    if (UMythicUserSettings *S = UMythicUserSettings::Get()) {
        S->SetDLSSMode(99);
        TestTrue(TEXT("an out-of-range DLSS mode clamps"), S->GetDLSSMode() <= 5);
        S->SetDLSSMode(-4);
        TestEqual(TEXT("a negative DLSS mode floors at off"), S->GetDLSSMode(), 0);
        S->SetFrameGenerationMode(99);
        TestTrue(TEXT("an out-of-range frame generation mode clamps"), S->GetFrameGenerationMode() <= 4);
        S->SetReflexMode(99);
        TestTrue(TEXT("an out-of-range reflex mode clamps"), S->GetReflexMode() <= 2);
    }

    return true;
}
