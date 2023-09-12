< [Documentation](../../Readme.md) / [Modding](../Readme.md) / [Bonus Format](../Bonus_Format.md) / Bonus Types

The bonuses were grouped according to their original purpose. The bonus system allows them to propagate freely betwen the nodes, however they may not be recognized properly beyond the scope of original use.

# General-purpose bonuses

### NONE

Special bonus that gives no effect

### MORALE

-   val = value

### LUCK

-   val = value

### MAGIC_SCHOOL_SKILL

eg. for magic plains terrain and for magic school secondary skills

-   subtype: school of magic (0 - all, 1 - air, 2 - fire, 4 - water, 8 -
    earth)
-   val - level

### DARKNESS

-   val = radius

# Hero bonuses

### MOVEMENT

Before 1.2: both water / land After 1.2: subtype 0 - sea, subtype 1 -
land

-   val = number of movement points (100 points for a tile)

### WATER_WALKING

-   subtype: 1 - without penalty, 2 - with penalty

### FLYING_MOVEMENT

-   subtype: 1 - without penalty, 2 - with penalty

### NO_TERRAIN_PENALTY

Hero does not get movement penalty on certain terrain type (Nomads
ability).

-   subtype - type of terrain

### PRIMARY_SKILL

-   uses subtype to pick skill
-   additional info if set: 1 - only melee, 2 - only distance

### SIGHT_RADIUS

Sight radius of a hero. Used for base radius + Scouting secondary skill

-   val = distance in tiles

### MANA_REGENERATION

Before 1.2: points per turn apart from normal (1 + mysticism) After 1.2:
points per turn (used for artifacts, global 1 point/turn regeneration
and mysticism secondary skill)

### FULL_MANA_REGENERATION

all mana points are replenished every day

### NONEVIL_ALIGNMENT_MIX

good and neutral creatures can be mixed without morale penalty

### SURRENDER_DISCOUNT

%

### IMPROVED_NECROMANCY

Determine units which is raised by necromancy skill.

-   subtype: creature raised
-   val: Necromancer power
-   addInfo: limiter by Necromancer power
-   Example (from Necromancy skill):

` "power" : {`  
`  "type" : "IMPROVED_NECROMANCY",`  
`  "subtype" : "creature.skeleton",`  
`  "addInfo" : 0`  
` }`

### LEARN_BATTLE_SPELL_CHANCE

-   subtype: 0 - from enemy hero, 1 - from entire battlefield (not
    implemented now).
-   val: chance to learn spell after battle victory

Note: used for Eagle Eye skill

### LEARN_BATTLE_SPELL_LEVEL_LIMIT

-   subtype: school (-1 for all), others TODO
-   val: maximum learning level

Note: used for Eagle Eye skill

### LEARN_MEETING_SPELL_LIMIT

-   subtype: school (-1 for all), others TODO
-   val: maximum learning level for learning a spell during hero
    exchange

Note: used for Scholar skill

### ROUGH_TERRAIN_DISCOUNT

-   val: Non-road terrain discount in movement points

Note: used for Pathfinding skill

### WANDERING_CREATURES_JOIN_BONUS

-   val: value than used as level of diplomacy inside joining
    probability calculating

### BEFORE_BATTLE_REPOSITION

-   val: number of hexes - 1 than should be used as repositionable hexes before battle (like H3 tactics skill)

Note that Tactics phase for both sides simultaneously is not supported

### BEFORE_BATTLE_REPOSITION_BLOCK

-   val: value than block opposite tactics, if value of opposite tactics is less than this value of your hero.

### HERO_EXPERIENCE_GAIN_PERCENT

-   val: how many experience hero gains from any source. There is a
    global effect which set it by 100 (global value) and it is used as
    learning skill

### UNDEAD_RAISE_PERCENTAGE

-   val: Percentage of killed enemy creatures to be raised after battle
    as undead.

Note: used for Necromancy secondary skill, Necromancy artifacts and town
buildings.

### MANA_PER_KNOWLEDGE

-   val: Percentage rate of translating 10 hero knowledge to mana, used
    for intelligence and global bonus

### HERO_GRANTS_ATTACKS

