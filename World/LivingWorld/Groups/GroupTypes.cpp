
#include "World/LivingWorld/Groups/GroupTypes.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"

namespace MythicGroupDefaults {
    void BuildDefaultTemplates(TArray<FMythicGroupTemplate> &Out) {
        Out.Reset();

        {
            FMythicGroupTemplate &T = Out.AddDefaulted_GetRef();
            T.GroupTag = TAG_NPC_GROUP_RETINUE;
            T.DisplayName = FText::FromString(TEXT("Noble Retinue"));
            T.IntraRelation = EMythicSocialRelation::Subordinate;
            T.IntraEdgeStrength = 0.7f;
            T.MinFactionMilitaryStrength = 0.4f;
            T.MinFactionPopulation = 30;
            T.MinReserveWealth = 50.0f;
            T.RelativeWeight = 1.0f;

            FMythicGroupMemberSpec Noble;
            Noble.RoleTag = TAG_NPC_ROLE_NOBLE;
            Noble.MinCount = 1;
            Noble.MaxCount = 1;
            Noble.bIsLeader = true;
            T.Members.Add(Noble);

            FMythicGroupMemberSpec Guards;
            Guards.RoleTag = TAG_NPC_ROLE_GUARD;
            Guards.MinCount = 2;
            Guards.MaxCount = 2;
            Guards.bIsLeader = false;
            T.Members.Add(Guards);
        }

        {
            FMythicGroupTemplate &T = Out.AddDefaulted_GetRef();
            T.GroupTag = TAG_NPC_GROUP_BARTER;
            T.DisplayName = FText::FromString(TEXT("Barter Party"));
            T.IntraRelation = EMythicSocialRelation::Associate;
            T.IntraEdgeStrength = 0.5f;
            T.MinFactionMilitaryStrength = 0.0f;
            T.MinFactionPopulation = 10;
            T.MinReserveWealth = 10.0f;
            T.RelativeWeight = 1.5f;

            FMythicGroupMemberSpec Merchant;
            Merchant.RoleTag = TAG_NPC_ROLE_MERCHANT;
            Merchant.MinCount = 1;
            Merchant.MaxCount = 1;
            Merchant.bIsLeader = true;
            T.Members.Add(Merchant);

            FMythicGroupMemberSpec Porters;
            Porters.RoleTag = TAG_NPC_ROLE_LABORER;
            Porters.MinCount = 2;
            Porters.MaxCount = 3;
            Porters.bIsLeader = false;
            T.Members.Add(Porters);
        }

        {
            FMythicGroupTemplate &T = Out.AddDefaulted_GetRef();
            T.GroupTag = TAG_NPC_GROUP_SOCIAL;
            T.DisplayName = FText::FromString(TEXT("Friends"));
            T.IntraRelation = EMythicSocialRelation::Friend;
            T.IntraEdgeStrength = 0.6f;
            T.MinFactionMilitaryStrength = 0.0f;
            T.MinFactionPopulation = 0;
            T.MinReserveWealth = 0.0f;
            T.RelativeWeight = 1.0f;

            FMythicGroupMemberSpec Friends;
            Friends.RoleTag = TAG_NPC_ROLE_CIVILIAN;
            Friends.MinCount = 3;
            Friends.MaxCount = 3;
            Friends.bIsLeader = false;
            T.Members.Add(Friends);
        }
    }
}
