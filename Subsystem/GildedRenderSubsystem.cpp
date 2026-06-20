#include "GildedRenderSubsystem.h"
#include "HAL/IConsoleManager.h"

void UGildedRenderSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);
    EnforceRenderInvariants();
}

void UGildedRenderSubsystem::Deinitialize() {
    Super::Deinitialize();
}

void UGildedRenderSubsystem::Tick(float DeltaTime) {
    // continuously enforce these so level sequences or post process volumes don't override the aesthetic
    EnforceRenderInvariants();
}

TStatId UGildedRenderSubsystem::GetStatId() const {
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGildedRenderSubsystem, STATGROUP_Tickables);
}

void UGildedRenderSubsystem::EnforceRenderInvariants() {
    // lock dynamic global illumination (No Lumen)
    if (IConsoleVariable *CVarGI = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DynamicGlobalIlluminationMethod"))) {
        CVarGI->Set(0, ECVF_SetByCode);
    }

    // lock reflection method (No Lumen Reflections)
    if (IConsoleVariable *CVarRefl = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ReflectionMethod"))) {
        CVarRefl->Set(0, ECVF_SetByCode);
    }

    // lock anti-aliasing method (No Default TAA or TSR)
    if (IConsoleVariable *CVarAA = IConsoleManager::Get().FindConsoleVariable(TEXT("r.AntiAliasingMethod"))) {
        CVarAA->Set(0, ECVF_SetByCode);
    }

    // disable auto-exposure completely
    if (IConsoleVariable *CVarExp = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DefaultFeature.AutoExposure"))) {
        CVarExp->Set(0, ECVF_SetByCode);
    }
    if (IConsoleVariable *CVarEye = IConsoleManager::Get().FindConsoleVariable(TEXT("r.EyeAdaptationQuality"))) {
        CVarEye->Set(0, ECVF_SetByCode);
    }

    // disable chromatic aberration globally
    if (IConsoleVariable *CVarCA = IConsoleManager::Get().FindConsoleVariable(TEXT("r.SceneColorFringeQuality"))) {
        CVarCA->Set(0, ECVF_SetByCode);
    }

    // disable lens flares
    if (IConsoleVariable *CVarLF = IConsoleManager::Get().FindConsoleVariable(TEXT("r.LensFlareQuality"))) {
        CVarLF->Set(0, ECVF_SetByCode);
    }
}
