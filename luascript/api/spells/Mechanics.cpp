/*
 * Mechanics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Mechanics.h"

#include "../Registry.h"
#include "../../LuaStack.h"
#include "../../LuaCallWrapper.h"

#include "../../../lib/battle/CBattleInfoCallback.h"
#include "../../../lib/spells/Problem.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
	namespace api
	{
		using ::spells::Mechanics;

		VCMI_REGISTER_CORE_SCRIPT_API(SpellsMechanicsProxy, "battle.SpellMechanics");

		const std::vector<SpellsMechanicsProxy::CustomRegType> SpellsMechanicsProxy::REGISTER_CUSTOM =
		{
			{"isPositive", LuaMethodWrapper<Mechanics, decltype(&Mechanics::isPositiveSpell), &Mechanics::isPositiveSpell>::invoke, false},
			{"isNegative", LuaMethodWrapper<Mechanics, decltype(&Mechanics::isNegativeSpell), &Mechanics::isNegativeSpell>::invoke, false},

			{"getEffectLevel", LuaMethodWrapper<Mechanics, decltype(&Mechanics::getEffectLevel), &Mechanics::getEffectLevel>::invoke, false},
			{"getRangeLevel", LuaMethodWrapper<Mechanics, decltype(&Mechanics::getRangeLevel), &Mechanics::getRangeLevel>::invoke, false},
			{"getEffectPower", LuaMethodWrapper<Mechanics, decltype(&Mechanics::getEffectPower), &Mechanics::getEffectPower>::invoke, false},
			{"getEffectDuration", LuaMethodWrapper<Mechanics, decltype(&Mechanics::getEffectDuration), &Mechanics::getEffectDuration>::invoke, false},
			{"getEffectValue", LuaMethodWrapper<Mechanics, decltype(&Mechanics::getEffectValue), &Mechanics::getEffectValue>::invoke, false},
			{"getCasterColor", LuaMethodWrapper<Mechanics, decltype(&Mechanics::getCasterColor), &Mechanics::getCasterColor>::invoke, false},
			{"getCasterSide", LuaMethodWrapper<Mechanics, decltype(&Mechanics::getCasterSide), &Mechanics::getCasterSide>::invoke, false},
			{"getHeroCaster", LuaMethodWrapper<Mechanics, decltype(&Mechanics::getHeroCaster), &Mechanics::getHeroCaster>::invoke, false},
			{"getBattle", LuaMethodWrapper<Mechanics, decltype(&Mechanics::battle), &Mechanics::battle>::invoke, false},
			{"getBattleID", LuaMethodWrapper<Mechanics, decltype(&Mechanics::getBattleID), &Mechanics::getBattleID>::invoke, false},
			{"calculateRawEffectValue", LuaMethodWrapper<Mechanics, decltype(&Mechanics::calculateRawEffectValue), &Mechanics::calculateRawEffectValue>::invoke, false},
			{"applySpecificSpellBonus", LuaMethodWrapper<Mechanics, decltype(&Mechanics::applySpecificSpellBonus), &Mechanics::applySpecificSpellBonus>::invoke, false},
		};
	}
}

VCMI_LIB_NAMESPACE_END
