< [Documentation](../../Readme.md) / [Modding](../Readme.md) / [Bonus Format](../Bonus_Format.md) / Bonus Types

The bonuses were grouped according to their original purpose. The bonus system allows them to propagate freely betwen the nodes, however they may not be recognized properly beyond the scope of original use.

# General-purpose bonuses

### NONE

Special bonus that gives no effect

### MORALE

Changes morale of affected units

- val: change in morale

### LUCK

Changes luck of affected units

- val: change in luck

### MAGIC_SCHOOL_SKILL

Changes mastery level of spells of affected heroes and units. Examples are magic plains terrain and magic school secondary skills

- subtype: school of magic
- val: level

### DARKNESS

On each turn, hides area in fog of war around affected town for all players other than town owner

- val: radius in tiles

# Hero bonuses

### MOVEMENT

Increases amount of movement points available to affected hero on new turn

- subtype: 0 - sea, subtype 1 - land
- val: number of movement points (100 points for a tile)

### WATER_WALKING

Allows movement over water for affected heroes

- subtype: 1 - without penalty, 2 - with penalty

### FLYING_MOVEMENT

Allows flying movement for affected heroes

- subtype: 1 - without penalty, 2 - with penalty

### NO_TERRAIN_PENALTY

Eliminates terrain penalty on certain terrain types for affected heroes (Nomads ability).

- subtype: type of terrain

### PRIMARY_SKILL

Changes selected primary skill for affected heroes and units

- subtype: primary skill
- additional info: 1 - only for melee attacks, 2 - only for ranged attacks

### SIGHT_RADIUS

Reveal area of fog of war around affected heroes when hero is recruited or moves

- val: radius in tiles

### MANA_REGENERATION

Restores specific amount of mana points for affected heroes on new turn

- val: amount of spell points to restore

### FULL_MANA_REGENERATION

Restores entire mana pool for affected heroes on new turn

### NONEVIL_ALIGNMENT_MIX

Allows mixing of creaturs of neutral and good factions in affected armies without penalty to morale

### SURRENDER_DISCOUNT

Changes surrender cost for affected heroes

- val: decrease in cost, in precentage

### IMPROVED_NECROMANCY

TODO: Determine units which is raised by necromancy skill.

- subtype: creature raised
- val: Necromancer power
- addInfo: limiter by Necromancer power
- Example (from Necromancy skill):

` "power" : {`  
`  "type" : "IMPROVED_NECROMANCY",`  
`  "subtype" : "creature.skeleton",`  
`  "addInfo" : 0`  
` }`

### LEARN_BATTLE_SPELL_CHANCE

Determines chance for affected heroes to learn spell casted by enemy hero after battle

- val: chance to learn spell, percentage

### LEARN_BATTLE_SPELL_LEVEL_LIMIT

Allows affected heroes to learn spell casted by enemy hero after battle

- subtype: must be set to -1
- val: maximal level of spell that can be learned

### LEARN_MEETING_SPELL_LIMIT

Allows affected heroes to learn spells from each other during hero exchange

- subtype: must be set to -1
- val: maximal level of spell that can be learned

### ROUGH_TERRAIN_DISCOUNT

Reduces movement points penalty when moving on terrains with movement cost over 100 points. Can not reduce movement cost below 100 points

- val: penalty reduction, in movement points per tile. 

### WANDERING_CREATURES_JOIN_BONUS

Increases probability for wandering monsters to join army of affected heroes

- val: change in disposition factor when calculating join chance

### BEFORE_BATTLE_REPOSITION

Allows affected heroes to position army before start of battle (Tactics)

- val: distance within which hero can reposition his troops

### BEFORE_BATTLE_REPOSITION_BLOCK

Reduces distance in which enemy hero can reposition. Counters BEFORE_BATTLE_REPOSITION bonus

- val: distance within which hero can reposition his troops

### HERO_EXPERIENCE_GAIN_PERCENT

Increases experience gain from all sources by affected heroes

- val: additional experience bonus, percentage

### UNDEAD_RAISE_PERCENTAGE