-   subtype: creature to have additional attacks
-   val: Number of attacks

Note: used for Artillery secondary skill

### BONUS_DAMAGE_PERCENTAGE

-   subtype: creature to have additional damage percentage
-   val: percentage to be granted

Note: used for Artillery secondary skill

### BONUS_DAMAGE_CHANCE

-   subtype: creature to have additional damage chance (will have
    BONUS_DAMAGE_PERCENTAGE applied before attack concluded)
-   val: chance in percent

### MAX_LEARNABLE_SPELL_LEVEL

-   val: maximum level of spells than hero can learn from any source.
    This bonus have priority above any other LEARN\_\*SPELL_LEVEL
    bonuses.

Note: used as global effect and as wisdom secondary skill.

## Hero specialties

### SPECIAL_SPELL_LEV

-   subtype = id
-   additionalInfo = value per level in percent

### SPELL_DAMAGE

- val = value in percent
- subtype - spell school

### SPECIFIC_SPELL_DAMAGE

-   subtype = id of spell
-   val = value in percent (Luna, Ciele)

### SPECIAL_PECULIAR_ENCHANT

blesses and curses with id = val dependent on unit's level

-   subtype = 0 or 1 for Coronius

### SPECIAL_UPGRADE

-   subtype = base creature ID
-   addInfo = target creature ID

# Artifact bonuses

### SPELL_DURATION

### SPELL

Hero knows spell, even if this spell is banned in map options or set to "special".

-   subtype - spell id
-   val - skill level (0 - 3)

### SPELLS_OF_LEVEL

hero knows all spells of given level

-   subtype - level
-   val - skill level

Does not grant spells banned in map options.

### SPELLS_OF_SCHOOL

-   subtype = spell school

All spells of this school are granted to hero, eg. by Tomes of Magic. Does not grant spells banned in map options.

### GENERATE_RESOURCE

-   subtype - resource
-   val - daily income

### CREATURE_GROWTH

for legion artifacts

-   value - weekly growth bonus,
-   subtype - monster level if aplicable

### CREATURE_GROWTH_PERCENT

increases growth of all units in all towns,

-   val - percentage

### BATTLE_NO_FLEEING

for Shackles of War

### NEGATE_ALL_NATURAL_IMMUNITIES

Orb of Vulnerability

### OPENING_BATTLE_SPELL

casts a spell at expert level at beginning of battle

-   subtype - Spell ID
-   val - spell power

### FREE_SHIP_BOARDING

movement points preserved with ship boarding and landing

### WHIRLPOOL_PROTECTION

hero won't lose army when teleporting through whirlpool

# Creature bonuses

### STACK_HEALTH

### STACKS_SPEED

-   additional info - percent of speed bonus applied after direct
    bonuses; \>0 - added, \<0 - subtracted to this part

### CREATURE_DAMAGE

-   subtype: 0 = both, 1 = min, 2 = max

### SHOTS

### EXP_MULTIPLIER

-   val - percent of additional exp gained by stack / commander (base
    value 100)

# Creature abilities

## Static abilities and immunities

### NON_LIVING

eg. golems, elementals

### GARGOYLE

Gargoyle is special than NON_LIVING, cannot be rised or healed

### UNDEAD

### SIEGE_WEAPON

War machines have it. They cannot be raised or healed, have no morale
and don't move.

### DRAGON_NATURE

### KING

Creatures take more damage from Slayer effect than have greater or equal
value than KING bonus.

### FEARLESS

### NO_LUCK

eg. when fighting on cursed ground

### NO_MORALE

eg. when fighting on cursed ground

## Combat abilities

### FLYING

-   subtype - 0 - regular, 1 - teleport

### SHOOTER

### CHARGE_IMMUNITY

### ADDITIONAL_ATTACK

### UNLIMITED_RETALIATIONS

### ADDITIONAL_RETALIATION

-   value - number of additional retaliations

### JOUSTING

for champions

-   val: percentage of charge

### HATE

eg. angels hate devils,

-   subtype - ID of hated creature,
-   val - damage bonus percent

### SPELL_LIKE_ATTACK

range is taken from spell, but damage from creature; eg. magog, lich

