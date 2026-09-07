#include "MythicAppearanceTypes.h"

bool FMythicItemAppearanceData::HasSameColors(const FMythicItemAppearanceData& Other) const {
    auto A=Colors, B=Other.Colors;
    return NormalizeColors(A) && NormalizeColors(B) && A==B;
}

bool FMythicItemAppearanceData::NormalizeColors(TArray<FMythicAppearanceColor>& Values) {
    if (Values.Num()>MythicAppearanceLimits::Channels) return false;
    TSet<FName> Seen;
    for (const auto& C:Values) {
        if (C.Channel.IsNone() || C.Channel.ToString().Len()>MythicAppearanceLimits::IdLength
            || C.Palette.ToString().Len()>MythicAppearanceLimits::IdLength || Seen.Contains(C.Channel)) return false;
        Seen.Add(C.Channel);
    }
    Values.RemoveAll([](const auto& C){return C.Palette.IsNone();});
    Values.Sort([](const auto& A,const auto& B){return A.Channel.LexicalLess(B.Channel);});
    return true;
}

bool FMythicAppearanceRecipe::operator==(const FMythicAppearanceRecipe& Other) const {
    return Frame==Other.Frame && Head==Other.Head && Hair==Other.Hair && SkinPalette==Other.SkinPalette
        && HairPalette==Other.HairPalette && EyePalette==Other.EyePalette;
}

bool FMythicAppearanceEditRequest::IsBounded() const {
    const auto ValidId=[](FName Id) { return Id.ToString().Len()<=MythicAppearanceLimits::IdLength; };
    if (!RequestId.IsValid() || !Session.IsValid() || Items.Num()>MythicAppearanceLimits::ItemEdits) return false;
    if (!ValidId(Personal.Frame) || !ValidId(Personal.Head) || !ValidId(Personal.Hair)
        || !ValidId(Personal.SkinPalette) || !ValidId(Personal.HairPalette) || !ValidId(Personal.EyePalette)) return false;
    TSet<FGuid> Seen;
    for (const auto& Item:Items) {
        if (!Item.ItemGuid.IsValid() || Seen.Contains(Item.ItemGuid) || Item.Changes.Num()>MythicAppearanceLimits::Channels) return false;
        Seen.Add(Item.ItemGuid);
        TSet<FName> Channels;
        for (const auto& Color:Item.Changes) {
            if (Color.Channel.IsNone() || !ValidId(Color.Channel) || !ValidId(Color.Palette) || Channels.Contains(Color.Channel)) return false;
            Channels.Add(Color.Channel);
        }
    }
    return true;
}
