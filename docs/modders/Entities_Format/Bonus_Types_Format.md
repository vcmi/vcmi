# Bonus Types Format

WARNING: currently custom bonus types can only be used for custom "traits", for example to use them in limiters. At the moment it is not possible to provide custom mechanics for such bonus, or have custom bonuses with subtypes or addInfo parameters

```json
{
	// If set to true, this bonus will be hidden in creature view
	"hidden" : false,

	// If set to true, this bonus will be considered a "creature nature" bonus
	// If creature has no creature nature bonuses, it is considered to be a LIVING creature
	"creatureNature" : false,
	
	// Generic human-readable description of this bonus
	// Visible in creature window
	// Can be overriden in creature abilities or artifact bonuses
	"description" : "{Bonus Name}\nBonus description",
	
	"graphics" : {
		// Generic icon of this bonus
		// Visible in creature window
		// Can be overriden in creature abilities or artifact bonuses
		"icon" : "path/to/icon.png",
		
		// Custom icons for specific subtypes of this bonus
		"subtypeIcons" : {
			"spellSchool.air" : "",
			"spellSchool.water" : "",
		},
		
		// Custom icons for specific values of this bonus
		// Note that values must be strings and wrapped in quotes
		"valueIcons" : {
			"1" : "",
			"2" : "",
		}
	},
	
	// Custom descriptions for specific subtypes of this bonus
	"subtypeDescriptions" : {
		"spellSchool.air" : ""
		"spellSchool.water" : "",
	},
	
	// Custom descriptions for specific values of this bonus
	// Note that values must be strings and wrapped in quotes
	"valueDescriptions" : {
			"1" : ""
			"2" : ""
		}
	}
}
```
