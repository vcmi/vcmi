# Bonus Effects
Execute an action when specific event occurs to the affected unit.

## Trigger: 
Represents conditions under which an event will be triggered.

### Event sequence: 
An array with a single value representing a combat event that will trigger the action. The value can be a single event or multiple alternative events separated by vertical bars.   
For example: "eventSequence":["WAIT|DEFEND"] will execute if unit waits or defends.
 
Combat Events:
- STARTS_ROUND: Executed for every unit at the start of a round
- STARTS_TURN: Executed at the start of unit's turn
- BEFORE_ATTACK: executed before unit attack another unit
- AFTER_ATTACK: executed after unit attack another unit
- BEFORE_ATTACKED: executed before unit is attacked by another unit
- AFTER_ATTACKED: executed after unit is attacked by another unit
- WAIT: executed when unit waits
- DEFEND: executed when unit defends
- BEFORE_MOVE: executed before unit starts movement
- AFTER_MOVE: executed after unit ends movement
- ENDS_TURN: executed when unit ends a turn

## Action: 
Represents action that will occur once trigger is activated.

## Allowed actions:

### bonus: 
Applies a bonus to a unit

- `trigger`: represent conditions under which an event will occur
- `targetEnemy`: if set to true, bonus will be added to opponent unit, if exists
- `bonus`: bonus to give. See bonus format. WARNING: make sure to correctly set bonus duration of such bonus

### spell: 
Casts a spell on a unit.

- `trigger`: represent conditions under which an event will occur
- `targetEnemy`: if set to true, spell will be casts on opponent unit, if exists
- `spell`: identifier of spell to cast
- `mastery`: mastery level with which to cast the spell

Example:
Lower speed by one and cast bless on themselves each time the affected stack uses defend action.

```json
{
    "type" : "ON_COMBAT_EVENT",	// bonusEffects can be added to any bonuses, but "ON_COMBAT_EVENT" is
    "battleEffects" : 
    [		// the recommended type for bonuses that serve only as a container for bonusEffects
        {
        "action":"bonus",
        "trigger":{
            "eventSequence":[
            "DEFEND"
            ]
        },
        "bonus" : {
            "type" : "STACKS_SPEED",
            "val" : -1,
            "duration" : "N_TURNS",
            "turns" : 2 //The turns during which you defend counts as the first one
	    }	
        },
        {
        "action": "spell",
        "spell": "bless",
        "trigger":{
            "eventSequence":[
            "DEFEND"
            ]
        },
        "mastery" : 0,
        "targetEnemy" : false,
        }    
    ]
}
```


# Advanced Bonus effects for spells
Some Bonus Effects' features are restricted to bonuses applied to units by combat spells.

### Event sequence (advanced):
An array representing a sequence of events needed for a trigger to activate. Events can be separated by a vertical bar to indicate alternative events.
For example: "eventSequence":["BEFORE_ATTACKED", "WAIT|DEFEND"] will execute if unit waits or defends after being attacked.

###oncePerBattle: 
Set to true makes trigger deactivate for a battle after the first time it is triggered. 

###continuous:
If set to true trigger will only activate when events occur one after another. 
For example: "eventSequence":["AFTER_MOVE", "BEFORE_ATTACK"] with continuous set to true will activate only if a unit moves-attack during the same turn.

## Additional allowed actions for combat spell bonuses:

### terminate: 
Removes the bonus

- `trigger`: represent conditions under which an event will occur.

### changeDuration:

- `trigger`: represent conditions under which an event will occur.
- `value`: adds turn to bonus duration. Accepts negative numbers.


Examples (spell bonuses):

End bonus effect at the start of the second turn of affected stack (like spell "frenzy"):

```json
"battleEffects": [
{
  "action": "terminate",
  "trigger": {
    "eventSequence": [
      "ENDS_TURN",
      "STARTS_TURN"
    ]
  }
}
]
```

Cast lightbolt at every enemy attacking affected stack. Subtract single turn from the duration of the bonus after every attack (like spell "avenging angel" from "King's Bounty: Armored Princess").

```json
"battleEffects":[
 {
    "action":"spell",
    "trigger":{
       "eventSequence":[
          "AFTER_ATTACKED"
       ],
       "oncePerBattle" : false
    },
    "spell":"lightningBolt",
    "mastery":1,
    "targetEnemy":true
 },
 {
    "action":"changeDuration",
    "trigger":{
       "eventSequence":[
          "AFTER_ATTACKED"
       ],
       "oncePerBattle" : false
    },
    "value":-1
 }
]
```

Make until invincible until spell duration passes or unit attacks (like spell "mist" from "Spell of Conquest").

```json
"type":"INVINCIBLE",
"duration":"N_TURNS",
"battleEffects": [
   {
      "action":"terminate",
      "trigger":{
	 "eventSequence":[
	    "AFTER_ATTACK|UNIT_SPELLCAST",
	    "ENDS_TURN"		//ends_turn is needed to keep unit invincible until the end of its turn, blocking the possible counterattack
	 ]
      }
   }
]  
```



