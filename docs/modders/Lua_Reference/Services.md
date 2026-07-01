# Services

The static game-content catalogue, bound to the global `LIBRARY`. Looks up artifacts, creatures, factions, hero classes/types, secondary skills and spells by their config name (the same string used in JSON definitions).

### getArtifactByName

Looks up an artifact by its JSON key. Returns nil if not found.

- param `name`: `string` — JSON key of the artifact (e.g. `core:goldenBow`).

- returns [`Artifact`](Artifact.md)

### getCreatureByName

Looks up a creature by its JSON key. Returns nil if not found.

- param `name`: `string` — JSON key of the creature (e.g. `core:pikeman`).

- returns [`Creature`](Creature.md)

### getFactionByName

Looks up a faction by its JSON key. Returns nil if not found.

- param `name`: `string` — JSON key of the faction (e.g. `core:castle`).

- returns [`Faction`](Faction.md)

### getHeroClassByName

Looks up a hero class by its JSON key. Returns nil if not found.

- param `name`: `string` — JSON key of the hero class (e.g. `core:knight`).

- returns [`HeroClass`](HeroClass.md)

### getHeroTypeByName

Looks up a hero type by its JSON key. Returns nil if not found.

- param `name`: `string` — JSON key of the hero type (e.g. `core:orrin`).

- returns [`HeroType`](HeroType.md)

### getSpellByName

Looks up a spell by its JSON key. Returns nil if not found.

- param `name`: `string` — JSON key of the spell (e.g. `core:magicArrow`).

- returns [`Spell`](Spell.md)

### getSecondarySkillByName

Looks up a secondary skill by its JSON key. Returns nil if not found.

- param `name`: `string` — JSON key of the secondary skill (e.g. `core:wisdom`).

- returns [`Skill`](Skill.md)

### getSpellSchoolByName

Looks up a spell school by its JSON key. Returns nil if not found.

- param `name`: `string` — JSON key of the spell school (e.g. `core:fire`).

- returns [`SpellSchool`](SpellSchool.md)