-   subtype - spell,
-   value - spell level

### THREE_HEADED_ATTACK

eg. cerberus

### ATTACKS_ALL_ADJACENT

eg. hydra

### TWO_HEX_ATTACK_BREATH

eg. dragons

### RETURN_AFTER_STRIKE

### ENEMY_DEFENCE_REDUCTION

in % (value) eg. behemots

### GENERAL_DAMAGE_REDUCTION

-   subtype - 0 - shield (melee) , 1 - air shield effect (ranged), -1 -
    armorer secondary skill (all, since 1.2)

### PERCENTAGE_DAMAGE_BOOST

-   subtype: -1 - any damage (not used), 0 - melee damage (offence), 1 -
    ranged damage (archery)

### GENERAL_ATTACK_REDUCTION

eg. while stoned or blinded - in %

-   subtype: -1 - any damage, 0 - melee damage, 1 - ranged damage

### DEFENSIVE_STANCE

WoG ability.

-   val - bonus to defense while defending

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

-   val: number of units gained per enemy killed
-   subtype: 0 - gained units survive after battle, 1 - they do not

### TRANSMUTATION

WoG ability. % chance to transform attacked unit to other type

-   val: chance to trigger in %
-   subtype: 0 - resurrection based on HP, 1 - based on unit count
-   addInfo: additional info - target creature ID (attacker default)

### SUMMON_GUARDIANS

WoG ability. Summon guardians surrounding desired stack

-   val - amount in % of stack count
-   subtype = creature ID

### RANGED_RETALIATION

Allows shooters to perform ranged retaliation

-   no additional parameters

### BLOCKS_RANGED_RETALIATION

Disallows ranged retaliation for shooter unit, BLOCKS_RETALIATION bonus
is for melee retaliation only

-   no additional parameters

### WIDE_BREATH

Dragon breath affecting multiple nearby hexes

-   no additional parameters

### FIRST_STRIKE

First counterattack, then attack if possible

-   no additional parameters

### SHOOTS_ALL_ADJACENT

H4 Cyclops-like shoot (attacks all hexes neighboring with target)
without spell-like mechanics

-   no additional parameters

### DESTRUCTION

Kills extra units after hit

-   subtype: 0 - kill percentage of units, 1 - kill amount
-   val: chance in percent to trigger
-   addInfo: amount/percentage to kill

### LIMITED_SHOOTING_RANGE

Limits shooting range and/or changes long range penalty

-   val: max shooting range in hexes
-   addInfo: optional - sets range for "broken arrow" half damage
    mechanic - default is 10

## Special abilities

### CATAPULT

-   subtype: ability to use when catapulting (usually it contains ballistics parameters, ability for standard catapult and affected by ballistics is core:spell.catapultShot)

### CATAPULT_EXTRA_SHOTS

-   subtype: ability to be affected. Ability of CATAPULT bonus should match. Used for ballistics secondary skill with subtype of core:spell.catapultShot.
-   val: ability level to be used with catapult. Additional shots configured in ability level, not here.

### MANUAL_CONTROL

-   manually control warmachine with
-   id = subtype
-   val = chance

### CHANGES_SPELL_COST_FOR_ALLY

eg. mage

-   value - reduction in mana points

### CHANGES_SPELL_COST_FOR_ENEMY

eg. Silver Pegasus

-   value - mana points penalty

### SPELL_RESISTANCE_AURA

eg. unicorns

-   value - resistance bonus in % for adjacent creatures

### HP_REGENERATION

creature regenerates val HP every new round

-   val: amount of HP regeneration per round

### MANA_DRAIN

value - spell points per turn

### MANA_CHANNELING

eg. familiar

-   value in %

### LIFE_DRAIN

-   val - precentage of life drained

### DOUBLE_DAMAGE_CHANCE

eg. dread knight

-   value in %

### FEAR

### HEALER

First aid tent

-   subtype: ability used for healing.

### FIRE_SHIELD

### MAGIC_MIRROR

-   value - chance of redirecting in %

### ACID_BREATH

-   val - damage per creature after attack,
-   additional info - chance in percent

### DEATH_STARE

