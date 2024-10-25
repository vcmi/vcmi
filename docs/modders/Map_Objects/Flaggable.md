# Flaggable objects

Flaggable object are those that can be captured by a visiting hero. H3 examples are mines, dwellings, or lighthouse.

```jsonc
{
  "baseObjectName" : {
    "name" : "Object name",
    "handler" : "flaggable", 
    "types" : {
      "objectName" : {
        
        // Text for message that player will get on capturing this object with a hero
        // Alternatively, it is possible to reuse existing string from H3 using form '@core.advevent.69'
        "onVisit" : "{Object Name}\r\n\r\nText of messages that player will see on visit.",
        
        // List of bonuses that will be granted to player that owns this object
        "bonuses" : {
          "firstBonus" : { BONUS FORMAT },
          "secondBonus" : { BONUS FORMAT },
        }
      }
    }
  }
}
```
