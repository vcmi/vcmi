# AdventureServer

The authoritative adventure-map mutation interface. Available only to scripts running on the server; every call emits a network pack so clients receive the resulting state change. Grants rewards, shows dialogs, removes map objects and stores script variables.

### setMapVariable

Stores a named value that persists in the save file and can be read back later with Game:getMapVariable. Use it to remember progress across visits, such as whether a one-time reward was already handed out.

- param `name`: `string` — Name of the variable to store under.
- param `value`: `any` — Value to store; any value that survives as JSON - number, string, boolean, or a table of those. Overwrites any previous value.

### removeObject

Permanently removes a map object from the adventure map. The object disappears for every player; if it was a town or a hero, ownership and garrison are lost as well. There is no undo.

- param `target`: [`MapObject`](MapObject.md) — Map object to remove.

### finishQuestOrRemoveObject

Ends the current quest of the given object. A seer hut's active quest is marked complete and cleared, without removing the hut itself; a quest guard is removed from the map. Errors when the object is not a quest source - use removeObject for plain events and pandoras.

- param `target`: [`MapObject`](MapObject.md) — The quest source (seer hut / quest guard) that just finished its quest.

### markQuestProposed

Internal plumbing for registerQuest: remembers that a player has already been offered this quest, so a later visit shows the progression text instead of the proposal text again.

- param `target`: [`MapObject`](MapObject.md) — The quest source (seer hut / quest guard) to mark.
- param `player`: `integer` — Player who has now seen the quest proposed.

### addToQuestLog

Adds the object's active quest to a player's in-game quest log. Calling it again for a quest already in the log does nothing.

- param `target`: [`MapObject`](MapObject.md) — The quest source (seer hut / quest guard) to add.
- param `player`: `integer` — Player whose quest log gains the entry.

### setQuestHintText

Internal plumbing for the setQuestHint helper. Scripts should call setQuestHint instead.

- param `target`: [`MapObject`](MapObject.md) — The quest source (seer hut / quest guard) whose hint changes.
- param `text`: [`MetaString`](MetaString.md) — New hover / quest-log text for the object's active quest.

### random

Returns a random whole number in the given range. Draws from the game's own random generator, so the result stays consistent with saved games and network play - do not use Lua's math.random for gameplay decisions.

- param `lower`: `integer` — Smallest value that may be returned.
- param `upper`: `integer` — Largest value that may be returned.

- returns `integer` — A whole number between lower and upper, both ends included.

### giveExperience

Awards experience points to a hero, triggering any level-ups (and the level-up dialog for a human player) that result.

- param `hero`: [`HeroInstance`](HeroInstance.md) — Hero that gains the experience.
- param `amount`: `integer` — Experience points to add. The hero levels up automatically if the total crosses a level threshold.

### giveResource

Adds or removes a single resource for one player.

- param `player`: `integer` — Player whose treasury changes.
- param `resource`: [`ResourceType`](ResourceType.md) — Resource to change, as returned by Services:getResourceByName.
- param `amount`: `integer` — How much to add. Use a negative number to take resources away; the treasury is clamped at zero and never goes negative.

### setOwner

Transfers ownership of a map object to another player. For a town or mine this immediately moves its income and control; it does not move any garrisoned army or visiting hero.

- param `object`: [`MapObject`](MapObject.md) — Map object whose owner changes, such as a town, mine or dwelling.
- param `owner`: `integer` — New owner. Pass the neutral player to make the object unowned.

### grantSpell

Teaches a spell to a hero, writing it into the hero's spellbook. The hero needs a spellbook for the spell to be usable in combat. Teaching a spell the hero already knows does nothing.

- param `hero`: [`HeroInstance`](HeroInstance.md) — Hero that learns the spell.
- param `spell`: [`Spell`](Spell.md) — Spell to teach, as returned by Services:getSpellByName.

### takeSpell

Removes a spell from a hero's spellbook. Does nothing if the hero did not know the spell.

