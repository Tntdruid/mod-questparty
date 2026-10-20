#include "Config.h"
#include "GameObject.h"
#include "Group.h"
#include "LootMgr.h"
#include "Player.h"
#include "ScriptMgr.h"

namespace
{
    bool sEnabled = true;

    // Allowed looters who have the quest item in their loot list and have not looted it yet.
    uint8 CountUnlootedHolders(Loot const* loot, LootItem const& item, uint32 index)
    {
        uint8 holders = 0;
        QuestItemMap const& playerQuestItems = loot->GetPlayerQuestItems();

        for (ObjectGuid const& guid : item.GetAllowedLooters())
        {
            auto itr = playerQuestItems.find(guid);
            if (itr == playerQuestItems.end())
                continue;

            for (QuestItem const& questItem : *itr->second)
                if (questItem.index == index && !questItem.is_looted)
                    ++holders;
        }

        return holders;
    }
}

class QuestPartyWorld : public WorldScript
{
public:
    QuestPartyWorld() : WorldScript("QuestPartyWorld", { WORLDHOOK_ON_AFTER_CONFIG_LOAD }) {}

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sEnabled = sConfigMgr->GetOption<bool>("QuestParty.Enable", true);
    }
};

class QuestPartyLoot : public PlayerScript
{
public:
    // Only register the hook we use; an empty list enables every PlayerScript hook.
    QuestPartyLoot() : PlayerScript("QuestPartyLoot", { PLAYERHOOK_ON_BEFORE_SEND_LOOT }) {}

    void OnPlayerBeforeSendLoot(Player* player, ObjectGuid lootGuid, Loot* loot) override
    {
        if (!sEnabled || !player || !loot || !lootGuid.IsGameObject() || !player->IsInWorld())
            return;

        Group* group = player->GetGroup();
        if (!group)
            return;

        GameObject* gameObject = player->GetMap()->GetGameObject(lootGuid);
        if (!gameObject || !gameObject->IsInWorld())
            return;

        // Make game object quest items personal. Items handled by group loot rules
        // (rolls, master loot) are left alone, since rolls keep references to them.
        for (uint32 i = 0; i < loot->quest_items.size(); ++i)
        {
            LootItem& item = loot->quest_items[i];
            if (item.freeforall || item.is_looted || item.follow_loot_rules || item.is_blocked || item.rollWinnerGUID)
                continue;

            item.freeforall = true;

            // Players who already got the item counted it once as a shared item.
            // Free-for-all items count once per player, so add the missing ones,
            // otherwise the first looter empties the object for everyone.
            uint8 holders = CountUnlootedHolders(loot, item, i);
            if (holders > 1)
                loot->unlootedCount += holders - 1;
        }

        for (GroupReference* memberRef = group->GetFirstMember(); memberRef; memberRef = memberRef->next())
        {
            Player* member = memberRef->GetSource();
            if (!member || member == player || !member->IsInWorld() || member->IsBeingTeleported())
                continue;

            if (member->IsAtLootRewardDistance(gameObject))
                loot->FillNotNormalLootFor(member);
        }
    }
};

void AddQuestPartyScripts()
{
    new QuestPartyWorld();
    new QuestPartyLoot();
}
