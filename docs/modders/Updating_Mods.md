# Updating Mods

## Hints to preserve save and map compatibility

While in some cases save-breaking changes are unavoidable, in many cases it is possible to avoid save breakage.

Things that are safe to change:

- Generally, properties of various game entities such as heroes, creatures, or artifacts, are safe to change, and in many cases - will apply immediately to the ongoing games. Some properties such as heroes armies or adventure map object configuration is only loaded on map start. Such change will only apply on starting a new game or restarting scenario in case of campaign.
- Mod updates that add new objects to existing mod are safe. New objects will be considered banned and will not appear in ongoing save, but save can still be loaded after such change.

Things that need caution when changing:

- Moving game entity into a different submod is safe on its own, however should be restricted to new submods, or to mods that player is likely to have enabled. Game can load save after moving entity to another submod, but only if submod to which this entity was moved to is also enabled
- Renaming a game entity can only be done while providing hint for the game on how to replace it. See [Compatibility identifiers field](#compatibility-identifiers-field) for details.
- Removing a game entity should be avoided if possible. Instead consider marking it as "special" to ban it from random selection in new saves to give players time to finish ungoing games, and after some period - remove it with some replacement using [Compatibility identifiers field](#compatibility-identifiers-field)

Things that are guaranteed to break the save:

- Removing or renaming an object without providing `compatibilityIdentifiers` field
- Moving a game entity to another mod (and not a submod within same main mod)
- Making you mod depend on another mod will break save since list of main mods from a save can't be changed right now

### Compatibility identifiers field

All game entities support field named `compatibilityIdentifiers`. This field allows VCMI to locate renamed entities or to provide fallback for removed objects. For example, if your mod has hero named `archibald` and at some point you've decided to rename it to `roland`, you can add following code:

```json
"roland" : {
	"compatibilityIdentifiers" : [ "archibald`" ],
	...
}
```

With this code, when VCMI locates hero named `archibald` when loading old save or old map, it will automatically replace `archibald` with `roland`. Same approach should be used for small changes, for example to fix a typo in entity name. Renaming an object without providing this field **will break saves and maps for all players using your mod**.

Similarly, when you need to remove an object, please provide some reasonable fallback for game to auto-replace your entity using this field. It can be used not only for renames, but also to completely replace an entity, including to existing one, so if you're removing it without replacement, this field can be used to replace entity with some other object that existed before and is not removed.

### Updating map after mod update

If one of mods on which your map depends made a map-breaking change and map can no longer be loaded in game, best option is to try to contact original mod author and ask it to provide save compability support as described on this page.

If contacting author is not possible or you wish to fix your map on your own, you can instead:

1. Alternatively, you can add map compability to the mod on your own using instructions from this page.
2. If changes are too major and can't be fixed via map compability tools, rename your map from `xxx.vmap` to `xxx.zip`, extract map as zip archive.
3. Open contained files using text editor such as Notepad++ and try to fix map manually, for example by removing broken objects or by renaming them to some other existing entity.
4. After you've made the changes, recreate .zip archive and rename it back to `xxx.vmap` and test loading of map in game
5. Once your map can be loaded, re-save map in map editor using fixed version of the mod
6. If possible, update mod with changes that you've made, if any.