Defines percentage of enemy troops that will be raised after battle into own army (Necromancy). Raised unit is determined by IMPROVED_NECROMANCY bonus

- val: percentage of raised troops

### MANA_PER_KNOWLEDGE

Defines amount of mana points that hero gains per each point of knowledge (Intelligence)

- val: Amount of mana points per knowledge

### HERO_GRANTS_ATTACKS

Gives additional attacks to specific creatures in affected army (Artillery)

- subtype: creature to give additional attacks
- val: number of attacks

### BONUS_DAMAGE_CHANCE

Gives specific creature in affected army chance to deal additional damage (Artillery)

- subtype: creature to give additional damage chance
- val: chance to deal additional damage, percentage

### BONUS_DAMAGE_PERCENTAGE

Defines additional damage dealt for creatures affected by BONUS_DAMAGE_CHANCE bonus (Artillery)

- subtype: creature to give additional damage percentage
- val: bonus damage, percentage

### MAX_LEARNABLE_SPELL_LEVEL

Defines maximum level of spells than hero can learn from any source (Wisdom)

- val: maximal level to learn

## Hero specialties

### SPECIAL_SPELL_LEV

Gives additional bonus to effect of specific spell based on level of creature it is casted on

- subtype: identifier of affected spell
- val: bonus to spell effect, percentage, divided by target creature level

### SPELL_DAMAGE

Gives additional bonus to effect of all spells of selected school

- subtype: affected spell school
- val: spell effect bonus, percentage

### SPECIFIC_SPELL_DAMAGE

Gives additional bonus to effect of specific spell

- subtype: identifier of affected spell
- val: bonus to spell effect, percentage

### SPECIAL_PECULIAR_ENCHANT

TODO: blesses and curses with id = val dependent on unit's level

- subtype: 0 or 1 for Coronius

### SPECIAL_UPGRADE

Allows creature upgrade for affected armies

- subtype: identifier of creature that can being upgraded
- addInfo: identifier of creature to which perform an upgrade

# Artifact bonuses

### SPELL_DURATION

Changes duration of timed spells casted by affected hero

- val: additional duration, turns

### SPELL

Allows affected heroes to cast specified spell, even if this spell is banned in map options or set to "special".

- subtype: spell identifier
- val: skill level mastery (0 - 3)

### SPELLS_OF_LEVEL

Allows affected heroes to cast any spell of specified level. Does not grant spells banned in map options.

- subtype: spell level

### SPELLS_OF_SCHOOL

Allows affected heroes to cast any spell of specified school. Does not grant spells banned in map options.

- subtype: spell school

### GENERATE_RESOURCE

Affected heroes will add specified resources amounts to player treasure on new day

- subtype: resource identifier
- val: additional daily income

### CREATURE_GROWTH

Increases weekly growth of creatures in affected towns (Legion artifacts)

- value: number of additional weekly creatures
- subtype: level of affected dwellings

### CREATURE_GROWTH_PERCENT

Increases weekly growth of creatures in affected towns

- val: additional growth, percentage

### BATTLE_NO_FLEEING

Heroes affected by this bonus can not retreat or surrender in battle

### NEGATE_ALL_NATURAL_IMMUNITIES

- subtype: TODO
Orb of Vulnerability

### OPENING_BATTLE_SPELL

In battle, army affected by this bonus will cast spell at the very start of the battle

- subtype: spell identifer
- val: spell mastery level

### FREE_SHIP_BOARDING

Heroes affected by this bonus will keep all their movement points when embarking or disembarking ship

### WHIRLPOOL_PROTECTION

Heroes affected by this bonus won't lose army when moving through whirlpool

# Creature bonuses

### STACK_HEALTH

Increases maximum hit point of affected units

- val: additional hit points to gain

### STACKS_SPEED

Increases movement speed of units in battle

- val: additional movement speed points

### CREATURE_DAMAGE

Increases base damage of creature in battle

- subtype: 0 = both min and max, 1 = min, 2 = max
- val: additional damage points

### SHOTS

