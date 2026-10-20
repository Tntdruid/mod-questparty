# mod-questparty

`mod-questparty` lets eligible party members loot their own quest items from the same game object.

When a grouped player opens a game object's loot, the module prepares personal quest loot for the online members of the group. AzerothCore checks each member's quest eligibility, so members who cannot loot a quest item do not receive it. Quest loot is free-for-all for grouped players, allowing each eligible member to loot their own copy before the game object disappears.

Players must still open the game object and loot the item themselves. The module does not automatically place items in players' bags, and it does not change quest-item loot for players who are not in a group.

## Configuration

The feature is enabled by default. Set the following option in the worldserver configuration to disable it:

```ini
QuestParty.Enable = 0
```

The distributed configuration template is `conf/mod_questparty.conf.dist`.