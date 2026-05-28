# Flaggable objects

Flaggable object are those that can be captured by a visiting hero. H3 examples are mines, dwellings, or lighthouse.

Currently, it is possible to make flaggable objects that provide player with:

- Any [Bonus](../Bonus_Format.md) supported by bonus system
- Daily resources income (wood, ore, gold, etc)

## Format description

```json
{
  "baseObjectName" : {
    "name" : "Object name",
    "handler" : "flaggable", 
    "types" : {
      "objectName" : {
        
        // Text for message that player will get on capturing this object with a hero
        // Alternatively, it is possible to reuse existing string from H3 using form '@core.advevent.69'
        "onVisit" : "{Object Name}\r\n\r\nText of messages that player will see on visit.",
        
        // List of bonuses that player that owns this object may receive
        // Make sure to use required propagator, such as PLAYER_PROPAGATOR
        "bonuses" : {
          "firstBonus" : { BONUS FORMAT },
          "secondBonus" : { BONUS FORMAT },
        },
        
        // Resources that will be given to owner on every day
        "dailyIncome" : {
          "wood" : 2,
          "ore"  : 2,
          "gold" : 1000
        }
      }
    }
  }
}
```