Increases starting amount of shots that unit has in battle

- val: additional shots

# Creature abilities

## Static abilities and immunities

### NON_LIVING

Affected unit is considered to not be alive and not affected by morale and certain spells

### GARGOYLE

Affected unit is considered to be a gargoyle and not affected by certain spells

### UNDEAD

Affected unit is considered to be undead

### SIEGE_WEAPON

Affected unit is considered to be a siege machine and can not be raised, healed, have morale or move.

### DRAGON_NATURE

Affected unit is dragon. This bonus proved no effect, but is used as limiter several effects.

### KING

Affected unit will take more damage from units under Slayer spell effect

- val: required skill mastery of Slayer spell to affect this unit

### FEARLESS

Affected unit is immune to Fear ability

### NO_LUCK

Affected units can not receive good or bad luck

### NO_MORALE

Affected units can not receive good or bad morale

## Combat abilities

### FLYING

Affected unit can fly on the battlefield

- subtype: 0 - flying unit, 1 - teleporting unit

### SHOOTER

Affected unit can shoot

### CHARGE_IMMUNITY

Affected unit is immune to JOUSTING ability

### ADDITIONAL_ATTACK

Affected unit can perform additional attacks. Attacked unit will retaliate after each attack if can

- val: number of additional attacks to perform

### UNLIMITED_RETALIATIONS

Affected unit will always retaliate if able

### ADDITIONAL_RETALIATION

Affected unit can retaliate multiple times per turn

- value: number of additional retaliations

### JOUSTING

Affected unit will deal more damage based on movement distance (Champions)

- val: additional damage per passed tile, percentage

### HATE

Affected unit will deal more damage when attacking specific creature

- subtype - identifier of hated creature,
- val - additional damage, percentage

### SPELL_LIKE_ATTACK

Affected unit ranged attack will use animation and range of specified spell (Magog, Lich)

- subtype - spell identifier
- value - spell mastery level

### THREE_HEADED_ATTACK

Affected unit attacks creatures located on tiles to left and right of targeted tile (Cerberus). Only directly targeted creature will attempt to retaliate

### ATTACKS_ALL_ADJACENT

Affected unit attacks all adjacent creatures (Hydra). Only directly targeted creature will attempt to retaliate

### TWO_HEX_ATTACK_BREATH

Affected unit attacks creature located directly behind targeted tile (Dragons). Only directly targeted creature will attempt to retaliate

### RETURN_AFTER_STRIKE

Affected unit can return to his starting location after attack (Harpies)

### ENEMY_DEFENCE_REDUCTION

Affected unit will ignore specified percentage of attacked unit defence (Behemoth)

- val: amount of defence points to ignore, percentage

### GENERAL_DAMAGE_REDUCTION

- subtype - 0 - shield (melee) , 1 - air shield effect (ranged), -1 -
    armorer secondary skill (all, since 1.2)

### PERCENTAGE_DAMAGE_BOOST

- subtype: -1 - any damage (not used), 0 - melee damage (offence), 1 -
    ranged damage (archery)

### GENERAL_ATTACK_REDUCTION

eg. while stoned or blinded - in %

- subtype: -1 - any damage, 0 - melee damage, 1 - ranged damage

### DEFENSIVE_STANCE

WoG ability.

- val - bonus to defense while defending

### NO_DISTANCE_PENALTY

### NO_MELEE_PENALTY

Ranged stack does full damage in melee.

### NO_WALL_PENALTY

### FREE_SHOOTING

