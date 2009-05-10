#ifndef __STACK_FEATURE_H__
#define __STACK_FEATURE_H__

struct StackFeature
{
	//general list of stack abilities and effects
	enum ECombatFeatures
	{
		NO_TYPE,
		DOUBLE_WIDE, FLYING, SHOOTER, CHARGE_IMMUNITY, ADDITIONAL_ATTACK, UNLIMITED_RETAILATIONS,
		NO_MELEE_PENALTY, JOUSTING /*for champions*/, 
		RAISING_MORALE, HATE /*eg. angels hate devils*/, 
		KING1,
		KING2, KING3, MAGIC_RESISTANCE /*in %*/, 
		CHANGES_SPELL_COST /*in mana points, eg. pegasus or mage*/,
		SPELL_AFTER_ATTACK /* subtype - spell id, value - spell level, aditional info - chance in %; eg. dendroids*/,
		SPELL_RESISTANCE_AURA /*eg. unicorns, value - resistance bonus in % for adjacent creatures*/, 
		LEVEL_SPELL_IMMUNITY /*creature is immune to all spell with level below value of this bonus*/, 
		TWO_HEX_ATTACK_BREATH /*eg. dragons*/, 
		SPELL_DAMAGE_REDUCTION /*eg. golems; value - reduction in %, subtype - spell school; -1 - all, 0 - air, 1 - fire, 2 - water, 3 - earth*/, 
		NO_WALL_PENALTY, NON_LIVING /*eg. gargoyle*/, 
		RANDOM_SPELLCASTER, BLOCKS_RETAILATION /*eg. naga*/, 
		SPELL_IMMUNITY /*value - spell id*/, 
		MANA_CHANNELING /*in %, eg. familiar*/, 
		SPELL_LIKE_ATTACK /*value - spell id; range is taken from spell, but damage from creature; eg. magog*/, 
		THREE_HEADED_ATTACK /*eg. cerberus*/, 
		DEAMON_SUMMONING /*pit lord*/, 
		FIRE_IMMUNITY, FIRE_SHIELD, ENEMY_MORALE_DECREASING, ENEMY_LUCK_DECREASING, UNDEAD, REGENERATION, MANA_DRAIN, LIFE_DRAIN, 
		DOUBLE_DAMAGE_CHANCE /*in %, eg. dread knight*/, 
		RETURN_AFTER_STRIKE, SELF_MORALE /*eg. minotaur*/, 
		SPELLCASTER /*value - spell id*/, CATAPULT, 
		ENEMY_DEFENCE_REDUCTION /*in %, eg. behemots*/, 
		GENERAL_DAMAGE_REDUCTION /*eg. while stoned or blinded - in %, subtype: -1 - any damage, 0 - melee damage, 1 - ranged damage*/, 
		ATTACKS_ALL_ADAJCENT /*eg. hydra*/, 
		MORE_DAMEGE_FROM_SPELL /*value - damage increase in %, subtype - spell id*/, 
		CASTS_SPELL_WHEN_KILLED /*similar to spell after attack*/, 
		FEAR, FEARLESS, NO_DISTANCE_PENALTY, NO_OBSTACLES_PENALTY, 
		SELF_LUCK /*halfling*/, 
		ATTACK_BONUS, DEFENCE_BONUS, SPEED_BONUS, HP_BONUS, ENCHANTER, HEALER, SIEGE_WEAPON, LUCK_BONUS, MORALE_BONUS, HYPNOTIZED, ADDITIONAL_RETAILATION, 
		MAGIC_MIRROR /* value - chance of redirecting in %*/, 
		SUMMONED, ALWAYS_MINUMUM_DAMAGE /*unit does its minimum damage from range; -1 - any attack, 0 - melee, 1 - ranged*/, 
		ALWAYS_MAXIMUM_DAMAGE /*eg. bless effect, -1 - any attack, 0 - melee, 1 - ranged*/, 
		ATTACKS_NEAREST_CREATURE /*while in berserk*/, IN_FRENZY, 
		SLAYER /*value - level*/, 
		FORGETFULL /*forgetfullnes spell effect*/, 
		CLONED, NOT_ACTIVE
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
		SPELL_EFFECT
	};

	ECombatFeatures type;
	EDuration duration;
	ESource source;
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

#endif //__STACK_FEATURE_H__
