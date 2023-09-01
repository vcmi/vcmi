< [Documentation](../../Readme.md) / [Modding](../Readme.md) / [Map Object Format](../Map_Object_Format.md) / Dwelling

This is description of fields for creature banks, part of [Object
Format](Object_Format "wikilink")

``` javascript
{
	/// List of creatures in this bank. Each list represents one "level" of bank
	/// Creatures on the same level will have shared growth and available number (similar to towns)
	/// Note that due to GUI limitation it is not recommended to have more than 4 creatures at once
	"creatures" : [ 
		[ "airElemental", "stormElemental" ],
		[ "waterElemental" ]
	],

	/// List of guards for this dwelling. Can have two possible values:
	/// Boolean true/false - If set to "true", guards will be generated using H3 formula:
	/// 3 week growth of first available creatures
	/// List of objects - custom guards, each entry represent one stack in defender army
	"guards" : [
		{ "amount" : 12, "type" : "earthElemental" }
	]
}
```