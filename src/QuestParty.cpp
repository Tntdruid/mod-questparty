#include "Config.h"
#include "GameObject.h"
#include "Group.h"
#include "LootMgr.h"
#include "Player.h"
#include "ScriptMgr.h"

namespace
{
    bool sEnabled = true;
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
        for (LootItem& item : loot->quest_items)
            if (!item.is_looted && !item.follow_loot_rules && !item.is_blocked && !item.rollWinnerGUID)
                item.freeforall = true;

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
