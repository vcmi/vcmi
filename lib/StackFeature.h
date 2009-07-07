#ifndef __STACK_FEATURE_H__
#define __STACK_FEATURE_H__

struct StackFeature
{
#define VCMI_CREATURE_ABILITY_LIST										\
	VCMI_CREATURE_ABILITY_NAME(NO_TYPE)									\
	VCMI_CREATURE_ABILITY_NAME(DOUBLE_WIDE)								\
	VCMI_CREATURE_ABILITY_NAME(FLYING)									\
	VCMI_CREATURE_ABILITY_NAME(SHOOTER)									\
	VCMI_CREATURE_ABILITY_NAME(CHARGE_IMMUNITY)							\
	VCMI_CREATURE_ABILITY_NAME(ADDITIONAL_ATTACK)						\
	VCMI_CREATURE_ABILITY_NAME(UNLIMITED_RETAILATIONS)					\
	VCMI_CREATURE_ABILITY_NAME(NO_MELEE_PENALTY)						\
	VCMI_CREATURE_ABILITY_NAME(JOUSTING) /*for champions*/				\
	VCMI_CREATURE_ABILITY_NAME(RAISING_MORALE) /*value - how much raises*/ \
	VCMI_CREATURE_ABILITY_NAME(HATE) /*eg. angels hate devils, subtype - ID of hated creature*/ \
	VCMI_CREATURE_ABILITY_NAME(KING1)									\
	VCMI_CREATURE_ABILITY_NAME(KING2)									\
	VCMI_CREATURE_ABILITY_NAME(KING3)									\
	VCMI_CREATURE_ABILITY_NAME(MAGIC_RESISTANCE) /*in % (value)*/		\
	VCMI_CREATURE_ABILITY_NAME(CHANGES_SPELL_COST_FOR_ALLY) /*in mana points (value) , eg. mage*/ \
	VCMI_CREATURE_ABILITY_NAME(CHANGES_SPELL_COST_FOR_ENEMY) /*in mana points (value) , eg. pegasus */ \
	VCMI_CREATURE_ABILITY_NAME(SPELL_AFTER_ATTACK) /* subtype - spell id, value - spell level, (aditional info)%100 - chance in %; eg. dendroids, (additional info)/100 -> [0 - all attacks, 1 - shot only, 2 - melee only*/ \
	VCMI_CREATURE_ABILITY_NAME(SPELL_RESISTANCE_AURA) /*eg. unicorns, value - resistance bonus in % for adjacent creatures*/ \
	VCMI_CREATURE_ABILITY_NAME(LEVEL_SPELL_IMMUNITY) /*creature is immune to all spell with level below or equal to value of this bonus*/ \
	VCMI_CREATURE_ABILITY_NAME(TWO_HEX_ATTACK_BREATH) /*eg. dragons*/	\
	VCMI_CREATURE_ABILITY_NAME(SPELL_DAMAGE_REDUCTION) /*eg. golems; value - reduction in %, subtype - spell school; -1 - all, 0 - air, 1 - fire, 2 - water, 3 - earth*/ \
	VCMI_CREATURE_ABILITY_NAME(NO_WALL_PENALTY)							\
	VCMI_CREATURE_ABILITY_NAME(NON_LIVING) /*eg. gargoyle*/				\
	VCMI_CREATURE_ABILITY_NAME(RANDOM_GENIE_SPELLCASTER) /*eg. master genie*/ \
	VCMI_CREATURE_ABILITY_NAME(BLOCKS_RETAILATION) /*eg. naga*/			\
	VCMI_CREATURE_ABILITY_NAME(SPELL_IMMUNITY) /*subid - spell id*/		\
	VCMI_CREATURE_ABILITY_NAME(MANA_CHANNELING) /*value in %, eg. familiar*/ \
	VCMI_CREATURE_ABILITY_NAME(SPELL_LIKE_ATTACK /*value - spell id; range is taken from spell, but damage from creature; eg. magog*/) \
	VCMI_CREATURE_ABILITY_NAME(THREE_HEADED_ATTACK) /*eg. cerberus*/	\
	VCMI_CREATURE_ABILITY_NAME(DEAMON_SUMMONING) /*pit lord*/			\
	VCMI_CREATURE_ABILITY_NAME(FIRE_IMMUNITY)							\
	VCMI_CREATURE_ABILITY_NAME(FIRE_SHIELD)								\
	VCMI_CREATURE_ABILITY_NAME(ENEMY_MORALE_DECREASING) /*value - how much it decreases*/ \
	VCMI_CREATURE_ABILITY_NAME(ENEMY_LUCK_DECREASING)					\
	VCMI_CREATURE_ABILITY_NAME(UNDEAD)									\
	VCMI_CREATURE_ABILITY_NAME(REGENERATION)							\
	VCMI_CREATURE_ABILITY_NAME(MANA_DRAIN) /*value - spell points per turn*/ \
	VCMI_CREATURE_ABILITY_NAME(LIFE_DRAIN)								\
	VCMI_CREATURE_ABILITY_NAME(DOUBLE_DAMAGE_CHANCE) /*value in %, eg. dread knight*/ \
	VCMI_CREATURE_ABILITY_NAME(RETURN_AFTER_STRIKE)						\
	VCMI_CREATURE_ABILITY_NAME(SELF_MORALE) /*eg. minotaur*/			\
	VCMI_CREATURE_ABILITY_NAME(SPELLCASTER) /*subtype - spell id, value - level of school, additional info - spell power*/ \
	VCMI_CREATURE_ABILITY_NAME(CATAPULT)								\
	VCMI_CREATURE_ABILITY_NAME(ENEMY_DEFENCE_REDUCTION) /*in % (value) eg. behemots*/ \
	VCMI_CREATURE_ABILITY_NAME(GENERAL_DAMAGE_REDUCTION) /*eg. while stoned or blinded - in %, subtype: -1 - any damage, 0 - melee damage, 1 - ranged damage*/ \
	VCMI_CREATURE_ABILITY_NAME(ATTACKS_ALL_ADAJCENT) /*eg. hydra*/		\
	VCMI_CREATURE_ABILITY_NAME(MORE_DAMEGE_FROM_SPELL) /*value - damage increase in %, subtype - spell id*/ \
	VCMI_CREATURE_ABILITY_NAME(CASTS_SPELL_WHEN_KILLED) /*similar to spell after attack*/ \
	VCMI_CREATURE_ABILITY_NAME(FEAR)									\
	VCMI_CREATURE_ABILITY_NAME(FEARLESS)								\
	VCMI_CREATURE_ABILITY_NAME(NO_DISTANCE_PENALTY)						\
	VCMI_CREATURE_ABILITY_NAME(NO_OBSTACLES_PENALTY)					\
	VCMI_CREATURE_ABILITY_NAME(SELF_LUCK) /*halfling*/					\
	VCMI_CREATURE_ABILITY_NAME(ATTACK_BONUS) /*subtype: -1 - any attack, 0 - melee, 1 - ranged*/ \
	VCMI_CREATURE_ABILITY_NAME(DEFENCE_BONUS) /*subtype: -1 - any attack, 0 - melee, 1 - ranged*/ \
	VCMI_CREATURE_ABILITY_NAME(SPEED_BONUS) /*additional info - percent of speed bonus applied after direct bonuses; >0 - added, <0 - substracted to this part*/ \
	VCMI_CREATURE_ABILITY_NAME(HP_BONUS)								\
	VCMI_CREATURE_ABILITY_NAME(ENCHANTER)								\
	VCMI_CREATURE_ABILITY_NAME(HEALER)									\
	VCMI_CREATURE_ABILITY_NAME(SIEGE_WEAPON)							\
	VCMI_CREATURE_ABILITY_NAME(LUCK_BONUS)								\
	VCMI_CREATURE_ABILITY_NAME(MORALE_BONUS)							\
	VCMI_CREATURE_ABILITY_NAME(HYPNOTIZED)								\
	VCMI_CREATURE_ABILITY_NAME(ADDITIONAL_RETAILATION) /*value - number of additional retailations*/ \
	VCMI_CREATURE_ABILITY_NAME(MAGIC_MIRROR) /* value - chance of redirecting in %*/ \
	VCMI_CREATURE_ABILITY_NAME(SUMMONED)								\
	VCMI_CREATURE_ABILITY_NAME(ALWAYS_MINUMUM_DAMAGE) /*unit does its minimum damage from range; subtype: -1 - any attack, 0 - melee, 1 - ranged, value: additional damage, additional info - multiplicative anti-bonus for dmg in % [eg 20 means that creature will inflict 80% of normal dmg]*/ \
	VCMI_CREATURE_ABILITY_NAME(ALWAYS_MAXIMUM_DAMAGE) /*eg. bless effect, subtype: -1 - any attack, 0 - melee, 1 - ranged, value: additional damage, additional info - multiplicative bonus for dmg in %*/ \
	VCMI_CREATURE_ABILITY_NAME(ATTACKS_NEAREST_CREATURE) /*while in berserk*/ \
	VCMI_CREATURE_ABILITY_NAME(IN_FRENZY) /*value - level*/				\
	VCMI_CREATURE_ABILITY_NAME(SLAYER) /*value - level*/				\
	VCMI_CREATURE_ABILITY_NAME(FORGETFULL) /*forgetfullnes spell effect, value - level*/ \
	VCMI_CREATURE_ABILITY_NAME(CLONED)									\
	VCMI_CREATURE_ABILITY_NAME(NOT_ACTIVE)
	