- param `hero`: [`HeroInstance`](HeroInstance.md) — Hero that forgets the spell.
- param `spell`: [`Spell`](Spell.md) — Spell to remove, as returned by Services:getSpellByName.

### grantPrimarySkill

Permanently raises or lowers one of a hero's four primary skills.

- param `hero`: [`HeroInstance`](HeroInstance.md) — Hero whose skill changes.
- param `skill`: `integer` — Primary skill to change; use ENUM.PrimarySkill.
- param `amount`: `integer` — How many points to add. Use a negative number to reduce the skill; it is clamped at zero and never goes negative.

### grantSecondarySkill

Teaches a secondary skill to a hero, or changes it to the given mastery. A hero who has no free skill slots left will not learn a brand-new skill.

- param `hero`: [`HeroInstance`](HeroInstance.md) — Hero that learns the skill.
- param `skill`: [`Skill`](Skill.md) — Secondary skill to grant, as returned by Services:getSecondarySkillByName.
- param `level`: `integer` — Mastery to move to: 1 = basic, 2 = advanced, 3 = expert.

### grantArtifact

Gives an artifact to a hero. It is equipped in a matching free slot, or placed in the backpack when no suitable slot is free. The hero gains the artifact's bonuses only while it is equipped.

- param `hero`: [`HeroInstance`](HeroInstance.md) — Hero that receives the artifact.
- param `artifact`: [`Artifact`](Artifact.md) — Artifact to give, as returned by Services:getArtifactByName.

### grantScroll

Gives a spell scroll to a hero. While the scroll is carried the hero may cast that spell even without a spellbook. The scroll occupies an artifact slot like any other artifact.

- param `hero`: [`HeroInstance`](HeroInstance.md) — Hero that receives the scroll.
- param `spell`: [`Spell`](Spell.md) — Spell written on the scroll, as returned by Services:getSpellByName.

### takeArtifact

Removes an artifact from a hero, whether it is equipped or sitting in the backpack. If the artifact is a part of an assembled combination artifact, the combination is taken apart first and the remaining parts stay with the hero. Does nothing if the hero does not own the artifact.

- param `hero`: [`HeroInstance`](HeroInstance.md) — Hero that loses the artifact.
- param `artifact`: [`Artifact`](Artifact.md) — Artifact to remove, as returned by Services:getArtifactByName.

### grantCreatures

Adds creatures to a hero's army.

- param `hero`: [`HeroInstance`](HeroInstance.md) — Hero whose army grows.
- param `creature`: [`Creature`](Creature.md) — Creature to add, as returned by Services:getCreatureByName.
- param `count`: `integer` — How many creatures to add. They join an existing stack of the same creature, or take a new army slot; if the army is full of other creatures the new ones are lost.

### takeCreatures

Removes creatures of one type from a hero's army.

- param `hero`: [`HeroInstance`](HeroInstance.md) — Hero whose army shrinks.
- param `creature`: [`Creature`](Creature.md) — Creature to remove, as returned by Services:getCreatureByName.
- param `count`: `integer` — How many to remove. If the hero has fewer, all of them are removed. Emptied stacks disappear.

### grantWarMachine

Gives a war machine to a hero, placing it in its dedicated equipment slot. Has no effect if the hero already carries a war machine in that slot.

- param `hero`: [`HeroInstance`](HeroInstance.md) — Hero that receives the war machine.
- param `machine`: [`Artifact`](Artifact.md) — War machine to give, as returned by Services:getArtifactByName.

### takeWarMachine

Removes a war machine from a hero. Does nothing if the hero did not carry it.

- param `hero`: [`HeroInstance`](HeroInstance.md) — Hero that loses the war machine.
- param `machine`: [`Artifact`](Artifact.md) — War machine to remove, as returned by Services:getArtifactByName.

### grantSpellbook

Gives a spellbook to a hero, without which learned spells cannot be cast in combat. Does nothing if the hero already has a spellbook.

- param `hero`: [`HeroInstance`](HeroInstance.md) — Hero that receives the spellbook.

### takeSpellbook

