/*
 * ObjProperty.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../constants/VariantIdentifier.h"
#include "../constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN

enum class ObjProperty : int8_t
{
	INVALID,
	OWNER,
	BLOCKVIS,
	PRIMARY_STACK_COUNT,
	VISITORS,
	VISITED,
	ID,
	AVAILABLE_CREATURE,
	MONSTER_COUNT,
	MONSTER_POWER,
	MONSTER_EXP,
	MONSTER_RESTORE_TYPE,
	MONSTER_REFUSED_JOIN,

	//town-specific
	STRUCTURE_ADD_VISITING_HERO,
	STRUCTURE_CLEAR_VISITORS,
	STRUCTURE_ADD_GARRISONED_HERO, //changing buildings state
	BONUS_VALUE_FIRST,
	BONUS_VALUE_SECOND, //used in Rampart for special building that generates resources (storing resource type and quantity)

	SEERHUT_VISITED,
	SEERHUT_COMPLETE,
	OBELISK_VISITED,

	//creature-bank specific
	BANK_DAYCOUNTER,
	BANK_RESET,
	BANK_CLEAR,

	//object with reward
	REWARD_RANDOMIZE,
	REWARD_SELECT,
	REWARD_CLEARED
};

class NumericID : public StaticIdentifier<NumericID>
{
public:
	using StaticIdentifier<NumericID>::StaticIdentifier;

	static si32 decode(const std::string & identifier)
	{
		return std::stoi(identifier);
	}
	static std::string encode(const si32 index)
	{
		return std::to_string(index);
	}
};

using ObjPropertyID = VariantIdentifier<NumericID, MapObjectID, ObjectInstanceID, CreatureID, PlayerColor, TeamID>;

VCMI_LIB_NAMESPACE_END
