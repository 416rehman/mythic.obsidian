
#include "Narrative/Dialogue/MythicDialogueJson.h"

#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

namespace MythicDialogueJsonPrivate {
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

void ReadChoice(const TSharedPtr<FJsonObject> &Obj, FMythicDialogueChoiceSpec &Out) {
    Out = FMythicDialogueChoiceSpec();
    Obj->TryGetStringField(TEXT("text"), Out.Text);
    ReadCondition(Obj, TEXT("condition"), Out.Condition);
    ReadStringArray(Obj, TEXT("grantTags"), Out.GrantTags);
    ReadRewards(Obj, TEXT("rewards"), Out.Rewards);
    Obj->TryGetStringField(TEXT("questOfferId"), Out.QuestOfferId);
    Obj->TryGetStringField(TEXT("storylineOfferId"), Out.StorylineOfferId);
    Obj->TryGetStringField(TEXT("gotoNodeId"), Out.GotoNodeId);
    Obj->TryGetBoolField(TEXT("endsDialogue"), Out.bEndsDialogue);
}

bool ReadNode(const TSharedPtr<FJsonObject> &Obj, FMythicDialogueNodeSpec &Out) {
    Out = FMythicDialogueNodeSpec();
    if (!Obj->TryGetStringField(TEXT("id"), Out.Id) || Out.Id.IsEmpty()) {
        return false;
    }
    Obj->TryGetStringField(TEXT("speaker"), Out.Speaker);
    Obj->TryGetStringField(TEXT("line"), Out.Line);
    ReadCondition(Obj, TEXT("entryCondition"), Out.EntryCondition);

    const TArray<TSharedPtr<FJsonValue>> *ChoiceArr = nullptr;
    if (Obj->TryGetArrayField(TEXT("choices"), ChoiceArr) && ChoiceArr) {
        for (const TSharedPtr<FJsonValue> &V : *ChoiceArr) {
            const TSharedPtr<FJsonObject> *CObj = nullptr;
            if (V.IsValid() && V->TryGetObject(CObj) && CObj && CObj->IsValid()) {
                FMythicDialogueChoiceSpec C;
                ReadChoice(*CObj, C);
                Out.Choices.Add(MoveTemp(C));
            }
        }
    }
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

TSharedRef<FJsonObject> WriteChoice(const FMythicDialogueChoiceSpec &C) {
    TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
    O->SetStringField(TEXT("text"), C.Text);
    O->SetObjectField(TEXT("condition"), WriteCondition(C.Condition));
    O->SetArrayField(TEXT("grantTags"), MakeStringArray(C.GrantTags));
    O->SetObjectField(TEXT("rewards"), WriteRewards(C.Rewards));
    O->SetStringField(TEXT("questOfferId"), C.QuestOfferId);
    O->SetStringField(TEXT("storylineOfferId"), C.StorylineOfferId);
    O->SetStringField(TEXT("gotoNodeId"), C.GotoNodeId);
    O->SetBoolField(TEXT("endsDialogue"), C.bEndsDialogue);
    return O;
}

TSharedRef<FJsonObject> WriteNode(const FMythicDialogueNodeSpec &N) {
    TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
    O->SetStringField(TEXT("id"), N.Id);
    O->SetStringField(TEXT("speaker"), N.Speaker);
    O->SetStringField(TEXT("line"), N.Line);
    O->SetObjectField(TEXT("entryCondition"), WriteCondition(N.EntryCondition));
    TArray<TSharedPtr<FJsonValue>> ChoiceVals;
    ChoiceVals.Reserve(N.Choices.Num());
    for (const FMythicDialogueChoiceSpec &C : N.Choices) {
        ChoiceVals.Add(MakeShared<FJsonValueObject>(WriteChoice(C)));
    }
    O->SetArrayField(TEXT("choices"), ChoiceVals);
    return O;
}
}


bool FMythicDialogueJson::IsDialogueDocument(const FString &Json) {
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) {
        return false;
    }
    FString Kind;
    if (Root->TryGetStringField(TEXT("kind"), Kind) && Kind.Equals(TEXT("dialogue"), ESearchCase::IgnoreCase)) {
        return true;
    }
    const TArray<TSharedPtr<FJsonValue>> *Nodes = nullptr;
    return Root->TryGetArrayField(TEXT("nodes"), Nodes);
}

bool FMythicDialogueJson::ParseGraphSpec(const FString &Json, FMythicDialogueGraphSpec &Out) {
    Out = FMythicDialogueGraphSpec();
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) {
        return false;
    }
    if (!Root->TryGetStringField(TEXT("id"), Out.Id) || Out.Id.IsEmpty()) {
        Out = FMythicDialogueGraphSpec();
        return false;
    }
    Root->TryGetStringField(TEXT("npcTag"), Out.NpcTag);
    Root->TryGetStringField(TEXT("role"), Out.Role);
    Root->TryGetStringField(TEXT("faction"), Out.Faction);
    Root->TryGetStringField(TEXT("entryNodeId"), Out.EntryNodeId);

    const TArray<TSharedPtr<FJsonValue>> *NodeArr = nullptr;
    if (Root->TryGetArrayField(TEXT("nodes"), NodeArr) && NodeArr) {
        for (const TSharedPtr<FJsonValue> &V : *NodeArr) {
            const TSharedPtr<FJsonObject> *NObj = nullptr;
            if (V.IsValid() && V->TryGetObject(NObj) && NObj && NObj->IsValid()) {
                FMythicDialogueNodeSpec N;
                if (!MythicDialogueJsonPrivate::ReadNode(*NObj, N)) {
                    Out = FMythicDialogueGraphSpec();
                    return false;
                }
                Out.Nodes.Add(MoveTemp(N));
            }
        }
    }
    return true;
}

FString FMythicDialogueJson::SerializeGraphSpec(const FMythicDialogueGraphSpec &Spec) {
    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("kind"), TEXT("dialogue"));
    Root->SetStringField(TEXT("id"), Spec.Id);
    Root->SetStringField(TEXT("npcTag"), Spec.NpcTag);
    Root->SetStringField(TEXT("role"), Spec.Role);
    Root->SetStringField(TEXT("faction"), Spec.Faction);
    Root->SetStringField(TEXT("entryNodeId"), Spec.EntryNodeId);
    TArray<TSharedPtr<FJsonValue>> NodeVals;
    NodeVals.Reserve(Spec.Nodes.Num());
    for (const FMythicDialogueNodeSpec &N : Spec.Nodes) {
        NodeVals.Add(MakeShared<FJsonValueObject>(MythicDialogueJsonPrivate::WriteNode(N)));
    }
    Root->SetArrayField(TEXT("nodes"), NodeVals);

    FString Out;
    const TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
        TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Out);
    FJsonSerializer::Serialize(Root, Writer);
    return Out;
}
