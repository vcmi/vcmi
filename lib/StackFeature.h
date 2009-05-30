#ifndef __STACK_FEATURE_H__
#define __STACK_FEATURE_H__

struct StackFeature
{
	//general list of stack abilities and effects
	enum ECombatFeatures
	{
		NO_TYPE,
		DOUBLE_WIDE, FLYING, SHOOTER, CHARGE_IMMUNITY, ADDITIONAL_ATTACK, UNLIMITED_RETAILATIONS,
		NO_MELEE_PENALTY,
		JOUSTING /*for champions*/, 
		RAISING_MORALE /*value - how much raises*/,
		HATE /*eg. angels hate devils, subtype - ID of hated creature*/, 
		KING1,
		KING2, KING3, MAGIC_RESISTANCE /*in % (value)*/, 
		CHANGES_SPELL_COST_FOR_ALLY /*in mana points (value) , eg. mage*/,
		CHANGES_SPELL_COST_FOR_ENEMY /*in mana points (value) , eg. pegasus */,
		SPELL_AFTER_ATTACK /* subtype - spell id, value - spell level, (aditional info)%100 - chance in %; eg. dendroids, (additional info)/100 -> [0 - all attacks, 1 - shot only, 2 - melee only*/,
		SPELL_RESISTANCE_AURA /*eg. unicorns, value - resistance bonus in % for adjacent creatures*/, 
		LEVEL_SPELL_IMMUNITY /*creature is immune to all spell with level below or equal to value of this bonus*/, 
		TWO_HEX_ATTACK_BREATH /*eg. dragons*/, 
		SPELL_DAMAGE_REDUCTION /*eg. golems; value - reduction in %, subtype - spell school; -1 - all, 0 - air, 1 - fire, 2 - water, 3 - earth*/, 
		NO_WALL_PENALTY, NON_LIVING /*eg. gargoyle*/, 
		RANDOM_GENIE_SPELLCASTER /*eg. master genie*/,
		BLOCKS_RETAILATION /*eg. naga*/, 
		SPELL_IMMUNITY /*subid - spell id*/, 
		MANA_CHANNELING /*value in %, eg. familiar*/, 
		SPELL_LIKE_ATTACK /*value - spell id; range is taken from spell, but damage from creature; eg. magog*/, 
		THREE_HEADED_ATTACK /*eg. cerberus*/, 
		DEAMON_SUMMONING /*pit lord*/, 
		FIRE_IMMUNITY, FIRE_SHIELD,
		ENEMY_MORALE_DECREASING /*value - how much it decreases*/,
		ENEMY_LUCK_DECREASING, UNDEAD,
		REGENERATION, MANA_DRAIN /*value - spell points per turn*/,
		LIFE_DRAIN, 
		DOUBLE_DAMAGE_CHANCE /*value in %, eg. dread knight*/, 
		RETURN_AFTER_STRIKE, SELF_MORALE /*eg. minotaur*/, 
		SPELLCASTER /*subtype - spell id, value - level of school, additional info - spell power*/,
		CATAPULT, 
		ENEMY_DEFENCE_REDUCTION /*in % (value), eg. behemots*/, 
		GENERAL_DAMAGE_REDUCTION /*eg. while stoned or blinded - in %, subtype: -1 - any damage, 0 - melee damage, 1 - ranged damage*/, 
		ATTACKS_ALL_ADAJCENT /*eg. hydra*/, 
		MORE_DAMEGE_FROM_SPELL /*value - damage increase in %, subtype - spell id*/, 
		CASTS_SPELL_WHEN_KILLED /*similar to spell after attack*/, 
		FEAR, FEARLESS, NO_DISTANCE_PENALTY, NO_OBSTACLES_PENALTY, 
		SELF_LUCK /*halfling*/, 
		ATTACK_BONUS /*subtype: -1 - any attack, 0 - melee, 1 - ranged*/,
		DEFENCE_BONUS /*subtype: -1 - any attack, 0 - melee, 1 - ranged*/,
		SPEED_BONUS /*additional info - percent of speed bonus applied after direct bonuses; >0 - added, <0 - substracted to this part*/,
		HP_BONUS, ENCHANTER, HEALER, SIEGE_WEAPON, LUCK_BONUS, MORALE_BONUS, HYPNOTIZED,
		ADDITIONAL_RETAILATION /*value - number of additional retailations*/, 
		MAGIC_MIRROR /* value - chance of redirecting in %*/, 
		SUMMONED,
		ALWAYS_MINUMUM_DAMAGE /*unit does its minimum damage from range; subtype: -1 - any attack, 0 - melee, 1 - ranged, value: additional damage, additional info - multiplicative anti-bonus for dmg in % [eg 20 means that creature will inflict 80% of normal dmg]*/, 
		ALWAYS_MAXIMUM_DAMAGE /*eg. bless effect, subtype: -1 - any attack, 0 - melee, 1 - ranged, value: additional damage, additional info - multiplicative bonus for dmg in %*/, 
		ATTACKS_NEAREST_CREATURE /*while in berserk*/,
		IN_FRENZY /*value - level*/, 
		SLAYER /*value - level*/, 
		FORGETFULL /*forgetfullnes spell effect, value - level*/, 
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
