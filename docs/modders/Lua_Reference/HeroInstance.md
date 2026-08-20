# HeroInstance

A hero placed on the adventure map. Provides access to such information as owner, type (Orrin, Kyrre, Astral...), position on map, artifacts, and secondary skills, army composition, primary skills, and all bonuses affecting hero.

### getBonuses

Returns the bonuses of the bearer that match the filter. Say as much as the filter can express, since that is also what the engine can cache; narrow whatever is left with `BonusList:filter`.

- param `filter`: [`BonusFilter`](BonusFilter.md) — Which bonuses to collect. An empty filter collects every one of them.

- returns [`BonusList`](BonusList.md) — Bonuses of the bearer the filter describes.

### getBonusesValue

Returns what the matching bonuses are worth together. Not a plain sum - percentages, independent floors and ceilings combine by the rules of the engine. Prefer this over adding up `getBonuses` where possible.

- param `filter`: [`BonusFilter`](BonusFilter.md) — Which bonuses to count. An empty filter counts every one of them.

- returns `integer` — Value of the matching bonuses taken together.

### hasBonuses

True if the bearer carries a bonus the filter describes. Prefer this over testing the size of `getBonuses`, which hands the whole list over to the script to answer a question the engine can answer on its own.

- param `filter`: [`BonusFilter`](BonusFilter.md) — Which bonuses to look for. An empty filter asks whether the bearer has any at all.

- returns `boolean` — True if the bearer carries at least one matching bonus.

### getStack

Returns the stack instance in the given army slot, or nil if the slot is empty.

- param `slot`: `integer` — Army slot to query (1-based).

- returns [`StackInstance`](StackInstance.md)

### getOwner

Returns the player color that owns this hero.

- returns `integer`

### getNameTextID

Returns the text ID of the hero's name.

- returns `string`

### isMale

True if the hero's gender is male.

- returns `boolean`

### isFemale

True if the hero's gender is female.

- returns `boolean`

### getLevel

Returns the hero's current level.

- returns `integer` — Current level of the hero.

### getExperience

Returns the hero's total experience points.

- returns `integer` — Total experience points of the hero.

### getPrimarySkill

Returns the value of one of the hero's primary skills.

- param `skill`: `integer` — Primary skill JSON key (`attack`, `defence`, `spellpower`, `knowledge`).

- returns `integer` — Current value of the primary skill.

### getSecondarySkill

Returns the hero's mastery of the given secondary skill.

- param `skill`: `integer` — Secondary skill JSON key.

- returns `integer` — Mastery level (0 = none, 1 = basic, 2 = advanced, 3 = expert).

### hasArtifact

Returns whether the hero owns the given artifact, either as equipped or in the backpack.

- param `artifact`: `integer` — Artifact JSON key.

- returns `boolean` — True if the hero owns the artifact.

### ownedArtifacts

Returns how many copies of the given artifact the hero owns, counting both equipped slots and the backpack.

- param `artifact`: `integer` — Artifact JSON key.

- returns `integer` — Number of copies of that artifact the hero carries.

### creatureCountInArmy

Returns how many creatures of the given type are in the hero's army across all unit stacks.

- param `creature`: `integer` — Creature JSON key.

- returns `integer` — Total number of that creature across the hero's army.
