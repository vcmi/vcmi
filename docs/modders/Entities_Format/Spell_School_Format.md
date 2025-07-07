# Spell School Format

WARNING: currently custom spell schools are only partially supported:

- it is possible to use custom spell schools in bonus system
- it is possible to make skill for specializing in such spell
- it is possible to specify border decorations for mastery level of such spell in spellbook
- it is possible to set bookmarks and header image for spellbook

```json
	// Internal field for H3 schools. Do not use for mods
	"index" : "",

	// displayed name of the school
	"name" : "",
	
	// animation file with spell borders for spell mastery levels
	"schoolBorders" : "SplevA",
	
	// animation file with bookmark symbol (first frame unselected, second is selected)
	"schoolBookmark" : "schoolBookmark",
	
	// image file for feader of scool for spellbook
	"schoolHeader" : "SchoolHeader"
```
