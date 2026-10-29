# ComponentType

Kinds of icons shown in message windows, used by Component `type`.

| Key | Value | Description |
| --- | ----- | ----------- |
| `primarySkill` | 0 | Primary skill; `subType` is the skill, `value` the change. |
| `secondarySkill` | 1 | Secondary skill; `subType` is the skill, `value` the mastery level (1-3). |
| `resource` | 2 | Resource; `subType` is the resource, `value` the amount. |
| `resourcePerDay` | 3 | Daily resource income; `subType` is the resource, `value` the amount per day. |
| `creature` | 4 | Creature; `subType` is the creature, `value` the optional count. |
| `artifact` | 5 | Artifact; `subType` is the artifact. |
| `spellScroll` | 6 | Spell scroll; `subType` is the spell in the scroll. |
| `mana` | 7 | Spell points; `value` is the change. |
| `experience` | 8 | Experience; `value` is the amount. |
| `level` | 9 | Hero levels; `value` is the number of levels. |
| `spell` | 10 | Spell; `subType` is the spell. |
| `morale` | 11 | Morale; `value` is the change. |
| `luck` | 12 | Luck; `value` is the change. |
| `building` | 13 | Town building; `subType` is the building and `faction` its faction. |
| `heroPortrait` | 14 | Hero portrait; `subType` is the hero. |
| `flag` | 15 | Player flag; `subType` is the player. |
