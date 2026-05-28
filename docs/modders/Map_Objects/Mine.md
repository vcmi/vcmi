# Mine

Multiple mines per resource are allowed.

Beside the common parameters from [Map Object Format](../Map_Object_Format.md) there are some additional parameters:

```json
{
	/// produced resource
	"resource" : "mithril",

	/// amount of resources produced each day
	"defaultQuantity" : 1,

	/// displayed name of mine
	"name" : "name text",

	/// displayed description of mine (for popup)
	"description" : "description text",

	/// Optional guards, uses the same stack format as other configurable guard lists.
	"guards" : [
		{ "type" : "marksman", "amount" : 40, "upgradeChance" : 50 }
	],

	/// Optional message shown when hero attempts to visit guarded mine.
	/// Can be a raw string, @textID, or adventure text index.
	"onGuardedMessage" : "This mine is guarded.\n\nDo you wish to fight the guards?",

	/// Optional message shown when hero attempts to visit guarded owned mine.
	/// Can be a raw string, @textID, or adventure text index.
	"ownedGuardedMessage" : "This mine belongs to an enemy, and is guarded.\n\nDo you wish to fight the guards?",

	/// Optional message shown after hero captures abandoned mine.
	/// Can be a raw string, @textID, or adventure text index.
	"onCaptureMessage" : "The mine has fallen into your hands.",

	/// Optional image showed on kingdom overview (animation; only frame 0 displayed)
	"kingdomOverviewImage" : "image.def"
}
```
