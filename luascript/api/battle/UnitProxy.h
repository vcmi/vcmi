/*
 * UnitProxy.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/scripting/Service.h>
#include "../../../lib/battle/Unit.h"
#include <vcmi/spells/Spell.h>

#include "../../LuaWrapper.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{
namespace battle
{
using ::battle::IUnitInfo;
using ::battle::Unit;

class UnitProxy : public RawPointerWrapper<const Unit, UnitProxy>
{
public:
	using Wrapper = RawPointerWrapper<const Unit, UnitProxy>;
	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;

	static void heal(Unit *, int64_t & amount, EHealLevel level, EHealPower power);
	static const Creature * getCreature(const Unit *);
	static int32_t getBaseAmount(const Unit *);
	static BattleHexArray getHexes(const Unit *);
	static bool hasAbsoluteImmunity(const Unit * unit, const spells::Spell * spell);
};

}
}
}

VCMI_LIB_NAMESPACE_END