	//general list of stack abilities and effects
	enum ECombatFeatures
	{
#define VCMI_CREATURE_ABILITY_NAME(x) x,
		VCMI_CREATURE_ABILITY_LIST
#undef VCMI_CREATURE_ABILITY_NAME
	};

	enum EDuration
	{
		WHOLE_BATTLE,
		N_TURNS,
		UNITL_BEING_ATTACKED,/*removed after attack and counterattacks are performed*/
		UNTIL_ATTACK /*removed after attack and counterattacks are performed*/
	};

	enum ESource
	{
		CREATURE_ABILITY,
		BONUS_FROM_HERO,
		SPELL_EFFECT,
		OTHER_SOURCE /*eg. bonus from terrain if native*/
	};

	ui8 type;//ECombatFeatures
	ui8 duration;//EDuration
	ui8 source;//ESource
	ui16 turnsRemain; //if duration is N_TURNS it describes how long the effect will last
	si16 subtype; //subtype of bonus/feature
	si32 value;
	si32 additionalInfo;

	inline bool operator == (const ECombatFeatures & cf) const
	{
		return type == cf;
	}
	StackFeature() : type(NO_TYPE)
	{}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & type & duration & source & turnsRemain & subtype & value & additionalInfo;
	}
};

//generates StackFeature from given data
inline StackFeature makeFeature(StackFeature::ECombatFeatures type, StackFeature::EDuration duration, si16 subtype, si32 value, StackFeature::ESource source, ui16 turnsRemain = 0, si32 additionalInfo = 0)
{
	StackFeature sf;
	sf.type = type;
	sf.duration = duration;
	sf.source = source;
	sf.turnsRemain = turnsRemain;
	sf.subtype = subtype;
	sf.value = value;
	sf.additionalInfo = additionalInfo;

	return sf;
}

inline StackFeature makeCreatureAbility(StackFeature::ECombatFeatures type, si32 value, si16 subtype = 0, si32 additionalInfo = 0)
{
	return makeFeature(type, StackFeature::WHOLE_BATTLE, subtype, value, StackFeature::CREATURE_ABILITY, 0, additionalInfo);
}

#endif //__STACK_FEATURE_H__
