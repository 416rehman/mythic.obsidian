
#include "Narrative/MythicNarrativeJson.h"

#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

namespace {
void ReadStringArray(const TSharedPtr<FJsonObject> &Obj, const TCHAR *Field, TArray<FString> &Out) {
    Out.Reset();
    const TArray<TSharedPtr<FJsonValue>> *Arr = nullptr;
    if (Obj->TryGetArrayField(Field, Arr) && Arr) {
        for (const TSharedPtr<FJsonValue> &V : *Arr) {
            FString S;
            if (V.IsValid() && V->TryGetString(S)) {
                Out.Add(S);
            }
        }
    }
}

void ReadCondition(const TSharedPtr<FJsonObject> &Parent, const TCHAR *Field, FMythicStoryConditionSpec &Out) {
    Out = FMythicStoryConditionSpec();
    const TSharedPtr<FJsonObject> *CondObj = nullptr;
    if (Parent->TryGetObjectField(Field, CondObj) && CondObj && CondObj->IsValid()) {
        ReadStringArray(*CondObj, TEXT("requireAll"), Out.RequireAll);
        ReadStringArray(*CondObj, TEXT("requireAny"), Out.RequireAny);
        ReadStringArray(*CondObj, TEXT("blockAny"), Out.BlockAny);
    }
}

void ReadRewards(const TSharedPtr<FJsonObject> &Parent, const TCHAR *Field, FMythicRewardsSpec &Out) {
    Out = FMythicRewardsSpec();
    const TSharedPtr<FJsonObject> *RObj = nullptr;
    if (Parent->TryGetObjectField(Field, RObj) && RObj && RObj->IsValid()) {
        double XpPct = 0.0;
        if ((*RObj)->TryGetNumberField(TEXT("xpPercentage"), XpPct)) {
            Out.XpPercentage = static_cast<float>(XpPct);
        }
        (*RObj)->TryGetStringField(TEXT("xpProficiency"), Out.XpProficiency);
        (*RObj)->TryGetStringField(TEXT("itemId"), Out.ItemId);
        int32 Qty = 0;
        if ((*RObj)->TryGetNumberField(TEXT("itemQuantity"), Qty)) {
            Out.ItemQuantity = Qty;
        }
    }
}

void ReadBranch(const TSharedPtr<FJsonObject> &BObj, FMythicBranchSpec &Out) {
    Out = FMythicBranchSpec();
    BObj->TryGetStringField(TEXT("outcome"), Out.Outcome);
    ReadStringArray(BObj, TEXT("grantFlags"), Out.GrantFlags);
    ReadStringArray(BObj, TEXT("next"), Out.Next);
    ReadStringArray(BObj, TEXT("cancel"), Out.Cancel);
}

bool ReadTask(const TSharedPtr<FJsonObject> &Obj, FMythicTaskSpec &Out) {
    Out = FMythicTaskSpec();
    if (!Obj->TryGetStringField(TEXT("id"), Out.Id) || Out.Id.IsEmpty()) {
        return false;
    }
    Obj->TryGetStringField(TEXT("display"), Out.Display);
    Obj->TryGetStringField(TEXT("trigger"), Out.TriggerTag);
    Obj->TryGetStringField(TEXT("payload"), Out.PayloadTag);
    int32 Count = 1;
    if (Obj->TryGetNumberField(TEXT("count"), Count)) {
        Out.Count = Count;
    }
    Obj->TryGetBoolField(TEXT("optional"), Out.bOptional);
    ReadCondition(Obj, TEXT("precondition"), Out.Precondition);
    ReadStringArray(Obj, TEXT("grantStoryTags"), Out.GrantStoryTags);

    const TArray<TSharedPtr<FJsonValue>> *BranchArr = nullptr;
    if (Obj->TryGetArrayField(TEXT("branches"), BranchArr) && BranchArr) {
        for (const TSharedPtr<FJsonValue> &V : *BranchArr) {
            const TSharedPtr<FJsonObject> *BObj = nullptr;
            if (V.IsValid() && V->TryGetObject(BObj) && BObj && BObj->IsValid()) {
                FMythicBranchSpec B;
                ReadBranch(*BObj, B);
                Out.Branches.Add(MoveTemp(B));
            }
        }
    }
    ReadStringArray(Obj, TEXT("next"), Out.Next);
    return true;
}

void ReadOutcome(const TSharedPtr<FJsonObject> &Obj, FMythicOutcomeSpec &Out) {
    Out = FMythicOutcomeSpec();
    Obj->TryGetStringField(TEXT("outcome"), Out.Outcome);
    ReadCondition(Obj, TEXT("when"), Out.When);
    ReadRewards(Obj, TEXT("rewards"), Out.Rewards);
    ReadStringArray(Obj, TEXT("grantStoryTags"), Out.GrantStoryTags);
}

bool ReadQuest(const TSharedPtr<FJsonObject> &Obj, FMythicQuestSpec &Out) {
    Out = FMythicQuestSpec();
    if (!Obj->TryGetStringField(TEXT("id"), Out.Id) || Out.Id.IsEmpty()) {
        return false;
    }
    Obj->TryGetStringField(TEXT("display"), Out.Display);
    ReadCondition(Obj, TEXT("unlockCondition"), Out.UnlockCondition);
    ReadStringArray(Obj, TEXT("exclusiveLockTags"), Out.ExclusiveLockTags);
    Obj->TryGetBoolField(TEXT("optional"), Out.bOptional);

    const TArray<TSharedPtr<FJsonValue>> *TaskArr = nullptr;
    if (Obj->TryGetArrayField(TEXT("tasks"), TaskArr) && TaskArr) {
        for (const TSharedPtr<FJsonValue> &V : *TaskArr) {
            const TSharedPtr<FJsonObject> *TObj = nullptr;
            if (V.IsValid() && V->TryGetObject(TObj) && TObj && TObj->IsValid()) {
                FMythicTaskSpec T;
                if (!ReadTask(*TObj, T)) {
                    return false;
                }
                Out.Tasks.Add(MoveTemp(T));
            }
        }
    }

    const TArray<TSharedPtr<FJsonValue>> *OutArr = nullptr;
    if (Obj->TryGetArrayField(TEXT("outcomes"), OutArr) && OutArr) {
        for (const TSharedPtr<FJsonValue> &V : *OutArr) {
            const TSharedPtr<FJsonObject> *OObj = nullptr;
            if (V.IsValid() && V->TryGetObject(OObj) && OObj && OObj->IsValid()) {
                FMythicOutcomeSpec Oc;
                ReadOutcome(*OObj, Oc);
                Out.Outcomes.Add(MoveTemp(Oc));
            }
        }
    }

    ReadRewards(Obj, TEXT("rewards"), Out.Rewards);
    ReadStringArray(Obj, TEXT("grantStoryTags"), Out.GrantStoryTags);
    return true;
}


TArray<TSharedPtr<FJsonValue>> MakeStringArray(const TArray<FString> &In) {
    TArray<TSharedPtr<FJsonValue>> Out;
    Out.Reserve(In.Num());
    for (const FString &S : In) {
        Out.Add(MakeShared<FJsonValueString>(S));
    }
    return Out;
}

TSharedRef<FJsonObject> WriteCondition(const FMythicStoryConditionSpec &C) {
    TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
    O->SetArrayField(TEXT("requireAll"), MakeStringArray(C.RequireAll));
    O->SetArrayField(TEXT("requireAny"), MakeStringArray(C.RequireAny));
    O->SetArrayField(TEXT("blockAny"), MakeStringArray(C.BlockAny));
    return O;
}

TSharedRef<FJsonObject> WriteRewards(const FMythicRewardsSpec &R) {
    TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
    O->SetNumberField(TEXT("xpPercentage"), R.XpPercentage);
    O->SetStringField(TEXT("xpProficiency"), R.XpProficiency);
    O->SetStringField(TEXT("itemId"), R.ItemId);
    O->SetNumberField(TEXT("itemQuantity"), R.ItemQuantity);
    return O;
}

TSharedRef<FJsonObject> WriteBranch(const FMythicBranchSpec &B) {
    TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
    O->SetStringField(TEXT("outcome"), B.Outcome);
    O->SetArrayField(TEXT("grantFlags"), MakeStringArray(B.GrantFlags));
    O->SetArrayField(TEXT("next"), MakeStringArray(B.Next));
    O->SetArrayField(TEXT("cancel"), MakeStringArray(B.Cancel));
    return O;
}

TSharedRef<FJsonObject> WriteTask(const FMythicTaskSpec &T) {
    TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
    O->SetStringField(TEXT("id"), T.Id);
    O->SetStringField(TEXT("display"), T.Display);
    O->SetStringField(TEXT("trigger"), T.TriggerTag);
    O->SetStringField(TEXT("payload"), T.PayloadTag);
    O->SetNumberField(TEXT("count"), T.Count);
    O->SetBoolField(TEXT("optional"), T.bOptional);
    O->SetObjectField(TEXT("precondition"), WriteCondition(T.Precondition));
    O->SetArrayField(TEXT("grantStoryTags"), MakeStringArray(T.GrantStoryTags));
    TArray<TSharedPtr<FJsonValue>> BranchVals;
    BranchVals.Reserve(T.Branches.Num());
    for (const FMythicBranchSpec &B : T.Branches) {
        BranchVals.Add(MakeShared<FJsonValueObject>(WriteBranch(B)));
    }
    O->SetArrayField(TEXT("branches"), BranchVals);
    O->SetArrayField(TEXT("next"), MakeStringArray(T.Next));
    return O;
}

TSharedRef<FJsonObject> WriteOutcome(const FMythicOutcomeSpec &Oc) {
    TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
    O->SetStringField(TEXT("outcome"), Oc.Outcome);
    O->SetObjectField(TEXT("when"), WriteCondition(Oc.When));
    O->SetObjectField(TEXT("rewards"), WriteRewards(Oc.Rewards));
    O->SetArrayField(TEXT("grantStoryTags"), MakeStringArray(Oc.GrantStoryTags));
    return O;
}

TSharedRef<FJsonObject> WriteQuest(const FMythicQuestSpec &Q) {
    TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
    O->SetStringField(TEXT("id"), Q.Id);
    O->SetStringField(TEXT("display"), Q.Display);
    O->SetObjectField(TEXT("unlockCondition"), WriteCondition(Q.UnlockCondition));
    O->SetArrayField(TEXT("exclusiveLockTags"), MakeStringArray(Q.ExclusiveLockTags));
    O->SetBoolField(TEXT("optional"), Q.bOptional);
    TArray<TSharedPtr<FJsonValue>> TaskVals;
    TaskVals.Reserve(Q.Tasks.Num());
    for (const FMythicTaskSpec &T : Q.Tasks) {
        TaskVals.Add(MakeShared<FJsonValueObject>(WriteTask(T)));
    }
    O->SetArrayField(TEXT("tasks"), TaskVals);
    TArray<TSharedPtr<FJsonValue>> OutcomeVals;
    OutcomeVals.Reserve(Q.Outcomes.Num());
    for (const FMythicOutcomeSpec &Oc : Q.Outcomes) {
        OutcomeVals.Add(MakeShared<FJsonValueObject>(WriteOutcome(Oc)));
    }
    O->SetArrayField(TEXT("outcomes"), OutcomeVals);
    O->SetObjectField(TEXT("rewards"), WriteRewards(Q.Rewards));
    O->SetArrayField(TEXT("grantStoryTags"), MakeStringArray(Q.GrantStoryTags));
    return O;
}

TSharedRef<FJsonObject> WriteStoryline(const FMythicStorylineSpec &S) {
    TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
    O->SetStringField(TEXT("id"), S.Id);
    O->SetStringField(TEXT("display"), S.Display);
    O->SetStringField(TEXT("arcTag"), S.ArcTag);
    O->SetObjectField(TEXT("arcGate"), WriteCondition(S.ArcGate));
    TArray<TSharedPtr<FJsonValue>> QuestVals;
    QuestVals.Reserve(S.Quests.Num());
    for (const FMythicQuestSpec &Q : S.Quests) {
        QuestVals.Add(MakeShared<FJsonValueObject>(WriteQuest(Q)));
    }
    O->SetArrayField(TEXT("quests"), QuestVals);
    O->SetObjectField(TEXT("rewards"), WriteRewards(S.Rewards));
    O->SetArrayField(TEXT("grantStoryTags"), MakeStringArray(S.GrantStoryTags));
    return O;
}
}


