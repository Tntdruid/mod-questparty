Author: Tntdruid
# mod-questparty

Author: Tntdruid

`mod-questparty` lets eligible party members loot their own quest items from the same game object.

When a grouped player opens a game object's loot, the module prepares personal quest loot for the online members of the group who are within loot range of the object. Members who are too far away do not get a copy until they come closer and open the object themselves. AzerothCore checks each member's quest eligibility, so members who cannot loot a quest item do not receive it. Quest loot is free-for-all for grouped players, allowing each eligible member to loot their own copy before the game object disappears.

Players must still open the game object and loot the item themselves. The module does not automatically place items in players' bags, and it does not change quest-item loot for players who are not in a group.

## Configuration

The feature is enabled by default. Set the following option in the worldserver configuration to disable it:

```ini
QuestParty.Enable = 0
```

To troubleshoot quest loot, set `QuestParty.Debug = 1`. The module then writes what happens on every open and loot of a game object to the `module` log (console and `Server.log` by default).

The distributed configuration template is `conf/mod_questparty.conf.dist`.

## Installation

Clone the module into the `modules` folder of your AzerothCore source, re-run CMake and rebuild:

```bash
cd azerothcore-wotlk/modules
git clone https://github.com/Tntdruid/mod-questparty.git
```

Copy `mod_questparty.conf.dist` to `mod_questparty.conf` in your server's module configuration folder if you want to change the default.

## License

This module is released under the [GNU GPL v2](LICENSE), the same license as AzerothCore.