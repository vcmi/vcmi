# BonusSource

Origin classes used by Bonus / BonusDescriptor `sourceType`.

| Key | Value | Description |
| --- | ----- | ----------- |
| `artifact` | 0 | Granted by an artifact definition. Does not stacks if hero has multiple of same artifacts |
| `artifactInstance` | 1 | Granted by a specific equipped artifact instance. Stacks with multiple copies of same artifact |
| `objectType` | 2 | Granted by a map-object type (e.g. Stables). |
| `objectInstance` | 3 | Granted by a specific instance of map object, stacks with each same object. |
| `creatureAbility` | 4 | Innate to the creature type (e.g. dragon breath, undead). |
| `terrainNative` | 5 | Native-terrain bonus (creature on its faction's home terrain). |
| `terrainOverlay` | 6 | Battlefield-terrain overlay bonus (magic plains, fiery fields, …). |
| `spellEffect` | 7 | Active spell effect on the bearer. |
| `townStructure` | 8 | Built town structure providing the bonus. |
| `heroBaseSkill` | 9 | Hero primary skill (attack/defense/power/knowledge). |
| `secondarySkill` | 10 | Hero secondary skill level (Tactics, Offense, Wisdom, …). |
| `heroSpecial` | 11 | Hero specialty (creature affinity, spell affinity, etc.). |
| `army` | 12 | Army composition bonus. Currently only used for morale. |
| `campaignBonus` | 13 | Carry-over bonus from a campaign scenario. |
| `stackExperience` | 14 | Stack-experience rank bonus. |
| `commander` | 15 | Commander unit ability. |
| `global` | 16 | Map-wide global effect. Defined in game settings. |
| `other` | 17 | Source not represented by another category. |
