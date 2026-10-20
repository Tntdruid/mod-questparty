#include "Config.h"
#include "GameObject.h"
#include "Group.h"
#include "Log.h"
#include "LootMgr.h"
#include "Player.h"
#include "ScriptMgr.h"

namespace
{
    bool sEnabled = true;
    bool sDebug = false;

    // Debug lines go to the "module" logger when QuestParty.Debug = 1.
#define QP_DEBUG(...) do { if (sDebug) LOG_INFO("module", "[QuestParty] " __VA_ARGS__); } while (0)

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
        sDebug = sConfigMgr->GetOption<bool>("QuestParty.Debug", false);
    }
};

class QuestPartyLoot : public PlayerScript
{
public:
    // Only register the hooks we use; an empty list enables every PlayerScript hook.
    QuestPartyLoot() : PlayerScript("QuestPartyLoot", { PLAYERHOOK_ON_BEFORE_SEND_LOOT, PLAYERHOOK_ON_LOOT_ITEM }) {}

    void OnPlayerBeforeSendLoot(Player* player, ObjectGuid lootGuid, Loot* loot) override
    {
        if (!sEnabled || !player || !loot || !lootGuid.IsGameObject())
            return;

        if (!player->IsInWorld())
        {
            QP_DEBUG("{} opened {}: skipped, player not in world", player->GetName(), lootGuid.ToString());
            return;
        }

        Group* group = player->GetGroup();
        if (!group)
        {
            QP_DEBUG("{} opened {}: skipped, player not in a group", player->GetName(), lootGuid.ToString());
            return;
        }

        GameObject* gameObject = player->GetMap()->GetGameObject(lootGuid);
        if (!gameObject || !gameObject->IsInWorld())
        {
            QP_DEBUG("{} opened {}: skipped, game object not found", player->GetName(), lootGuid.ToString());
            return;
        }

        QP_DEBUG("{} opened '{}' (entry {}, lootState {}, groupLootRules {}): unlootedCount {}, {} quest items",
            player->GetName(), gameObject->GetName(), gameObject->GetEntry(), uint32(gameObject->getLootState()),
            gameObject->GetGoType() == GAMEOBJECT_TYPE_CHEST && gameObject->GetGOInfo()->chest.groupLootRules,
            uint32(loot->unlootedCount), loot->quest_items.size());

        // Make game object quest items personal. Items handled by group loot rules
        // (rolls, master loot) are left alone, since rolls keep references to them.
        for (uint32 i = 0; i < loot->quest_items.size(); ++i)
        {
            LootItem& item = loot->quest_items[i];

            QP_DEBUG("  quest item {} (slot {}): freeforall {}, looted {}, blocked {}, followLootRules {}, rollWinner {}, allowedLooters {}",
                item.itemid, i, item.freeforall, item.is_looted, item.is_blocked, item.follow_loot_rules,
                bool(item.rollWinnerGUID), item.GetAllowedLooters().size());

            if (item.freeforall || item.is_looted || item.follow_loot_rules || item.is_blocked || item.rollWinnerGUID)
                continue;

            item.freeforall = true;

            // Players who already got the item counted it once as a shared item.
            // Free-for-all items count once per player, so add the missing ones,
            // otherwise the first looter empties the object for everyone.
            uint8 holders = CountUnlootedHolders(loot, item, i);
            if (holders > 1)
                loot->unlootedCount += holders - 1;

            QP_DEBUG("  quest item {} made free-for-all: {} holders, unlootedCount now {}",
                item.itemid, uint32(holders), uint32(loot->unlootedCount));
        }

        for (GroupReference* memberRef = group->GetFirstMember(); memberRef; memberRef = memberRef->next())
        {
            Player* member = memberRef->GetSource();
            if (!member || member == player)
                continue;

            if (!member->IsInWorld() || member->IsBeingTeleported())
            {
                QP_DEBUG("  member {}: skipped, not in world or teleporting", member->GetName());
                continue;
            }

            if (!member->IsAtLootRewardDistance(gameObject))
            {
                QP_DEBUG("  member {}: skipped, out of loot range ({:.1f} yards)", member->GetName(), member->GetDistance(gameObject));
                continue;
            }

            bool alreadyFilled = loot->GetPlayerQuestItems().count(member->GetGUID()) > 0;
            loot->FillNotNormalLootFor(member);

            auto itr = loot->GetPlayerQuestItems().find(member->GetGUID());
            QP_DEBUG("  member {}: {}, {} quest items for them",
                member->GetName(), alreadyFilled ? "already filled" : "filled now",
                itr != loot->GetPlayerQuestItems().end() ? itr->second->size() : 0);
        }

        QP_DEBUG("  done: unlootedCount {}", uint32(loot->unlootedCount));
    }

    void OnPlayerLootItem(Player* player, Item* item, uint32 count, ObjectGuid lootGuid) override
    {
        if (!sEnabled || !sDebug || !player || !item || !lootGuid.IsGameObject())
            return;

        GameObject* gameObject = player->GetMap()->GetGameObject(lootGuid);
        if (!gameObject)
            return;

        QP_DEBUG("{} looted {}x item {} from '{}': unlootedCount now {}",
            player->GetName(), count, item->GetEntry(), gameObject->GetName(), uint32(gameObject->loot.unlootedCount));
    }
};

void AddQuestPartyScripts()
{
    new QuestPartyWorld();
    new QuestPartyLoot();
}
