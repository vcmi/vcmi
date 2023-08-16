# Market schema

Since VCMI-1.3 it's possible to create customizable markets on adventure map.
Markets can be added as any other object with special handler called "market".

Here is schema describing such object

```js
"seafaringAcademy" : //object name
{
	"handler" : "market", //market handler
	"name" : "Seafaring Academy",
	... //describe any other regular parameters, such as sounds
	"types" : {
		"object" : { //object here is a type name
			... //describe any other regular parameters, such as aiValue or rmg
			"modes": ["resource-skill"], //modes available for market
			"offer": ["navigation"], //optional parameter - specific items, must be presented on market
			"title": "Seafaring Academy", //optional parameter - title for market window
			"efficiency": 5, //market exchange rate, equivalent to amount of markets of certain type owning by player
			"speech": "", //optional parameter - extra message showing on market

			"templates" : {
				... //describe templates in a common way
			}
		}
	}
}	
```

# Modes

Mode parameter defines a way to exchange different entities. Multiple modes can be specified to support several types of exchange.
Following options are supported:
* `"resource-resource"` - regular resource exchange, like trading post
* `"resource-player"` - allows to send resources to another player
* `"creature-resource"` - acts like freelance guild
* `"resource-artifact"` - black market
* `"artifact-resource"` - allows to sell artifacts for resources
* `"artifact-experience"` - acts like altar of sacrifice for good factions
* `"creature-experience"` - acts like altar of sacrifice for evil factions
* `"creature-undead"` - acts like skeleton transformer
* `"resource-skill"` - acts like university, where skills can be learned

## Examples

### Trading post

Trading post allows to exchange resources and send resources to another player, so it shall be configured this way:
```json
"modes" : ["resource-resource", "resource-player"]
```

### Black market

```json
"modes" : ["resource-artifact"]
```

### Freelance guild

```json
"modes" : ["creature-resource"]
```

### Altar of sacrifice

Altar of sacrifice allows exchange creatures for experience for evil factions and artifacts for experience for good factions.
So both modes shall be available in the market.
Game logic prohibits using modes unavailable for faction

```json
"modes" : ["creature-experience", "artifact-experience"]
```

# Offer

This field allows to configure specific items available in the market.
In VCMI-1.3 it can be used only for `resource-skill` mode

See [Secondary skills](https://github.com/vcmi/vcmi/wiki/Modding-~-Objects-~-Rewardable#secondary-skills) description for more details

### Example for University of magic (e.g conflux building)

```js
"modes" : ["resource-skill"],
"offer" : ["airMagic", "waterMagic", "earthMagic", "fireMagic"]
```

### Example for regular University

```js
"modes" : ["resource-skill"],
"offer" : [ //4 random skills except necromancy
    { "noneOf" : ["necromancy"] },
    { "noneOf" : ["necromancy"] },
    { "noneOf" : ["necromancy"] },
    { "noneOf" : ["necromancy"] }
]
```