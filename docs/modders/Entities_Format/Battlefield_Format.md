# Battlefield Format

```json
	// Human-readable name of the battlefield
	"name" : "",
	
	// If set to true, obstacles will be taken from "specialBattlefields" property of an obstacle
	// If set to false, obstacles will be taken from "allowedTerrains" instead
	"isSpecial" : false,
	
	// List of bonuses that will affect all battles on this battlefield
	"bonuses" : { BONUS_FORMAT },
	
	// Background image for this battlefield
	"graphics" : "",
	
	// Optional, filename for custom music to play during combat on this terrain
	"music" : "",
	
	// Optional, filename for custom sound to play during combat opening on this terrain
	"openingSound" : "",
	
	// List of battle hexes that will be always blocked on this battlefield (e.g. ship to ship battles)
	"impassableHexes" : [ 10, 20, 50 ],
```

### Impassable Hexes

Impassable hexes operate in absolute coordinates. For reference on tiles indexes see image below:

![Battlefield Hexes Layout](../../images/Battle_Field_Hexes.svg)