bool FMythicNarrativeJson::ParseTaskSpec(const FString &Json, FMythicTaskSpec &Out) {
    Out = FMythicTaskSpec();
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) {
        return false;
    }
    return ReadTask(Root, Out);
}

FString FMythicNarrativeJson::SerializeTaskSpec(const FMythicTaskSpec &Spec) {
    FString Out;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
    FJsonSerializer::Serialize(WriteTask(Spec), Writer);
    return Out;
}

bool FMythicNarrativeJson::ParseStorylineSpec(const FString &Json, FMythicStorylineSpec &Out) {
    Out = FMythicStorylineSpec();
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) {
        return false;
    }
    if (!Root->TryGetStringField(TEXT("id"), Out.Id) || Out.Id.IsEmpty()) {
        return false;
    }
    Root->TryGetStringField(TEXT("display"), Out.Display);
    Root->TryGetStringField(TEXT("arcTag"), Out.ArcTag);
    ReadCondition(Root, TEXT("arcGate"), Out.ArcGate);

    const TArray<TSharedPtr<FJsonValue>> *QuestArr = nullptr;
    if (Root->TryGetArrayField(TEXT("quests"), QuestArr) && QuestArr) {
        for (const TSharedPtr<FJsonValue> &V : *QuestArr) {
            const TSharedPtr<FJsonObject> *QObj = nullptr;
            if (V.IsValid() && V->TryGetObject(QObj) && QObj && QObj->IsValid()) {
                FMythicQuestSpec Q;
                if (!ReadQuest(*QObj, Q)) {
                    return false;
                }
                Out.Quests.Add(MoveTemp(Q));
            }
        }
    }
    ReadRewards(Root, TEXT("rewards"), Out.Rewards);
    ReadStringArray(Root, TEXT("grantStoryTags"), Out.GrantStoryTags);
    return true;
}

FString FMythicNarrativeJson::SerializeStorylineSpec(const FMythicStorylineSpec &Spec) {
    FString Out;
    const TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
        TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Out);
    FJsonSerializer::Serialize(WriteStoryline(Spec), Writer);
    return Out;
}