Removes the hero's spellbook. The hero keeps the list of learned spells but can no longer cast them until given a spellbook again. Does nothing if the hero had no spellbook.

- param `hero`: [`HeroInstance`](HeroInstance.md) — Hero that loses the spellbook.

### grantMorale

Gives a hero a temporary morale bonus that lasts until the end of the hero's next battle. Repeated calls each add another separate bonus rather than replacing the previous one.

- param `hero`: [`HeroInstance`](HeroInstance.md) — Hero that receives the morale change.
- param `amount`: `integer` — Morale points to add; use a negative number to lower morale.

### grantLuck

Gives a hero a temporary luck bonus that lasts until the end of the hero's next battle. Repeated calls each add another separate bonus rather than replacing the previous one.

- param `hero`: [`HeroInstance`](HeroInstance.md) — Hero that receives the luck change.
- param `amount`: `integer` — Luck points to add; use a negative number to lower luck.

### grantSpellPoints

Changes a hero's remaining spell points. The mode selects whether the amount is added, subtracted or set as the new total.

- param `hero`: [`HeroInstance`](HeroInstance.md) — Hero whose spell points change.
- param `amount`: `integer` — Spell points involved in the change.
- param `mode`: `integer` — 0 adds the amount, 1 subtracts it (clamped at zero), 2 sets the total to the amount.

### grantMovementPoints

Changes a hero's remaining movement points for the current turn. The mode selects whether the amount is added, subtracted or set.

- param `hero`: [`HeroInstance`](HeroInstance.md) — Hero whose movement points change.
- param `amount`: `integer` — Movement points involved in the change.
- param `mode`: `integer` — 0 adds the amount, 1 subtracts it (clamped at zero), 2 sets the total to the amount.

### grantCreaturesToHire

Changes the number of creatures available to hire at one tier of a town on the map.

- param `town`: [`TownInstance`](TownInstance.md) — Town whose pool of creatures available to hire changes.
- param `level`: `integer` — Creature tier (0-7) whose available pool is modified.
- param `count`: `integer` — How many extra creatures become available to hire at that tier. Use a negative number to reduce the pool; it is clamped at zero.

### constructBuilding

Erects a building in a town for free, ignoring the usual cost and prerequisites.

- param `town`: [`TownInstance`](TownInstance.md) — Town that gains the building.
- param `building`: `integer` — Identifier of the building to erect.

### showMessage

Pops up a message box for one player. This call does not wait for the player to react - the script continues immediately and the box is shown at the next opportunity. Use it for notifications, not for questions.

- param `player`: `integer` — Player who should see the message. Other players see nothing.
- param `text`: [`MetaString`](MetaString.md) — The message text, built with MetaString so it can be translated and can embed names and numbers.
- param `components`: [`Component[]?`](Component.md) — Optional icons shown under the text, such as awarded resources or artifacts. Omit for a plain text box.
- param `soundID`: `integer?` — Optional sound to play when the message opens. Omit for the default.
- param `windowType`: `integer?` — Optional look of the window. Omit to let the engine pick automatically based on the contents.

### spawnDialog

Internal plumbing for the blocking showQuestion / showRewardsMessage helpers: shows a modal dialog and registers the query whose reply resumes the paused script. Scripts should call showQuestion / showRewardsMessage instead.

- param `player`: `integer` — Player who must answer the dialog.
- param `text`: [`MetaString`](MetaString.md) — The dialog text, built with MetaString.
- param `mode`: `integer` — 0 shows a plain acknowledge box; any other value shows a yes/no question.
- param `components`: [`Component[]?`](Component.md) — Optional icons shown under the text.

### spawnCombat

Internal plumbing for the startCombat helper: replaces the host object's garrison with the given creatures and starts a battle against the visiting hero. Scripts should call startCombat instead.

- param `host`: [`MapObject`](MapObject.md) — The visited event/pandora whose garrison is replaced with the opposing army.
- param `hero`: [`HeroInstance`](HeroInstance.md) — Hero that fights the army.
- param `army`: `any` — List of {count, creatureKey} pairs describing the opposing army.
