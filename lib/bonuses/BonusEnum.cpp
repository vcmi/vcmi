/*
 * BonusEnum.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */


#include "StdInc.h"

#include "BonusEnum.h"

VCMI_LIB_NAMESPACE_BEGIN

#define BONUS_NAME(x) { #x, BonusType::x },
	const std::map<std::string, BonusType> bonusNameMap = {
		BONUS_LIST
	};
#undef BONUS_NAME

#define BONUS_VALUE(x) { #x, BonusValueType::x },
	const std::map<std::string, BonusValueType> bonusValueMap = { BONUS_VALUE_LIST };
#undef BONUS_VALUE

#define BONUS_SOURCE(x) { #x, BonusSource::x },
	const std::map<std::string, BonusSource> bonusSourceMap = { BONUS_SOURCE_LIST };
#undef BONUS_SOURCE

#define BONUS_ITEM(x) { #x, BonusDuration::x },
const std::map<std::string, BonusDuration> bonusDurationMap =
{
	BONUS_ITEM(PERMANENT)
	BONUS_ITEM(ONE_BATTLE)
	BONUS_ITEM(ONE_DAY)
	BONUS_ITEM(ONE_WEEK)
	BONUS_ITEM(N_TURNS)
	BONUS_ITEM(N_DAYS)
	BONUS_ITEM(UNTIL_BEING_ATTACKED)
	BONUS_ITEM(UNTIL_ATTACK)
	BONUS_ITEM(STACK_GETS_TURN)
	BONUS_ITEM(COMMANDER_KILLED)
	{ "UNITL_BEING_ATTACKED", BonusDuration::UNTIL_BEING_ATTACKED }//typo, but used in some mods
};
#undef BONUS_ITEM

#define BONUS_ITEM(x) { #x, BonusLimitEffect::x },
const std::map<std::string, BonusLimitEffect> bonusLimitEffect =
{
	BONUS_ITEM(NO_LIMIT)
	BONUS_ITEM(ONLY_DISTANCE_FIGHT)
	BONUS_ITEM(ONLY_MELEE_FIGHT)
};
#undef BONUS_ITEM

VCMI_LIB_NAMESPACE_END