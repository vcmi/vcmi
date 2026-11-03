# Rumor Format

Rumors are texts told in taverns. Each week a rumor can be picked from rumors of all active mods, from rumors defined in the map, or generated from game state. Rumors of Heroes III are defined in `config/rumors.json`.

- `text` - text of the rumor. Text that starts with `@` is an identifier of an existing text, for example `@core.randtvrn.0`
- `faction` - optional. Rumor can only be told if a town of this faction is present on the map
- `weight` - optional, default is 1. Relative frequency of this rumor. Currently unused

```json
"newRumor" :
{
	"text" : "Monks of the Castles train their Zealots to strike enemies from afar and up close alike.",
	"faction" : "castle",
	"weight" : 1
}
```
