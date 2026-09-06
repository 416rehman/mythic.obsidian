#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "GAS/Abilities/MythicAbilityIntentTargetData.h"
#include "Serialization/BitReader.h"
#include "Serialization/BitWriter.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicAbilityIntentWireTest,"Mythic.Combat.Abilities.IntentWire",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FMythicAbilityIntentWireTest::RunTest(const FString&) {
    const auto RoundTrip=[this](const FVector& Movement) {
        FMythicAbilityIntentTargetData Source,Loaded;
        Source.AimDirection=FVector(2,3,-1).GetSafeNormal(); Source.MovementDirection=Movement;
        FBitWriter Writer(256,true); bool bSaved=false;
        Source.NetSerialize(Writer,nullptr,bSaved);
        TestTrue(TEXT("valid intent serializes without error"),bSaved && !Writer.IsError());
        TestTrue(TEXT("two direction vectors remain bounded on the wire"),Writer.GetNumBits()<=128);
        FBitReader Reader(Writer.GetData(),Writer.GetNumBits()); bool bRead=false;
        Loaded.NetSerialize(Reader,nullptr,bRead);
        TestTrue(TEXT("decoded intent is valid"),bRead && !Reader.IsError() && Loaded.HasValidDirections());
        TestTrue(TEXT("camera aim round trips independently of movement"),FVector(Loaded.AimDirection).Equals(Source.AimDirection,0.0001));
        TestTrue(TEXT("movement preserves lateral and neutral intent"),FVector(Loaded.MovementDirection).Equals(Movement,0.0001));
    };
    RoundTrip(FVector::ZeroVector); RoundTrip(FVector::RightVector); RoundTrip(FVector(-1,1,0).GetSafeNormal());
    FMythicAbilityIntentTargetData Invalid;
    Invalid.AimDirection=FVector::ZeroVector;
    TestFalse(TEXT("zero aim is rejected"),Invalid.HasValidDirections());
    Invalid.AimDirection=FVector(0.5,0,0);
    TestFalse(TEXT("short aim cannot shorten server targeting"),Invalid.HasValidDirections());
    Invalid.AimDirection=FVector::ForwardVector; Invalid.MovementDirection=FVector(0,0,1);
    TestFalse(TEXT("vertical movement intent cannot request flight"),Invalid.HasValidDirections());
    Invalid.MovementDirection=FVector(0.4,0,0);
    TestFalse(TEXT("partial magnitude cannot alter utility distance"),Invalid.HasValidDirections());
    Invalid.MovementDirection=FVector::ZeroVector;
    FBitWriter Writer(256,true); bool bSaved=false; Invalid.NetSerialize(Writer,nullptr,bSaved);
    FBitReader Truncated(Writer.GetData(),Writer.GetNumBits()-10); bool bRead=false;
    {
        auto* EnsureOverflow=IConsoleManager::Get().FindConsoleVariable(TEXT("net.BitReader.EnsureOnOverflow"));
        auto* FatalOverflow=IConsoleManager::Get().FindConsoleVariable(TEXT("BitReader.LogFatalOnOverflow"));
        if (!TestTrue(TEXT("engine exposes scoped malformed-packet diagnostics"),EnsureOverflow && FatalOverflow)) return false;
        const int32 PreviousEnsure=EnsureOverflow->GetInt(),PreviousFatal=FatalOverflow->GetInt();
        const auto EnsurePriority=static_cast<EConsoleVariableFlags>(EnsureOverflow->GetFlags() & ECVF_SetByMask);
        const auto FatalPriority=static_cast<EConsoleVariableFlags>(FatalOverflow->GetFlags() & ECVF_SetByMask);
        ON_SCOPE_EXIT {
            EnsureOverflow->Set(PreviousEnsure,EnsurePriority);
            FatalOverflow->Set(PreviousFatal,FatalPriority);
        };
        EnsureOverflow->Set(0,EnsurePriority);
        FatalOverflow->Set(0,FatalPriority);
        AddExpectedError(TEXT("FBitReader::SetOverflowed"),EAutomationExpectedErrorFlags::Contains,1);
        FMythicAbilityIntentTargetData Loaded; Loaded.NetSerialize(Truncated,nullptr,bRead);
    }
    TestTrue(TEXT("truncated direction payload sets archive error"),Truncated.IsError());
    TestFalse(TEXT("truncated direction payload cannot be accepted"),bRead);
    // Exercise GAS's derived-struct cache and target-data handle transport as well.
    // Iris uses reflected fields instead; authority always validates the decoded struct.
    const auto HandleRoundTrip=[this](bool bValid) {
        FGameplayAbilityTargetDataHandle Source,LoadedHandle;
        auto* Intent=new FMythicAbilityIntentTargetData();
        Intent->AimDirection=bValid ? FVector::ForwardVector : FVector(0.5,0,0);
        Intent->MovementDirection=FVector::RightVector; Source.Add(Intent);
        FBitWriter HandleWriter(1024,true); bool bHandleSaved=false;
        Source.NetSerialize(HandleWriter,nullptr,bHandleSaved);
        TestFalse(TEXT("bounded GAS handle writes without archive failure"),HandleWriter.IsError());
        FBitReader HandleReader(HandleWriter.GetData(),HandleWriter.GetNumBits()); bool bHandleRead=false;
        LoadedHandle.NetSerialize(HandleReader,nullptr,bHandleRead);
        if (!TestTrue(TEXT("GAS restores exactly one typed intent"),!HandleReader.IsError() && LoadedHandle.Num()==1
            && LoadedHandle.Get(0) && LoadedHandle.Get(0)->GetScriptStruct()==FMythicAbilityIntentTargetData::StaticStruct())) return;
        const auto* Decoded=static_cast<const FMythicAbilityIntentTargetData*>(LoadedHandle.Get(0));
        TestEqual(TEXT("authority validation rejects malformed directions even when GAS accepts its handle"),Decoded->HasValidDirections(),bValid);
        TestTrue(TEXT("lateral movement survives the GAS handle"),FVector(Decoded->MovementDirection).Equals(FVector::RightVector,0.0001));
    };
    HandleRoundTrip(true); HandleRoundTrip(false);
    return true;
}
#endif