stacks can shoot even if otherwise blocked (sharpshooter's bow effect)

### BLOCKS_RETALIATION

eg. naga

### SOUL_STEAL

WoG ability. Gains new creatures for each enemy killed

- val: number of units gained per enemy killed
- subtype: 0 - gained units survive after battle, 1 - they do not

### TRANSMUTATION

WoG ability. % chance to transform attacked unit to other type

- val: chance to trigger in %
- subtype: 0 - resurrection based on HP, 1 - based on unit count
- addInfo: additional info - target creature ID (attacker default)

### SUMMON_GUARDIANS

WoG ability. Summon guardians surrounding desired stack

- val - amount in % of stack count
- subtype = creature ID

### RANGED_RETALIATION

Allows shooters to perform ranged retaliation

- no additional parameters

### BLOCKS_RANGED_RETALIATION

Disallows ranged retaliation for shooter unit, BLOCKS_RETALIATION bonus
is for melee retaliation only

- no additional parameters

### WIDE_BREATH

Dragon breath affecting multiple nearby hexes

- no additional parameters

### FIRST_STRIKE

First counterattack, then attack if possible

- no additional parameters

### SHOOTS_ALL_ADJACENT

H4 Cyclops-like shoot (attacks all hexes neighboring with target)
without spell-like mechanics

- no additional parameters

### DESTRUCTION

Kills extra units after hit

- subtype: 0 - kill percentage of units, 1 - kill amount
- val: chance in percent to trigger
- addInfo: amount/percentage to kill

### LIMITED_SHOOTING_RANGE

Limits shooting range and/or changes long range penalty

- val: max shooting range in hexes
- addInfo: optional - sets range for "broken arrow" half damage
    mechanic - default is 10

## Special abilities

### CATAPULT

- subtype: ability to use when catapulting (usually it contains ballistics parameters, ability for standard catapult and affected by ballistics is core:spell.catapultShot)

### CATAPULT_EXTRA_SHOTS

- subtype: ability to be affected. Ability of CATAPULT bonus should match. Used for ballistics secondary skill with subtype of core:spell.catapultShot.
- val: ability level to be used with catapult. Additional shots configured in ability level, not here.

### MANUAL_CONTROL

- manually control warmachine with
- id = subtype
- val = chance

### CHANGES_SPELL_COST_FOR_ALLY

eg. mage

- value - reduction in mana points

### CHANGES_SPELL_COST_FOR_ENEMY

eg. Silver Pegasus

- value - mana points penalty

### SPELL_RESISTANCE_AURA

eg. unicorns

- value - resistance bonus in % for adjacent creatures

### HP_REGENERATION

creature regenerates val HP every new round

- val: amount of HP regeneration per round

### MANA_DRAIN

value - spell points per turn

### MANA_CHANNELING

eg. familiar

- value in %

### LIFE_DRAIN

- val - precentage of life drained

### DOUBLE_DAMAGE_CHANCE

eg. dread knight

- value in %

### FEAR

### HEALER

First aid tent

- subtype: ability used for healing.

### FIRE_SHIELD

### MAGIC_MIRROR

- value - chance of redirecting in %

### ACID_BREATH

- val - damage per creature after attack,
- additional info - chance in percent

### DEATH_STARE

- subtype: 0 - gorgon, 1 - commander
- val: if subtype is 0, its the (average) percentage of killed
    creatures related to size of attacking stack

### SPECIAL_CRYSTAL_GENERATION

Crystal dragon crystal generation

### NO_SPELLCAST_BY_DEFAULT

Spellcast will not be default attack option for this creature

## Creature spellcasting and activated abilities

### SPELLCASTER

Creature gain activated ability to cast a spell. Example: Archangel,
Faerie Dragon

- subtype - spell id
- value - level of school
- additional info - weighted chance

use SPECIFIC_SPELL_POWER, CREATURE_SPELL_POWER or CREATURE_ENCHANT_POWER
for calculating the power (since 1.2 those bonuses can be used for
calculating CATAPULT and HEALER bonuses)

### ENCHANTER

for Enchanter spells

- val - skill level
- subtype - spell id
- additionalInfo - cooldown (number of turns)

### RANDOM_SPELLCASTER

eg. master genie

- val - spell mastery level

### SPELL_AFTER_ATTACK

- subtype - spell id
- value - chance %
- additional info - \[X, Y\]
- X - spell level
- Y = 0 - all attacks, 1 - shot only, 2 - melee only

### SPELL_BEFORE_ATTACK

- subtype - spell id
- value - chance %
- additional info - \[X, Y\]
- X - spell level
- Y = 0 - all attacks, 1 - shot only, 2 - melee only

### CASTS

how many times creature can cast activated spell

### SPECIFIC_SPELL_POWER

- value used for Thunderbolt and Resurrection casted by units, also
    for Healing secondary skill (for core:spell.firstAid used by First
    Aid tent)
- subtype - spell id

### CREATURE_SPELL_POWER

- value per unit, divided by 100 (so faerie Dragons have 500)

### CREATURE_ENCHANT_POWER

total duration of spells casted by creature

### REBIRTH

- val - percent of total stack HP restored
- subtype = 0 - regular, 1 - at least one unit (sacred Phoenix)

### ENCHANTED

Stack is permanently enchanted with spell subID of skill level = val, if val > 3 then spell is mass and has level of val-3. Enchantment is refreshed every turn.

## Spell immunities

### LEVEL_SPELL_IMMUNITY

creature is immune to all spell with level below or equal to value of this bonus

### MAGIC_RESISTANCE

- value - percent

### SPELL_DAMAGE_REDUCTION

eg. golems

- value - reduction in %
- subtype - spell school; -1 - all, 0 - air, 1 - fire, 2 - water, 3 -
    earth

### MORE_DAMAGE_FROM_SPELL

- value - damage increase in %
- subtype - spell id

### FIRE_IMMUNITY,WATER_IMMUNITY, EARTH_IMMUNITY, AIR_IMMUNITY

- subtype 0 - all, 1 - all except positive, 2 - only damage spells

### MIND_IMMUNITY

Creature is immune to all mind spells.

### SPELL_IMMUNITY

- subtype - spell id
- ainfo - 0 - normal, 1 - absolute

### RECEPTIVE

WoG ability. Creature accepts all friendly spells even though it would
be normally immune to it.

# Spell effects

### POISON

- val - max health penalty from poison possible

### SLAYER

- value - spell level

### BIND_EFFECT

doesn't do anything particular, works as a marker

### FORGETFULL

forgetfulness spell effect

- value - level

### NOT_ACTIVE

### ALWAYS_MINIMUM_DAMAGE

unit does its minimum damage from range

- subtype: -1 - any attack, 0 - melee, 1 - ranged
- value: additional damage penalty (it'll subtracted from dmg)
- additional info - multiplicative anti-bonus for dmg in % \[eg 20
    means that creature will inflict 80% of normal minimal dmg\]

### ALWAYS_MAXIMUM_DAMAGE

eg. bless effect

- subtype: -1 - any attack, 0 - melee, 1 - ranged
- value: additional damage
- additional info - multiplicative bonus for dmg in %

### ATTACKS_NEAREST_CREATURE

while in berserk

### IN_FRENZY

- value - level

### HYPNOTIZED

### NO_RETALIATION

Eg. when blinded or paralyzed.

# Undocumented

### LEVEL_COUNTER
for commander artifacts

### BLOCK_MAGIC_ABOVE
blocks casting spells of the level > value 

### BLOCK_ALL_MAGIC
blocks casting spells

### SPELL_IMMUNITY
subid - spell id

### GENERAL_DAMAGE_PREMY

### ADDITIONAL_UNITS
val of units with id = subtype will be added to hero's army at the beginning of battle

### SPOILS_OF_WAR
val * 10^-6 * gained exp resources of subtype will be given to hero after battle

### BLOCK

### DISGUISED
subtype - spell level

### VISIONS
subtype - spell level

### SYNERGY_TARGET
dummy skill for alternative upgrades mod

### BLOCK_MAGIC_BELOW
blocks casting spells of the level < value 

### SPECIAL_ADD_VALUE_ENCHANT
specialty spell like Aenin has, increased effect of spell, additionalInfo = value to add

### SPECIAL_FIXED_VALUE_ENCHANT
specialty spell like Melody has, constant spell effect (i.e. 3 luck), additionalInfo = value to fix.

### TOWN_MAGIC_WELL
one-time pseudo-bonus to implement Magic Well in the town