-   subtype: 0 - gorgon, 1 - commander
-   val: if subtype is 0, its the (average) percentage of killed
    creatures related to size of attacking stack

### SPECIAL_CRYSTAL_GENERATION

Crystal dragon crystal generation

### NO_SPELLCAST_BY_DEFAULT

Spellcast will not be default attack option for this creature

## Creature spellcasting and activated abilities

### SPELLCASTER

Creature gain activated ability to cast a spell. Example: Archangel,
Faerie Dragon

-   subtype - spell id
-   value - level of school
-   additional info - weighted chance

use SPECIFIC_SPELL_POWER, CREATURE_SPELL_POWER or CREATURE_ENCHANT_POWER
for calculating the power (since 1.2 those bonuses can be used for
calculating CATAPULT and HEALER bonuses)

### ENCHANTER

for Enchanter spells

-   val - skill level
-   subtype - spell id
-   additionalInfo - cooldown (number of turns)

### RANDOM_SPELLCASTER

eg. master genie

-   val - spell mastery level

### SPELL_AFTER_ATTACK

-   subtype - spell id
-   value - chance %
-   additional info - \[X, Y\]
-   X - spell level
-   Y = 0 - all attacks, 1 - shot only, 2 - melee only

### SPELL_BEFORE_ATTACK

-   subtype - spell id
-   value - chance %
-   additional info - \[X, Y\]
-   X - spell level
-   Y = 0 - all attacks, 1 - shot only, 2 - melee only

### CASTS

how many times creature can cast activated spell

### SPECIFIC_SPELL_POWER

-   value used for Thunderbolt and Resurrection casted by units, also
    for Healing secondary skill (for core:spell.firstAid used by First
    Aid tent)
-   subtype - spell id

### CREATURE_SPELL_POWER

-   value per unit, divided by 100 (so faerie Dragons have 500)

### CREATURE_ENCHANT_POWER

total duration of spells casted by creature

### REBIRTH

-   val - percent of total stack HP restored
-   subtype = 0 - regular, 1 - at least one unit (sacred Phoenix)

### ENCHANTED

Stack is permanently enchanted with spell subID of skill level = val, if val > 3 then spell is mass and has level of val-3. Enchantment is refreshed every turn.

## Spell immunities

### LEVEL_SPELL_IMMUNITY

creature is immune to all spell with level below or equal to value of this bonus

### MAGIC_RESISTANCE

-   value - percent

### SPELL_DAMAGE_REDUCTION

eg. golems

-   value - reduction in %
-   subtype - spell school; -1 - all, 0 - air, 1 - fire, 2 - water, 3 -
    earth

### MORE_DAMAGE_FROM_SPELL

-   value - damage increase in %
-   subtype - spell id

### FIRE_IMMUNITY,WATER_IMMUNITY, EARTH_IMMUNITY, AIR_IMMUNITY

-   subtype 0 - all, 1 - all except positive, 2 - only damage spells

### MIND_IMMUNITY

Creature is immune to all mind spells.

### SPELL_IMMUNITY

-   subtype - spell id
-   ainfo - 0 - normal, 1 - absolute

### RECEPTIVE

WoG ability. Creature accepts all friendly spells even though it would
be normally immune to it.

# Spell effects

### POISON

-   val - max health penalty from poison possible

### SLAYER

-   value - spell level

### BIND_EFFECT

doesn't do anything particular, works as a marker

### FORGETFULL

forgetfulness spell effect

-   value - level

### NOT_ACTIVE

### ALWAYS_MINIMUM_DAMAGE

unit does its minimum damage from range

-   subtype: -1 - any attack, 0 - melee, 1 - ranged
-   value: additional damage penalty (it'll subtracted from dmg)
-   additional info - multiplicative anti-bonus for dmg in % \[eg 20
    means that creature will inflict 80% of normal minimal dmg\]

### ALWAYS_MAXIMUM_DAMAGE

eg. bless effect

-   subtype: -1 - any attack, 0 - melee, 1 - ranged
-   value: additional damage
-   additional info - multiplicative bonus for dmg in %

### ATTACKS_NEAREST_CREATURE

while in berserk

### IN_FRENZY

-   value - level

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
